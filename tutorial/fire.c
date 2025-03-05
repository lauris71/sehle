#define __HELLO_C__

/*
 * sehle test
 */

#include <stdio.h>

#include <arikkei/arikkei-iolib.h>

#include <elea/aabox.h>
#include <elea/color.h>
#include <elea/geometry.h>
#include <elea/matrix3x4.h>
#include <elea/matrix4x4.h>
#include <elea/vector3.h>
#include <nr/pixblock.h>
#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/material-control.h>
#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/render-target.h>
#include <sehle/shader.h>
#include <sehle/static-mesh.h>
#include <sehle/texture-2d.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include <SDL2/SDL.h>

static SDL_Window* window;
static SDL_GLContext gl_context;

static int width = 800;
static int height = 600;

#define LAYER_FIRE 1

static void
setup_sdl()
{
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Cannot initialize SDL\n");
        exit(1);
    }
    atexit(SDL_Quit);
    if (!SDL_GL_LoadLibrary(NULL)) {
        fprintf(stderr, "SDL_GL_LoadLibrary:%s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Request an OpenGL 4.5 context (should be core)
    if (!SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1)) {
        fprintf(stderr, "SDL_GL_SetAttribute:%s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4)) {
        fprintf(stderr, "SDL_GL_SetAttribute:%s\n", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1)) {
        fprintf(stderr, "SDL_GL_SetAttribute:%s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow("Sehle Tutorial 2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow:%s\n", SDL_GetError());
    }
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext:%s\n", SDL_GetError());
        exit(1);
    }
    int major, minor;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    fprintf(stderr, "OpenGL version %d.%d\n", major, minor);
    SDL_GL_SetSwapInterval(0);
}

typedef struct _TGAHeader TGAHeader;

struct _TGAHeader {
    uint8_t ID_len;
    uint8_t CMap_type;
    uint8_t Img_type;
    uint8_t Cmap[5];
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t depth;
    uint8_t desc;
    uint8_t pixels[];
};

static void
read_texture(NRPixBlock *pxb, const char *filename)
{
    uint64_t csize;
    const uint8_t *cdata = arikkei_mmap((const uint8_t *) filename, &csize);
    if (!cdata) {
        fprintf(stderr, "Cannot read %s\n", filename);
        exit(1);
    }
    TGAHeader *hdr = (TGAHeader *) cdata;
#if 1
    nr_pixblock_setup_transient(pxb, NR_PIXBLOCK_MODE_R8G8B8A8N,
        hdr->x_origin, hdr->y_origin, hdr->x_origin + hdr->width, hdr->y_origin + hdr->height, 0);
    for (uint16_t row = 0; row < hdr->height; row++) {
        const uint8_t *s = hdr->pixels + row * hdr->width * 4;
        uint8_t *d = nr_pixblock_get_row(pxb, row);
        for (uint16_t col = 0; col < hdr->width; col++) {
            *d++ = s[2];
            *d++ = s[1];
            *d++ = s[0];
            *d++ = s[3];
            s += 4;
        }
    }
#else
    nr_pixblock_setup_extern(pxb, NR_PIXBLOCK_MODE_R8G8B8A8N,
        hdr->x_origin, hdr->y_origin, hdr->x_origin + hdr->width, hdr->y_origin + hdr->height,
        hdr->pixels, hdr->width * 4, 0, 0);
#endif
}

enum {
    O2V_PROJECTION, O2W,
    CAMERA_POSITION,
    COLOR, TIME, SEED,
    INVMODELMATRIX,
    SCALE, NOISE_SCALE, MAGNITUDE, LACUNARITY, GAIN,
    FIRE_TEX,
    NUM_UNIFORMS
};

const char *uniforms[] = {
    "o2v_projection", "o2w",
    "cameraPosition",
    "color", "time", "seed",
    "invModelMatrix",
    "scale", "noiseScale", "magnitude", "lacunarity", "gain",
    "fireTex"
};

typedef struct _FireImpl FireImpl;
typedef struct _FireInst FireInst;

struct _FireImpl {
    SehleRenderableImplementation rend_impl;
    SehleMaterialImplementation mat_impl;
};

struct _FireInst {
    SehleRenderableInstance rend_inst;
    SehleMaterialInstance mat_inst;

    SehleVertexArray *va;

    EleaMat3x4f r2w;
    EleaColor4f color;
    float time;
    float seed;
    EleaVec3f scale;
    EleaVec4f noiseScale;
    float magnitude;
    float lacunarity;
    float gain;
};

static void
fire_mat_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
    FireInst *fire = (FireInst *) ARIKKEI_BASE_ADDRESS(FireInst, mat_inst, inst);
	SehleProgram *prog = inst->programs[0];
	sehle_render_context_set_program (ctx, prog);
    EleaMat3x4f o2v;
    elea_mat3x4f_multiply(&o2v, &ctx->w2v, &fire->r2w);
    EleaMat4x4f o2v_proj;
    elea_mat4x4f_multiply_mat3x4(&o2v_proj, &ctx->proj, &o2v);
	sehle_program_setUniformMatrix4fv(prog, O2V_PROJECTION, 1, o2v_proj.c);
    sehle_program_setUniformMatrix4x3fv(prog, O2W, 1, 1, fire->r2w.c);
    EleaVec3f cam_pos;
    elea_mat3x4f_get_translation(&cam_pos, &ctx->v2w);
    sehle_program_setUniform3fv(prog, CAMERA_POSITION, 1, cam_pos.c);
    sehle_program_setUniform3fv(prog, COLOR, 1, fire->color.c);
    sehle_program_setUniform1f(prog, TIME, fire->time);
    sehle_program_setUniform1f(prog, SEED, fire->seed);
    EleaMat3x4f w2o;
    elea_mat3x4f_invert_normalized(&w2o, &fire->r2w);
    sehle_program_setUniformMatrix4x3fv(prog, INVMODELMATRIX, 1, 1, w2o.c);
    sehle_program_setUniform3fv(prog, SCALE, 1, fire->scale.c);
    sehle_program_setUniform4fv(prog, NOISE_SCALE, 1, fire->noiseScale.c);
    sehle_program_setUniform1f(prog, MAGNITUDE, fire->magnitude);
    sehle_program_setUniform1f(prog, LACUNARITY, fire->lacunarity);
    sehle_program_setUniform1f(prog, GAIN, fire->gain);
	sehle_material_instance_bind_texture (inst, ctx, 0, 0, FIRE_TEX);
}

