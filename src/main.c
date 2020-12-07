#include <windows.h>
#include <strsafe.h>

#include <lizard/lizard.h>

#include <cse/cse_utils.h>
#include <cse/install.h>
#include <cse/bundle.h>
#include <cse/log.h>
#include <cse/cse_options.h>

#define CSE_LOG_TAG "Cse"

#define _CSE_APP_ERROR_BASE (-0x10000000)
#define LZ_ERROR_BUNDLE_EXTRACTION (_CSE_APP_ERROR_BASE - 0)
#define LZ_ERROR_MULTIPLE_CSE_INSTANCES (_CSE_APP_ERROR_BASE - 1)

#define CSE_INSTANCE_MUTEX_NAME_W L"Global\\WaykNowCSEInstance"

typedef struct
{
	bool hasBranding;
	bool hasPowerShellInitScript;
	bool hasEmbeddedInstaller;
} BundleOptionalContentInfo;


static int ExtractBundle(
	const char* extractionPath,
	WaykBinariesBitness bitness,
	BundleOptionalContentInfo* contentInfo)
{
	int status = LZ_ERROR_BUNDLE_EXTRACTION;

	contentInfo->hasBranding = false;
	contentInfo->hasPowerShellInitScript = false;

	WaykCseBundle* bundle = WaykCseBundle_Open();

	if (WaykCseBundle_ExtractOptionsJson(bundle, extractionPath) != WAYK_CSE_BUNDLE_OK)
	{
		CSE_LOG_ERROR("Options json is not found inside CSE bundle");
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}

	if (WaykCseBundle_ExtractWaykNowExecutable(bundle, bitness, extractionPath) == WAYK_CSE_BUNDLE_OK)
	{
		CSE_LOG_DEBUG("Extracting executable %s", extractionPath);
		contentInfo->hasEmbeddedInstaller = true;
	}

	if (WaykCseBundle_ExtractWaykNowInstaller(bundle, bitness, extractionPath) == WAYK_CSE_BUNDLE_OK)
	{
		CSE_LOG_DEBUG("Extracting installer %s", extractionPath);
		contentInfo->hasEmbeddedInstaller = true;
	}

	if (WaykCseBundle_ExtractBrandingZip(bundle, extractionPath) == WAYK_CSE_BUNDLE_OK)
	{
		contentInfo->hasBranding = true;
	}

	if (WaykCseBundle_ExtractPowerShellInitScript(bundle, extractionPath) == WAYK_CSE_BUNDLE_OK)
	{
		contentInfo->hasPowerShellInitScript = true;
	}

	status = LZ_OK;

cleanup:
	if (bundle)
		WaykCseBundle_Close(bundle);

	return status;
}

static CseLogLevel GetLogLevel()
{
	char logLevelStr[16];

	// Log settings overwritten by env variable
	if (LzEnv_GetEnv("CSE_LOG", logLevelStr, 16) >= 0)
	{
		if (strcmp(logLevelStr, "trace") == 0)
		{
			return CSE_LOG_LEVEL_TRACE;
		}
		else if (strcmp(logLevelStr, "debug") == 0)
		{
			return CSE_LOG_LEVEL_DEBUG;
		}
		else if (strcmp(logLevelStr, "info") == 0)
		{
			return CSE_LOG_LEVEL_INFO;
		}
		else if (strcmp(logLevelStr, "warn") == 0)
		{
			return CSE_LOG_LEVEL_WARN;
		}
		else if (strcmp(logLevelStr, "error") == 0)
		{
			return CSE_LOG_LEVEL_ERROR;
		}
	}

	// Default settings
	#ifdef NDEBUG
		return CSE_LOG_LEVEL_INFO;
	#else
		return CSE_LOG_LEVEL_DEBUG;
	#endif
}

