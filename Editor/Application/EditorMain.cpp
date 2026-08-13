#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>

#include <Jevaing/Jevaing.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "misc/cpp/imgui_stdlib.h"
#include "../../Engine/Build/BuildSystem.h"
#include "../../Engine/Core/Logger.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
);

namespace
{
    struct ConsoleEntry
    {
        Jevaing::Internal::LogLevel Level = Jevaing::Internal::LogLevel::Info;
        std::string Message;
    };

    struct EditorState
    {
        bool HasProject = false;
        bool Playing = false;
        bool Paused = false;
        bool StepRequested = false;
        bool DirtyScene = false;
        bool ShowInfo = true;
        bool ShowProjectBrowser = true;
        bool ShowHierarchy = true;
        bool ShowScene = true;
        bool ShowGame = true;
        bool ShowInspector = true;
        bool ShowProject = true;
        bool ShowConsole = true;
        bool ShowBuildSettings = true;
        bool RequestUnsavedPopup = false;
        std::string NewProjectName = "MyGame";
        std::string NewProjectLocation;
        std::string OpenProjectPath;
        std::string CurrentScenePath;
        std::string PlaySnapshotPath;
        std::string PendingPath;
        std::string ProjectSelectedDirectory;
        std::string NewAssetName = "NewScript.cpp";
        Jevaing::ProjectConfig Project;
        Jevaing::Scene EditScene{"Untitled"};
        Jevaing::Scene PlayScene{"Play"};
        Jevaing::EntityId SelectedEntity = Jevaing::InvalidEntityId;
        Jevaing::Internal::BuildSettings BuildSettings;
        std::vector<ConsoleEntry> Console;
        bool ShowInfoLogs = true;
        bool ShowWarningLogs = true;
        bool ShowErrorLogs = true;
    };

    enum class PendingEditorAction
    {
        None,
        Exit,
        CloseProject,
        LoadProject,
        LoadScene
    };

    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    IDXGISwapChain* g_swapChain = nullptr;
    ID3D11RenderTargetView* g_renderTargetView = nullptr;
    HWND g_hwnd = nullptr;
    EditorState g_editor;
    PendingEditorAction g_pendingAction = PendingEditorAction::None;

    void RequestPendingAction(PendingEditorAction action, const std::string& path = {});
    void StopPlayMode();

    void AddConsoleLog(
        Jevaing::Internal::LogLevel level,
        const std::string& message
    )
    {
        g_editor.Console.push_back({ level, message });

        if (g_editor.Console.size() > 500)
        {
            g_editor.Console.erase(g_editor.Console.begin());
        }
    }

