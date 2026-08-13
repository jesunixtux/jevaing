#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "../../Core/Window.h"

namespace Jevaing::Platform
{
    class WindowsWindow final : public Internal::Window
    {
    public:
        WindowsWindow();
        ~WindowsWindow() override;

        void Show() override;
        bool ProcessEvents() override;
        void* GetNativeHandle() const override;
        int GetWidth() const override;
        int GetHeight() const override;

    protected:
        bool Initialize(const Internal::WindowConfig& config) override;

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
        bool m_classRegistered = false;
    };
}

#endif
