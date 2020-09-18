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

CseOptions* CseOptions_New();
void CseOptions_Free(CseOptions* ctx);

CseOptionsResult CseOptions_LoadFromFile(CseOptions* ctx, const char* path);
CseOptionsResult CseOptions_LoadFromString(CseOptions* ctx, const char* json);

// Configuration
char* CseOptions_GenerateAdditionalMsiOptions(CseOptions* ctx);
void CseOptions_FreeAdditionalMsiOptions(char* msiOptions);
// Installation
bool CseOptions_StartAfterInstall(CseOptions* ctx);
// Post install script
bool CseOptions_WaykNowPsModuleImportRequired(CseOptions* ctx);
// Enrollment
const char* CseOptions_GetEnrollmentUrl(CseOptions* ctx);
const char* CseOptions_GetEnrollmentToken(CseOptions* ctx);

#endif // _WAYKCSE_CSE_OPTIONS_H_
