#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Types.h"

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

    enum class MouseButton
    {
        Left,
        Right,
        Middle,
        Count
    };

    namespace Input
    {
        bool IsKeyDown(Key key);
        bool IsKeyPressed(Key key);
        bool IsKeyReleased(Key key);

        Vec2 GetMousePosition();
        Vec2 GetMouseDelta();
        float GetMouseWheelDelta();
        bool IsMouseButtonDown(MouseButton button);
        bool IsMouseButtonPressed(MouseButton button);
        bool IsMouseButtonReleased(MouseButton button);
    }

    class InputMap
    {
    public:
        void Bind(const std::string& action, Key key)
        {
            m_keyBindings[action].push_back(key);
        }

        void Bind(const std::string& action, MouseButton button)
        {
            m_mouseBindings[action].push_back(button);
        }

        bool IsDown(const std::string& action) const
        {
            const auto keyIt = m_keyBindings.find(action);

            if (keyIt != m_keyBindings.end())
            {
                for (Key key : keyIt->second)
                {
                    if (Input::IsKeyDown(key))
                    {
                        return true;
                    }
                }
            }

            const auto mouseIt = m_mouseBindings.find(action);

            if (mouseIt != m_mouseBindings.end())
            {
                for (MouseButton button : mouseIt->second)
                {
                    if (Input::IsMouseButtonDown(button))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        bool IsPressed(const std::string& action) const
        {
            const auto keyIt = m_keyBindings.find(action);

            if (keyIt != m_keyBindings.end())
            {
                for (Key key : keyIt->second)
                {
                    if (Input::IsKeyPressed(key))
                    {
                        return true;
                    }
                }
            }

            const auto mouseIt = m_mouseBindings.find(action);

            if (mouseIt != m_mouseBindings.end())
            {
                for (MouseButton button : mouseIt->second)
                {
                    if (Input::IsMouseButtonPressed(button))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        bool IsReleased(const std::string& action) const
        {
            const auto keyIt = m_keyBindings.find(action);

            if (keyIt != m_keyBindings.end())
            {
                for (Key key : keyIt->second)
                {
                    if (Input::IsKeyReleased(key))
                    {
                        return true;
                    }
                }
            }

            const auto mouseIt = m_mouseBindings.find(action);

            if (mouseIt != m_mouseBindings.end())
            {
                for (MouseButton button : mouseIt->second)
                {
                    if (Input::IsMouseButtonReleased(button))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

    private:
        std::unordered_map<std::string, std::vector<Key>> m_keyBindings;
        std::unordered_map<std::string, std::vector<MouseButton>> m_mouseBindings;
    };
}
