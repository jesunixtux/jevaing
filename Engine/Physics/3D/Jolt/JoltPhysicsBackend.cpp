#include <Jevaing/Physics.h>

#if defined(JEVAING_HAS_JOLT)
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../Core/Logger.h"

namespace Jevaing
{
    namespace
    {
#if defined(JEVAING_HAS_JOLT)
        namespace Layers
        {
            static constexpr JPH::ObjectLayer NonMoving = 0;
            static constexpr JPH::ObjectLayer Moving = 1;
            static constexpr JPH::ObjectLayer Sensor = 2;
            static constexpr JPH::ObjectLayer Count = 3;
        }

        namespace BroadPhaseLayers
        {
            static constexpr JPH::BroadPhaseLayer NonMoving(0);
            static constexpr JPH::BroadPhaseLayer Moving(1);
            static constexpr JPH::BroadPhaseLayer Sensor(2);
            static constexpr JPH::uint Count = 3;
        }

        std::once_flag g_joltInitOnce;

        void InitializeJoltRuntime()
        {
            std::call_once(
                g_joltInitOnce,
                []
                {
                    JPH::RegisterDefaultAllocator();

                    if (!JPH::Factory::sInstance)
                    {
                        JPH::Factory::sInstance = new JPH::Factory();
                    }

                    JPH::RegisterTypes();
                }
            );
        }

        JPH::Vec3 ToJolt(const Vec3& value)
        {
            return JPH::Vec3(value.X, value.Y, value.Z);
        }

        JPH::RVec3 ToJoltPosition(const Vec3& value)
        {
            return JPH::RVec3(value.X, value.Y, value.Z);
        }

        Vec3 FromJolt(const JPH::Vec3& value)
        {
            return { value.GetX(), value.GetY(), value.GetZ() };
        }

        JPH::Quat ToJoltRotation(const Vec3& radians)
        {
            return JPH::Quat::sEulerAngles(ToJolt(radians));
        }

        Vec3 FromJoltRotation(JPH::QuatArg rotation)
        {
            return FromJolt(rotation.GetEulerAngles());
        }

        float MaxAbsScale(const Vec3& scale)
        {
            return std::max({
                std::fabs(scale.X),
                std::fabs(scale.Y),
                std::fabs(scale.Z)
            });
        }

        bool NearlyEqual(float left, float right)
        {
            return std::fabs(left - right) < 0.0001f;
        }

        bool NearlyEqual(const Vec3& left, const Vec3& right)
        {
            return
                NearlyEqual(left.X, right.X) &&
                NearlyEqual(left.Y, right.Y) &&
                NearlyEqual(left.Z, right.Z);
        }

        JPH::EMotionType ToJoltMotionType(BodyType type)
        {
            switch (type)
            {
                case BodyType::Static:
                    return JPH::EMotionType::Static;
                case BodyType::Kinematic:
                    return JPH::EMotionType::Kinematic;
                case BodyType::Dynamic:
                    return JPH::EMotionType::Dynamic;
            }

            return JPH::EMotionType::Static;
        }

        JPH::ObjectLayer ToJoltLayer(BodyType type, bool isSensor)
        {
            if (isSensor)
            {
                return Layers::Sensor;
            }

            return type == BodyType::Static ? Layers::NonMoving : Layers::Moving;
        }

        std::uint32_t ToKey(const JPH::BodyID& bodyId)
        {
            return bodyId.GetIndexAndSequenceNumber();
        }

        class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
        {
        public:
            bool ShouldCollide(
                JPH::ObjectLayer left,
                JPH::ObjectLayer right
            ) const override
            {
                if (left == Layers::NonMoving)
                {
                    return right == Layers::Moving || right == Layers::Sensor;
                }

                if (left == Layers::Moving)
                {
                    return true;
                }

                if (left == Layers::Sensor)
                {
                    return right == Layers::Moving || right == Layers::NonMoving;
                }

                return false;
            }
        };

