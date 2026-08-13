#include <Jevaing/Scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Logger.h"

namespace Jevaing
{
    namespace
    {
        std::string Trim(const std::string& value)
        {
            const auto begin = std::find_if_not(
                value.begin(),
                value.end(),
                [](unsigned char character)
                {
                    return std::isspace(character) != 0;
                }
            );

            const auto end = std::find_if_not(
                value.rbegin(),
                value.rend(),
                [](unsigned char character)
                {
                    return std::isspace(character) != 0;
                }
            ).base();

            if (begin >= end)
            {
                return {};
            }

            return std::string(begin, end);
        }

        bool SplitKeyValue(
            const std::string& line,
            std::string& key,
            std::string& value
        )
        {
            const std::size_t separator = line.find('=');

            if (separator == std::string::npos)
            {
                return false;
            }

            key = Trim(line.substr(0, separator));
            value = Trim(line.substr(separator + 1));
            return !key.empty();
        }

        bool ParseVec2(const std::string& value, Vec2& result)
        {
            std::stringstream stream(value);
            char comma = '\0';
            return
                (stream >> result.X) &&
                (stream >> comma) &&
                comma == ',' &&
                (stream >> result.Y);
        }

        bool ParseVec3(const std::string& value, Vec3& result)
        {
            std::stringstream stream(value);
            char commaA = '\0';
            char commaB = '\0';
            return
                (stream >> result.X) &&
                (stream >> commaA) &&
                commaA == ',' &&
                (stream >> result.Y) &&
                (stream >> commaB) &&
                commaB == ',' &&
                (stream >> result.Z);
        }

        bool ParseColor(const std::string& value, Color& result)
        {
            std::stringstream stream(value);
            char commaA = '\0';
            char commaB = '\0';
            char commaC = '\0';
            return
                (stream >> result.R) &&
                (stream >> commaA) &&
                commaA == ',' &&
                (stream >> result.G) &&
                (stream >> commaB) &&
                commaB == ',' &&
                (stream >> result.B) &&
                (stream >> commaC) &&
                commaC == ',' &&
                (stream >> result.A);
        }

