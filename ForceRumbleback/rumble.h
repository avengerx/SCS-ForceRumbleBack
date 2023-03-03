#ifndef __RUMBLE_H_INCLUDED__
#define __RUMBLE_H_INCLUDED__
#include "pch.h"
#include "log.h"

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"

#define FORCEGRANULARITY DI_FFNOMINALMAX / 254.0

HRESULT ShutdownDirectInput();

HRESULT SendForce(long value);
HRESULT SendForceWait(long, DWORD);

#endif