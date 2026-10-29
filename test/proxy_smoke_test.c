#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

typedef HRESULT (WINAPI *DwmFlushFn)(void);
typedef HRESULT (WINAPI *DwmIsCompositionEnabledFn)(BOOL *);

int main(void) {
    WCHAR modulePath[MAX_PATH];
    HMODULE proxy = LoadLibraryW(L".\\dwmapi.dll");
    DwmFlushFn flush;
    DwmIsCompositionEnabledFn isCompositionEnabled;
    BOOL enabled = FALSE;
    HRESULT status;

    if (proxy == NULL) {
        fprintf(stderr, "LoadLibraryW failed: %lu\n", GetLastError());
        return 1;
    }
    if (GetModuleFileNameW(proxy, modulePath, MAX_PATH) == 0 || wcsstr(modulePath, L".\\") != NULL) {
        fprintf(stderr, "Could not resolve loaded proxy path.\n");
        return 5;
    }
    wprintf(L"PROXY_PATH=%ls\n", modulePath);

    flush = (DwmFlushFn)GetProcAddress(proxy, "DwmFlush");
    isCompositionEnabled = (DwmIsCompositionEnabledFn)GetProcAddress(proxy, "DwmIsCompositionEnabled");
    if (flush == NULL || isCompositionEnabled == NULL) {
        fprintf(stderr, "Expected proxy exports are missing.\n");
        return 2;
    }

    status = isCompositionEnabled(&enabled);
    if (FAILED(status)) {
        fprintf(stderr, "DwmIsCompositionEnabled failed: 0x%08lx\n", (unsigned long)status);
        return 3;
    }

    status = flush();
    if (FAILED(status)) {
        fprintf(stderr, "DwmFlush failed: 0x%08lx\n", (unsigned long)status);
        return 4;
    }

    printf("PROXY_SMOKE_TEST_OK composition=%d\n", enabled ? 1 : 0);
    return 0;
}
