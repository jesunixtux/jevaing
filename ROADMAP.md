# Roadmap / Hoja de ruta

Status on 2026-11-25: early prototype; closed beta is not approved.

1. Finish security: project and asset path validation, transactional save and recovery tests, isolated export staging and runtime dependency audit.
2. Verify Debug and Release on CI and a clean Windows machine; execute exported games with selected scenes.
3. Complete and test C ABI v1 beyond the implemented scene, box physics and texture slice: more components, collision events, model assets, audio and complete game samples.
4. Make editor Play run the actual game code in a separate process while preserving edit state.
5. Separate runtime and SDK build products. Add versioned native module interfaces and dependency resolution.
6. Add opt-in mods and Lua, with native plugins treated as trusted code.
7. Add audio, animation and save checkpoints for a JumpFall prequel prototype.
8. Add optional authenticated, encrypted project recovery after key management and threat model review.

Linux, macOS, Vulkan, Metal and Xbox remain unverified or unimplemented as noted in the existing documentation.
