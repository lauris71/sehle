#define __HELLO2_C__

/*
 * Sehle tutorial 3
 *
 * Textures
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

static SDL_Window* window;
static SDL_GLContext gl_context;

static int width = 800;
static int height = 600;

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
    nr_pixblock_setup_extern(pxb, NR_PIXBLOCK_MODE_R8G8B8A8N,
        hdr->x_origin, hdr->y_origin, hdr->x_origin + hdr->width, hdr->y_origin + hdr->height,
        hdr->pixels, hdr->width * 4, 0, 0);
}

/*
 * A helper function to create 20x20m ground platform
 */

static void
create_platform(SehleStaticMesh *mesh, SehleEngine *engine)
{
    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, "Platform", SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, 24, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, SEHLE_ATTRIBUTE_TEXCOORD0, 2, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, "Platform", SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, 36);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);
    /*
     * Generate unit cube using Elea
     */
    EleaVec3f p0 = {-10, -10, -2.1};
    EleaVec3f p1 = {10, 10, -2};
    elea_generate_box(vertices, 8 * 4, vertices + 3, 8 * 4, vertices + 6, 8 * 4, indices, &p0, &p1, 1);
    sehle_vertex_buffer_unmap(vbuf);
    sehle_index_buffer_unmap(ibuf);
    /* Assign created buffers to vertex array */
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);

    az_instance_init(mesh, SEHLE_TYPE_STATIC_MESH);
    /* Bind static mesh to engine and set render layers */
    sehle_static_mesh_setup(mesh, engine, 1);
    /* Set up static mesh geometry and bounding box */
    sehle_static_mesh_set_vertex_array(mesh, va);
    elea_aabox3f_grow_p(&mesh->renderable_inst.bbox, &mesh->renderable_inst.bbox, &p0);
    elea_aabox3f_grow_p(&mesh->renderable_inst.bbox, &mesh->renderable_inst.bbox, &p1);

    SehlematerialDNS *mat = sehle_material_dns_new(engine);
    sehle_material_dns_set_has_colors(mat, 0);
    sehle_material_dns_set_transparent(mat, 0, 0);
    SehleTexture2D *tex = sehle_engine_get_standard_texture(engine, SEHLE_TEXTURE_WHITE);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_DIFFUSE, tex);
    SehleMaterialImplementation *mat_impl = SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION;

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(mesh, 1);
    sehle_static_mesh_set_material(mesh, 0, mat_impl, &mat->material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(mesh, 1);
    mesh->frags[0].first = 0;
    mesh->frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh->frags[0].mat_idx = 0;
}

