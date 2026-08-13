#include <Jevaing/Jevaing.h>
#include "GameCode.h"

class Game final : public Jevaing::Game
{
public:
    void OnStart() override
    {
        m_scene.Load("Scenes/main.scene", "Assets", m_error);
    }

    void OnUpdate(double deltaTime) override
    {
        jevaing_game_code_tick(static_cast<float>(deltaTime));
        m_scene.Update(deltaTime);
    }

    void OnRender(Jevaing::Graphics3D& graphics) override
    {
        m_scene.Render(graphics);
    }

private:
    Jevaing::Scene m_scene{"MyGame"};
    std::string m_error;
};

int main(int argc, char** argv)
{
    Game game;
    Jevaing::GameConfig config;
    config.Title = "MyGame";
    config.Width = 1280;
    config.Height = 720;
    return Jevaing::Run(game, config, argc, argv);
}
