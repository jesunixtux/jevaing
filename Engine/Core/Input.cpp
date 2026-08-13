#include <Jevaing/Input.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Xinput.h>
#endif

#include "InputState.h"

namespace
{
    constexpr std::size_t KeyCount =
        static_cast<std::size_t>(Jevaing::Key::Count);
    constexpr std::size_t MouseButtonCount =
        static_cast<std::size_t>(Jevaing::MouseButton::Count);
    constexpr std::size_t GamepadButtonCount =
        static_cast<std::size_t>(Jevaing::GamepadButton::Count);
    constexpr std::size_t MaxGamepads = 4;

    std::array<bool, KeyCount> CurrentKeys = {};
    std::array<bool, KeyCount> PreviousKeys = {};
    std::array<bool, MouseButtonCount> CurrentMouseButtons = {};
    std::array<bool, MouseButtonCount> PreviousMouseButtons = {};
    std::array<std::array<bool, GamepadButtonCount>, MaxGamepads> CurrentGamepadButtons = {};
    std::array<std::array<bool, GamepadButtonCount>, MaxGamepads> PreviousGamepadButtons = {};
    std::array<Jevaing::GamepadState, MaxGamepads> CurrentGamepads = {};
    std::array<Jevaing::GamepadState, MaxGamepads> PreviousGamepads = {};
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

    std::size_t ToIndex(Jevaing::GamepadButton button)
    {
        return static_cast<std::size_t>(button);
    }

    bool IsValidGamepadButton(Jevaing::GamepadButton button)
    {
        return ToIndex(button) < GamepadButtonCount;
    }

    bool IsValidGamepadIndex(int index)
    {
        return index >= 0 && static_cast<std::size_t>(index) < MaxGamepads;
    }

    float NormalizeThumb(short value, short deadzone)
    {
        const float raw = static_cast<float>(value);
        const float absValue = std::fabs(raw);

        if (absValue <= static_cast<float>(deadzone))
        {
            return 0.0f;
        }

        const float sign = raw < 0.0f ? -1.0f : 1.0f;
        const float normalized =
            (absValue - static_cast<float>(deadzone)) /
            (32767.0f - static_cast<float>(deadzone));

        return std::max(-1.0f, std::min(1.0f, normalized * sign));
    }

    float NormalizeTrigger(unsigned char value)
    {
#if defined(_WIN32)
        if (value <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        {
            return 0.0f;
        }

        return
            (static_cast<float>(value) - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) /
            (255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
#else
        (void)value;
        return 0.0f;
#endif
    }

    void SetGamepadButton(
        std::size_t index,
        Jevaing::GamepadButton button,
        bool isDown
    )
    {
        CurrentGamepadButtons[index][ToIndex(button)] = isDown;
    }

    void UpdateGamepads()
    {
        PreviousGamepads = CurrentGamepads;
        PreviousGamepadButtons = CurrentGamepadButtons;

        for (std::size_t index = 0; index < MaxGamepads; ++index)
        {
            CurrentGamepads[index] = {};
            CurrentGamepadButtons[index].fill(false);

#if defined(_WIN32)
            XINPUT_STATE state = {};
            const DWORD result =
                XInputGetState(static_cast<DWORD>(index), &state);

            if (result != ERROR_SUCCESS)
            {
                continue;
            }

            CurrentGamepads[index].Connected = true;
            CurrentGamepads[index].LeftStickX =
                NormalizeThumb(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            CurrentGamepads[index].LeftStickY =
                NormalizeThumb(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            CurrentGamepads[index].RightStickX =
                NormalizeThumb(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            CurrentGamepads[index].RightStickY =
                NormalizeThumb(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            CurrentGamepads[index].LeftTrigger =
                NormalizeTrigger(state.Gamepad.bLeftTrigger);
            CurrentGamepads[index].RightTrigger =
                NormalizeTrigger(state.Gamepad.bRightTrigger);

            const WORD buttons = state.Gamepad.wButtons;
            SetGamepadButton(index, Jevaing::GamepadButton::A, (buttons & XINPUT_GAMEPAD_A) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::B, (buttons & XINPUT_GAMEPAD_B) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::X, (buttons & XINPUT_GAMEPAD_X) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::Y, (buttons & XINPUT_GAMEPAD_Y) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::LeftShoulder, (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::RightShoulder, (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::View, (buttons & XINPUT_GAMEPAD_BACK) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::Menu, (buttons & XINPUT_GAMEPAD_START) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::DPadUp, (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::DPadDown, (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::DPadLeft, (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::DPadRight, (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::LeftStick, (buttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
            SetGamepadButton(index, Jevaing::GamepadButton::RightStick, (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
#endif
        }
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
        UpdateGamepads();
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
        for (auto& buttons : CurrentGamepadButtons)
        {
            buttons.fill(false);
        }
        for (auto& buttons : PreviousGamepadButtons)
        {
            buttons.fill(false);
        }
        CurrentGamepads.fill({});
        PreviousGamepads.fill({});
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

    bool IsGamepadConnected(int index)
    {
        return
            IsValidGamepadIndex(index) &&
            CurrentGamepads[static_cast<std::size_t>(index)].Connected;
    }

    bool IsGamepadButtonDown(int index, GamepadButton button)
    {
        return
            IsValidGamepadIndex(index) &&
            IsValidGamepadButton(button) &&
            CurrentGamepadButtons[static_cast<std::size_t>(index)][ToIndex(button)];
    }

    bool IsGamepadButtonPressed(int index, GamepadButton button)
    {
        if (!IsValidGamepadIndex(index) || !IsValidGamepadButton(button))
        {
            return false;
        }

        const std::size_t gamepad = static_cast<std::size_t>(index);
        const std::size_t buttonIndex = ToIndex(button);
        return
            CurrentGamepadButtons[gamepad][buttonIndex] &&
            !PreviousGamepadButtons[gamepad][buttonIndex];
    }

    bool IsGamepadButtonReleased(int index, GamepadButton button)
    {
        if (!IsValidGamepadIndex(index) || !IsValidGamepadButton(button))
        {
            return false;
        }

        const std::size_t gamepad = static_cast<std::size_t>(index);
        const std::size_t buttonIndex = ToIndex(button);
        return
            !CurrentGamepadButtons[gamepad][buttonIndex] &&
            PreviousGamepadButtons[gamepad][buttonIndex];
    }

    GamepadState GetGamepadState(int index)
    {
        if (!IsValidGamepadIndex(index))
        {
            return {};
        }

        return CurrentGamepads[static_cast<std::size_t>(index)];
    }
}
