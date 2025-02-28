#define __HELLO2_C__

/*
 * Sehle tutorial 2
 *
 * Simple forward lighting
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
#include <sehle/light-directional.h>
#include <sehle/light-spot.h>
#include <sehle/light-point.h>
#include <sehle/material-control.h>
#include <sehle/material-dns.h>
#include <sehle/render-context.h>
#include <sehle/render-target.h>
#include <sehle/renderable-list.h>
#include <sehle/static-mesh.h>
#include <sehle/texture-2d.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include <SDL2/SDL.h>

static SDL_Window* window;
static SDL_GLContext gl_context;

static int width = 800;
static int height = 600;

/*
 * A base render layer
 */

#define LAYER_SCENE 1

/*
 * A helper function to create 20x20m ground platform
 */

static void
create_platform(SehleStaticMesh *mesh, SehleEngine *engine)
{
    az_instance_init(mesh, SEHLE_TYPE_STATIC_MESH);
    sehle_static_mesh_setup(mesh, engine, LAYER_SCENE);
    /* Use pre-built flat rectangle as geometry */
    SehleVertexArray *va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_GRID_8x8);
    sehle_static_mesh_set_vertex_array(mesh, va);

    /* Create material with lighting */
    SehlematerialDNS *mat = sehle_material_dns_new(engine);
    sehle_material_dns_set_has_colors(mat, 0);
    sehle_material_dns_set_transparent(mat, 0, 0);

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(mesh, 1);
    sehle_static_mesh_set_material(mesh, 0, SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION, &mat->material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(mesh, 1);
    mesh->frags[0].first = 0;
    mesh->frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh->frags[0].mat_idx = 0;

    EleaMat3x4f m;
    elea_mat3x4f_set_scale_xyz(&m, 20, 20, 20);
    elea_mat3x4f_translate_left_xyz(&mesh->r2w, &m, -10, -10, 0);

    elea_aabox3f_set_values(&mesh->renderable_inst.bbox, -10, -10, 0, 10, 10, 0);
}

