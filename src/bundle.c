#include <cse/bundle.h>

#include <stdio.h>
#include <wchar.h>
#include <lizard/lizard.h>

#include <resource.h>

#define BRANDING_FILE_NAME "branding.zip"
#define POWER_SHELL_INIT_SCRIPT_FILE_NAME "init.ps1"

#define INSTALLER_FILE_NAME_X86 "Installer_x86.msi"
#define INSTALLER_FILE_NAME_X64 "Installer_x64.msi"

#define WAYK_NOW_BINARY_FILE_NAME_X86 "WaykNow_x86.exe"
#define WAYK_NOW_BINARY_FILE_NAME_X64 "WaykNow_x64.exe"

#define JSON_OPTIONS_FILE_NAME "options.json"

struct waykcse_bundle
{
	LzArchive* archiveHandle;
};

WaykCseBundle* WaykCseBundle_Open()
{
	WaykCseBundle* result = 0;
	WaykCseBundle* bundle = 0;
	HRSRC resourceInfo = 0;
	HGLOBAL resource = 0;
	BYTE* resourceData = 0;
	DWORD resourceSize = 0;

	bundle = calloc(1, sizeof(WaykCseBundle));
	if (!bundle)
	{
		fwprintf(stderr, L"Can't allocate WaykCseBundle\n");
		goto cleanup;
	}

	resourceInfo = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_WAYK_BUNDLE), (LPCWSTR) RT_RCDATA);
	if (!resourceInfo)
	{
		fwprintf(stderr, L"Can't find wayk bundle resource\n");
		goto cleanup;
	}

	resource = LoadResource(0, resourceInfo);
	if (!resource)
	{
		fwprintf(stderr, L"Can't load wayk bundle resource\n");
		goto cleanup;
	}

	// Actually on Windows version > XP LockResource just calculated and returns
	// pointer; unlock and FreeResoource is not required anymore on modern Windows
	resourceData = (BYTE*) LockResource(resource);
	resourceSize = SizeofResource(NULL, resourceInfo);

	bundle->archiveHandle = LzArchive_New();
	if (!bundle->archiveHandle)
	{
		fwprintf(stderr, L"Can't create LzArchive\n");
		goto cleanup;
	}

	if (LzArchive_OpenData(bundle->archiveHandle, resourceData, resourceSize) != LZ_OK)
	{
		fwprintf(stderr, L"Embedded bundle has invalid format\n");
		goto cleanup;
	}

	result = bundle;
	bundle = 0;

cleanup:
	if (bundle)
		WaykCseBundle_Close(bundle);

	return result;
}

void WaykCseBundle_Close(WaykCseBundle* ctx)
{
	if (ctx->archiveHandle)
	{
		LzArchive_Close(ctx->archiveHandle);
		LzArchive_Free(ctx->archiveHandle);
	}

	free(ctx);
}

static WaykCseBundleStatus WaykCseBundle_ExtractSingleFile(
	WaykCseBundle* ctx,
	const char* targetFolder,
	const char* fileName)
{
	char outputPath[LZ_MAX_PATH];
	outputPath[0] = '\0';
	LzPathCchAppend(outputPath, sizeof(outputPath), targetFolder);
	LzPathCchAppend(outputPath, sizeof(outputPath), fileName);

	if (LzArchive_ExtractFile(ctx->archiveHandle, -1, fileName, outputPath) != LZ_OK)
	{
		fprintf(stderr, "Failed to extract %s from the bundle\n", fileName);
		return WAYK_CSE_BUNDLE_MISSING_PACKAGE;
	}

	return WAYK_CSE_BUNDLE_OK;
}

WaykCseBundleStatus WaykCseBundle_ExtractBrandingZip(
	WaykCseBundle* ctx,
	const char* targetFolder)
{
	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, BRANDING_FILE_NAME);
}

WaykCseBundleStatus WaykCseBundle_ExtractWaykNowExecutable(
	WaykCseBundle* ctx,
	WaykBinariesBitness bitness,
	const char* targetFolder)
{
	const char* sourceFile = (bitness == WAYK_BINARIES_BITNESS_X86)
		? WAYK_NOW_BINARY_FILE_NAME_X86
		: WAYK_NOW_BINARY_FILE_NAME_X64;

	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, sourceFile);
}

WaykCseBundleStatus WaykCseBundle_ExtractWaykNowInstaller(
	WaykCseBundle* ctx,
	WaykBinariesBitness bitness,
	const char* targetFolder)
{
	const char* sourceFile = (bitness == WAYK_BINARIES_BITNESS_X86)
		? INSTALLER_FILE_NAME_X86
		: INSTALLER_FILE_NAME_X64;

	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, sourceFile);
}

WaykCseBundleStatus WaykCseBundle_ExtractPowerShellInitScript(
	WaykCseBundle* ctx,
	const char* targetFolder)
{
	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, POWER_SHELL_INIT_SCRIPT_FILE_NAME);
}

WaykCseBundleStatus WaykCseBundle_ExtractOptionsJson(
	WaykCseBundle* ctx,
	const char* targetFolder)
{
	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, JSON_OPTIONS_FILE_NAME);
}