#pragma once

#include <Jevaing/Input.h>

namespace Jevaing::Internal::InputState
{
    void BeginFrame();
    void SetKeyState(Key key, bool isDown);
    void SetMouseButtonState(MouseButton button, bool isDown);
    void SetMousePosition(float x, float y);
    void AddMouseWheelDelta(float delta);
    void Reset();
}
