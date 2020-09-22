#include <cse/install.h>

#include "test_utils.h"

#include <string.h>

int all_available_options()
{
	CseInstall* install = CseInstall_WithLocalMsi("C:\\wayk.exe", "C:\\installer.msi");
	if (!install)
		return 1;
	if (CseInstall_SetEnrollmentOptions(install,"http://my-url.com","1234567890") != CSE_INSTALL_OK)
		return 2;
	if (CseInstall_SetConfigOption(install, "analyticsEnabled", "false") != CSE_INSTALL_OK)
		return 3;
	if (CseInstall_SetInstallDirectory(install, "D:\\wayk_install") != CSE_INSTALL_OK)
		return 4;
	if (CseInstall_EnableLaunchWaykNowAfterInstall(install) != CSE_INSTALL_OK)
		return 5;
	if (CseInstall_DisableDesktopShortcut(install) != CSE_INSTALL_OK)
		return 6;
	if (CseInstall_DisableStartMenuShortcut(install) != CSE_INSTALL_OK)
		return 7;

	const char* expected =
		"install-local-package \"C:\\installer.msi\" ENROLL_DEN_URL=\"http://my-url.com\" "
  		"ENROLL_TOKEN_ID=\"1234567890\" CONFIG_ANALYTICS_ENABLED=\"false\" "
		"INSTALLDIR=\"D:\\wayk_install\" SUPPRESSLAUNCH=\"0\" INSTALLDESKTOPSHORTCUT=\"0\" "
  		"INSTALLSTARTMENUSHORTCUT=\"0\"";

	if (strcmp(CseInstall_GetCli(install), expected) != 0)
		return 8;

	return 0;
}

int download_msi()
{
	CseInstall* install = CseInstall_WithMsiDownload("C:\\wayk.exe");
	if (!install)
		return 1;

	if (CseInstall_SetEnrollmentOptions(install,"http://my-url.com","1234567890") != CSE_INSTALL_OK)
		return 2;

	const char* expected =
		"install ENROLL_DEN_URL=\"http://my-url.com\" ENROLL_TOKEN_ID=\"1234567890\"";

	if (strcmp(CseInstall_GetCli(install), expected) != 0)
		return 3;

	return 0;
}

int quoted_argument_escape()
{
	CseInstall* install = CseInstall_WithMsiDownload("C:\\wayk.exe");
	if (!install)
		return 1;

	if (CseInstall_SetConfigOption(install,"personalPassword","qwe\"rty") != CSE_INSTALL_OK)
		return 2;

	const char* expected =
		"install CONFIG_PERSONAL_PASSWORD=\"\"";

	if (strcmp(CseInstall_GetCli(install), expected) != 0)
		return 3;

	return 0;
}

int main()
{
	assert_test_succeeded(all_available_options());
	assert_test_succeeded(download_msi());
	return 0;
}