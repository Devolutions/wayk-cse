#include <cse/cse_options.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <lizard/LzJson.h>

struct wayk_now_config_option
{
	char* key;
	char* value;
	struct wayk_now_config_option* next;
};

typedef struct wayk_now_config_option WaykNowConfigOption;

static WaykNowConfigOption* WaykNowConfigOption_Append(WaykNowConfigOption* prev, char* key, char* value)
{
	WaykNowConfigOption* node = calloc(1, sizeof(WaykNowConfigOption));
	if (!node)
	{
		return 0;
	}

	node->key = _strdup(key);
	if (!node->key)
	{
		goto error;
	}

	node->value = _strdup(value);
	if (!node->value)
	{
		goto error;
	}

	if (prev)
	{
		prev->next = node;
	}

	return node;

error:
	if (node->value)
	{
		free(node->value);
	}
	if (node->key)
	{
		free(node->key);
	}
	free(node);
	return 0;
}

struct cse_options
{
	JSON_Value* jsonRoot;
	bool startAfterInstall;
	bool waykPsModuleImportRequired;
	char* enrollmentUrl;
	char* enrollmentToken;
};

static char* ToSnakeCase(const char* str)
{
	unsigned int originalSize = strlen(str);
	unsigned int bufferSize = originalSize * 2;
	unsigned int currentSize = 0;
	char* buffer = calloc(bufferSize, sizeof(char));
	if (!buffer)
	{
		return 0;
	}

	for (unsigned int i = 0; i < originalSize; ++i)
	{
		if (isupper(str[i]) && (i != 0))
		{
			buffer[currentSize++] = '_';
			buffer[currentSize++] = str[i];
		}
		else
		{
			buffer[currentSize++] = _toupper(str[i]);
		}
	}

	return buffer;
}


CseOptions* CseOptions_New()
{
	CseOptions* options = calloc(1, sizeof(CseOptions));
	options->jsonRoot = lz_json_value_init_null();
	if (!options->jsonRoot)
	{
		goto error;
	}

	return options;

error:
	CseOption_Free(options);
	return 0;
}

void CseOption_Free(CseOptions* ctx)
{
	if (!ctx)
	{
		return;
	}

	if (ctx->jsonRoot)
	{
		lz_json_value_free(ctx->jsonRoot);
	}
	if (ctx->enrollmentToken)
	{
		free(ctx->enrollmentToken);
	}
	if (ctx->enrollmentUrl)
	{
		free(ctx->enrollmentUrl);
	}

	free(ctx);
}

CseOptionsResult CseOptions_Load(CseOptions* ctx, const char* path)
{
	CseOptionsResult result = CSE_OPTIONS_OK;
	JSON_Value* rootValue = lz_json_parse_file(path);
	JSON_Object* root = lz_json_value_get_object(rootValue);
	if (!root)
	{
		result = CSE_OPTIONS_INVALID_JSON;
		goto error;
	}

	int startAfterInstall = lz_json_object_dotget_boolean(
		root,
		"install.startAfterInstall");
	if (startAfterInstall >= 0)
	{
		ctx->startAfterInstall = startAfterInstall;
	}

	int waykPsModuleImportRequired = lz_json_object_dotget_boolean(
		root,
		"postInstallScript.importWaykNowModule");
	if (waykPsModuleImportRequired >= 0)
	{
		ctx->waykPsModuleImportRequired = waykPsModuleImportRequired;
	}

	const char* enrollmentUrl = lz_json_object_dotget_string(root, "enrollment.url");
	if (enrollmentUrl)
	{
		ctx->enrollmentUrl = _strdup(enrollmentUrl);
		if (!ctx->enrollmentUrl)
		{
			result = CSE_OPTIONS_NOMEM;
			goto error;
		}
	}

	const char* enrollmentToken = lz_json_object_dotget_string(root, "enrollment.token");
	if (enrollmentToken)
	{
		ctx->enrollmentToken = _strdup(enrollmentToken);
		if (!ctx->enrollmentToken)
		{
			result = CSE_OPTIONS_NOMEM;
			goto error;
		}
	}

	// TODO: MSI options generation

	return CSE_OPTIONS_OK;

error:
	if (rootValue)
	{
		lz_json_value_free(rootValue);
	}
	return result;
}


char* CseOptions_GenerateAdditionalMsiOptions(CseOptions* ctx)
{
	// TODO: Not implemented yet
	return 0;
}

void CseOptions_FreeAdditionalMsiOptions(char* msiOptions)
{
	free(msiOptions);
}

bool CseOptions_StartAfterInstall(CseOptions* ctx)
{
	return ctx->startAfterInstall;
}

bool CseOptions_WaykNowPsModuleImportRequired(CseOptions* ctx)
{
	return ctx->waykPsModuleImportRequired;
}

const char* CseOptions_GetEnrollmentUrl(CseOptions* ctx)
{
	return ctx->enrollmentUrl;
}

const char* CseOptions_GetEnrollmentToken(CseOptions* ctx)
{
	return ctx->enrollmentToken;
}