        class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
        {
        public:
            BroadPhaseLayerInterfaceImpl()
            {
                m_objectToBroadPhase[Layers::NonMoving] = BroadPhaseLayers::NonMoving;
                m_objectToBroadPhase[Layers::Moving] = BroadPhaseLayers::Moving;
                m_objectToBroadPhase[Layers::Sensor] = BroadPhaseLayers::Sensor;
            }

            JPH::uint GetNumBroadPhaseLayers() const override
            {
                return BroadPhaseLayers::Count;
            }

            JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
            {
                return m_objectToBroadPhase[layer];
            }

            const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
            {
                if (layer == BroadPhaseLayers::NonMoving)
                {
                    return "NonMoving";
                }

                if (layer == BroadPhaseLayers::Moving)
                {
                    return "Moving";
                }

                if (layer == BroadPhaseLayers::Sensor)
                {
                    return "Sensor";
                }

                return "Invalid";
            }

        private:
            JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::Count];
        };

        class ObjectVsBroadPhaseLayerFilterImpl final :
            public JPH::ObjectVsBroadPhaseLayerFilter
        {
        public:
            bool ShouldCollide(
                JPH::ObjectLayer layer,
                JPH::BroadPhaseLayer broadPhaseLayer
            ) const override
            {
                if (layer == Layers::NonMoving)
                {
                    return
                        broadPhaseLayer == BroadPhaseLayers::Moving ||
                        broadPhaseLayer == BroadPhaseLayers::Sensor;
                }

                if (layer == Layers::Moving)
                {
                    return true;
                }

                if (layer == Layers::Sensor)
                {
                    return
                        broadPhaseLayer == BroadPhaseLayers::Moving ||
                        broadPhaseLayer == BroadPhaseLayers::NonMoving;
                }

                return false;
            }
        };
#endif
    }

    struct PhysicsWorld3D::Impl
    {
        struct BodyRecord
        {
            EntityId Entity = InvalidEntityId;
            Transform LastTransform;
            BodyType Type = BodyType::Static;
            float GravityFactor = 1.0f;
            float LinearDamping = 0.0f;
            float AngularDamping = 0.0f;
            bool IsTrigger = false;
            bool HasBox = false;
            bool HasSphere = false;
            bool HasCapsule = false;
            BoxCollider3DComponent Box;
            SphereCollider3DComponent Sphere;
            CapsuleCollider3DComponent Capsule;
#if defined(JEVAING_HAS_JOLT)
            JPH::BodyID BodyId;
#endif
        };

#if defined(JEVAING_HAS_JOLT)
        class ContactListenerImpl final : public JPH::ContactListener
        {
        public:
            explicit ContactListenerImpl(Impl& owner)
                : m_owner(owner)
            {
            }

            JPH::ValidateResult OnContactValidate(
                const JPH::Body&,
                const JPH::Body&,
                JPH::RVec3Arg,
                const JPH::CollideShapeResult&
            ) override
            {
                return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
            }

            void OnContactAdded(
                const JPH::Body& bodyA,
                const JPH::Body& bodyB,
                const JPH::ContactManifold& manifold,
                JPH::ContactSettings&
            ) override
            {
                const EntityId entityA = static_cast<EntityId>(bodyA.GetUserData());
                const EntityId entityB = static_cast<EntityId>(bodyB.GetUserData());
                const bool trigger = bodyA.IsSensor() || bodyB.IsSensor();
                const JPH::RVec3 point =
                    !manifold.mRelativeContactPointsOn1.empty()
                        ? manifold.GetWorldSpaceContactPointOn1(0)
                        : bodyA.GetPosition();

                CollisionEvent3D event;
                event.Type = PhysicsEventType::Enter;
                event.EntityA = entityA;
                event.EntityB = entityB;
                event.Point = FromJolt(point);
                event.Normal = FromJolt(manifold.mWorldSpaceNormal);

                if (trigger)
                {
                    m_owner.TriggerEvents.push_back(event);
                }
                else
                {
                    m_owner.CollisionEvents.push_back(event);
                }
            }

            void OnContactRemoved(const JPH::SubShapeIDPair& pair) override
            {
                const auto left = m_owner.BodyToEntity.find(ToKey(pair.GetBody1ID()));
                const auto right = m_owner.BodyToEntity.find(ToKey(pair.GetBody2ID()));

                if (left == m_owner.BodyToEntity.end() || right == m_owner.BodyToEntity.end())
                {
                    return;
                }

                CollisionEvent3D event;
                event.Type = PhysicsEventType::Exit;
                event.EntityA = left->second;
                event.EntityB = right->second;

                const bool trigger =
                    m_owner.SensorBodies.find(ToKey(pair.GetBody1ID())) != m_owner.SensorBodies.end() ||
                    m_owner.SensorBodies.find(ToKey(pair.GetBody2ID())) != m_owner.SensorBodies.end();

                if (trigger)
                {
                    m_owner.TriggerEvents.push_back(event);
                }
                else
                {
                    m_owner.CollisionEvents.push_back(event);
                }
            }

        private:
            Impl& m_owner;
        };

        BroadPhaseLayerInterfaceImpl BroadPhaseLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl ObjectVsBroadPhaseLayerFilter;
        ObjectLayerPairFilterImpl ObjectLayerPairFilter;
        std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
        std::unique_ptr<JPH::JobSystemSingleThreaded> JobSystem;
        std::unique_ptr<JPH::PhysicsSystem> PhysicsSystem;
        std::unique_ptr<ContactListenerImpl> ContactListener;
#endif

        Physics3DBackend Backend = Physics3DBackend::None;
        bool Available = false;
        std::unordered_map<EntityId, BodyRecord> Bodies;
        std::unordered_map<std::uint32_t, EntityId> BodyToEntity;
        std::unordered_map<std::uint32_t, EntityId> SensorBodies;
        std::unordered_set<EntityId> RejectedParentWarnings;
        std::vector<CollisionEvent3D> CollisionEvents;
        std::vector<CollisionEvent3D> TriggerEvents;

#if defined(JEVAING_HAS_JOLT)
        JPH::BodyInterface& BodyInterface()
        {
            return PhysicsSystem->GetBodyInterface();
        }

        const JPH::BodyInterface& BodyInterface() const
        {
            return PhysicsSystem->GetBodyInterface();
        }

        static JPH::ShapeRefC BuildBaseShape(
            const Transform& transform,
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider
        )
        {
            if (sphereCollider)
            {
                const float radius =
                    std::max(0.001f, sphereCollider->Radius * MaxAbsScale(transform.Scale));
                return new JPH::SphereShape(radius);
            }

            if (capsuleCollider)
            {
                const float radius =
                    std::max(0.001f, capsuleCollider->Radius * MaxAbsScale(transform.Scale));
                const float halfHeight =
                    std::max(0.001f, capsuleCollider->Height * std::fabs(transform.Scale.Y) * 0.5f);
                return new JPH::CapsuleShape(halfHeight, radius);
            }

            const Vec3 size = {
                std::max(0.001f, boxCollider->Size.X * std::fabs(transform.Scale.X) * 0.5f),
                std::max(0.001f, boxCollider->Size.Y * std::fabs(transform.Scale.Y) * 0.5f),
                std::max(0.001f, boxCollider->Size.Z * std::fabs(transform.Scale.Z) * 0.5f)
            };
            return new JPH::BoxShape(ToJolt(size), 0.0f);
        }

        static JPH::ShapeRefC BuildShape(
            const Transform& transform,
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider
        )
        {
            JPH::ShapeRefC baseShape =
                BuildBaseShape(transform, boxCollider, sphereCollider, capsuleCollider);

            Vec3 offset;

            if (sphereCollider)
            {
                offset = sphereCollider->Offset;
            }
            else if (capsuleCollider)
            {
                offset = capsuleCollider->Offset;
            }
            else
            {
                offset = boxCollider->Offset;
            }

            if (NearlyEqual(offset, {}))
            {
                return baseShape;
            }

            JPH::RotatedTranslatedShapeSettings offsetShapeSettings(
                ToJolt(offset),
                JPH::Quat::sIdentity(),
                baseShape
            );
            const JPH::ShapeSettings::ShapeResult result = offsetShapeSettings.Create();

            if (result.HasError())
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics3D][ERROR] Failed to create Jolt offset shape: " +
                    std::string(result.GetError().c_str())
                );
                return baseShape;
            }

            return result.Get();
        }

        static PhysicsMaterial GetMaterial(
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider
        )
        {
            if (sphereCollider)
            {
                return sphereCollider->Material;
            }

            if (capsuleCollider)
            {
                return capsuleCollider->Material;
            }

            return boxCollider->Material;
        }

        static bool IsTrigger(
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider
        )
        {
            return
                (boxCollider && boxCollider->IsTrigger) ||
                (sphereCollider && sphereCollider->IsTrigger) ||
                (capsuleCollider && capsuleCollider->IsTrigger);
        }

        bool NeedsRecreate(
            const BodyRecord& record,
            const Transform& transform,
            BodyType type,
            const BoxCollider3DComponent* boxCollider,
            const SphereCollider3DComponent* sphereCollider,
            const CapsuleCollider3DComponent* capsuleCollider
        ) const
        {
            if (
                record.Type != type ||
                record.HasBox != (boxCollider != nullptr) ||
                record.HasSphere != (sphereCollider != nullptr) ||
                record.HasCapsule != (capsuleCollider != nullptr) ||
                !NearlyEqual(record.LastTransform.Scale, transform.Scale)
            )
            {
                return true;
            }

            if (boxCollider)
            {
                return
                    !NearlyEqual(record.Box.Size, boxCollider->Size) ||
                    !NearlyEqual(record.Box.Offset, boxCollider->Offset) ||
                    record.Box.IsTrigger != boxCollider->IsTrigger;
            }

            if (sphereCollider)
            {
                return
                    !NearlyEqual(record.Sphere.Offset, sphereCollider->Offset) ||
                    !NearlyEqual(record.Sphere.Radius, sphereCollider->Radius) ||
                    record.Sphere.IsTrigger != sphereCollider->IsTrigger;
            }

            if (capsuleCollider)
            {
                return
                    !NearlyEqual(record.Capsule.Offset, capsuleCollider->Offset) ||
                    !NearlyEqual(record.Capsule.Radius, capsuleCollider->Radius) ||
                    !NearlyEqual(record.Capsule.Height, capsuleCollider->Height) ||
                    record.Capsule.IsTrigger != capsuleCollider->IsTrigger;
            }

            return false;
        }

        void ApplyRuntimeProperties(const BodyRecord& record)
        {
            const PhysicsMaterial material =
                GetMaterial(
                    record.HasBox ? &record.Box : nullptr,
                    record.HasSphere ? &record.Sphere : nullptr,
                    record.HasCapsule ? &record.Capsule : nullptr
                );

            BodyInterface().SetFriction(record.BodyId, material.Friction);
            BodyInterface().SetRestitution(record.BodyId, material.Restitution);

            if (record.Type != BodyType::Static)
            {
                BodyInterface().SetGravityFactor(record.BodyId, record.GravityFactor);
            }
        }

        bool CreateJoltBody(BodyRecord& record)
        {
            const JPH::ShapeRefC shape =
                BuildShape(
                    record.LastTransform,
                    record.HasBox ? &record.Box : nullptr,
                    record.HasSphere ? &record.Sphere : nullptr,
                    record.HasCapsule ? &record.Capsule : nullptr
                );

            JPH::BodyCreationSettings settings(
                shape,
                ToJoltPosition(record.LastTransform.Position),
                ToJoltRotation(record.LastTransform.Rotation),
                ToJoltMotionType(record.Type),
                ToJoltLayer(record.Type, record.IsTrigger)
            );
            settings.mUserData = record.Entity;
            settings.mIsSensor = record.IsTrigger;
            settings.mCollideKinematicVsNonDynamic = true;
            settings.mFriction =
                GetMaterial(
                    record.HasBox ? &record.Box : nullptr,
                    record.HasSphere ? &record.Sphere : nullptr,
                    record.HasCapsule ? &record.Capsule : nullptr
                ).Friction;
            settings.mRestitution =
                GetMaterial(
                    record.HasBox ? &record.Box : nullptr,
                    record.HasSphere ? &record.Sphere : nullptr,
                    record.HasCapsule ? &record.Capsule : nullptr
                ).Restitution;
            settings.mLinearDamping = record.LinearDamping;
            settings.mAngularDamping = record.AngularDamping;
            settings.mGravityFactor = record.GravityFactor;

            record.BodyId =
                BodyInterface().CreateAndAddBody(
                    settings,
                    record.Type == BodyType::Static
                        ? JPH::EActivation::DontActivate
                        : JPH::EActivation::Activate
                );

            if (record.BodyId.IsInvalid())
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics3D][ERROR] Jolt failed to create body for Entity " +
                    std::to_string(record.Entity)
                );
                return false;
            }

            BodyToEntity[ToKey(record.BodyId)] = record.Entity;

            if (record.IsTrigger)
            {
                SensorBodies[ToKey(record.BodyId)] = record.Entity;
            }

            return true;
        }

        void DestroyJoltBody(const BodyRecord& record)
        {
            if (record.BodyId.IsInvalid())
            {
                return;
            }

            BodyToEntity.erase(ToKey(record.BodyId));
            SensorBodies.erase(ToKey(record.BodyId));

            if (BodyInterface().IsAdded(record.BodyId))
            {
                BodyInterface().RemoveBody(record.BodyId);
            }

            BodyInterface().DestroyBody(record.BodyId);
        }
