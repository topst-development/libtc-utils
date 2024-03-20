/****************************************************************************************
 *   FileName    : TCDBusRawAPI.h
 *   Description : This library that allows users to easily use the dbus
 ****************************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This library contains confidential information of Telechips.
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
#ifndef DBUS_RAW_API_H
#define DBUS_RAW_API_H

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ErrorCodeNoError,
	ErrorCodeUnknown,
	TotalErrorCodes
} DBusMsgErrorCode;

typedef DBusMsgErrorCode (*DBusMessageCallBack)(DBusMessage *message, const char *interface);
typedef void (*DBusInitializeCallBack)(void);

void InitializeRawDBusConnection(const char *name);
void ReleaseRawDBusConnection(void);
void SetCallBackFunctions(DBusMessageCallBack signalCall, DBusMessageCallBack methodCall);
void SetDBusInitCallBackFunctions(DBusInitializeCallBack busInitCall);
void SetDBusPrimaryOwner(const char *name);
int32_t AddSignalInterface(const char *interface);
int32_t AddMethodInterface(const char *interface);
DBusMessage *CreateDBusMsgMethodCall(const char *dest, const char *path, const char *interface, 
							   const char *method, int32_t firstType, ...);
DBusMessage *CreateDBusMsgMethodReturn(DBusMessage *reqMsg, int32_t firstType, ...);
DBusMessage *CreateDBusMsgSignal(const char *path, const char *interface, 
							   const char *signalName, int32_t firstType, ...);
int32_t SendDBusMessage(DBusMessage *message, DBusPendingCall **pending);
int32_t GetArgumentFromDBusPendingCall(DBusPendingCall *pending, int32_t firstType, ...);
int32_t GetArgumentFromDBusMessage(DBusMessage *message, int32_t firstType, ...);

#ifdef __cplusplus
}
#endif

#ifdef USE_SESSION_BUS
#define DBUS_BUS_TELECHIPS	DBUS_BUS_SESSION
#else
#define DBUS_BUS_TELECHIPS	DBUS_BUS_SYSTEM
#endif

#endif // DBUS_RAW_API_H
