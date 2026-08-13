#include <Jevaing/Physics.h>

#if defined(JEVAING_HAS_BOX2D)
#include <box2d/box2d.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../../Core/Logger.h"

namespace Jevaing
{
    namespace
    {
        bool NearlyEqual(float left, float right)
        {
            return std::fabs(left - right) < 0.0001f;
        }

        bool NearlyEqual(const Vec2& left, const Vec2& right)
        {
            return
                NearlyEqual(left.X, right.X) &&
                NearlyEqual(left.Y, right.Y);
        }

#if defined(JEVAING_HAS_BOX2D)
        b2Vec2 ToBox2D(const Vec2& value)
        {
            return { value.X, value.Y };
        }

        b2Vec2 ToBox2DPosition(const Transform& transform)
        {
            return { transform.Position.X, transform.Position.Y };
        }

        Vec2 FromBox2D(const b2Vec2& value)
        {
            return { value.x, value.y };
        }

        b2BodyType ToBox2DBodyType(BodyType type)
        {
            switch (type)
            {
                case BodyType::Static:
                    return b2_staticBody;
                case BodyType::Kinematic:
                    return b2_kinematicBody;
                case BodyType::Dynamic:
                    return b2_dynamicBody;
            }

            return b2_staticBody;
        }

        float MaxAbsScale2D(const Transform& transform)
        {
            return std::max(
                std::fabs(transform.Scale.X),
                std::fabs(transform.Scale.Y)
            );
        }

        std::uint64_t ToKey(const b2BodyId& bodyId)
        {
            return b2StoreBodyId(bodyId);
        }

        std::uint64_t ToKey(const b2ShapeId& shapeId)
        {
            return b2StoreShapeId(shapeId);
        }

        b2ShapeDef MakeShapeDef(const PhysicsMaterial& material, bool isTrigger)
        {
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = std::max(0.0f, material.Density);
            shapeDef.material.friction = std::max(0.0f, material.Friction);
            shapeDef.material.restitution = std::max(0.0f, material.Restitution);
            shapeDef.isSensor = isTrigger;
            shapeDef.enableSensorEvents = true;
            shapeDef.enableContactEvents = !isTrigger;
            return shapeDef;
        }
#endif
    }

    struct PhysicsWorld2D::Impl
    {
        struct BodyRecord
        {
            EntityId Entity = InvalidEntityId;
            Transform LastTransform;
            BodyType Type = BodyType::Static;
            bool Enabled = true;
            float GravityScale = 1.0f;
            float LinearDamping = 0.0f;
            float AngularDamping = 0.0f;
            bool HasBox = false;
            bool HasCircle = false;
            BoxCollider2DComponent Box;
            CircleCollider2DComponent Circle;

#if defined(JEVAING_HAS_BOX2D)
            b2BodyId BodyId = b2_nullBodyId;
            b2ShapeId BoxShapeId = b2_nullShapeId;
            b2ShapeId CircleShapeId = b2_nullShapeId;
#endif
        };

        Physics2DBackend Backend = Physics2DBackend::None;
        bool Available = false;
        Vec2 Gravity = { 0.0f, -9.81f };
        std::unordered_map<EntityId, BodyRecord> Bodies;
        std::unordered_set<EntityId> RejectedParentWarnings;
        std::vector<CollisionEvent2D> CollisionEvents;
        std::vector<CollisionEvent2D> TriggerEvents;

#if defined(JEVAING_HAS_BOX2D)
        b2WorldId WorldId = b2_nullWorldId;
        std::unordered_map<std::uint64_t, EntityId> BodyToEntity;
        std::unordered_map<std::uint64_t, EntityId> ShapeToEntity;

        bool HasWorld() const
        {
            return B2_IS_NON_NULL(WorldId);
        }

        void CreateWorld()
        {
            if (HasWorld())
            {
                return;
            }

            b2WorldDef worldDef = b2DefaultWorldDef();
            worldDef.gravity = ToBox2D(Gravity);
            WorldId = b2CreateWorld(&worldDef);
        }

