/****************************************************************************************
 *   FileName    : TCInput.c
 *   Description : This library that allows users to easily use linux input event
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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <linux/input.h>
#include <sys/select.h>
#include <pthread.h>
#include "TCInput.h"

#define KEY_INFO_POOL_SIZE			KEY_CNT  // KEY_MAX(0x2ff) + 1

typedef enum {
	KeyStatusRelease,
	KeyStatusPress,
	KeyStatusHold,
	KeyStatusLongPress,
	KeyStatusLongLongPress,
	KeyStatusClick,
	TotalKeyStatus
} KeyStatus;

typedef struct {
	KeyStatus status;
	uint16_t type;
	uint16_t code;
	int32_t value;
	uint32_t cnt;
	struct timeval time;
	int32_t emitPressed;
} KeyInfo;

static int32_t InitializeKeyInfoPool(void);
static void *MainThread(void *arg);
static void *UpdateStatusThread(void *arg);
static void *RotaryThread(void *arg);
static void UpdateKeyInfoPool(struct input_event *event);
static inline int64_t GetElapsedMiliSeconds(struct timeval prevTime, struct timeval now);
static inline void msleep(useconds_t  msec);

static KeyInfo *g_keyInfoPool = NULL;
static int32_t g_init = 0;
static char g_device[32];
static int32_t g_fd = -1;
static int32_t g_fdRotary = -1;
static int32_t g_mainRun = 0;
static int32_t g_updateRun = 0;
static int32_t g_rotaryRun = 0;
static pthread_mutex_t g_keyInfoMutex;
static pthread_mutex_t* g_keyInfoMutexPtr = NULL;
static pthread_t g_mainThread;
static pthread_t g_updateThread;
static pthread_t g_rotaryThread;

static InputEventCallBack PressedEventCallBack = NULL;
static InputEventCallBack LongPressedEventCallBack = NULL;
static InputEventCallBack LongLongPressedEventCallBack = NULL;
static InputEventCallBack ReleasedEventCallBack = NULL;
static InputEventCallBack ClickedEventCallBack = NULL;
static InputEventCallBack RotaryEventCallBack = NULL;


int32_t InitialzieInputProcess(const char *name)
{
	static const char *default_device_name = "/dev/input/keyboard0";
	const char *device_name;
	int32_t err;

	if (g_init != 0)
	{
		ExitInputProcess();
		g_init = 0;
	}

	if (name != NULL)
	{
		device_name = name;
	}
	else
	{
		device_name = default_device_name;
	}


	if(access(device_name, F_OK) == 0)
	{
		(void)strncpy(g_device, device_name, 32);

		err = InitializeKeyInfoPool();

		if (err == 0)
		{
			err = pthread_mutex_init(&g_keyInfoMutex, NULL);
			g_keyInfoMutexPtr = &g_keyInfoMutex;
			if (err == 0)
			{
				g_init = 1;
			}
			else
			{
				perror("pthread_mutexg_init failed: ");
			}
		}
	}
	else
	{
		(void)fprintf(stderr, "%s: not exist device(%s)\n", __func__, device_name);
	}

	return g_init;
}

void ExitInputProcess(void)
{
	if (g_init != 0)
	{
		int32_t err;
		void *res;

		if (g_mainRun != 0)
		{
			g_mainRun = 0;
			err = pthread_join(g_mainThread, &res);
			if (err != 0)
			{
				perror("main thread joion faild: ");
			}
		}

		if (g_updateRun != 0)
		{
			g_updateRun = 0;
			err = pthread_join(g_updateThread, &res);
			if (err != 0)
			{
				perror("update thread joion faild: ");
			}
		}

		if (g_rotaryRun != 0)
		{
			g_rotaryRun = 0;
			err = pthread_join(g_rotaryThread, &res);
			if (err != 0)
			{
				perror("rotary thread joion faild: ");
			}
		}

		if (g_keyInfoMutexPtr != NULL)
		{
			err = pthread_mutex_destroy(g_keyInfoMutexPtr);
			if (err != 0)
			{
				perror(" g_keyInfoMutexPtr mutex destroy faild: ");
			}
		}

		if (g_fd != -1)
		{
			(void)close(g_fd);
			g_fd = -1;
		}

		if (g_fdRotary != -1)
		{
			(void)close(g_fdRotary);
			g_fdRotary = -1;
		}

		if (g_keyInfoPool != NULL)
		{
			free(g_keyInfoPool);
 			g_keyInfoPool = NULL;
		}
	}
}

int32_t StartInputProcess(void)
{
	int32_t ret = 0;

	if (g_init != 0)
	{
		int32_t err;
		g_mainRun = 1;
		err = pthread_create(&g_mainThread, NULL, MainThread, NULL);
		if (err == 0)
		{
			g_updateRun = 1;
			err = pthread_create(&g_updateThread, NULL, UpdateStatusThread, NULL);
			if (err == 0)
			{
				ret = 1;
			}
			else
			{
				perror("create update thread failed: ");
				g_updateRun = 0;
				ExitInputProcess();
			}
		}
		else
		{
			perror("create main thread failed: ");
			g_mainRun = 0;
			ExitInputProcess();
		}

		g_rotaryRun = 1;
		err = pthread_create(&g_rotaryThread, NULL, RotaryThread, NULL);
		if (err == 0)
		{
			ret = 1;
		}
		else
		{
			perror("create update thread failed: ");
			g_rotaryRun = 0;
		}
	}
	else
	{
		(void)fprintf(stderr, "input process not initialize\n");
	}

	return ret;
}

void SetPressedEvent(InputEventCallBack callback)
{
	PressedEventCallBack = callback;
}

void SetLongPressedEvent(InputEventCallBack callback)
{
	LongPressedEventCallBack = callback;
}

void SetLongLongPressedEvent(InputEventCallBack callback)
{
	LongLongPressedEventCallBack = callback;
}

void SetReleasedEvent(InputEventCallBack callback)
{
	ReleasedEventCallBack = callback;
}

void SetClickedEvent(InputEventCallBack callback)
{
	ClickedEventCallBack = callback;
}

void SetRotaryEvent(InputEventCallBack callback)
{
	RotaryEventCallBack = callback;
}

static int32_t InitializeKeyInfoPool(void)
{
	int32_t err = -1;
	int32_t idx;
	struct timeval now;

	(void)gettimeofday(&now, NULL);

	g_keyInfoPool = (KeyInfo *)malloc(sizeof (KeyInfo) * (uint32_t)KEY_INFO_POOL_SIZE);
	if (g_keyInfoPool != NULL)
	{
		for (idx = 0; idx < KEY_INFO_POOL_SIZE; idx++)
		{
			g_keyInfoPool[idx].status = KeyStatusRelease;
			g_keyInfoPool[idx].type = 0;
			g_keyInfoPool[idx].code = 0;
			g_keyInfoPool[idx].value = 0;
			g_keyInfoPool[idx].time = now;
			g_keyInfoPool[idx].cnt = 0;
		}
		err = 0;
	}
	else
	{
		(void)fprintf(stderr, "%s: memory allocation for key information pool failed\n", __func__);
	}

	return err;
}

static void *MainThread(void *arg)
{
	struct input_event inputEvent;
	int32_t readBytes;
	size_t eventSize;
	struct timeval now;
	fd_set readSet, tempReadSet;
	struct timeval timeOut;
	int32_t flags;
	int32_t sel;
	int32_t cnt = 0;
	static int32_t retMain = 0;

	while ((g_fd == -1) && (cnt < 10))
	{
		if(access(g_device, F_OK) == 0)
		{
			g_fd = open(g_device, O_RDONLY);
		}
		msleep(1000);
		cnt ++;
	}

	if (g_fd != -1)
	{
		flags = fcntl(g_fd, F_GETFL, 0);
		if (flags == -1)
		{
			flags = 0;
		}
		(void)fcntl(g_fd, F_SETFL, (uint32_t)flags | (uint32_t)O_NONBLOCK);

		FD_ZERO(&readSet);
		FD_SET(g_fd, &readSet);
		timeOut.tv_sec = 0;
		timeOut.tv_usec = 1000;

		eventSize = sizeof (struct input_event);
	}
	else
	{
		(void)fprintf(stderr, "can not open %s\n", g_device);
		g_init = 0;
		g_mainRun = 0;
	}

	while (g_mainRun != 0)
	{
		(void)memcpy(&tempReadSet, &readSet, sizeof (fd_set));

		sel = select(g_fd + 1, &tempReadSet, NULL, NULL, &timeOut);
		if (sel > 0)
		{
			if (FD_ISSET(g_fd, &tempReadSet) != 0)
			{
				readBytes = read(g_fd, &inputEvent, eventSize);
				while (readBytes >= (int32_t)eventSize)
				{
					(void)gettimeofday(&now, NULL);
					inputEvent.time = now;
					UpdateKeyInfoPool(&inputEvent);

					readBytes = read(g_fd, &inputEvent, eventSize);
				}
			}
		}
		msleep(1);
	}

	(void)arg;
    pthread_exit((void *)&retMain);
}

static void *UpdateStatusThread(void *arg)
{
	int32_t idx;
	struct timeval now;
	static int32_t retUpdate = 0;

	while (g_updateRun != 0)
	{
		for (idx = 0; idx < KEY_INFO_POOL_SIZE; idx++)
		{
			(void)gettimeofday(&now, NULL);
			(void)pthread_mutex_lock(&g_keyInfoMutex);

			if ((g_keyInfoPool[idx].status >= KeyStatusPress) &&
				(g_keyInfoPool[idx].status < TotalKeyStatus))
			{
				if (GetElapsedMiliSeconds(g_keyInfoPool[idx].time, now) > 100)
				{
					if (PressedEventCallBack != NULL)
					{
						PressedEventCallBack((int32_t)g_keyInfoPool[idx].code);
					}
					g_keyInfoPool[idx].cnt++;
					g_keyInfoPool[idx].time = now;
				}

				if (g_keyInfoPool[idx].cnt > (uint32_t)10)
				{
					if ((g_keyInfoPool[idx].status == KeyStatusPress) ||
						(g_keyInfoPool[idx].status == KeyStatusHold))
					{
						if (LongPressedEventCallBack != NULL)
						{
							LongPressedEventCallBack((int32_t)g_keyInfoPool[idx].code);
						}
						g_keyInfoPool[idx].status = KeyStatusLongPress;
					}
					else if (g_keyInfoPool[idx].status == KeyStatusLongPress)
					{
						if (LongLongPressedEventCallBack != NULL)
						{
							LongLongPressedEventCallBack((int32_t)g_keyInfoPool[idx].code);
						}
						g_keyInfoPool[idx].status = KeyStatusLongLongPress;
					}
					else
					{
						(void)fprintf(stderr, "%s: not support process after long long press event\n", __func__);
					}
					g_keyInfoPool[idx].time = now;
					g_keyInfoPool[idx].cnt = 0;
				}
			}
			(void)pthread_mutex_unlock(&g_keyInfoMutex);
			msleep(1);
		}
	}

	(void)arg;
    pthread_exit((void *)&retUpdate);
}

static void *RotaryThread(void *arg)
{
	struct input_event inputEvent;
	int32_t readBytes;
	size_t eventSize;
	struct timeval now;
	fd_set readSet, tempReadSet;
	struct timeval timeOut;
	int32_t flags;
	int32_t sel;
	int32_t cnt = 0;
	static int32_t retRotary = 0;
	static const char *default_rotary_device_name = "/dev/input/rotary0";

	while ((g_fdRotary == -1) && (cnt < 10))
	{
		if (access(default_rotary_device_name, F_OK) == 0)
		{
			g_fdRotary = open(default_rotary_device_name, O_RDONLY);
		}
		msleep(1000);
		cnt ++;
	}

	if (g_fdRotary != -1)
	{
		flags = fcntl(g_fdRotary, F_GETFL, 0);
		if (flags == -1)
		{
			flags = 0;
		}
		(void)fcntl(g_fdRotary, F_SETFL, (uint32_t)flags | (uint32_t)O_NONBLOCK);

		FD_ZERO(&readSet);
		FD_SET(g_fdRotary, &readSet);
		timeOut.tv_sec = 0;
		timeOut.tv_usec = 1000;

		eventSize = sizeof (struct input_event);
	}
	else
	{
		(void)fprintf(stderr, "can not open %s\n", default_rotary_device_name);
		g_rotaryRun = 0;
	}

	while (g_rotaryRun != 0)
	{
		(void)memcpy(&tempReadSet, &readSet, sizeof (fd_set));

		sel = select(g_fdRotary + 1, &tempReadSet, NULL, NULL, &timeOut);
		if (sel > 0)
		{
			if (FD_ISSET(g_fdRotary, &tempReadSet) != 0)
			{
				readBytes = read(g_fdRotary, &inputEvent, eventSize);
				while (readBytes >= (int32_t)eventSize)
				{
					if (inputEvent.type == (uint16_t)EV_REL)
					{
						if (RotaryEventCallBack != NULL)
						{
							RotaryEventCallBack(inputEvent.value);
						}
					}

					readBytes = read(g_fdRotary, &inputEvent, eventSize);
				}
			}
		}
		msleep(1);
	}

	(void)arg;
    pthread_exit((void *)&retRotary);
}

static void UpdateKeyInfoPool(struct input_event *event)
{
	if (event != NULL)
	{
		uint16_t idx = event->code;
		(void)pthread_mutex_lock(&g_keyInfoMutex);

		if (idx < (uint16_t)KEY_INFO_POOL_SIZE)
		{
			if (event->type == (uint16_t)EV_KEY)
			{
				if (event->value == 0) // key released
				{
					if (g_keyInfoPool[idx].emitPressed != 0)
					{
						if (g_keyInfoPool[idx].status == KeyStatusPress)
						{
							if (ClickedEventCallBack != NULL)
							{
								ClickedEventCallBack((int32_t)event->code);
							}
						}
						g_keyInfoPool[idx].status = KeyStatusRelease;
						g_keyInfoPool[idx].time = event->time;

						if (ReleasedEventCallBack != NULL)
						{
							ReleasedEventCallBack((int32_t)event->code);
						}
					}
				}
				else if (event->value == 1) // key pressed
				{
					if (GetElapsedMiliSeconds(g_keyInfoPool[idx].time, event->time) > 10)
					{
						g_keyInfoPool[idx].status = KeyStatusPress;
						g_keyInfoPool[idx].time = event->time;
						g_keyInfoPool[idx].emitPressed = 1;
						if (PressedEventCallBack != NULL)
						{
							PressedEventCallBack((int32_t)event->code);
						}
					}
					else
					{
						g_keyInfoPool[idx].emitPressed = 0;
					}
				}
				else if (event->value == 2) // key pressed continue
				{
				}
				else
				{
					(void)fprintf(stderr, "%s: not support event(%d)\n", __func__, event->value);
				}

				g_keyInfoPool[idx].type = event->type;
				g_keyInfoPool[idx].code = event->code;
				g_keyInfoPool[idx].value = event->value;
				g_keyInfoPool[idx].cnt = 0;
			}
		}
		else
		{
			(void)fprintf(stderr, "%s: invalid key code(%d)\n", __func__, event->code);
		}
		(void)pthread_mutex_unlock(&g_keyInfoMutex);
	}
}

static inline int64_t GetElapsedMiliSeconds(struct timeval prevTime, struct timeval now)
{
	int64_t ret;
	ret =  ((now.tv_sec - prevTime.tv_sec)*1000) + ((now.tv_usec - prevTime.tv_usec) / 1000);

	return ret;
}

static inline void msleep(useconds_t  msec)
{
	(void)usleep(msec * (useconds_t)1000);
}

// set reference to Dolphin Board
const int32_t g_knobKeys[TotalTCKeys] = {
	KEY_POWER,				//TCKeyPower
	-1,						//TCKeyMenu
	KEY_MEDIA,				//TCKeyMedia
	KEY_HOME,				//TCKeyHome
	KEY_ESC,				//TCKeyBack
	KEY_ENTER,				//TCKeyOk
	KEY_RIGHT,				//TCKeyNext
	KEY_LEFT,			//TCKeyPrev
	KEY_PLAY,				//TCKeyPlay
	KEY_PAUSE,				//TCKeyPause
	KEY_PLAYPAUSE,			//TCKeyPlayOrPause,
	KEY_STOP,				//TCKeyStop
	KEY_RIGHTCTRL,			//TCKeyRight
	KEY_LEFTCTRL,			//TCKeyLeft
	KEY_UP,					//TCKeyUp
	KEY_DOWN,				//TCKeyDown
	-1,						//TCKeyJogRight
	-1,						//TCKeyJoglLeft
	KEY_1,					//TCKey1
	KEY_2,					//TCKey2
	-1,						//TCKey3
	-1,						//TCKey4
	KEY_5,					//TCKey5
	KEY_6,					//TCKey6
	KEY_VOLUMEUP,			//TCKeyVolumeUp
	KEY_VOLUMEDOWN,			//TCKeyVolumeDown
	KEY_VOICECOMMAND,		//TCKeyVoiceCommand,
	KEY_3,					//TCKeyNavi,
	KEY_RADIO,				//TCKeyRadio,
#ifdef KEY_DMB
	KEY_DMB,				//TCKeyDMB,
#else
	-1,				//TCKeyDMB,
#endif
	 KEY_SETUP,			//TCKeySetting,
	-1,						//TCKeyPhoneHook,
	-1,						//TCKeyPhoneDrop,
	KEY_PHONE,				//TCKeyPhoneFlash,
	-1,						//TCKeyPhoneKey0,
	-1,						//TCKeyPhoneKey1,
	-1,						//TCKeyPhoneKey2,
	-1,						//TCKeyPhoneKey3,
	-1,						//TCKeyPhoneKey4,
	-1,						//TCKeyPhoneKey5,
	-1,						//TCKeyPhoneKey6,
	-1,						//TCKeyPhoneKey7,
	-1,						//TCKeyPhoneKey8,
	-1,						//TCKeyPhoneKey9,
	-1,						//TCKeyPhoneKeyStar,
	-1,						//TCKeyPhoneKeyPound,
	-1,						//TCKeyTakeScreen,
	-1,						//TCKeyUnTakeScreen,
	-1,						//TCKeyBorrowScreen,
	-1,						//TCKeyUnBorrowScreen,
	KEY_SEARCH,				//TCKeyScan
	KEY_4,					//TCKeyMap,
};
