#ifndef _WAYKCSE_CSE_OPTIONS_H_
#define _WAYKCSE_CSE_OPTIONS_H_

#include <stdbool.h>

typedef struct cse_options CseOptions;

typedef enum
{
	CSE_OPTIONS_OK,
	CSE_OPTIONS_INVALID_JSON,
	CSE_OPTIONS_NOMEM
} CseOptionsResult;

typedef struct wayk_now_config_option WaykNowConfigOption;

CseOptions* CseOptions_New();
void CseOptions_Free(CseOptions* ctx);

CseOptionsResult CseOptions_LoadFromFile(CseOptions* ctx, const char* path);
CseOptionsResult CseOptions_LoadFromString(CseOptions* ctx, const char* json);

// Installation
bool CseOptions_StartAfterInstall(CseOptions* ctx);
// Post install script
bool CseOptions_WaykNowPsModuleImportRequired(CseOptions* ctx);
// Enrollment
const char* CseOptions_GetEnrollmentUrl(CseOptions* ctx);
const char* CseOptions_GetEnrollmentToken(CseOptions* ctx);

WaykNowConfigOption* CseOptions_GetFirstMsiWaykNowConfigOption(CseOptions* ctx);

void WaykNowConfigOption_Next(WaykNowConfigOption** pOption);
const char* WaykNowConfigOption_GetKey(WaykNowConfigOption* option);
const char* WaykNowConfigOption_GetValue(WaykNowConfigOption* option);



#endif // _WAYKCSE_CSE_OPTIONS_H_
