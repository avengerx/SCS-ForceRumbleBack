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
        log_error(DIERRSTR "Unable to create Direct Input handle.");
        return DIERR_GENERIC;
    }

    if (DI_OK != DirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumFFDevices, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)) {
        log_error(DIERRSTR "Unable to locate a Force Feedback device connected.");
        return DIERR_NOTFOUND;
    }

    if (DIDev == nullptr) {
        log_error(DIERRSTR "No Force Feedback compatible adapter was found in the system.");
        return DIERR_NOTFOUND;
    }

    LPDIDEVICEINSTANCE device_info = new DIDEVICEINSTANCE();
    device_info->dwSize = sizeof(DIDEVICEINSTANCE);

    if (DI_OK != DIDev->GetDeviceInfo(device_info))
    {
        log_error(DIERRSTR "Unable to fetch information about the force-feedback device.");
        return DIERR_GENERIC;
    }

    log_message(L"Found force feedback device: %s\n", device_info->tszProductName);
    delete device_info;

    if (DI_OK != DIDev->SetDataFormat(&c_dfDIJoystick)) {
        log_error(DIERRSTR "Unable to set device's data format to 'Joystick'.");
        return DIERR_GENERIC;
    }

    win = GetActiveWindow();

    if (win == nullptr) {
        DWORD ownpid = GetCurrentProcessId();

        EnumWindows(FindMainWindow, ownpid);

        if (win == nullptr) {
            log_error("Couldn't get the handle of the active window. It is required to bind the Force Feedback device.");
            return DIERR_GENERIC;
        }
    }

    if (DI_OK != DIDev->SetCooperativeLevel(win, DISCL_EXCLUSIVE | DISCL_FOREGROUND)) {
        log_error(DIERRSTR "Unable to set required force-feedback cooperation level with the device using the terminal window.");
        return DIERR_GENERIC;
    }

    if (DI_OK != DIDev->EnumObjects(EnumAxes, (VOID*)&ActuatorCount, DIDFT_AXIS)) {
        log_error(DIERRSTR "Unable to enumerate force feedback actuators in the device.");
        return DIERR_GENERIC;
    }

    HRESULT acquired = DIDev->Acquire();
    if (acquired != DI_OK && acquired != S_FALSE) {
        log_error("Unable to acquire device. Acquisition returned: 0x%08lx.");
        return DIERR_NOTACQUIRED;
    }

    log_message("Loaded device reports %u force feedback actuators present.", ActuatorCount);
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

    log_message("DirectInput shutdown: %s.", was_initialized ? "allocated resources released" : "no resources to release");
    return DI_OK;
}

HRESULT SendForce(long value) {
    HRESULT retstat;
    if (DIDev == nullptr) {
        log_message("Initializing DirectInput on demand.");
        retstat = initDirectInput();
        if (retstat == DI_OK)
            log_message("Initialized DirectInput on demand.");
        else {
            log_error("Attempt to apply force of %li while device could not be initialized.", value);
            return retstat;
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

    retstat = DIDev->CreateEffect(GUID_ConstantForce, &ffx, &DIFX, nullptr);
    if (DI_OK != retstat) {
#define ERRPFX "Error: unable to create effect on device: "
#define ERRSUF " [0x%04lx].\n", retstat

        switch (retstat) {
        case DIERR_DEVICENOTREG:
            log_error(ERRPFX "device not registered" ERRSUF);
            break;
        case DIERR_DEVICEFULL:
            log_error(ERRPFX "device effects capacity is full" ERRSUF);
            break;
        case DIERR_INVALIDPARAM:
            log_error(ERRPFX "invalid parameter" ERRSUF);
            break;
        case DIERR_NOTINITIALIZED:
            log_error(ERRPFX "device not initialized" ERRSUF);
            break;
        default:
            log_error(ERRPFX "unknown error" ERRSUF);
        }
    }

    return retstat;
}

HRESULT SendForceWait(long force, DWORD ms) {
    HRESULT retstat = SendForce(force);

    if (retstat == DI_OK) Sleep(ms);

    return retstat;
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