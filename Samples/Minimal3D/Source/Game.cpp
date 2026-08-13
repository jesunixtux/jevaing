#include <Jevaing/Jevaing.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    std::string FindProjectFile()
    {
        namespace fs = std::filesystem;

        fs::path current = fs::current_path();

        for (int depth = 0; depth < 6; ++depth)
        {
            const fs::path candidate = current / "jevaing.project";

            if (fs::exists(candidate))
            {
                return candidate.string();
            }

            const fs::path sampleCandidate =
                current / "Samples" / "Minimal3D" / "jevaing.project";

            if (fs::exists(sampleCandidate))
            {
                return sampleCandidate.string();
            }

            if (!current.has_parent_path())
            {
                break;
            }

            current = current.parent_path();
        }

        return "jevaing.project";
    }

    class Minimal3DGame final : public Jevaing::Game
    {
    public:
        void OnStart() override
        {
            std::string error;
            const std::string projectPath = FindProjectFile();

            if (!Jevaing::Project::Load(projectPath, m_project, error))
            {
                std::cerr << error << std::endl;
                m_failed = true;
                return;
            }

            const std::string scenePath =
                Jevaing::Project::ResolveStartupScenePath(m_project);
            const std::string assetRoot =
                Jevaing::Project::ResolvePath(m_project, m_project.AssetRoot);

            if (!Jevaing::Scene::LoadFromFile(scenePath, assetRoot, m_scene, error))
            {
                std::cerr << error << std::endl;
                m_failed = true;
                return;
            }

            m_actions.Bind("Forward", Jevaing::Key::W);
            m_actions.Bind("Backward", Jevaing::Key::S);
            m_actions.Bind("Left", Jevaing::Key::A);
            m_actions.Bind("Right", Jevaing::Key::D);
            m_actions.Bind("Boost", Jevaing::MouseButton::Left);
        }

        void OnResize(int width, int height) override
        {
            Jevaing::SceneEntity* camera = m_scene.FindEntityByName("Camera");

            if (camera && camera->Camera && height > 0)
            {
                camera->Camera->Camera.AspectRatio =
                    static_cast<float>(width) /
                    static_cast<float>(height);
            }
        }

        void OnUpdate(double deltaTime) override
        {
            if (m_failed)
            {
                return;
            }

            constexpr float MoveSpeed = 1.4f;
            const float movement =
                MoveSpeed *
                static_cast<float>(deltaTime) *
                (m_actions.IsDown("Boost") ? 2.0f : 1.0f);

            Jevaing::SceneEntity* camera = m_scene.FindEntityByName("Camera");

            if (camera)
            {
                if (m_actions.IsDown("Forward"))
                {
                    camera->Transform.LocalTransform.Position.Z += movement;
                }

                if (m_actions.IsDown("Backward"))
                {
                    camera->Transform.LocalTransform.Position.Z -= movement;
                }

                if (m_actions.IsDown("Left"))
                {
                    camera->Transform.LocalTransform.Position.X -= movement;
                }

                if (m_actions.IsDown("Right"))
                {
                    camera->Transform.LocalTransform.Position.X += movement;
                }

                const Jevaing::Vec2 mouseDelta = Jevaing::Input::GetMouseDelta();
                camera->Camera->Camera.Target.X += mouseDelta.X * 0.002f;
                camera->Camera->Camera.Target.Y -= mouseDelta.Y * 0.002f;
            }

            Jevaing::SceneEntity* tux = m_scene.FindEntityByName("Tux");

            if (tux)
            {
                tux->Transform.LocalTransform.Rotation.Y +=
                    static_cast<float>(deltaTime) * 0.9f;
            }

            Jevaing::SceneEntity* parent = m_scene.FindEntityByName("Parent");

            if (parent)
            {
                parent->Transform.LocalTransform.Rotation.Y +=
                    static_cast<float>(deltaTime) * 1.2f;
            }

            m_cubeRotation += static_cast<float>(deltaTime);
            m_scene.Update(deltaTime);
        }

        void OnRender(Jevaing::Graphics2D& graphics) override
        {
            graphics.Clear({ 0.02f, 0.03f, 0.05f, 1.0f });
            m_scene.Render(graphics);
        }

        void OnRender(Jevaing::Graphics3D& graphics) override
        {
            m_scene.Render(graphics);

            Jevaing::Transform cube;
            cube.Position = { -1.8f, 0.0f, 0.0f };
            cube.Rotation = { m_cubeRotation * 0.5f, m_cubeRotation, 0.0f };
            graphics.DrawCube(cube, { 0.25f, 0.72f, 1.0f, 1.0f });

            const Jevaing::Transform childCube =
                m_scene.GetWorldTransform(
                    m_scene.FindEntityByName("ChildCube")
                        ? m_scene.FindEntityByName("ChildCube")->Id
                        : Jevaing::InvalidEntityId
                );

            graphics.DrawCube(childCube, { 0.95f, 0.48f, 0.22f, 1.0f });
        }

    private:
        Jevaing::ProjectConfig m_project;
        Jevaing::Scene m_scene;
        Jevaing::InputMap m_actions;
        float m_cubeRotation = 0.0f;
        bool m_failed = false;
    };
}

int main(int argc, char** argv)
{
    Minimal3DGame game;

    Jevaing::GameConfig config;
    config.Title = "Minimal3D - Jevaing 0.0.10";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
