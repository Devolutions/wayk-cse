#ifndef WAYKCSE_INSTALL_H
#define WAYKCSE_INSTALL_H

typedef enum
{
	CSE_INSTALL_OK,
	CSE_INSTALL_FAILURE,
	CSE_INSTALL_NOMEM,
	CSE_INSTALL_TOO_BIG_CLI,
} CseInstallResult;

typedef struct cse_isntall CseInstall;

CseInstall* CseInstall_WithLocalMsi(const char* waykNowExecutable, const char* msiPath);

CseInstallResult CseInstall_AppendEnrollmentOptions(const char* url, const char* token);
CseInstallResult CseInstall_AppendConfigOption(const char* key, const char* value);

void CseInstallFree(CseInstall* ctx);

#endif //WAYKCSE_INSTALL_H
