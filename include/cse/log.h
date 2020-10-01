#ifndef WAYKCSE_LOG_H
#define WAYKCSE_LOG_H

#include <stdarg.h>
#include <stdio.h>

typedef enum
{
	CSE_LOG_LEVEL_TRACE,
	CSE_LOG_LEVEL_DEBUG,
	CSE_LOG_LEVEL_INFO,
	CSE_LOG_LEVEL_WARN,
	CSE_LOG_LEVEL_ERROR
} CseLogLevel;

#define CSE_LOG_TRACE(...) CseLog_Message(CSE_LOG_LEVEL_TRACE, CSE_LOG_TAG, __LINE__, __VA_ARGS__)
#define CSE_LOG_DEBUG(...) CseLog_Message(CSE_LOG_LEVEL_DEBUG, CSE_LOG_TAG, __LINE__, __VA_ARGS__)
#define CSE_LOG_INFO(...)  CseLog_Message(CSE_LOG_LEVEL_INFO,  CSE_LOG_TAG, __LINE__, __VA_ARGS__)
#define CSE_LOG_WARN(...)  CseLog_Message(CSE_LOG_LEVEL_WARN,  CSE_LOG_TAG, __LINE__, __VA_ARGS__)
#define CSE_LOG_ERROR(...) CseLog_Message(CSE_LOG_LEVEL_ERROR, CSE_LOG_TAG, __LINE__, __VA_ARGS__)


void CseLog_Init(FILE* outputFile, CseLogLevel logLevel);
void CseLog_Message(CseLogLevel level, const char *file, int line, const char *fmt, ...);

#endif //WAYKCSE_LOG_H
