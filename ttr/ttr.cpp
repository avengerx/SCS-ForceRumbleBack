// ttr.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"
#include <stdio.h>
#include <conio.h>

LPDIRECTINPUT8 DirectInput = nullptr;
LPDIRECTINPUTDEVICE8 DIDev = nullptr;
LPDIRECTINPUTEFFECT DIFX = nullptr;
DWORD ActuatorCount = 0;

BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCE* pOInst, VOID* pContext) noexcept;
BOOL CALLBACK EnumFFDevices(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept;
static unsigned __int64 rdtsc();

void sendforce(long value);
void start_engine();
void stop_engine();
void accel();
void deccel();
void cruiserpm();
void back_to_idle();

int main()
{
    if (DI_OK != DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&DirectInput, nullptr)) {
        printf("Error: Unable to create Direct Input handle.\n");
        exit(1);
    }

    if (DI_OK != DirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumFFDevices, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)) {
        printf("Error: Unable to locate a Force Feedback device connected.\n");
        exit(1);
    }

    if (DIDev == nullptr) {
        printf("Error: No Force Feedback compatible adapter was found in the system.\n");
        exit(1);
    }

    LPDIDEVICEINSTANCE device_info = new DIDEVICEINSTANCE();
    device_info->dwSize = sizeof(DIDEVICEINSTANCE);

    if (DI_OK != DIDev->GetDeviceInfo(device_info))
    {
        printf("Error: Unable to fetch information about the force-feedback device.\n");
        exit(1);
    }

    wprintf(L"Found force feedback device: %s\n", device_info->tszProductName);
    delete device_info;

    if (DI_OK != DIDev->SetDataFormat(&c_dfDIJoystick)) {
        printf("Error: Unable to set device's data format to 'Joystick'.\n");
        exit(1);
    }

    if (DI_OK != DIDev->SetCooperativeLevel(GetConsoleWindow(), DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
        printf("Error: Unable to set required force-feedback cooperation level with the device using the terminal window.\n");
        exit(1);
    }

    if (DI_OK != DIDev->EnumObjects(EnumAxes, (VOID*)&ActuatorCount, DIDFT_AXIS)) {
        printf("Error: Unable to enumerate force feedback actuators in the device.\n");
        exit(1);
    }

    printf("Device reports %u actuators present.\n", ActuatorCount);

    sendforce(0);
    Sleep(100);

    printf("Device initialized.\n");

    bool started = false;
    bool accel_high = false;
    bool cruise = false;
    char cmd = ' ';
    bool handled = false;
    while(true) {
        printf("Type a command to feel its force feedback.\n");

        if (started) {
            printf("s) shutdown engine\n");
            if (cruise) printf("c) back to idle\n");
            else printf("c) accel to cruise rpm\n");
            if (accel_high) printf("a) decelerate\n");
            else printf("a) accelerate\n");
        } else {
            printf("s) start engine\n");
        }
        printf("q) quit\n Choose an option ::> ");

        while (true) {
            handled = false;
            cmd = _getche();
            printf("\b \b");

            if (cmd == 'q') {
                printf("bail out\n");
                if (started) {
                    handled = true;
                    if (accel_high) deccel();
                    stop_engine();
                }
                break;
            }

            if (started) {
                if (cmd == 's') {
                    printf("shut down\n");
                    handled = true;
                    started = false;
                    if (accel_high) {
                        deccel();
                        accel_high = false;
                    }
                    stop_engine();
                } else if (cmd == 'a') {
                    if (accel_high) {
                        printf("easy, baby\n");
                        handled = true;
                        accel_high = false;
                        deccel();
                    } else {
                        printf("pedal to the metal\n");
                        handled = true;
                        cruise = false;
                        accel_high = true;
                        accel();
                    }
                } else if (cmd == 'c') {
                    if (cruise) {
                        handled = true;
                        back_to_idle();
                        cruise = false;
                    } else {
                        handled = true;
                        cruiserpm();
                        cruise = true;
                    }
                }

            }
            else if (cmd == 's') {
                printf("fire it up\n");
                handled = true;
                started = true;
                start_engine();
            }

            if (handled) break;
        }

        if (cmd == 'q') break;
    }
}

