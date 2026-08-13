#include "Window.h"

#ifdef _WIN32
#include "../Platform/Windows/WindowsWindow.h"
#endif

namespace Jevaing::Internal
{
    std::unique_ptr<Window> Window::Create(const WindowConfig& config)
    {
        std::unique_ptr<Window> window;

#ifdef _WIN32
        window = std::make_unique<Jevaing::Platform::WindowsWindow>();
#else
        return nullptr;
#endif

        if (!window->Initialize(config))
        {
            return nullptr;
        }

        return window;
    }
}
