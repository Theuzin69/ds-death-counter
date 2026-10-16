#include "app.h"
#include <objbase.h>

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int)
{
    // Only one counter at a time; a second launch just brings the first one to the front.
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"DSDeathCounter_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND existing = FindWindowW(kMainWindowClass, nullptr)) PostMessageW(existing, WM_APP_SHOWME, 0, 0);
        return 0;
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    bool startInTray = cmdLine && wcsstr(cmdLine, L"--tray");
    App app(inst);
    int rc = app.Run(startInTray);

    CoUninitialize();
    if (mutex) {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
    return rc;
}