static void
create_sphere(SehleStaticMesh *mesh, SehleEngine *engine)
{
    unsigned int n_vertices, n_indices;
    elea_generate_sphere(NULL, 0, NULL, 0, NULL, 0, NULL, 1, 4, 1, &n_vertices, &n_indices);

    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, "Hello sphere", SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, n_vertices, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, "Hello sphere", SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, n_indices);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);
    elea_generate_sphere(vertices, 6 * 4, vertices + 3, 6 * 4, NULL, 0, indices, 1, 4, 1, &n_vertices, &n_indices);
    sehle_vertex_buffer_unmap(vbuf);
    sehle_index_buffer_unmap(ibuf);
    /* Assign created buffers to vertex array */
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);

    az_instance_init(mesh, SEHLE_TYPE_STATIC_MESH);
    /* Bind static mesh to engine and set render layers */
    sehle_static_mesh_setup(mesh, engine, 1);
    /* Set up static mesh geometry and bounding box */
    sehle_static_mesh_set_vertex_array(mesh, va);
    mesh->renderable_inst.bbox = EleaAABox3fInfinite;

    SehlematerialDNS *mat = sehle_material_dns_new(engine);
    sehle_material_dns_set_has_colors(mat, 0);
    sehle_material_dns_set_transparent(mat, 0, 0);
    SehleTexture2D *tex = sehle_engine_get_standard_texture(engine, SEHLE_TEXTURE_WHITE);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_DIFFUSE, tex);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_SPECULAR, tex);
    SehleMaterialImplementation *mat_impl = SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION;

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(mesh, 1);
    sehle_static_mesh_set_material(mesh, 0, mat_impl, &mat->material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(mesh, 1);
    mesh->frags[0].first = 0;
    mesh->frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh->frags[0].mat_idx = 0;
}

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
    window = SDL_CreateWindow("ShinYa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
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
    ctx.global_ambient = EleaColor4fBlack;// elea_color4f_from_rgba(0.1f, 0.1f, 0.1f, 1);
    sehle_render_context_set_target (&ctx, tgt);
    sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
    
#if 0
    /*
     * Create geometry
     *
     * First we create vertex and index buffers and map these to CPU memory
     * 
     * Vertex buffer has the following layout:
     *   Vertex: 3 floats
     *   Normal: 3 floats
     *   TexCoord: 2 floats
     *   Color: 4 floats
     */
    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, 24, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, SEHLE_ATTRIBUTE_TEXCOORD0, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, 36);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);
    /*
     * Generate unit cube using Elea
     */
    EleaVec3f p0 = {-1, -1, -1};
    EleaVec3f p1 = {1, 1, 1};
    elea_generate_box(vertices, 12 * 4, vertices + 3, 12 * 4, vertices + 6, 12 * 4, indices, &p0, &p1, 1);
    /*
     * The cube generator did not assign colors so we have to do that manually
     */
    for (int i = 0; i < 24; i++) {
        float *v = vertices + 12 * i;
        float x = v[0];
        float y = v[1];
        float z = v[2];
        v[8 + 0] = (z + 1) / 2;
        v[8 + 1] = (y + 1) / 2;
        v[8 + 2] = (x + 1) / 2;
        v[8 + 3]  = 1;
    }
    sehle_vertex_buffer_unmap(vbuf);
    sehle_index_buffer_unmap(ibuf);
