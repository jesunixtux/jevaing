#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Jevaing::Internal
{
    inline bool WriteAtomicFile(const std::filesystem::path& destination,
        const std::function<void(std::ostream&)>& write, std::string& error)
    {
        namespace fs = std::filesystem;
        static std::atomic<unsigned long long> sequence{0};
        const fs::path temporary = destination.string() + ".tmp." +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
            "." + std::to_string(++sequence);
        const fs::path backup = destination.string() + ".bak";
        std::error_code ec;
        if (fs::is_symlink(backup, ec))
        {
            error = "Backup path is a symbolic link: " + backup.string();
            return false;
        }
        ec.clear();
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                error = "Cannot open temporary file: " + temporary.string();
                return false;
            }
            write(output);
            output.flush();
            if (!output)
            {
                error = "Cannot finish temporary file: " + temporary.string();
                output.close();
                fs::remove(temporary, ec);
                return false;
            }
            output.close();
            if (!output)
            {
                error = "Cannot close temporary file: " + temporary.string();
                fs::remove(temporary, ec);
                return false;
            }
        }
        if (fs::exists(destination, ec) && !ec)
        {
            fs::copy_file(destination, backup, fs::copy_options::overwrite_existing, ec);
            if (ec)
            {
                error = "Cannot back up file: " + ec.message();
                fs::remove(temporary, ec);
                return false;
            }
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            error = "Cannot replace file: Windows error " + std::to_string(GetLastError());
            fs::remove(temporary, ec);
            return false;
        }
#else
        fs::rename(temporary, destination, ec);
        if (ec)
        {
            error = "Cannot replace file: " + ec.message();
            fs::remove(temporary, ec);
            return false;
        }
#endif
        return true;
    }
}
