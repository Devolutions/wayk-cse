#include <cse/cse_options.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include <lizard/LzJson.h>

struct wayk_now_config_option
{
	char* key;
	char* value;
	struct wayk_now_config_option* next;
};

typedef struct wayk_now_config_option WaykNowConfigOption;

static WaykNowConfigOption* WaykNowConfigOption_Append(WaykNowConfigOption* prev, const char* key, const char* value)
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

void WaykNowConfigOption_FreeRecursive(WaykNowConfigOption* ctx)
{
	if (ctx->next)
	{
		WaykNowConfigOption_FreeRecursive(ctx->next);
	}
	free(ctx->key);
	free(ctx->value);
}

struct cse_options
{
	JSON_Value* jsonRoot;
	bool startAfterInstall;
	bool waykPsModuleImportRequired;
	char* enrollmentUrl;
	char* enrollmentToken;
	WaykNowConfigOption* waykOptions;
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
	if (ctx->waykOptions)
	{
		WaykNowConfigOption_FreeRecursive(ctx->waykOptions);
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

	JSON_Object* waykOptions = lz_json_object_get_object(root, "config");
	WaykNowConfigOption* optionsHead = 0;
	WaykNowConfigOption* optionsTail = 0;
	if (waykOptions)
	{
		for (int i = 0; i < lz_json_object_get_count(waykOptions); ++i)
		{
			JSON_Value* currentValue = lz_json_object_get_value_at(waykOptions, i);
			switch (lz_json_value_get_type(currentValue))
			{
				case JSONString:
				{
					WaykNowConfigOption* newTail = WaykNowConfigOption_Append(
						optionsTail,
						lz_json_object_get_name(waykOptions, i),
						lz_json_value_get_string(currentValue));

					if (!newTail)
					{
						result = CSE_OPTIONS_NOMEM;
						goto error;
					}
					optionsTail = newTail;
					break;
				}
				case JSONBoolean:
				{
					const char* value = lz_json_value_get_boolean(currentValue) ? "true" : "false";

					WaykNowConfigOption* newTail = WaykNowConfigOption_Append(
						optionsTail,
						lz_json_object_get_name(waykOptions, i),
						value);

					if (!newTail)
					{
						result = CSE_OPTIONS_NOMEM;
						goto error;
					}
					optionsTail = newTail;
					break;
				}
				case JSONNumber:
				{
					char* numberBuffer = malloc(64);
					if (!numberBuffer)
					{
						result = CSE_OPTIONS_NOMEM;
						goto error;
					}
					sprintf_s(
						numberBuffer,
						64,
						"%d",
						(int)lz_json_value_get_number(currentValue));
					WaykNowConfigOption* newTail = WaykNowConfigOption_Append(
						optionsTail,
						lz_json_object_get_name(waykOptions, i),
						numberBuffer);
					free(numberBuffer);
					if (!newTail)
					{
						result = CSE_OPTIONS_NOMEM;
						goto error;
					}
					optionsTail = newTail;
					break;
				}
				default:{
					result = CSE_OPTIONS_INVALID_JSON;
					goto error;
				}
			}

			if (!optionsHead)
			{
				optionsHead = optionsTail;
			}
		}
	}

	return CSE_OPTIONS_OK;

error:
	if (rootValue)
	{
		lz_json_value_free(rootValue);
	}
	if (optionsHead)
	{
		WaykNowConfigOption_FreeRecursive(optionsHead);
	}
	return result;
}


char* CseOptions_GenerateAdditionalMsiOptions(CseOptions* ctx)
{
	int bufferSize = 256;
	char* cliOptions = malloc(bufferSize);
	cliOptions[0] = '\0';

	WaykNowConfigOption* option = ctx->waykOptions;

	while (option)
	{
		char* msiOption = ToSnakeCase(option->key);
		if (!msiOption)
		{
			goto error;
		}

		unsigned int prevSize = strlen(cliOptions);
		unsigned int result = 0;
		do
		{
			result = sprintf_s(
				cliOptions + prevSize,
				bufferSize - prevSize,
				" %s=\"%s\"",
				msiOption,
				option->value);
			if (result < 0)
			{
				char* newBlock = realloc(cliOptions, bufferSize * 2);
				if (!newBlock)
				{
					goto error;
				}
				cliOptions = newBlock;
				bufferSize *= 2;
			}

		} while (result < 0);

		option = option->next;
	}

	return cliOptions;

error:
	if (cliOptions)
	{
		free(cliOptions);
	}
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