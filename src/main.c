#include <windows.h>
#include <strsafe.h>

#include <lizard/lizard.h>

#include <cse/cse_utils.h>
#include <cse/bundle.h>
#include <cse/log.h>
#include <cse/cse_options.h>

#define MAX_TEXT_MESSAGE_SIZE 1024

#define START_UNATTENDED_SERVICE_TIMEOUT 15000 // 15s

#define _CSE_APP_ERROR_BASE (-0x10000000)
#define LZ_ERROR_BUNDLE_EXTRACTION (_CSE_APP_ERROR_BASE - 0)
#define LZ_ERROR_MULTIPLE_CSE_INSTANCES (_CSE_APP_ERROR_BASE - 1)
#define LZ_ERROR_CONFLICTING_SERVICE (_CSE_APP_ERROR_BASE - 2)

#define CSE_INSTANCE_MUTEX_NAME_W L"Global\\WaykNowCSEInstance"
#define CSE_SERVICE_LAUNCHER_READY_EVENT_W L"Global\\WaykNowCSEServiceLauncherReady"

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

	if (WaykCseBundle_ExtractWaykNowExecutable(bundle, bitness, extractionPath) != WAYK_CSE_BUNDLE_OK)
	{
		CSE_LOG_ERROR("Wayk Now binary with required bitness is not found inside CSE bundle");
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}

	if (WaykCseBundle_ExtractWaykNowInstaller(bundle, bitness, extractionPath) != WAYK_CSE_BUNDLE_OK)
	{
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

char* GetWaykInstallationDir()
{
	wchar_t* installPathW = 0;
	char* installPath = 0;
	DWORD installPathSize = LZ_MAX_PATH;
	LSTATUS keyOpenStatus = ERROR_PATH_NOT_FOUND;
	HKEY regKey;
	REGSAM regKeyAccess = KEY_READ;
	if (LzIsWow64())
	{
		regKeyAccess |= KEY_WOW64_64KEY;
	}

	keyOpenStatus = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Wayk\\WaykNow",
		0,
		regKeyAccess,
		&regKey);
	if (keyOpenStatus != ERROR_SUCCESS)
	{
		CSE_LOG_WARN("Failed to open Wayk Now registry key");
		goto cleanup;
	}

	installPathW = malloc(LZ_MAX_PATH);
	if (!installPathW)
	{
		CSE_LOG_WARN("Failed to allocate InstallPath buffer");
	}
	if (RegQueryValueExW(
		regKey,
		L"InstallDir",
		0,
		0,
		(LPBYTE)installPathW,
		&installPathSize) != ERROR_SUCCESS)
	{
		CSE_LOG_WARN("Failed to read InstallPath registry value");
	}
	else
	{
		installPath = LzUnicode_UTF16toUTF8_dup(installPathW);
		CSE_LOG_DEBUG("Wayk Now installation dir: %s", installPath);
	}

cleanup:
	if (keyOpenStatus == ERROR_SUCCESS)
	{
		RegCloseKey(regKey);
	}
	if (installPathW)
	{
		free(installPathW);
	}

	return installPath;
}

int main(int argc, char** argv)
{
	int status;
	BOOL wow64;
	WaykBinariesBitness waykBinariesBitness;
	BundleOptionalContentInfo bundleOptionalContentInfo;
	PROCESS_INFORMATION processInfo;
	char text[MAX_TEXT_MESSAGE_SIZE];
	char psInitScriptPath[LZ_MAX_PATH];
	char extractionPath[LZ_MAX_PATH];
	char optionsPath[LZ_MAX_PATH];
	char waykNowBinaryPath[LZ_MAX_PATH];
	char* productName = 0;
	char* waykNowInstallationDir = 0;
	HANDLE cseStartedMutex = 0;
	CseOptions* cseOptions = 0;

#ifdef NDEBUG
	CseLog_Init(stderr, CSE_LOG_LEVEL_INFO);
#else
	CseLog_Init(stderr, CSE_LOG_LEVEL_DEBUG);
#endif

	wow64 = LzIsWow64();
	waykBinariesBitness = wow64 ? WAYK_BINARIES_BITNESS_X64 : WAYK_BINARIES_BITNESS_X86;

	ZeroMemory(&bundleOptionalContentInfo, sizeof(BundleOptionalContentInfo));

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

	cseStartedMutex = CreateMutexW(NULL, true, CSE_INSTANCE_MUTEX_NAME_W);
	if (!cseStartedMutex || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		status = LZ_ERROR_MULTIPLE_CSE_INSTANCES;
		CSE_LOG_ERROR("%s CSE is already launched", productName);
		goto cleanup;
	}

	if (LzEnv_GetTempPath(extractionPath, LZ_MAX_PATH) != LZ_OK)
	{
		CSE_LOG_ERROR("Failed to get temp path");
		status = LZ_ERROR_UNEXPECTED;
		goto cleanup;
	}

	sprintf_s(text, MAX_TEXT_MESSAGE_SIZE, "%s CSE", productName);
	if (LzPathCchAppend(extractionPath, LZ_MAX_PATH, text) != LZ_OK)
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

	status = ExtractBundle(
		extractionPath,
		waykBinariesBitness,
		&bundleOptionalContentInfo);

	if (status != LZ_OK)
	{
		CSE_LOG_ERROR("Failed to extract %s resources", productName);
		goto cleanup;
	}

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


	// TODO: make MSI_options;
	// TODO: execute MSI;

	//WaitForSingleObject(processInfo.hProcess, INFINITE);


	waykNowInstallationDir = GetWaykInstallationDir();
	if (!waykNowInstallationDir)
	{
		CSE_LOG_ERROR("Failed to query %s installation dir", productName);
		goto cleanup;
	}

	// Run Power Shell after installation
	if (bundleOptionalContentInfo.hasPowerShellInitScript)
	{
		psInitScriptPath[0] = '\0';
		LzPathCchAppend(
			psInitScriptPath,
			sizeof(psInitScriptPath),
			extractionPath);
		LzPathCchAppend(
			psInitScriptPath,
			sizeof(psInitScriptPath),
			GetPowerShellInitScriptFileName());

		const char* modulePath = CseOptions_WaykNowPsModuleImportRequired(cseOptions)
			? GetPowerShellModulePath(waykNowInstallationDir)
			: 0;
		status = RunWaykNowInitScript(modulePath, psInitScriptPath);
		if (status != LZ_OK)
		{
			CSE_LOG_ERROR("Failed to run %s initialization script", productName);
			goto cleanup;
		}
	}

	status = RmDirRecursively(extractionPath);

cleanup:
	if (productName)
		free(productName);
	if (cseStartedMutex)
		CloseHandle(cseStartedMutex);
	if (cseOptions)
		CseOptions_Free(cseOptions);

	return status;
}
