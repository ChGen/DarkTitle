#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <unordered_map>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

//Non-public Win32 APIs. Credits to:
//https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h
//https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
//https://github.com/notepad-plus-plus/notepad-plus-plus/labels/dark%20mode

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_USEDARKMODECOLORS = 26,
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;

bool _isLight = false;
bool _mark = false;
std::unordered_map<HWND, bool> _titles;

bool isLightTheme() {
    DWORD value = 0;
    DWORD cbData = static_cast<DWORD>(sizeof(value));
    auto res = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD, NULL, &value, &cbData
    );
    if (res != ERROR_SUCCESS) {
        return true;
    }
    return value == 1;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    if (!hUser32)
        return 1;
    _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));
    if (!_SetWindowCompositionAttribute)
        return 2;

    BOOL dark = TRUE;
    WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    WNDENUMPROC proc = [](HWND hWnd, LPARAM lParam)
    {
        BOOL val = !_isLight;
        if (_titles.find(hWnd) == _titles.cend())
            DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val));
        _titles[hWnd] = _mark;
        return TRUE;
    };

    _isLight = isLightTheme();
    while (true)
    {
        EnumWindows(proc, (LPARAM)&data);

        for (auto it = _titles.begin(); it != _titles.end();)
        {
            if (it->second != _mark)
                it = _titles.erase(it);
            else
                ++it;
        }
        Sleep(75);
        _mark = !_mark;
        bool status = isLightTheme();
        if (status != _isLight)
        {
            _isLight = status;
            _titles.clear();
        }
    }
    return 0;
}