#endif
    };

    PhysicsWorld3D::PhysicsWorld3D()
        : m_impl(std::make_unique<Impl>())
    {
    }

    PhysicsWorld3D::~PhysicsWorld3D() = default;
    PhysicsWorld3D::PhysicsWorld3D(PhysicsWorld3D&&) noexcept = default;
    PhysicsWorld3D& PhysicsWorld3D::operator=(PhysicsWorld3D&&) noexcept = default;

    bool PhysicsWorld3D::Initialize(Physics3DBackend backend)
    {
#if !defined(JEVAING_HAS_JOLT)
        if (backend == Physics3DBackend::Jolt)
        {
            m_impl->Backend = Physics3DBackend::None;
            m_impl->Available = false;
            return false;
        }
#else
        if (backend == Physics3DBackend::Jolt && !m_impl->PhysicsSystem)
        {
            InitializeJoltRuntime();

            m_impl->TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
            m_impl->JobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(2048);
            m_impl->PhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
            m_impl->ContactListener = std::make_unique<Impl::ContactListenerImpl>(*m_impl);

            constexpr JPH::uint MaxBodies = 4096;
            constexpr JPH::uint NumBodyMutexes = 0;
            constexpr JPH::uint MaxBodyPairs = 8192;
            constexpr JPH::uint MaxContactConstraints = 8192;

            m_impl->PhysicsSystem->Init(
                MaxBodies,
                NumBodyMutexes,
                MaxBodyPairs,
                MaxContactConstraints,
                m_impl->BroadPhaseLayerInterface,
                m_impl->ObjectVsBroadPhaseLayerFilter,
                m_impl->ObjectLayerPairFilter
            );
            m_impl->PhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
            m_impl->PhysicsSystem->SetContactListener(m_impl->ContactListener.get());
        }
#endif

        m_impl->Backend = backend;
        m_impl->Available = backend != Physics3DBackend::None;

        if (m_impl->Available)
        {
            Internal::Logger::Info("[Jevaing][Physics3D][INFO] Jolt backend initialized.");
        }

        return m_impl->Available || backend == Physics3DBackend::None;
    }

    void PhysicsWorld3D::Shutdown()
    {
        Clear();
        m_impl->Available = false;
        m_impl->Backend = Physics3DBackend::None;
#if defined(JEVAING_HAS_JOLT)
        if (m_impl->PhysicsSystem)
        {
            m_impl->PhysicsSystem->SetContactListener(nullptr);
        }
        m_impl->ContactListener.reset();
        m_impl->PhysicsSystem.reset();
        m_impl->JobSystem.reset();
        m_impl->TempAllocator.reset();
#endif
    }

    void PhysicsWorld3D::Clear()
    {
#if defined(JEVAING_HAS_JOLT)
        if (m_impl->PhysicsSystem)
        {
            for (const auto& item : m_impl->Bodies)
            {
                m_impl->DestroyJoltBody(item.second);
            }
        }
#endif
        m_impl->Bodies.clear();
        m_impl->BodyToEntity.clear();
        m_impl->SensorBodies.clear();
        m_impl->RejectedParentWarnings.clear();
        m_impl->CollisionEvents.clear();
        m_impl->TriggerEvents.clear();
    }

    bool PhysicsWorld3D::IsAvailable() const
    {
        return m_impl->Available;
    }

    Physics3DBackend PhysicsWorld3D::GetBackend() const
    {
        return m_impl->Backend;
    }

    std::size_t PhysicsWorld3D::GetBodyCount() const
    {
        return m_impl->Bodies.size();
    }

    bool PhysicsWorld3D::HasBody(EntityId entity) const
    {
        return m_impl->Bodies.find(entity) != m_impl->Bodies.end();
    }

    void PhysicsWorld3D::CreateOrUpdateBody(
        EntityId entity,
        const Transform& transform,
        const RigidBody3DComponent* rigidBody,
        const BoxCollider3DComponent* boxCollider,
        const SphereCollider3DComponent* sphereCollider,
        const CapsuleCollider3DComponent* capsuleCollider,
        bool rejectDynamicParent
    )
    {
        if (
            !m_impl->Available ||
            entity == InvalidEntityId ||
            (!boxCollider && !sphereCollider && !capsuleCollider)
        )
        {
            return;
        }

        if (rigidBody && !rigidBody->Enabled)
        {
            RemoveEntity(entity);
            return;
        }

        const BodyType type = rigidBody ? rigidBody->Type : BodyType::Static;

        if (type == BodyType::Dynamic && rejectDynamicParent)
        {
            if (m_impl->RejectedParentWarnings.insert(entity).second)
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics3D][ERROR] Entity " +
                    std::to_string(entity) +
                    " has Dynamic RigidBody3D with unsupported parent relationship."
                );
            }

            RemoveEntity(entity);
            return;
        }

        m_impl->RejectedParentWarnings.erase(entity);