        bool ParseFloat(const std::string& value, float& result)
        {
            try
            {
                std::size_t parsed = 0;
                result = std::stof(value, &parsed);
                return parsed == value.size();
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        bool ParseInt(const std::string& value, int& result)
        {
            try
            {
                std::size_t parsed = 0;
                result = std::stoi(value, &parsed);
                return parsed == value.size();
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        bool ParseEntityId(const std::string& value, EntityId& result)
        {
            try
            {
                std::size_t parsed = 0;
                result = static_cast<EntityId>(std::stoull(value, &parsed));
                return parsed == value.size();
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        bool ParseBool(const std::string& value, bool& result)
        {
            if (value == "1" || value == "true")
            {
                result = true;
                return true;
            }

            if (value == "0" || value == "false")
            {
                result = false;
                return true;
            }

            return false;
        }

        std::string ParseError(
            const std::string& key,
            int lineNumber,
            const std::string& path
        )
        {
            return
                "Invalid value for '" +
                key +
                "' near line " +
                std::to_string(lineNumber) +
                " in " +
                path;
        }

        Vec3 TransformPoint(const Mat4& matrix, const Vec3& point)
        {
            return {
                point.X * matrix.M[0][0] +
                    point.Y * matrix.M[1][0] +
                    point.Z * matrix.M[2][0] +
                    matrix.M[3][0],
                point.X * matrix.M[0][1] +
                    point.Y * matrix.M[1][1] +
                    point.Z * matrix.M[2][1] +
                    matrix.M[3][1],
                point.X * matrix.M[0][2] +
                    point.Y * matrix.M[1][2] +
                    point.Z * matrix.M[2][2] +
                    matrix.M[3][2]
            };
        }

        std::string Vec2ToString(const Vec2& value)
        {
            return std::to_string(value.X) + "," + std::to_string(value.Y);
        }

        std::string Vec3ToString(const Vec3& value)
        {
            return
                std::to_string(value.X) + "," +
                std::to_string(value.Y) + "," +
                std::to_string(value.Z);
        }

        std::string ColorToString(const Color& value)
        {
            return
                std::to_string(value.R) + "," +
                std::to_string(value.G) + "," +
                std::to_string(value.B) + "," +
                std::to_string(value.A);
        }
    }

    Scene::Scene(std::string name)
        : m_name(std::move(name))
    {
    }

    EntityId Scene::CreateEntity(const std::string& name)
    {
        return CreateEntityWithId(AllocateEntityId(), name);
    }

    EntityId Scene::CreateEntityWithId(EntityId id, const std::string& name)
    {
        if (id == InvalidEntityId || FindEntity(id))
        {
            return InvalidEntityId;
        }

        SceneEntity entity;
        entity.Id = id;
        entity.Name = name;
        m_entities.push_back(entity);
        m_nextEntityId = std::max(m_nextEntityId, id + 1);
        return id;
    }

    bool Scene::DestroyEntity(EntityId id)
    {
        SceneEntity* entity = FindEntity(id);

        if (!entity)
        {
            return false;
        }

        const std::vector<EntityId> children = entity->Children;

        for (EntityId child : children)
        {
            RemoveParent(child);
        }

        if (entity->Parent != InvalidEntityId)
        {
            RemoveChildLink(entity->Parent, id);
        }

        m_physics2D.RemoveEntity(id);
        m_physics3D.RemoveEntity(id);

        m_entities.erase(
            std::remove_if(
                m_entities.begin(),
                m_entities.end(),
                [id](const SceneEntity& candidate)
                {
                    return candidate.Id == id;
                }
            ),
            m_entities.end()
        );

        return true;
    }

    SceneEntity* Scene::FindEntity(EntityId id)
    {
        for (SceneEntity& entity : m_entities)
        {
            if (entity.Id == id)
            {
                return &entity;
            }
        }

        return nullptr;
    }

    const SceneEntity* Scene::FindEntity(EntityId id) const
    {
        for (const SceneEntity& entity : m_entities)
        {
            if (entity.Id == id)
            {
                return &entity;
            }
        }

        return nullptr;
    }

    SceneEntity* Scene::FindEntityByName(const std::string& name)
    {
        for (SceneEntity& entity : m_entities)
        {
            if (entity.Name == name)
            {
                return &entity;
            }
        }

        return nullptr;
    }

    const SceneEntity* Scene::FindEntityByName(const std::string& name) const
    {
        for (const SceneEntity& entity : m_entities)
        {
            if (entity.Name == name)
            {
                return &entity;
            }
        }

        return nullptr;
    }

    bool Scene::SetParent(EntityId child, EntityId parent, std::string* error)
    {
        if (child == InvalidEntityId || parent == InvalidEntityId || child == parent)
        {
            if (error)
            {
                *error = "Invalid parent relationship.";
            }

            return false;
        }

        SceneEntity* childEntity = FindEntity(child);
        SceneEntity* parentEntity = FindEntity(parent);

        if (!childEntity || !parentEntity)
        {
            if (error)
            {
                *error = "SetParent references a missing entity.";
            }

            return false;
        }

        if (WouldCreateCycle(child, parent))
        {
            if (error)
            {
                *error = "SetParent would create a transform hierarchy cycle.";
            }

            return false;
        }

        if (childEntity->Parent != InvalidEntityId)
        {
            RemoveChildLink(childEntity->Parent, child);
        }

        childEntity->Parent = parent;

        if (
            std::find(
                parentEntity->Children.begin(),
                parentEntity->Children.end(),
                child
            ) == parentEntity->Children.end()
        )
        {
            parentEntity->Children.push_back(child);
        }

        return true;
    }

    bool Scene::RemoveParent(EntityId child)
    {
        SceneEntity* childEntity = FindEntity(child);

        if (!childEntity)
        {
            return false;
        }

        if (childEntity->Parent != InvalidEntityId)
        {
            RemoveChildLink(childEntity->Parent, child);
            childEntity->Parent = InvalidEntityId;
        }

        return true;
    }

    Transform Scene::GetWorldTransform(EntityId id) const
    {
        const SceneEntity* entity = FindEntity(id);

        if (!entity)
        {
            return {};
        }

        Transform world = entity->Transform.LocalTransform;
        EntityId parent = entity->Parent;

        while (parent != InvalidEntityId)
        {
            const SceneEntity* parentEntity = FindEntity(parent);

            if (!parentEntity)
            {
                break;
            }

            const Transform parentTransform = parentEntity->Transform.LocalTransform;
            world.Position = TransformPoint(parentTransform.ToMatrix(), world.Position);
            world.Rotation = parentTransform.Rotation + world.Rotation;
            world.Scale = {
                parentTransform.Scale.X * world.Scale.X,
                parentTransform.Scale.Y * world.Scale.Y,
                parentTransform.Scale.Z * world.Scale.Z
            };
            parent = parentEntity->Parent;
        }

        return world;
    }

    void Scene::OnLoad()
    {
    }

    void Scene::OnStart()
    {
        m_started = true;
    }

    void Scene::Update(double deltaTime)
    {
        if (!m_started)
        {
            OnStart();
        }

        bool hasPhysicsComponents =
            m_physics2D.GetBodyCount() > 0 ||
            m_physics3D.GetBodyCount() > 0;

        for (const SceneEntity& entity : m_entities)
        {
            hasPhysicsComponents =
                hasPhysicsComponents ||
                entity.BoxCollider2D ||
                entity.CircleCollider2D ||
                entity.BoxCollider3D ||
                entity.SphereCollider3D ||
                entity.CapsuleCollider3D;
        }

        if (!hasPhysicsComponents)
        {
            return;
        }

        EnsurePhysicsInitialized();
        SyncSceneToPhysics();

        const double fixedDeltaTime =
            m_physicsSettings.FixedDeltaTime > 0.0
                ? m_physicsSettings.FixedDeltaTime
                : 1.0 / 60.0;

        m_physicsAccumulator += deltaTime;
        bool stepped = false;

        while (m_physicsAccumulator >= fixedDeltaTime)
        {
            m_physics2D.Step(fixedDeltaTime);
            m_physics3D.Step(fixedDeltaTime);
            m_physicsAccumulator -= fixedDeltaTime;
            stepped = true;
        }

        if (stepped)
        {
            SyncPhysicsToScene();
        }
    }

    void Scene::Render(Graphics2D& graphics)
    {
        std::vector<const SceneEntity*> sprites;

        for (const SceneEntity& entity : m_entities)
        {
            if (entity.SpriteRenderer2D)
            {
                sprites.push_back(&entity);
            }
        }

        std::sort(
            sprites.begin(),
            sprites.end(),
            [](const SceneEntity* left, const SceneEntity* right)
            {
                return left->SpriteRenderer2D->Layer < right->SpriteRenderer2D->Layer;
            }
        );

        for (const SceneEntity* entity : sprites)
        {
            const SpriteRenderer2DComponent& sprite = *entity->SpriteRenderer2D;
            const Transform world = GetWorldTransform(entity->Id);
            graphics.DrawSprite(
                sprite.Texture,
                { world.Position.X, world.Position.Y },
                sprite.Size,
                sprite.Tint
            );
        }
    }

    void Scene::Render(Graphics3D& graphics)
    {
        const SceneEntity* primaryCamera = nullptr;

        for (const SceneEntity& entity : m_entities)
        {
            if (entity.Camera && entity.Camera->Primary)
            {
                primaryCamera = &entity;
                break;
            }
        }

        if (primaryCamera)
        {
            PerspectiveCamera camera = primaryCamera->Camera->Camera;
            const Transform cameraTransform = GetWorldTransform(primaryCamera->Id);
            camera.Position = cameraTransform.Position;
            graphics.SetCamera(camera);
        }
        else
        {
            static bool warnedMissingPrimaryCamera = false;

            if (!warnedMissingPrimaryCamera)
            {
                Internal::Logger::Warning(
                    "[Jevaing][Scene][WARNING] 3D Scene render skipped because no primary CameraComponent exists."
                );
                warnedMissingPrimaryCamera = true;
            }

            return;
        }

        for (const SceneEntity& entity : m_entities)
        {
            if (!entity.MeshRenderer || !entity.MeshRenderer->ModelAsset)
            {
                continue;
            }

            const Transform world = GetWorldTransform(entity.Id);

            for (const Mesh& mesh : entity.MeshRenderer->ModelAsset->Meshes)
            {
                Material material;

                if (entity.MeshRenderer->HasMaterialOverride)
                {
                    material = entity.MeshRenderer->MaterialOverride;
                }
                else if (mesh.MaterialIndex < entity.MeshRenderer->ModelAsset->Materials.size())
                {
                    material = entity.MeshRenderer->ModelAsset->Materials[mesh.MaterialIndex];
                }

                graphics.DrawMesh(mesh, world, material);
            }
        }
    }

    void Scene::OnUnload()
    {
        m_started = false;
        m_physicsAccumulator = 0.0;
        m_physics2D.Clear();
        m_physics3D.Clear();
    }

    bool Scene::Load(
        const std::string& path,
        const std::string& assetRoot,
        std::string& error
    )
    {
        return LoadFromFile(path, assetRoot, *this, error);
    }

    bool Scene::Save(const std::string& path, std::string& error) const
    {
        error.clear();
        std::ofstream file(path);

        if (!file)
        {
            error = "Failed to write scene file: " + path;
            return false;
        }

        file << "scene=" << m_name << "\n\n";

        for (const SceneEntity& entity : m_entities)
        {
            const Transform& transform = entity.Transform.LocalTransform;
            file
                << "[entity]\n"
                << "id=" << entity.Id << "\n"
                << "name=" << entity.Name << "\n"
                << "parent=" << entity.Parent << "\n"
                << "position=" << Vec3ToString(transform.Position) << "\n"
                << "rotation=" << Vec3ToString(transform.Rotation) << "\n"
                << "scale=" << Vec3ToString(transform.Scale) << "\n";

            if (entity.Camera)
            {
                file
                    << "camera.primary=" << (entity.Camera->Primary ? 1 : 0) << "\n"
                    << "camera.fov=" << entity.Camera->Camera.VerticalFovRadians << "\n"
                    << "camera.near=" << entity.Camera->Camera.NearPlane << "\n"
                    << "camera.far=" << entity.Camera->Camera.FarPlane << "\n";
            }

            if (entity.MeshRenderer)
            {
                file << "mesh.model=" << entity.MeshRenderer->ModelPath << "\n";

                if (entity.MeshRenderer->HasMaterialOverride)
                {
                    file
                        << "mesh.color="
                        << ColorToString(entity.MeshRenderer->MaterialOverride.BaseColor)
                        << "\n";
                }
            }

            if (entity.SpriteRenderer2D)
            {
                file
                    << "sprite.texture=" << entity.SpriteRenderer2D->TexturePath << "\n"
                    << "sprite.size=" << Vec2ToString(entity.SpriteRenderer2D->Size) << "\n"
                    << "sprite.tint=" << ColorToString(entity.SpriteRenderer2D->Tint) << "\n"
                    << "sprite.layer=" << entity.SpriteRenderer2D->Layer << "\n";
            }

            if (entity.RigidBody2D)
            {
                file
                    << "rigidbody2d.type=" << BodyTypeToString(entity.RigidBody2D->Type) << "\n"
                    << "rigidbody2d.gravityScale=" << entity.RigidBody2D->GravityScale << "\n"
                    << "rigidbody2d.linearDamping=" << entity.RigidBody2D->LinearDamping << "\n"
                    << "rigidbody2d.angularDamping=" << entity.RigidBody2D->AngularDamping << "\n"
                    << "rigidbody2d.enabled=" << (entity.RigidBody2D->Enabled ? 1 : 0) << "\n";
            }

            if (entity.BoxCollider2D)
            {
                file
                    << "boxcollider2d.offset=" << Vec2ToString(entity.BoxCollider2D->Offset) << "\n"
                    << "boxcollider2d.size=" << Vec2ToString(entity.BoxCollider2D->Size) << "\n"
                    << "boxcollider2d.friction=" << entity.BoxCollider2D->Material.Friction << "\n"
                    << "boxcollider2d.restitution=" << entity.BoxCollider2D->Material.Restitution << "\n"
                    << "boxcollider2d.density=" << entity.BoxCollider2D->Material.Density << "\n"
                    << "boxcollider2d.trigger=" << (entity.BoxCollider2D->IsTrigger ? 1 : 0) << "\n";
            }

            if (entity.CircleCollider2D)
            {
                file
                    << "circlecollider2d.offset=" << Vec2ToString(entity.CircleCollider2D->Offset) << "\n"
                    << "circlecollider2d.radius=" << entity.CircleCollider2D->Radius << "\n"
                    << "circlecollider2d.friction=" << entity.CircleCollider2D->Material.Friction << "\n"
                    << "circlecollider2d.restitution=" << entity.CircleCollider2D->Material.Restitution << "\n"
                    << "circlecollider2d.density=" << entity.CircleCollider2D->Material.Density << "\n"
                    << "circlecollider2d.trigger=" << (entity.CircleCollider2D->IsTrigger ? 1 : 0) << "\n";
            }

            if (entity.RigidBody3D)
            {
                file
                    << "rigidbody3d.type=" << BodyTypeToString(entity.RigidBody3D->Type) << "\n"
                    << "rigidbody3d.gravityFactor=" << entity.RigidBody3D->GravityFactor << "\n"
                    << "rigidbody3d.linearDamping=" << entity.RigidBody3D->LinearDamping << "\n"
                    << "rigidbody3d.angularDamping=" << entity.RigidBody3D->AngularDamping << "\n"
                    << "rigidbody3d.enabled=" << (entity.RigidBody3D->Enabled ? 1 : 0) << "\n";
            }

            if (entity.BoxCollider3D)
            {
                file
                    << "boxcollider3d.offset=" << Vec3ToString(entity.BoxCollider3D->Offset) << "\n"
                    << "boxcollider3d.size=" << Vec3ToString(entity.BoxCollider3D->Size) << "\n"
                    << "boxcollider3d.friction=" << entity.BoxCollider3D->Material.Friction << "\n"
                    << "boxcollider3d.restitution=" << entity.BoxCollider3D->Material.Restitution << "\n"
                    << "boxcollider3d.density=" << entity.BoxCollider3D->Material.Density << "\n"
                    << "boxcollider3d.trigger=" << (entity.BoxCollider3D->IsTrigger ? 1 : 0) << "\n";
            }

            if (entity.SphereCollider3D)
            {
                file
                    << "spherecollider3d.offset=" << Vec3ToString(entity.SphereCollider3D->Offset) << "\n"
                    << "spherecollider3d.radius=" << entity.SphereCollider3D->Radius << "\n"
                    << "spherecollider3d.friction=" << entity.SphereCollider3D->Material.Friction << "\n"
                    << "spherecollider3d.restitution=" << entity.SphereCollider3D->Material.Restitution << "\n"
                    << "spherecollider3d.density=" << entity.SphereCollider3D->Material.Density << "\n"
                    << "spherecollider3d.trigger=" << (entity.SphereCollider3D->IsTrigger ? 1 : 0) << "\n";
            }

            if (entity.CapsuleCollider3D)
            {
                file
                    << "capsulecollider3d.offset=" << Vec3ToString(entity.CapsuleCollider3D->Offset) << "\n"
                    << "capsulecollider3d.radius=" << entity.CapsuleCollider3D->Radius << "\n"
                    << "capsulecollider3d.height=" << entity.CapsuleCollider3D->Height << "\n"
                    << "capsulecollider3d.friction=" << entity.CapsuleCollider3D->Material.Friction << "\n"
                    << "capsulecollider3d.restitution=" << entity.CapsuleCollider3D->Material.Restitution << "\n"
                    << "capsulecollider3d.density=" << entity.CapsuleCollider3D->Material.Density << "\n"
                    << "capsulecollider3d.trigger=" << (entity.CapsuleCollider3D->IsTrigger ? 1 : 0) << "\n";
            }

            file << "[/entity]\n\n";
        }

        return true;
    }

    bool Scene::LoadFromFile(
        const std::string& path,
        const std::string& assetRoot,
        Scene& scene,
        std::string& error
    )
    {
        error.clear();
        std::ifstream file(path);

        if (!file)
        {
            error = "Failed to open scene file: " + path;
            return false;
        }

        Scene loaded;
        SceneEntity* current = nullptr;
        std::vector<std::pair<EntityId, EntityId>> parentLinks;

        std::string line;
        int lineNumber = 0;

        while (std::getline(file, line))
        {
            ++lineNumber;
            line = Trim(line);

            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            if (line == "[entity]")
            {
                loaded.m_entities.push_back({});
                current = &loaded.m_entities.back();
                current->Transform.LocalTransform.Scale = { 1.0f, 1.0f, 1.0f };
                continue;
            }

            if (line == "[/entity]")
            {
                if (!current || current->Id == InvalidEntityId)
                {
                    error =
                        "Scene entity missing id near line " +
                        std::to_string(lineNumber) +
                        " in " +
                        path;
                    return false;
                }

                loaded.m_nextEntityId = std::max(loaded.m_nextEntityId, current->Id + 1);
                current = nullptr;
                continue;
            }

            std::string key;
            std::string value;

            if (!SplitKeyValue(line, key, value))
            {
                error =
                    "Invalid scene line " +
                    std::to_string(lineNumber) +
                    " in " +
                    path;
                return false;
            }

            if (!current)
            {
                if (key == "scene")
                {
                    loaded.m_name = value;
                    continue;
                }

                error =
                    "Scene key outside entity near line " +
                    std::to_string(lineNumber) +
                    " in " +
                    path;
                return false;
            }

            if (key == "id")
            {
                if (!ParseEntityId(value, current->Id))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "name")
            {
                current->Name = value;
            }
            else if (key == "parent")
            {
                EntityId parent = InvalidEntityId;
                if (!ParseEntityId(value, parent))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }

                parentLinks.push_back({
                    current->Id,
                    parent
                });
            }
            else if (key == "position")
            {
                if (!ParseVec3(value, current->Transform.LocalTransform.Position))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rotation")
            {
                if (!ParseVec3(value, current->Transform.LocalTransform.Rotation))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "scale")
            {
                if (!ParseVec3(value, current->Transform.LocalTransform.Scale))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "camera.primary")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                if (!ParseBool(value, current->Camera->Primary))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "camera.fov")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                if (!ParseFloat(value, current->Camera->Camera.VerticalFovRadians))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "camera.near")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                if (!ParseFloat(value, current->Camera->Camera.NearPlane))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "camera.far")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                if (!ParseFloat(value, current->Camera->Camera.FarPlane))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "mesh.model")
            {
                current->MeshRenderer = current->MeshRenderer.value_or(MeshRendererComponent{});
                current->MeshRenderer->ModelPath = value;
            }
            else if (key == "mesh.color")
            {
                current->MeshRenderer = current->MeshRenderer.value_or(MeshRendererComponent{});
                current->MeshRenderer->HasMaterialOverride = true;
                if (!ParseColor(value, current->MeshRenderer->MaterialOverride.BaseColor))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "sprite.texture")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                current->SpriteRenderer2D->TexturePath = value;
            }
            else if (key == "sprite.size")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                if (!ParseVec2(value, current->SpriteRenderer2D->Size))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "sprite.tint")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                if (!ParseColor(value, current->SpriteRenderer2D->Tint))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "sprite.layer")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                if (!ParseInt(value, current->SpriteRenderer2D->Layer))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody2d.type")
            {
                current->RigidBody2D = current->RigidBody2D.value_or(RigidBody2DComponent{});
                if (!BodyTypeFromString(value, current->RigidBody2D->Type))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody2d.gravityScale")
            {
                current->RigidBody2D = current->RigidBody2D.value_or(RigidBody2DComponent{});
                if (!ParseFloat(value, current->RigidBody2D->GravityScale))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody2d.linearDamping")
            {
                current->RigidBody2D = current->RigidBody2D.value_or(RigidBody2DComponent{});
                if (!ParseFloat(value, current->RigidBody2D->LinearDamping))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody2d.angularDamping")
            {
                current->RigidBody2D = current->RigidBody2D.value_or(RigidBody2DComponent{});
                if (!ParseFloat(value, current->RigidBody2D->AngularDamping))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody2d.enabled")
            {
                current->RigidBody2D = current->RigidBody2D.value_or(RigidBody2DComponent{});
                if (!ParseBool(value, current->RigidBody2D->Enabled))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.offset")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseVec2(value, current->BoxCollider2D->Offset))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.size")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseVec2(value, current->BoxCollider2D->Size))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.friction")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseFloat(value, current->BoxCollider2D->Material.Friction))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.restitution")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseFloat(value, current->BoxCollider2D->Material.Restitution))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.density")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseFloat(value, current->BoxCollider2D->Material.Density))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider2d.trigger")
            {
                current->BoxCollider2D = current->BoxCollider2D.value_or(BoxCollider2DComponent{});
                if (!ParseBool(value, current->BoxCollider2D->IsTrigger))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.offset")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseVec2(value, current->CircleCollider2D->Offset))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.radius")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseFloat(value, current->CircleCollider2D->Radius))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.friction")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseFloat(value, current->CircleCollider2D->Material.Friction))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.restitution")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseFloat(value, current->CircleCollider2D->Material.Restitution))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.density")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseFloat(value, current->CircleCollider2D->Material.Density))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "circlecollider2d.trigger")
            {
                current->CircleCollider2D = current->CircleCollider2D.value_or(CircleCollider2DComponent{});
                if (!ParseBool(value, current->CircleCollider2D->IsTrigger))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody3d.type")
            {
                current->RigidBody3D = current->RigidBody3D.value_or(RigidBody3DComponent{});
                if (!BodyTypeFromString(value, current->RigidBody3D->Type))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody3d.gravityFactor")
            {
                current->RigidBody3D = current->RigidBody3D.value_or(RigidBody3DComponent{});
                if (!ParseFloat(value, current->RigidBody3D->GravityFactor))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody3d.linearDamping")
            {
                current->RigidBody3D = current->RigidBody3D.value_or(RigidBody3DComponent{});
                if (!ParseFloat(value, current->RigidBody3D->LinearDamping))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody3d.angularDamping")
            {
                current->RigidBody3D = current->RigidBody3D.value_or(RigidBody3DComponent{});
                if (!ParseFloat(value, current->RigidBody3D->AngularDamping))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "rigidbody3d.enabled")
            {
                current->RigidBody3D = current->RigidBody3D.value_or(RigidBody3DComponent{});
                if (!ParseBool(value, current->RigidBody3D->Enabled))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.offset")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseVec3(value, current->BoxCollider3D->Offset))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.size")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseVec3(value, current->BoxCollider3D->Size))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.friction")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseFloat(value, current->BoxCollider3D->Material.Friction))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.restitution")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseFloat(value, current->BoxCollider3D->Material.Restitution))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.density")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseFloat(value, current->BoxCollider3D->Material.Density))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "boxcollider3d.trigger")
            {
                current->BoxCollider3D = current->BoxCollider3D.value_or(BoxCollider3DComponent{});
                if (!ParseBool(value, current->BoxCollider3D->IsTrigger))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.offset")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseVec3(value, current->SphereCollider3D->Offset))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.radius")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseFloat(value, current->SphereCollider3D->Radius))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.friction")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseFloat(value, current->SphereCollider3D->Material.Friction))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.restitution")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseFloat(value, current->SphereCollider3D->Material.Restitution))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.density")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseFloat(value, current->SphereCollider3D->Material.Density))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "spherecollider3d.trigger")
            {
                current->SphereCollider3D = current->SphereCollider3D.value_or(SphereCollider3DComponent{});
                if (!ParseBool(value, current->SphereCollider3D->IsTrigger))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.offset")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseVec3(value, current->CapsuleCollider3D->Offset))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.radius")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseFloat(value, current->CapsuleCollider3D->Radius))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.height")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseFloat(value, current->CapsuleCollider3D->Height))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.friction")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseFloat(value, current->CapsuleCollider3D->Material.Friction))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.restitution")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseFloat(value, current->CapsuleCollider3D->Material.Restitution))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.density")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseFloat(value, current->CapsuleCollider3D->Material.Density))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
            else if (key == "capsulecollider3d.trigger")
            {
                current->CapsuleCollider3D = current->CapsuleCollider3D.value_or(CapsuleCollider3DComponent{});
                if (!ParseBool(value, current->CapsuleCollider3D->IsTrigger))
                {
                    error = ParseError(key, lineNumber, path);
                    return false;
                }
            }
        }

        for (const auto& link : parentLinks)
        {
            if (link.second != InvalidEntityId)
            {
                if (!loaded.SetParent(link.first, link.second, &error))
                {
                    error = "Invalid scene parent link in " + path + ": " + error;
                    return false;
                }
            }
        }

        loaded.AttachLoadedAssets(assetRoot, error);

        if (!error.empty())
        {
            return false;
        }

        scene = std::move(loaded);
        scene.OnLoad();
        return true;
    }

    const std::string& Scene::GetName() const
    {
        return m_name;
    }

    void Scene::SetName(const std::string& name)
    {
        m_name = name;
    }

    const std::vector<SceneEntity>& Scene::GetEntities() const
    {
        return m_entities;
    }

    std::vector<SceneEntity>& Scene::GetEntities()
    {
        return m_entities;
    }

    PhysicsWorld2D& Scene::Physics2D()
    {
        EnsurePhysicsInitialized();
        return m_physics2D;
    }

    const PhysicsWorld2D& Scene::Physics2D() const
    {
        return m_physics2D;
    }

    PhysicsWorld3D& Scene::Physics3D()
    {
        EnsurePhysicsInitialized();
        return m_physics3D;
    }

    const PhysicsWorld3D& Scene::Physics3D() const
    {
        return m_physics3D;
    }

    void Scene::SetPhysicsSettings(const PhysicsSettings& settings)
    {
        m_physicsSettings = settings;
    }

    const PhysicsSettings& Scene::GetPhysicsSettings() const
    {
        return m_physicsSettings;
    }

    const std::vector<CollisionEvent2D>& Scene::GetCollisionEvents2D() const
    {
        return m_physics2D.GetCollisionEvents();
    }

    const std::vector<CollisionEvent2D>& Scene::GetTriggerEvents2D() const
    {
        return m_physics2D.GetTriggerEvents();
    }

    const std::vector<CollisionEvent3D>& Scene::GetCollisionEvents3D() const
    {
        return m_physics3D.GetCollisionEvents();
    }

    const std::vector<CollisionEvent3D>& Scene::GetTriggerEvents3D() const
    {
        return m_physics3D.GetTriggerEvents();
    }

    EntityId Scene::AllocateEntityId()
    {
        while (FindEntity(m_nextEntityId))
        {
            ++m_nextEntityId;
        }

        return m_nextEntityId++;
    }

    bool Scene::WouldCreateCycle(EntityId child, EntityId parent) const
    {
        EntityId current = parent;

        while (current != InvalidEntityId)
        {
            if (current == child)
            {
                return true;
            }

            const SceneEntity* entity = FindEntity(current);

            if (!entity)
            {
                return false;
            }

            current = entity->Parent;
        }

        return false;
    }

    void Scene::RemoveChildLink(EntityId parent, EntityId child)
    {
        SceneEntity* parentEntity = FindEntity(parent);

        if (!parentEntity)
        {
            return;
        }

        parentEntity->Children.erase(
            std::remove(
                parentEntity->Children.begin(),
                parentEntity->Children.end(),
                child
            ),
            parentEntity->Children.end()
        );
    }

    void Scene::AttachLoadedAssets(const std::string& assetRoot, std::string& error)
    {
        namespace fs = std::filesystem;

        for (SceneEntity& entity : m_entities)
        {
            if (entity.MeshRenderer && !entity.MeshRenderer->ModelPath.empty())
            {
                const fs::path modelPath = fs::path(assetRoot) / entity.MeshRenderer->ModelPath;
                std::string loadError;
                entity.MeshRenderer->ModelAsset =
                    Assets::LoadModel(modelPath.string(), &loadError);

                if (!entity.MeshRenderer->ModelAsset)
                {
                    error =
                        "Entity " +
                        std::to_string(entity.Id) +
                        " references missing model: " +
                        entity.MeshRenderer->ModelPath +
                        " (" +
                        loadError +
                        ")";
                    return;
                }
            }

            if (entity.SpriteRenderer2D && !entity.SpriteRenderer2D->TexturePath.empty())
            {
                const fs::path texturePath =
                    fs::path(assetRoot) / entity.SpriteRenderer2D->TexturePath;
                std::string loadError;
                entity.SpriteRenderer2D->Texture =
                    Assets::LoadTexture2D(texturePath.string(), &loadError);

                if (!entity.SpriteRenderer2D->Texture)
                {
                    error =
                        "Entity " +
                        std::to_string(entity.Id) +
                        " references missing sprite texture: " +
                        entity.SpriteRenderer2D->TexturePath +
                        " (" +
                        loadError +
                        ")";
                    return;
                }
            }
        }
    }

    void Scene::EnsurePhysicsInitialized()
    {
        if (!m_physics2D.IsAvailable())
        {
            m_physics2D.Initialize(Physics2DBackend::Box2D);
        }

        if (!m_physics3D.IsAvailable())
        {
            m_physics3D.Initialize(Physics3DBackend::Jolt);
        }
    }

    void Scene::SyncSceneToPhysics()
    {
        std::vector<EntityId> active2D;
        std::vector<EntityId> active3D;

        for (const SceneEntity& entity : m_entities)
        {
            const Transform world = GetWorldTransform(entity.Id);

            if (entity.BoxCollider2D || entity.CircleCollider2D)
            {
                active2D.push_back(entity.Id);
                m_physics2D.CreateOrUpdateBody(
                    entity.Id,
                    world,
                    entity.RigidBody2D ? &*entity.RigidBody2D : nullptr,
                    entity.BoxCollider2D ? &*entity.BoxCollider2D : nullptr,
                    entity.CircleCollider2D ? &*entity.CircleCollider2D : nullptr,
                    entity.Parent != InvalidEntityId
                );
            }

            if (entity.BoxCollider3D || entity.SphereCollider3D || entity.CapsuleCollider3D)
            {
                active3D.push_back(entity.Id);
                m_physics3D.CreateOrUpdateBody(
                    entity.Id,
                    world,
                    entity.RigidBody3D ? &*entity.RigidBody3D : nullptr,
                    entity.BoxCollider3D ? &*entity.BoxCollider3D : nullptr,
                    entity.SphereCollider3D ? &*entity.SphereCollider3D : nullptr,
                    entity.CapsuleCollider3D ? &*entity.CapsuleCollider3D : nullptr,
                    entity.Parent != InvalidEntityId
                );
            }
        }

        m_physics2D.Prune(active2D);
        m_physics3D.Prune(active3D);
    }

    void Scene::SyncPhysicsToScene()
    {
        for (SceneEntity& entity : m_entities)
        {
            if (
                entity.RigidBody2D &&
                entity.RigidBody2D->Type == BodyType::Dynamic &&
                entity.RigidBody2D->Enabled &&
                entity.Parent == InvalidEntityId
            )
            {
                Transform transform;

                if (m_physics2D.GetBodyTransform(entity.Id, transform))
                {
                    entity.Transform.LocalTransform.Position.X = transform.Position.X;
                    entity.Transform.LocalTransform.Position.Y = transform.Position.Y;
                    entity.Transform.LocalTransform.Rotation.Z = transform.Rotation.Z;
                }
            }

            if (
                entity.RigidBody3D &&
                entity.RigidBody3D->Type == BodyType::Dynamic &&
                entity.RigidBody3D->Enabled &&
                entity.Parent == InvalidEntityId
            )
            {
                Transform transform;

                if (m_physics3D.GetBodyTransform(entity.Id, transform))
                {
                    entity.Transform.LocalTransform = transform;
                }
            }
        }
    }
}
