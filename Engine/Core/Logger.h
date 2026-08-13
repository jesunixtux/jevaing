#pragma once

#include <functional>
#include <string>

namespace Jevaing::Internal
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    class Logger
    {
    public:
        using Sink = std::function<void(LogLevel, const std::string&)>;

        static void Info(const std::string& message);
        static void Warning(const std::string& message);
        static void Error(const std::string& message);
        static void SetSink(Sink sink);

    private:
        static void Write(LogLevel level, const std::string& message);
    };
}
