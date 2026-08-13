#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Components.h"
#include "Entity.h"
#include "Graphics2D.h"
#include "Graphics3D.h"

namespace Jevaing
{
    struct SceneEntity
    {
        EntityId Id = InvalidEntityId;
        std::string Name;
        EntityId Parent = InvalidEntityId;
        std::vector<EntityId> Children;
        TransformComponent Transform;
        std::optional<CameraComponent> Camera;
        std::optional<MeshRendererComponent> MeshRenderer;
        std::optional<SpriteRenderer2DComponent> SpriteRenderer2D;
    };

    class Scene
    {
    public:
        explicit Scene(std::string name = {});

        EntityId CreateEntity(const std::string& name);
        EntityId CreateEntityWithId(EntityId id, const std::string& name);
        bool DestroyEntity(EntityId id);

        SceneEntity* FindEntity(EntityId id);
        const SceneEntity* FindEntity(EntityId id) const;
        SceneEntity* FindEntityByName(const std::string& name);
        const SceneEntity* FindEntityByName(const std::string& name) const;

        bool SetParent(EntityId child, EntityId parent, std::string* error = nullptr);
        bool RemoveParent(EntityId child);

        Transform GetWorldTransform(EntityId id) const;

        void OnLoad();
        void OnStart();
        void Update(double deltaTime);
        void Render(Graphics2D& graphics);
        void Render(Graphics3D& graphics);
        void OnUnload();

        bool Load(const std::string& path, const std::string& assetRoot, std::string& error);
        bool Save(const std::string& path, std::string& error) const;

        static bool LoadFromFile(
            const std::string& path,
            const std::string& assetRoot,
            Scene& scene,
            std::string& error
        );

        const std::string& GetName() const;
        void SetName(const std::string& name);

        const std::vector<SceneEntity>& GetEntities() const;
        std::vector<SceneEntity>& GetEntities();

    private:
        EntityId AllocateEntityId();
        bool WouldCreateCycle(EntityId child, EntityId parent) const;
        void RemoveChildLink(EntityId parent, EntityId child);
        void AttachLoadedAssets(const std::string& assetRoot, std::string& error);

    private:
        std::string m_name;
        std::vector<SceneEntity> m_entities;
        EntityId m_nextEntityId = 1;
        bool m_started = false;
    };
}
