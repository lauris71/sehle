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
#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/material-control.h>
#include <sehle/render-context.h>
#include <sehle/render-target.h>
#include <sehle/static-mesh.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include <SDL2/SDL.h>

static int width = 800;
static int height = 600;

int
main(int argc, const char **argv)
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

    // Request an OpenGL 4.1 context (should be core)
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
    SDL_Window *window = SDL_CreateWindow("ShinYa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow:%s\n", SDL_GetError());
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext:%s\n", SDL_GetError());
        exit(1);
    }
    int major, minor;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
    fprintf(stderr, "OpenGL version %d.%d\n", major, minor);
    SDL_GL_SetSwapInterval(0);

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
    ctx.global_ambient = EleaColor4fWhite;
    sehle_render_context_set_target (&ctx, tgt);
    sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
    
    /*
     * Create geometry
     *
     * First we create vertex and index buffers and map these to CPU memory
     * 
     * Vertex buffer has the following layout:
     *   Vertex: 3 floats
     *   TexCoord: 2 floats
     *   Color: 4 floats
     */
    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, 24, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_TEXCOORD0, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, 36);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);
    /*
     * Generate unit cube using Elea
     */
    EleaVec3f p0 = {-1, -1, -1};
    EleaVec3f p1 = {1, 1, 1};
    elea_generate_box(vertices, 9 * 4, NULL, 0, vertices + 3, 9 * 4, indices, &p0, &p1, 1);
    /*
     * The cube generator did not assign colors so we have to do that manually
     */
    for (int i = 0; i < 24; i++) {
        float *v = vertices + 9 * i;
        float x = v[0];
        float y = v[1];
        float z = v[2];
        v[5 + 0] = (z + 1) / 2;
        v[5 + 1] = (y + 1) / 2;
        v[5 + 2] = (x + 1) / 2;
        v[5 + 3]  = 1;
    }
    sehle_vertex_buffer_unmap(vbuf);
    sehle_index_buffer_unmap(ibuf);
    /* Assign created buffers to vertex array */
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);

    /*
     * Create static mesh renderable
     * As it is stack-allocated object we have to call instance_init to set it up
     */
    SehleStaticMesh mesh;
    az_instance_init(&mesh, SEHLE_TYPE_STATIC_MESH);
    /* Bind static mesh to engine and set render layers */
    sehle_static_mesh_setup(&mesh, engine, 1);
    /* Set up static mesh geometry and bounding box */
    sehle_static_mesh_set_vertex_array(&mesh, va);
    elea_aabox3f_set_values(&mesh.renderable_inst.bbox, -1, -1, -1, 1, 1, 1);

    /*
     * Create materials
     * We use simple forward-rendered material
     */
    SehleMaterialControl mat;
    sehle_material_control_init(&mat, engine);
    sehle_material_control_set_has_colors(&mat, 1);
    mat.ambient = EleaColor4fWhite;
    mat.color = EleaColor4fWhite;

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(&mesh, 1);
    sehle_static_mesh_set_material(&mesh, 0, SEHLE_MATERIAL_CONTROL_MATERIAL_IMPLEMENTATION, &mat.material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(&mesh, 1);
    mesh.frags[0].first = 0;
    mesh.frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh.frags[0].mat_idx = 0;

    /*
     * Set up view and projection matrices
     */
    EleaVec3f viewpoint = {2, 4, 3};
    EleaMat3x4f v2w;
    elea_mat3x4f_set_look_at(&v2w, &viewpoint, &EleaVec3f0, &EleaVec3fZ);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 0.1f, (float) width / height, 0.1f, 100.0f);
    /* Update view parameters in render context  */
    sehle_render_context_set_view (&ctx, &v2w, &proj);

    /*
     * Bind render context - i.e. ensure that the engine state matches that of context
     */
    sehle_render_context_bind(&ctx);

    double start = arikkei_get_time();
    double time;
    unsigned int alive = 1;
    while (alive) {
        SDL_Event evt;
        SDL_PollEvent(&evt);
        switch (evt.type) {
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDL_MOUSEBUTTONUP:
                break;
            case SDL_MOUSEMOTION:
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
                elea_mat3x4f_set_rotation_axis_angle(&mesh.r2w, &EleaVec3fZ, fmod(time, 2 * M_PI));
                break;
        }
        /* Re-bind context to ensure the synchronization with engine */
        sehle_render_context_bind(&ctx);
        /* Clear frame */
        EleaColor4f bg = {0.32f, 0.3f, 0.27f};
        sehle_render_context_clear (&ctx, 1, 1, &bg);
        /* Display renderable */
        sehle_render_context_display_frame(&ctx, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &mesh.renderable_inst, 1, SEHLE_STAGE_FORWARD | SEHLE_STAGE_SOLID);
        /* Render everything that was submitted at display stage  */
        sehle_render_context_render (&ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 1, 0);
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
