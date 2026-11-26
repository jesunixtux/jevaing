#include <Jevaing/JevaingC.h>
#include <stdio.h>
#include <string.h>
typedef struct state { float x; int starts,updates,renders2d,renders3d,resizes,stops,errors; } state;
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
    if(jevaing_clear(bg)!=0 || jevaing_draw_quad(center,size,blue)!=0) s->errors++;
}
static void render3d(void* p) { ((state*)p)->renders3d++; }
static void resize(void* p,int32_t w,int32_t h) { state* s=(state*)p; s->resizes++; if(w<=0||h<=0) s->errors++; }
static void stop(void* p) { ((state*)p)->stops++; }
static int abi_test(void) {
    jevaing_game_desc d=JEVAING_GAME_DESC_INIT;
    jevaing_color c={0,0,0,1};
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
    puts("[PASS] Pure C header, linkage and ABI validation."); return 0;
}
int main(int argc,char** argv) {
    state s={0}; jevaing_game_desc d=JEVAING_GAME_DESC_INIT;
    int runtime_test=0,i;
    for(i=1;i<argc;++i) {
        if(strcmp(argv[i],"--abi-test")==0) return abi_test();
        if(strcmp(argv[i],"--runtime-test")==0) runtime_test=1;
    }
    d.user_data=&s; d.on_start=start; d.on_update=update;
    d.on_render_2d=render2d; d.on_render_3d=render3d; d.on_resize=resize; d.on_stop=stop;
    if(jevaing_run(&d,argc,argv)!=0) return 2;
    if(runtime_test && (s.starts!=1||s.stops!=1||s.updates<1||s.renders2d<1||s.renders3d<1||s.resizes<1||s.errors)) return 2;
    if(runtime_test) puts("[PASS] Pure C game lifecycle.");
    return 0;
}
