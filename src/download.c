#include <cse/download.h>
#include <cse/log.h>

#include <lizard/lizard.h>

#define CSE_LOG_TAG "CseDownload"

#define CSE_USER_AGENT "WaykCse"

int CseDownload_OnWriteFile(void* param, LzHttp* ctx, uint8_t* data, DWORD len)
{
	FILE* fp = (FILE*) param;
	fwrite(data, sizeof(uint8_t), len, fp);

	return 0;
}

CseDownloadResult CseDownload_DownloadMsi(WaykBinariesBitness bitness, const char* msiPath)
{
	CseDownloadResult result = CSE_DOWNLOAD_FAILURE;
	int status;
	FILE* fp = NULL;
	WCHAR* msiPathW = NULL;
	LzHttp* http = NULL;
	char* response = NULL;
	uint32_t error = 0;
	const char* keyLine;
	const char* keyValue;
	char key[128];
	char msiUrl[MAX_PATH];
	int msiUrlLen = 0;

	snprintf(key, sizeof(key), "WaykAgentmsi%s.Url", bitness == WAYK_BINARIES_BITNESS_X64 ? "64" : "86");

	msiPathW = LzUnicode_UTF8toUTF16_dup(msiPath);

	if (!msiPathW)
	{
		result = CSE_DOWNLOAD_PARAM;
		goto exit;
	}

	http = LzHttp_New(CSE_USER_AGENT);

	if (!http)
	{
		result = CSE_DOWNLOAD_NOMEM;
		goto exit;
	}

	CSE_LOG_INFO("Requesting MSI URL");

	status = LzHttp_Get(http, "https://www.devolutions.net/productinfo.htm", NULL, NULL, &error);

	if (status != LZ_OK)
	{
		CSE_LOG_ERROR("HTTP request failed %d (%lu)", status, error);
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	response = LzHttp_Response(http);

	if (!response)
	{
		CSE_LOG_ERROR("Bad HTTP response");
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	keyLine = strstr(response, key);

	if (!keyLine)
	{
		CSE_LOG_ERROR("Bad HTTP response. Key %s not found.", key);
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	keyValue = strstr(keyLine, "=");

	if (!keyValue)
	{
		CSE_LOG_ERROR("Bad HTTP response. Key value not found.");
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	keyValue++;

	while ((*keyValue != 0) && (*keyValue != '\r'))
	{
		msiUrl[msiUrlLen++] = *keyValue;
		keyValue++;
	}

	msiUrl[msiUrlLen] = '\0';

	LzHttp_Free(http);
	http = NULL;

	fp = _wfopen(msiPathW, L"wb");

	if (!fp)
	{
		CSE_LOG_ERROR("Failed to open temporary file: %s", msiPath);
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	http = LzHttp_New(CSE_USER_AGENT);

	if (!http)
	{
		result = CSE_DOWNLOAD_NOMEM;
		goto exit;
	}

	CSE_LOG_INFO("Downloading MSI from %s", msiUrl);

	LzHttp_SetRecvTimeout(http, 600 * 1000);
	status = LzHttp_Get(http, msiUrl, CseDownload_OnWriteFile, fp, error);

	if (status != LZ_OK)
	{
		CSE_LOG_ERROR("HTTP request failed %d (%lu)", status, error);
		result = CSE_DOWNLOAD_FAILURE;
		goto exit;
	}

	CSE_LOG_INFO("Downloaded MSI", msiUrl);

	result = CSE_DOWNLOAD_OK;

exit:
	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if (msiPathW)
	{
		free(msiPathW);
		msiPathW = NULL;
	}

	if (http)
	{
		LzHttp_Free(http);
		http = NULL;
	}

	return result;
}