/****************************************************************************************
 *   FileName    : TCLog.c
 *   Description : 
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided “AS IS” and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information’s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pwd.h>

#include "TCLog.h"

#define MAX_LOG_FILE_SIZE	10485760 // 10 MB
#define MAX_STRING_SIZE		256

static FILE *tc_internal_logFp = NULL;
static pthread_mutex_t g_logMutex;
static pthread_mutex_t* g_logMutexPtr = NULL;
static char g_stringBuffer[5][MAX_STRING_SIZE];
static char *g_fileDir = (char *)&g_stringBuffer[0];
static char *g_fileName =(char *)&g_stringBuffer[1];
static char *g_filePath =(char *)&g_stringBuffer[2];
static char *g_prefix = NULL;
static char *g_sub_prefix = NULL;
static unsigned int g_level = TCLogLevelInfo;
static int g_enable = 0;
static int g_use_time = 0;
static int g_fileIndex = 0;
static int g_fileHour = -1;
static int g_freeSpaceErrorCnt = 0;
static const char *g_logLevelNames[TotalTCLogLevels] = {
	"ERROR",
	"WARN",
	"INFO",
	"DEBUG"
};

void TCLogInitialize(const char *prefix, const char *sub_prefix, int use_time)
{
	int err;
	struct passwd *pw;

	if (prefix != NULL)
	{
		g_prefix = (char *)&g_stringBuffer[2];
		memset(g_prefix, 0x00, MAX_STRING_SIZE);
		strncpy(g_prefix, prefix, MAX_STRING_SIZE - 1);
	}

	if (sub_prefix != NULL)
	{
		g_sub_prefix = (char *)&g_stringBuffer[3];
		memset(g_sub_prefix, 0x00, MAX_STRING_SIZE);
		strncpy(g_sub_prefix, sub_prefix, MAX_STRING_SIZE - 1);
	}

	err = pthread_mutex_init(&g_logMutex, NULL);
	if (err == 0)
	{
		g_logMutexPtr = &g_logMutex;
		TCEnableLog(1);
	}
	else
	{
		perror("pthread_mutexg_init failed: ");
		fprintf(stderr, "TCLog disabled\n");
		TCEnableLog(0);
	}

	g_use_time = (use_time != 0);

	pw = getpwuid(getuid());
	memset(g_fileDir, 0x00, MAX_STRING_SIZE);
	if (pw != NULL)
	{
		strncpy(g_fileDir, pw->pw_dir, MAX_STRING_SIZE - 1);
	}
	else
	{
		fprintf(stderr, "%s: get user home directory failed.\n", __func__);
		strncpy(g_fileDir, "/run/log", MAX_STRING_SIZE - 1);
	}

	tc_internal_logFp = stdout;
}

void TCEnableLog(int enable)
{
	g_enable = enable;
}

FILE *TCRedirectLog(FILE *fp)
{
	FILE *oldFp = tc_internal_logFp;
	tc_internal_logFp = fp;
	return oldFp;
}

void TCLogSetFileName(const char *name)
{
	if (name != NULL)
	{
		if (tc_internal_logFp != stdout && tc_internal_logFp != NULL)
			fclose(tc_internal_logFp);

		memset(g_fileName, 0x00, MAX_STRING_SIZE);
		strncpy(g_fileName, name, MAX_STRING_SIZE - 1);
	}
	else
	{
		fprintf(stderr, "%s: set file name failed. name is null pointer\n", __func__);
	}
}

void TCLogSetLevel(int level)
{
	if (level >= TCLogLevelError && level < TotalTCLogLevels)
	{
		g_level = level;
	}
	else
	{
		fprintf(stderr, "%s: set log level failed\n", __func__);
	}
}

int TCLog(TCLogLevel level, const char *format, ...)
{
	int	printLog = ((level >= TCLogLevelError) && (level <= g_level)) ? 1 : 0;
	if ((g_enable != 0) && (printLog != 0))
	{
		int year, mon, day, hour, min, ms;
		double second;
		time_t timeNow;
		va_list va;

		(void)pthread_mutex_lock(g_logMutexPtr);
		struct timeval timeVal;
		struct tm *tmData;
		gettimeofday(&timeVal, NULL);
		timeNow = timeVal.tv_sec;
		ms = (int)(timeVal.tv_usec / 1000.0 + 0.5);
		if (ms >= 1000)
		{
			timeNow++;
			ms %= 1000;
		}

		tmData = localtime(&timeNow);

		year = tmData->tm_year + 1900;
		mon = tmData->tm_mon + 1;
		day =  tmData->tm_mday;
		hour = tmData->tm_hour;
		min = tmData->tm_min;
		second = tmData->tm_sec + ms / 1000.0;

		printLog = OpenFile(year, mon, day, hour);

		if (printLog != 0)
		{
			if (g_use_time != 0)
			{
				fprintf(tc_internal_logFp, "[%d-%02d-%02d %02d:%02d:%02.03f]",
						year, mon, day, hour, min, second);
			}

			if (g_prefix != NULL && g_sub_prefix != NULL)
			{
				fprintf(tc_internal_logFp, "[%s][%s][%s] ",
						g_logLevelNames[level], g_prefix, g_sub_prefix);
			}
			else if (g_prefix != NULL)
			{
				fprintf(tc_internal_logFp, "[%s][%s] ",
						g_logLevelNames[level], g_prefix);
			}
			else
			{
				fprintf(tc_internal_logFp, "[%s][NO NAME] ",
						g_logLevelNames[level]);
			}

			va_start(va, format);
			vfprintf(tc_internal_logFp, format, va);
			fflush(tc_internal_logFp);
			if (tc_internal_logFp != stdout)
			{
				fclose(tc_internal_logFp);
				tc_internal_logFp = NULL;
			}
			va_end(va);
		}

		(void)pthread_mutex_unlock(g_logMutexPtr);
	}
	else
		printLog = 0;

	return printLog;
}

int TCLogHex(TCLogLevel level, const void *buffer, unsigned int length, const char *title)
{
	int	printLog = ((level >= TCLogLevelError) && (level < g_level)) ? 1 : 0;
	if ((g_enable != 0) && (printLog != 0))
	{
		int year, mon, day, hour, ms;
		time_t timeNow;

		(void)pthread_mutex_lock(g_logMutexPtr);
		struct timeval timeVal;
		struct tm *tmData;

		gettimeofday(&timeVal, NULL);
		timeNow = timeVal.tv_sec;
		ms = (int)(timeVal.tv_usec / 1000.0 + 0.5);
		if (ms >= 1000)
		{
			timeNow++;
			ms %= 1000;
		}

		tmData = localtime(&timeNow);

		year = tmData->tm_year + 1900;
		mon = tmData->tm_mon + 1;
		day =  tmData->tm_mday;
		hour = tmData->tm_hour;

		printLog = OpenFile(year, mon, day, hour);

		if (printLog != 0)
		{
			unsigned char *bufp = (unsigned char *)buffer;
			unsigned int i;
			fprintf(tc_internal_logFp, "\n    %s%sHEXDUMP [%u BYTES]\n             | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n",
					title != NULL ? title : "", title != NULL ? " " : "", length);
			fprintf(tc_internal_logFp, "    ---------+------------------------------------------------");

			for (i = 0; i < length; i++)
			{
				if (i % 16 == 0)
					fprintf(tc_internal_logFp, "\n    %08X | ", i);
				fprintf(tc_internal_logFp, "%02X ", bufp[i]);
			}
			fprintf(tc_internal_logFp, "\n\n");
			fflush(tc_internal_logFp);
			if (tc_internal_logFp != stdout)
			{
				fclose(tc_internal_logFp);
				tc_internal_logFp = NULL;
			}
		}

		(void)pthread_mutex_unlock(g_logMutexPtr);
	}
	else
		printLog = 0;

	return printLog;
}

int getFileLength(FILE *fp)
{
	return ftell(fp);
}

double GetTotalFreeSpaceRate()
{
	double spaceRate = -1.0;
	if (g_logMutexPtr != NULL)
	{
		double free, total;
		struct statfs stfs;

		(void)pthread_mutex_lock(g_logMutexPtr);
		if (statfs(g_fileDir, &stfs) == 0)
		{
			free = (double)stfs.f_bavail;
			total = (double)stfs.f_blocks;

			if ((free > 0.0) && (total > 0.0))
			{
				g_freeSpaceErrorCnt = 0;
				spaceRate =  free / total * 100;
			}
			else
			{
				g_freeSpaceErrorCnt++;
			}
		}
		else
		{
			fprintf(stderr, "%s: statfs(%s) failed\n", __func__, g_fileDir);
		}
		(void)pthread_mutex_unlock(g_logMutexPtr);
	}
	return spaceRate;
}

FILE *TCLogFilePtr()
{
	if (g_enable != 0)
	{
		if (tc_internal_logFp == NULL)
		{
			struct tm *tmData;
			time_t timeNow;
			int year, mon, day, hour, ms;
			struct timeval timeVal;

			gettimeofday(&timeVal, NULL);
			timeNow = timeVal.tv_sec;
			ms = (int)(timeVal.tv_usec / 1000.0 + 0.5);
			if (ms >= 1000)
			{
				timeNow++;
				ms %= 1000;
			}

			tmData = localtime(&timeNow);
			year = tmData->tm_year + 1900;
			mon = tmData->tm_mon + 1;
			day =  tmData->tm_mday;
			hour = tmData->tm_hour;

			OpenFile(year, mon, day, hour);
		}

		return tc_internal_logFp;
	}
	else
		return NULL;
}

int OpenFile(int year, int month, int day, int hour)
{
	int opened;
	if (g_enable != 0)
	{
		if (g_freeSpaceErrorCnt >= 10 && GetTotalFreeSpaceRate() < 5.0)
		{
			fprintf(stderr, "can not append logs because current partition`s available space was less than 5 percents\n");
			if (tc_internal_logFp != NULL && tc_internal_logFp != stdout)
			{
				fclose(tc_internal_logFp);
			}
			tc_internal_logFp = NULL;
			opened = 0;
		}
		else
		{
			if (tc_internal_logFp != stdout)
			{
				snprintf(g_filePath, MAX_STRING_SIZE, "%s-%d%02d%02d%02d_%d.log",
						g_fileName, year, month, day, hour, g_fileIndex);

				if (g_fileHour == -1)
				{
					g_fileHour = hour;
				}
				else if (g_fileHour != hour)
				{
					g_fileHour = hour;
					g_fileIndex = 0;
				}

				if (access(g_filePath, F_OK) == 0)
				{
					tc_internal_logFp = fopen(g_filePath, "a");
				}
				else
				{
					tc_internal_logFp = fopen(g_filePath, "w");
				}

				if (tc_internal_logFp != NULL && getFileLength(tc_internal_logFp) > MAX_LOG_FILE_SIZE)
				{
					fclose(tc_internal_logFp);

					g_fileIndex++;
					snprintf(g_filePath, MAX_STRING_SIZE, "%s-%d%02d%02d%02d_%d.log",
							g_fileName, year, month, day, hour, g_fileIndex);
					tc_internal_logFp = fopen(g_filePath, "w");
				}
			}

			if (tc_internal_logFp == NULL)
			{
				opened = 0;
			}
			else
			{
				opened = 1;
			}
		}
	}
	else
	{
		opened = 0;
	}

	return opened;
}