static void
fire_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
    FireInst *fire = (FireInst *) ARIKKEI_BASE_ADDRESS(FireInst, rend_inst, inst);
    FireImpl *fire_impl = (FireImpl *) ARIKKEI_BASE_ADDRESS(FireImpl, rend_impl, impl);
    sehle_render_context_schedule_render (ctx, impl, inst, &fire_impl->mat_impl, &fire->mat_inst, NULL);
}

static void
fire_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
    FireInst *fire = (FireInst *) ARIKKEI_BASE_ADDRESS(FireInst, rend_inst, inst);
	sehle_vertex_array_render_triangles (fire->va, 1, 0, fire->va->ibuf->buffer.n_elements);
}


static void
fire_initialize(FireImpl *impl, FireInst *inst, SehleEngine *engine)
{
    az_implementation_init(&impl->rend_impl.interface_impl, SEHLE_TYPE_RENDERABLE);
    az_implementation_init(&impl->mat_impl.parent_implementation, SEHLE_TYPE_MATERIAL);
    az_interface_init(&impl->rend_impl.interface_impl, &inst->rend_inst);
    az_interface_init(&impl->mat_impl.parent_implementation, &inst->mat_inst);
    sehle_material_setup(&inst->mat_inst, 1, 1);

    inst->r2w = EleaMat3x4fIdentity;
    inst->color = (EleaColor4f) {0.75f, 0.75f, 0.75f, 1};
    inst->time = 0;
    inst->seed = (float) (rand() / (RAND_MAX + 1.0)) * 19.19f;
    inst->scale = (EleaVec3f) {1, 1, 1};
    inst->noiseScale = (EleaVec4f) {1, 2, 1, 0.3f};
    inst->magnitude = 2.5f;
    inst->lacunarity = 3.0f;
    inst->gain = 0.6f;

    /* Material */
    impl->mat_impl.bind = fire_mat_bind;
	inst->mat_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
	inst->mat_inst.render_types = SEHLE_RENDER_TRANSPARENT;
    SehleShader *vshader = sehle_shader_new(engine, SEHLE_SHADER_VERTEX, (const uint8_t *) "fire-vertex");
    sehle_shader_build_from_file(vshader, (const uint8_t *) "fire-vertex.glsl");
    SehleShader *fshader = sehle_shader_new(engine, SEHLE_SHADER_FRAGMENT, (const uint8_t *) "fire-fragment");
    sehle_shader_build_from_file(fshader, (const uint8_t *) "fire-fragment.glsl");
	SehleProgram *prog = sehle_engine_get_program (engine, "fire-program", 1, 1, NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, vshader);
		sehle_program_add_shader (prog, fshader);
		sehle_program_set_uniform_names (prog, 0, NUM_UNIFORMS, (const uint8_t **) uniforms);
	}
    inst->mat_inst.programs[0] = prog;

    NRPixBlock pxb;
    read_texture(&pxb, "fire.tga");
    SehleTexture2D *tex = sehle_texture_2d_new(engine, (const uint8_t *) "Crate diffuse");
    sehle_texture_2d_set_pixels_from_pixblock(tex, &pxb);
    sehle_material_set_texture(&inst->mat_inst, 0, &tex->texture);
    nr_pixblock_release(&pxb);

    /* Renderable */