static void
create_cube(SehleStaticMesh *mesh, SehleEngine *engine, EleaVec3f pos)
{
    az_instance_init(mesh, SEHLE_TYPE_STATIC_MESH);
    sehle_static_mesh_setup(mesh, engine, LAYER_SCENE);
    /* Use pre-built flat rectangle as geometry */
    SehleVertexArray *va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_UNIT_CUBE_OUTSIDE);
    sehle_static_mesh_set_vertex_array(mesh, va);

    /* Create material with lighting */
    SehlematerialDNS *mat = sehle_material_dns_new(engine);
    sehle_material_dns_set_has_colors(mat, 0);
    sehle_material_dns_set_transparent(mat, 0, 0);

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(mesh, 1);
    sehle_static_mesh_set_material(mesh, 0, SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION, &mat->material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(mesh, 1);
    mesh->frags[0].first = 0;
    mesh->frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh->frags[0].mat_idx = 0;

    elea_mat3x4f_set_translate(&mesh->r2w, &pos);

    elea_aabox3f_set_values(&mesh->renderable_inst.bbox, pos.x - 2, pos.y - 2, pos.z - 2, pos.x + 2, pos.y + 2, pos.z + 2);
}

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
     * It s not needef if neither renderables, material system nor renderers are used
     */
    SehleRenderContext ctx;
    sehle_render_context_setup(&ctx, engine);
    sehle_render_context_set_target (&ctx, tgt);
    sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
    
    /*
     * As we create more than one renderable we want to put these into renderable collection so
     * we do not have to call display separately for each item.
     * The collection also handles bbox and layer based culling.
     * The collection does not perform any memory management, it is strictly for grouping rendering calls.
     */
    SehleRenderableList list;
    sehle_renderable_list_setup(&list, engine, LAYER_SCENE);

    /* First create 10x10m platform at z=0 */
    SehleStaticMesh platform;
    create_platform(&platform, engine);
    /* Once created, add each renderable to renderable list */
    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &platform.renderable_inst);

    /* Now create 2m cube howering above it */
    SehleStaticMesh mesh;
    create_cube(&mesh, engine, (EleaVec3f) {0, 0, 2});
    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &mesh.renderable_inst);

    /*
     * Now we are ready to create some lights
     *
     * Light are themselves renderables that can both:
     * - render themselves to framebuffer in deferred lighting pass
     * - give lighting information to materials in forward and transparent passes (we are using this feature here)
     */
    SehleDirectionalLightImplementation dirl_impl;
    SehleDirectionalLightInstance dirl_inst;
    az_implementation_init((AZImplementation *) &dirl_impl, SEHLE_TYPE_DIRECTIONAL_LIGHT);
    az_interface_init((AZImplementation *) &dirl_impl, &dirl_inst);
    sehle_directional_light_setup(&dirl_inst, engine, 0, 0);
    /* Set diffuse color */
    dirl_inst.light_inst.diffuse = EleaColor4fBlue;
    EleaVec3f light_pos = {-100, 100, 100};
    elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
    sehle_renderable_list_add_child(&list, &dirl_impl.light_impl.renderable_impl, &dirl_inst.light_inst.renderable_inst);

    EleaVec3f point_pos = {5, -5, 2};
    SehlePointLightImplementation point_impl;
    SehlePointLightInstance point_inst;
    az_implementation_init((AZImplementation *) &point_impl, SEHLE_TYPE_POINT_LIGHT);
    az_interface_init((AZImplementation *) &point_impl, &point_inst);
    sehle_point_light_setup(&point_inst, engine, 0);
    /* Set ambient and diffuse color */
    point_inst.light_inst.ambient = elea_color4f_div(EleaColor4fRed, 3);
    point_inst.light_inst.diffuse = EleaColor4fRed;
    /*
     * Point attenuation describes how the lighting intensity changes by distance
     *  - min_distance is the length of unlit zone immediately next to light
     *  - radius is the maximum distance where light reaches
     *  - falloff describes how fast the light decays by distance
     */
    sehle_point_light_set_point_attenuation (&point_inst, 2.0f, 10.0f, 2);
    elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);
    /*
     * Like all renderables, lights have bounding boxes. These have to be recalcualte whenever
     * the local coordinate system or attenuation parameters change (the exception is directional
     * lght that always has infinite bounding box).
     */
    sehle_point_light_update_visuals(&point_inst);
    sehle_renderable_list_add_child(&list, &point_impl.light_impl.renderable_impl, &point_inst.light_inst.renderable_inst);

    EleaVec3f spot_pos = {-2, 2, 4};
    SehleSpotLightImplementation spot_impl;
    SehleSpotLightInstance spot_inst;
    az_implementation_init((AZImplementation *) &spot_impl, SEHLE_TYPE_SPOT_LIGHT);
    az_interface_init((AZImplementation *) &spot_impl, &spot_inst);
    sehle_spot_light_setup(&spot_inst, engine, 0);
    spot_inst.light_inst.ambient = elea_color4f_div(EleaColor4fGreen, 3);
    spot_inst.light_inst.diffuse = EleaColor4fGreen;
    sehle_spot_light_set_point_attenuation (&spot_inst, 0, 20.0f, 1);
    /*
     * Spot attenuation describes how the lighting intensity changes by angle
     *  - inner angle is the maximum angle (from center line, in radians) where light has full intnsity
     *  - outer_angle is the maximum angle where light reaches
     *  - falloff describes how fast the light decays by angle from inner to outer
     */
    sehle_spot_light_set_spot_attenuation (&spot_inst, 0.6f, 0.75f, 1);
    /*
     * Spot light has complex light cone that has to be rebuilt whenever either spot or point
     * attenuation parameters change. It is not needed for forward lighting, but we include it here
     * for completeness.
     */
    sehle_spot_light_update_geometry(&spot_inst);
    elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &EleaVec3f0, &EleaVec3fZ);
    /* Update bounding box */
    sehle_spot_light_update_visuals(&spot_inst);
    sehle_renderable_list_add_child(&list, &spot_impl.light_impl.renderable_impl, &spot_inst.light_inst.renderable_inst);

    /*
     * Once all renderables have been added to the collection we let the collection to
     * update mask and bbox to cover all included renderables.
     */
    sehle_renderable_list_update_visuals(&list);

    /*
     * Set up view and projection matrices
     */
    EleaVec3f viewpoint = {10, 20, 20};
    EleaMat3x4f v2w;
    elea_mat3x4f_set_look_at(&v2w, &viewpoint, &EleaVec3f0, &EleaVec3fZ);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 0.01f, (float) width / height, 0.01f, 100.0f);
    /* Update view parameters in render context  */
    sehle_render_context_set_view (&ctx, &v2w, &proj);

    /*
     * Bind render context - i.e. ensure that the engine state matches that of context
     */
    sehle_render_context_bind(&ctx);

    double start = arikkei_get_time();
    double time, x, y;
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
                /* Rotate the cube */
                elea_mat3x4f_set_rotation_axis_angle(&mesh.r2w, &EleaVec3fZ, fmod(time / 10, 2 * M_PI));

                /* Rotate directional light and change it's intensity */
                light_pos.x = 100 * cos(time);
                light_pos.y = 100 * sin(time);
                elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
                dirl_inst.light_inst.diffuse = elea_color4f_mul(EleaColor4fBlue, (1 + sin(time / 2)) / 2);

                /* Rotate the position of point light around the center */
                point_pos.x = -5 * cos(-time);
                point_pos.y = -5 * sin(-time);
                elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);

                /* Rotate the direction of spot light */
                EleaVec3f spot_dst;
                spot_dst.x = spot_pos.x + 2 * cos(time);
                spot_dst.y = spot_pos.y + 2 * sin(time);
                spot_dst.z = 0;
                elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &spot_dst, &EleaVec3fZ);

                break;
        }
        /* Re-bind context to ensure the synchronization with engine */
        sehle_render_context_bind(&ctx);
        /* Clear frame */
        sehle_render_context_clear (&ctx, 1, 1, &EleaColor4fBlack);
        /* Display renderable */
        sehle_render_context_display_frame(&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst, LAYER_SCENE, SEHLE_STAGE_FORWARD);
        /* Render everything that was submitted at display stage  */
        //sehle_render_context_render (&ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 1, 0);
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
