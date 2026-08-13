#include <Jevaing/Physics.h>

#if defined(JEVAING_HAS_BOX2D)
#include <box2d/box2d.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "../../../Core/Logger.h"

namespace Jevaing
{
    namespace
    {
        struct Bounds2D
        {
            Vec2 Min;
            Vec2 Max;
        };

        std::uint64_t PairKey(EntityId left, EntityId right)
        {
            if (left > right)
            {
                std::swap(left, right);
            }

            return (left << 32) ^ right;
        }

        bool Overlaps(const Bounds2D& left, const Bounds2D& right)
        {
            return
                left.Min.X <= right.Max.X &&
                left.Max.X >= right.Min.X &&
                left.Min.Y <= right.Max.Y &&
                left.Max.Y >= right.Min.Y;
        }

        bool RayAabb(
            const Vec2& origin,
            const Vec2& direction,
            float maxDistance,
            const Bounds2D& bounds,
            float& distance,
            Vec2& normal
        )
        {
            float tMin = 0.0f;
            float tMax = maxDistance;
            normal = {};

            const float originValues[2] = { origin.X, origin.Y };
            const float directionValues[2] = { direction.X, direction.Y };
            const float minValues[2] = { bounds.Min.X, bounds.Min.Y };
            const float maxValues[2] = { bounds.Max.X, bounds.Max.Y };

            for (int axis = 0; axis < 2; ++axis)
            {
                if (std::fabs(directionValues[axis]) < 0.000001f)
                {
                    if (originValues[axis] < minValues[axis] || originValues[axis] > maxValues[axis])
                    {
                        return false;
                    }

                    continue;
                }

                const float inverse = 1.0f / directionValues[axis];
                float t1 = (minValues[axis] - originValues[axis]) * inverse;
                float t2 = (maxValues[axis] - originValues[axis]) * inverse;
                const float axisNormal = inverse < 0.0f ? 1.0f : -1.0f;

                if (t1 > t2)
                {
                    std::swap(t1, t2);
                }

                if (t1 > tMin)
                {
                    tMin = t1;
                    normal = axis == 0 ? Vec2{ axisNormal, 0.0f } : Vec2{ 0.0f, axisNormal };
                }

                tMax = std::min(tMax, t2);

                if (tMin > tMax)
                {
                    return false;
                }
            }

            distance = tMin;
            return distance >= 0.0f && distance <= maxDistance;
        }
    }

    struct PhysicsWorld2D::Impl
    {
        struct Body
        {
            EntityId Entity = InvalidEntityId;
            Transform TransformState;
            BodyType Type = BodyType::Static;
            float GravityScale = 1.0f;
            float LinearDamping = 0.0f;
            float AngularDamping = 0.0f;
            bool Enabled = true;
            bool LoggedRejectedParent = false;
            Vec2 LinearVelocity = {};
            Vec2 AccumulatedForce = {};
            float AngularVelocity = 0.0f;
            bool HasBox = false;
            bool HasCircle = false;
            BoxCollider2DComponent Box;
            CircleCollider2DComponent Circle;
        };

        Physics2DBackend Backend = Physics2DBackend::None;
        bool Available = false;
        Vec2 Gravity = { 0.0f, -9.81f };
        std::unordered_map<EntityId, Body> Bodies;
        std::unordered_set<std::uint64_t> CollisionPairs;
        std::unordered_set<std::uint64_t> TriggerPairs;
        std::vector<CollisionEvent2D> CollisionEvents;
        std::vector<CollisionEvent2D> TriggerEvents;

        Bounds2D GetBounds(const Body& body) const
        {
            const Vec2 position = {
                body.TransformState.Position.X,
                body.TransformState.Position.Y
            };

            if (body.HasCircle)
            {
                const float radius =
                    body.Circle.Radius *
                    std::max(
                        std::fabs(body.TransformState.Scale.X),
                        std::fabs(body.TransformState.Scale.Y)
                    );
                const Vec2 center = position + body.Circle.Offset;
                return {
                    { center.X - radius, center.Y - radius },
                    { center.X + radius, center.Y + radius }
                };
            }

            const Vec2 size = {
                body.Box.Size.X * std::fabs(body.TransformState.Scale.X),
                body.Box.Size.Y * std::fabs(body.TransformState.Scale.Y)
            };
            const Vec2 center = position + body.Box.Offset;
            return {
                { center.X - size.X * 0.5f, center.Y - size.Y * 0.5f },
                { center.X + size.X * 0.5f, center.Y + size.Y * 0.5f }
            };
        }

        bool IsTrigger(const Body& body) const
        {
            return
                (body.HasBox && body.Box.IsTrigger) ||
                (body.HasCircle && body.Circle.IsTrigger);
        }

