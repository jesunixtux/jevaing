#include <Jevaing/Jevaing.h>

class XboxSmokeGame final : public Jevaing::Game
{
public:
    void OnStart() override
    {
        m_cube.Transform.LocalTransform.Position = { 0.0f, 0.0f, 0.0f };
        m_cube.Transform.LocalTransform.Scale = { 1.2f, 1.2f, 1.2f };
    }

    void OnUpdate(double deltaTime) override
    {
        const Jevaing::GamepadState pad = Jevaing::Input::GetGamepadState(0);
        const float dt = static_cast<float>(deltaTime);

        if (pad.Connected)
        {
            m_cube.Transform.LocalTransform.Rotation.Y += pad.LeftStickX * dt * 2.5f;
            m_cube.Transform.LocalTransform.Rotation.X += pad.LeftStickY * dt * 2.5f;

            if (Jevaing::Input::IsGamepadButtonPressed(0, Jevaing::GamepadButton::A))
            {
                m_blue = !m_blue;
            }
        }
        else
        {
            m_cube.Transform.LocalTransform.Rotation.Y += dt;
        }
    }

    void OnRender(Jevaing::Graphics2D& graphics) override
    {
        graphics.DrawTriangle(
            { 120.0f, 120.0f },
            { 260.0f, 120.0f },
            { 190.0f, 40.0f },
            m_blue ? Jevaing::Color{ 0.25f, 0.55f, 1.0f, 1.0f } : Jevaing::Color{ 0.95f, 0.35f, 0.25f, 1.0f }
        );
    }

    void OnRender(Jevaing::Graphics3D& graphics) override
    {
        graphics.DrawCube(
            m_cube.Transform.LocalTransform,
            m_blue ? Jevaing::Color{ 0.25f, 0.55f, 1.0f, 1.0f } : Jevaing::Color{ 0.95f, 0.35f, 0.25f, 1.0f }
        );
    }

private:
    Jevaing::SceneEntity m_cube;
    bool m_blue = false;
};

int main(int argc, char** argv)
{
    XboxSmokeGame game;

    Jevaing::GameConfig config;
    config.Title = "Jevaing Xbox Smoke Test";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
