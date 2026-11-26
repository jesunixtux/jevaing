#include <Jevaing/Jevaing.h>

#include "PixelText.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

namespace
{
    struct FallingItem
    {
        float X = 0.0f;
        float Y = 0.0f;
        bool Reward = false;
        bool Active = false;
    };

    struct GameState
    {
        enum class Phase { Ready, Playing, Paused, Won, Lost };
        static constexpr int Goal = 8;

        Phase CurrentPhase = Phase::Ready;
        float PlayerX = 0.0f;
        float SpawnTimer = 0.0f;
        float Elapsed = 0.0f;
        float HitFlash = 0.0f;
        unsigned SpawnCount = 0;
        int Score = 0;
        int Lives = 3;
        std::array<FallingItem, 24> Items{};

        void Reset()
        {
            *this = {};
            Lives = 3;
        }

        void Start() { CurrentPhase = Phase::Playing; }

        void TogglePause()
        {
            if (CurrentPhase == Phase::Playing) CurrentPhase = Phase::Paused;
            else if (CurrentPhase == Phase::Paused) CurrentPhase = Phase::Playing;
        }

        static bool Overlaps(float ax, float ay, float aw, float ah,
            float bx, float by, float bw, float bh)
        {
            return std::abs(ax - bx) * 2.0f < aw + bw &&
                std::abs(ay - by) * 2.0f < ah + bh;
        }

        void Step(float deltaTime, int direction)
        {
            if (CurrentPhase != Phase::Playing) return;
            const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
            Elapsed += dt;
            HitFlash = std::max(0.0f, HitFlash - dt);
            PlayerX = std::clamp(PlayerX + direction * dt * 1.6f, -0.88f, 0.88f);
            SpawnTimer += dt;
            if (SpawnTimer >= 0.65f)
            {
                SpawnTimer -= 0.65f;
                for (FallingItem& item : Items)
                {
                    if (!item.Active)
                    {
                        constexpr float lanes[] = { -0.78f, -0.46f, -0.15f,
                            0.15f, 0.46f, 0.78f };
                        item = { lanes[(SpawnCount * 5 + SpawnCount / 6) % 6], 1.12f,
                            SpawnCount % 3 == 0, true };
                        ++SpawnCount;
                        break;
                    }
                }
            }
            for (FallingItem& item : Items)
            {
                if (CurrentPhase != Phase::Playing) break;
                if (!item.Active) continue;
                item.Y -= dt * (0.48f + std::min(Score, 20) * 0.02f);
                if (Overlaps(PlayerX, -0.76f, 0.17f, 0.13f,
                    item.X, item.Y, 0.12f, 0.12f))
                {
                    if (item.Reward)
                    {
                        ++Score;
                        if (Score >= Goal) CurrentPhase = Phase::Won;
                    }
                    else
                    {
                        --Lives;
                        HitFlash = 0.25f;
                        if (Lives <= 0) CurrentPhase = Phase::Lost;
                    }
                    item.Active = false;
                }
                else if (item.Y < -1.12f)
                {
                    item.Active = false;
                }
            }
        }
    };

    bool TestLogic()
    {
        GameState state;
        state.Reset();
        state.Step(0.05f, 1);
        if (state.PlayerX != 0.0f || state.CurrentPhase != GameState::Phase::Ready) return false;
        state.Start();
        state.Step(0.05f, 1);
        if (state.PlayerX <= 0.0f || state.Lives != 3) return false;
        if (!GameState::Overlaps(0, 0, 1, 1, 0.4f, 0, 1, 1) ||
            GameState::Overlaps(0, 0, 1, 1, 2, 0, 1, 1)) return false;
        state.Items[0] = { state.PlayerX, -0.76f, true, true };
        state.Step(0.0f, 0);
        if (state.Score != 1 || state.Items[0].Active) return false;
        state.Items[0] = { state.PlayerX, -0.76f, false, true };
        state.Step(0.0f, 0);
        if (state.Lives != 2) return false;
        state.TogglePause();
        const float pausedX = state.PlayerX;
        state.Step(0.05f, -1);
        if (state.PlayerX != pausedX || state.CurrentPhase != GameState::Phase::Paused) return false;
        state.TogglePause();
        state.Score = GameState::Goal - 1;
        state.Items[0] = { state.PlayerX, -0.76f, true, true };
        state.Items[1] = { state.PlayerX, -0.76f, false, true };
        state.Step(0.0f, 0);
        if (state.CurrentPhase != GameState::Phase::Won || state.Lives != 2) return false;
        state.Reset();
        state.Start();
        state.Lives = 1;
        state.Items[0] = { state.PlayerX, -0.76f, false, true };
        state.Step(0.0f, 0);
        return state.CurrentPhase == GameState::Phase::Lost && state.Lives == 0;
    }

