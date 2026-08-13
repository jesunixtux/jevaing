# Jevaing public API

The `Api/Include/Jevaing` directory contains the headers intended for client code.

As of Jevaing 0.0.6 BIG BEAR GUMMY the public surface includes:

- `Jevaing.h` — engine version helpers and `Run(...)` entry points.
- `Game.h` — client lifecycle and `GameConfig`.
- `Graphics2D.h` — backend-neutral immediate 2D drawing interface.
- `Input.h` — basic keyboard key/state queries.
- `Types.h` — small public value types such as `Vec2` and `Color`.

Client code should prefer these headers instead of including files from `Engine/Core`, `Engine/Platform` or `Engine/Renderer` directly.

The API is pre-1.0 and may change while Jevaing is still establishing its runtime, asset and scene foundations.
