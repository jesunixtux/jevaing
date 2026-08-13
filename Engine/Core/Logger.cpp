#include "Logger.h"

#include <iostream>

namespace Jevaing::Internal
{
    void Logger::Info(const std::string& message)
    {
        Write(LogLevel::Info, message);
    }

    void Logger::Warning(const std::string& message)
    {
        Write(LogLevel::Warning, message);
    }

    void Logger::Error(const std::string& message)
    {
        Write(LogLevel::Error, message);
    }

    void Logger::Write(LogLevel level, const std::string& message)
    {
        const char* label = "INFO";
        std::ostream* stream = &std::cout;

        switch (level)
        {
            case LogLevel::Warning:
                label = "WARN";
                stream = &std::clog;
                break;

            case LogLevel::Error:
                label = "ERROR";
                stream = &std::cerr;
                break;

            case LogLevel::Info:
            default:
                break;
        }

        (*stream) << "[Jevaing][" << label << "] " << message << std::endl;
    }
}
