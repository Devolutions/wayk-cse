
#include <windows.h>
#include <strsafe.h>

#include <lizard/lizard.h>

#include <cse/cse_utils.h>
#include <cse/bundle.h>
#include <resource.h>

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
} BundleOptionalContentInfo;


static int ExtractBundle(
	const char* extractionPath,
	const char* waykDataPath,
	WaykBinariesBitness bitness,
	BundleOptionalContentInfo* contentInfo)
{
	int status = LZ_ERROR_BUNDLE_EXTRACTION;

	contentInfo->hasBranding = false;
	contentInfo->hasPowerShellInitScript = false;

	WaykCseBundle* bundle = WaykCseBundle_Open();

	if (WaykCseBundle_ExtractWaykBinaries(bundle, extractionPath, bitness) != WAYK_CSE_BUNDLE_OK)
	{
		// Hard failure; Nothing to launch.
		goto cleanup;
	}

	if (WaykCseBundle_ExtractBrandingZip(bundle, waykDataPath) == WAYK_CSE_BUNDLE_OK)
	{
		contentInfo->hasBranding = true;
	}

	// Extract PS module only if configuration with init script is needed
	if (WaykCseBundle_ExtractPowerShellInitScript(bundle, extractionPath) == WAYK_CSE_BUNDLE_OK)
	{
		if (WaykCseBundle_ExtractPowerShelModule(bundle, extractionPath) != WAYK_CSE_BUNDLE_OK)
		{
			goto cleanup;
		}

		contentInfo->hasPowerShellInitScript = true;
	}

	status = LZ_OK;

cleanup:
	if (bundle)
		WaykCseBundle_Close(bundle);

	return status;
}

void FormatServiceName(char* buffer, unsigned int size, const char* productName)
{
	snprintf(buffer, size, "%s CSE Service", productName);
}

static int StartUnattendedService(
	const char* serviceBinary,
	const char* waykSystemFolderPath,
	const char* productName)
{
	int result;

	HANDLE cseInstanceMutex = 0;
	HANDLE serviceStartedEvent = 0;
	char text[MAX_TEXT_MESSAGE_SIZE];
	char serviceName[MAX_TEXT_MESSAGE_SIZE];

	cseInstanceMutex = OpenMutexW(MUTEX_ALL_ACCESS, false, CSE_INSTANCE_MUTEX_NAME_W);
	if (!cseInstanceMutex)
	{
		result = LZ_ERROR_UNEXPECTED;
		goto cleanup;
	}

	serviceStartedEvent = OpenEventW(EVENT_MODIFY_STATE, false, CSE_SERVICE_LAUNCHER_READY_EVENT_W);
	if (!serviceStartedEvent)
	{
		result = LZ_ERROR_UNEXPECTED;
		goto cleanup;
	}

	result = LzIsServiceLaunched("WaykNowService");
	if (result == FALSE || result == LZ_ERROR_OPEN_SERVICE)
	{
		result = LZ_OK;
	}
	else if (result == TRUE)
	{
		snprintf(
			text,
			MAX_TEXT_MESSAGE_SIZE,
			"%s service can't be launched alongside with Wayk Unattended Service",
			productName);

		LzMessageBox(NULL,
					 text,
					 productName,
					 MB_OK | MB_ICONERROR
		);

		result = LZ_ERROR_CONFLICTING_SERVICE;
		goto cleanup;
	}
	else
	{
		// Keep LzIsServiceLaunched returned error in result
		goto cleanup;
	}


	FormatServiceName(serviceName, sizeof(serviceName), productName);


	result = LzInstallService(serviceName, serviceBinary);
	if (result != LZ_OK)
		goto cleanup;

	const char* args[2];
	args[0] = "--wayk-system-path";
	args[1] = waykSystemFolderPath;

	result = LzStartService(serviceName, 2, args);
	if (result != LZ_OK)
		goto cleanup;

	// Signal that service indeed started
	SetEvent(serviceStartedEvent);

	// Wait until main app ends
	WaitForSingleObjectEx(cseInstanceMutex, INFINITE, false);

	// Stop service return result is ignored -- we need to remove
	// service even if stop has been failed (service will be marked for removal)
	LzStopService(serviceName);

	result = LzRemoveService(serviceName);
	if (result != LZ_OK)
		goto cleanup;

	result = LZ_OK;

cleanup:
	if (cseInstanceMutex)
		CloseHandle(cseInstanceMutex);
	if (serviceStartedEvent)
		CloseHandle(serviceStartedEvent);

	return result;
}

