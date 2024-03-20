/****************************************************************************************
 *   FileName    : TCLog.h
 *   Description : 
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited 
to re-distribution in source or binary form is strictly prohibited.
This source code is provided ¡°AS IS¡± and nothing contained in this source code 
shall constitute any express or implied warranty of any kind, including without limitation, 
any warranty of merchantability, fitness for a particular purpose or non-infringement of any patent, 
copyright or other third party intellectual property right. 
No warranty is made, express or implied, regarding the information¡¯s accuracy, 
completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, 
out of or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************************/

#ifndef _TC_LOG_H
#define _TC_LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TCLogLevelError,
	TCLogLevelWarn,
	TCLogLevelInfo,
	TCLogLevelDebug,
	TotalTCLogLevels
} TCLogLevel;

void TCLogInitialize(const char *prefix, const char *sub_prefix, int use_time);
void TCEnableLog(int enable);
FILE *TCRedirectLog(FILE *fp);
void TCLogSetFileName(const char *name);
void TCLogSetLevel(int level);
int TCLog(TCLogLevel level, const char *format, ...);
int TCLogHex(TCLogLevel level, const void *buffer, unsigned int length, const char *title);
int getFileLength(FILE *fp);
double GetTotalFreeSpaceRate();
FILE *TCLogFilePtr();
int OpenFile(int year, int month, int day, int hour);

#ifdef __cplusplus
}
#endif
#endif // _TC_LOG_H
