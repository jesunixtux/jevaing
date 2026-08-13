#include <algorithm>

#include <Jevaing/Jevaing.h>

namespace
{
    class BigBearGummyDemo final : public Jevaing::Game
    {
    public:
        void OnResize(int width, int height) override
        {
            if (width > 0 && height > 0)
            {
                m_camera.AspectRatio =
                    static_cast<float>(width) /
                    static_cast<float>(height);
            }
        }

        void OnUpdate(double deltaTime) override
        {
            constexpr float MoveSpeed = 1.35f;
            constexpr float RotateSpeed = 1.75f;
            const float movement = MoveSpeed * static_cast<float>(deltaTime);
            const float rotation = RotateSpeed * static_cast<float>(deltaTime);

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::A))
            {
                m_transform.Position.X -= movement;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::D))
            {
                m_transform.Position.X += movement;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::W))
            {
                m_transform.Position.Z += movement;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::S))
            {
                m_transform.Position.Z -= movement;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::Left))
            {
                m_transform.Rotation.Y -= rotation;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::Right))
            {
                m_transform.Rotation.Y += rotation;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::Up))
            {
                m_transform.Rotation.X -= rotation;
            }

            if (Jevaing::Input::IsKeyDown(Jevaing::Key::Down))
            {
                m_transform.Rotation.X += rotation;
            }

            m_transform.Position.X = std::clamp(m_transform.Position.X, -1.45f, 1.45f);
            m_transform.Position.Z = std::clamp(m_transform.Position.Z, -0.80f, 1.80f);

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

            graphics.DrawQuad(
                { 0.0f, -0.03f },
                { 0.018f, 0.018f },
                { 0.90f, 0.94f, 1.0f, 1.0f }
            );
        }

        void OnRender(Jevaing::Graphics3D& graphics) override
        {
            graphics.SetCamera(m_camera);

            const Jevaing::Color cube = m_spaceHeld
                ? Jevaing::Color{ 0.30f, 0.95f, 0.54f, 1.0f }
                : Jevaing::Color{ 0.25f, 0.65f, 1.0f, 1.0f };

            graphics.DrawCube(m_transform, cube);
        }

    private:
        Jevaing::PerspectiveCamera m_camera = {
            { 0.0f, 1.15f, -4.8f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            Jevaing::Pi / 3.0f,
            16.0f / 9.0f,
            0.1f,
            100.0f
        };

        Jevaing::Transform m_transform = {
            { 0.0f, 0.0f, 0.0f },
            { 0.35f, 0.65f, 0.0f },
            { 1.20f, 1.20f, 1.20f }
        };

        bool m_spaceHeld = false;
    };
}

int main(int argc, char** argv)
{
    BigBearGummyDemo game;

    Jevaing::GameConfig config;
    config.Title = "Jevaing 0.0.7 - BIG BEAR GUMMY Sandbox";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
