/****************************************************************************************
 *   FileName    : TCInput.h
 *   Description : This library that allows users to easily use linux input event
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
#ifndef INPUT_PROCESS_H_
#define INPUT_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif	

typedef void (*InputEventCallBack)(int32_t key);
	
int32_t InitialzieInputProcess(const char *name);
void ExitInputProcess(void);
int32_t StartInputProcess(void);
void SetPressedEvent(InputEventCallBack callback);
void SetLongPressedEvent(InputEventCallBack callback);
void SetLongLongPressedEvent(InputEventCallBack callback);
void SetReleasedEvent(InputEventCallBack callback);
void SetClickedEvent(InputEventCallBack callback);
void SetRotaryEvent(InputEventCallBack callback);

typedef enum {
	TCKeyPower,
	TCKeyMenu,
	TCKeyMedia,
	TCKeyHome,
	TCKeyBack,
	TCKeyOk,
	TCKeyNext,
	TCKeyPrev,
	TCKeyPlay,
	TCKeyPause,
	TCKeyPlayOrPause,
	TCKeyStop,
	TCKeyRight,
	TCKeyLeft,
	TCKeyUp,
	TCKeyDown,
	TCKeyJogRight,
	TCKeyJoglLeft,
	TCKey1,
	TCKey2,
	TCKey3,
	TCKey4,
	TCKey5,
	TCKey6,
	TCKeyVolumeUp,
	TCKeyVolumeDown,
	TCKeyVoiceCommand,
	TCKeyNavi,
	TCKeyRadio,
	TCKeyDMB,
	TCKeySetting,
	TCKeyPhoneHook,
	TCKeyPhoneDrop,
	TCKeyPhoneFlash,
	TCKeyPhoneKey0,
	TCKeyPhoneKey1,
	TCKeyPhoneKey2,
	TCKeyPhoneKey3,
	TCKeyPhoneKey4,
	TCKeyPhoneKey5,
	TCKeyPhoneKey6,
	TCKeyPhoneKey7,
	TCKeyPhoneKey8,
	TCKeyPhoneKey9,
	TCKeyPhoneKeyStar,
	TCKeyPhoneKeyPound,
	TCKeyTakeScreen,
	TCKeyUnTakeScreen,
	TCKeyBorrowScreen,
	TCKeyUnBorrowScreen,
	TCKeyScan,
	TCKeyMap,
	TotalTCKeys
}TCKeyValue;
extern const int32_t g_knobKeys[TotalTCKeys];

#ifdef __cplusplus
}
#endif	
		
#endif // INPUT_PROCESS_H_

