#include "CommandLine.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace Jevaing::Internal
{
    bool ParseCommandLine(
        int argc,
        char** argv,
        CommandLineOptions& options,
        std::string& error
    )
    {
        for (int index = 1; index < argc; ++index)
        {
            const std::string argument = argv[index] ? argv[index] : "";

            if (argument == "--help" || argument == "-h")
            {
                options.ShowHelp = true;
            }
            else if (argument == "--version")
            {
                options.ShowVersion = true;
            }
            else if (argument == "--self-test")
            {
                options.SelfTest = true;
            }
            else if (argument == "--renderer-info")
            {
                options.ShowRendererInfo = true;
            }
            else if (argument == "--graphics-test")
            {
                options.GraphicsTest = true;
            }
            else if (argument == "--graphics-test-penguin")
            {
                options.PenguinGraphicsTest = true;
            }
            else if (argument == "--graphics-test-3d")
            {
                options.GraphicsTest3D = true;
            }
            else if (argument == "--runtime-test")
            {
                options.RuntimeTest = true;
            }
            else if (argument == "--renderer")
            {
                if (index + 1 >= argc)
                {
                    error = "--renderer requires a value.";
                    return false;
                }

                options.Renderer = argv[++index] ? argv[index] : "";
            }
            else if (argument == "--frames")
            {
                if (index + 1 >= argc)
                {
                    error = "--frames requires a positive integer.";
                    return false;
                }

                const std::string value = argv[++index] ? argv[index] : "";

                const bool containsOnlyDigits =
                    !value.empty() &&
                    std::all_of(
                        value.begin(),
                        value.end(),
                        [](unsigned char character)
                        {
                            return std::isdigit(character) != 0;
                        }
                    );

                if (!containsOnlyDigits)
                {
                    error = "Invalid value for --frames: " + value;
                    return false;
                }

                try
                {
                    const unsigned long long parsed = std::stoull(value);

                    if (parsed == 0)
                    {
                        error = "--frames requires a positive integer.";
                        return false;
                    }

                    options.FrameLimit = static_cast<std::uint64_t>(parsed);
                    options.HasFrameLimit = true;
                }
                catch (const std::exception&)
                {
                    error = "Invalid value for --frames: " + value;
                    return false;
                }
            }
            else
            {
                error = "Unknown command-line option: " + argument;
                return false;
            }
        }

        return true;
    }

    void PrintCommandLineHelp()
    {
        std::cout
            << "Jevaing command-line options\n\n"
            << "  --help, -h                 Show this help.\n"
            << "  --version                  Print the engine version and exit.\n"
            << "  --self-test                Run headless core tests.\n"
            << "  --renderer-info            Show renderer backend availability.\n"
            << "  --graphics-test            Run the colored triangle GPU smoke test.\n"
            << "  --graphics-test-penguin    Run the penguin GPU smoke test.\n"
            << "  --graphics-test-3d         Run the rotating cube GPU smoke test.\n"
            << "  --runtime-test             Run client callbacks for a fixed smoke test.\n"
            << "  --renderer <backend>       Select: directx, null, vulkan, metal.\n"
            << "  --frames <count>           Exit automatically after N frames.\n\n"
            << "Examples:\n"
            << "  JevaingSandbox.exe --version\n"
            << "  JevaingSandbox.exe --self-test\n"
            << "  JevaingSandbox.exe --renderer-info\n"
            << "  JevaingSandbox.exe --graphics-test\n"
            << "  JevaingSandbox.exe --graphics-test-penguin\n"
            << "  JevaingSandbox.exe --graphics-test-3d\n"
            << "  JevaingSandbox.exe --runtime-test\n"
            << "  JevaingSandbox.exe --renderer directx --frames 300\n"
            << "  JevaingSandbox.exe --renderer null --frames 60\n";
    }
}
