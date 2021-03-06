#include <cse/cse_options.h>
#include <cse/log.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include <lizard/LzJson.h>

#define CSE_LOG_TAG "CseOptions"

struct wayk_now_config_option
{
	char* key;
	char* value;
	struct wayk_now_config_option* next;
};

static WaykNowConfigOption* WaykNowConfigOption_Append(WaykNowConfigOption* prev, const char* key, const char* value)
{
	WaykNowConfigOption* node = calloc(1, sizeof(WaykNowConfigOption));
	if (!node)
	{
		CSE_LOG_ERROR("Allocation failed");
		return 0;
	}

	node->key = _strdup(key);
	if (!node->key)
	{
		CSE_LOG_ERROR("Failed to create config option node");
		goto error;
	}

	node->value = _strdup(value);
	if (!node->value)
	{
		CSE_LOG_ERROR("Failed to create config option node");
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
	bool quiet;
	bool startAfterInstall;
	bool createDesktopShortcut;
	bool createStartMenuShortcut;
	bool waykPsModuleImportRequired;
	char* installPath;
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
		CSE_LOG_ERROR("Allocation failed");
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
		CSE_LOG_ERROR("Failed to create empty json root");
		goto error;
	}

	return options;

error:
	CseOptions_Free(options);
	return 0;
}

void CseOptions_Free(CseOptions* ctx)
{
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

static CseOptionsResult CseOptions_ParseCseKnownCseOptions(CseOptions* ctx, JSON_Object* root)
{
	int quiet = lz_json_object_dotget_boolean(
		root,
		"install.quiet");
	if (quiet >= 0)
	{
		CSE_LOG_TRACE("Found option -> install.quiet: %d", quiet);
		ctx->quiet = quiet;
	}

	int startAfterInstall = lz_json_object_dotget_boolean(
		root,
		"install.startAfterInstall");
	if (startAfterInstall >= 0)
	{
		CSE_LOG_TRACE("Found option -> install.startAfterInstall: %d", startAfterInstall);
		ctx->startAfterInstall = startAfterInstall;
	}

	int createDesktopShortcut = lz_json_object_dotget_boolean(
		root,
		"install.createDesktopShortcut");
	if (createDesktopShortcut >= 0)
	{
		CSE_LOG_TRACE("Found option -> install.createDesktopShortcut: %d", createDesktopShortcut);
		ctx->createDesktopShortcut = createDesktopShortcut;
	}
	else
	{
		ctx->createDesktopShortcut = true;
	}

	int createStartMenuShortcut = lz_json_object_dotget_boolean(
		root,
		"install.createStartMenuShortcut");
	if (createStartMenuShortcut >= 0)
	{
		CSE_LOG_TRACE("Found option -> install.createStartMenuShortcut: %d", createStartMenuShortcut);
		ctx->createStartMenuShortcut = createStartMenuShortcut;
	}
	else
	{
		ctx->createStartMenuShortcut = true;
	}

	int waykPsModuleImportRequired = lz_json_object_dotget_boolean(
		root,
		"postInstallScript.importWaykNowModule");
	if (waykPsModuleImportRequired >= 0)
	{
		CSE_LOG_TRACE("Found option -> postInstallScript.importWaykNowModule: %d", waykPsModuleImportRequired);
		ctx->waykPsModuleImportRequired = waykPsModuleImportRequired;
	}

	const char* installPath = lz_json_object_dotget_string(root, "install.installPath");
	if (installPath)
	{
		ctx->installPath = _strdup(installPath);
		if (!ctx->installPath)
		{
			CSE_LOG_ERROR("Allocation failed");
			return CSE_OPTIONS_NOMEM;
		}
		CSE_LOG_TRACE("Found option -> install.installPath: %s", installPath);
	}

	const char* enrollmentUrl = lz_json_object_dotget_string(root, "enrollment.url");
	if (enrollmentUrl)
	{
		ctx->enrollmentUrl = _strdup(enrollmentUrl);
		if (!ctx->enrollmentUrl)
		{
			CSE_LOG_ERROR("Allocation failed");
			return CSE_OPTIONS_NOMEM;
		}
		CSE_LOG_TRACE("Found option -> enrollment.url: %s", enrollmentUrl);
	}

	const char* enrollmentToken = lz_json_object_dotget_string(root, "enrollment.token");
	if (enrollmentToken)
	{
		ctx->enrollmentToken = _strdup(enrollmentToken);
		if (!ctx->enrollmentToken)
		{
			CSE_LOG_ERROR("Allocation failed");
			return CSE_OPTIONS_NOMEM;
		}
		CSE_LOG_TRACE("Found option -> enrollment.token: %s", enrollmentToken);
	}
	return CSE_OPTIONS_OK;
}

static CseOptionsResult CseOptions_ParseWaykConfigOptions(CseOptions* ctx, JSON_Object* root)
{
	CseOptionsResult result = CSE_OPTIONS_OK;

	JSON_Object* waykOptions = lz_json_object_get_object(root, "config");
	WaykNowConfigOption* optionsHead = 0;
	WaykNowConfigOption* optionsTail = 0;

	if (!waykOptions)
	{
		return CSE_OPTIONS_OK;
	}

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
					CSE_LOG_ERROR("Allocation failed");
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
					CSE_LOG_ERROR("Allocation failed");
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
					CSE_LOG_ERROR("Allocation failed");
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
					CSE_LOG_ERROR("Allocation failed");
					result = CSE_OPTIONS_NOMEM;
					goto error;
				}
				optionsTail = newTail;
				break;
			}
			default:{
				CSE_LOG_ERROR("Encountered invalid json type");
				result = CSE_OPTIONS_INVALID_JSON;
				goto error;
			}
		}
		CSE_LOG_TRACE("Found option -> config.%s: \"%s\"", optionsTail->key, optionsTail->value);
		if (!optionsHead)
		{
			optionsHead = optionsTail;
		}
	}

	if (optionsHead)
	{
		ctx->waykOptions = optionsHead;
	}

	return CSE_OPTIONS_OK;

