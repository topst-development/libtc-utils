#include <stdio.h>
#include <stdint.h>
#include <dbus/dbus.h>
#include "TCDBusRawAPI.h"
#include "TCInput.h"

static DBusMsgErrorCode OnReceivedDBusSignal(DBusMessage *message, const char *interface);
static DBusMsgErrorCode OnReceivedMethodCall(DBusMessage *message, const char *interface);
static void OnKeyboardPressed(int32_t key);
static void OnKeyboardLongPressed(int32_t key);
static void OnKeyboardLongLongPressed(int32_t key);
static void OnKeyboardReleased(int32_t key);
static void OnKeyboardClicked(int32_t key);
static void OnRotaryEvent(int32_t key);

void main(void)
{
	DBusMessage *message;
	int32_t value = 0;

	InitializeRawDBusConnection("EXAMPLE");
	SetCallBackFunctions(OnReceivedDBusSignal, OnReceivedMethodCall);
	SetDBusPrimaryOwner("EXAMPLE DBUS NAME");
	(void)AddSignalInterface("example signal interface");
	(void)AddMethodInterface("example method interface");

	message = CreateDBusMsgSignal("example object path", "example event interface",
									  "example signal name",
									  DBUS_TYPE_INT32, &value,
									  DBUS_TYPE_INVALID);
	if (message != NULL)
	{
		/*
	 	this is server side
		*/
		if (SendDBusMessage(message, NULL) != 0)
		{
			(void)printf("send mesage success\n");
		}
		else
		{
			(void)printf("send mesage failed\n");
		}

		/*
	 	this is client side
		*/
		if (GetArgumentFromDBusMessage(message,
					DBUS_TYPE_INT32, &value,
					DBUS_TYPE_INVALID) != 0)
		{
			(void)printf("GetArgumentFromDBusMessage value : %d \n", value);
		}
		else
		{
			(void)printf("GetArgumentFromDBusMessage failed\n");
		}

		dbus_message_unref(message);
	}
	else
	{
		(void)printf("CreateDBusMsgSignal failed\n");
	}

	message = CreateDBusMsgMethodCall("example dbus server name", "example object path",
										"example event interface", "example method name",
									  DBUS_TYPE_INT32, value,
									  DBUS_TYPE_INVALID);
	if (message != NULL)
	{
		DBusPendingCall* pending = NULL;
		DBusMessage* returnMessage;

		/*
		   this is client side
		*/
		if (SendDBusMessage(message, &pending) != 0)
		{
			if (pending != NULL)
			{
				if (GetArgumentFromDBusPendingCall(pending,
							DBUS_TYPE_UINT32, &value,
							DBUS_TYPE_INVALID) != 0)
				{
					(void)printf("GetArgmentFromDBusPendingCall value : %d \n", value);
				}
				else
				{
					(void)printf("GetArgmentFromDBusPendingCall return error\n");
				}

				dbus_pending_call_unref(pending);
			}
		}
		else
		{
			(void)printf("send mesage failed\n");
		}

		/*
		   this is server side
		*/
		returnMessage = CreateDBusMsgMethodReturn(message,
				DBUS_TYPE_UINT32, &value,
				DBUS_TYPE_INVALID);
		if (returnMessage != NULL)
		{
			if (SendDBusMessage(returnMessage, NULL) != 0)
			{
				(void)printf("SendDBusMessage failed\n");
			}

			dbus_message_unref(returnMessage);
		}

		dbus_message_unref(message);
	}
	else
	{
		(void)printf("CreateDBusMsgMethodCall failed\n");
	}

	ReleaseRawDBusConnection();

	(void)InitialzieInputProcess(NULL);

	SetPressedEvent(OnKeyboardPressed);
	SetLongPressedEvent(OnKeyboardLongPressed);
	SetLongLongPressedEvent(OnKeyboardLongLongPressed);
	SetReleasedEvent(OnKeyboardReleased);
	SetClickedEvent(OnKeyboardClicked);
	SetRotaryEvent(OnRotaryEvent);
	
	(void)StartInputProcess();
	ExitInputProcess();
}

static DBusMsgErrorCode OnReceivedDBusSignal(DBusMessage *message, const char *interface)
{
	DBusMsgErrorCode error = ErrorCodeNoError;
	(void)message;
	(void)interface;

	return error;
}

static DBusMsgErrorCode OnReceivedMethodCall(DBusMessage *message, const char *interface)
{
	DBusMsgErrorCode error = ErrorCodeNoError;
	(void)message;
	(void)interface;

	return error;
}

static void OnKeyboardPressed(int32_t key)
{
	(void)key;
}

static void OnKeyboardLongPressed(int32_t key)
{
	(void)key;
}

static void OnKeyboardLongLongPressed(int32_t key)
{
	(void)key;
}

static void OnKeyboardReleased(int32_t key)
{
	(void)key;
}

static void OnKeyboardClicked(int32_t key)
{
	(void)key;
}

static void OnRotaryEvent(int32_t key)
{
	(void)key;
}