#endif
    /* Assign created buffers to vertex array */
    //SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);
    SehleVertexArray *va = sehle_engine_get_standard_geometry (engine, SEHLE_GEOMETRY_UNIT_CUBE_INSIDE);

    /*
     * As we create more than one renderable we want to put these into renderable collection so
     * we do not have to call display separately for each item
     * The collection also handles bbox and layewr based culling
     */
    SehleRenderableList list;
    sehle_renderable_list_setup(&list, engine, 1);

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
    elea_aabox3f_set_values(&mesh.renderable_inst.bbox, -2, -2, -2, 2, 2, 2);

    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &mesh.renderable_inst);

    /*
     * Create materials
     *
     * As MaterialForward only knows about directional lights, we are going to use a more
     * advanced MaterialDNS (Diffuse/Normal/Specular)
     */

    SehlematerialDNS *mat = sehle_material_dns_new(engine);
    sehle_material_dns_set_has_colors(mat, 0);
    sehle_material_dns_set_transparent(mat, 0, 0);

    NRPixBlock pxb;
    read_texture(&pxb, "ab_crate_a.tga");
    SehleTexture2D *tex = sehle_texture_2d_new(engine, (const uint8_t *) "Crate diffuse");
    sehle_texture_2d_set_pixels_from_pixblock(tex, &pxb);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_DIFFUSE, tex);
    nr_pixblock_release(&pxb);

    read_texture(&pxb, "ab_crate_a_nm.tga");
    tex = sehle_texture_2d_new(engine, (const uint8_t *) "Crate normal");
    sehle_texture_2d_set_pixels_from_pixblock(tex, &pxb);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_NORMAL, tex);
    nr_pixblock_release(&pxb);

    read_texture(&pxb, "ab_crate_a_sm.tga");
    tex = sehle_texture_2d_new(engine, (const uint8_t *) "Crate specular");
    sehle_texture_2d_set_pixels_from_pixblock(tex, &pxb);
    sehle_material_dns_set_texture(mat, SEHLE_MATERIAL_DNS_MAP_SPECULAR, tex);
    nr_pixblock_release(&pxb);

    SehleMaterialImplementation *mat_impl = SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION;

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(&mesh, 1);
    sehle_static_mesh_set_material(&mesh, 0, mat_impl, &mat->material_inst);
    /*
     * Set up static mesh fragment
     * Fragments are simply parts of mesh with different slice of index buffer and material
     * LOD is automatically set up to draw everything by default
     */
    sehle_static_mesh_resize_fragments(&mesh, 1);
    mesh.frags[0].first = 0;
    mesh.frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh.frags[0].mat_idx = 0;

    /* Create plafrom with helper function */
    SehleStaticMesh platform;
    create_platform(&platform, engine);
    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &platform.renderable_inst);

    //SehleStaticMesh sphere;
    //create_sphere(&sphere, engine);
    //sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &sphere.renderable_inst);

    /*
     * Now we are ready to create some lights
     *
     * Light are themselves renderables that can both:
     * - render themselves to framebuffer in deferred lighting pass
     * - give lighting information to materials in forward and transparent passes (we are using this)
     */
    SehleDirectionalLightImplementation dirl_impl;
    SehleDirectionalLightInstance dirl_inst;
    az_implementation_init((AZImplementation *) &dirl_impl, SEHLE_TYPE_DIRECTIONAL_LIGHT);
    az_interface_init((AZImplementation *) &dirl_impl, &dirl_inst);
    sehle_directional_light_setup(&dirl_inst, engine, 0, 0);
    dirl_inst.light_inst.ambient = EleaColor4fBlack;
    dirl_inst.light_inst.diffuse = EleaColor4fBlue;
    dirl_inst.light_inst.direct = EleaColor4fWhite;
    EleaVec3f light_pos = {-100, 100, 100};
    elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
    sehle_renderable_list_add_child(&list, &dirl_impl.light_impl.renderable_impl, &dirl_inst.light_inst.renderable_inst);

    SehlePointLightImplementation point_impl;
    SehlePointLightInstance point_inst;
    az_implementation_init((AZImplementation *) &point_impl, SEHLE_TYPE_POINT_LIGHT);
    az_interface_init((AZImplementation *) &point_impl, &point_inst);
    sehle_point_light_setup(&point_inst, engine, 0);
    point_inst.light_inst.ambient = elea_color4f_div(EleaColor4fRed, 5);
    point_inst.light_inst.diffuse = EleaColor4fRed;
    point_inst.light_inst.direct = EleaColor4fWhite;
    sehle_point_light_set_point_attenuation (&point_inst, 2.0f, 10.0f, 2);
    EleaVec3f point_pos = {5, -5, 2};
    elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);
    sehle_point_light_update_visuals(&point_inst);
    sehle_renderable_list_add_child(&list, &point_impl.light_impl.renderable_impl, &point_inst.light_inst.renderable_inst);

    SehleSpotLightImplementation spot_impl;
    SehleSpotLightInstance spot_inst;
    az_implementation_init((AZImplementation *) &spot_impl, SEHLE_TYPE_SPOT_LIGHT);
    az_interface_init((AZImplementation *) &spot_impl, &spot_inst);
    sehle_spot_light_setup(&spot_inst, engine, 0);
    spot_inst.light_inst.ambient = elea_color4f_div(EleaColor4fGreen, 5);
    spot_inst.light_inst.diffuse = EleaColor4fGreen;
    spot_inst.light_inst.direct = EleaColor4fWhite;
    sehle_spot_light_set_point_attenuation (&spot_inst, 0, 20.0f, 1);
    sehle_spot_light_set_spot_attenuation (&spot_inst, 0.6f, 0.75f, 1);
    EleaVec3f spot_pos = {-2, 2, 2};
    elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &EleaVec3f0, &EleaVec3fZ);
    /* Spot light has complex geometry, in forward pass it is needed for bbox */
    sehle_spot_light_update_visuals(&spot_inst);
    sehle_renderable_list_add_child(&list, &spot_impl.light_impl.renderable_impl, &spot_inst.light_inst.renderable_inst);

    /*
     * Once all renderables have been added to the collection we let the collection to
     * update mask and bbox
     */
    sehle_renderable_list_update_visuals(&list);

    /*
     * Set up view and projection matrices
     */
    EleaVec3f viewpoint = {3, 4, 3};
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
                dirl_inst.light_inst.diffuse = elea_color4f_mul(EleaColor4fBlue, (1 + sin(time / 2.7)) / 2);

                /* Rotate the position of point light */
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
        sehle_render_context_display_frame(&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst, 1, SEHLE_STAGE_FORWARD);
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
