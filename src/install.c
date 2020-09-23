#include <cse/install.h>

#include <lizard/lizard.h>

#define MAX_OPTION_NAME_SIZE 256
#define MSI_OPTION_PREFIX "CONFIG_"

#define MAX_CLI_MIN_BUFFER_SIZE 256

// CreateProcessW limit (32767) + null terminator
#define MAX_CLI_BUFFER_SIZE 32768

struct cse_isntall
{
	char* waykNowExecutable;
	char* cli;
	size_t cliBufferCapacity; // stored cli buffer available capacity
	size_t cliBufferSize;     // stored cli string size (without trailing '\0')
};

static CseInstallResult ToSnakeCase(const char* str, char* buffer, size_t bufferSize)
{
	unsigned int originalSize = strlen(str);
	unsigned int currentSize = 0;
	if (!buffer)
	{
		return 0;
	}

	for (unsigned int i = 0; i < originalSize; ++i)
	{
		if (isupper(str[i]) && (i != 0))
		{
			if (currentSize + 3 > bufferSize)
				return CSE_INSTALL_FAILURE;
			buffer[currentSize++] = '_';
			buffer[currentSize++] = str[i];
		}
		else
		{
			if (currentSize + 2 > bufferSize)
				return CSE_INSTALL_FAILURE;
			buffer[currentSize++] = _toupper(str[i]);
		}
	}

	buffer[currentSize] = '\0';
	return CSE_INSTALL_OK;
}

CseInstallResult WaykConfigOptionToMsiOption(const char* str, char* buffer, size_t bufferSize)
{
	strcpy_s(buffer, bufferSize, MSI_OPTION_PREFIX);
	size_t currentBufferSize = sizeof(MSI_OPTION_PREFIX) / sizeof(char) - 1;
	return ToSnakeCase(str, buffer + currentBufferSize, bufferSize - currentBufferSize);
}

static CseInstallResult CseInstall_EnsureCliBufferCapacity(CseInstall* ctx, size_t requiredStringSize)
{
	size_t required_capacity = (ctx->cliBufferCapacity)
		? ctx->cliBufferCapacity
		: MAX_CLI_MIN_BUFFER_SIZE;
	while (required_capacity < (ctx->cliBufferSize + (requiredStringSize + 1)))
	{
		required_capacity *= 2;
	}

	if (required_capacity > MAX_CLI_BUFFER_SIZE)
	{
		return CSE_INSTALL_TOO_BIG_CLI;
	}

	if (required_capacity <= ctx->cliBufferCapacity)
	{
		return CSE_INSTALL_OK;
	}

	if (ctx->cliBufferCapacity)
	{
		char* newBuffer = realloc(ctx->cli, required_capacity);
		if (!newBuffer)
		{
			return CSE_INSTALL_NOMEM;
		}
		ctx->cli = newBuffer;
	}
	else
	{
		ctx->cli = malloc(required_capacity);
	}

	ctx->cliBufferCapacity = required_capacity;
	return CSE_INSTALL_OK;
}

static CseInstallResult CseInstall_CliAppendString(CseInstall* ctx, const char* str)
{
	size_t strSize = strlen(str);
	CseInstallResult result = CseInstall_EnsureCliBufferCapacity(ctx, strSize);
	if (result != CSE_INSTALL_OK)
	{
		return result;
	}
	strcpy_s(ctx->cli + ctx->cliBufferSize, ctx->cliBufferCapacity - ctx->cliBufferSize, str);
	ctx->cliBufferSize += strSize;
	return CSE_INSTALL_OK;
}

static CseInstallResult CseInstall_CliAppendChar(CseInstall* ctx, char ch)
{
	CseInstallResult result = CseInstall_EnsureCliBufferCapacity(ctx, 1);
	if (result != CSE_INSTALL_OK)
	{
		return result;
	}
	ctx->cli[ctx->cliBufferSize++] = ch;
	ctx->cli[ctx->cliBufferSize] = '\0';
	return CSE_INSTALL_OK;
}

static CseInstallResult CseInstall_CliAppendWhitespace(CseInstall* ctx)
{
	return CseInstall_CliAppendChar(ctx, ' ');
}

static size_t GetQuotesCount(const char* str)
{
	size_t strSize = strlen(str);
	size_t quotes = 0;
	for (size_t i = 0; i <  strSize; ++i)
	{
		if (str[i] == '"')
			++quotes;
	}
	return quotes;
}

static char* MakeEscapedArgument(const char* str)
{
	if (!str)
		return 0;

	size_t originalSize = strlen(str);
	// allocate original size + space for escape symbols for escape
	char* escaped = calloc(originalSize + GetQuotesCount(str) + 1, sizeof(char));
	if (!escaped)
		return 0;
	size_t resultStringSize = 0;
	for (int i = 0; i < originalSize; ++i)
	{
		if (str[i] == '"')
		{
			escaped[resultStringSize++] = '\\';
		}

		escaped[resultStringSize++] = str[i];
	}

	escaped[resultStringSize] = '\0';

	return escaped;
}

static CseInstallResult CseInstall_CliAppendQuotedString(CseInstall* ctx, const char* str)
{
	CseInstallResult result = CseInstall_CliAppendChar(ctx, '"');
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_CliAppendString(ctx, str);
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_CliAppendChar(ctx, '"');
	if (result != CSE_INSTALL_OK) return result;
	return result;
}

