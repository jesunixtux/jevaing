#include <Jevaing/Input.h>

#include <array>
#include <cstddef>

#include "InputState.h"

namespace
{
    constexpr std::size_t KeyCount =
        static_cast<std::size_t>(Jevaing::Key::Count);
    constexpr std::size_t MouseButtonCount =
        static_cast<std::size_t>(Jevaing::MouseButton::Count);

    std::array<bool, KeyCount> CurrentKeys = {};
    std::array<bool, KeyCount> PreviousKeys = {};
    std::array<bool, MouseButtonCount> CurrentMouseButtons = {};
    std::array<bool, MouseButtonCount> PreviousMouseButtons = {};
    Jevaing::Vec2 CurrentMousePosition = {};
    Jevaing::Vec2 PreviousMousePosition = {};
    float CurrentMouseWheelDelta = 0.0f;

    std::size_t ToIndex(Jevaing::Key key)
    {
        return static_cast<std::size_t>(key);
    }

    bool IsValidKey(Jevaing::Key key)
    {
        return ToIndex(key) < KeyCount;
    }

    std::size_t ToIndex(Jevaing::MouseButton button)
    {
        return static_cast<std::size_t>(button);
    }

    bool IsValidMouseButton(Jevaing::MouseButton button)
    {
        return ToIndex(button) < MouseButtonCount;
    }
}

namespace Jevaing::Internal::InputState
{
    void BeginFrame()
    {
        PreviousKeys = CurrentKeys;
        PreviousMouseButtons = CurrentMouseButtons;
        PreviousMousePosition = CurrentMousePosition;
        CurrentMouseWheelDelta = 0.0f;
    }

    void SetKeyState(Key key, bool isDown)
    {
        if (!IsValidKey(key))
        {
            return;
        }

        CurrentKeys[ToIndex(key)] = isDown;
    }

    void SetMouseButtonState(MouseButton button, bool isDown)
    {
        if (!IsValidMouseButton(button))
        {
            return;
        }

        CurrentMouseButtons[ToIndex(button)] = isDown;
    }

    void SetMousePosition(float x, float y)
    {
        CurrentMousePosition = { x, y };
    }

    void AddMouseWheelDelta(float delta)
    {
        CurrentMouseWheelDelta += delta;
    }

    void Reset()
    {
        CurrentKeys.fill(false);
        PreviousKeys.fill(false);
        CurrentMouseButtons.fill(false);
        PreviousMouseButtons.fill(false);
        CurrentMousePosition = {};
        PreviousMousePosition = {};
        CurrentMouseWheelDelta = 0.0f;
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

    Vec2 GetMousePosition()
    {
        return CurrentMousePosition;
    }

    Vec2 GetMouseDelta()
    {
        return CurrentMousePosition - PreviousMousePosition;
    }

    float GetMouseWheelDelta()
    {
        return CurrentMouseWheelDelta;
    }

    bool IsMouseButtonDown(MouseButton button)
    {
        return IsValidMouseButton(button) && CurrentMouseButtons[ToIndex(button)];
    }

    bool IsMouseButtonPressed(MouseButton button)
    {
        if (!IsValidMouseButton(button))
        {
            return false;
        }

        const std::size_t index = ToIndex(button);
        return CurrentMouseButtons[index] && !PreviousMouseButtons[index];
    }

    bool IsMouseButtonReleased(MouseButton button)
    {
        if (!IsValidMouseButton(button))
        {
            return false;
        }

        const std::size_t index = ToIndex(button);
        return !CurrentMouseButtons[index] && PreviousMouseButtons[index];
    }
}
