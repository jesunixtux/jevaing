#pragma once

namespace Jevaing
{
    enum class Key
    {
        W,
        A,
        S,
        D,
        Up,
        Down,
        Left,
        Right,
        Space,
        Enter,
        Escape,
        Count
    };

    namespace Input
    {
        bool IsKeyDown(Key key);
        bool IsKeyPressed(Key key);
        bool IsKeyReleased(Key key);
    }
}
