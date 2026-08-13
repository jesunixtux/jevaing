#pragma once

namespace Jevaing
{
    // Returns the current engine version.
    const char* GetVersion();

    // Returns the current development codename.
    const char* GetCodename();

    // Starts Jevaing without command-line arguments.
    int Run();

    // Starts Jevaing and forwards command-line arguments to the engine.
    int Run(int argc, char** argv);
}
