# Editor / Editor

The Windows ImGui editor currently has Project Browser, Hierarchy, Scene, Game, Inspector, Project, Console and Build Settings panels, 2D/3D preview modes, basic component edits and Play/Pause/Step/Stop controls. Build Settings lists project scenes, lets the developer choose the included scenes and startup scene, and requires explicit project build trust.

Play serializes the editable scene to a temporary snapshot, loads a runtime scene copy, and discards that copy on Stop. It **does not execute the project's compiled game code**. Scene/Game views are prototype previews, not final runtime render targets. Prefabs, Undo/Redo, autosave, asset/module managers and full project settings remain pending. CMake build trust must be checked anew for each opened project.

When no entity is selected, Inspector shows Scene Settings for enabling and editing the procedural 3D skybox. See `documents/SKYBOX.md`.