    class SkyDodgeGame final : public Jevaing::Game
    {
    public:
        explicit SkyDodgeGame(std::filesystem::path executableDirectory)
            : m_executableDirectory(std::move(executableDirectory)) {}

        bool Failed() const { return m_failed; }

        void OnStart() override
        {
            std::string error;
            const std::string projectFile =
                (m_executableDirectory / "jevaing.project").string();
            if (!Jevaing::Project::Load(projectFile, m_project, error))
            {
                std::cerr << error << '\n';
                m_failed = true;
                return;
            }
            const std::string sceneFile =
                Jevaing::Project::ResolveStartupScenePath(m_project);
            const std::string assetRoot =
                Jevaing::Project::ResolvePath(m_project, m_project.AssetRoot);
            if (!m_scene.Load(sceneFile, assetRoot, error))
            {
                std::cerr << error << '\n';
                m_failed = true;
                return;
            }
            m_state.Reset();
        }

        void OnUpdate(double deltaTime) override
        {
            if (m_failed) return;
            if (Jevaing::Input::IsKeyPressed(Jevaing::Key::Enter) &&
                (m_state.CurrentPhase == GameState::Phase::Ready ||
                    m_state.CurrentPhase == GameState::Phase::Won ||
                    m_state.CurrentPhase == GameState::Phase::Lost))
            {
                m_state.Reset();
                m_state.Start();
            }
            if (Jevaing::Input::IsKeyPressed(Jevaing::Key::Space))
                m_state.TogglePause();
            const bool left = Jevaing::Input::IsKeyDown(Jevaing::Key::A) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Left) ||
                Jevaing::Input::IsKeyPressed(Jevaing::Key::A) ||
                Jevaing::Input::IsKeyPressed(Jevaing::Key::Left);
            const bool right = Jevaing::Input::IsKeyDown(Jevaing::Key::D) ||
                Jevaing::Input::IsKeyDown(Jevaing::Key::Right) ||
                Jevaing::Input::IsKeyPressed(Jevaing::Key::D) ||
                Jevaing::Input::IsKeyPressed(Jevaing::Key::Right);
            m_state.Step(static_cast<float>(deltaTime), static_cast<int>(right) - static_cast<int>(left));
        }

