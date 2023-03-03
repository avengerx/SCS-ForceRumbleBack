#include "pch.h"
#include "rumble.h"

#define DIERRSTR "DirectInput error: "

LPDIRECTINPUT8 DirectInput = nullptr;
LPDIRECTINPUTDEVICE8 DIDev = nullptr;
LPDIRECTINPUTEFFECT DIFX = nullptr;
HWND win = nullptr;
DWORD ActuatorCount = 0;

BOOL CALLBACK EnumAxes(const DIDEVICEOBJECTINSTANCE*, VOID*) noexcept;
BOOL CALLBACK EnumFFDevices(const DIDEVICEINSTANCE*, VOID*) noexcept;
BOOL CALLBACK FindMainWindow(HWND, LPARAM) noexcept;

HRESULT initDirectInput() {
    if (DI_OK != DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&DirectInput, nullptr)) {
        log(DIERRSTR "Unable to create Direct Input handle.");
        return DIERR_GENERIC;
    }

    if (DI_OK != DirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumFFDevices, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)) {
        log(DIERRSTR "Unable to locate a Force Feedback device connected.");
        return DIERR_NOTFOUND;
    }

    if (DIDev == nullptr) {
        log(DIERRSTR "No Force Feedback compatible adapter was found in the system.");
        return DIERR_NOTFOUND;
    }

    LPDIDEVICEINSTANCE device_info = new DIDEVICEINSTANCE();
    device_info->dwSize = sizeof(DIDEVICEINSTANCE);

    if (DI_OK != DIDev->GetDeviceInfo(device_info))
    {
        log(DIERRSTR "Unable to fetch information about the force-feedback device.");
        return DIERR_GENERIC;
    }

    log(L"Found force feedback device: %s\n", device_info->tszProductName);
    delete device_info;

    if (DI_OK != DIDev->SetDataFormat(&c_dfDIJoystick)) {
        log(DIERRSTR "Unable to set device's data format to 'Joystick'.");
        return DIERR_GENERIC;
    }

    win = GetActiveWindow();

    if (win == nullptr) {
        DWORD ownpid = GetCurrentProcessId();

        EnumWindows(FindMainWindow, ownpid);

        if (win == nullptr) {
            log("Couldn't get the handle of the active window. It is required to bind the Force Feedback device.");
            return DIERR_GENERIC;
        }
    }

    if (DI_OK != DIDev->SetCooperativeLevel(win, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
        log(DIERRSTR "Unable to set required force-feedback cooperation level with the device using the terminal window.");
        return DIERR_GENERIC;
    }

    if (DI_OK != DIDev->EnumObjects(EnumAxes, (VOID*)&ActuatorCount, DIDFT_AXIS)) {
        log(DIERRSTR "Unable to enumerate force feedback actuators in the device.");
        return DIERR_GENERIC;
    }

    HRESULT acquired = DIDev->Acquire();
    if (acquired != DI_OK && acquired != S_FALSE) {
        log("Unable to acquire device. Acquisition returned: 0x%08lx.");
        return DIERR_NOTACQUIRED;
    }

    log("Loaded device reports %u force feedback actuators present.", ActuatorCount);
    return DI_OK;
}

HRESULT ShutdownDirectInput() {
    bool was_initialized = false;
    if (DIDev || DIFX || DirectInput) {
        was_initialized = true;
    }

    if (DIDev) DIDev->Unacquire();

#define safe_release(inptr) if (inptr) { inptr->Release(); inptr = nullptr; }
    safe_release(DIFX);
    safe_release(DIDev);
    safe_release(DirectInput);
#undef safe_release

    win = nullptr;

    log("DirectInput shutdown: %s.", was_initialized ? "allocated resources released" : "no resources to release");
    return DI_OK;
}

void sendforce(long value) {
    if (DIDev == nullptr) {
        log("Initializing DirectInput on demand.");
        if (initDirectInput() == DI_OK)
            log("Initialized DirectInput on demand.");
        else {
            log("Attempt to apply force of %l while device could not be initialized.");
            return;
        }
    }

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
#define ERRPFX "Error: unable to create effect on device: "
#define ERRSUF " [0x%04lx].\n", retstat

        switch (retstat) {
        case DIERR_DEVICENOTREG:
            log(ERRPFX "device not registered" ERRSUF);
            break;
        case DIERR_DEVICEFULL:
            log(ERRPFX "device effects capacity is full" ERRSUF);
            break;
        case DIERR_INVALIDPARAM:
            log(ERRPFX "invalid parameter" ERRSUF);
            break;
        case DIERR_NOTINITIALIZED:
            log(ERRPFX "device not initialized" ERRSUF);
            break;
        default:
            log(ERRPFX "unknown error" ERRSUF);
        }
    }
}

void sendforceWait(long force, DWORD ms) {
    sendforce(force);
    Sleep(ms);
}

long EngineIdlespeed = 900;
long EngineCruisespeed = 9400;

void start_engine() {
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
}

void stop_engine() {
    sendforceWait(2500, 100);
    sendforceWait(6500, 100);
    sendforceWait(10000, 200);
    sendforceWait(8500, 100);
    sendforceWait(6500, 100);
    sendforceWait(2500, 200);
    sendforceWait(850, 600);
    sendforceWait(500, 300);
    sendforceWait(0, 100);
}

void accel() {
    long force = 0;
    while (force < 10000) {
        force += 800;
        sendforceWait(force, 150);
    }
}

void deccel() {
    long force = 10000;
    while (force > EngineIdlespeed) {
        force -= 800;
        if (force < EngineIdlespeed) force = EngineIdlespeed;
        sendforceWait(force, 150);
    }
}

void cruiserpm() {
    long force = EngineIdlespeed;
    while (force < EngineCruisespeed) {
        force += 800;
        if (force > EngineCruisespeed) force = EngineCruisespeed;
        sendforceWait(force, 50);
    }
}

void back_to_idle() {
    long force = EngineCruisespeed;
    while (force > EngineIdlespeed) {
        force -= 800;
        if (force < EngineIdlespeed) force = EngineIdlespeed;
        sendforceWait(force, 50);
    }
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

BOOL CALLBACK FindMainWindow(HWND handle, LPARAM pidptr) noexcept {
    DWORD ownpid = (DWORD)pidptr;
    DWORD winpid;
    
    GetWindowThreadProcessId(handle, &winpid);

    if (ownpid == winpid) {
        win = handle;
        return false;
    }
    return true;
}