#include <cse/install.h>

#include "test_utils.h"

#include <string.h>

int all_available_options()
{
	CseInstall* install = CseInstall_WithLocalMsi( "C:\\installer.msi");
	if (!install)
		return 1;
	if (CseInstall_SetEnrollmentOptions(install,"http://my-url.com","1234567890") != CSE_INSTALL_OK)
		return 2;
	if (CseInstall_SetConfigOption(install, "AnalyticsEnabled", "false") != CSE_INSTALL_OK)
		return 3;
	if (CseInstall_SetInstallDirectory(install, "D:\\wayk_install") != CSE_INSTALL_OK)
		return 4;
	if (CseInstall_DisableDesktopShortcut(install) != CSE_INSTALL_OK)
		return 5;
	if (CseInstall_DisableStartMenuShortcut(install) != CSE_INSTALL_OK)
		return 6;

	const char* expected =
		"msiexec /i \"C:\\installer.msi\" "
		"ENROLL_DEN_URL=\"http://my-url.com\" ENROLL_TOKEN_ID=\"1234567890\" "
		"CONFIG_ANALYTICS_ENABLED=\"false\" INSTALLDIR=\"D:\\wayk_install\" "
		"INSTALLDESKTOPSHORTCUT=\"\" "
		"INSTALLSTARTMENUSHORTCUT=\"\"";

	const char* actual = CseInstall_GetCli(install);


	if (strcmp(actual, expected) != 0)
		return 8;

	return 0;
}

int quoted_argument_escape()
{
	CseInstall* install = CseInstall_WithLocalMsi( "C:\\installer.msi");
	if (!install)
		return 1;

	if (CseInstall_SetConfigOption(install,"personalPassword","qwe\"rty") != CSE_INSTALL_OK)
		return 2;

	const char* expected =
		"msiexec /i \"C:\\installer.msi\" "
		"CONFIG_PERSONAL_PASSWORD=\"qwe\\\\\\\"rty\"";

	const char* actual = CseInstall_GetCli(install);

	if (strcmp(actual, expected) != 0)
		return 3;

	return 0;
}

int main()
{
	assert_test_succeeded(all_available_options());
	assert_test_succeeded(quoted_argument_escape());
	return 0;
}