        void OnRender(Jevaing::Graphics2D& graphics) override
        {
            graphics.Clear({ 0.035f, 0.07f, 0.14f, 1.0f });
            for (int row = 0; row < 20; ++row)
            {
                const float t = static_cast<float>(row) / 19.0f;
                graphics.DrawQuad({ 0.0f, -0.95f + row * 0.10f },
                    { 2.0f, 0.105f }, { 0.035f + 0.11f * t,
                        0.07f + 0.20f * t, 0.14f + 0.31f * t, 1.0f });
            }
            constexpr Jevaing::Vec2 stars[] = {
                {-0.87f, 0.65f}, {-0.57f, 0.36f}, {-0.26f, 0.72f},
                { 0.13f, 0.48f}, { 0.49f, 0.76f}, { 0.83f, 0.39f},
                {-0.69f,-0.05f}, {-0.13f, 0.07f}, { 0.66f,-0.12f}
            };
            for (const Jevaing::Vec2& star : stars)
                graphics.DrawQuad(star, { 0.012f, 0.012f },
                    { 0.55f, 0.76f, 0.96f, 1.0f });
            graphics.DrawQuad({ 0.0f, -0.90f }, { 2.0f, 0.04f },
                { 0.25f, 0.85f, 0.9f, 1.0f });
            for (const FallingItem& item : m_state.Items)
            {
                if (item.Active)
                {
                    graphics.DrawQuad({ item.X, item.Y }, { 0.12f, 0.12f },
                        item.Reward ? Jevaing::Color{ 1.0f, 0.83f, 0.2f, 1.0f } :
                            Jevaing::Color{ 0.95f, 0.24f, 0.22f, 1.0f });
                    graphics.DrawQuad({ item.X, item.Y }, { 0.055f, 0.055f },
                        item.Reward ? Jevaing::Color{ 1.0f, 0.98f, 0.63f, 1.0f } :
                            Jevaing::Color{ 0.52f, 0.10f, 0.15f, 1.0f });
                }
            }
            graphics.DrawQuad({ m_state.PlayerX, -0.76f }, { 0.17f, 0.13f },
                m_state.HitFlash > 0.0f ? Jevaing::Color{ 1.0f, 0.85f, 0.85f, 1.0f } :
                    Jevaing::Color{ 0.28f, 0.92f, 0.95f, 1.0f });
            graphics.DrawTriangle({ m_state.PlayerX, -0.64f },
                { m_state.PlayerX - 0.07f, -0.71f },
                { m_state.PlayerX + 0.07f, -0.71f },
                { 0.65f, 1.0f, 1.0f, 1.0f });
            SkyDodge::DrawText(graphics, "LIVES", -0.96f, 0.97f, 0.007f,
                { 0.88f, 0.94f, 1.0f, 1.0f });
            for (int i = 0; i < 3; ++i)
                graphics.DrawQuad({ -0.85f + i * 0.10f, 0.84f }, { 0.07f, 0.07f },
                    i < m_state.Lives ? Jevaing::Color{ 0.95f, 0.3f, 0.38f, 1.0f } :
                        Jevaing::Color{ 0.25f, 0.31f, 0.4f, 1.0f });
            const std::string scoreText = "SCORE " + std::to_string(m_state.Score) +
                "/" + std::to_string(GameState::Goal);
            SkyDodge::DrawText(graphics, scoreText, 0.14f, 0.97f, 0.007f,
                { 1.0f, 0.91f, 0.56f, 1.0f });
            graphics.DrawQuad({ 0.50f, 0.84f }, { 0.7f, 0.055f },
                { 0.2f, 0.28f, 0.36f, 1.0f });
            if (m_state.Score > 0)
                graphics.DrawQuad({ 0.15f + std::min(m_state.Score, GameState::Goal) *
                    (0.35f / GameState::Goal), 0.84f },
                    { std::min(m_state.Score, GameState::Goal) *
                        (0.7f / GameState::Goal), 0.055f },
                    { 1.0f, 0.83f, 0.2f, 1.0f });

            if (m_state.CurrentPhase != GameState::Phase::Playing)
            {
                graphics.DrawQuad({ 0.0f, 0.0f }, { 1.55f, 0.90f },
                    { 0.04f, 0.10f, 0.20f, 1.0f });
                graphics.DrawQuad({ 0.0f, 0.44f }, { 1.55f, 0.025f },
                    { 0.27f, 0.86f, 0.96f, 1.0f });
                const Jevaing::Color bright = { 0.9f, 0.98f, 1.0f, 1.0f };
                const Jevaing::Color soft = { 0.55f, 0.77f, 0.92f, 1.0f };
                if (m_state.CurrentPhase == GameState::Phase::Ready)
                {
                    SkyDodge::DrawCenteredText(graphics, "SKY DODGE", 0.0f, 0.30f, 0.017f, bright);
                    SkyDodge::DrawCenteredText(graphics, "COLLECT 8 GOLD", 0.0f, 0.02f, 0.010f, soft);
                    SkyDodge::DrawCenteredText(graphics, "AVOID RED", 0.0f, -0.10f, 0.010f, soft);
                    SkyDodge::DrawCenteredText(graphics, "A D OR ARROWS MOVE", 0.0f, -0.23f, 0.008f, bright);
                    SkyDodge::DrawCenteredText(graphics, "ENTER TO START", 0.0f, -0.37f, 0.009f, bright);
                }
                else if (m_state.CurrentPhase == GameState::Phase::Paused)
                {
                    SkyDodge::DrawCenteredText(graphics, "PAUSED", 0.0f, 0.22f, 0.018f, bright);
                    SkyDodge::DrawCenteredText(graphics, "SPACE TO RESUME", 0.0f, -0.18f, 0.010f, soft);
                }
                else
                {
                    const bool won = m_state.CurrentPhase == GameState::Phase::Won;
                    SkyDodge::DrawCenteredText(graphics, won ? "YOU WIN" : "GAME OVER",
                        0.0f, 0.22f, 0.018f, bright);
                    SkyDodge::DrawCenteredText(graphics, "ENTER TO PLAY AGAIN",
                        0.0f, -0.18f, 0.009f, soft);
                }
            }
            else
            {
                SkyDodge::DrawCenteredText(graphics, "SPACE PAUSE", 0.0f, -0.94f, 0.006f,
                    { 0.81f, 0.92f, 1.0f, 1.0f });
            }
        }

    private:
        std::filesystem::path m_executableDirectory;
        Jevaing::ProjectConfig m_project;
        Jevaing::Scene m_scene;
        GameState m_state;
        bool m_failed = false;
    };
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--logic-test")
    {
        const bool passed = TestLogic();
        std::cout << (passed ? "[PASS] SkyDodge2D logic\n" : "[FAIL] SkyDodge2D logic\n");
        return passed ? 0 : 2;
    }
    if (argc < 1 || !argv[0]) return 2;
    SkyDodgeGame game(std::filesystem::absolute(argv[0]).parent_path());
    Jevaing::GameConfig config;
    config.Title = "SkyDodge2D | Jevaing";
    config.Width = 960;
    config.Height = 640;
    const int result = Jevaing::Run(game, config, argc, argv);
    return game.Failed() ? 2 : result;
}
