#include <Jevaing/JevaingC.h>
#include <stdio.h>
#include <string.h>
typedef struct state { float x; jevaing_texture* texture; int starts,updates,renders2d,renders3d,resizes,stops,errors; } state;
static void start(void* p) { ((state*)p)->starts++; }
static void update(void* p,double dt) {
    state* s=(state*)p; s->updates++;
    if(jevaing_key_down(JEVAING_KEY_D)==1) s->x+=(float)dt;
    if(jevaing_key_down(JEVAING_KEY_A)==1) s->x-=(float)dt;
    if(jevaing_key_down(-100)!=JEVAING_C_INVALID_ARGUMENT) s->errors++;
}
static void render2d(void* p) {
    state* s=(state*)p; s->renders2d++;
    jevaing_color bg={0.04f,0.05f,0.08f,1}, blue={0.2f,0.7f,1,1};
    jevaing_vec2 center={s->x,0}, size={0.25f,0.25f};
    if(jevaing_clear(bg)!=0 || jevaing_draw_quad(center,size,blue)!=0 ||
       jevaing_draw_sprite(s->texture,center,size,blue)!=0) s->errors++;
}
static void render3d(void* p) { ((state*)p)->renders3d++; }
static void resize(void* p,int32_t w,int32_t h) { state* s=(state*)p; s->resizes++; if(w<=0||h<=0) s->errors++; }
static void stop(void* p) { ((state*)p)->stops++; }
static int abi_test(void) {
    jevaing_game_desc d=JEVAING_GAME_DESC_INIT;
    jevaing_color c={0,0,0,1};
    jevaing_scene* scene=NULL;
    jevaing_entity_id parent=0,child=0;
    uint64_t count=0;
    jevaing_transform transform={{1,2,3},{0,0,0},{1,1,1}}, restored;
    jevaing_texture* texture=NULL;
    int32_t width=0,height=0;
    jevaing_vec2 center={0,0}, size={1,1};
    if(jevaing_abi_version()!=JEVAING_C_ABI_VERSION || !jevaing_engine_version() ||
       jevaing_run(NULL,0,NULL)!=JEVAING_C_INVALID_ARGUMENT ||
       jevaing_clear(c)!=JEVAING_C_INVALID_STATE ||
       jevaing_key_down(JEVAING_KEY_A)!=JEVAING_C_INVALID_STATE) return 2;
    d.abi_version=0;
    if(jevaing_run(&d,0,NULL)!=JEVAING_C_INVALID_ARGUMENT) return 2;
    d.abi_version=JEVAING_C_ABI_VERSION; d.struct_size=0;
    if(jevaing_run(&d,0,NULL)!=JEVAING_C_INVALID_ARGUMENT) return 2;
    d.struct_size=(uint32_t)sizeof(d); d.width=0;
    if(jevaing_run(&d,0,NULL)!=JEVAING_C_INVALID_ARGUMENT) return 2;
    if(jevaing_texture_create_checker(8,8,2,&texture)!=JEVAING_C_OK ||
       jevaing_texture_get_size(texture,&width,&height)!=JEVAING_C_OK ||
       width!=8 || height!=8 ||
       jevaing_draw_sprite(texture,center,size,c)!=JEVAING_C_INVALID_STATE ||
       jevaing_texture_destroy(texture)!=JEVAING_C_OK) return 2;
    if(jevaing_scene_create("C scene",&scene)!=JEVAING_C_OK || !scene ||
       jevaing_scene_entity_create(scene,"Parent",&parent)!=JEVAING_C_OK ||
       jevaing_scene_entity_create(scene,"Child",&child)!=JEVAING_C_OK ||
       jevaing_scene_entity_set_parent(scene,child,parent)!=JEVAING_C_OK ||
       jevaing_scene_entity_set_parent(scene,parent,child)!=JEVAING_C_INVALID_ARGUMENT ||
       jevaing_scene_entity_set_transform(scene,child,&transform)!=JEVAING_C_OK ||
       jevaing_scene_entity_get_transform(scene,child,&restored)!=JEVAING_C_OK ||
       restored.position.x!=1 || restored.position.y!=2 || restored.scale.z!=1 ||
       jevaing_scene_entity_count(scene,&count)!=JEVAING_C_OK || count!=2 ||
       jevaing_scene_entity_destroy(scene,parent)!=JEVAING_C_OK ||
       jevaing_scene_entity_destroy(scene,parent)!=JEVAING_C_NOT_FOUND ||
       jevaing_scene_entity_count(scene,&count)!=JEVAING_C_OK || count!=1 ||
       jevaing_scene_destroy(scene)!=JEVAING_C_OK) return 2;
    puts("[PASS] Pure C header, linkage and ABI validation."); return 0;
}
static int scene_roundtrip_test(const char* path) {
    jevaing_scene* scene=NULL;
    jevaing_entity_id id=0;
    jevaing_transform value={{4,5,6},{0,0,0},{1,1,1}}, loaded={0};
    uint64_t count=0;
    int ok=1;
    if(jevaing_scene_create("C roundtrip",&scene)!=JEVAING_C_OK) return 2;
    ok &= jevaing_scene_entity_create(scene,"Player",&id)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_transform(scene,id,&value)==JEVAING_C_OK;
    ok &= jevaing_scene_save(scene,path)==JEVAING_C_OK;
    jevaing_scene_destroy(scene); scene=NULL;
    if(jevaing_scene_create("empty",&scene)!=JEVAING_C_OK) return 2;
    ok &= jevaing_scene_load(scene,path,"")==JEVAING_C_OK;
    ok &= jevaing_scene_entity_count(scene,&count)==JEVAING_C_OK && count==1;
    ok &= jevaing_scene_entity_get_transform(scene,id,&loaded)==JEVAING_C_OK;
    ok &= loaded.position.x==4 && loaded.position.y==5 && loaded.position.z==6;
    jevaing_scene_destroy(scene);
    remove(path);
    if(ok) puts("[PASS] Pure C scene save/load roundtrip.");
    return ok ? 0 : 2;
}
static int physics_test(void) {
    jevaing_scene* scene=NULL;
    jevaing_entity_id ground=0, player=0;
    jevaing_transform ground_t={{0,-0.5f,0},{0,0,0},{1,1,1}};
    jevaing_transform player_t={{0,5,0},{0,0,0},{1,1,1}}, result={0};
    jevaing_vec2 ground_size={10,1}, player_size={1,1}, velocity2={0.25f,0}, read2={0};
    jevaing_vec3 ground_size3={10,1,10}, player_size3={1,1,1}, velocity3={0.25f,0,0}, read3={0};
    int ok=1,i;
    if(jevaing_scene_create("physics-2d",&scene)!=JEVAING_C_OK) return 2;
    ok &= jevaing_scene_entity_create(scene,"Ground",&ground)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_create(scene,"Player",&player)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_transform(scene,ground,&ground_t)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_transform(scene,player,&player_t)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_body2d(scene,ground,JEVAING_BODY_STATIC)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_body2d(scene,player,JEVAING_BODY_DYNAMIC)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_box2d(scene,ground,ground_size)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_box2d(scene,player,player_size)==JEVAING_C_OK;
    ok &= jevaing_scene_update(scene,0)==JEVAING_C_OK;
    ok &= jevaing_scene_body2d_set_velocity(scene,player,velocity2)==JEVAING_C_OK;
    ok &= jevaing_scene_body2d_get_velocity(scene,player,&read2)==JEVAING_C_OK;
    ok &= read2.x>0.24f && read2.x<0.26f;
    for(i=0;i<240;++i) ok &= jevaing_scene_update(scene,1.0/60.0)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_get_transform(scene,player,&result)==JEVAING_C_OK;
    ok &= result.position.y>0.3f && result.position.y<0.8f;
    jevaing_scene_destroy(scene); scene=NULL;

    if(jevaing_scene_create("physics-3d",&scene)!=JEVAING_C_OK) return 2;
    ok &= jevaing_scene_entity_create(scene,"Ground",&ground)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_create(scene,"Player",&player)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_transform(scene,ground,&ground_t)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_transform(scene,player,&player_t)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_body3d(scene,ground,JEVAING_BODY_STATIC)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_body3d(scene,player,JEVAING_BODY_DYNAMIC)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_box3d(scene,ground,ground_size3)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_set_box3d(scene,player,player_size3)==JEVAING_C_OK;
    ok &= jevaing_scene_update(scene,0)==JEVAING_C_OK;
    ok &= jevaing_scene_body3d_set_velocity(scene,player,velocity3)==JEVAING_C_OK;
    ok &= jevaing_scene_body3d_get_velocity(scene,player,&read3)==JEVAING_C_OK;
    ok &= read3.x>0.24f && read3.x<0.26f;
    for(i=0;i<240;++i) ok &= jevaing_scene_update(scene,1.0/60.0)==JEVAING_C_OK;
    ok &= jevaing_scene_entity_get_transform(scene,player,&result)==JEVAING_C_OK;
    ok &= result.position.y>0.3f && result.position.y<0.8f;
    jevaing_scene_destroy(scene);
    if(ok) puts("[PASS] Pure C 2D and 3D physics.");
    return ok ? 0 : 2;
}
static int asset_test(const char* path) {
    jevaing_texture* texture=NULL;
    int32_t width=0,height=0;
    int ok=jevaing_texture_load(path,&texture)==JEVAING_C_OK && texture!=NULL;
    if(ok) ok=jevaing_texture_get_size(texture,&width,&height)==JEVAING_C_OK &&
        width>0 && height>0;
    if(texture) jevaing_texture_destroy(texture);
    if(ok) puts("[PASS] Pure C texture loading.");
    return ok ? 0 : 2;
}
int main(int argc,char** argv) {
    state s={0}; jevaing_game_desc d=JEVAING_GAME_DESC_INIT;
    int runtime_test=0,i;
    for(i=1;i<argc;++i) {
        if(strcmp(argv[i],"--abi-test")==0) return abi_test();
        if(strcmp(argv[i],"--scene-roundtrip-test")==0)
            return i+1<argc ? scene_roundtrip_test(argv[i+1]) : 2;
        if(strcmp(argv[i],"--physics-test")==0) return physics_test();
        if(strcmp(argv[i],"--asset-test")==0)
            return i+1<argc ? asset_test(argv[i+1]) : 2;
        if(strcmp(argv[i],"--runtime-test")==0) runtime_test=1;
    }
    d.user_data=&s; d.on_start=start; d.on_update=update;
    d.on_render_2d=render2d; d.on_render_3d=render3d; d.on_resize=resize; d.on_stop=stop;
    if(jevaing_texture_create_checker(16,16,4,&s.texture)!=JEVAING_C_OK) return 2;
    {
        const int32_t result=jevaing_run(&d,argc,argv);
        jevaing_texture_destroy(s.texture);
        if(result!=0) return 2;
    }
    if(runtime_test && (s.starts!=1||s.stops!=1||s.updates<1||s.renders2d<1||s.renders3d<1||s.resizes<1||s.errors)) return 2;
    if(runtime_test) puts("[PASS] Pure C game lifecycle.");
    return 0;
}
