/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "LoggerStdio"
#include <imagine/base/Base.hh>
#include <imagine/fs/sys.hh>
#include <imagine/logger/logger.h>
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __APPLE__
#include <asl.h>
#endif

static const bool bufferLogLineOutput = Config::envIsAndroid || Config::envIsIOS;
static char logLineBuffer[512] {0};
uint loggerVerbosity = loggerMaxVerbosity;
static const bool useExternalLogFile = false;
static FILE *logExternalFile = nullptr;
static bool logEnabled = Config::DEBUG_BUILD; // default logging off in release builds

static void printExternalLogPath(FsSys::PathString &path)
{
	const char *prefix = ".";
	if(Config::envIsIOS)
		prefix = "/var/mobile";
	else if(Config::envIsAndroid)
		prefix = Base::storagePath();
	else if(Config::envIsWebOS)
		prefix = "/media/internal";
	string_printf(path, "%s/imagine.log", prefix);
}

CallResult logger_init()
{
	if(!logEnabled)
		return OK;
	if(useExternalLogFile && !logExternalFile)
	{
		FsSys::PathString path;
		printExternalLogPath(path);
		logMsg("external log file: %s", path.data());
		logExternalFile = fopen(path.data(), "wb");
		if(!logExternalFile)
		{
			return IO_ERROR;
		}
	}

	//logMsg("init logger");
	return OK;
}

void logger_setEnabled(bool enable)
{
	logEnabled = enable;
	if(enable)
	{
		logger_init();
	}
}

bool logger_isEnabled()
{
	return logEnabled;
}

static void printToLogLineBuffer(const char* msg, va_list args)
{
	vsnprintf(logLineBuffer + strlen(logLineBuffer), sizeof(logLineBuffer) - strlen(logLineBuffer), msg, args);
}

void logger_vprintf(LoggerSeverity severity, const char* msg, va_list args)
{
	if(!logEnabled)
		return;
	if(severity > loggerVerbosity) return;

	if(logExternalFile)
	{
		va_list args2;
		va_copy(args2, args);
		vfprintf(logExternalFile, msg, args2);
		va_end(args2);
		fflush(logExternalFile);
	}

	if(bufferLogLineOutput && !strchr(msg, '\n'))
	{
		printToLogLineBuffer(msg, args);
		return;
	}

	#ifdef __ANDROID__
	if(strlen(logLineBuffer))
	{
		printToLogLineBuffer(msg, args);
		__android_log_write(ANDROID_LOG_INFO, "imagine", logLineBuffer);
		logLineBuffer[0] = 0;
	}
	else
		__android_log_vprint(ANDROID_LOG_INFO, "imagine", msg, args);
	#elif defined CONFIG_BASE_IOS
	if(strlen(logLineBuffer))
	{
		printToLogLineBuffer(msg, args);
		asl_log(nullptr, nullptr, ASL_LEVEL_NOTICE, "%s", logLineBuffer);
		logLineBuffer[0] = 0;
	}
	else
		asl_vlog(nullptr, nullptr, ASL_LEVEL_NOTICE, msg, args);
	#else
	vfprintf(stderr, msg, args);
	#endif
}

void logger_printf(LoggerSeverity severity, const char* msg, ...)
{
	if(!logEnabled)
		return;
	va_list args;
	va_start(args, msg);
	logger_vprintf(severity, msg, args);
	va_end(args);
}