#define LOWER_RADIUS 0.5f
#define UPPER_RADIUS 0.25f
#define N_CORNERS 16
    impl->rend_impl.display = fire_display;
    impl->rend_impl.render = fire_render;
    inst->rend_inst.layer_mask = LAYER_FIRE;
    inst->rend_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
    inst->rend_inst.bbox = (EleaAABox3f) {-2, -2, -2, 2, 2, 2};
    /* Create cone */
    SehleVertexBuffer *vb = sehle_vertex_buffer_new(engine, NULL, SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vb, 2 * N_CORNERS, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, -1);
    float *attrs = sehle_vertex_buffer_map(vb, SEHLE_BUFFER_WRITE);
    unsigned int idx = 0;
    for (int i = 0; i < 16; i++) {
        attrs[idx++] = LOWER_RADIUS * cosf(i * 2 * M_PI / 16);
        attrs[idx++] = LOWER_RADIUS * sinf(i * 2 * M_PI / 16);
        attrs[idx++] = 0;
        attrs[idx++] = cosf(i * 2 * M_PI / 16);
        attrs[idx++] = sinf(i * 2 * M_PI / 16);
        attrs[idx++] = 0;
    }
    for (int i = 0; i < 16; i++) {
        attrs[idx++] = UPPER_RADIUS * cosf(i * 2 * M_PI / 16);
        attrs[idx++] = UPPER_RADIUS * sinf(i * 2 * M_PI / 16);
        attrs[idx++] = 1;
        attrs[idx++] = cosf(i * 2 * M_PI / 16);
        attrs[idx++] = sinf(i * 2 * M_PI / 16);
        attrs[idx++] = 0;
    }
    sehle_vertex_buffer_unmap(vb);
    SehleIndexBuffer *ib = sehle_index_buffer_new(engine, NULL, SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ib, 6 * N_CORNERS);
    uint32_t *indices = sehle_index_buffer_map(ib, SEHLE_BUFFER_WRITE);
    for (int i = 0; i < N_CORNERS; i++) {
        *indices++ = i;
        *indices++ = (i + 1) % N_CORNERS;
        *indices++ = (i + 1) % N_CORNERS + N_CORNERS;
        *indices++ = i;
        *indices++ = (i + 1) % N_CORNERS + N_CORNERS;
        *indices++ = i + N_CORNERS;
    }
    sehle_index_buffer_unmap(ib);
    inst->va = sehle_vertex_array_new_from_buffers(engine, NULL, vb, ib);
}