    void CreateRenderTarget()
    {
        ID3D11Texture2D* backBuffer = nullptr;
        g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (backBuffer)
        {
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTargetView);
            backBuffer->Release();
        }
    }

    void CleanupRenderTarget()
    {
        if (g_renderTargetView)
        {
            g_renderTargetView->Release();
            g_renderTargetView = nullptr;
        }
    }

    bool CreateDeviceD3D(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC desc = {};
        desc.BufferCount = 2;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.RefreshRate.Numerator = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = hwnd;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        const D3D_FEATURE_LEVEL featureLevelArray[2] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0
        };
        D3D_FEATURE_LEVEL featureLevel;

        const HRESULT result =
            D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                0,
                featureLevelArray,
                2,
                D3D11_SDK_VERSION,
                &desc,
                &g_swapChain,
                &g_device,
                &featureLevel,
                &g_context
            );

        if (FAILED(result))
        {
            return false;
        }

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D()
    {
        CleanupRenderTarget();

        if (g_swapChain)
        {
            g_swapChain->Release();
            g_swapChain = nullptr;
        }

        if (g_context)
        {
            g_context->Release();
            g_context = nullptr;
        }

        if (g_device)
        {
            g_device->Release();
            g_device = nullptr;
        }
    }

    LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        {
            return true;
        }

        switch (msg)
        {
            case WM_SIZE:
                if (g_device && wParam != SIZE_MINIMIZED)
                {
                    CleanupRenderTarget();
                    g_swapChain->ResizeBuffers(
                        0,
                        static_cast<UINT>(LOWORD(lParam)),
                        static_cast<UINT>(HIWORD(lParam)),
                        DXGI_FORMAT_UNKNOWN,
                        0
                    );
                    CreateRenderTarget();
                }
                return 0;

            case WM_SYSCOMMAND:
                if ((wParam & 0xfff0) == SC_KEYMENU)
                {
                    return 0;
                }
                break;

            case WM_DESTROY:
                g_hwnd = nullptr;
                PostQuitMessage(0);
                return 0;

            case WM_CLOSE:
                RequestPendingAction(PendingEditorAction::Exit);
                return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    std::string ToRelativeScenePath(const std::filesystem::path& path)
    {
        if (!g_editor.HasProject)
        {
            return path.string();
        }

        std::error_code ec;
        const std::filesystem::path relative =
            std::filesystem::relative(
                path,
                g_editor.Project.ProjectDirectory,
                ec
            );

        return ec ? path.string() : relative.string();
    }

    bool LoadProject(const std::string& projectPath)
    {
        std::string error;
        Jevaing::ProjectConfig config;

        if (!Jevaing::Project::Load(projectPath, config, error))
        {
            Jevaing::Internal::Logger::Error(error);
            return false;
        }

        Jevaing::Scene scene;
        const std::string scenePath =
            Jevaing::Project::ResolveStartupScenePath(config);

        if (!scene.Load(scenePath, Jevaing::Project::ResolvePath(config, config.AssetRoot), error))
        {
            Jevaing::Internal::Logger::Error(error);
            return false;
        }

        g_editor.Project = config;
        g_editor.EditScene = std::move(scene);
        g_editor.CurrentScenePath = scenePath;
        g_editor.HasProject = true;
        g_editor.Playing = false;
        g_editor.Paused = false;
        g_editor.StepRequested = false;
        g_editor.DirtyScene = false;
        g_editor.SelectedEntity = Jevaing::InvalidEntityId;
        g_editor.ProjectSelectedDirectory =
            (std::filesystem::path(config.ProjectDirectory) / config.AssetRoot).string();
        g_editor.BuildSettings.StartupScene = config.StartupScene;
        g_editor.BuildSettings.ScenesInBuild = { config.StartupScene };
        Jevaing::Internal::Logger::Info("Opened project: " + config.Name);
        return true;
    }

    void SaveScene()
    {
        if (!g_editor.HasProject || g_editor.CurrentScenePath.empty())
        {
            return;
        }

        std::string error;
        if (!g_editor.EditScene.Save(g_editor.CurrentScenePath, error))
        {
            Jevaing::Internal::Logger::Error(error);
            return;
        }

        g_editor.DirtyScene = false;
        Jevaing::Internal::Logger::Info("Saved scene: " + g_editor.CurrentScenePath);
    }

    bool LoadSceneFile(const std::string& scenePath)
    {
        if (!g_editor.HasProject)
        {
            return false;
        }

        std::string error;
        Jevaing::Scene scene;
        const std::string assetRoot =
            Jevaing::Project::ResolvePath(g_editor.Project, g_editor.Project.AssetRoot);

        if (!scene.Load(scenePath, assetRoot, error))
        {
            Jevaing::Internal::Logger::Error(error);
            return false;
        }

        StopPlayMode();
        g_editor.EditScene = std::move(scene);
        g_editor.CurrentScenePath = scenePath;
        g_editor.SelectedEntity = Jevaing::InvalidEntityId;
        g_editor.DirtyScene = false;
        Jevaing::Internal::Logger::Info("Loaded scene: " + scenePath);
        return true;
    }

    void CloseProject()
    {
        StopPlayMode();
        g_editor.HasProject = false;
        g_editor.DirtyScene = false;
        g_editor.CurrentScenePath.clear();
        g_editor.PlaySnapshotPath.clear();
        g_editor.PendingPath.clear();
        g_editor.ProjectSelectedDirectory.clear();
        g_editor.Project = Jevaing::ProjectConfig{};
        g_editor.EditScene = Jevaing::Scene{"Untitled"};
        g_editor.PlayScene = Jevaing::Scene{"Play"};
        g_editor.SelectedEntity = Jevaing::InvalidEntityId;
        g_editor.ShowProjectBrowser = true;
        Jevaing::Internal::Logger::Info("Closed project.");
    }

    void ExecutePendingAction()
    {
        const PendingEditorAction action = g_pendingAction;
        const std::string path = g_editor.PendingPath;
        g_pendingAction = PendingEditorAction::None;
        g_editor.PendingPath.clear();

        switch (action)
        {
            case PendingEditorAction::Exit:
                PostQuitMessage(0);
                break;

            case PendingEditorAction::CloseProject:
                CloseProject();
                break;

            case PendingEditorAction::LoadProject:
                LoadProject(path);
                break;

            case PendingEditorAction::LoadScene:
                LoadSceneFile(path);
                break;

            case PendingEditorAction::None:
                break;
        }
    }

    void RequestPendingAction(PendingEditorAction action, const std::string& path)
    {
        if (g_editor.DirtyScene)
        {
            g_pendingAction = action;
            g_editor.PendingPath = path;
            g_editor.RequestUnsavedPopup = true;
            return;
        }

        g_pendingAction = action;
        g_editor.PendingPath = path;
        ExecutePendingAction();
    }

    Jevaing::Scene& ActiveScene()
    {
        return g_editor.Playing ? g_editor.PlayScene : g_editor.EditScene;
    }

    void StartPlayMode()
    {
        if (!g_editor.HasProject || g_editor.Playing)
        {
            return;
        }

        std::string error;
        const std::filesystem::path snapshot =
            std::filesystem::temp_directory_path() / "jevaing-editor-playmode.scene";

        if (!g_editor.EditScene.Save(snapshot.string(), error))
        {
            Jevaing::Internal::Logger::Error(error);
            return;
        }

        Jevaing::Scene playScene;

        if (!playScene.Load(
            snapshot.string(),
            Jevaing::Project::ResolvePath(g_editor.Project, g_editor.Project.AssetRoot),
            error
        ))
        {
            Jevaing::Internal::Logger::Error(error);
            return;
        }

        g_editor.PlaySnapshotPath = snapshot.string();
        g_editor.PlayScene = std::move(playScene);
        g_editor.Playing = true;
        g_editor.Paused = false;
        g_editor.StepRequested = false;
        Jevaing::Internal::Logger::Info("Play Mode started.");
    }

    void StopPlayMode()
    {
        if (!g_editor.Playing)
        {
            return;
        }

        g_editor.PlayScene = Jevaing::Scene{"Play"};
        g_editor.Playing = false;
        g_editor.Paused = false;
        g_editor.StepRequested = false;
        Jevaing::Internal::Logger::Info("Play Mode stopped. Edit Scene restored.");
    }

    void RenderProjectBrowser()
    {
        ImGui::SetNextWindowSize(ImVec2(560, 320), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Project Browser", &g_editor.ShowProjectBrowser, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }
        ImGui::TextUnformatted("New Project");
        ImGui::InputText("Project Name", &g_editor.NewProjectName);
        ImGui::InputText("Location", &g_editor.NewProjectLocation);

        if (ImGui::Button("Create Project"))
        {
            std::string projectFile;
            std::string error;
            if (Jevaing::Internal::CreateProjectFromTemplate(
                g_editor.NewProjectName,
                g_editor.NewProjectLocation.empty()
                    ? std::filesystem::current_path().string()
                    : g_editor.NewProjectLocation,
                projectFile,
                error
            ))
            {
                RequestPendingAction(PendingEditorAction::LoadProject, projectFile);
            }
            else
            {
                Jevaing::Internal::Logger::Error(error);
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Open Project");
        ImGui::InputText("jevaing.project", &g_editor.OpenProjectPath);

        if (ImGui::Button("Open Project"))
        {
            RequestPendingAction(PendingEditorAction::LoadProject, g_editor.OpenProjectPath);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Recent Projects");
        ImGui::TextDisabled("Recent projects persistence is available in the 0.0.11 settings file roadmap.");
        ImGui::End();
    }

    void RenderEntityNode(Jevaing::SceneEntity& entity)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (entity.Children.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (g_editor.SelectedEntity == entity.Id)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open =
            ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<intptr_t>(entity.Id)),
                flags,
                "%s",
                entity.Name.c_str()
            );

        if (ImGui::IsItemClicked())
        {
            g_editor.SelectedEntity = entity.Id;
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Create Empty"))
            {
                Jevaing::EntityId childId = g_editor.EditScene.CreateEntity("Entity");
                g_editor.EditScene.SetParent(childId, entity.Id);
                g_editor.DirtyScene = true;
            }

            if (ImGui::MenuItem("Delete"))
            {
                g_editor.EditScene.DestroyEntity(entity.Id);
                g_editor.SelectedEntity = Jevaing::InvalidEntityId;
                g_editor.DirtyScene = true;
                ImGui::EndPopup();
                if (open)
                {
                    ImGui::TreePop();
                }
                return;
            }

            ImGui::EndPopup();
        }

        if (open)
        {
            std::vector<Jevaing::EntityId> children = entity.Children;
            for (Jevaing::EntityId childId : children)
            {
                Jevaing::SceneEntity* child = g_editor.EditScene.FindEntity(childId);
                if (child)
                {
                    RenderEntityNode(*child);
                }
            }
            ImGui::TreePop();
        }
    }

    void RenderHierarchy()
    {
        ImGui::SetNextWindowPos(ImVec2(8, 58), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(280, 380), ImGuiCond_Once);
        if (!ImGui::Begin("Hierarchy", &g_editor.ShowHierarchy))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Create Empty"))
        {
            g_editor.SelectedEntity = g_editor.EditScene.CreateEntity("Entity");
            g_editor.DirtyScene = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Create Cube"))
        {
            Jevaing::SceneEntity* cube =
                g_editor.EditScene.FindEntity(g_editor.EditScene.CreateEntity("Cube"));
            cube->MeshRenderer = Jevaing::MeshRendererComponent{};
            cube->BoxCollider3D = Jevaing::BoxCollider3DComponent{};
            g_editor.SelectedEntity = cube->Id;
            g_editor.DirtyScene = true;
        }

        for (Jevaing::SceneEntity& entity : g_editor.EditScene.GetEntities())
        {
            if (entity.Parent == Jevaing::InvalidEntityId)
            {
                RenderEntityNode(entity);
            }
        }

        ImGui::End();
    }

    void EditVec3(const char* label, Jevaing::Vec3& value)
    {
        float values[3] = { value.X, value.Y, value.Z };
        if (ImGui::DragFloat3(label, values, 0.05f))
        {
            value = { values[0], values[1], values[2] };
            g_editor.DirtyScene = true;
        }
    }

    template <typename Component>
    void RemoveComponentButton(const char* label, std::optional<Component>& component)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton(label))
        {
            component.reset();
            g_editor.DirtyScene = true;
        }
    }

    void RenderInspector()
    {
        ImGui::SetNextWindowPos(ImVec2(980, 350), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(340, 360), ImGuiCond_Once);
        if (!ImGui::Begin("Inspector", &g_editor.ShowInspector))
        {
            ImGui::End();
            return;
        }
        Jevaing::SceneEntity* entity =
            g_editor.EditScene.FindEntity(g_editor.SelectedEntity);

        if (!entity)
        {
            ImGui::TextUnformatted("No entity selected.");
            ImGui::End();
            return;
        }

        if (ImGui::InputText("Name", &entity->Name))
        {
            g_editor.DirtyScene = true;
        }

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            EditVec3("Position", entity->Transform.LocalTransform.Position);
            EditVec3("Rotation", entity->Transform.LocalTransform.Rotation);
            EditVec3("Scale", entity->Transform.LocalTransform.Scale);
        }

        if (entity->Camera)
        {
            ImGui::SeparatorText("CameraComponent");
            RemoveComponentButton("Remove Camera", entity->Camera);
        }

        if (entity->MeshRenderer)
        {
            ImGui::SeparatorText("MeshRendererComponent");
            RemoveComponentButton("Remove MeshRenderer", entity->MeshRenderer);
        }

        if (entity->SpriteRenderer2D)
        {
            ImGui::SeparatorText("SpriteRenderer2DComponent");
            RemoveComponentButton("Remove SpriteRenderer2D", entity->SpriteRenderer2D);
        }

        if (entity->RigidBody3D)
        {
            ImGui::SeparatorText("RigidBody3DComponent");
            int type = static_cast<int>(entity->RigidBody3D->Type);
            if (ImGui::Combo("Type##rb3d", &type, "Static\0Kinematic\0Dynamic\0"))
            {
                entity->RigidBody3D->Type = static_cast<Jevaing::BodyType>(type);
                g_editor.DirtyScene = true;
            }
            ImGui::DragFloat("Gravity Factor", &entity->RigidBody3D->GravityFactor, 0.05f);
            RemoveComponentButton("Remove RigidBody3D", entity->RigidBody3D);
        }

        if (entity->BoxCollider3D)
        {
            ImGui::SeparatorText("BoxCollider3DComponent");
            EditVec3("Size##box3d", entity->BoxCollider3D->Size);
            ImGui::Checkbox("Trigger##box3d", &entity->BoxCollider3D->IsTrigger);
            RemoveComponentButton("Remove BoxCollider3D", entity->BoxCollider3D);
        }

        if (entity->SphereCollider3D)
        {
            ImGui::SeparatorText("SphereCollider3DComponent");
            ImGui::DragFloat("Radius##sphere3d", &entity->SphereCollider3D->Radius, 0.05f, 0.01f);
            ImGui::Checkbox("Trigger##sphere3d", &entity->SphereCollider3D->IsTrigger);
            RemoveComponentButton("Remove SphereCollider3D", entity->SphereCollider3D);
        }

        if (entity->CapsuleCollider3D)
        {
            ImGui::SeparatorText("CapsuleCollider3DComponent");
            ImGui::DragFloat("Radius##capsule3d", &entity->CapsuleCollider3D->Radius, 0.05f, 0.01f);
            ImGui::DragFloat("Height##capsule3d", &entity->CapsuleCollider3D->Height, 0.05f, 0.01f);
            RemoveComponentButton("Remove CapsuleCollider3D", entity->CapsuleCollider3D);
        }

        if (entity->RigidBody2D)
        {
            ImGui::SeparatorText("RigidBody2DComponent");
            int type = static_cast<int>(entity->RigidBody2D->Type);
            if (ImGui::Combo("Type##rb2d", &type, "Static\0Kinematic\0Dynamic\0"))
            {
                entity->RigidBody2D->Type = static_cast<Jevaing::BodyType>(type);
                g_editor.DirtyScene = true;
            }
            RemoveComponentButton("Remove RigidBody2D", entity->RigidBody2D);
        }

        if (entity->BoxCollider2D)
        {
            ImGui::SeparatorText("BoxCollider2DComponent");
            float size[2] = { entity->BoxCollider2D->Size.X, entity->BoxCollider2D->Size.Y };
            if (ImGui::DragFloat2("Size##box2d", size, 0.05f))
            {
                entity->BoxCollider2D->Size = { size[0], size[1] };
                g_editor.DirtyScene = true;
            }
            RemoveComponentButton("Remove BoxCollider2D", entity->BoxCollider2D);
        }

        if (entity->CircleCollider2D)
        {
            ImGui::SeparatorText("CircleCollider2DComponent");
            ImGui::DragFloat("Radius##circle2d", &entity->CircleCollider2D->Radius, 0.05f, 0.01f);
            RemoveComponentButton("Remove CircleCollider2D", entity->CircleCollider2D);
        }

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("CameraComponent")) entity->Camera = Jevaing::CameraComponent{};
            if (ImGui::MenuItem("MeshRendererComponent")) entity->MeshRenderer = Jevaing::MeshRendererComponent{};
            if (ImGui::MenuItem("SpriteRenderer2DComponent")) entity->SpriteRenderer2D = Jevaing::SpriteRenderer2DComponent{};
            if (ImGui::MenuItem("RigidBody2DComponent")) entity->RigidBody2D = Jevaing::RigidBody2DComponent{};
            if (ImGui::MenuItem("BoxCollider2DComponent")) entity->BoxCollider2D = Jevaing::BoxCollider2DComponent{};
            if (ImGui::MenuItem("CircleCollider2DComponent")) entity->CircleCollider2D = Jevaing::CircleCollider2DComponent{};
            if (ImGui::MenuItem("RigidBody3DComponent")) entity->RigidBody3D = Jevaing::RigidBody3DComponent{};
            if (ImGui::MenuItem("BoxCollider3DComponent")) entity->BoxCollider3D = Jevaing::BoxCollider3DComponent{};
            if (ImGui::MenuItem("SphereCollider3DComponent")) entity->SphereCollider3D = Jevaing::SphereCollider3DComponent{};
            if (ImGui::MenuItem("CapsuleCollider3DComponent")) entity->CapsuleCollider3D = Jevaing::CapsuleCollider3DComponent{};
            g_editor.DirtyScene = true;
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    ImVec2 EntityScreenPosition(
        const Jevaing::SceneEntity& entity,
        const ImVec2& center,
        float scale,
        const Jevaing::Vec3& offset = {}
    )
    {
        const Jevaing::Vec3& p = entity.Transform.LocalTransform.Position;
        return ImVec2(
            center.x + (p.X - offset.X) * scale,
            center.y - (p.Y - offset.Y) * scale
        );
    }

    ImVec2 EntityScreenSize(const Jevaing::SceneEntity& entity, float scale)
    {
        const Jevaing::Vec3& s = entity.Transform.LocalTransform.Scale;
        return ImVec2(
            std::max(10.0f, std::fabs(s.X) * scale),
            std::max(10.0f, std::fabs(s.Y) * scale)
        );
    }

    bool PointInEntityRect(
        const ImVec2& point,
        const Jevaing::SceneEntity& entity,
        const ImVec2& center,
        float scale
    )
    {
        const ImVec2 screen = EntityScreenPosition(entity, center, scale);
        const ImVec2 size = EntityScreenSize(entity, scale);
        return
            point.x >= screen.x - size.x * 0.5f &&
            point.x <= screen.x + size.x * 0.5f &&
            point.y >= screen.y - size.y * 0.5f &&
            point.y <= screen.y + size.y * 0.5f;
    }

    void DrawSceneEntity(
        ImDrawList* drawList,
        const Jevaing::SceneEntity& entity,
        const ImVec2& center,
        float scale,
        bool selected,
        const Jevaing::Vec3& offset = {}
    )
    {
        const ImVec2 screen = EntityScreenPosition(entity, center, scale, offset);
        const ImVec2 size = EntityScreenSize(entity, scale);
        const ImU32 color =
            selected
                ? IM_COL32(255, 210, 75, 255)
                : entity.Camera
                    ? IM_COL32(120, 230, 150, 255)
                    : IM_COL32(95, 180, 255, 255);

        drawList->AddRect(
            ImVec2(screen.x - size.x * 0.5f, screen.y - size.y * 0.5f),
            ImVec2(screen.x + size.x * 0.5f, screen.y + size.y * 0.5f),
            color,
            0.0f,
            0,
            selected ? 3.0f : 1.5f
        );
        drawList->AddText(ImVec2(screen.x + 6.0f, screen.y - 18.0f), color, entity.Name.c_str());
    }

    void RenderSceneView()
    {
        ImGui::SetNextWindowPos(ImVec2(300, 58), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(560, 380), ImGuiCond_Once);
        if (!ImGui::Begin("Scene", &g_editor.ShowScene))
        {
            ImGui::End();
            return;
        }

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton(
            "SceneCanvas",
            size,
            ImGuiButtonFlags_MouseButtonLeft
        );

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(22, 24, 28, 255));

        const float scale = 42.0f;
        const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.65f);

        for (int i = -10; i <= 10; ++i)
        {
            const float x = center.x + i * scale;
            drawList->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + size.y), IM_COL32(48, 52, 58, 255));
            const float y = center.y + i * scale;
            drawList->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + size.x, y), IM_COL32(48, 52, 58, 255));
        }

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            g_editor.SelectedEntity = Jevaing::InvalidEntityId;
            std::vector<Jevaing::SceneEntity>& entities = g_editor.EditScene.GetEntities();
            for (auto it = entities.rbegin(); it != entities.rend(); ++it)
            {
                if (PointInEntityRect(mouse, *it, center, scale))
                {
                    g_editor.SelectedEntity = it->Id;
                    break;
                }
            }
        }

        if (active && !g_editor.Playing && g_editor.SelectedEntity != Jevaing::InvalidEntityId)
        {
            Jevaing::SceneEntity* entity =
                g_editor.EditScene.FindEntity(g_editor.SelectedEntity);
            if (entity && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                entity->Transform.LocalTransform.Position.X = (mouse.x - center.x) / scale;
                entity->Transform.LocalTransform.Position.Y = -(mouse.y - center.y) / scale;
                g_editor.DirtyScene = true;
            }
        }

        for (const Jevaing::SceneEntity& entity : g_editor.EditScene.GetEntities())
        {
            DrawSceneEntity(
                drawList,
                entity,
                center,
                scale,
                entity.Id == g_editor.SelectedEntity
            );
        }

        ImGui::End();
    }

    void RenderGameView()
    {
        ImGui::SetNextWindowPos(ImVec2(872, 58), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(430, 280), ImGuiCond_Once);
        if (!ImGui::Begin("Game", &g_editor.ShowGame))
        {
            ImGui::End();
            return;
        }

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("GameCanvas", available);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(
            origin,
            ImVec2(origin.x + available.x, origin.y + available.y),
            IM_COL32(12, 13, 16, 255)
        );

        const float targetAspect = 16.0f / 9.0f;
        float width = available.x;
        float height = width / targetAspect;
        if (height > available.y)
        {
            height = available.y;
            width = height * targetAspect;
        }

        const ImVec2 frameMin(
            origin.x + (available.x - width) * 0.5f,
            origin.y + (available.y - height) * 0.5f
        );
        const ImVec2 frameMax(frameMin.x + width, frameMin.y + height);
        drawList->AddRectFilled(frameMin, frameMax, IM_COL32(24, 26, 31, 255));
        drawList->AddRect(frameMin, frameMax, IM_COL32(75, 82, 94, 255));

        const Jevaing::Scene& scene = ActiveScene();
        const Jevaing::SceneEntity* cameraEntity = nullptr;
        for (const Jevaing::SceneEntity& entity : scene.GetEntities())
        {
            if (entity.Camera && (!cameraEntity || entity.Camera->Primary))
            {
                cameraEntity = &entity;
                if (entity.Camera->Primary)
                {
                    break;
                }
            }
        }

        const Jevaing::Vec3 cameraOffset =
            cameraEntity
                ? cameraEntity->Transform.LocalTransform.Position
                : Jevaing::Vec3{};
        const ImVec2 center(frameMin.x + width * 0.5f, frameMin.y + height * 0.5f);
        const float scale = std::max(20.0f, std::min(width, height) * 0.12f);

        for (const Jevaing::SceneEntity& entity : scene.GetEntities())
        {
            if (entity.Camera)
            {
                continue;
            }

            DrawSceneEntity(
                drawList,
                entity,
                center,
                scale,
                false,
                cameraOffset
            );
        }

        if (g_editor.Playing && g_editor.Paused)
        {
            drawList->AddText(
                ImVec2(frameMin.x + 8.0f, frameMin.y + 8.0f),
                IM_COL32(255, 210, 75, 255),
                "Paused"
            );
        }

        ImGui::End();
    }

    std::string LowerExtension(const std::filesystem::path& path)
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return extension;
    }

    bool IsKnownProjectFile(const std::filesystem::path& path)
    {
        const std::string extension = LowerExtension(path);
        return
            extension == ".scene" ||
            extension == ".cpp" ||
            extension == ".c" ||
            extension == ".h" ||
            extension == ".hpp" ||
            extension == ".glb" ||
            extension == ".gltf" ||
            extension == ".fbx" ||
            extension == ".png" ||
            extension == ".jpg" ||
            extension == ".jpeg";
    }

    void WriteProjectTextFile(const std::filesystem::path& path, const std::string& text)
    {
        if (std::filesystem::exists(path))
        {
            Jevaing::Internal::Logger::Warning("File already exists: " + path.string());
            return;
        }

        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file)
        {
            Jevaing::Internal::Logger::Error("Failed to create file: " + path.string());
            return;
        }

        file << text;
        Jevaing::Internal::Logger::Info("Created file: " + path.string());
    }

    void RenderFolderNode(const std::filesystem::path& path)
    {
        const std::string id = path.string();
        ImGui::PushID(id.c_str());
        const bool selected = g_editor.ProjectSelectedDirectory == path.string();
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_DefaultOpen;
        if (selected)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool open = ImGui::TreeNodeEx(path.filename().string().c_str(), flags);
        if (ImGui::IsItemClicked())
        {
            g_editor.ProjectSelectedDirectory = path.string();
        }

        if (open)
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_directory())
                {
                    RenderFolderNode(entry.path());
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    std::filesystem::path CodeCreationDirectory(const std::filesystem::path& selectedDirectory)
    {
        const std::filesystem::path sourceDirectory =
            std::filesystem::path(g_editor.Project.ProjectDirectory) / "Source";
        std::error_code ec;
        const std::filesystem::path relative =
            std::filesystem::relative(selectedDirectory, sourceDirectory, ec);
        if (!ec && !relative.empty() && *relative.begin() != "..")
        {
            return selectedDirectory;
        }

        return sourceDirectory;
    }

    void CreateProjectAssetFromMenu(const std::filesystem::path& directory)
    {
        const std::filesystem::path codeDirectory = CodeCreationDirectory(directory);

        if (ImGui::MenuItem("C++ Script"))
        {
            WriteProjectTextFile(
                codeDirectory / "NewScript.cpp",
                "#include <Jevaing/Jevaing.h>\n\nvoid NewScriptTick(double deltaTime)\n{\n    (void)deltaTime;\n}\n"
            );
        }
        if (ImGui::MenuItem("C Source"))
        {
            WriteProjectTextFile(
                codeDirectory / "NewModule.c",
                "#include \"NewModule.h\"\n\nvoid new_module_tick(float delta_time)\n{\n    (void)delta_time;\n}\n"
            );
            WriteProjectTextFile(
                codeDirectory / "NewModule.h",
                "#pragma once\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\nvoid new_module_tick(float delta_time);\n\n#ifdef __cplusplus\n}\n#endif\n"
            );
        }
        if (ImGui::MenuItem("Header"))
        {
            WriteProjectTextFile(codeDirectory / "NewHeader.h", "#pragma once\n");
        }
        if (ImGui::MenuItem("Folder"))
        {
            std::filesystem::create_directories(directory / "NewFolder");
        }
    }

    void RenderProject()
    {
        ImGui::SetNextWindowPos(ImVec2(300, 452), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(650, 240), ImGuiCond_Once);
        ImGui::Begin("Project", &g_editor.ShowProject);

        if (!g_editor.HasProject)
        {
            ImGui::TextUnformatted("No project open.");
            ImGui::End();
            return;
        }

        const std::filesystem::path projectDir = g_editor.Project.ProjectDirectory;
        if (g_editor.ProjectSelectedDirectory.empty())
        {
            g_editor.ProjectSelectedDirectory =
                (projectDir / g_editor.Project.AssetRoot).string();
        }

        if (ImGui::Button("Refresh"))
        {
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(ToRelativeScenePath(g_editor.ProjectSelectedDirectory).c_str());

        if (ImGui::BeginTable("ProjectSplit", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Assets");
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const std::filesystem::path roots[] =
            {
                projectDir / g_editor.Project.AssetRoot,
                projectDir / g_editor.Project.SceneRoot,
                projectDir / "Source"
            };

            for (const std::filesystem::path& root : roots)
            {
                if (std::filesystem::exists(root))
                {
                    RenderFolderNode(root);
                }
            }

            ImGui::TableSetColumnIndex(1);
            const std::filesystem::path selectedDir = g_editor.ProjectSelectedDirectory;
            if (std::filesystem::exists(selectedDir))
            {
                if (ImGui::BeginPopupContextWindow("ProjectCreatePopup"))
                {
                    if (ImGui::BeginMenu("Create"))
                    {
                        CreateProjectAssetFromMenu(selectedDir);
                        ImGui::EndMenu();
                    }
                    ImGui::EndPopup();
                }

                for (const auto& entry : std::filesystem::directory_iterator(selectedDir))
                {
                    if (!entry.is_regular_file() || !IsKnownProjectFile(entry.path()))
                    {
                        continue;
                    }

                    const std::string label = entry.path().filename().string();
                    if (ImGui::Selectable(label.c_str()))
                    {
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            if (LowerExtension(entry.path()) == ".scene")
                            {
                                RequestPendingAction(
                                    PendingEditorAction::LoadScene,
                                    entry.path().string()
                                );
                            }
                        }
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void RenderConsole()
    {
        ImGui::SetNextWindowPos(ImVec2(8, 452), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(280, 240), ImGuiCond_Once);
        ImGui::Begin("Console", &g_editor.ShowConsole);
        if (ImGui::Button("Clear"))
        {
            g_editor.Console.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Info", &g_editor.ShowInfoLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &g_editor.ShowWarningLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &g_editor.ShowErrorLogs);
        ImGui::Separator();

        for (const ConsoleEntry& entry : g_editor.Console)
        {
            const bool visible =
                (entry.Level == Jevaing::Internal::LogLevel::Info && g_editor.ShowInfoLogs) ||
                (entry.Level == Jevaing::Internal::LogLevel::Warning && g_editor.ShowWarningLogs) ||
                (entry.Level == Jevaing::Internal::LogLevel::Error && g_editor.ShowErrorLogs);

            if (!visible)
            {
                continue;
            }

            ImVec4 color(0.82f, 0.86f, 0.9f, 1.0f);
            if (entry.Level == Jevaing::Internal::LogLevel::Warning)
            {
                color = ImVec4(1.0f, 0.82f, 0.32f, 1.0f);
            }
            else if (entry.Level == Jevaing::Internal::LogLevel::Error)
            {
                color = ImVec4(1.0f, 0.36f, 0.36f, 1.0f);
            }

            ImGui::TextColored(color, "%s", entry.Message.c_str());
        }

        ImGui::End();
    }

    void RenderBuildSettings()
    {
        ImGui::SetNextWindowPos(ImVec2(964, 58), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(360, 280), ImGuiCond_Once);
        ImGui::Begin("Build Settings", &g_editor.ShowBuildSettings);
        ImGui::TextUnformatted("Scenes in Build");
        for (const std::string& scene : g_editor.BuildSettings.ScenesInBuild)
        {
            bool enabled = true;
            ImGui::Checkbox(scene.c_str(), &enabled);
        }

        ImGui::SeparatorText("Target");
        for (const Jevaing::Internal::BuildTargetInfo& target : Jevaing::Internal::GetBuildTargetInfo())
        {
            const bool selected = g_editor.BuildSettings.Target == target.Target;
            std::string label = target.DisplayName;
            if (target.Experimental)
            {
                label += " [Experimental]";
            }
            if (!target.Available)
            {
                label += " [Unavailable]";
            }

            if (ImGui::RadioButton(label.c_str(), selected))
            {
                g_editor.BuildSettings.Target = target.Target;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", target.Reason.c_str());
            }
        }

        int config = g_editor.BuildSettings.Configuration == Jevaing::Internal::BuildConfiguration::Debug ? 0 : 1;
        if (ImGui::Combo("Configuration", &config, "Debug\0Release\0"))
        {
            g_editor.BuildSettings.Configuration =
                config == 0
                    ? Jevaing::Internal::BuildConfiguration::Debug
                    : Jevaing::Internal::BuildConfiguration::Release;
        }

        ImGui::InputText("Output", &g_editor.BuildSettings.OutputDirectory);

        if (ImGui::Button("Build"))
        {
            if (!g_editor.HasProject)
            {
                Jevaing::Internal::Logger::Error("No project open.");
            }
            else if (g_editor.BuildSettings.Target == Jevaing::Internal::BuildTarget::WindowsDesktopX64)
            {
                const Jevaing::Internal::BuildResult result =
                    Jevaing::Internal::BuildWindowsDesktop(
                        g_editor.Project,
                        g_editor.BuildSettings,
                        false
                    );
                const std::string message =
                    result.Message + (result.OutputPath.empty() ? "" : " Output: " + result.OutputPath);
                if (result.Success)
                {
                    Jevaing::Internal::Logger::Info(message);
                }
                else
                {
                    Jevaing::Internal::Logger::Error(message);
                }
            }
            else
            {
                Jevaing::Internal::Logger::Warning("Selected target is not buildable from the editor in this pass.");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Build & Run"))
        {
            if (g_editor.HasProject && g_editor.BuildSettings.Target == Jevaing::Internal::BuildTarget::WindowsDesktopX64)
            {
                Jevaing::Internal::BuildWindowsDesktop(g_editor.Project, g_editor.BuildSettings, true);
            }
            else
            {
                Jevaing::Internal::Logger::Warning("Build & Run is implemented only for Windows Desktop x64.");
            }
        }

        ImGui::End();
    }

    void RenderMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    SaveScene();
                }
                if (ImGui::MenuItem("Close Project", nullptr, false, g_editor.HasProject))
                {
                    RequestPendingAction(PendingEditorAction::CloseProject);
                }
                if (ImGui::MenuItem("Stop Play Mode", nullptr, false, g_editor.Playing))
                {
                    StopPlayMode();
                }
                if (ImGui::MenuItem("Exit"))
                {
                    RequestPendingAction(PendingEditorAction::Exit);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("GameObject"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Assets"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Build"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows"))
            {
                ImGui::MenuItem("Project Browser", nullptr, &g_editor.ShowProjectBrowser);
                ImGui::MenuItem("Hierarchy", nullptr, &g_editor.ShowHierarchy);
                ImGui::MenuItem("Scene", nullptr, &g_editor.ShowScene);
                ImGui::MenuItem("Game", nullptr, &g_editor.ShowGame);
                ImGui::MenuItem("Inspector", nullptr, &g_editor.ShowInspector);
                ImGui::MenuItem("Project", nullptr, &g_editor.ShowProject);
                ImGui::MenuItem("Console", nullptr, &g_editor.ShowConsole);
                ImGui::MenuItem("Build Settings", nullptr, &g_editor.ShowBuildSettings);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::EndMenu();
            }

            ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 94.0f);
            if (!g_editor.Playing)
            {
                if (ImGui::Button("Play"))
                {
                    StartPlayMode();
                }
            }
            else
            {
                if (ImGui::Button(g_editor.Paused ? "Resume" : "Pause"))
                {
                    g_editor.Paused = !g_editor.Paused;
                }
                ImGui::SameLine();
                if (ImGui::Button("Step"))
                {
                    g_editor.StepRequested = true;
                    g_editor.Paused = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop"))
                {
                    StopPlayMode();
                }
            }

            ImGui::EndMainMenuBar();
        }
    }

    void RenderStatusBar()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 24.0f));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 24.0f));
        ImGui::Begin(
            "StatusBar",
            nullptr,
            ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings
        );
        ImGui::Text(
            "%s | %s%s | %s | %s",
            g_editor.HasProject ? g_editor.Project.Name.c_str() : "No Project",
            g_editor.CurrentScenePath.empty() ? "No Scene" : ToRelativeScenePath(g_editor.CurrentScenePath).c_str(),
            g_editor.DirtyScene ? " *" : "",
            g_editor.Playing
                ? (g_editor.Paused ? "PAUSED" : "PLAY")
                : "EDIT",
            Jevaing::Internal::BuildTargetToString(g_editor.BuildSettings.Target)
        );
        ImGui::End();
    }

    void RenderUnsavedChangesModal()
    {
        if (g_editor.RequestUnsavedPopup)
        {
            ImGui::OpenPopup("Unsaved Changes");
            g_editor.RequestUnsavedPopup = false;
        }

        bool executeAction = false;
        if (ImGui::BeginPopupModal(
            "Unsaved Changes",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        ))
        {
            ImGui::TextUnformatted("The current scene has unsaved changes.");
            ImGui::Separator();

            if (ImGui::Button("Save"))
            {
                SaveScene();
                if (!g_editor.DirtyScene)
                {
                    executeAction = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();

            if (ImGui::Button("Discard"))
            {
                g_editor.DirtyScene = false;
                executeAction = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                g_pendingAction = PendingEditorAction::None;
                g_editor.PendingPath.clear();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (executeAction)
        {
            ExecutePendingAction();
        }
    }

    void UpdatePlaySimulation(double deltaTime)
    {
        if (!g_editor.Playing)
        {
            return;
        }

        if (!g_editor.Paused || g_editor.StepRequested)
        {
            g_editor.PlayScene.Update(deltaTime);
            g_editor.StepRequested = false;
        }
    }

    void RenderEditor(double deltaTime)
    {
        RenderMainMenu();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        RenderUnsavedChangesModal();

        if (!g_editor.HasProject)
        {
            if (g_editor.ShowProjectBrowser)
            {
                RenderProjectBrowser();
            }
            if (g_editor.ShowConsole)
            {
                RenderConsole();
            }
            RenderStatusBar();
            return;
        }

        UpdatePlaySimulation(deltaTime);

        if (g_editor.ShowHierarchy)
        {
            RenderHierarchy();
        }
        if (g_editor.ShowScene)
        {
            RenderSceneView();
        }
        if (g_editor.ShowGame)
        {
            RenderGameView();
        }
        if (g_editor.ShowInspector)
        {
            RenderInspector();
        }
        if (g_editor.ShowProject)
        {
            RenderProject();
        }
        if (g_editor.ShowConsole)
        {
            RenderConsole();
        }
        if (g_editor.ShowBuildSettings)
        {
            RenderBuildSettings();
        }
        RenderStatusBar();
    }

    int RunEditor()
    {
        g_editor.NewProjectLocation = std::filesystem::current_path().string();
        Jevaing::Internal::Logger::SetSink(AddConsoleLog);
        Jevaing::Internal::Logger::Info("JevaingEditor initialized.");

        WNDCLASSEXW wc =
        {
            sizeof(WNDCLASSEXW),
            CS_CLASSDC,
            WndProc,
            0L,
            0L,
            GetModuleHandle(nullptr),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            L"JevaingEditor",
            nullptr
        };
        RegisterClassExW(&wc);

        HWND hwnd =
            CreateWindowW(
                wc.lpszClassName,
                L"JevaingEditor 0.0.11",
                WS_OVERLAPPEDWINDOW,
                100,
                100,
                1440,
                900,
                nullptr,
                nullptr,
                wc.hInstance,
                nullptr
            );
        g_hwnd = hwnd;

        if (!CreateDeviceD3D(hwnd))
        {
            CleanupDeviceD3D();
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return 1;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = "JevaingEditor.ini";

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(g_device, g_context);

        MSG msg = {};
        LARGE_INTEGER frequency = {};
        LARGE_INTEGER previous = {};
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&previous);

        while (msg.message != WM_QUIT)
        {
            while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            LARGE_INTEGER current = {};
            QueryPerformanceCounter(&current);
            const double deltaTime =
                static_cast<double>(current.QuadPart - previous.QuadPart) /
                static_cast<double>(frequency.QuadPart);
            previous = current;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            RenderEditor(deltaTime);

            ImGui::Render();
            const float clearColor[4] = { 0.08f, 0.09f, 0.11f, 1.0f };
            g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);
            g_context->ClearRenderTargetView(g_renderTargetView, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_swapChain->Present(1, 0);
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D();
        if (g_hwnd)
        {
            DestroyWindow(hwnd);
            g_hwnd = nullptr;
        }
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 0;
    }

    int RunEditorInfo()
    {
        Jevaing::Internal::Logger::Info("Editor enabled");
        Jevaing::Internal::Logger::Info(std::string("ImGui version: ") + IMGUI_VERSION);
        Jevaing::Internal::Logger::Info("Panels: Project Browser, Hierarchy, Scene, Game, Inspector, Project, Console, Build Settings");
        Jevaing::Internal::Logger::Info("Editor controls: Play, Pause, Step, Stop, Windows menu, mouse Scene editing, unsaved-change prompt");
        for (const Jevaing::Internal::BuildTargetInfo& target : Jevaing::Internal::GetBuildTargetInfo())
        {
            Jevaing::Internal::Logger::Info(
                target.DisplayName +
                ": " +
                (target.Available ? "available" : "unavailable") +
                " - " +
                target.Reason
            );
        }
        return 0;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "--editor-info")
    {
        return RunEditorInfo();
    }

    return RunEditor();
}
