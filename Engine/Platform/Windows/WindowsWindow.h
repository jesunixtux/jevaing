#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace Jevaing::Platform
{
    class WindowsWindow
    {
    public:
        WindowsWindow();
        ~WindowsWindow();

        bool Create(
            const wchar_t* title,
            int width,
            int height
        );

        bool ProcessMessages();

        void Show();

    private:
        static LRESULT CALLBACK WindowProc(
            HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        );

    private:
        HINSTANCE m_instance = nullptr;
        HWND m_window = nullptr;
        const wchar_t* m_className = L"JevaingWindowClass";
    };
}

#endif