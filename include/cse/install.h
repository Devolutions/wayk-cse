#ifndef WAYKCSE_INSTALL_H
#define WAYKCSE_INSTALL_H

#include <stdbool.h>

typedef enum
{
	CSE_INSTALL_OK,
	CSE_INSTALL_FAILURE,
	CSE_INSTALL_NOMEM,
	CSE_INSTALL_TOO_BIG_CLI,
	CSE_INSTALL_INVALID_ARGS,
	CSE_INSTALL_CREATE_PROCESS_FAILED,
	CSE_INSTALL_MSI_FAILED,
} CseInstallResult;

typedef struct cse_install CseInstall;

CseInstall* CseInstall_WithLocalMsi(const char* msiPath);
void CseInstall_Free(CseInstall* ctx);

CseInstallResult CseInstall_SetEnrollmentOptions(CseInstall* ctx, const char* url, const char* token);
CseInstallResult CseInstall_SetConfigOption(CseInstall* ctx, const char* key, const char* value);
CseInstallResult CseInstall_SetInstallDirectory(CseInstall* ctx, const char* dir);
CseInstallResult CseInstall_SetQuiet(CseInstall* ctx, bool quiet);
CseInstallResult CseInstall_DisableDesktopShortcut(CseInstall* ctx);
CseInstallResult CseInstall_DisableStartMenuShortcut(CseInstall* ctx);
CseInstallResult CseInstall_DisableSuppressLaunch(CseInstall* ctx);
CseInstallResult CseInstall_SetBrandingFile(CseInstall* ctx, const char* brandingFilePath);

CseInstallResult CseInstall_Run(CseInstall* ctx);

#ifdef CSE_TESTING
char* CseInstall_GetCli(CseInstall* ctx);
#endif

#endif //WAYKCSE_INSTALL_H
