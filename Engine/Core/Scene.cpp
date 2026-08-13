#include <Jevaing/Scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

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

    void Scene::Update(double)
    {
        if (!m_started)
        {
            OnStart();
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
                current->Id = static_cast<EntityId>(std::stoull(value));
            }
            else if (key == "name")
            {
                current->Name = value;
            }
            else if (key == "parent")
            {
                parentLinks.push_back({
                    current->Id,
                    static_cast<EntityId>(std::stoull(value))
                });
            }
            else if (key == "position")
            {
                ParseVec3(value, current->Transform.LocalTransform.Position);
            }
            else if (key == "rotation")
            {
                ParseVec3(value, current->Transform.LocalTransform.Rotation);
            }
            else if (key == "scale")
            {
                ParseVec3(value, current->Transform.LocalTransform.Scale);
            }
            else if (key == "camera.primary")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                current->Camera->Primary = value == "1" || value == "true";
            }
            else if (key == "camera.fov")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                current->Camera->Camera.VerticalFovRadians = std::stof(value);
            }
            else if (key == "camera.near")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                current->Camera->Camera.NearPlane = std::stof(value);
            }
            else if (key == "camera.far")
            {
                current->Camera = current->Camera.value_or(CameraComponent{});
                current->Camera->Camera.FarPlane = std::stof(value);
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
                ParseColor(value, current->MeshRenderer->MaterialOverride.BaseColor);
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
                ParseVec2(value, current->SpriteRenderer2D->Size);
            }
            else if (key == "sprite.tint")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                ParseColor(value, current->SpriteRenderer2D->Tint);
            }
            else if (key == "sprite.layer")
            {
                current->SpriteRenderer2D =
                    current->SpriteRenderer2D.value_or(SpriteRenderer2DComponent{});
                current->SpriteRenderer2D->Layer = std::stoi(value);
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
}