        void DestroyWorld()
        {
            if (HasWorld())
            {
                b2DestroyWorld(WorldId);
                WorldId = b2_nullWorldId;
            }

            BodyToEntity.clear();
            ShapeToEntity.clear();
        }

        bool TryGetEntity(b2ShapeId shapeId, EntityId& entity) const
        {
            const auto found = ShapeToEntity.find(ToKey(shapeId));

            if (found == ShapeToEntity.end())
            {
                return false;
            }

            entity = found->second;
            return true;
        }

        bool BodyExists(const BodyRecord& record) const
        {
            return B2_IS_NON_NULL(record.BodyId) && b2Body_IsValid(record.BodyId);
        }

        bool NeedsRecreate(
            const BodyRecord& record,
            const Transform& transform,
            BodyType type,
            const BoxCollider2DComponent* boxCollider,
            const CircleCollider2DComponent* circleCollider
        ) const
        {
            if (
                record.Type != type ||
                record.HasBox != (boxCollider != nullptr) ||
                record.HasCircle != (circleCollider != nullptr) ||
                !NearlyEqual(
                    { record.LastTransform.Scale.X, record.LastTransform.Scale.Y },
                    { transform.Scale.X, transform.Scale.Y }
                )
            )
            {
                return true;
            }

            if (boxCollider)
            {
                if (
                    !NearlyEqual(record.Box.Offset, boxCollider->Offset) ||
                    !NearlyEqual(record.Box.Size, boxCollider->Size) ||
                    record.Box.IsTrigger != boxCollider->IsTrigger ||
                    !NearlyEqual(record.Box.Material.Friction, boxCollider->Material.Friction) ||
                    !NearlyEqual(record.Box.Material.Restitution, boxCollider->Material.Restitution) ||
                    !NearlyEqual(record.Box.Material.Density, boxCollider->Material.Density)
                )
                {
                    return true;
                }
            }

            if (circleCollider)
            {
                if (
                    !NearlyEqual(record.Circle.Offset, circleCollider->Offset) ||
                    !NearlyEqual(record.Circle.Radius, circleCollider->Radius) ||
                    record.Circle.IsTrigger != circleCollider->IsTrigger ||
                    !NearlyEqual(record.Circle.Material.Friction, circleCollider->Material.Friction) ||
                    !NearlyEqual(record.Circle.Material.Restitution, circleCollider->Material.Restitution) ||
                    !NearlyEqual(record.Circle.Material.Density, circleCollider->Material.Density)
                )
                {
                    return true;
                }
            }

            return false;
        }

        void DestroyBox2DBody(const BodyRecord& record)
        {
            if (!BodyExists(record))
            {
                return;
            }

            if (B2_IS_NON_NULL(record.BoxShapeId))
            {
                ShapeToEntity.erase(ToKey(record.BoxShapeId));
            }

            if (B2_IS_NON_NULL(record.CircleShapeId))
            {
                ShapeToEntity.erase(ToKey(record.CircleShapeId));
            }

            BodyToEntity.erase(ToKey(record.BodyId));
            b2DestroyBody(record.BodyId);
        }