#if defined(JEVAING_HAS_JOLT)
        const bool isTrigger =
            Impl::IsTrigger(boxCollider, sphereCollider, capsuleCollider);

        auto found = m_impl->Bodies.find(entity);
        const bool needsCreate = found == m_impl->Bodies.end();
        const bool needsRecreate =
            !needsCreate &&
            m_impl->NeedsRecreate(
                found->second,
                transform,
                type,
                boxCollider,
                sphereCollider,
                capsuleCollider
            );

        if (needsRecreate)
        {
            m_impl->DestroyJoltBody(found->second);
            found = m_impl->Bodies.erase(found);
        }

        if (found == m_impl->Bodies.end())
        {
            Impl::BodyRecord record;
            record.Entity = entity;
            record.LastTransform = transform;
            record.Type = type;
            record.GravityFactor = rigidBody ? rigidBody->GravityFactor : 0.0f;
            record.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
            record.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;
            record.IsTrigger = isTrigger;
            record.HasBox = boxCollider != nullptr;
            record.HasSphere = sphereCollider != nullptr;
            record.HasCapsule = capsuleCollider != nullptr;

            if (boxCollider)
            {
                record.Box = *boxCollider;
            }

            if (sphereCollider)
            {
                record.Sphere = *sphereCollider;
            }

            if (capsuleCollider)
            {
                record.Capsule = *capsuleCollider;
            }

            if (m_impl->CreateJoltBody(record))
            {
                m_impl->Bodies[entity] = record;
            }

            return;
        }

        Impl::BodyRecord& record = found->second;
        record.LastTransform.Scale = transform.Scale;
        record.GravityFactor = rigidBody ? rigidBody->GravityFactor : 0.0f;
        record.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
        record.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;

        if (record.Type != BodyType::Dynamic)
        {
            record.LastTransform.Position = transform.Position;
            record.LastTransform.Rotation = transform.Rotation;
            m_impl->BodyInterface().SetPositionAndRotation(
                record.BodyId,
                ToJoltPosition(transform.Position),
                ToJoltRotation(transform.Rotation),
                record.Type == BodyType::Kinematic
                    ? JPH::EActivation::Activate
                    : JPH::EActivation::DontActivate
            );
        }

        if (record.Type == BodyType::Kinematic)
        {
            m_impl->BodyInterface().SetLinearAndAngularVelocity(
                record.BodyId,
                JPH::Vec3::sZero(),
                JPH::Vec3::sZero()
            );
        }

        m_impl->BodyInterface().SetLinearVelocity(
            record.BodyId,
            m_impl->BodyInterface().GetLinearVelocity(record.BodyId)
        );
        m_impl->BodyInterface().SetAngularVelocity(
            record.BodyId,
            m_impl->BodyInterface().GetAngularVelocity(record.BodyId)
        );
        m_impl->ApplyRuntimeProperties(record);
