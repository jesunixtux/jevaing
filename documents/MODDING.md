# Modding / Modificaciones

Experimental content-only mod mounts are available through `-DJEVAING_ENABLE_MODS=ON` and the separate `JevaingMods` library. The option is **off by default**, and games must instantiate `ModContentManager` with `ModPolicy.Enabled=true`. The policy independently allows assets, custom maps and overrides. When overrides are off, original game files win; mod-only files are still available. With overrides on, a mod file takes precedence during lookup, but the base file is not changed.

Each package lives in `Mods/<Name>/mod.jevmod` and declares `name`, semantic `version`, `game`, `gameMajor`, relative `contentRoot` and optional `dependency=Name@major` entries. The resolver rejects incompatible games/major versions, cycles and traversal paths. It returns a physical path for the game to load; existing engine asset APIs do not automatically use this resolver yet. `Samples/ModContent` verifies disabled and enabled policies, dependencies, additions and overlays.

Lua and native mod plugin execution are not integrated. No permissions in a manifest can isolate native code. Developers should only allow native plugins from sources they trust. Games that do not link `JevaingMods` exclude this infrastructure from their executable.
