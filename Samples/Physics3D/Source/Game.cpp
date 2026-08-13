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
            const fs::path local = current / "jevaing.project";
            if (fs::exists(local))
            {
                return local.string();
            }

            const fs::path repo = current / "Samples" / "Physics3D" / "jevaing.project";
            if (fs::exists(repo))
            {
                return repo.string();
            }

            if (!current.has_parent_path())
            {
                break;
            }

            current = current.parent_path();
        }

        return "jevaing.project";
    }

    class Physics3DGame final : public Jevaing::Game
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

            if (!Jevaing::Scene::LoadFromFile(scenePath, ".", m_scene, error))
            {
                std::cerr << error << std::endl;
                m_failed = true;
                return;
            }

            m_actions.Bind("Impulse", Jevaing::Key::Space);
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

            m_scene.Update(deltaTime);

            Jevaing::SceneEntity* cube = m_scene.FindEntityByName("Cube");

            if (cube && m_actions.IsPressed("Impulse"))
            {
                m_scene.Physics3D().ApplyImpulse(cube->Id, { 0.0f, 5.5f, -1.0f });
            }

            m_lastRayHit =
                m_scene.Physics3D().Raycast({ 0.0f, 8.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, 20.0f);
        }

        void OnRender(Jevaing::Graphics2D& graphics) override
        {
            graphics.Clear({ 0.025f, 0.035f, 0.050f, 1.0f });
        }

        void OnRender(Jevaing::Graphics3D& graphics) override
        {
            m_scene.Render(graphics);

            for (const Jevaing::SceneEntity& entity : m_scene.GetEntities())
            {
                if (entity.BoxCollider3D)
                {
                    Jevaing::Transform transform = entity.Transform.LocalTransform;
                    transform.Scale = {
                        transform.Scale.X * entity.BoxCollider3D->Size.X,
                        transform.Scale.Y * entity.BoxCollider3D->Size.Y,
                        transform.Scale.Z * entity.BoxCollider3D->Size.Z
                    };

                    const Jevaing::Color color =
                        entity.BoxCollider3D->IsTrigger
                            ? Jevaing::Color{ 0.35f, 0.85f, 0.95f, 0.35f }
                            : (entity.RigidBody3D ? Jevaing::Color{ 0.94f, 0.48f, 0.22f, 1.0f } : Jevaing::Color{ 0.28f, 0.42f, 0.32f, 1.0f });

                    graphics.DrawCube(transform, color);
                }

                if (entity.SphereCollider3D)
                {
                    Jevaing::Transform transform = entity.Transform.LocalTransform;
                    transform.Scale = {
                        entity.SphereCollider3D->Radius * 2.0f,
                        entity.SphereCollider3D->Radius * 2.0f,
                        entity.SphereCollider3D->Radius * 2.0f
                    };
                    graphics.DrawCube(transform, { 0.38f, 0.72f, 1.0f, 1.0f });
                }
            }

            if (m_lastRayHit.Hit)
            {
                Jevaing::Transform marker;
                marker.Position = m_lastRayHit.Point;
                marker.Scale = { 0.15f, 0.15f, 0.15f };
                graphics.DrawCube(marker, { 1.0f, 0.96f, 0.35f, 1.0f });
            }
        }

    private:
        Jevaing::ProjectConfig m_project;
        Jevaing::Scene m_scene;
        Jevaing::InputMap m_actions;
        Jevaing::RaycastHit3D m_lastRayHit;
        bool m_failed = false;
    };
}

int main(int argc, char** argv)
{
    Physics3DGame game;

    Jevaing::GameConfig config;
    config.Title = "Physics3D - Jevaing 0.0.10";
    config.Width = 1280;
    config.Height = 720;

    return Jevaing::Run(game, config, argc, argv);
}