int main(int argc, char** argv)
{
	int status;
	WaykBinariesBitness waykBinariesBitness;
	BundleOptionalContentInfo bundleOptionalContentInfo;
	char tempFolderName[LZ_MAX_PATH];
	char psInitScriptPath[LZ_MAX_PATH];
	char extractionPath[LZ_MAX_PATH];
	char optionsPath[LZ_MAX_PATH];
	char waykNowBinaryPath[LZ_MAX_PATH];
	char msiPath[LZ_MAX_PATH];
	char brandingPath[LZ_MAX_PATH];
	char* productName = 0;
	char* waykNowInstallationDir = 0;
	HANDLE cseStartedMutex = 0;
	CseOptions* cseOptions = 0;
	CseInstall* cseInstall = 0;

	CseLog_Init(stderr, GetLogLevel());
	waykBinariesBitness = LzIsWow64() ? WAYK_BINARIES_BITNESS_X64 : WAYK_BINARIES_BITNESS_X86;

	if (!IsElevated())
	{
		CSE_LOG_WARN(
			"Cse is running from non-elevated environment, elevation prompt will be presented");
	}

	productName = GetProductName();
	if (!productName)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}

	CSE_LOG_INFO("Starting %s CSE deploy...", productName);

	cseStartedMutex = CreateMutexW(NULL, true, CSE_INSTANCE_MUTEX_NAME_W);
	if (!cseStartedMutex || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		status = LZ_ERROR_MULTIPLE_CSE_INSTANCES;
		CSE_LOG_ERROR("%s CSE is already launched", productName);
		goto cleanup;
	}

	extractionPath[0] = '\0';
	if (LzEnv_GetTempPath(extractionPath, LZ_MAX_PATH) <= 0)
	{
		CSE_LOG_ERROR("Failed to get temp path");
		status = LZ_ERROR_UNEXPECTED;
		goto cleanup;
	}

	sprintf_s(tempFolderName, LZ_MAX_PATH, "%s CSE", productName);
	if (LzPathCchAppend(extractionPath, LZ_MAX_PATH, tempFolderName) != LZ_OK)
	{
		CSE_LOG_ERROR("Failed to construct temp path for CSE extraction");
		status = LZ_ERROR_UNEXPECTED;
		goto cleanup;
	}

	if (!LzFile_Exists(extractionPath))
	{
		if (LzMkPath(extractionPath, 0) != LZ_OK)
		{
			status = LZ_ERROR_FILE;
			CSE_LOG_ERROR("Failed to create %s extraction directory", productName);
			goto cleanup;
		}
	}

	CSE_LOG_INFO("Extracting compressed CSE artifacts...");

	ZeroMemory(&bundleOptionalContentInfo, sizeof(BundleOptionalContentInfo));
	status = ExtractBundle(
		extractionPath,
		waykBinariesBitness,
		&bundleOptionalContentInfo);

	if (status != LZ_OK)
	{
		CSE_LOG_ERROR("Failed to extract %s resources", productName);
		goto cleanup;
	}

	CSE_LOG_INFO("Parsing CSE config..");

	cseOptions = CseOptions_New();
	if (!cseOptions)
	{
		status = LZ_ERROR_MEM;
		goto  cleanup;
	}

	optionsPath[0] = '\0';
	LzPathCchAppend(optionsPath, sizeof(optionsPath), extractionPath);
	LzPathCchAppend(optionsPath, sizeof(optionsPath), GetJsonOptionsFileName());
	if (CseOptions_LoadFromFile(cseOptions, optionsPath) != CSE_OPTIONS_OK)
	{
		CSE_LOG_ERROR("Failed to load JSON options");
		status = LZ_ERROR_FAIL;
		goto cleanup;
	}

	waykNowBinaryPath[0] = '\0';
	LzPathCchAppend(waykNowBinaryPath, sizeof(waykNowBinaryPath), extractionPath);
	LzPathCchAppend(waykNowBinaryPath, sizeof(waykNowBinaryPath), GetWaykNowBinaryFileName(waykBinariesBitness));

	if (bundleOptionalContentInfo.hasEmbeddedInstaller)
	{
		CSE_LOG_INFO("Preparing for embedded MSI isntall...");

		msiPath[0] = '\0';
		LzPathCchAppend(msiPath, sizeof(msiPath), extractionPath);
		LzPathCchAppend(msiPath, sizeof(msiPath), GetInstallerFileName(waykBinariesBitness));
		cseInstall = CseInstall_WithLocalMsi(waykNowBinaryPath, msiPath);
	}
	else
	{
		CSE_LOG_INFO("Preparing for online MSI download and installation");

		cseInstall = CseInstall_WithMsiDownload(waykNowBinaryPath);
	}

	if (!cseInstall)
	{
		CSE_LOG_ERROR("Failed to stat MSI arguments generation process");
		status = LZ_ERROR_FAIL;
		goto cleanup;
	}

	const char* enrollmentToken = CseOptions_GetEnrollmentToken(cseOptions);
	const char* enrollmentUrl = CseOptions_GetEnrollmentUrl(cseOptions);
	if (enrollmentToken || enrollmentUrl)
	{
		if (CseInstall_SetEnrollmentOptions(
			cseInstall,
			enrollmentUrl,
			enrollmentToken) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR("Failed to set enrollment info for MSI arguments");
			status = LZ_ERROR_FAIL;
			goto cleanup;
		}
	}

	if (bundleOptionalContentInfo.hasBranding)
	{
		brandingPath[0] = '\0';
		LzPathCchAppend(brandingPath, sizeof(brandingPath), extractionPath);
		LzPathCchAppend(brandingPath, sizeof(brandingPath), GetBrandingFileName());

		if (CseInstall_SetBrandingFile(cseInstall, brandingPath) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR("Failed to set branding file path for MSI arguments");
			status = LZ_ERROR_FAIL;
			goto cleanup;
		}
	}

	const char* requestedInstallDirectory = CseOptions_GetInstallDirectory(cseOptions);
	if (requestedInstallDirectory)
	{
		if (CseInstall_SetInstallDirectory(cseInstall, requestedInstallDirectory) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR("Failed to set install directory for MSI");
			status = LZ_ERROR_FAIL;
			goto cleanup;
		}
	}

	bool startAfterInstall = CseOptions_StartAfterInstall(cseOptions);
	
	bool createDesktopShortcut = CseOptions_CreateDesktopShortcut(cseOptions);
	if (!createDesktopShortcut)
	{
		if (CseInstall_DisableDesktopShortcut(cseInstall) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR("Failed to disable create desktop shortcut option for MSI");
			status = LZ_ERROR_FAIL;
			goto cleanup;
		}
	}

	bool createStartMenuShortcut = CseOptions_CreateStartMenuShortcut(cseOptions);
	if (!createStartMenuShortcut)
	{
		if (CseInstall_DisableStartMenuShortcut(cseInstall) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR("Failed to disable create start menu shortcut option for MSI");
			status = LZ_ERROR_FAIL;
			goto cleanup;
		}
	}

	WaykNowConfigOption* configOption = CseOptions_GetFirstMsiWaykNowConfigOption(cseOptions);
	while (configOption)
	{
		if (CseInstall_SetConfigOption(
			cseInstall,
			WaykNowConfigOption_GetKey(configOption),
			WaykNowConfigOption_GetValue(configOption)) != CSE_INSTALL_OK)
		{
			CSE_LOG_ERROR(
				"Failed to set %s config option for MSI",
				WaykNowConfigOption_GetKey(configOption));

			status = LZ_ERROR_FAIL;
			goto cleanup;
		}

		WaykNowConfigOption_Next(&configOption);
	}

	CSE_LOG_INFO("Starting MSI installation...");

	if (CseInstall_Run(cseInstall) != CSE_INSTALL_OK)
	{
		CSE_LOG_ERROR("Failed to execute MSI installation");
		status = LZ_ERROR_FAIL;
		goto cleanup;
	}

	CSE_LOG_INFO("Successfully deployed %s CSE!", productName);

cleanup:
	if (productName)
		free(productName);
	if (cseStartedMutex)
		CloseHandle(cseStartedMutex);
	if (cseOptions)
		CseOptions_Free(cseOptions);
	if (cseInstall)
		CseInstall_Free(cseInstall);

	if (status != LZ_OK)
	{
		CSE_LOG_ERROR("CSE deploy failed with code %d", status);
	}

	return status;
}
