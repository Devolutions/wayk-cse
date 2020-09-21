#include <cse/install.h>

#include <lizard/lizard.h>

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
	strcpy(ctx->cli + ctx->cliBufferSize, str);
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
	ctx->cli[ctx->cliBufferSize] = ch;
	ctx->cli[ctx->cliBufferSize] = '\0';
	++ctx->cliBufferSize;
	return CSE_INSTALL_OK;
}

static CseInstallResult CseInstall_CliAppendWhitespace(CseInstall* ctx)
{
	return CseInstall_CliAppendChar(ctx, ' ');
}

static CseInstallResult CseInstall_CliAppendQuotedString(CseInstall* ctx, const char* str)
{
	CseInstallResult result = CseInstall_CliAppendChar(ctx, '"');
	if (result != CSE_INSTALL_OK) return result;
	// TODO: escape quotes in string
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

error:
	if (ctx)
	{
		CseInstall_Free(ctx);
	}
	return 0;
}

CseInstall* CseInstall_WithLocalMsi(const char* waykNowExecutable, const char* msiPath)
{
	return CseInstall_New(waykNowExecutable, msiPath);
}

CseInstall* CseInstall_WithMsiDownload(const char* waykNowExecutable)
{
	return CseInstall_New(waykNowExecutable, 0);
}

CseInstallResult CseInstall_AppendEnrollmentOptions(const char* url, const char* token)
{
	// TODO: Not implemented
	return CSE_INSTALL_FAILURE;
}

CseInstallResult CseInstall_AppendConfigOption(const char* key, const char* value)
{
	// TODO: Not implemented
	return CSE_INSTALL_FAILURE;
}