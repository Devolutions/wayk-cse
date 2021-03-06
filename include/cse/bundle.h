#ifndef WAYKCSE_BUNDLE_H
#define WAYKCSE_BUNDLE_H

#include <stdbool.h>

typedef enum
{
	WAYK_CSE_BUNDLE_OK,
	WAYK_CSE_BUNDLE_MISSING_PACKAGE,
	WAYK_CSE_BUNDLE_FS_ERROR,
} WaykCseBundleStatus;

typedef enum
{
	WAYK_BINARIES_BITNESS_X86,
	WAYK_BINARIES_BITNESS_X64,
} WaykBinariesBitness;


struct waykcse_bundle;
typedef struct waykcse_bundle WaykCseBundle;

WaykCseBundle* WaykCseBundle_Open();
void WaykCseBundle_Close(WaykCseBundle* ctx);

WaykCseBundleStatus WaykCseBundle_ExtractWaykNowInstaller(
	WaykCseBundle* ctx,
	WaykBinariesBitness bitness,
	const char* targetFolder);
WaykCseBundleStatus WaykCseBundle_ExtractBrandingZip(
	WaykCseBundle* ctx,
	const char* targetFolder);
WaykCseBundleStatus WaykCseBundle_ExtractPowerShellInitScript(
	WaykCseBundle* ctx,
	const char* targetFolder);
WaykCseBundleStatus WaykCseBundle_ExtractOptionsJson(
	WaykCseBundle* ctx,
	const char* targetFolder);

const char* GetBrandingFileName();
const char* GetPowerShellInitScriptFileName();
const char* GetJsonOptionsFileName();
const char* GetInstallerFileName(WaykBinariesBitness bitness);


#endif //WAYKCSE_BUNDLE_H