        bool CreateBox2DBody(BodyRecord& record)
        {
            if (!HasWorld())
            {
                return false;
            }

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = ToBox2DBodyType(record.Type);
            bodyDef.position = ToBox2DPosition(record.LastTransform);
            bodyDef.rotation = b2MakeRot(record.LastTransform.Rotation.Z);
            bodyDef.gravityScale = record.GravityScale;
            bodyDef.linearDamping = record.LinearDamping;
            bodyDef.angularDamping = record.AngularDamping;
            bodyDef.isEnabled = record.Enabled;

            record.BodyId = b2CreateBody(WorldId, &bodyDef);

            if (B2_IS_NULL(record.BodyId))
            {
                return false;
            }

            BodyToEntity[ToKey(record.BodyId)] = record.Entity;

            bool createdShape = false;

            if (record.HasBox)
            {
                const float halfWidth =
                    std::max(0.0001f, record.Box.Size.X * std::fabs(record.LastTransform.Scale.X) * 0.5f);
                const float halfHeight =
                    std::max(0.0001f, record.Box.Size.Y * std::fabs(record.LastTransform.Scale.Y) * 0.5f);
                const b2Polygon polygon =
                    b2MakeOffsetBox(
                        halfWidth,
                        halfHeight,
                        ToBox2D(record.Box.Offset),
                        b2Rot_identity
                    );
                const b2ShapeDef shapeDef =
                    MakeShapeDef(record.Box.Material, record.Box.IsTrigger);

                record.BoxShapeId =
                    b2CreatePolygonShape(record.BodyId, &shapeDef, &polygon);

                if (B2_IS_NON_NULL(record.BoxShapeId))
                {
                    ShapeToEntity[ToKey(record.BoxShapeId)] = record.Entity;
                    createdShape = true;
                }
            }

            if (record.HasCircle)
            {
                b2Circle circle = {};
                circle.center = ToBox2D(record.Circle.Offset);
                circle.radius =
                    std::max(0.0001f, record.Circle.Radius * MaxAbsScale2D(record.LastTransform));
                const b2ShapeDef shapeDef =
                    MakeShapeDef(record.Circle.Material, record.Circle.IsTrigger);

                record.CircleShapeId =
                    b2CreateCircleShape(record.BodyId, &shapeDef, &circle);

                if (B2_IS_NON_NULL(record.CircleShapeId))
                {
                    ShapeToEntity[ToKey(record.CircleShapeId)] = record.Entity;
                    createdShape = true;
                }
            }

            if (!createdShape)
            {
                DestroyBox2DBody(record);
                record.BodyId = b2_nullBodyId;
                return false;
            }

            return true;
        }

        void ApplyRuntimeProperties(const BodyRecord& record)
        {
            if (!BodyExists(record))
            {
                return;
            }

            b2Body_SetLinearDamping(record.BodyId, record.LinearDamping);
            b2Body_SetAngularDamping(record.BodyId, record.AngularDamping);
            b2Body_SetGravityScale(record.BodyId, record.GravityScale);

            const bool enabled = b2Body_IsEnabled(record.BodyId);

            if (record.Enabled && !enabled)
            {
                b2Body_Enable(record.BodyId);
            }
            else if (!record.Enabled && enabled)
            {
                b2Body_Disable(record.BodyId);
            }
        }

        void SyncTransformFromBox2D(BodyRecord& record)
        {
            if (!BodyExists(record))
            {
                return;
            }

            const b2Transform transform = b2Body_GetTransform(record.BodyId);
            record.LastTransform.Position.X = transform.p.x;
            record.LastTransform.Position.Y = transform.p.y;
            record.LastTransform.Rotation.Z = b2Rot_GetAngle(transform.q);
        }

        CollisionEvent2D MakeContactEvent(
            PhysicsEventType type,
            b2ShapeId shapeA,
            b2ShapeId shapeB,
            const b2Manifold* manifold = nullptr
        ) const
        {
            CollisionEvent2D event;
            event.Type = type;

            TryGetEntity(shapeA, event.EntityA);
            TryGetEntity(shapeB, event.EntityB);

            if (manifold)
            {
                event.Normal = FromBox2D(manifold->normal);

                if (manifold->pointCount > 0)
                {
                    event.Point = FromBox2D(manifold->points[0].point);
                }
            }

            return event;
        }
#endif
    };

    PhysicsWorld2D::PhysicsWorld2D()
        : m_impl(std::make_unique<Impl>())
    {
    }

    PhysicsWorld2D::~PhysicsWorld2D()
    {
        if (m_impl)
        {
            Shutdown();
        }
    }

    PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2D&&) noexcept = default;
    PhysicsWorld2D& PhysicsWorld2D::operator=(PhysicsWorld2D&&) noexcept = default;

    bool PhysicsWorld2D::Initialize(Physics2DBackend backend)
    {
#if !defined(JEVAING_HAS_BOX2D)
        if (backend == Physics2DBackend::Box2D)
        {
            m_impl->Backend = Physics2DBackend::None;
            m_impl->Available = false;
            return false;
        }
#endif

        m_impl->Backend = backend;
        m_impl->Available = backend != Physics2DBackend::None;

#if defined(JEVAING_HAS_BOX2D)
        if (backend == Physics2DBackend::Box2D)
        {
            m_impl->CreateWorld();
            m_impl->Available = m_impl->HasWorld();
        }
#endif

        if (m_impl->Available)
        {
            Internal::Logger::Info("[Jevaing][Physics2D][INFO] Box2D backend initialized.");
        }

        return m_impl->Available || backend == Physics2DBackend::None;
    }

    void PhysicsWorld2D::Shutdown()
    {
        Clear();

#if defined(JEVAING_HAS_BOX2D)
        m_impl->DestroyWorld();
#endif

        m_impl->Available = false;
        m_impl->Backend = Physics2DBackend::None;
    }

    void PhysicsWorld2D::Clear()
    {
#if defined(JEVAING_HAS_BOX2D)
        m_impl->DestroyWorld();

        if (m_impl->Backend == Physics2DBackend::Box2D && m_impl->Available)
        {
            m_impl->CreateWorld();
        }
#endif

        m_impl->Bodies.clear();
        m_impl->RejectedParentWarnings.clear();
        m_impl->CollisionEvents.clear();
        m_impl->TriggerEvents.clear();
    }

    bool PhysicsWorld2D::IsAvailable() const
    {
        return m_impl->Available;
    }

    Physics2DBackend PhysicsWorld2D::GetBackend() const
    {
        return m_impl->Backend;
    }

    std::size_t PhysicsWorld2D::GetBodyCount() const
    {
        return m_impl->Bodies.size();
    }

    bool PhysicsWorld2D::HasBody(EntityId entity) const
    {
        return m_impl->Bodies.find(entity) != m_impl->Bodies.end();
    }

    void PhysicsWorld2D::CreateOrUpdateBody(
        EntityId entity,
        const Transform& transform,
        const RigidBody2DComponent* rigidBody,
        const BoxCollider2DComponent* boxCollider,
        const CircleCollider2DComponent* circleCollider,
        bool rejectDynamicParent
    )
    {
        if (!m_impl->Available || entity == InvalidEntityId || (!boxCollider && !circleCollider))
        {
            return;
        }

        const BodyType type = rigidBody ? rigidBody->Type : BodyType::Static;

        if (type == BodyType::Dynamic && rejectDynamicParent)
        {
            if (m_impl->RejectedParentWarnings.insert(entity).second)
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics2D][ERROR] Entity " +
                    std::to_string(entity) +
                    " has Dynamic RigidBody2D with unsupported parent relationship."
                );
            }

            RemoveEntity(entity);
            return;
        }

        m_impl->RejectedParentWarnings.erase(entity);

#if defined(JEVAING_HAS_BOX2D)
        auto found = m_impl->Bodies.find(entity);
        const bool needsCreate = found == m_impl->Bodies.end();
        const bool needsRecreate =
            !needsCreate &&
            m_impl->NeedsRecreate(
                found->second,
                transform,
                type,
                boxCollider,
                circleCollider
            );

        if (needsRecreate)
        {
            m_impl->DestroyBox2DBody(found->second);
            found = m_impl->Bodies.erase(found);
        }

        if (found == m_impl->Bodies.end())
        {
            Impl::BodyRecord record;
            record.Entity = entity;
            record.LastTransform = transform;
            record.Type = type;
            record.Enabled = !rigidBody || rigidBody->Enabled;
            record.GravityScale = rigidBody ? rigidBody->GravityScale : 0.0f;
            record.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
            record.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;
            record.HasBox = boxCollider != nullptr;
            record.HasCircle = circleCollider != nullptr;

            if (boxCollider)
            {
                record.Box = *boxCollider;
            }

            if (circleCollider)
            {
                record.Circle = *circleCollider;
            }

            if (m_impl->CreateBox2DBody(record))
            {
                m_impl->Bodies[entity] = record;
            }

            return;
        }

        Impl::BodyRecord& record = found->second;
        record.LastTransform.Scale = transform.Scale;
        record.Enabled = !rigidBody || rigidBody->Enabled;
        record.GravityScale = rigidBody ? rigidBody->GravityScale : 0.0f;
        record.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
        record.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;

        if (record.Type != BodyType::Dynamic)
        {
            record.LastTransform.Position = transform.Position;
            record.LastTransform.Rotation = transform.Rotation;
            b2Body_SetTransform(
                record.BodyId,
                ToBox2DPosition(transform),
                b2MakeRot(transform.Rotation.Z)
            );
        }

        m_impl->ApplyRuntimeProperties(record);
