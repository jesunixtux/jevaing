#ifdef _WIN32

#include "WindowsWindow.h"

namespace Jevaing::Platform
{
    WindowsWindow::WindowsWindow()
    {
        m_instance = GetModuleHandleW(nullptr);
    }

    WindowsWindow::~WindowsWindow()
    {
        if (m_window)
        {
            DestroyWindow(m_window);
            m_window = nullptr;
        }

        if (m_instance)
        {
            UnregisterClassW(
                m_className,
                m_instance
            );
        }
    }

    bool WindowsWindow::Create(
        const wchar_t* title,
        int width,
        int height
    )
    {
        WNDCLASSW windowClass = {};

        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = m_instance;
        windowClass.lpszClassName = m_className;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

        if (!RegisterClassW(&windowClass))
        {
            return false;
        }

        RECT windowRect = {
            0,
            0,
            width,
            height
        };

        AdjustWindowRect(
            &windowRect,
            WS_OVERLAPPEDWINDOW,
            FALSE
        );

        const int windowWidth =
            windowRect.right -
            windowRect.left;

        const int windowHeight =
            windowRect.bottom -
            windowRect.top;

        m_window = CreateWindowExW(
            0,
            m_className,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowWidth,
            windowHeight,
            nullptr,
            nullptr,
            m_instance,
            nullptr
        );

        if (!m_window)
        {
            return false;
        }

        return true;
    }

    void WindowsWindow::Show()
    {
        ShowWindow(
            m_window,
            SW_SHOW
        );

        UpdateWindow(m_window);
    }

    bool WindowsWindow::ProcessMessages()
    {
        MSG message = {};

        while (
            PeekMessageW(
                &message,
                nullptr,
                0,
                0,
                PM_REMOVE
            )
        )
        {
            if (message.message == WM_QUIT)
            {
                m_window = nullptr;
                return false;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return true;
    }

    LRESULT CALLBACK WindowsWindow::WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        switch (message)
        {
            case WM_KEYDOWN:
            {
                if (wParam == VK_ESCAPE)
                {
                    DestroyWindow(hwnd);
                    return 0;
                }

                break;
            }

            case WM_CLOSE:
            {
                DestroyWindow(hwnd);
                return 0;
            }

            case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }
        }

        return DefWindowProcW(
            hwnd,
            message,
            wParam,
            lParam
        );
    }
}

#endif
