#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "Entity.h"
#include "Types.h"

namespace Jevaing
{
    enum class Physics2DBackend
    {
        None,
        Box2D
    };

    enum class Physics3DBackend
    {
        None,
        Jolt
    };

    enum class BodyType
    {
        Static,
        Kinematic,
        Dynamic
    };

    enum class PhysicsEventType
    {
        Enter,
        Exit
    };

    struct PhysicsSettings
    {
        double FixedDeltaTime = 1.0 / 60.0;
    };

    struct PhysicsMaterial
    {
        float Friction = 0.55f;
        float Restitution = 0.05f;
        float Density = 1.0f;
    };

    struct RigidBody2DComponent
    {
        BodyType Type = BodyType::Dynamic;
        float GravityScale = 1.0f;
        float LinearDamping = 0.0f;
        float AngularDamping = 0.0f;
        bool Enabled = true;
    };

    struct BoxCollider2DComponent
    {
        Vec2 Offset = {};
        Vec2 Size = { 1.0f, 1.0f };
        PhysicsMaterial Material;
        bool IsTrigger = false;
    };

    struct CircleCollider2DComponent
    {
        Vec2 Offset = {};
        float Radius = 0.5f;
        PhysicsMaterial Material;
        bool IsTrigger = false;
    };

    struct RigidBody3DComponent
    {
        BodyType Type = BodyType::Dynamic;
        float GravityFactor = 1.0f;
        float LinearDamping = 0.0f;
        float AngularDamping = 0.0f;
        bool Enabled = true;
    };

    struct BoxCollider3DComponent
    {
        Vec3 Offset = {};
        Vec3 Size = { 1.0f, 1.0f, 1.0f };
        PhysicsMaterial Material;
        bool IsTrigger = false;
    };

    struct SphereCollider3DComponent
    {
        Vec3 Offset = {};
        float Radius = 0.5f;
        PhysicsMaterial Material;
        bool IsTrigger = false;
    };

    struct CapsuleCollider3DComponent
    {
        Vec3 Offset = {};
        float Radius = 0.35f;
        float Height = 1.0f;
        PhysicsMaterial Material;
        bool IsTrigger = false;
    };

    struct CollisionEvent2D
    {
        PhysicsEventType Type = PhysicsEventType::Enter;
        EntityId EntityA = InvalidEntityId;
        EntityId EntityB = InvalidEntityId;
        Vec2 Point = {};
        Vec2 Normal = {};
    };

    struct CollisionEvent3D
    {
        PhysicsEventType Type = PhysicsEventType::Enter;
        EntityId EntityA = InvalidEntityId;
        EntityId EntityB = InvalidEntityId;
        Vec3 Point = {};
        Vec3 Normal = {};
    };

    struct RaycastHit2D
    {
        bool Hit = false;
        EntityId Entity = InvalidEntityId;
        Vec2 Point = {};
        Vec2 Normal = {};
        float Distance = 0.0f;
    };

    struct RaycastHit3D
    {
        bool Hit = false;
        EntityId Entity = InvalidEntityId;
        Vec3 Point = {};
        Vec3 Normal = {};
        float Distance = 0.0f;
    };

    const char* Physics2DBackendToString(Physics2DBackend backend);
    const char* Physics3DBackendToString(Physics3DBackend backend);
    const char* BodyTypeToString(BodyType type);
    bool BodyTypeFromString(const std::string& value, BodyType& type);

    class PhysicsWorld2D
    {
    public:
        PhysicsWorld2D();
        ~PhysicsWorld2D();
        PhysicsWorld2D(PhysicsWorld2D&&) noexcept;
        PhysicsWorld2D& operator=(PhysicsWorld2D&&) noexcept;
        PhysicsWorld2D(const PhysicsWorld2D&) = delete;
        PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;

        bool Initialize(Physics2DBackend backend = Physics2DBackend::Box2D);
        void Shutdown();
        void Clear();

        bool IsAvailable() const;
        Physics2DBackend GetBackend() const;
        std::size_t GetBodyCount() const;
        bool HasBody(EntityId entity) const;

        void CreateOrUpdateBody(
            EntityId entity,
            const Transform& transform,
            const RigidBody2DComponent* rigidBody,
            const BoxCollider2DComponent* boxCollider,
            const CircleCollider2DComponent* circleCollider,
            bool rejectDynamicParent
        );
        void RemoveEntity(EntityId entity);
        void Prune(const std::vector<EntityId>& activeEntities);

        void SetBodyTransform(EntityId entity, const Transform& transform);
        bool GetBodyTransform(EntityId entity, Transform& transform) const;

        void SetLinearVelocity(EntityId entity, const Vec2& velocity);
        Vec2 GetLinearVelocity(EntityId entity) const;
        void ApplyForce(EntityId entity, const Vec2& force);
        void ApplyImpulse(EntityId entity, const Vec2& impulse);
        void SetAngularVelocity(EntityId entity, float velocity);
        float GetAngularVelocity(EntityId entity) const;

        void Step(double fixedDeltaTime);
        RaycastHit2D Raycast(const Vec2& origin, const Vec2& direction, float maxDistance) const;
        const std::vector<CollisionEvent2D>& GetCollisionEvents() const;
        const std::vector<CollisionEvent2D>& GetTriggerEvents() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    class PhysicsWorld3D
    {
    public:
        PhysicsWorld3D();
        ~PhysicsWorld3D();
        PhysicsWorld3D(PhysicsWorld3D&&) noexcept;
        PhysicsWorld3D& operator=(PhysicsWorld3D&&) noexcept;
        PhysicsWorld3D(const PhysicsWorld3D&) = delete;
        PhysicsWorld3D& operator=(const PhysicsWorld3D&) = delete;

        bool Initialize(Physics3DBackend backend = Physics3DBackend::Jolt);
        void Shutdown();
        void Clear();

        bool IsAvailable() const;
        Physics3DBackend GetBackend() const;
        std::size_t GetBodyCount() const;
        bool HasBody(EntityId entity) const;

        void CreateOrUpdateBody(
            EntityId entity,
            const Transform& transform,
            const RigidBody3DComponent* rigidBody,
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider,
            bool rejectDynamicParent
        );
        void RemoveEntity(EntityId entity);
        void Prune(const std::vector<EntityId>& activeEntities);

        void SetBodyTransform(EntityId entity, const Transform& transform);
        bool GetBodyTransform(EntityId entity, Transform& transform) const;

        void SetLinearVelocity(EntityId entity, const Vec3& velocity);
        Vec3 GetLinearVelocity(EntityId entity) const;
        void ApplyForce(EntityId entity, const Vec3& force);
        void ApplyImpulse(EntityId entity, const Vec3& impulse);
        void SetAngularVelocity(EntityId entity, const Vec3& velocity);
        Vec3 GetAngularVelocity(EntityId entity) const;
        void WakeUp(EntityId entity);

        void Step(double fixedDeltaTime);
        RaycastHit3D Raycast(const Vec3& origin, const Vec3& direction, float maxDistance) const;
        const std::vector<CollisionEvent3D>& GetCollisionEvents() const;
        const std::vector<CollisionEvent3D>& GetTriggerEvents() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
