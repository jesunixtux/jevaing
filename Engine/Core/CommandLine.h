#pragma once

#include <cstdint>
#include <string>

namespace Jevaing::Internal
{
    struct CommandLineOptions
    {
        bool ShowHelp = false;
        bool ShowVersion = false;
        bool SelfTest = false;
        bool ShowRendererInfo = false;
        bool GraphicsTest = false;
        bool PenguinGraphicsTest = false;
        bool GraphicsTest3D = false;
        bool PenguinTest3D = false;
        bool Gummy3DTest = false;
        bool ModelTest = false;
        bool TextureTest = false;
        bool MaterialTest = false;
        bool LightingTest = false;
        bool MultiModelTest = false;
        bool AssetCacheTest = false;
        bool AssetErrorTest = false;
        bool AssetInfo = false;
        bool Mixed2D3DTest = false;
        bool RuntimeTest = false;
        bool HasFrameLimit = false;
        std::uint64_t FrameLimit = 0;
        std::string Renderer;
        std::string ModelTestPath;
        std::string AssetInfoPath;
    };

    bool ParseCommandLine(
        int argc,
        char** argv,
        CommandLineOptions& options,
        std::string& error
    );

    void PrintCommandLineHelp();
}
