#ifndef WAYKCSE_LOG_H
#define WAYKCSE_LOG_H

#include <stdarg.h>
#include <stdio.h>

enum
{
	CSE_LOG_LEVEL_TRACE,
	CSE_LOG_LEVEL_DEBUG,
	CSE_LOG_LEVEL_INFO,
	CSE_LOG_LEVEL_WARN,
	CSE_LOG_LEVEL_ERROR
};

#define CSE_LOG_TRACE(...) CseLog_Message(CSE_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define CSE_LOG_DEBUG(...) CseLog_Message(CSE_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define CSE_LOG_INFO(...)  CseLog_Message(CSE_LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define CSE_LOG_WARN(...)  CseLog_Message(CSE_LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define CSE_LOG_ERROR(...) CseLog_Message(CSE_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)


void CseLog_Init(FILE* outputFile, int logLevel);
void CseLog_Message(int level, const char *file, int line, const char *fmt, ...);

#endif //WAYKCSE_LOG_H