#else
        (void)transform;
#endif
    }

    void PhysicsWorld3D::RemoveEntity(EntityId entity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        if (m_impl->PhysicsSystem)
        {
            m_impl->DestroyJoltBody(found->second);
        }
#endif

        m_impl->Bodies.erase(found);
        m_impl->RejectedParentWarnings.erase(entity);
    }

    void PhysicsWorld3D::Prune(const std::vector<EntityId>& activeEntities)
    {
        std::vector<EntityId> toRemove;

        for (const auto& item : m_impl->Bodies)
        {
            if (
                std::find(
                    activeEntities.begin(),
                    activeEntities.end(),
                    item.first
                ) == activeEntities.end()
            )
            {
                toRemove.push_back(item.first);
            }
        }

        for (EntityId entity : toRemove)
        {
            RemoveEntity(entity);
        }
    }

    void PhysicsWorld3D::SetBodyTransform(EntityId entity, const Transform& transform)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

        found->second.LastTransform.Position = transform.Position;
        found->second.LastTransform.Rotation = transform.Rotation;
        found->second.LastTransform.Scale = transform.Scale;

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().SetPositionAndRotation(
            found->second.BodyId,
            ToJoltPosition(transform.Position),
            ToJoltRotation(transform.Rotation),
            JPH::EActivation::Activate
        );
