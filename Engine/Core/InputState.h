#pragma once

#include <Jevaing/Input.h>

namespace Jevaing::Internal::InputState
{
    void BeginFrame();
    void SetKeyState(Key key, bool isDown);
    void Reset();
}
