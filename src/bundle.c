#include <cse/bundle.h>

#include <stdio.h>
#include <wchar.h>
#include <lizard/lizard.h>

#include <resource.h>

#define BRANDING_FILE_NAME "branding.zip"
#define POWER_SHELL_INIT_SCRIPT_FILE_NAME "init.ps1"
#define POWER_SHELL_MODULE_DIR_NAME "PowerShell"

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

WaykCseBundleStatus WaykCseBundle_ExtractPowerShellInitScript(
	WaykCseBundle* ctx,
	const char* targetFolder)
{
	return WaykCseBundle_ExtractSingleFile(ctx, targetFolder, POWER_SHELL_INIT_SCRIPT_FILE_NAME);
}

static bool IsStringPrefixedWith(const char *str, const char *prefix)
{
	return strncmp(prefix, str, strlen(prefix)) == 0;
}

static void TrimPrefix(char *str, const char *prefix)
{
	unsigned int prefixLen = strlen(prefix);
	unsigned int strLen = strlen(str);

	if (prefixLen >= strLen)
	{
		str[0] = '\0';
		return;
	}

	if ((str[prefixLen] == '\\') || (str[prefixLen] == '/'))
	{
		++prefixLen;
	}

	unsigned int oldIdx = prefixLen, newIdx = 0;
	for (; oldIdx < strLen; ++oldIdx, ++newIdx)
	{
		str[newIdx] = str[oldIdx];
	}

	str[newIdx] = '\0';
}

static WaykCseBundleStatus WaykCseBundle_ExtractRecursive(
	WaykCseBundle* ctx,
	const char* targetFolder,
	const char* sourceFolder)
{
	WaykCseBundleStatus status = WAYK_CSE_BUNDLE_OK;

	// Make directory structure
	for (int fileIdx = 0; fileIdx < LzArchive_Count(ctx->archiveHandle); ++fileIdx)
	{
		if (LzArchive_IsDir(ctx->archiveHandle, fileIdx))
		{
			char fileName[LZ_MAX_PATH];
			fileName[0] = '\0';
			LzArchive_GetFileName(ctx->archiveHandle, fileIdx, fileName, sizeof(fileName));

			if (!IsStringPrefixedWith(fileName, sourceFolder))
			{
				continue;
			}

			TrimPrefix(fileName, sourceFolder);

			char outputPath[LZ_MAX_PATH];
			outputPath[0] = '\0';
			LzPathCchAppend(outputPath, sizeof(outputPath), targetFolder);
			LzPathCchAppend(outputPath, sizeof(outputPath), fileName);

			if (!LzFile_Exists(outputPath))
			{
				if (LzMkDir(outputPath,0) < 0)
				{
					status = WAYK_CSE_BUNDLE_FS_ERROR;
					goto cleanup;
				}
			}
		}
	}

	// Extract files
	for (int fileIdx = 0; fileIdx < LzArchive_Count(ctx->archiveHandle); ++fileIdx)
	{
		if (!LzArchive_IsDir(ctx->archiveHandle, fileIdx))
		{
			char fileName[LZ_MAX_PATH];
			fileName[0] = '\0';
			LzArchive_GetFileName(ctx->archiveHandle, fileIdx, fileName, sizeof(fileName));

			if (!IsStringPrefixedWith(fileName, sourceFolder))
			{
				continue;
			}

			TrimPrefix(fileName, sourceFolder);

			char outputPath[LZ_MAX_PATH];
			outputPath[0] = '\0';
			LzPathCchAppend(outputPath, sizeof(outputPath), targetFolder);
			LzPathCchAppend(outputPath, sizeof(outputPath), fileName);

			if (LzArchive_ExtractFile(ctx->archiveHandle, fileIdx, 0, outputPath) != LZ_OK)
			{
				fprintf(stderr, "Failed to extract %s from the bundle\n", fileName);
				status = WAYK_CSE_BUNDLE_MISSING_PACKAGE;
				goto cleanup;
			}
		}
	}

cleanup:
	return status;
}

WaykCseBundleStatus WaykCseBundle_ExtractPowerShelModule(
	WaykCseBundle* ctx,
	const char* targetFolder)
{
	char outputPath[LZ_MAX_PATH];
	outputPath[0] = '\0';
	LzPathCchAppend(outputPath, sizeof(outputPath), targetFolder);
	LzPathCchAppend(outputPath, sizeof(outputPath), POWER_SHELL_MODULE_DIR_NAME);

	if (!LzFile_Exists(outputPath) && LzMkDir(outputPath, 0) < 0)
	{
		return WAYK_CSE_BUNDLE_FS_ERROR;
	}

	return WaykCseBundle_ExtractRecursive(ctx, outputPath, "PowerShell");
}

WaykCseBundleStatus WaykCseBundle_ExtractWaykBinaries(
	WaykCseBundle* ctx,
	const char* targetFolder,
	WaykBinariesBitness bitness)
{
	const char* sourceFolder = (bitness == WAYK_BINARIES_BITNESS_X86)
		? "Wayk_x86"
		: "Wayk_x64";

	return WaykCseBundle_ExtractRecursive(ctx, targetFolder, sourceFolder);
}