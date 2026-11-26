#include <Jevaing/ModContent.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static std::string FirstLine(const std::string& path)
{
    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    return line;
}

int main(int argc, char** argv)
{
    if (argc != 2 || !argv[1]) return 2;
    const fs::path root = fs::weakly_canonical(argv[1]);
    const fs::path game = root / "Game";
    const fs::path mods = root / "Mods";
    const fs::path example = mods / "Example" / "mod.jevmod";
    std::string path, error;

    Jevaing::ModContentManager disabled(game.string(), mods.string(), "SampleGame", 1, {});
    if (disabled.Mount(example.string(), error) || disabled.Count() != 0 ||
        !disabled.Resolve(Jevaing::ModContentKind::Asset, "common.txt", path, error) ||
        FirstLine(path) != "original game asset" ||
        disabled.Resolve(Jevaing::ModContentKind::Asset, "extra.txt", path, error))
    {
        std::cerr << "Disabled mod policy failed.\n";
        return 2;
    }

    Jevaing::ModPolicy policy;
    policy.Enabled = true;
    policy.AllowAssets = true;
    policy.AllowMaps = true;
    Jevaing::ModContentManager mounted(game.string(), mods.string(), "SampleGame", 1, policy);
    if (!mounted.Mount(example.string(), error) || mounted.Count() != 2 ||
        !mounted.Mount(example.string(), error) || mounted.Count() != 2 ||
        !mounted.Resolve(Jevaing::ModContentKind::Asset, "common.txt", path, error) ||
        FirstLine(path) != "original game asset" ||
        !mounted.Resolve(Jevaing::ModContentKind::Asset, "extra.txt", path, error) ||
        FirstLine(path) != "extra mod asset" ||
        !mounted.Resolve(Jevaing::ModContentKind::Map, "level.map", path, error) ||
        FirstLine(path) != "custom map" ||
        mounted.Resolve(Jevaing::ModContentKind::Asset, "../outside", path, error))
    {
        std::cerr << "Mod mount or content lookup failed: " << error << '\n';
        return 2;
    }
    mounted.UnmountAll();
    if (mounted.Count() != 0 ||
        mounted.Resolve(Jevaing::ModContentKind::Asset, "extra.txt", path, error))
    { std::cerr << "Mod unmount failed.\n"; return 2; }
    for (const char* invalid : {"BadGame", "BadPath", "CycleA"})
    {
        if (mounted.Mount((mods / invalid / "mod.jevmod").string(), error) ||
            mounted.Count() != 0)
        { std::cerr << "Invalid mod accepted: " << invalid << '\n'; return 2; }
    }

    policy.AllowAssetOverrides = true;
    Jevaing::ModContentManager overrides(game.string(), mods.string(), "SampleGame", 1, policy);
    if (!overrides.Mount(example.string(), error) ||
        !overrides.Resolve(Jevaing::ModContentKind::Asset, "common.txt", path, error) ||
        FirstLine(path) != "modded asset" ||
        FirstLine((game / "Assets" / "common.txt").string()) != "original game asset")
    { std::cerr << "Mod overlay changed original content.\n"; return 2; }

    std::cout << "[PASS] Optional mod mounts, policies, dependencies and overlays.\n";
    return 0;
}
