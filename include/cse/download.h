#ifndef WAYKCSE_DOWNLOAD_H
#define WAYKCSE_DOWNLOAD_H

#include <cse/bundle.h>

typedef enum
{
	CSE_DOWNLOAD_OK,
	CSE_DOWNLOAD_FAILURE,
	CSE_DOWNLOAD_PARAM,
	CSE_DOWNLOAD_NOMEM,
} CseDownloadResult;

CseDownloadResult CseDownload_DownloadMsi(WaykBinariesBitness bitness, const char* msiPath);

#endif //WAYKCSE_DOWNLOAD_H
