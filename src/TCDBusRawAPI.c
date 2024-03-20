/****************************************************************************************
 *   FileName    : TCDBusRawAPI.c
 *   Description : This library that allows users to easily use the dbus
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include "TCDBusRawAPI.h"

#define FORCE_DISPATCH

#ifndef FORCE_DISPATCH
#include <glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

static void InitializeDBus(void);
static void ReleaseDBus(void);
static DBusHandlerResult DBusMessageFilter(DBusConnection *connection, DBusMessage *msg, void *userData);
static void *DBusDispatcher(void *arg);

static DBusConnection *g_dbusConnection = NULL;
static pthread_t g_dispatcher;

#ifdef FORCE_DISPATCH
static int32_t g_running = 0;
#else
static GMainLoop *g_threadEventLoop = NULL;
#endif

#define MAX_INTERFACE	(uint32_t)50
#define MAX_NAME_SIZE	32

static DBusMessageCallBack SignalCallBack = NULL;
static DBusMessageCallBack MethodCallBack = NULL;
static DBusInitializeCallBack BusInitCallBack = NULL;

static char *g_primaryBusName = NULL;
static char *g_signalInterfaces[MAX_INTERFACE];
static uint32_t g_signalCount = 0;

static char *g_methodInterfaces[MAX_INTERFACE];
static uint32_t g_methodCount = 0;

static char g_myName[MAX_NAME_SIZE];
static int32_t g_setName = 0;

static const char *g_errorCodeNames[TotalErrorCodes];
static const char *g_errorCodeMessages[TotalErrorCodes];


void InitializeRawDBusConnection(const char *name)
{
	int32_t err;
#ifdef FORCE_DISPATCH
	g_running = 1;
#else
	GMainContext *context;

	context = g_main_context_new();
	g_threadEventLoop = g_main_loop_new(context, FALSE);
	g_main_context_unref(context);
#endif

	if (name != NULL)
	{
		(void)strncpy(g_myName, name, MAX_NAME_SIZE);
		g_setName = 1;
	}

	err = pthread_create(&g_dispatcher, NULL, DBusDispatcher, NULL);
	if (err != 0)
	{
		(void)fprintf(stderr, "[%s] %s : create dbus dispatcher thread failed\n", 
				(g_setName == 1) ? g_myName : "NO NAME", __func__);
		ReleaseRawDBusConnection();
		exit(EXIT_FAILURE);
	}
}

void ReleaseRawDBusConnection(void)
{
	int32_t joinThread = 0;

#ifdef FORCE_DISPATCH
	if (g_running != 0)
	{
		joinThread = 1;
		g_running = 0;
	}

#else
	if (g_threadEventLoop != NULL)
	{
		joinThread = 1;
		g_main_loop_quit(g_threadEventLoop);
		g_threadEventLoop = NULL;
	}
#endif

	if (joinThread != 0)
	{
		int32_t err;
		void *res;

		err = pthread_join(g_dispatcher, &res);
		if (err != 0)
		{
			perror("dbus dispatcher thread joion faild: ");
		}
		else
		{
			(void)fprintf(stderr, "%s: DBusDispatcher thread exit\n", __func__);
		}
	}

	ReleaseDBus();
}

void SetCallBackFunctions(DBusMessageCallBack signalCall, DBusMessageCallBack methodCall)
{
	SignalCallBack = signalCall;
	MethodCallBack = methodCall;
}

void SetDBusInitCallBackFunctions(DBusInitializeCallBack busInitCall)
{
	BusInitCallBack = busInitCall;
}

void SetDBusPrimaryOwner(const char *name)
{
	uint32_t length = strlen(name);
	g_primaryBusName = (char *)malloc(length + (uint32_t)1);
	(void)memcpy(g_primaryBusName, name, length);
	g_primaryBusName[length] = '\0';
}

int32_t AddSignalInterface(const char *interface)
{
	int32_t added = 0;
	if ((g_signalCount < MAX_INTERFACE) &&
		(interface != NULL))
	{
		uint32_t length = strlen(interface);
		g_signalInterfaces[g_signalCount] = (char *)malloc(length + (uint32_t)1);
		(void)memcpy(g_signalInterfaces[g_signalCount], interface, length);
		g_signalInterfaces[g_signalCount][length] = '\0';

		g_signalCount++;
		added = 1;
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: insert interface failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return added;
}

int32_t AddMethodInterface(const char *interface)
{
	int32_t added = 0;
	if ((g_methodCount < MAX_INTERFACE) &&
		(interface != NULL))
	{
		uint32_t length = strlen(interface);
		g_methodInterfaces[g_methodCount] = (char *)malloc(length + (uint32_t)1);
		(void)memcpy(g_methodInterfaces[g_methodCount], interface, length);
		g_methodInterfaces[g_methodCount][length] = '\0';

		g_methodCount++;
		added = 1;
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: insert interface failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return added;
}

DBusMessage *CreateDBusMsgMethodCall(const char *dest, const char *path, const char *interface, 
									  const char *method, int32_t firstType, ...)
{
	DBusMessage *message = NULL;

	if (g_dbusConnection != NULL)
	{
		if ((dest != NULL) &&
			(path != NULL) &&
			(interface != NULL) &&
			(method != NULL))
		{
			message = dbus_message_new_method_call(dest, path, interface, method);
			if (message != NULL)
			{
				int32_t ret;
				va_list va;

				va_start(va, firstType);
				ret = (int32_t)dbus_message_append_args_valist(message, firstType, va);
				va_end(va);

				if (ret == 0)
				{
					(void)fprintf(stderr, "[%s] %s: dbus_message_append_args_valist failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
					dbus_message_unref(message);
					message = NULL;
				}
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: dbus_message_new_method_call failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
			}
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: one parameter is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: dbus connection not initialized\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return message;
}

DBusMessage *CreateDBusMsgMethodReturn(DBusMessage *reqMsg, int32_t firstType, ...)
{
	DBusMessage *message = NULL;

	if (g_dbusConnection != NULL)
	{
		if (reqMsg != NULL)
		{
			message = dbus_message_new_method_return(reqMsg);
			if (message != NULL)
			{
				int32_t ret;
				va_list va;

				va_start(va, firstType);
				ret = (int32_t)dbus_message_append_args_valist(message, firstType, va);
				va_end(va);

				if (ret == 0)
				{
					(void)fprintf(stderr, "[%s] %s: dbus_message_append_args_valist failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
					dbus_message_unref(message);
					message = NULL;
				}
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: dbus_message_new_method_call failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
			}
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: one parameter is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: dbus connection not initialized\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return message;
}

DBusMessage *CreateDBusMsgSignal(const char *path, const char *interface, 
							   const char *signalName, int32_t firstType, ...)
{
	DBusMessage *message = NULL;

	if (g_dbusConnection != NULL)
	{
		if ((path != NULL) &&
			(interface != NULL) &&
			(signalName != NULL))
		{
			message = dbus_message_new_signal(path, interface, signalName);
			if (message != NULL)
			{
				int32_t ret;
				va_list va;

				va_start(va, firstType);
				ret = (int32_t)dbus_message_append_args_valist(message, firstType, va);
				va_end(va);

				if (ret == 0)
				{
					(void)fprintf(stderr, "[%s] %s: dbus_message_append_args_valist failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
					dbus_message_unref(message);
					message = NULL;
				}
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: dbus_message_new_signal failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
			}
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: one parameter is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: dbus connection not initialized\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return message;
}

int32_t SendDBusMessage(DBusMessage *message, DBusPendingCall **pending)
{
	int32_t ret = 0;

	if (g_dbusConnection != NULL)
	{
		if (message != NULL)
		{
			int32_t type = dbus_message_get_type(message);

			if ((type == DBUS_MESSAGE_TYPE_SIGNAL) ||
				(type == DBUS_MESSAGE_TYPE_METHOD_RETURN) ||
				(type == DBUS_MESSAGE_TYPE_ERROR))
			{
				dbus_message_set_no_reply(message, TRUE);
				if (dbus_connection_send(g_dbusConnection, message, NULL) != (uint32_t)0) 
				{
					dbus_connection_flush(g_dbusConnection);
					ret = 1;
				}
				else
				{
					(void)fprintf(stderr, "[%s] %s: dbus_connection_send failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__); 
				}
			}
			else if (type == DBUS_MESSAGE_TYPE_METHOD_CALL)
			{
				if (pending != NULL)
				{
					if (dbus_connection_send_with_reply(g_dbusConnection, message, pending, DBUS_TIMEOUT_USE_DEFAULT) != (uint32_t)0)
					{
						dbus_connection_flush(g_dbusConnection);
						ret = 1;
					}
					else
					{
						(void)fprintf(stderr, "[%s] %s: dbus_connection_send_with_reply failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
					}
				}
				else
				{
					dbus_message_set_no_reply(message, TRUE);
					if (dbus_connection_send(g_dbusConnection, message, NULL) != (uint32_t)0) 
					{
						dbus_connection_flush(g_dbusConnection);
						ret = 1;
					}
					else
					{
						(void)fprintf(stderr, "[%s] %s: dbus_connection_send failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__); 
					}
				}
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: message is not surpported type(%d)\n", 
						(g_setName == 1) ? g_myName : "NO NAME", 
						__func__,
						type);
			}
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: message is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: dbus connection not initialized\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return ret;
}

int32_t GetArgumentFromDBusPendingCall(DBusPendingCall *pending, int32_t firstType, ...)
{
	int32_t type;
	int32_t ret = 0;
	int32_t retDBus;

	if (pending != NULL)
	{
		DBusMessage *message;

		dbus_pending_call_block(pending);
		message = dbus_pending_call_steal_reply(pending);
		if (message != NULL)
		{
			type = dbus_message_get_type(message);
			if (type == DBUS_MESSAGE_TYPE_ERROR)
			{
				DBusError error;
				const char *errorName;
				const char *errorMessage;

				dbus_error_init(&error);
				retDBus = (int32_t)dbus_message_get_args(message, &error,
											DBUS_TYPE_STRING, &errorMessage,
											DBUS_TYPE_INVALID);
				if (retDBus == 1)
				{
					errorName = dbus_message_get_error_name(message);

					(void)fprintf(stderr, "[%s] %s: received error message(%s:%s)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
							(errorName != NULL) ? errorName : "",
							(errorMessage != NULL) ? errorMessage : "");
					ret = 1;
				}
				else
				{
					(void)fprintf(stderr, "[%s] %s: dbus_message_get_args failed(%s)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
							((int32_t)dbus_error_is_set(&error) != 0) ? error.message : "unknow error");
				}
			}
			else if (type == DBUS_MESSAGE_TYPE_METHOD_RETURN)
			{
				DBusError error;
				va_list va;

				dbus_error_init(&error);
				va_start(va, firstType);
				retDBus = (int32_t)dbus_message_get_args_valist(message, &error, firstType, va);
				va_end(va);

				if (retDBus == 1)
				{
					ret = 1;
				}
				else
				{
					(void)fprintf(stderr, "[%s] %s: dbus_message_get_args_valist failed(%s)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
							((int32_t)dbus_error_is_set(&error) != 0) ? error.message : "unknow error");
				}
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: not supported message type(%d)\n", 
						(g_setName == 1) ? g_myName : "NO NAME", __func__, type);
			}
			dbus_message_unref(message);
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: dbus_pending_call_steal_reply failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: pending message is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return ret;
}

int32_t GetArgumentFromDBusMessage(DBusMessage *message, int32_t firstType, ...)
{
	int32_t ret = 0;

	if (message != NULL)
	{
		DBusError error;
		va_list va;

		dbus_error_init(&error);
		va_start(va, firstType);
		ret = (int32_t)dbus_message_get_args_valist(message, &error, firstType, va);
		va_end(va);	

		if (ret == 0)
		{
			(void)fprintf(stderr, "[%s] %s: dbus_message_get_args_valist failed(%s)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
					((int32_t)dbus_error_is_set(&error) != 0) ? error.message : "unknow error");
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: dbus message is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	return ret;
}

static void InitializeDBus(void)
{
	DBusError dbusError;
	char *rule;
	uint32_t length;
	uint32_t idx;
	int32_t ret;

	dbus_error_init(&dbusError);
	g_dbusConnection = dbus_bus_get(DBUS_BUS_TELECHIPS, &dbusError);

	if (dbus_error_is_set(&dbusError) != (uint32_t)0)
	{
		(void)fprintf(stderr, "[%s] %s: Cannot get DBUS(%d) connection: %s\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
				DBUS_BUS_TELECHIPS, dbusError.message);
		dbus_error_free(&dbusError);
		ReleaseRawDBusConnection();
		exit(EXIT_FAILURE);
	}

#ifndef FORCE_DISPATCH
	if (g_threadEventLoop != NULL)
	{
		dbus_connection_setup_with_g_main(g_dbusConnection, g_main_loop_get_context(g_threadEventLoop));
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: g_main_loop not created\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}
#endif

	if (g_primaryBusName != NULL)
	{
		ret = dbus_bus_request_name(g_dbusConnection, g_primaryBusName, DBUS_NAME_FLAG_REPLACE_EXISTING , &dbusError);
		if (dbus_error_is_set(&dbusError) != (uint32_t)0) 
		{
			(void)fprintf(stderr, "[%s] %s: dbus_bus_request_name(%s) failed. cause:%s\n", (g_setName == 1) ? g_myName : "NO NAME", __func__, 
					g_primaryBusName, dbusError.message);
			dbus_error_free(&dbusError);
			ReleaseRawDBusConnection();
			exit(EXIT_FAILURE);
		}

		if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) 
		{
			(void)fprintf(stderr, "[%s] %s: not primary(%s) owner\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
					g_primaryBusName);
			ReleaseRawDBusConnection();
			exit(EXIT_FAILURE);
		}
	}

	for (idx = 0; idx < g_signalCount; idx++)
	{
		length = (uint32_t)snprintf(NULL, 0, "type='signal',interface='%s'", g_signalInterfaces[idx]) + (uint32_t)1;
		rule = (char *)malloc(length);
		if (rule != NULL)
		{
			(void)snprintf(rule, length, "type='signal',interface='%s'", g_signalInterfaces[idx]);

			dbus_bus_add_match(g_dbusConnection, rule, &dbusError);
			if (dbus_error_is_set(&dbusError) != (uint32_t)0)
			{
				(void)fprintf(stderr, "[%s] %s: Cannot add D-BUS match rule(%s), cause: %s\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
						rule, dbusError.message);
				dbus_error_free(&dbusError);
				ReleaseRawDBusConnection();
				exit(EXIT_FAILURE);
			}
			dbus_connection_flush(g_dbusConnection);
			free(rule);
		}
		else
		{
			(void)fprintf(stderr, "[%s] %s: memory allocation failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
			ReleaseRawDBusConnection();
			exit(EXIT_FAILURE);
		}
	}

	(void)dbus_connection_add_filter(g_dbusConnection, DBusMessageFilter, NULL, NULL);
}

static void ReleaseDBus(void)
{
	uint32_t idx;

	if (g_dbusConnection != NULL)
	{
		dbus_connection_unref(g_dbusConnection);
		g_dbusConnection = NULL;
	}

	if (g_primaryBusName != NULL)
	{
		free(g_primaryBusName);
		g_primaryBusName = NULL;
	}

	for (idx = 0; idx < g_signalCount; idx++)
	{
		free(g_signalInterfaces[idx]);
		g_signalInterfaces[idx] = NULL;
	}
	g_signalCount = 0;

	for (idx = 0; idx < g_methodCount; idx++)
	{
		free(g_methodInterfaces[idx]);
		g_methodInterfaces[idx] = NULL;
	}
	g_methodCount = 0;
}

static DBusHandlerResult DBusMessageFilter(DBusConnection *connection, DBusMessage *msg, void *userData)
{
	const char *interface;
	int32_t messageType;
	DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if ((connection != NULL) && (msg != NULL))
	{
		interface = dbus_message_get_interface(msg); 
		messageType = dbus_message_get_type(msg);

		if (interface != NULL)
		{
			uint32_t idx;
			int32_t stop;
			DBusMsgErrorCode errorCode = ErrorCodeNoError;

			if (messageType == DBUS_MESSAGE_TYPE_SIGNAL/* 4  */)
			{
				stop = 0;
				for (idx = 0; (idx < g_signalCount) && (stop == 0); idx++)
				{
					if (strcmp(interface, g_signalInterfaces[idx]) == 0)
					{
						if (SignalCallBack != NULL)
						{
							errorCode = SignalCallBack(msg, g_signalInterfaces[idx]);
						}
						stop = 1;
						result = DBUS_HANDLER_RESULT_HANDLED;
					}
				}
			}
			else if (messageType == DBUS_MESSAGE_TYPE_METHOD_CALL/* 1 */)
			{
				stop = 0;
				for (idx = 0; (idx < g_methodCount) && (stop == 0); idx++)
				{
					if (strcmp(interface, g_methodInterfaces[idx]) == 0)
					{
						if (MethodCallBack != NULL)
						{
							errorCode = MethodCallBack(msg, g_methodInterfaces[idx]);
						}
						stop = 1;
						result = DBUS_HANDLER_RESULT_HANDLED;
					}
				}

				if (errorCode == ErrorCodeUnknown)
				{
					DBusMessage *returnMessage;
					returnMessage = CreateDBusMsgMethodReturn(msg, DBUS_TYPE_INVALID);
					if (returnMessage != NULL)
					{
						if (SendDBusMessage(returnMessage, NULL) != 0)
						{
							(void)fprintf(stderr, "[%s] %s: received unknow message call. temporary send return message\n", 
								(g_setName == 1) ? g_myName : "NO NAME", __func__);
						}
						else
						{
							(void)fprintf(stderr, "[%s] %s: sendDBusMessage failed\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
						}
						dbus_message_unref(returnMessage);
					}
				}
			}
			else if (messageType == DBUS_MESSAGE_TYPE_ERROR/* 3 */)
			{
				DBusError error;
				const char *errorName;
				const char *errorMessage;
				const char *sender, *path, *dest;
				int32_t ret;

				dbus_error_init(&error);
				ret = (int32_t)dbus_message_get_args(msg, &error,
											DBUS_TYPE_STRING, &errorMessage, 
											DBUS_TYPE_INVALID);
				if (ret == 0)
				{
					errorMessage = NULL;
				}
				errorName = dbus_message_get_error_name(msg);
				sender = dbus_message_get_sender(msg);
				path = dbus_message_get_path(msg);
				interface = dbus_message_get_interface(msg);
				dest = dbus_message_get_destination(msg);

				(void)fprintf(stderr, "[%s] %s: received error message(%s:%s)\n(type(%d), sender(%s), object path(%s), interface(%s), destination(%s)\n", 
						(g_setName == 1) ? g_myName : "NO NAME",
						__func__,
						(errorName != NULL) ? errorName : "",
						(errorMessage != NULL) ? errorMessage : "",
						messageType,
						(sender != NULL) ? sender : "NULL",
						(path != NULL) ? path : "NULL",
						(interface != NULL) ? interface : "NULL",
						(dest != NULL) ? dest : "NULL");

				result = DBUS_HANDLER_RESULT_HANDLED;
			}
			else if (messageType == DBUS_MESSAGE_TYPE_INVALID/* 0 */)
			{
				(void)fprintf(stderr, "[%s] %s: received invalid dbus message\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
			}
			else if (messageType == DBUS_MESSAGE_TYPE_METHOD_RETURN/* 2 */)
			{
				result = DBUS_HANDLER_RESULT_HANDLED;
			}
			else
			{
				(void)fprintf(stderr, "[%s] %s: unknow message type(%d)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
						messageType);
			}

			(void)errorCode;
#if 0
			if (errorCode != ErrorCodeNoError)
			{
				const char *sender, *path, *dest;
				DBusMessage *errorMessage;

				(void)fprintf(stderr, "[%s] %s: create error message(%s:%s)\n", (g_setName == 1) ? g_myName : "NO NAME", __func__,
						g_errorCodeNames[errorCode], 
						g_errorCodeMessages[errorCode]);
				errorMessage = dbus_message_new_error(msg, 
													  g_errorCodeNames[errorCode], 
													  g_errorCodeMessages[errorCode]);
				if (errorMessage != NULL)
				{
					(void)fprintf(stderr, "[%s] %s: send error message\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
					if (SendDBusMessage(errorMessage, NULL) == 0)
					{
						(void)fprintf(stderr, "%s: SendDBusMessage failed\n", __FUNCTION__);
					}
					dbus_message_unref(errorMessage);
				}
				sender = dbus_message_get_sender(msg);
				path = dbus_message_get_path(msg);
				interface = dbus_message_get_interface(msg);
				dest = dbus_message_get_destination(msg);

				(void)fprintf(stderr, "[%s] %s: %s(%s) (type(%d), sender(%s), object path(%s), interface(%s), destination(%s)\n", 
						(g_setName == 1) ? g_myName : "NO NAME",
						__func__,
						g_errorCodeNames[errorCode],
						g_errorCodeMessages[errorCode],
						messageType,
						(sender != NULL) ? sender : "NULL",
						(path != NULL) ? path : "NULL",
						(interface != NULL) ? interface : "NULL",
						(dest != NULL) ? dest : "NULL");
			}
#endif
		}
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: connection or dbus message is NULL\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
	}

	(void)userData;

	return result;
}

static void *DBusDispatcher(void *arg)
{
	static int32_t ret = 0;
	InitializeDBus();
	if (g_dbusConnection != NULL)
	{
		if (BusInitCallBack != NULL)
		{
			BusInitCallBack();
		}
	}

#ifdef FORCE_DISPATCH
	while (g_running != 0)
	{
		if (g_dbusConnection != NULL)
		{
			(void)dbus_connection_read_write_dispatch(g_dbusConnection, 1);
		}
		(void)usleep(10000);
	}
#else
	if (g_threadEventLoop != NULL)
	{
		g_main_loop_run(g_threadEventLoop);
	}
	else
	{
		(void)fprintf(stderr, "[%s] %s: g_main_loop not created\n", (g_setName == 1) ? g_myName : "NO NAME", __func__);
		ret = -1;
	}
#endif

    (void)arg;
    pthread_exit((void *)&ret);
}

static const char *g_errorCodeNames[TotalErrorCodes] = {
	"No Error",
	"Unknown Message"
};

static const char *g_errorCodeMessages[TotalErrorCodes] = {
	"",
	"check your message name"
};
