#pragma once

#include <memory>
#include <string>

namespace Jevaing::Internal
{
    struct WindowConfig
    {
        std::string Title = "Jevaing";
        int Width = 1280;
        int Height = 720;
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        virtual void Show() = 0;
        virtual bool ProcessEvents() = 0;
        virtual void* GetNativeHandle() const = 0;
        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;

        static std::unique_ptr<Window> Create(const WindowConfig& config);

    protected:
        virtual bool Initialize(const WindowConfig& config) = 0;
    };
}
