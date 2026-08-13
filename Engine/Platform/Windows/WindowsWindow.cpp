#ifdef _WIN32

#include "WindowsWindow.h"

#include <string>

#include "../../Core/InputState.h"

namespace
{
    std::wstring Utf8ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int requiredSize = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0
        );

        if (requiredSize <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<std::size_t>(requiredSize), L'\0');

        const int convertedSize = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            requiredSize
        );

        if (convertedSize != requiredSize)
        {
            return {};
        }

        return result;
    }

    bool TryMapKey(WPARAM virtualKey, Jevaing::Key& key)
    {
        switch (virtualKey)
        {
            case 'W': key = Jevaing::Key::W; return true;
            case 'A': key = Jevaing::Key::A; return true;
            case 'S': key = Jevaing::Key::S; return true;
            case 'D': key = Jevaing::Key::D; return true;
            case VK_UP: key = Jevaing::Key::Up; return true;
            case VK_DOWN: key = Jevaing::Key::Down; return true;
            case VK_LEFT: key = Jevaing::Key::Left; return true;
            case VK_RIGHT: key = Jevaing::Key::Right; return true;
            case VK_SPACE: key = Jevaing::Key::Space; return true;
            case VK_RETURN: key = Jevaing::Key::Enter; return true;
            case VK_ESCAPE: key = Jevaing::Key::Escape; return true;
            default: return false;
        }
    }
}

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

        if (m_classRegistered && m_instance)
        {
            UnregisterClassW(m_className, m_instance);
            m_classRegistered = false;
        }
    }

    bool WindowsWindow::Initialize(const Internal::WindowConfig& config)
    {
        if (!m_instance || config.Width <= 0 || config.Height <= 0)
        {
            return false;
        }

        const std::wstring title = Utf8ToWide(config.Title);

        if (!config.Title.empty() && title.empty())
        {
            return false;
        }

        WNDCLASSW windowClass = {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = m_instance;
        windowClass.lpszClassName = m_className;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

        if (!RegisterClassW(&windowClass))
        {
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }
        }
        else
        {
            m_classRegistered = true;
        }

        RECT windowRect = { 0, 0, config.Width, config.Height };

        if (!AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE))
        {
            return false;
        }

        const int windowWidth = windowRect.right - windowRect.left;
        const int windowHeight = windowRect.bottom - windowRect.top;
        const wchar_t* windowTitle = title.empty() ? L"Jevaing" : title.c_str();

        m_window = CreateWindowExW(
            0,
            m_className,
            windowTitle,
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

        return m_window != nullptr;
    }

    void WindowsWindow::Show()
    {
        ShowWindow(m_window, SW_SHOW);
        UpdateWindow(m_window);
    }

    bool WindowsWindow::ProcessEvents()
    {
        MSG message = {};

        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
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

    void* WindowsWindow::GetNativeHandle() const
    {
        return static_cast<void*>(m_window);
    }

    int WindowsWindow::GetWidth() const
    {
        if (!m_window)
        {
            return 0;
        }

        RECT clientRect = {};
        return GetClientRect(m_window, &clientRect)
            ? clientRect.right - clientRect.left
            : 0;
    }

    int WindowsWindow::GetHeight() const
    {
        if (!m_window)
        {
            return 0;
        }

        RECT clientRect = {};
        return GetClientRect(m_window, &clientRect)
            ? clientRect.bottom - clientRect.top
            : 0;
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
                Jevaing::Key key;

                if (TryMapKey(wParam, key))
                {
                    Internal::InputState::SetKeyState(key, true);
                }

                if (wParam == VK_ESCAPE)
                {
                    DestroyWindow(hwnd);
                    return 0;
                }

                break;
            }

            case WM_KEYUP:
            {
                Jevaing::Key key;

                if (TryMapKey(wParam, key))
                {
                    Internal::InputState::SetKeyState(key, false);
                }

                break;
            }

            case WM_KILLFOCUS:
            {
                Internal::InputState::Reset();
                return 0;
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

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

#endif
