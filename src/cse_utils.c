#include <cse/cse_utils.h>
#include <windows.h>
#include <lizard/lizard.h>
#include <resource.h>
#include <cse/log.h>

#define CSE_LOG_TAG "CseUtils"

#define MAX_COMMAND_LINE 8192

typedef enum
{
	COMMAND_INTERPRETER_CMD = 0,
	COMMAND_INTERPRETER_POWER_SHELL = 1,
} CommandInterpreter;


char* ExpandEnvironmentVariables(const char* input)
{
	char* result = 0;
	char* buffer = calloc(LZ_MAX_PATH, sizeof(char));
	size_t currentUsedBufferSize = 0;
	char* variableName = 0;
	char* variableValue = calloc(LZ_MAX_PATH, sizeof(char));

	if (!buffer || !variableValue)
	{
		goto cleanup;
	}

	while (*input != '\0')
	{

		if (*input == '$')
		{
			// detect "${" prefix
			++input;

			if (*input == '$')
			{
				// escape
				*(buffer + currentUsedBufferSize) = '$';
				++currentUsedBufferSize;
				++input;
				continue;
			}

			if (*input != '{')
			{
				// Invalid expression. "{" should go after "$"
				goto cleanup;
			}
			++input;

			size_t var_name_size = 0;

			// read actual variable name
			while (*input != '}')
			{
				if (*input == '\0')
				{
					// Invalid expression (closing "}" not found)
					goto cleanup;
				}
				++var_name_size;
				++input;
			}

			// extract variable
			if (variableName)
				free(variableName);
			variableName = calloc(var_name_size + 1, sizeof(char));
			if (!variableName)
				goto cleanup;

			strncpy_s(variableName, var_name_size + 1, input - var_name_size, var_name_size);

			// skip "}"
			input++;

			int variableSize = LzEnv_GetEnv(variableName, variableValue, LZ_MAX_PATH);
			if (variableSize < 0 || variableSize > LZ_MAX_PATH - 1)
			{
				// Failed to get variable
				goto cleanup;
			}

			strncpy_s(
				buffer + currentUsedBufferSize,
				LZ_MAX_PATH - currentUsedBufferSize,
				variableValue,
				variableSize
			);

			currentUsedBufferSize += variableSize;

			continue;
		}
		else
		{
			if (currentUsedBufferSize == (LZ_MAX_PATH - 1))
			{
				goto cleanup;
			}

			*(buffer + currentUsedBufferSize) = *input;
			++currentUsedBufferSize;
			++input;
		}
	}
	buffer[currentUsedBufferSize] = '\0';
	result = buffer;
	buffer = 0;

cleanup:
	if (buffer)
		free(buffer);
	if (variableName)
		free(variableName);
	if (variableValue)
		free(variableValue);

	return result;
}

char* GetWaykCseOption(int key)
{
	char optionValue[LZ_MAX_PATH];
	wchar_t optionValueW[LZ_MAX_PATH];
	if (LoadStringW(NULL, key, optionValueW, sizeof(optionValueW)) == 0)
	{
		// String is not found
		return 0;
	}

	if (LzUnicode_UTF16toUTF8(
		optionValueW,
		-1,
		(uint8_t*) optionValue,
		sizeof(optionValue)) < 0)
	{
		// Invalid data
		return 0;
	}

	return _strdup(optionValue);
}

char* GetProductName() {
	return GetWaykCseOption(IDS_WAYK_PRODUCT_NAME);
}

int GetPowerShellPath(char* pathBuffer, int pathBufferSize)
{
	int result;

	result = LzEnv_GetEnv("SYSTEMROOT", pathBuffer, pathBufferSize);
	if (result < 0)
		return LZ_ERROR_FAIL;

	return LzPathCchAppend(
		pathBuffer,
		pathBufferSize,
		"System32\\WindowsPowerShell\\v1.0\\powershell.exe"
	);
}

int GetCmdPath(char* pathBuffer, int pathBufferSize)
{
	int result;

	result = LzEnv_GetEnv("SYSTEMROOT", pathBuffer, pathBufferSize);
	if (result < 0)
		return LZ_ERROR_FAIL;

	return LzPathCchAppend(
		pathBuffer,
		pathBufferSize,
		"System32\\cmd.exe"
	);
}

