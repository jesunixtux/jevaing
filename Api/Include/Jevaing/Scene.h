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
        std::optional<RigidBody2DComponent> RigidBody2D;
        std::optional<BoxCollider2DComponent> BoxCollider2D;
        std::optional<CircleCollider2DComponent> CircleCollider2D;
        std::optional<RigidBody3DComponent> RigidBody3D;
        std::optional<BoxCollider3DComponent> BoxCollider3D;
        std::optional<SphereCollider3DComponent> SphereCollider3D;
        std::optional<CapsuleCollider3DComponent> CapsuleCollider3D;
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

        PhysicsWorld2D& Physics2D();
        const PhysicsWorld2D& Physics2D() const;
        PhysicsWorld3D& Physics3D();
        const PhysicsWorld3D& Physics3D() const;

        void SetPhysicsSettings(const PhysicsSettings& settings);
        const PhysicsSettings& GetPhysicsSettings() const;

        const std::vector<CollisionEvent2D>& GetCollisionEvents2D() const;
        const std::vector<CollisionEvent2D>& GetTriggerEvents2D() const;
        const std::vector<CollisionEvent3D>& GetCollisionEvents3D() const;
        const std::vector<CollisionEvent3D>& GetTriggerEvents3D() const;

    private:
        EntityId AllocateEntityId();
        bool WouldCreateCycle(EntityId child, EntityId parent) const;
        void RemoveChildLink(EntityId parent, EntityId child);
        void AttachLoadedAssets(const std::string& assetRoot, std::string& error);
        void EnsurePhysicsInitialized();
        void SyncSceneToPhysics();
        void SyncPhysicsToScene();

    private:
        std::string m_name;
        std::vector<SceneEntity> m_entities;
        EntityId m_nextEntityId = 1;
        bool m_started = false;
        PhysicsSettings m_physicsSettings;
        PhysicsWorld2D m_physics2D;
        PhysicsWorld3D m_physics3D;
        double m_physicsAccumulator = 0.0;
    };
}
