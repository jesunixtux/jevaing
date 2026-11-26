#include <Jevaing/JevaingC.h>
#include <Jevaing/Jevaing.h>
#include <atomic>
#include <cmath>
#include <memory>
#include <utility>
struct jevaing_scene {
    Jevaing::Scene value;
    explicit jevaing_scene(const char* name) : value(name) {}
};
struct jevaing_texture {
    std::shared_ptr<const Jevaing::Texture2D> value;
    explicit jevaing_texture(std::shared_ptr<const Jevaing::Texture2D> texture)
        : value(std::move(texture)) {}
};
namespace {
    std::atomic_flag running = ATOMIC_FLAG_INIT;
    struct RunGuard { ~RunGuard() { running.clear(); } };
    thread_local bool inCallback = false;
    thread_local Jevaing::Graphics2D* g2 = nullptr;
    thread_local Jevaing::Graphics3D* g3 = nullptr;
    struct Scope {
        Scope(Jevaing::Graphics2D* a=nullptr, Jevaing::Graphics3D* b=nullptr) {
            inCallback=true; g2=a; g3=b;
        }
        ~Scope() { inCallback=false; g2=nullptr; g3=nullptr; }
    };
    Jevaing::Color color(jevaing_color c) { return {c.r,c.g,c.b,c.a}; }
    Jevaing::Vec3 vec(jevaing_vec3 v) { return {v.x,v.y,v.z}; }
    bool body_type(int32_t value, Jevaing::BodyType& out) {
        switch (value) {
            case JEVAING_BODY_STATIC: out=Jevaing::BodyType::Static; return true;
            case JEVAING_BODY_KINEMATIC: out=Jevaing::BodyType::Kinematic; return true;
            case JEVAING_BODY_DYNAMIC: out=Jevaing::BodyType::Dynamic; return true;
            default: return false;
        }
    }
    class CGame final : public Jevaing::Game {
        jevaing_game_desc d;
    public:
        explicit CGame(const jevaing_game_desc& desc): d(desc) {}
        void OnStart() override { Scope s; if(d.on_start) d.on_start(d.user_data); }
        void OnUpdate(double dt) override { Scope s; if(d.on_update) d.on_update(d.user_data,dt); }
        void OnRender(Jevaing::Graphics2D& g) override { Scope s(&g); if(d.on_render_2d) d.on_render_2d(d.user_data); }
        void OnRender(Jevaing::Graphics3D& g) override { Scope s(nullptr,&g); if(d.on_render_3d) d.on_render_3d(d.user_data); }
        void OnResize(int w,int h) override { Scope s; if(d.on_resize) d.on_resize(d.user_data,w,h); }
        void OnStop() override { Scope s; if(d.on_stop) d.on_stop(d.user_data); }
    };
}
extern "C" uint32_t jevaing_abi_version(void) { return JEVAING_C_ABI_VERSION; }
extern "C" const char* jevaing_engine_version(void) { return Jevaing::GetVersion(); }
extern "C" int32_t jevaing_run(const jevaing_game_desc* d,int argc,char** argv) {
    if(!d || d->abi_version!=JEVAING_C_ABI_VERSION || d->struct_size<sizeof(jevaing_game_desc) ||
       d->width<=0 || d->height<=0 || argc<0 || (argc>0 && !argv)) return JEVAING_C_INVALID_ARGUMENT;
    if(running.test_and_set()) return JEVAING_C_INVALID_STATE;
    RunGuard guard;
    try {
        CGame game(*d);
        Jevaing::GameConfig config;
        if(d->title) config.Title=d->title;
        config.Width=d->width; config.Height=d->height;
        return Jevaing::Run(game,config,argc,argv);
    } catch(...) { return JEVAING_C_INTERNAL_ERROR; }
}
extern "C" int32_t jevaing_key_down(jevaing_key key) {
    if(!inCallback) return JEVAING_C_INVALID_STATE;
    Jevaing::Key k;
    switch(key) {
        case JEVAING_KEY_W: k=Jevaing::Key::W; break;
        case JEVAING_KEY_A: k=Jevaing::Key::A; break;
        case JEVAING_KEY_S: k=Jevaing::Key::S; break;
        case JEVAING_KEY_D: k=Jevaing::Key::D; break;
        case JEVAING_KEY_UP: k=Jevaing::Key::Up; break;
        case JEVAING_KEY_DOWN: k=Jevaing::Key::Down; break;
        case JEVAING_KEY_LEFT: k=Jevaing::Key::Left; break;
        case JEVAING_KEY_RIGHT: k=Jevaing::Key::Right; break;
        case JEVAING_KEY_SPACE: k=Jevaing::Key::Space; break;
        case JEVAING_KEY_ENTER: k=Jevaing::Key::Enter; break;
        case JEVAING_KEY_ESCAPE: k=Jevaing::Key::Escape; break;
        default: return JEVAING_C_INVALID_ARGUMENT;
    }
    try { return Jevaing::Input::IsKeyDown(k)?1:0; }
    catch(...) { return JEVAING_C_INTERNAL_ERROR; }
}
extern "C" int32_t jevaing_clear(jevaing_color c) {
    if(!g2) return JEVAING_C_INVALID_STATE;
    try { g2->Clear(color(c)); return JEVAING_C_OK; } catch(...) { return JEVAING_C_INTERNAL_ERROR; }
}
extern "C" int32_t jevaing_draw_quad(jevaing_vec2 p,jevaing_vec2 s,jevaing_color c) {
    if(!g2) return JEVAING_C_INVALID_STATE;
    try { g2->DrawQuad({p.x,p.y},{s.x,s.y},color(c)); return JEVAING_C_OK; } catch(...) { return JEVAING_C_INTERNAL_ERROR; }
}
extern "C" int32_t jevaing_draw_cube(jevaing_transform t,jevaing_color c) {
    if(!g3) return JEVAING_C_INVALID_STATE;
    try { Jevaing::Transform x; x.Position=vec(t.position); x.Rotation=vec(t.rotation); x.Scale=vec(t.scale);
        g3->DrawCube(x,color(c)); return JEVAING_C_OK; } catch(...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_texture_load(const char* path, jevaing_texture** out) {
    if (!path || !*path || !out) return JEVAING_C_INVALID_ARGUMENT;
    *out=nullptr;
    try {
        auto texture=Jevaing::Assets::LoadTexture2D(path);
        if (!texture) return JEVAING_C_IO_ERROR;
        *out=new jevaing_texture(std::move(texture));
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_texture_create_checker(int32_t width, int32_t height,
    int32_t cell_size, jevaing_texture** out) {
    if (!out || width<=0 || height<=0 || cell_size<=0) return JEVAING_C_INVALID_ARGUMENT;
    *out=nullptr;
    try {
        auto texture=Jevaing::Assets::CreateCheckerTexture(width,height,cell_size);
        if (!texture) return JEVAING_C_INTERNAL_ERROR;
        *out=new jevaing_texture(std::move(texture));
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_texture_get_size(const jevaing_texture* texture,
    int32_t* width, int32_t* height) {
    if (!texture || !width || !height) return JEVAING_C_INVALID_ARGUMENT;
    *width=texture->value->Width;
    *height=texture->value->Height;
    return JEVAING_C_OK;
}

extern "C" int32_t jevaing_texture_destroy(jevaing_texture* texture) {
    if (!texture) return JEVAING_C_INVALID_ARGUMENT;
    try { delete texture; return JEVAING_C_OK; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_draw_sprite(const jevaing_texture* texture,
    jevaing_vec2 center, jevaing_vec2 size, jevaing_color tint) {
    if (!g2) return JEVAING_C_INVALID_STATE;
    if (!texture || !std::isfinite(size.x) || !std::isfinite(size.y) ||
        size.x<=0 || size.y<=0) return JEVAING_C_INVALID_ARGUMENT;
    try {
        g2->DrawSprite(texture->value,{center.x,center.y},{size.x,size.y},color(tint));
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_create(const char* name, jevaing_scene** out_scene) {
    if (!name || !out_scene) return JEVAING_C_INVALID_ARGUMENT;
    *out_scene = nullptr;
    try { *out_scene = new jevaing_scene(name); return JEVAING_C_OK; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_destroy(jevaing_scene* scene) {
    if (!scene) return JEVAING_C_INVALID_ARGUMENT;
    try { delete scene; return JEVAING_C_OK; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_load(jevaing_scene* scene, const char* path, const char* asset_root) {
    if (!scene || !path || !*path || !asset_root) return JEVAING_C_INVALID_ARGUMENT;
    try {
        std::string error;
        return scene->value.Load(path, asset_root, error) ? JEVAING_C_OK : JEVAING_C_IO_ERROR;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_save(const jevaing_scene* scene, const char* path) {
    if (!scene || !path || !*path) return JEVAING_C_INVALID_ARGUMENT;
    try {
        std::string error;
        return scene->value.Save(path, error) ? JEVAING_C_OK : JEVAING_C_IO_ERROR;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_count(const jevaing_scene* scene, uint64_t* out_count) {
    if (!scene || !out_count) return JEVAING_C_INVALID_ARGUMENT;
    try { *out_count = static_cast<uint64_t>(scene->value.GetEntities().size()); return JEVAING_C_OK; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_create(jevaing_scene* scene, const char* name,
    jevaing_entity_id* out_id) {
    if (!scene || !name || !out_id) return JEVAING_C_INVALID_ARGUMENT;
    *out_id = JEVAING_INVALID_ENTITY_ID;
    try {
        *out_id = scene->value.CreateEntity(name);
        return *out_id ? JEVAING_C_OK : JEVAING_C_INTERNAL_ERROR;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_destroy(jevaing_scene* scene, jevaing_entity_id id) {
    if (!scene || !id) return JEVAING_C_INVALID_ARGUMENT;
    try { return scene->value.DestroyEntity(id) ? JEVAING_C_OK : JEVAING_C_NOT_FOUND; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_parent(jevaing_scene* scene,
    jevaing_entity_id child, jevaing_entity_id parent) {
    if (!scene || !child || child == parent) return JEVAING_C_INVALID_ARGUMENT;
    try {
        if (!scene->value.FindEntity(child) || (parent && !scene->value.FindEntity(parent)))
            return JEVAING_C_NOT_FOUND;
        if (!parent) return scene->value.RemoveParent(child) ? JEVAING_C_OK : JEVAING_C_NOT_FOUND;
        return scene->value.SetParent(child, parent) ? JEVAING_C_OK : JEVAING_C_INVALID_ARGUMENT;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_get_transform(const jevaing_scene* scene,
    jevaing_entity_id id, jevaing_transform* out_transform) {
    if (!scene || !id || !out_transform) return JEVAING_C_INVALID_ARGUMENT;
    try {
        const auto* entity = scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        const auto& t = entity->Transform.LocalTransform;
        *out_transform = {{t.Position.X,t.Position.Y,t.Position.Z},
            {t.Rotation.X,t.Rotation.Y,t.Rotation.Z}, {t.Scale.X,t.Scale.Y,t.Scale.Z}};
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_transform(jevaing_scene* scene,
    jevaing_entity_id id, const jevaing_transform* transform) {
    if (!scene || !id || !transform) return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto* entity = scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        auto& t = entity->Transform.LocalTransform;
        t.Position = vec(transform->position);
        t.Rotation = vec(transform->rotation);
        t.Scale = vec(transform->scale);
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_body2d(jevaing_scene* scene,
    jevaing_entity_id id, int32_t type) {
    Jevaing::BodyType parsed;
    if (!scene || !id || !body_type(type, parsed)) return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto* entity=scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        entity->RigidBody2D=Jevaing::RigidBody2DComponent{};
        entity->RigidBody2D->Type=parsed;
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_box2d(jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec2 size) {
    if (!scene || !id || !std::isfinite(size.x) || !std::isfinite(size.y) ||
        size.x<=0 || size.y<=0) return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto* entity=scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        entity->BoxCollider2D=Jevaing::BoxCollider2DComponent{};
        entity->BoxCollider2D->Size={size.x,size.y};
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_body3d(jevaing_scene* scene,
    jevaing_entity_id id, int32_t type) {
    Jevaing::BodyType parsed;
    if (!scene || !id || !body_type(type, parsed)) return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto* entity=scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        entity->RigidBody3D=Jevaing::RigidBody3DComponent{};
        entity->RigidBody3D->Type=parsed;
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_entity_set_box3d(jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec3 size) {
    if (!scene || !id || !std::isfinite(size.x) || !std::isfinite(size.y) ||
        !std::isfinite(size.z) || size.x<=0 || size.y<=0 || size.z<=0)
        return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto* entity=scene->value.FindEntity(id);
        if (!entity) return JEVAING_C_NOT_FOUND;
        entity->BoxCollider3D=Jevaing::BoxCollider3DComponent{};
        entity->BoxCollider3D->Size={size.x,size.y,size.z};
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_update(jevaing_scene* scene, double dt) {
    if (!scene || !std::isfinite(dt) || dt<0) return JEVAING_C_INVALID_ARGUMENT;
    try { scene->value.Update(dt); return JEVAING_C_OK; }
    catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_body2d_set_velocity(jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec2 velocity) {
    if (!scene || !id || !std::isfinite(velocity.x) || !std::isfinite(velocity.y))
        return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto& world=scene->value.Physics2D();
        if (!world.IsAvailable() || !world.HasBody(id)) return JEVAING_C_INVALID_STATE;
        world.SetLinearVelocity(id,{velocity.x,velocity.y});
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_body2d_get_velocity(const jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec2* out) {
    if (!scene || !id || !out) return JEVAING_C_INVALID_ARGUMENT;
    try {
        const auto& world=scene->value.Physics2D();
        if (!world.IsAvailable() || !world.HasBody(id)) return JEVAING_C_INVALID_STATE;
        const auto value=world.GetLinearVelocity(id);
        *out={value.X,value.Y}; return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_body3d_set_velocity(jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec3 velocity) {
    if (!scene || !id || !std::isfinite(velocity.x) || !std::isfinite(velocity.y) ||
        !std::isfinite(velocity.z)) return JEVAING_C_INVALID_ARGUMENT;
    try {
        auto& world=scene->value.Physics3D();
        if (!world.IsAvailable() || !world.HasBody(id)) return JEVAING_C_INVALID_STATE;
        world.SetLinearVelocity(id,vec(velocity));
        return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}

extern "C" int32_t jevaing_scene_body3d_get_velocity(const jevaing_scene* scene,
    jevaing_entity_id id, jevaing_vec3* out) {
    if (!scene || !id || !out) return JEVAING_C_INVALID_ARGUMENT;
    try {
        const auto& world=scene->value.Physics3D();
        if (!world.IsAvailable() || !world.HasBody(id)) return JEVAING_C_INVALID_STATE;
        const auto value=world.GetLinearVelocity(id);
        *out={value.X,value.Y,value.Z}; return JEVAING_C_OK;
    } catch (...) { return JEVAING_C_INTERNAL_ERROR; }
}
