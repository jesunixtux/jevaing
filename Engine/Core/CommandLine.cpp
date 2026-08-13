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
            else if (argument == "--penguin-test-3d")
            {
                options.PenguinTest3D = true;
            }
            else if (argument == "--gummy3d-test")
            {
                options.Gummy3DTest = true;
            }
            else if (argument == "--model-test")
            {
                if (index + 1 >= argc)
                {
                    error = "--model-test requires a model path.";
                    return false;
                }

                options.ModelTest = true;
                options.ModelTestPath = argv[++index] ? argv[index] : "";
            }
            else if (argument == "--texture-test")
            {
                options.TextureTest = true;
            }
            else if (argument == "--material-test")
            {
                options.MaterialTest = true;
            }
            else if (argument == "--lighting-test")
            {
                options.LightingTest = true;
            }
            else if (argument == "--multi-model-test")
            {
                options.MultiModelTest = true;
            }
            else if (argument == "--asset-cache-test")
            {
                options.AssetCacheTest = true;
            }
            else if (argument == "--asset-error-test")
            {
                options.AssetErrorTest = true;
            }
            else if (argument == "--asset-info")
            {
                if (index + 1 >= argc)
                {
                    error = "--asset-info requires an asset path.";
                    return false;
                }

                options.AssetInfo = true;
                options.AssetInfoPath = argv[++index] ? argv[index] : "";
            }
            else if (argument == "--mixed-2d-3d-test")
            {
                options.Mixed2D3DTest = true;
            }
            else if (argument == "--scene-test")
            {
                options.SceneTest = true;
            }
            else if (argument == "--scene-serialization-test")
            {
                options.SceneSerializationTest = true;
            }
            else if (argument == "--hierarchy-test")
            {
                options.HierarchyTest = true;
            }
            else if (argument == "--mouse-test")
            {
                options.MouseTest = true;
            }
            else if (argument == "--sprite-test")
            {
                options.SpriteTest = true;
            }
            else if (argument == "--gpu-mesh-test")
            {
                options.GpuMeshTest = true;
            }
            else if (argument == "--project-test")
            {
                if (index + 1 >= argc)
                {
                    error = "--project-test requires a project path.";
                    return false;
                }

                options.ProjectTest = true;
                options.ProjectTestPath = argv[++index] ? argv[index] : "";
            }
            else if (argument == "--physics-info")
            {
                options.PhysicsInfo = true;
            }
            else if (argument == "--physics-fixed-step-test")
            {
                options.PhysicsFixedStepTest = true;
            }
            else if (argument == "--physics-3d-test")
            {
                options.Physics3DTest = true;
            }
            else if (argument == "--physics-3d-stack-test")
            {
                options.Physics3DStackTest = true;
            }
            else if (argument == "--physics-3d-sphere-test")
            {
                options.Physics3DSphereTest = true;
            }
            else if (argument == "--physics-3d-trigger-test")
            {
                options.Physics3DTriggerTest = true;
            }
            else if (argument == "--physics-3d-raycast-test")
            {
                options.Physics3DRaycastTest = true;
            }
            else if (argument == "--physics-2d-test")
            {
                options.Physics2DTest = true;
            }
            else if (argument == "--physics-2d-circle-test")
            {
                options.Physics2DCircleTest = true;
            }
            else if (argument == "--physics-2d-trigger-test")
            {
                options.Physics2DTriggerTest = true;
            }
            else if (argument == "--physics-2d-raycast-test")
            {
                options.Physics2DRaycastTest = true;
            }
            else if (argument == "--physics-scene-serialization-test")
            {
                options.PhysicsSceneSerializationTest = true;
            }
            else if (argument == "--physics-destroy-test")
            {
                options.PhysicsDestroyTest = true;
            }
            else if (argument == "--physics-hierarchy-test")
            {
                options.PhysicsHierarchyTest = true;
            }
            else if (argument == "--build-target-info")
            {
                options.BuildTargetInfo = true;
            }
            else if (argument == "--gamepad-test")
            {
                options.GamepadTest = true;
            }
            else if (argument == "--project-template-test")
            {
                options.ProjectTemplateTest = true;
            }
            else if (argument == "--editor-scene-roundtrip-test")
            {
                options.EditorSceneRoundTripTest = true;
            }
            else if (argument == "--playmode-restore-test")
            {
                options.PlayModeRestoreTest = true;
            }
            else if (argument == "--windows-build-test")
            {
                options.WindowsBuildTest = true;
            }
            else if (argument == "--xbox-build-environment-test")
            {
                options.XboxBuildEnvironmentTest = true;
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
            << "  --graphics-test-penguin    Run the 2D penguin GPU smoke test.\n"
            << "  --graphics-test-3d         Run the rotating cube GPU smoke test.\n"
            << "  --penguin-test-3d          Load tux.glb and rotate it in 3D.\n"
            << "  --gummy3d-test             Load gummybear.fbx and rotate it in 3D.\n"
            << "  --model-test <path>        Load any supported model and rotate it in 3D.\n"
            << "  --texture-test             Draw a textured plane using Texture2D.\n"
            << "  --material-test            Draw objects with independent materials.\n"
            << "  --lighting-test            Draw lit 3D geometry with a directional light.\n"
            << "  --multi-model-test         Draw tux, gummybear and a Jevaing cube together.\n"
            << "  --asset-cache-test         Run headless AssetManager cache validation.\n"
            << "  --asset-error-test         Run headless asset error-path validation.\n"
            << "  --asset-info <path>        Print model asset information without a window.\n"
            << "  --mixed-2d-3d-test         Draw a 3D scene with a 2D overlay.\n"
            << "  --scene-test               Render a simple Scene with hierarchy/components.\n"
            << "  --scene-serialization-test Run headless Scene save/load validation.\n"
            << "  --hierarchy-test           Run headless transform hierarchy validation.\n"
            << "  --mouse-test               Show mouse state in a visual test.\n"
            << "  --sprite-test              Draw a textured SpriteRenderer2D test.\n"
            << "  --gpu-mesh-test            Validate persistent GPU mesh reuse.\n"
            << "  --project-test <path>      Validate a jevaing.project file.\n"
            << "  --physics-info             Show physics backend availability.\n"
            << "  --physics-fixed-step-test  Run deterministic fixed-step physics validation.\n"
            << "  --physics-3d-test          Run 3D box physics validation.\n"
            << "  --physics-3d-stack-test    Run 3D box stack physics validation.\n"
            << "  --physics-3d-sphere-test   Run 3D sphere physics validation.\n"
            << "  --physics-3d-trigger-test  Run 3D trigger event validation.\n"
            << "  --physics-3d-raycast-test  Run 3D raycast validation.\n"
            << "  --physics-2d-test          Run 2D box physics validation.\n"
            << "  --physics-2d-circle-test   Run 2D circle physics validation.\n"
            << "  --physics-2d-trigger-test  Run 2D trigger event validation.\n"
            << "  --physics-2d-raycast-test  Run 2D raycast validation.\n"
            << "  --physics-scene-serialization-test Run physics Scene save/load validation.\n"
            << "  --physics-destroy-test     Validate physics body cleanup on DestroyEntity.\n"
            << "  --physics-hierarchy-test   Validate dynamic-parent rejection policy.\n"
            << "  --build-target-info        Show platform/build target availability.\n"
            << "  --gamepad-test             Print neutral gamepad state once.\n"
            << "  --project-template-test    Create and validate a temporary project template.\n"
            << "  --editor-scene-roundtrip-test Validate editor Scene save/load behavior.\n"
            << "  --playmode-restore-test    Validate Play/Stop restores edit transforms.\n"
            << "  --windows-build-test       Build a temporary project if CMake/toolchain is available.\n"
            << "  --xbox-build-environment-test Report Xbox SDK/GDK prerequisites only.\n"
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
            << "  JevaingSandbox.exe --penguin-test-3d\n"
            << "  JevaingSandbox.exe --gummy3d-test\n"
            << "  JevaingSandbox.exe --model-test geometry/3D/.hide/easter/tux.glb\n"
            << "  JevaingSandbox.exe --texture-test\n"
            << "  JevaingSandbox.exe --material-test\n"
            << "  JevaingSandbox.exe --lighting-test\n"
            << "  JevaingSandbox.exe --multi-model-test\n"
            << "  JevaingSandbox.exe --asset-info geometry/3D/.hide/easter/tux.glb\n"
            << "  JevaingSandbox.exe --runtime-test\n"
            << "  JevaingSandbox.exe --renderer directx --frames 300\n"
            << "  JevaingSandbox.exe --renderer null --frames 60\n";
    }
}
