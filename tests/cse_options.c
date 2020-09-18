#include <cse/cse_options.h>

#include "test_utils.h"

#include <string.h>
#include <stdio.h>

int connect_json()
{
	int result = 0;
	CseOptions* options = CseOptions_New();
	if (!options)
	{
		result = 1;
		goto finalize;
	}

	CseOptionsResult loadResult = CseOptions_LoadFromFile(
		options,
		"tests/data/options.json");

	if (loadResult != CSE_OPTIONS_OK)
	{
		result = 2;
		goto finalize;
	}

	if (strcmp(CseOptions_GetEnrollmentUrl(options), "test.com") != 0)
	{
		result = 3;
		goto finalize;
	}

	if (strcmp(CseOptions_GetEnrollmentToken(options), "123") != 0)
	{
		result = 4;
		goto finalize;
	}

	if (!CseOptions_StartAfterInstall(options))
	{
		result = 5;
		goto finalize;
	}

	if (!CseOptions_WaykNowPsModuleImportRequired(options))
	{
		result = 6;
		goto finalize;
	}

	const char* argsResult = CseOptions_GenerateAdditionalMsiOptions(options);
	if (!argsResult)
	{
		result = 7;
		goto finalize;
	}

	if (strcmp(
		argsResult,
		" CONFIG_AUTO_UPDATE_ENABLED=\"true\""
		" CONFIG_AUTO_LAUNCH_ON_USER_LOGON=\"false\""
		" CONFIG_GENERATED_PASSWORD_LENGTH=\"13\""
		" CONFIG_LOGGING_FILTER=\"test\"") != 0)
	{
		result = 8;
		goto finalize;
	}

	return loadResult;

finalize:
	if (options)
		CseOptions_Free(options);

	return result;
}

int invalid_json()
{
	int result = 0;
	CseOptions* options = CseOptions_New();
	if (!options)
	{
		result = 1;
		goto finalize;
	}

	CseOptionsResult loadResult = CseOptions_LoadFromString(options, "{{{{");

	if (loadResult != CSE_OPTIONS_INVALID_JSON)
	{
		result = 1;
		goto finalize;
	}

finalize:
	if (options)
		CseOptions_Free(options);

	return result;
}

int main()
{
	assert_test_succeeded(connect_json());
	assert_test_succeeded(invalid_json());
	return 0;
}