static int RunCommandInterpreterCommand(CommandInterpreter interpreter, const char* command)
{
	int result;
	int bytesWritten;
	DWORD exitCode;

	char commandInterpreterPath[LZ_MAX_PATH];
	char commandLine[MAX_COMMAND_LINE];

	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	if (interpreter == COMMAND_INTERPRETER_CMD)
	{
		result = GetCmdPath(commandInterpreterPath, sizeof(commandInterpreterPath));
		if (result != LZ_OK)
			goto cleanup;
		bytesWritten = snprintf(
			commandLine,
			sizeof(commandLine),
			"\"%s\" /C %s",
			commandInterpreterPath,
			command
		);
	}
	else if (interpreter == COMMAND_INTERPRETER_POWER_SHELL)
	{
		result = GetPowerShellPath(commandInterpreterPath, sizeof(commandInterpreterPath));
		if (result != LZ_OK)
			goto cleanup;
		bytesWritten = snprintf(
			commandLine,
			sizeof(commandLine),
			"\"%s\" -WindowStyle Hidden -ExecutionPolicy Bypass -NoLogo -Command \"%s\"",
			commandInterpreterPath,
			command
		);
	}
	else
	{
		result = LZ_ERROR_PARAM;
		goto cleanup;
	}

	if (bytesWritten < 0 || bytesWritten >= sizeof(commandLine))
	{
		result = LZ_ERROR_PARAM;
		goto cleanup;
	}

	startupInfo.cb = sizeof(STARTUPINFOA);

	if (!LzCreateProcess(
		NULL,
		commandLine,
		NULL,
		NULL,
		false,
		CREATE_NO_WINDOW,
		0,
		NULL,
		&startupInfo,
		&processInfo))
	{
		result = LZ_ERROR_FAIL;
		goto cleanup;
	}

	WaitForSingleObject( processInfo.hProcess, INFINITE );

	GetExitCodeProcess(processInfo.hProcess, &exitCode);

	if (exitCode != 0)
	{
		result = LZ_ERROR_FAIL;
		goto cleanup;
	}

	result = LZ_OK;

cleanup:
	if (processInfo.hProcess)
		CloseHandle(processInfo.hProcess);

	return result;
}

int RunCmdCommand(const char* command)
{
	return RunCommandInterpreterCommand(COMMAND_INTERPRETER_CMD, command);
}

int RunPowerShellCommand(const char* command)
{
	return RunCommandInterpreterCommand(COMMAND_INTERPRETER_POWER_SHELL, command);
}

int RunWaykNowInitScript(const char* waykModulePath, const char* initScriptPath)
{
	int result;
	int bytesWritten;

	char powerShellCommand[LZ_MAX_PATH];

	if (waykModulePath)
	{
		bytesWritten = snprintf(
			powerShellCommand,
			sizeof(powerShellCommand),
			"Import-Module -Name '%s'; .'%s'",
			waykModulePath,
			initScriptPath
		);
	}
	else
	{
		bytesWritten = snprintf(
			powerShellCommand,
			sizeof(powerShellCommand),
			".'%s'",
			initScriptPath
		);
	}

	if (bytesWritten < 0 || bytesWritten >= sizeof(powerShellCommand))
	{
		result = LZ_ERROR_PARAM;
		goto cleanup;
	}

	return RunPowerShellCommand(powerShellCommand);

cleanup:
	return result;
}

int RmDirRecursively(const char* path)
{
	int result;
	int bytesWritten;

	char command[LZ_MAX_PATH];

	bytesWritten = snprintf(
		command,
		sizeof(command),
		"rmdir /S /Q \"%s\"",
		path
	);

	if (bytesWritten < 0 || bytesWritten >= sizeof(command))
	{
		result = LZ_ERROR_PARAM;
		goto cleanup;
	}

	return RunCmdCommand(command);

cleanup:
	return result;
}

char* GetPowerShellModulePath(char* waykNowPath)
{
	char path[LZ_MAX_PATH];
	path[0] = '\0';
	LzPathCchAppend(path, LZ_MAX_PATH, waykNowPath);
	LzPathCchAppend(path, LZ_MAX_PATH, "PowerShell\\Modules\\WaykNow");
	return _strdup(path);
}

int IsElevated()
{
	int isElevated = 0;
	HANDLE hToken = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION elevationState;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if(GetTokenInformation(
			hToken,
			TokenElevation,
			&elevationState,
			sizeof(elevationState),
			&cbSize))
		{
			if (elevationState.TokenIsElevated == TRUE)
			{
				isElevated = 1;
			}
		}
	}

	if(hToken)
	{
		CloseHandle(hToken);
	}

	return isElevated;
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