int CseServiceLauncherMain()
{
	// args: cse.exe --start-service "Path/To/NowService.exe" "/Path/To/WAYK_NOW_SYSTEM" "productPath"
	if (__argc <= 4)
	{
		LzMessageBox(NULL,
					  "Invalid CSE service launcher arguments",
					  "Custom standalone executable",
					  MB_OK | MB_ICONERROR
		);
		return 1;
	}

	char unattendedServicePath[LZ_MAX_PATH];
	char unattendedSystemFolderPath[LZ_MAX_PATH];
	char productName[LZ_MAX_PATH];
	LzUnicode_UTF16toUTF8(__wargv[2], -1, (uint8_t*) unattendedServicePath, LZ_MAX_PATH);
	LzUnicode_UTF16toUTF8(__wargv[3], -1, (uint8_t*) unattendedSystemFolderPath, LZ_MAX_PATH);
	LzUnicode_UTF16toUTF8(__wargv[4], -1, (uint8_t*) productName, LZ_MAX_PATH);

	return StartUnattendedService(unattendedServicePath, unattendedSystemFolderPath,productName);
}

void StartServiceLauncher(char* extractionPath, char* systemPath, char* productName)
{
	char cwd[LZ_MAX_PATH];
	char csePath[LZ_MAX_PATH];
	char servicePath[LZ_MAX_PATH];
	char cseParams[LZ_MAX_PATH];

	SHELLEXECUTEINFOA shellExecuteInfo;

	ZeroMemory(&shellExecuteInfo, sizeof(SHELLEXECUTEINFOA));

	LzEnv_GetCwd(cwd, LZ_MAX_PATH);
	LzGetModuleFileName(NULL, csePath, sizeof(csePath));

	servicePath[0] = '\0';
	LzPathCchAppend(servicePath, sizeof(servicePath), extractionPath);
	LzPathCchAppend(servicePath, sizeof(servicePath), "NowService.exe");

	snprintf(
		cseParams,
		sizeof(cseParams),
		"--start-service \"%s\" \"%s\" \"%s\"",
		servicePath,
		systemPath,
		productName
	);

	shellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
	shellExecuteInfo.lpVerb = "runas";
	shellExecuteInfo.lpFile = csePath;
	shellExecuteInfo.lpDirectory = cwd;
	shellExecuteInfo.lpParameters = cseParams;

	if (LzShellExecuteEx(&shellExecuteInfo) != TRUE)
	{
		LzMessageBox(
			NULL,
			"Failed to start unattended service. Functionality will be limited.",
			productName,
			MB_OK | MB_ICONWARNING
		);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, WCHAR* lpCmdLine, int nCmdShow)
{
	int status;

	BOOL wow64;
	WaykBinariesBitness waykBinariesBitness;
	bool enableUnattendedService = false;
	BundleOptionalContentInfo bundleOptionalContentInfo;
	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	char text[2048];
	char cwd[LZ_MAX_PATH];
	char psModulePath[LZ_MAX_PATH];
	char psInitScriptPath[LZ_MAX_PATH];
	char serviceName[MAX_TEXT_MESSAGE_SIZE];

	void* fsRedir = NULL;

	char* commandLine = NULL;
	char* extractionPath = NULL;
	char* dataPath = NULL;
	char* systemPath = NULL;
	HANDLE cseStartedMutex = NULL;
	HANDLE serviceStartedEvent = NULL;
	char* enableUnattendedServiceOpt = NULL;
	char* productName = NULL;

	if ((__argc > 1) && (wcscmp(__wargv[1], L"--start-service") == 0))
	{
		return CseServiceLauncherMain();
	}

	wow64 = LzIsWow64();
	waykBinariesBitness = wow64 ? WAYK_BINARIES_BITNESS_X64 : WAYK_BINARIES_BITNESS_X86;

	ZeroMemory(&bundleOptionalContentInfo, sizeof(BundleOptionalContentInfo));

	extractionPath = GetWaykCsePathOption(IDS_WAYK_EXTRACTION_PATH);
	dataPath = GetWaykCsePathOption(IDS_WAYK_OPTION_DATA_PATH);
	systemPath = GetWaykCsePathOption(IDS_WAYK_OPTION_SYSTEM_PATH);
	enableUnattendedServiceOpt = GetWaykCseOption(IDS_WAYK_OPTION_UNATTENDED);
	productName = GetWaykCseOption(IDS_WAYK_PRODUCT_NAME);

	cseStartedMutex = CreateMutexW(NULL, true, CSE_INSTANCE_MUTEX_NAME_W);
	if (!cseStartedMutex || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		status = LZ_ERROR_MULTIPLE_CSE_INSTANCES;
		snprintf(text, sizeof(text), "%s application is already launched", productName);
		LzMessageBox(NULL, text, productName, MB_OK | MB_ICONERROR);
		goto cleanup;
	}

	serviceStartedEvent =
		CreateEventW(NULL, false, false, CSE_SERVICE_LAUNCHER_READY_EVENT_W);

	if (!extractionPath)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}
	if (!dataPath)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}
	if (!systemPath)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}
	if (!enableUnattendedServiceOpt)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}
	if (!productName)
	{
		status = LZ_ERROR_NOT_FOUND;
		goto cleanup;
	}

	enableUnattendedService = (strcmp(enableUnattendedServiceOpt, "1") == 0);

	if (!LzFile_Exists(extractionPath))
	{
		if (LzMkPath(extractionPath, 0) != LZ_OK)
		{
			status = LZ_ERROR_FILE;
			LzMessageBox(
				NULL,
				"Failed to create wayk extraction directory",
				"Wayk Now",
				MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	LzSetEnv("WAYK_DATA_PATH", dataPath);

	if (!LzFile_Exists(dataPath))
	{
		if (LzMkPath(dataPath, 0) != LZ_OK)
		{
			status = LZ_ERROR_FILE;
			LzMessageBox(
				NULL,
				"Failed to create wayk data directory",
				"Wayk Now",
				MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	LzSetEnv("WAYK_SYSTEM_PATH", systemPath);

	if (!LzFile_Exists(systemPath))
	{
		if (LzMkPath(systemPath, 0) != LZ_OK)
		{
			status = LZ_ERROR_FILE;
			LzMessageBox(
				NULL,
				"Failed to create wayk system directory",
				"Wayk Now",
				MB_OK | MB_ICONERROR);
			goto cleanup;
		}
	}

	status = ExtractBundle(
		extractionPath,
		dataPath,
		waykBinariesBitness,
		&bundleOptionalContentInfo);

	if (status != LZ_OK)
	{
		LzMessageBox(
			NULL,
			"Failed to extract the portable application",
			productName,
			MB_OK | MB_ICONERROR
		);
		goto cleanup;
	}

	// Run Power Shell init script before running unattended service
	if (bundleOptionalContentInfo.hasPowerShellInitScript)
	{
		psModulePath[0] = '\0';
		psInitScriptPath[0] = '\0';
		LzPathCchAppend(psModulePath, sizeof(psModulePath), extractionPath);
		LzPathCchAppend(psInitScriptPath, sizeof(psInitScriptPath), extractionPath);

		LzPathCchAppend(psModulePath, sizeof(psModulePath), "PowerShell\\Modules\\WaykNow");
		LzPathCchAppend(psInitScriptPath, sizeof(psInitScriptPath), "init.ps1");

		status = RunWaykNowInitScript(psModulePath, psInitScriptPath);
		if (status != LZ_OK)
		{
			LzMessageBox(
				NULL,
				"Failed to run initialization script",
				productName,
				MB_OK | MB_ICONWARNING
			);
		}
	}


	LzEnv_GetCwd(cwd, LZ_MAX_PATH);

	if (enableUnattendedService)
	{
		FormatServiceName(serviceName, sizeof(serviceName), productName);
		LzSetEnv("WAYK_UNATTENDED_SERVICE_NAME", serviceName);

		StartServiceLauncher(extractionPath, systemPath, productName);

		// Wait 10 seconds for service to start
		if (WaitForSingleObjectEx(
			serviceStartedEvent,
			START_UNATTENDED_SERVICE_TIMEOUT,
			false) != WAIT_OBJECT_0)
		{
			LzMessageBox(
				NULL,
				"Failed to start unattended service. Functionality will be limited.",
				productName,
				MB_OK | MB_ICONERROR
			);
		}
	}

	LzEnv_SetCwd(extractionPath);

	commandLine = LzGetCommandLine();

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
	startupInfo.cb = sizeof(STARTUPINFOA);

	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	if (wow64)
		pfnWow64DisableWow64FsRedirection(&fsRedir);

	if (!LzCreateProcess(
		"WaykNow.exe",
		commandLine,
		NULL,
		NULL,
		FALSE,
		0,
		0,
		extractionPath,
		&startupInfo,
		&processInfo))
	{
		status = LZ_ERROR_FAIL;
		LzMessageBox(
			NULL,
			"Wayk Now failed to launch",
			"Wayk Now",
			MB_OK | MB_ICONERROR
		);
	}

	if (wow64)
		pfnWow64RevertWow64FsRedirection(fsRedir);

	WaitForSingleObject(processInfo.hProcess, INFINITE);

	status = 0;

cleanup:
	if (processInfo.hThread)
		CloseHandle(processInfo.hThread);
	if (processInfo.hProcess)
		CloseHandle(processInfo.hProcess);

	if (commandLine)
		free(commandLine);

	if (extractionPath)
		free(extractionPath);
	if (dataPath)
		free(dataPath);
	if (systemPath)
		free(systemPath);
	if (enableUnattendedServiceOpt)
		free(enableUnattendedServiceOpt);
	if (productName)
		free(productName);

	if (cseStartedMutex)
		CloseHandle(cseStartedMutex);
	if (serviceStartedEvent)
		CloseHandle(serviceStartedEvent);

	return status;
}