#endif
    }

    bool PhysicsWorld3D::GetBodyTransform(EntityId entity, Transform& transform) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return false;
        }

        transform = found->second.LastTransform;

#if defined(JEVAING_HAS_JOLT)
        JPH::RVec3 position;
        JPH::Quat rotation;
        m_impl->BodyInterface().GetPositionAndRotation(
            found->second.BodyId,
            position,
            rotation
        );
        transform.Position = FromJolt(position);
        transform.Rotation = FromJoltRotation(rotation);
#endif

        return true;
    }

    void PhysicsWorld3D::SetLinearVelocity(EntityId entity, const Vec3& velocity)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().SetLinearVelocity(found->second.BodyId, ToJolt(velocity));
#else
        (void)velocity;
#endif
    }

    Vec3 PhysicsWorld3D::GetLinearVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return {};
        }

#if defined(JEVAING_HAS_JOLT)
        return FromJolt(m_impl->BodyInterface().GetLinearVelocity(found->second.BodyId));
#else
        return {};
#endif
    }

    void PhysicsWorld3D::ApplyForce(EntityId entity, const Vec3& force)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().AddForce(found->second.BodyId, ToJolt(force));
#else
        (void)force;
#endif
    }

    void PhysicsWorld3D::ApplyImpulse(EntityId entity, const Vec3& impulse)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().AddImpulse(found->second.BodyId, ToJolt(impulse));
