#include <Jevaing/Physics.h>

#if defined(JEVAING_HAS_JOLT)
#include <Jolt/Jolt.h>
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
        struct Bounds3
        {
            Vec3 Min;
            Vec3 Max;
        };

        std::uint64_t PairKey(EntityId left, EntityId right)
        {
            if (left > right)
            {
                std::swap(left, right);
            }

            return (left << 32) ^ right;
        }

        bool Overlaps(const Bounds3& left, const Bounds3& right)
        {
            return
                left.Min.X <= right.Max.X &&
                left.Max.X >= right.Min.X &&
                left.Min.Y <= right.Max.Y &&
                left.Max.Y >= right.Min.Y &&
                left.Min.Z <= right.Max.Z &&
                left.Max.Z >= right.Min.Z;
        }

        bool RayAabb(
            const Vec3& origin,
            const Vec3& direction,
            float maxDistance,
            const Bounds3& bounds,
            float& distance,
            Vec3& normal
        )
        {
            float tMin = 0.0f;
            float tMax = maxDistance;
            normal = {};

            const float originValues[3] = { origin.X, origin.Y, origin.Z };
            const float directionValues[3] = { direction.X, direction.Y, direction.Z };
            const float minValues[3] = { bounds.Min.X, bounds.Min.Y, bounds.Min.Z };
            const float maxValues[3] = { bounds.Max.X, bounds.Max.Y, bounds.Max.Z };

            for (int axis = 0; axis < 3; ++axis)
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
                    normal =
                        axis == 0
                            ? Vec3{ axisNormal, 0.0f, 0.0f }
                            : (axis == 1 ? Vec3{ 0.0f, axisNormal, 0.0f } : Vec3{ 0.0f, 0.0f, axisNormal });
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

    struct PhysicsWorld3D::Impl
    {
        struct Body
        {
            EntityId Entity = InvalidEntityId;
            Transform TransformState;
            BodyType Type = BodyType::Static;
            float GravityFactor = 1.0f;
            float LinearDamping = 0.0f;
            float AngularDamping = 0.0f;
            bool Enabled = true;
            bool LoggedRejectedParent = false;
            Vec3 LinearVelocity = {};
            Vec3 AngularVelocity = {};
            Vec3 AccumulatedForce = {};
            bool HasBox = false;
            bool HasSphere = false;
            bool HasCapsule = false;
            BoxCollider3DComponent Box;
            SphereCollider3DComponent Sphere;
            CapsuleCollider3DComponent Capsule;
        };

        Physics3DBackend Backend = Physics3DBackend::None;
        bool Available = false;
        Vec3 Gravity = { 0.0f, -9.81f, 0.0f };
        std::unordered_map<EntityId, Body> Bodies;
        std::unordered_set<std::uint64_t> CollisionPairs;
        std::unordered_set<std::uint64_t> TriggerPairs;
        std::vector<CollisionEvent3D> CollisionEvents;
        std::vector<CollisionEvent3D> TriggerEvents;

        Bounds3 GetBounds(const Body& body) const
        {
            const Vec3 position = body.TransformState.Position;

            if (body.HasSphere)
            {
                const float radius =
                    body.Sphere.Radius *
                    std::max({
                        std::fabs(body.TransformState.Scale.X),
                        std::fabs(body.TransformState.Scale.Y),
                        std::fabs(body.TransformState.Scale.Z)
                    });
                const Vec3 center = position + body.Sphere.Offset;
                return {
                    { center.X - radius, center.Y - radius, center.Z - radius },
                    { center.X + radius, center.Y + radius, center.Z + radius }
                };
            }

            if (body.HasCapsule)
            {
                const float radius = body.Capsule.Radius;
                const float halfHeight = body.Capsule.Height * 0.5f + radius;
                const Vec3 center = position + body.Capsule.Offset;
                return {
                    { center.X - radius, center.Y - halfHeight, center.Z - radius },
                    { center.X + radius, center.Y + halfHeight, center.Z + radius }
                };
            }

            const Vec3 size = {
                body.Box.Size.X * std::fabs(body.TransformState.Scale.X),
                body.Box.Size.Y * std::fabs(body.TransformState.Scale.Y),
                body.Box.Size.Z * std::fabs(body.TransformState.Scale.Z)
            };
            const Vec3 center = position + body.Box.Offset;
            return {
                {
                    center.X - size.X * 0.5f,
                    center.Y - size.Y * 0.5f,
                    center.Z - size.Z * 0.5f
                },
                {
                    center.X + size.X * 0.5f,
                    center.Y + size.Y * 0.5f,
                    center.Z + size.Z * 0.5f
                }
            };
        }

        bool IsTrigger(const Body& body) const
        {
            return
                (body.HasBox && body.Box.IsTrigger) ||
                (body.HasSphere && body.Sphere.IsTrigger) ||
                (body.HasCapsule && body.Capsule.IsTrigger);
        }

        float Restitution(const Body& body) const
        {
            if (body.HasBox)
            {
                return body.Box.Material.Restitution;
            }

            if (body.HasSphere)
            {
                return body.Sphere.Material.Restitution;
            }

            return body.Capsule.Material.Restitution;
        }
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
    }

    void PhysicsWorld3D::Clear()
    {
        m_impl->Bodies.clear();
        m_impl->CollisionPairs.clear();
        m_impl->TriggerPairs.clear();
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

        const BodyType type = rigidBody ? rigidBody->Type : BodyType::Static;

        if (type == BodyType::Dynamic && rejectDynamicParent)
        {
            auto& body = m_impl->Bodies[entity];
            if (!body.LoggedRejectedParent)
            {
                Internal::Logger::Error(
                    "[Jevaing][Physics3D][ERROR] Entity " +
                    std::to_string(entity) +
                    " has Dynamic RigidBody3D with unsupported parent relationship."
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
        body.GravityFactor = rigidBody ? rigidBody->GravityFactor : 0.0f;
        body.LinearDamping = rigidBody ? rigidBody->LinearDamping : 0.0f;
        body.AngularDamping = rigidBody ? rigidBody->AngularDamping : 0.0f;
        body.HasBox = boxCollider != nullptr;
        body.HasSphere = sphereCollider != nullptr;
        body.HasCapsule = capsuleCollider != nullptr;

        if (boxCollider)
        {
            body.Box = *boxCollider;
        }

        if (sphereCollider)
        {
            body.Sphere = *sphereCollider;
        }

        if (capsuleCollider)
        {
            body.Capsule = *capsuleCollider;
        }
    }

    void PhysicsWorld3D::RemoveEntity(EntityId entity)
    {
        m_impl->Bodies.erase(entity);
    }

    void PhysicsWorld3D::Prune(const std::vector<EntityId>& activeEntities)
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

    void PhysicsWorld3D::SetBodyTransform(EntityId entity, const Transform& transform)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.TransformState = transform;
        }
    }

    bool PhysicsWorld3D::GetBodyTransform(EntityId entity, Transform& transform) const
    {
        const auto found = m_impl->Bodies.find(entity);

        if (found == m_impl->Bodies.end())
        {
            return false;
        }

        transform = found->second.TransformState;
        return true;
    }

    void PhysicsWorld3D::SetLinearVelocity(EntityId entity, const Vec3& velocity)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.LinearVelocity = velocity;
        }
    }

    Vec3 PhysicsWorld3D::GetLinearVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);
        return found != m_impl->Bodies.end() ? found->second.LinearVelocity : Vec3{};
    }

    void PhysicsWorld3D::ApplyForce(EntityId entity, const Vec3& force)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.AccumulatedForce = found->second.AccumulatedForce + force;
        }
    }

    void PhysicsWorld3D::ApplyImpulse(EntityId entity, const Vec3& impulse)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.LinearVelocity = found->second.LinearVelocity + impulse;
        }
    }

    void PhysicsWorld3D::SetAngularVelocity(EntityId entity, const Vec3& velocity)
    {
        auto found = m_impl->Bodies.find(entity);

        if (found != m_impl->Bodies.end())
        {
            found->second.AngularVelocity = velocity;
        }
    }

    Vec3 PhysicsWorld3D::GetAngularVelocity(EntityId entity) const
    {
        const auto found = m_impl->Bodies.find(entity);
        return found != m_impl->Bodies.end() ? found->second.AngularVelocity : Vec3{};
    }

    void PhysicsWorld3D::WakeUp(EntityId)
    {
    }

    void PhysicsWorld3D::Step(double fixedDeltaTime)
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

            const Vec3 acceleration =
                m_impl->Gravity * body.GravityFactor +
                body.AccumulatedForce;
            body.LinearVelocity = body.LinearVelocity + acceleration * dt;
            body.LinearVelocity = body.LinearVelocity * std::max(0.0f, 1.0f - body.LinearDamping * dt);
            body.TransformState.Position = body.TransformState.Position + body.LinearVelocity * dt;
            body.TransformState.Rotation = body.TransformState.Rotation + body.AngularVelocity * dt;
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

                const Bounds3 boundsA = m_impl->GetBounds(a);
                const Bounds3 boundsB = m_impl->GetBounds(b);

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
                const Bounds3* dynamicBounds = nullptr;
                const Bounds3* otherBounds = nullptr;

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
                        dynamicBody->LinearVelocity.Z *= 0.92f;
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

    RaycastHit3D PhysicsWorld3D::Raycast(
        const Vec3& origin,
        const Vec3& direction,
        float maxDistance
    ) const
    {
        const Vec3 normalized = Normalize(direction);
        RaycastHit3D best;
        best.Distance = std::numeric_limits<float>::max();

        for (const auto& item : m_impl->Bodies)
        {
            const Impl::Body& body = item.second;
            float distance = 0.0f;
            Vec3 normal;

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

    const std::vector<CollisionEvent3D>& PhysicsWorld3D::GetCollisionEvents() const
    {
        return m_impl->CollisionEvents;
    }

    const std::vector<CollisionEvent3D>& PhysicsWorld3D::GetTriggerEvents() const
    {
        return m_impl->TriggerEvents;
    }
}
