#include <Jevaing/Input.h>

#include <array>
#include <cstddef>

#include "InputState.h"

namespace
{
    constexpr std::size_t KeyCount =
        static_cast<std::size_t>(Jevaing::Key::Count);

    std::array<bool, KeyCount> CurrentKeys = {};
    std::array<bool, KeyCount> PreviousKeys = {};

    std::size_t ToIndex(Jevaing::Key key)
    {
        return static_cast<std::size_t>(key);
    }

    bool IsValidKey(Jevaing::Key key)
    {
        return ToIndex(key) < KeyCount;
    }
}

namespace Jevaing::Internal::InputState
{
    void BeginFrame()
    {
        PreviousKeys = CurrentKeys;
    }

    void SetKeyState(Key key, bool isDown)
    {
        if (!IsValidKey(key))
        {
            return;
        }

        CurrentKeys[ToIndex(key)] = isDown;
    }

    void Reset()
    {
        CurrentKeys.fill(false);
        PreviousKeys.fill(false);
    }
}

namespace Jevaing::Input
{
    bool IsKeyDown(Key key)
    {
        return IsValidKey(key) && CurrentKeys[ToIndex(key)];
    }

    bool IsKeyPressed(Key key)
    {
        if (!IsValidKey(key))
        {
            return false;
        }

        const std::size_t index = ToIndex(key);
        return CurrentKeys[index] && !PreviousKeys[index];
    }

    bool IsKeyReleased(Key key)
    {
        if (!IsValidKey(key))
        {
            return false;
        }

        const std::size_t index = ToIndex(key);
        return !CurrentKeys[index] && PreviousKeys[index];
    }
}
