#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

__declspec(dllimport) HRESULT WINAPI DwmIsCompositionEnabled(BOOL *enabled);

int main(void) {
    WCHAR modulePath[MAX_PATH];
    HMODULE module;
    BOOL enabled = FALSE;
    HRESULT status = DwmIsCompositionEnabled(&enabled);

    if (FAILED(status)) {
        fprintf(stderr, "Imported DwmIsCompositionEnabled failed: 0x%08lx\n", (unsigned long)status);
        return 1;
    }

    module = GetModuleHandleW(L"dwmapi.dll");
    if (module == NULL || GetModuleFileNameW(module, modulePath, MAX_PATH) == 0) {
        fprintf(stderr, "Dwmapi module path is unavailable.\n");
        return 2;
    }

    wprintf(L"IMPORT_SMOKE_TEST_OK composition=%d module=%ls\n", enabled ? 1 : 0, modulePath);
    return 0;
}
