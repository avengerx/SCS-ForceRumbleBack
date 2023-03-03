#ifndef __RUMBLE_H_INCLUDED__
#define __RUMBLE_H_INCLUDED__
#include "pch.h"
#include "log.h"

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"

HRESULT ShutdownDirectInput();

void sendforce(long value);
void sendforceWait(long, DWORD);
void start_engine();
void stop_engine();
void accel();
void deccel();
void cruiserpm();
void back_to_idle();

#endif