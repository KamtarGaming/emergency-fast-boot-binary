#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * EMERGENCY Fast Boot - local DWMAPI proxy
 *
 * This DLL is intentionally tiny.  It forwards every DWM API imported by the
 * current EMERGENCY shipping executable, then queues one Space key press on
 * that process's first visible top-level window.  EMERGENCY's startup warning
 * accepts any key; Space uses the game's ordinary input route.
 *
 * The input is queued inside the game process and happens only once per game
 * launch.  It is never sent to another application.
 */

typedef HRESULT (WINAPI *DwmFlushFn)(void);
typedef HRESULT (WINAPI *DwmIsCompositionEnabledFn)(BOOL *);
typedef HRESULT (WINAPI *DwmGetCompositionTimingInfoFn)(HWND, void *);
typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);

static INIT_ONCE g_dwmInit = INIT_ONCE_STATIC_INIT;
static HMODULE g_realDwm = NULL;
static DwmFlushFn g_dwmFlush = NULL;
static DwmIsCompositionEnabledFn g_dwmIsCompositionEnabled = NULL;
static DwmGetCompositionTimingInfoFn g_dwmGetCompositionTimingInfo = NULL;
static DwmSetWindowAttributeFn g_dwmSetWindowAttribute = NULL;
static HINSTANCE g_instance = NULL;

#ifdef FASTBOOT_DIAGNOSTIC
static void DebugLog(const WCHAR *event) {
    WCHAR path[MAX_PATH];
    WCHAR line[256];
    WCHAR *lastSlash;
    HANDLE file;
    DWORD written;

    if (g_instance == NULL || GetModuleFileNameW(g_instance, path, MAX_PATH) == 0) {
        return;
    }
    lastSlash = wcsrchr(path, L'\\');
    if (lastSlash == NULL) {
        return;
    }
    lstrcpyW(lastSlash + 1, L"FastBoot.debug.log");
    wsprintfW(line, L"%10lu ms  %s\r\n", GetTickCount(), event);
    file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        WriteFile(file, line, (DWORD)(lstrlenW(line) * sizeof(WCHAR)), &written, NULL);
        CloseHandle(file);
    }
}
#else
#define DebugLog(event) ((void)0)
#endif

static BOOL CALLBACK LoadRealDwm(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    WCHAR systemDirectory[MAX_PATH];
    WCHAR path[MAX_PATH];
    UINT length = GetSystemDirectoryW(systemDirectory, MAX_PATH);

    (void)once;
    (void)parameter;
    (void)context;

    if (length == 0 || length >= MAX_PATH) {
        return TRUE;
    }
    if (lstrlenW(systemDirectory) + lstrlenW(L"\\dwmapi.dll") >= MAX_PATH) {
        return TRUE;
    }

    lstrcpyW(path, systemDirectory);
    lstrcatW(path, L"\\dwmapi.dll");
    g_realDwm = LoadLibraryW(path);
    if (g_realDwm != NULL) {
        g_dwmFlush = (DwmFlushFn)GetProcAddress(g_realDwm, "DwmFlush");
        g_dwmIsCompositionEnabled = (DwmIsCompositionEnabledFn)GetProcAddress(g_realDwm, "DwmIsCompositionEnabled");
        g_dwmGetCompositionTimingInfo = (DwmGetCompositionTimingInfoFn)GetProcAddress(g_realDwm, "DwmGetCompositionTimingInfo");
        g_dwmSetWindowAttribute = (DwmSetWindowAttributeFn)GetProcAddress(g_realDwm, "DwmSetWindowAttribute");
    }
    return TRUE;
}

static void EnsureRealDwm(void) {
    InitOnceExecuteOnce(&g_dwmInit, LoadRealDwm, NULL, NULL);
}

__declspec(dllexport) HRESULT WINAPI DwmFlush(void) {
    EnsureRealDwm();
    return g_dwmFlush != NULL ? g_dwmFlush() : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI DwmIsCompositionEnabled(BOOL *enabled) {
    EnsureRealDwm();
    return g_dwmIsCompositionEnabled != NULL ? g_dwmIsCompositionEnabled(enabled) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI DwmGetCompositionTimingInfo(HWND window, void *timingInfo) {
    EnsureRealDwm();
    return g_dwmGetCompositionTimingInfo != NULL ? g_dwmGetCompositionTimingInfo(window, timingInfo) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI DwmSetWindowAttribute(HWND window, DWORD attribute, LPCVOID value, DWORD valueSize) {
    EnsureRealDwm();
    return g_dwmSetWindowAttribute != NULL ? g_dwmSetWindowAttribute(window, attribute, value, valueSize) : E_FAIL;
}

typedef struct WindowSearch {
    DWORD processId;
    HWND found;
} WindowSearch;

static BOOL CALLBACK FindGameWindow(HWND window, LPARAM lParam) {
    WindowSearch *search = (WindowSearch *)lParam;
    DWORD ownerProcess = 0;

    GetWindowThreadProcessId(window, &ownerProcess);
    if (ownerProcess != search->processId || !IsWindowVisible(window) || GetWindow(window, GW_OWNER) != NULL) {
        return TRUE;
    }

    search->found = window;
    return FALSE;
}

static DWORD WINAPI BootstrapThread(LPVOID parameter) {
    DWORD deadline = GetTickCount() + 20000;
    WindowSearch search;

    (void)parameter;
    DebugLog(L"bootstrap thread started");
    search.processId = GetCurrentProcessId();
    search.found = NULL;

    while (search.found == NULL && (LONG)(deadline - GetTickCount()) > 0) {
        EnumWindows(FindGameWindow, (LPARAM)&search);
        if (search.found == NULL) {
            Sleep(100);
        }
    }

    if (search.found == NULL) {
        DebugLog(L"no game window found before deadline");
        return 0;
    }

    DebugLog(L"game window found; waiting 10000 ms");

    /* UE has created its game window; wait for the initial UMG screen. */
    Sleep(10000);

    /* The startup window can be recreated during UE's initialization. */
    search.found = NULL;
    deadline = GetTickCount() + 5000;
    while (search.found == NULL && (LONG)(deadline - GetTickCount()) > 0) {
        EnumWindows(FindGameWindow, (LPARAM)&search);
        if (search.found == NULL) {
            Sleep(100);
        }
    }

    if (search.found == NULL) {
        DebugLog(L"no game window found after startup transition");
        return 0;
    }

    if (IsWindow(search.found)) {
        DWORD ownerProcess = 0;
        GetWindowThreadProcessId(search.found, &ownerProcess);
        if (ownerProcess == search.processId) {
            LPARAM keyDown = 1 | ((LPARAM)0x39 << 16);
            LPARAM keyUp = keyDown | ((LPARAM)1 << 30) | ((LPARAM)1 << 31);
            BOOL downPosted = PostMessageW(search.found, WM_KEYDOWN, VK_SPACE, keyDown);
            BOOL charPosted = PostMessageW(search.found, WM_CHAR, VK_SPACE, 0);
            BOOL upPosted = PostMessageW(search.found, WM_KEYUP, VK_SPACE, keyUp);
            DebugLog(downPosted && charPosted && upPosted ? L"Space messages queued" : L"Space message queue failed");
        } else {
            DebugLog(L"game window no longer belongs to this process");
        }
    } else {
        DebugLog(L"game window no longer exists");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_instance = instance;
        DisableThreadLibraryCalls(instance);
        CreateThread(NULL, 0, BootstrapThread, NULL, 0, NULL);
    }
    return TRUE;
}
