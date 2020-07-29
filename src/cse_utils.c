#include <cse/cse_utils.h>

#include <windows.h>

#include <lizard/lizard.h>

#define MAX_COMMAND_LINE 8192

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

char* GetWaykCsePathOption(int key)
{
	char* expandedPath = 0;

	char* originalOption = GetWaykCseOption(key);
	if (!originalOption)
	{
		goto cleanup;
	}

	expandedPath = ExpandEnvironmentVariables(originalOption);

	cleanup:
	if (originalOption)
		free(originalOption);

	return expandedPath;
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

int RunPowerShellCommand(const char* command)
{
	int result;
	int bytesWritten;
	DWORD exitCode;

	char powerShellPath[LZ_MAX_PATH];
	char commandLine[MAX_COMMAND_LINE];

	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	result = GetPowerShellPath(powerShellPath, sizeof(powerShellPath));
	if (result != LZ_OK)
		goto cleanup;

	bytesWritten = snprintf(
		commandLine,
		sizeof(commandLine),
		"\"%s\" -WindowStyle Hidden -ExecutionPolicy Bypass -NoLogo -Command \"%s\"",
		powerShellPath,
		command
	);

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

int RunWaykNowInitScript(const char* waykModulePath, const char* initScriptPath)
{
	int result;
	int bytesWritten;

	char powerShellCommand[LZ_MAX_PATH];

	bytesWritten = snprintf(
		powerShellCommand,
		sizeof(powerShellCommand),
		"Import-Module -Name '%s'; .'%s'",
		waykModulePath,
		initScriptPath
	);

	if (bytesWritten < 0 || bytesWritten >= sizeof(powerShellCommand))
	{
		result = LZ_ERROR_PARAM;
		goto cleanup;
	}

	return RunPowerShellCommand(powerShellCommand);

cleanup:
	return result;
}