error:
	if (optionsHead)
	{
		WaykNowConfigOption_FreeRecursive(optionsHead);
	}

	return result;
}

static CseOptionsResult CseOptions_Process(CseOptions* ctx, JSON_Value* rootValue)
{
	CseOptionsResult result = CSE_OPTIONS_OK;
	JSON_Object* root = lz_json_value_get_object(rootValue);
	if (!root)
	{
		CSE_LOG_ERROR("JSON file have invalid structure: root object is missing");
		result = CSE_OPTIONS_INVALID_JSON;
		goto error;
	}

	result = CseOptions_ParseCseKnownCseOptions(ctx, root);
	if (result != CSE_OPTIONS_OK)
		goto error;

	result = CseOptions_ParseWaykConfigOptions(ctx, root);
	if (result != CSE_OPTIONS_OK)
		goto error;

	return CSE_OPTIONS_OK;

error:
	if (rootValue)
	{
		lz_json_value_free(rootValue);
	}
	return result;
}

CseOptionsResult CseOptions_LoadFromFile(CseOptions* ctx, const char* path)
{
	JSON_Value* rootValue = lz_json_parse_file(path);
	if (!rootValue)
	{
		CSE_LOG_ERROR("Failed to parse json file");
		return CSE_OPTIONS_INVALID_JSON;
	}

	return CseOptions_Process(ctx, rootValue);
}

CseOptionsResult CseOptions_LoadFromString(CseOptions* ctx, const char* json)
{
	JSON_Value* rootValue = lz_json_parse_string(json);
	if (!rootValue)
	{
		CSE_LOG_ERROR("Failed to parse json string");
		return CSE_OPTIONS_INVALID_JSON;
	}

	return CseOptions_Process(ctx, rootValue);
}

bool CseOptions_Quiet(CseOptions* ctx)
{
		return ctx->quiet;
}

bool CseOptions_StartAfterInstall(CseOptions* ctx)
{
	return ctx->startAfterInstall;
}

bool CseOptions_CreateDesktopShortcut(CseOptions* ctx)
{
	return ctx->createDesktopShortcut;
}

bool CseOptions_CreateStartMenuShortcut(CseOptions* ctx)
{
	return ctx->createStartMenuShortcut;
}

const char* CseOptions_GetInstallDirectory(CseOptions* ctx)
{
	return ctx->installPath;
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

WaykNowConfigOption* CseOptions_GetFirstMsiWaykNowConfigOption(CseOptions* ctx)
{
	return ctx->waykOptions;
}

void WaykNowConfigOption_Next(WaykNowConfigOption** pOption)
{
	if (pOption)
	{
		(*pOption) = (*pOption)->next;
	}
}

const char* WaykNowConfigOption_GetKey(WaykNowConfigOption* option)
{
	if (!option)
		return 0;

	return option->key;
}

const char* WaykNowConfigOption_GetValue(WaykNowConfigOption* option)
{
	if (!option)
		return 0;

	return option->value;
}
