# Modding / Modificaciones

Mod loading is not implemented. It is disabled by default because no mod runtime or package loader exists. Planned controls are per-game opt-in for content, maps, Lua scripts and trusted native plugins. Content packages must mount without overwriting original game files.

No permissions in a manifest can isolate native code. Developers should only allow native plugins from sources they trust. A future CMake option should exclude mod infrastructure from games that do not use it.