#else
        (void)impulse;
#endif
    }

    void PhysicsWorld3D::SetAngularVelocity(EntityId entity, const Vec3& velocity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().SetAngularVelocity(found->second.BodyId, ToJolt(velocity));
#else
        (void)velocity;
#endif
    }

    Vec3 PhysicsWorld3D::GetAngularVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return {};
        }

#if defined(JEVAING_HAS_JOLT)
        return FromJolt(m_impl->BodyInterface().GetAngularVelocity(found->second.BodyId));
#else
        return {};
#endif
    }

    void PhysicsWorld3D::WakeUp(EntityId entity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        m_impl->BodyInterface().ActivateBody(found->second.BodyId);
#endif
    }

    void PhysicsWorld3D::Step(double fixedDeltaTime)
    {
        m_impl->CollisionEvents.clear();
        m_impl->TriggerEvents.clear();

        if (!m_impl->Available)
        {
            return;
        }

#if defined(JEVAING_HAS_JOLT)
        if (!m_impl->PhysicsSystem)
        {
            return;
        }

        m_impl->PhysicsSystem->Update(
            static_cast<float>(fixedDeltaTime),
            1,
            m_impl->TempAllocator.get(),
            m_impl->JobSystem.get()
        );

        for (auto& item : m_impl->Bodies)
        {
            Impl::BodyRecord& record = item.second;
            JPH::RVec3 position;
            JPH::Quat rotation;

            m_impl->BodyInterface().GetPositionAndRotation(
                record.BodyId,
                position,
                rotation
            );
            record.LastTransform.Position = FromJolt(position);
            record.LastTransform.Rotation = FromJoltRotation(rotation);
        }
#else
        (void)fixedDeltaTime;
#endif
    }

    RaycastHit3D PhysicsWorld3D::Raycast(
        const Vec3& origin,
        const Vec3& direction,
        float maxDistance
    ) const
    {
        RaycastHit3D result;

        if (!m_impl->Available || maxDistance <= 0.0f)
        {
            return result;
        }

#if defined(JEVAING_HAS_JOLT)
        const Vec3 normalized = Normalize(direction);

        if (Length(normalized) <= 0.000001f)
        {
            return result;
        }

        JPH::RRayCast ray(
            ToJoltPosition(origin),
            ToJolt(normalized * maxDistance)
        );
        JPH::RayCastResult hit;

        if (!m_impl->PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit))
        {
            return result;
        }

        const auto mappedEntity = m_impl->BodyToEntity.find(ToKey(hit.mBodyID));

        if (mappedEntity == m_impl->BodyToEntity.end())
        {
            return result;
        }

        result.Hit = true;
        result.Entity = mappedEntity->second;
        result.Distance = hit.mFraction * maxDistance;
        result.Point = origin + normalized * result.Distance;

        JPH::BodyLockRead lock(
            m_impl->PhysicsSystem->GetBodyLockInterface(),
            hit.mBodyID
        );

        if (lock.Succeeded())
        {
            result.Normal =
                FromJolt(
                    lock.GetBody().GetWorldSpaceSurfaceNormal(
                        hit.mSubShapeID2,
                        ToJoltPosition(result.Point)
                    )
                );
        }
#else
        (void)origin;
        (void)direction;
#endif

        return result;
    }

    const std::vector<CollisionEvent3D>& PhysicsWorld3D::GetCollisionEvents() const
    {
        return m_impl->CollisionEvents;
    }

    const std::vector<CollisionEvent3D>& PhysicsWorld3D::GetTriggerEvents() const
    {
        return m_impl->TriggerEvents;
    }
}