#else
        (void)transform;
        (void)rigidBody;
        (void)boxCollider;
        (void)circleCollider;
#endif
    }

    void PhysicsWorld2D::RemoveEntity(EntityId entity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        m_impl->DestroyBox2DBody(found->second);
#endif

        m_impl->Bodies.erase(found);
    }

    void PhysicsWorld2D::Prune(const std::vector<EntityId>& activeEntities)
    {
        std::unordered_set<EntityId> active(activeEntities.begin(), activeEntities.end());

        for (auto iterator = m_impl->Bodies.begin(); iterator != m_impl->Bodies.end();)
        {
            if (active.find(iterator->first) == active.end())
            {
#if defined(JEVAING_HAS_BOX2D)
                m_impl->DestroyBox2DBody(iterator->second);
#endif
                iterator = m_impl->Bodies.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }
    }

    void PhysicsWorld2D::SetBodyTransform(EntityId entity, const Transform& transform)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

        found->second.LastTransform = transform;

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            b2Body_SetTransform(
                found->second.BodyId,
                ToBox2DPosition(transform),
                b2MakeRot(transform.Rotation.Z)
            );
        }
#endif
    }

    bool PhysicsWorld2D::GetBodyTransform(EntityId entity, Transform& transform) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return false;
        }

        transform = found->second.LastTransform;

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            const b2Transform bodyTransform = b2Body_GetTransform(found->second.BodyId);
            transform.Position.X = bodyTransform.p.x;
            transform.Position.Y = bodyTransform.p.y;
            transform.Rotation.Z = b2Rot_GetAngle(bodyTransform.q);
        }
#endif

        return true;
    }

    void PhysicsWorld2D::SetLinearVelocity(EntityId entity, const Vec2& velocity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            b2Body_SetLinearVelocity(found->second.BodyId, ToBox2D(velocity));
        }
#else
        (void)velocity;
#endif
    }

    Vec2 PhysicsWorld2D::GetLinearVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return {};
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            return FromBox2D(b2Body_GetLinearVelocity(found->second.BodyId));
        }
#endif

        return {};
    }

    void PhysicsWorld2D::ApplyForce(EntityId entity, const Vec2& force)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            b2Body_ApplyForceToCenter(found->second.BodyId, ToBox2D(force), true);
        }
#else
        (void)force;
#endif
    }

    void PhysicsWorld2D::ApplyImpulse(EntityId entity, const Vec2& impulse)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            b2Body_ApplyLinearImpulseToCenter(found->second.BodyId, ToBox2D(impulse), true);
        }
#else
        (void)impulse;
#endif
    }

    void PhysicsWorld2D::SetAngularVelocity(EntityId entity, float velocity)
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            b2Body_SetAngularVelocity(found->second.BodyId, velocity);
        }
#else
        (void)velocity;
#endif
    }

    float PhysicsWorld2D::GetAngularVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return 0.0f;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (m_impl->BodyExists(found->second))
        {
            return b2Body_GetAngularVelocity(found->second.BodyId);
        }
