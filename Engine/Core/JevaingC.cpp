#include <Jevaing/JevaingC.h>
#include <Jevaing/Jevaing.h>
#include <atomic>
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
