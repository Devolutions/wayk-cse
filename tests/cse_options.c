#include <cse/cse_options.h>

#include "test_utils.h"

#include <string.h>

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

	WaykNowConfigOption* option = CseOptions_GetFirstMsiWaykNowConfigOption(options);
	if (!option)
	{
		result = 7;
		goto finalize;
	}
	if (strcmp(WaykNowConfigOption_GetKey(option), "autoUpdateEnabled") != 0)
	{
		result = 8;
		goto finalize;
	}
	if (strcmp(WaykNowConfigOption_GetValue(option), "true") != 0)
	{
		result = 9;
		goto finalize;
	}

	WaykNowConfigOption_Next(&option);
	if (!option)
	{
		result = 10;
		goto finalize;
	}

	if (strcmp(WaykNowConfigOption_GetKey(option), "autoLaunchOnUserLogon") != 0)
	{
		result = 11;
		goto finalize;
	}
	if (strcmp(WaykNowConfigOption_GetValue(option), "false") != 0)
	{
		result = 12;
		goto finalize;
	}

	WaykNowConfigOption_Next(&option);
	if (!option)
	{
		result = 13;
		goto finalize;
	}

	if (strcmp(WaykNowConfigOption_GetKey(option), "generatedPasswordLength") != 0)
	{
		result = 14;
		goto finalize;
	}
	if (strcmp(WaykNowConfigOption_GetValue(option), "13") != 0)
	{
		result = 15;
		goto finalize;
	}

	WaykNowConfigOption_Next(&option);
	if (!option)
	{
		result = 16;
		goto finalize;
	}

	if (strcmp(WaykNowConfigOption_GetKey(option), "loggingFilter") != 0)
	{
		result = 17;
		goto finalize;
	}
	if (strcmp(WaykNowConfigOption_GetValue(option), "test") != 0)
	{
		result = 18;
		goto finalize;
	}

	WaykNowConfigOption_Next(&option);
	if (option)
	{
		result = 19;
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