static CseInstall* CseInstall_New(const char* waykNowExecutable, const char* msiPath)
{
	CseInstall* ctx = calloc(1, sizeof(CseInstall));
	if (!ctx)
	{
		goto error;
	}

	ctx->waykNowExecutable = _strdup(waykNowExecutable);
	if (!ctx->waykNowExecutable)
	{
		goto error;
	}

	CseInstallResult result = CSE_INSTALL_OK;
	if (msiPath)
	{
		result = CseInstall_CliAppendString(ctx, "install-local-package");
		if (result != CSE_INSTALL_OK) goto error;
		result = CseInstall_CliAppendWhitespace(ctx);
		if (result != CSE_INSTALL_OK) goto error;
		result = CseInstall_CliAppendQuotedString(ctx, msiPath);
		if (result != CSE_INSTALL_OK) goto error;
	}
	else
	{
		result = CseInstall_CliAppendString(ctx, "install");
		if (result != CSE_INSTALL_OK) goto error;
	}

	return ctx;

error:
	if (ctx)
	{
		CseInstall_Free(ctx);
	}
	return 0;
}

void CseInstall_Free(CseInstall* ctx)
{
	if (ctx->waykNowExecutable)
		free(ctx->waykNowExecutable);
	if (ctx->cli)
		free(ctx->cli);
}

CseInstall* CseInstall_WithLocalMsi(const char* waykNowExecutable, const char* msiPath)
{
	return CseInstall_New(waykNowExecutable, msiPath);
}

CseInstall* CseInstall_WithMsiDownload(const char* waykNowExecutable)
{
	return CseInstall_New(waykNowExecutable, 0);
}

static CseInstallResult CseInstall_SetMsiOption(CseInstall* ctx, const char* key, const char* value)
{
	if (!key || !value)
	{
		return CSE_INSTALL_INVALID_ARGS;
	}

	CseInstallResult result;

	// appends CONFIG_KEY="value"
	result = CseInstall_CliAppendWhitespace(ctx);
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_CliAppendString(ctx, key);
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_CliAppendChar(ctx, '=');
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_CliAppendQuotedString(ctx, value);
	if (result != CSE_INSTALL_OK) return result;

	return CSE_INSTALL_OK;
}

CseInstallResult CseInstall_SetEnrollmentOptions(CseInstall* ctx, const char* url, const char* token)
{
	if (!url || !token)
	{
		return CSE_INSTALL_INVALID_ARGS;
	}

	CseInstallResult result;

	result = CseInstall_SetMsiOption(ctx, "ENROLL_DEN_URL", url);
	if (result != CSE_INSTALL_OK) return result;
	result = CseInstall_SetMsiOption(ctx, "ENROLL_TOKEN_ID", token);
	if (result != CSE_INSTALL_OK) return result;

	return CSE_INSTALL_OK;
}

CseInstallResult CseInstall_SetConfigOption(CseInstall* ctx, const char* key, const char* value)
{
	char msiOptionName[MAX_OPTION_NAME_SIZE];
	CseInstallResult result = WaykConfigOptionToMsiOption(key, msiOptionName, MAX_OPTION_NAME_SIZE);
	if (result != CSE_INSTALL_OK) return result;


	char* escapedValue = MakeEscapedArgument(value);
	if (!escapedValue)
	{
		return CSE_INSTALL_NOMEM;
	}

	result = CseInstall_SetMsiOption(ctx, msiOptionName, value);
	free(escapedValue);
	return result;
}

CseInstallResult CseInstall_SetInstallDirectory(CseInstall* ctx, const char* dir)
{
	if (!dir)
	{
		return CSE_INSTALL_INVALID_ARGS;
	}

	return CseInstall_SetMsiOption(ctx, "INSTALLDIR", dir);
}

CseInstallResult CseInstall_EnableLaunchWaykNowAfterInstall(CseInstall* ctx)
{
	return CseInstall_SetMsiOption(ctx, "SUPPRESSLAUNCH", "0");
}

CseInstallResult CseInstall_DisableDesktopShortcut(CseInstall* ctx)
{
	return CseInstall_SetMsiOption(ctx, "INSTALLDESKTOPSHORTCUT", "0");
}

CseInstallResult CseInstall_DisableStartMenuShortcut(CseInstall* ctx)
{
	return CseInstall_SetMsiOption(ctx, "INSTALLSTARTMENUSHORTCUT", "0");
}

#ifdef CSE_TESTING

char* CseInstall_GetCli(CseInstall* ctx)
{
	return ctx->cli;
}

#endif


CseInstallResult CseInstall_Run(CseInstall* ctx)
{
	CseInstallResult result = CSE_INSTALL_OK;
	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	DWORD returnCode = 0;
	void* wow64FsRedirectionContext = 0;

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
	startupInfo.cb = sizeof(STARTUPINFOA);
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	if (LzIsWow64())
		pfnWow64DisableWow64FsRedirection(&wow64FsRedirectionContext);

	BOOL processCreated = LzCreateProcess(
		ctx->waykNowExecutable,
		ctx->cli,
		0,
		0,
		FALSE,
		0,
		0,
		0,
		&startupInfo,
		&processInfo);

	if (LzIsWow64())
		pfnWow64RevertWow64FsRedirection(wow64FsRedirectionContext);

	if (processCreated != TRUE)
	{
		result = CSE_INSTALL_CREATE_PROCESS_FAILED;
		goto finalize;
	}

	WaitForSingleObject(processInfo.hProcess, INFINITE);
	GetExitCodeProcess(processInfo.hProcess, &returnCode);
	if (returnCode != 0)
	{
		result = CSE_INSTALL_MSI_FAILED;
	}

finalize:
	if (processInfo.hThread)
		CloseHandle(processInfo.hThread);
	if (processInfo.hProcess)
		CloseHandle(processInfo.hProcess);

	return result;
}