#include <algorithm>

#include <Jevaing/Jevaing.h>

namespace
{
    class BigBearGummyDemo final : public Jevaing::Game
    {
    public:
        void OnUpdate(double deltaTime) override
        {
            constexpr float Speed = 0.90f;
            const float movement = Speed * static_cast<float>(deltaTime);

            if (
                Jevaing::Input::IsKeyDown(Jevaing::Key::A) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Left)
            )
            {
                m_position.X -= movement;
            }

            if (
                Jevaing::Input::IsKeyDown(Jevaing::Key::D) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Right)
            )
            {
                m_position.X += movement;
            }

            if (
                Jevaing::Input::IsKeyDown(Jevaing::Key::W) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Up)
            )
            {
                m_position.Y += movement;
            }

            if (
                Jevaing::Input::IsKeyDown(Jevaing::Key::S) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Down)
            )
            {
                m_position.Y -= movement;
            }

            m_position.X = std::clamp(m_position.X, -0.78f, 0.78f);
            m_position.Y = std::clamp(m_position.Y, -0.52f, 0.48f);

            m_spaceHeld = Jevaing::Input::IsKeyDown(Jevaing::Key::Space);
        }

        void OnRender(Jevaing::Graphics2D& graphics) override
        {
            graphics.Clear({ 0.025f, 0.035f, 0.060f, 1.0f });

            graphics.DrawQuad(
                { 0.0f, -0.91f },
                { 2.0f, 0.18f },
                { 0.10f, 0.16f, 0.22f, 1.0f }
            );

            const Jevaing::Color gummy = m_spaceHeld
                ? Jevaing::Color{ 0.28f, 0.92f, 0.48f, 1.0f }
                : Jevaing::Color{ 0.96f, 0.38f, 0.18f, 1.0f };

            const Jevaing::Color gummyLight = m_spaceHeld
                ? Jevaing::Color{ 0.55f, 1.0f, 0.68f, 1.0f }
                : Jevaing::Color{ 1.0f, 0.63f, 0.30f, 1.0f };

            const Jevaing::Color dark = { 0.07f, 0.05f, 0.08f, 1.0f };

            const float x = m_position.X;
            const float y = m_position.Y;

            // Shadow.
            graphics.DrawQuad(
                { x, y - 0.30f },
                { 0.38f, 0.06f },
                { 0.015f, 0.020f, 0.030f, 1.0f }
            );

            // Legs and arms.
            graphics.DrawQuad({ x - 0.10f, y - 0.22f }, { 0.11f, 0.18f }, gummy);
            graphics.DrawQuad({ x + 0.10f, y - 0.22f }, { 0.11f, 0.18f }, gummy);
            graphics.DrawQuad({ x - 0.18f, y - 0.01f }, { 0.10f, 0.22f }, gummy);
            graphics.DrawQuad({ x + 0.18f, y - 0.01f }, { 0.10f, 0.22f }, gummy);

            // Body and head.
            graphics.DrawQuad({ x, y - 0.03f }, { 0.31f, 0.38f }, gummy);
            graphics.DrawQuad({ x, y + 0.25f }, { 0.29f, 0.25f }, gummy);

            // Ears.
            graphics.DrawQuad({ x - 0.105f, y + 0.39f }, { 0.095f, 0.095f }, gummy);
            graphics.DrawQuad({ x + 0.105f, y + 0.39f }, { 0.095f, 0.095f }, gummy);

            // Muzzle and face.
            graphics.DrawQuad({ x, y + 0.205f }, { 0.15f, 0.09f }, gummyLight);
            graphics.DrawQuad({ x - 0.060f, y + 0.285f }, { 0.035f, 0.045f }, dark);
            graphics.DrawQuad({ x + 0.060f, y + 0.285f }, { 0.035f, 0.045f }, dark);
            graphics.DrawQuad({ x, y + 0.225f }, { 0.045f, 0.035f }, dark);
        }

    private:
        Jevaing::Vec2 m_position = { 0.0f, -0.18f };
        bool m_spaceHeld = false;
    };
}

int main(int argc, char** argv)
{
    BigBearGummyDemo game;

    Jevaing::GameConfig config;
    config.Title = "Jevaing 0.0.6 - BIG BEAR GUMMY Sandbox";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