        float Restitution(const Body& body) const
        {
            if (body.HasBox)
            {
                return body.Box.Material.Restitution;
            }

            return body.Circle.Material.Restitution;
        }
    };

    PhysicsWorld2D::PhysicsWorld2D()
        : m_impl(std::make_unique<Impl>())
    {
    }

    PhysicsWorld2D::~PhysicsWorld2D() = default;
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

        if (m_impl->Available)
        {
            Internal::Logger::Info("[Jevaing][Physics2D][INFO] Box2D backend initialized.");
        }

        return m_impl->Available || backend == Physics2DBackend::None;
    }

    void PhysicsWorld2D::Shutdown()
    {
        Clear();
        m_impl->Available = false;
        m_impl->Backend = Physics2DBackend::None;
    }

    void PhysicsWorld2D::Clear()
    {
        m_impl->Bodies.clear();
        m_impl->CollisionPairs.clear();
        m_impl->TriggerPairs.clear();
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
            auto& body = m_impl->Bodies[entity];
            if (!body.LoggedRejectedParent)
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics2D][ERROR] Entity " +
                    std::to_string(entity) +
                    " has Dynamic RigidBody2D with unsupported parent relationship."
                );
                body.LoggedRejectedParent = true;
            }
            m_impl->Bodies.erase(entity);
            return;
        }

        auto& body = m_impl->Bodies[entity];
        body.Entity = entity;
        body.TransformState = transform;
        body.Type = type;
        body.Enabled = !rigidBody || rigidBody->Enabled;
        body.GravityScale = rigidBody ? rigidBody->GravityScale : 0.0f;
        body.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
        body.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;
        body.HasBox = boxCollider != nullptr;
        body.HasCircle = circleCollider != nullptr;

        if (boxCollider)
        {
            body.Box = *boxCollider;
        }

        if (circleCollider)
        {
            body.Circle = *circleCollider;
        }
    }

    void PhysicsWorld2D::RemoveEntity(EntityId entity)
    {
        m_impl->Bodies.erase(entity);
    }

    void PhysicsWorld2D::Prune(const std::vector<EntityId>& activeEntities)
    {
        std::unordered_set<EntityId> active(activeEntities.begin(), activeEntities.end());

        for (auto iterator = m_impl->Bodies.begin(); iterator != m_impl->Bodies.end();)
        {
            if (active.find(iterator->first) == active.end())
            {
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

        if (found != m_impl->Bodies.end())
        {
            found->second.TransformState = transform;
        }
    }

    bool PhysicsWorld2D::GetBodyTransform(EntityId entity, Transform& transform) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return false;
        }

        transform = found->second.TransformState;
        return true;
    }

    void PhysicsWorld2D::SetLinearVelocity(EntityId entity, const Vec2& velocity)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.LinearVelocity = velocity;
        }
    }

    Vec2 PhysicsWorld2D::GetLinearVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);
        return found != m_impl->Bodies.end() ? found->second.LinearVelocity : Vec2{};
    }

    void PhysicsWorld2D::ApplyForce(EntityId entity, const Vec2& force)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.AccumulatedForce = found->second.AccumulatedForce + force;
        }
    }

    void PhysicsWorld2D::ApplyImpulse(EntityId entity, const Vec2& impulse)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.LinearVelocity = found->second.LinearVelocity + impulse;
        }
    }

    void PhysicsWorld2D::SetAngularVelocity(EntityId entity, float velocity)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.AngularVelocity = velocity;
        }
    }

    float PhysicsWorld2D::GetAngularVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);
        return found != m_impl->Bodies.end() ? found->second.AngularVelocity : 0.0f;
    }

    void PhysicsWorld2D::Step(double fixedDeltaTime)
    {
        m_impl->CollisionEvents.clear();
        m_impl->TriggerEvents.clear();

        const float dt = static_cast<float>(fixedDeltaTime);

        for (auto& item : m_impl->Bodies)
        {
            Impl::Body& body = item.second;

            if (!body.Enabled || body.Type != BodyType::Dynamic)
            {
                continue;
            }

            const Vec2 acceleration =
                m_impl->Gravity * body.GravityScale +
                body.AccumulatedForce;
            body.LinearVelocity = body.LinearVelocity + acceleration * dt;
            body.LinearVelocity = body.LinearVelocity * std::max(0.0f, 1.0f - body.LinearDamping * dt);
            body.TransformState.Position.X += body.LinearVelocity.X * dt;
            body.TransformState.Position.Y += body.LinearVelocity.Y * dt;
            body.TransformState.Rotation.Z += body.AngularVelocity * dt;
            body.AccumulatedForce = {};
        }

        std::unordered_set<std::uint64_t> collisionsThisStep;
        std::unordered_set<std::uint64_t> triggersThisStep;

        for (auto left = m_impl->Bodies.begin(); left != m_impl->Bodies.end(); ++left)
        {
            auto right = left;
            ++right;

            for (; right != m_impl->Bodies.end(); ++right)
            {
                Impl::Body& a = left->second;
                Impl::Body& b = right->second;

                if (!a.Enabled || !b.Enabled)
                {
                    continue;
                }

                const Bounds2D boundsA = m_impl->GetBounds(a);
                const Bounds2D boundsB = m_impl->GetBounds(b);

                if (!Overlaps(boundsA, boundsB))
                {
                    continue;
                }

                const std::uint64_t key = PairKey(a.Entity, b.Entity);
                const bool trigger = m_impl->IsTrigger(a) || m_impl->IsTrigger(b);

                if (trigger)
                {
                    triggersThisStep.insert(key);

                    if (m_impl->TriggerPairs.find(key) == m_impl->TriggerPairs.end())
                    {
                        m_impl->TriggerEvents.push_back({ PhysicsEventType::Enter, a.Entity, b.Entity });
                    }

                    continue;
                }

                collisionsThisStep.insert(key);

                if (m_impl->CollisionPairs.find(key) == m_impl->CollisionPairs.end())
                {
                    m_impl->CollisionEvents.push_back({ PhysicsEventType::Enter, a.Entity, b.Entity });
                }

                Impl::Body* dynamicBody = nullptr;
                const Bounds2D* dynamicBounds = nullptr;
                const Bounds2D* otherBounds = nullptr;

                if (a.Type == BodyType::Dynamic && b.Type != BodyType::Dynamic)
                {
                    dynamicBody = &a;
                    dynamicBounds = &boundsA;
                    otherBounds = &boundsB;
                }
                else if (b.Type == BodyType::Dynamic && a.Type != BodyType::Dynamic)
                {
                    dynamicBody = &b;
                    dynamicBounds = &boundsB;
                    otherBounds = &boundsA;
                }
                else if (a.Type == BodyType::Dynamic && b.Type == BodyType::Dynamic)
                {
                    dynamicBody = boundsA.Max.Y > boundsB.Max.Y ? &a : &b;
                    dynamicBounds = dynamicBody == &a ? &boundsA : &boundsB;
                    otherBounds = dynamicBody == &a ? &boundsB : &boundsA;
                }

                if (dynamicBody && dynamicBounds && otherBounds)
                {
                    const float penetrationY = otherBounds->Max.Y - dynamicBounds->Min.Y;

                    if (penetrationY > 0.0f && dynamicBody->LinearVelocity.Y <= 0.0f)
                    {
                        dynamicBody->TransformState.Position.Y += penetrationY;
                        dynamicBody->LinearVelocity.Y =
                            -dynamicBody->LinearVelocity.Y * m_impl->Restitution(*dynamicBody);
                        dynamicBody->LinearVelocity.X *= 0.92f;
                    }
                }
            }
        }

        for (std::uint64_t pair : m_impl->CollisionPairs)
        {
            if (collisionsThisStep.find(pair) == collisionsThisStep.end())
            {
                m_impl->CollisionEvents.push_back({ PhysicsEventType::Exit });
            }
        }

        for (std::uint64_t pair : m_impl->TriggerPairs)
        {
            if (triggersThisStep.find(pair) == triggersThisStep.end())
            {
                m_impl->TriggerEvents.push_back({ PhysicsEventType::Exit });
            }
        }

        m_impl->CollisionPairs = std::move(collisionsThisStep);
        m_impl->TriggerPairs = std::move(triggersThisStep);
    }

    RaycastHit2D PhysicsWorld2D::Raycast(
        const Vec2& origin,
        const Vec2& direction,
        float maxDistance
    ) const
    {
        const Vec2 normalized = Normalize(direction);
        RaycastHit2D best;
        best.Distance = std::numeric_limits<float>::max();

        for (const auto& item : m_impl->Bodies)
        {
            const Impl::Body& body = item.second;
            float distance = 0.0f;
            Vec2 normal;

            if (RayAabb(origin, normalized, maxDistance, m_impl->GetBounds(body), distance, normal) &&
                distance < best.Distance)
            {
                best.Hit = true;
                best.Entity = body.Entity;
                best.Distance = distance;
                best.Point = origin + normalized * distance;
                best.Normal = normal;
            }
        }

        if (!best.Hit)
        {
            best.Distance = 0.0f;
        }

        return best;
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
