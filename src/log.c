#include <cse/log.h>

#include <time.h>

typedef struct
{
	va_list args;
	const char *fmt;
	const char *fileName;
	int lineNum;
	struct tm time;
	int level;
} LogMessageContext;

typedef void (*log_LogFn)(LogMessageContext *ev);

static struct
{
	CseLogLevel level;
	log_LogFn* logFn;
	FILE* outputFile;
} CseLog;

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};

static void WriteLogInternal(LogMessageContext* ctx)
{
	char buf[64];
	buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ctx->time)] = '\0';
	fprintf(
		CseLog.outputFile,
		"%s [%s] %s:%d: ",
		buf,
		level_strings[ctx->level],
		ctx->fileName,
		ctx->lineNum);
	vfprintf(CseLog.outputFile, ctx->fmt, ctx->args);
	fprintf(CseLog.outputFile, "\n");
	fflush(CseLog.outputFile);
}

void CseLog_Message(CseLogLevel level, const char *file, int line, const char *fmt, ...)
{
	if (!CseLog.outputFile)
	{
		return;
	}

	if (level >= CseLog.level)
	{
		LogMessageContext ctx =
		{
			.fmt   = fmt,
			.fileName  = file,
			.lineNum  = line,
			.level = level,
		};

		time_t now = time(0);
		localtime_s(&ctx.time, &now);

		va_start(ctx.args, fmt);
		WriteLogInternal(&ctx);
		va_end(ctx.args);
	}
}

void CseLog_Init(FILE* outputFile, CseLogLevel logLevel)
{
	CseLog.outputFile = outputFile;
	CseLog.level = logLevel;
}