void sendforce(long value) {
    DICONSTANTFORCE fx = { value };
    DWORD axisId[1] = { DIJOFS_X };
    LONG axisDir[1] = { DI_FFNOMINALMAX };

    DIEFFECT ffx = {};
    ffx.dwSize = sizeof(DIEFFECT);
    ffx.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    ffx.dwDuration = INFINITE;
    ffx.dwSamplePeriod = 0;
    ffx.dwGain = DI_FFNOMINALMAX;
    ffx.dwTriggerButton = DIEB_NOTRIGGER;
    ffx.dwTriggerRepeatInterval = INFINITE;
    ffx.cAxes = 1;
    ffx.rgdwAxes = axisId;
    ffx.rglDirection = axisDir;
    ffx.lpEnvelope = 0;
    ffx.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    ffx.lpvTypeSpecificParams = &fx;
    ffx.dwStartDelay = 0;

    HRESULT retstat = DIDev->CreateEffect(GUID_ConstantForce, &ffx, &DIFX, nullptr);
    if (DI_OK != retstat) {
        printf("Error: unable to create effect on device: ");

        switch (retstat) {
        case DIERR_DEVICENOTREG:
            printf("device not registered");
            break;
        case DIERR_DEVICEFULL:
            printf("device effects capacity is full");
            break;
        case DIERR_INVALIDPARAM:
            printf("invalid parameter");
            break;
        case DIERR_NOTINITIALIZED:
            printf("device not initialized");
            break;
        default:
            printf("unknown error");
        }

        printf(" [0x%04lx].\n", retstat);
        exit(1);
    }
}

void sendforceWait(long force, DWORD ms) {
    sendforce(force);
    Sleep(ms);
}

long EngineIdlespeed = 900;
long EngineCruisespeed = 9400;

void start_engine() {
    printf("Starting engine...");

    sendforceWait(500, 350);
    sendforceWait(2500, 150);
    sendforceWait(2500, 150);
    sendforceWait(4500, 150);
    sendforceWait(6500, 150);
    sendforceWait(10000, 200);
    sendforceWait(8500, 100);
    sendforceWait(6500, 100);
    sendforceWait(2500, 200);
    sendforceWait(EngineIdlespeed, 500);

    printf("\nEngine started.\n");
}

void stop_engine() {
    printf("Stopping engine...");
    sendforceWait(2500, 100);
    sendforceWait(6500, 100);
    sendforceWait(10000, 200);
    sendforceWait(8500, 100);
    sendforceWait(6500, 100);
    sendforceWait(2500, 200);
    sendforceWait(850, 600);
    sendforceWait(500, 300);
    sendforceWait(0, 100);
    printf("\nEngine stopped.\n");
}

void accel() {
    long force = 0;

    printf("Accellerating: ");
    while (force < 10000) {
        force += 800;
        sendforceWait(force, 150);
    }
    printf("done.\nFull gas.\n");
}

void deccel() {
    long force = 10000;

    printf("Decellerating: ");
    while (force > EngineIdlespeed) {
        force -= 800;
        if (force < EngineIdlespeed) force = EngineIdlespeed;
        sendforceWait(force, 150);
    }
    printf("done.\nIdling.\n");
}

void cruiserpm() {
    long force = EngineIdlespeed;

    printf("Accelling to cruise rpm: ");
    while (force < EngineCruisespeed) {
        force += 800;
        if (force > EngineCruisespeed) force = EngineCruisespeed;
        sendforceWait(force, 50);
    }
    printf("done.\n");
}

void back_to_idle() {
    long force = EngineCruisespeed;

    printf("Revving back to idle: ");
    while (force > EngineIdlespeed) {
        force -= 800;
        if (force < EngineIdlespeed) force = EngineIdlespeed;
        sendforceWait(force, 50);
    }
    printf("done.\n");
}

BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCE* pOInst, VOID* pContext) noexcept
{
    if (pContext == nullptr) {
        return DIENUM_CONTINUE;
    }

    DWORD* pdwNumFFAxes = static_cast<DWORD*>(pContext);

    if ((pOInst->dwFlags & DIDOI_FFACTUATOR) != 0) (*pdwNumFFAxes)++;

    return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumFFDevices(const DIDEVICEINSTANCE* pInst, VOID* pContext) noexcept
{
    LPDIRECTINPUTDEVICE8 device;
    HRESULT retstat;

    retstat = DirectInput->CreateDevice(pInst->guidInstance, &device, nullptr);

    if (retstat != DI_OK) {
        return DIENUM_CONTINUE;
    }

    DIDev = device;
    return DIENUM_STOP;
}

static unsigned __int64 rdtsc() {
    return __rdtsc();
}