int
main(int argc, const char **argv)
{
    setup_sdl();

    /* 
     * Create sehle engine and ensure it is running
     */
    SehleEngine *engine = sehle_engine_new();
    sehle_engine_ensure_started(engine);
    if (!engine->running) {
        fprintf (stderr, "main:Cannot start sehle engine (invalid/missing OpenGL context?)");
        exit(1);
    }

    /*
     * Create a render target for drawing into vieport
     * It has to be resized manually when window size changes
     * Later we bind appropriate event for this
     */
    SehleRenderTarget *tgt = sehle_render_target_new_viewport(engine, 800, 600);

    /*
     * Create render context
     * It manages automatically:
     *  - low level engine state (drawing flags, render target, viewport, program)
     *  - high-level state for material system
     * If renderables, material system and renderers are not used it is not needed
     */
    SehleRenderContext ctx;
    sehle_render_context_setup(&ctx, engine);
    sehle_render_context_set_target (&ctx, tgt);
    sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
    
    FireImpl fire_impl;
    FireInst fire_inst;
    fire_initialize(&fire_impl, &fire_inst, engine);

    /*
     * Set up view and projection matrices
     */
    float cam_dist = 3;
    float cam_angle_z = 0;
    float cam_angle_x = 0;
    EleaMat3x4f v2w, m0, m1;
    m0 = EleaMat3x4fIdentity;
    elea_mat3x4f_rotate_right_axis_angle(&m1, &m0, &EleaVec3fX, M_PI_2 - cam_angle_x);
    elea_mat3x4f_rotate_left_axis_angle(&v2w, &m1, &EleaVec3fZ, cam_angle_z);
    elea_mat3x4f_translate_self_right_xyz(&v2w, 0, 0, cam_dist);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 0.1f, (float) width / height, 0.1f, 100.0f);
    /* Update view parameters in render context  */
    sehle_render_context_set_view (&ctx, &v2w, &proj);

    double start = arikkei_get_time();
    double time;
    unsigned int alive = 1;
    while (alive) {
        static unsigned int button = 0;
        SDL_Event evt;
        SDL_PollEvent(&evt);
        switch (evt.type) {
            case SDL_MOUSEBUTTONDOWN:
                fprintf(stderr, "Button %d down\n", evt.button.button);
                button = 1;
                break;
            case SDL_FINGERDOWN:
                fprintf(stderr, "Fingerdown\n");
                break;
            case SDL_MOUSEBUTTONUP:
                fprintf(stderr, "Button %d up\n", evt.button.button);
                button = 0;
                break;
            case SDL_MOUSEMOTION:
                fprintf(stderr, "Mouse motion: %d\n", evt.motion.xrel);
                if (button) {
                    cam_angle_x += evt.motion.yrel / 180.0f;
                    cam_angle_z -= evt.motion.xrel / 180.0f;
                }
                break;
            case SDL_FINGERMOTION:
                fprintf(stderr, "Fingermotion\n");
                break;
            case SDL_MOUSEWHEEL:
                break;
            case SDL_KEYDOWN:
                if (evt.key.keysym.sym == SDLK_ESCAPE) {
                    alive = 0;
                }
                break;
            case SDL_KEYUP:
                break;
            case SDL_TEXTINPUT:
                break;
            case SDL_WINDOWEVENT:
                switch (evt.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        width = evt.window.data1;
                        height = evt.window.data2;
                        SDL_SetWindowSize(window, width, height);
                        /*
                         * Resize render target
                         * This is only needed for proper framebuffers (not window taget) but we keep it here
                         */
                        sehle_render_target_resize(tgt, width, height);
                        /*
                         * Resize render context
                         */
                        sehle_render_context_set_viewport(&ctx, 0, 0, width, height);
                        break;
                }
                break;
            case SDL_QUIT:
                alive = 0;
                break;
            default:
                time = arikkei_get_time() - start;
                fire_inst.time = (float) time;

                EleaMat3x4f v2w, m0, m1;
                m0 = EleaMat3x4fIdentity;
                elea_mat3x4f_rotate_right_axis_angle(&m1, &m0, &EleaVec3fX, M_PI_2 - cam_angle_x);
                elea_mat3x4f_rotate_left_axis_angle(&v2w, &m1, &EleaVec3fZ, cam_angle_z);
                elea_mat3x4f_translate_self_right_xyz(&v2w, 0, 0, cam_dist);
                EleaMat4x4f proj;
                elea_mat4x4f_set_frustum_fov(&proj, 0.1f, (float) width / height, 0.1f, 100.0f);
                /* Update view parameters in render context  */
                sehle_render_context_set_view (&ctx, &v2w, &proj);

                //elea_mat3x4f_set_rotation_axis_angle(&mesh.r2w, &EleaVec3fZ, fmod(time, 2 * M_PI));
                break;
        }
        /* Re-bind context to ensure the synchronization with engine */
        sehle_render_context_bind(&ctx);
        /* Clear frame */
        EleaColor4f bg = {0.16f, 0.15f, 0.14f};
        sehle_render_context_clear (&ctx, 1, 1, &bg);
        /* Display renderable */
        sehle_render_context_display_frame(&ctx, &fire_impl.rend_impl, &fire_inst.rend_inst, 1, SEHLE_STAGE_TRANSPARENT);
        /* Render everything that was submitted at display stage  */
        sehle_render_context_render_transparent (&ctx);
        /* Clean up context */
        sehle_render_context_finish_frame(&ctx);
        
        SDL_GL_SwapWindow(window);
    }

    sehle_engine_delete(engine);

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(gl_context);
    SDL_Quit();

    return 0;
}