#endif

        return 0.0f;
    }

    void PhysicsWorld2D::Step(double fixedDeltaTime)
    {
        m_impl->CollisionEvents.clear();
        m_impl->TriggerEvents.clear();

        if (!m_impl->Available)
        {
            return;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (!m_impl->HasWorld())
        {
            return;
        }

        b2World_Step(m_impl->WorldId, static_cast<float>(fixedDeltaTime), 4);

        const b2ContactEvents contacts = b2World_GetContactEvents(m_impl->WorldId);

        for (int index = 0; index < contacts.beginCount; ++index)
        {
            const b2ContactBeginTouchEvent& begin = contacts.beginEvents[index];
            const CollisionEvent2D event =
                m_impl->MakeContactEvent(
                    PhysicsEventType::Enter,
                    begin.shapeIdA,
                    begin.shapeIdB,
                    &begin.manifold
                );

            if (event.EntityA != InvalidEntityId && event.EntityB != InvalidEntityId)
            {
                m_impl->CollisionEvents.push_back(event);
            }
        }

        for (int index = 0; index < contacts.endCount; ++index)
        {
            const b2ContactEndTouchEvent& end = contacts.endEvents[index];
            const CollisionEvent2D event =
                m_impl->MakeContactEvent(
                    PhysicsEventType::Exit,
                    end.shapeIdA,
                    end.shapeIdB
                );

            if (event.EntityA != InvalidEntityId && event.EntityB != InvalidEntityId)
            {
                m_impl->CollisionEvents.push_back(event);
            }
        }

        const b2SensorEvents sensors = b2World_GetSensorEvents(m_impl->WorldId);

        for (int index = 0; index < sensors.beginCount; ++index)
        {
            const b2SensorBeginTouchEvent& begin = sensors.beginEvents[index];
            const CollisionEvent2D event =
                m_impl->MakeContactEvent(
                    PhysicsEventType::Enter,
                    begin.sensorShapeId,
                    begin.visitorShapeId
                );

            if (event.EntityA != InvalidEntityId && event.EntityB != InvalidEntityId)
            {
                m_impl->TriggerEvents.push_back(event);
            }
        }

        for (int index = 0; index < sensors.endCount; ++index)
        {
            const b2SensorEndTouchEvent& end = sensors.endEvents[index];
            const CollisionEvent2D event =
                m_impl->MakeContactEvent(
                    PhysicsEventType::Exit,
                    end.sensorShapeId,
                    end.visitorShapeId
                );

            if (event.EntityA != InvalidEntityId && event.EntityB != InvalidEntityId)
            {
                m_impl->TriggerEvents.push_back(event);
            }
        }

        for (auto& item : m_impl->Bodies)
        {
            m_impl->SyncTransformFromBox2D(item.second);
        }
#else
        (void)fixedDeltaTime;
#endif
    }

    RaycastHit2D PhysicsWorld2D::Raycast(
        const Vec2& origin,
        const Vec2& direction,
        float maxDistance
    ) const
    {
        RaycastHit2D result;

        if (!m_impl->Available || maxDistance <= 0.0f)
        {
            return result;
        }

#if defined(JEVAING_HAS_BOX2D)
        if (!m_impl->HasWorld())
        {
            return result;
        }

        const Vec2 normalized = Normalize(direction);

        if (Length(normalized) <= 0.000001f)
        {
            return result;
        }

        const b2QueryFilter filter = b2DefaultQueryFilter();
        const b2RayResult hit =
            b2World_CastRayClosest(
                m_impl->WorldId,
                ToBox2D(origin),
                ToBox2D(normalized * maxDistance),
                filter
            );

        if (!hit.hit)
        {
            return result;
        }

        EntityId entity = InvalidEntityId;

        if (!m_impl->TryGetEntity(hit.shapeId, entity))
        {
            return result;
        }

        result.Hit = true;
        result.Entity = entity;
        result.Point = FromBox2D(hit.point);
        result.Normal = FromBox2D(hit.normal);
        result.Distance = hit.fraction * maxDistance;
#else
        (void)origin;
        (void)direction;
#endif

        return result;
    }

    const std::vector<CollisionEvent2D>& PhysicsWorld2D::GetCollisionEvents() const
    {
        return m_impl->CollisionEvents;
    }

    const std::vector<CollisionEvent2D>& PhysicsWorld2D::GetTriggerEvents() const
    {
        return m_impl->TriggerEvents;
    }
}
