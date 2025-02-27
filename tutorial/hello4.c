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
#include <sehle/material-overlay.h>
#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/render-target-deferred.h>
#include <sehle/render-target-texture.h>
#include <sehle/renderable-list.h>
#include <sehle/static-mesh.h>
#include <sehle/texture-2d.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include <SDL2/SDL.h>

#define LAYER_SOLID 1
#define LAYER_DEBUG 65536

static SDL_Window* window;
static SDL_GLContext gl_context;

static int width = 800;
static int height = 600;

typedef struct _HelloShape HelloShape;

enum {
    CUBE,
    SPHERE
};

struct _HelloShape {
    SehleStaticMesh mesh;
    EleaVec3f pos;
};

static void
create_shape(HelloShape *shape, SehleEngine *engine, unsigned int type, int dir)
{
    unsigned int n_vertices, n_indices;
    if (type == CUBE) {
        n_vertices = 24;
        n_indices = 36;
    } else {
        elea_generate_sphere(NULL, 0, NULL, 0, NULL, 0, NULL, 1, 3, dir, &n_vertices, &n_indices);
    }
    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, NULL, SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, n_vertices, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, SEHLE_ATTRIBUTE_TEXCOORD0, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, NULL, SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, n_indices);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);
    if (type == CUBE) {
        EleaVec3f p0 = {-1, -1, -1};
        EleaVec3f p1 = {1, 1, 1};
        elea_generate_box(vertices, 12 * 4, vertices + 3, 12 * 4, vertices + 6, 12 * 4, indices, &p0, &p1, dir);
    } else {
        elea_generate_sphere(vertices, 12 * 4, vertices + 3, 12 * 4, vertices + 6, 12 * 4, indices, 1, 3, dir, &n_vertices, &n_indices);
    }
    for (int i = 0; i < n_vertices; i++) {
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
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, NULL, vbuf, ibuf);

    az_instance_init(&shape->mesh, SEHLE_TYPE_STATIC_MESH);
    sehle_static_mesh_setup(&shape->mesh, engine, 1);
    sehle_static_mesh_set_vertex_array(&shape->mesh, va);
    shape->mesh.renderable_inst.bbox = EleaAABox3fInfinite;

    SehleMaterialControl *mat = sehle_material_control_new(engine);
    sehle_material_control_set_has_colors(mat, 1);
    mat->ambient = EleaColor4fWhite;
    mat->color = EleaColor4fWhite;
    SehleMaterialImplementation *mat_impl = SEHLE_MATERIAL_CONTROL_MATERIAL_IMPLEMENTATION;

    sehle_static_mesh_resize_materials(&shape->mesh, 1);
    sehle_static_mesh_set_material(&shape->mesh, 0, mat_impl, &mat->material_inst);

    sehle_static_mesh_resize_fragments(&shape->mesh, 1);
    shape->mesh.frags[0].first = 0;
    shape->mesh.frags[0].n_indices = n_indices;
    shape->mesh.frags[0].mat_idx = 0;
}

static void
update_shape(HelloShape *shape, EleaVec3f pos, float angle)
{
    EleaMat3x4f m;
    elea_mat3x4f_set_translate(&m, &pos);
    elea_mat3x4f_rotate_right_axis_angle(&shape->mesh.r2w, &m, &EleaVec3fZ, angle);
}

#define N_POINTS 100
typedef struct _HelloPoint HelloPoint;

struct _HelloPoint {
    SehlePointLightImplementation point_impl;
    SehlePointLightInstance point_inst;

    EleaVec3f pos;
    EleaVec3f speed;
    EleaVec3f acc;

    EleaAABox3f bbox;
};

static void
create_point(HelloPoint *point, SehleEngine *engine, EleaColor4f color, EleaVec3f pos)
{
    point->pos = pos;
    point->speed = EleaVec3f0;
    point->acc = EleaVec3f0;
    point->bbox = (EleaAABox3f) {-10, -10, -1.8f, 10, 10, 0};

    az_implementation_init((AZImplementation *) &point->point_impl, SEHLE_TYPE_POINT_LIGHT);
    az_interface_init((AZImplementation *) &point->point_impl, &point->point_inst);
    sehle_point_light_setup(&point->point_inst, engine, 0);
    point->point_inst.light_inst.ambient = EleaColor4fBlack;
    point->point_inst.light_inst.diffuse = color;
    sehle_point_light_set_point_attenuation (&point->point_inst, 0.1f, 3.0f, 1);
    sehle_point_light_update_visuals(&point->point_inst);

    elea_mat3x4f_set_translate(&point->point_inst.light_inst.l2w, &point->pos);
    sehle_point_light_update_visuals(&point->point_inst);
}

static void
point_animate(HelloPoint *point, float dt)
{
    point->pos = elea_vec3f_mul_add(point->pos, point->speed, dt);
    elea_mat3x4f_set_translate(&point->point_inst.light_inst.l2w, &point->pos);
    sehle_point_light_update_visuals(&point->point_inst);

    point->speed = elea_vec3f_mul_add(point->speed, point->acc, dt);
    for (int i = 0; i < 3; i++) {
        if (point->pos.c[i] > point->bbox.max.c[i]) {
            point->acc.c[i] = -(point->pos.c[i] - point->bbox.max.c[i]);
        } else if (point->pos.c[i] < point->bbox.min.c[i]) {
            point->acc.c[i] = -(point->pos.c[i] - point->bbox.min.c[i]);
        } else {
            point->acc.c[i] = 0;
        }
        point->acc.c[i] -= point->speed.c[i] * fabs(point->speed.c[i]) / 10;
        float v = (float) (rand() / (RAND_MAX + 1.0));
        v = v - 0.5f;
        v *= 0.1f;
        point->acc.c[i] += v;
    }
}

typedef struct _HelloSpot HelloSpot;

struct _HelloSpot {
    SehleSpotLightImplementation spot_impl;
    SehleSpotLightInstance spot_inst;
    EleaVec3f pos;
    EleaVec3f tgt;

    SehleStaticMesh mesh;
    SehleMaterialControl mat;

    SehleRenderableList list;

    SehleRenderableImplementation *r_impl;
    SehleRenderableInstance *r_inst;
};

static void
create_spot(HelloSpot *spot, SehleEngine *engine, EleaColor4f color, EleaVec3f pos, EleaVec3f tgt)
{
    spot->pos = pos;
    spot->tgt = tgt;

    sehle_renderable_list_setup(&spot->list, engine, 1);
    spot->list.collection_inst.renderable_inst.layer_mask = 0xffffffff;
    spot->list.collection_inst.renderable_inst.bbox = EleaAABox3fInfinite;
    spot->r_impl = SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION;
    spot->r_inst = &spot->list.collection_inst.renderable_inst;

    az_implementation_init((AZImplementation *) &spot->spot_impl, SEHLE_TYPE_SPOT_LIGHT);
    az_interface_init((AZImplementation *) &spot->spot_impl, &spot->spot_inst);
    sehle_spot_light_setup(&spot->spot_inst, engine, 0);
    spot->spot_inst.light_inst.ambient = EleaColor4fBlack;
    spot->spot_inst.light_inst.diffuse = EleaColor4fGreen;
    sehle_spot_light_set_point_attenuation (&spot->spot_inst, 2, 20.0f, 1);
    sehle_spot_light_set_spot_attenuation (&spot->spot_inst, 10.0f * M_PI / 180, 25.0f * M_PI / 180, 1);
    /* Spot light has complex geometry, in forward pass it is needed for bbox */
    sehle_spot_light_update_geometry(&spot->spot_inst);
    sehle_renderable_list_add_child(&spot->list, &spot->spot_impl.light_impl.renderable_impl, &spot->spot_inst.light_inst.renderable_inst);

    az_instance_init(&spot->mesh, SEHLE_TYPE_STATIC_MESH);
    /* Bind static mesh to engine and set render layers */
    sehle_static_mesh_setup(&spot->mesh, engine, LAYER_DEBUG);
    /* Set up static mesh geometry and bounding box */
    sehle_static_mesh_set_vertex_array(&spot->mesh, spot->spot_inst.light_inst.va);
    spot->mesh.renderable_inst.bbox = EleaAABox3fInfinite;

    sehle_material_control_init(&spot->mat, engine);
    sehle_material_control_set_has_colors(&spot->mat, 0);
    spot->mat.material_inst.state_flags &= ~SEHLE_FILL;
    spot->mat.ambient = color;
    spot->mat.color = color;

    /* Add material slot to static mesh and set material */
    sehle_static_mesh_resize_materials(&spot->mesh, 1);
    sehle_static_mesh_set_material(&spot->mesh, 0, SEHLE_MATERIAL_CONTROL_MATERIAL_IMPLEMENTATION, &spot->mat.material_inst);
    sehle_static_mesh_resize_fragments(&spot->mesh, 1);
    spot->mesh.frags[0].first = 0;
    spot->mesh.frags[0].n_indices = spot->mesh.va->ibuf->buffer.n_elements;
    spot->mesh.frags[0].mat_idx = 0;

    sehle_renderable_list_add_child(&spot->list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &spot->mesh.renderable_inst);

    elea_mat3x4f_set_look_at(&spot->spot_inst.light_inst.l2w, &pos, &tgt, &EleaVec3fZ);
    spot->mesh.r2w = spot->spot_inst.light_inst.l2w;
    sehle_spot_light_update_visuals(&spot->spot_inst);
}

static void
spot_animate(HelloSpot *spot, double time)
{
    EleaMat3x4f base;
    elea_mat3x4f_set_look_at(&base, &spot->pos, &spot->tgt, &EleaVec3fZ);

    float radius = 0.25f;
    float x = radius * cosf((float) time);
    float y = radius * sinf((float) time);
    EleaVec3f tgt_base = {x, y, -1};
    EleaVec3f tgt_w;
    elea_mat3x4f_transform_point(&tgt_w, &base, &tgt_base);

    elea_mat3x4f_set_look_at(&spot->spot_inst.light_inst.l2w, &spot->pos, &tgt_w, &EleaVec3fZ);
    spot->mesh.r2w = spot->spot_inst.light_inst.l2w;
    sehle_spot_light_update_visuals(&spot->spot_inst);
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
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "Platform", vbuf, ibuf);

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
}

static SehleVertexArray *
create_colored_cube_va(SehleEngine *engine)
{
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
    //for (int i = 0; i < 36; i++) indices[i] = i / 2;
    /*
     * Generate unit cube using Elea
     */
    EleaVec3f p0 = {-1, -1, -1};
    EleaVec3f p1 = {1, 1, 1};
    elea_generate_box(vertices, 12 * 4, vertices + 3, 12 * 4, vertices + 6, 12 * 4, indices, &p0, &p1, -1);
    //elea_generate_box(vertices, 12 * 4, vertices + 3, 12 * 4, vertices + 6, 12 * 4, NULL, &p0, &p1, -1);
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
    /* Assign created buffers to vertex array */
    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);
    return va;
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
    SehleRenderTarget *tgt_viewport = sehle_render_target_new_viewport(engine, width, height);

    /*
     * Deferred target
     */
	SehleRenderTargetDeferred *tgt_gbuf = sehle_render_target_deferred_new (engine, width, height);
    /*
     * Framebuffer target
     */
    SehleRenderTargetTexture *tgt_fbuf = sehle_render_target_texture_new (engine, width, height, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_F16, 1, 1);
	SEHLE_CHECK_ERRORS (0);
	/* Share single depth texture between framebuffer and gbuffer */
	sehle_render_target_texture_set_depth (tgt_fbuf, 1, tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH]);

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
    //sehle_render_context_set_target (&ctx, tgt_viewport);
    //sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
    
    /*
     * As we create more than one renderable we want to put these into renderable collection so
     * we do not have to call display separately for each item
     * The collection also handles bbox and layewr based culling
     */
    SehleRenderableList list;
    sehle_renderable_list_setup(&list, engine, 1);

#if 0
    SehleVertexArray *va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_UNIT_CUBE_OUTSIDE);
    sehle_vertex_array_bind(va);
    va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_GRID_8x8);
    sehle_vertex_array_bind(va);
    va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_UNIT_CUBE_INSIDE);
    sehle_vertex_array_bind(va);
#endif
    HelloShape shapes[16];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned int s = ((x + y) & 1) ? CUBE : SPHERE;
            int dir = ((x + y) & 2) ? 1 : -1;
            create_shape(&shapes[4 * y + x], engine, s, dir);
            update_shape(&shapes[4 * y + x], (EleaVec3f) {2 * x - 3, 2 * y - 3, 0}, 0);
            sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &shapes[4 * y + x].mesh.renderable_inst);
        }
    }

    /* Create plafrom with helper function */
    SehleStaticMesh platform;
    create_platform(&platform, engine);
    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &platform.renderable_inst);

    /*
     * Now we are ready to create some lights
     *
     * Light are themselves renderables that can both:
     * - render themselves to framebuffer in deferred lighting pass
     * - give lighting information to materials in forward and transparent passes (we are using this)
     */
#if 0
    SehleDirectionalLightImplementation dirl_impl;
    SehleDirectionalLightInstance dirl_inst;
    az_implementation_init((AZImplementation *) &dirl_impl, SEHLE_TYPE_DIRECTIONAL_LIGHT);
    az_interface_init((AZImplementation *) &dirl_impl, &dirl_inst);
    sehle_directional_light_setup(&dirl_inst, engine, 0, 0);
    dirl_inst.light_inst.va = va_cube;
    dirl_inst.light_inst.ambient = EleaColor4fBlue;
    dirl_inst.light_inst.diffuse = EleaColor4fBlue;
    EleaVec3f light_pos = {-100, 100, 100};
    elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
    //sehle_renderable_list_add_child(&list, &dirl_impl.light_impl.renderable_impl, &dirl_inst.light_inst.renderable_inst);
#endif

#if 0
    SehlePointLightImplementation point_impl;
    SehlePointLightInstance point_inst;
    az_implementation_init((AZImplementation *) &point_impl, SEHLE_TYPE_POINT_LIGHT);
    az_interface_init((AZImplementation *) &point_impl, &point_inst);
    sehle_point_light_setup(&point_inst, engine, 0);
    //point_inst.light_inst.va = va_cube;
    point_inst.light_inst.ambient = elea_color4f_div(EleaColor4fRed, 3);
    point_inst.light_inst.diffuse = EleaColor4fRed;
    point_inst.light_inst.direct = EleaColor4fRed;
    sehle_point_light_set_point_attenuation (&point_inst, 0, 10.0f, 2);
#endif
    EleaVec3f point_pos = {5, -5, 2};
#if 0
    elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);
    sehle_point_light_update_visuals(&point_inst);
    sehle_renderable_list_add_child(&list, &point_impl.light_impl.renderable_impl, &point_inst.light_inst.renderable_inst);
#endif

    HelloPoint point[N_POINTS];
    for (int i = 0; i < N_POINTS; i++) {
        float h = (float) (rand() / (RAND_MAX + 1.0));
        EleaColor4f color;
        elea_color4fp_set_hsv(&color, h, 1.0f, 1.0f, 1.0f);
        create_point(&point[i], engine, color, (EleaVec3f) {i % 10, i / 10, 0});
        sehle_renderable_list_add_child(&list, &point[i].point_impl.light_impl.renderable_impl, &point[i].point_inst.light_inst.renderable_inst);
    }
#if 0
    HelloSpot spot;
    create_spot(&spot, engine, EleaColor4fGreen, (EleaVec3f) {-2, 0, 5}, (EleaVec3f) {0, 0, 0});
    sehle_renderable_list_add_child(&list, spot.r_impl, spot.r_inst);
#endif

    /*
     * Once all renderables have been added to the collection we let the collection to
     * update mask and bbox
     */
    sehle_renderable_list_update_visuals(&list);

    /*
     * Set up view and projection matrices
     */
    EleaVec3f viewpoint = {2, 20, 10};
    EleaMat3x4f v2w;
    elea_mat3x4f_set_look_at(&v2w, &viewpoint, &EleaVec3f0, &EleaVec3fZ);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 1.0f, (float) width / height, 1.0f, 100.0f);
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
                        sehle_render_target_resize(tgt_viewport, width, height);
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
                //update_shape(&shape, (EleaVec3f) {0, 0, 0}, fmod(time / 10, 2 * M_PI));
#if 0
                /* Rotate directional light and change it's intensity */
                light_pos.x = 100 * cos(time);
                light_pos.y = 100 * sin(time);
                elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
                dirl_inst.light_inst.diffuse = elea_color4f_mul(EleaColor4fBlue, (1 + sin(time / 2)) / 2);
#endif
#if 0
                /* Rotate the position of point light */
                point_pos.x = -5 * cos(-time);
                point_pos.y = -5 * sin(-time);
                elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);
#endif
                for (int i = 0; i < N_POINTS; i++) {
                    point_animate(&point[i],  0.01f);
                }
#if 0
                spot_animate(&spot, time);
#endif
#if 0
                /* Rotate the direction of spot light */
                EleaVec3f spot_dst;
                spot_dst.x = spot_pos.x + 2 * cos(time);
                spot_dst.y = spot_pos.y + 2 * sin(time);
                spot_dst.z = 0;
                elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &spot_dst, &EleaVec3fZ);
#endif
                break;
        }

        sehle_render_context_reset (&ctx);
        sehle_render_context_set_view (&ctx, &v2w, &proj);
#if 1
        /*
         * Render opaque geometry into GBuffer
         */
        sehle_render_context_set_target (&ctx, (SehleRenderTarget *) tgt_gbuf);
        sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
        /* Clear everything */
        sehle_render_context_clear (&ctx, 1, 1, &EleaColor4fBlack);
        /* Opaque layer objects to GBuffer */
        sehle_render_context_display_frame (&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst,
            1, SEHLE_STAGE_SOLID);
        sehle_render_context_render_gbuffer (&ctx);
        sehle_render_context_finish_frame (&ctx);
#endif
        /*
         * Set target to framebuffer
         */
	    sehle_render_context_set_target (&ctx, (SehleRenderTarget *) tgt_fbuf);
        sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
        sehle_render_context_bind(&ctx);
        /* Clear colors, keep depth */
        EleaColor4f bg = elea_color4f_div(EleaColor4fBlue, 4);
        sehle_render_context_clear (&ctx, 0, 1, &bg);
#if 0
        sehle_render_context_display_frame (&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst,
            1, SEHLE_STAGE_FORWARD);
        sehle_render_context_render_forward (&ctx);
        sehle_render_context_finish_frame (&ctx);
#endif
#if 0
        /* Display renderable */
        sehle_render_context_display_frame(&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst, LAYER_DEBUG, SEHLE_STAGE_FORWARD);
        /* Render everything that was submitted at display stage  */
        sehle_render_context_render_forward(&ctx);
        //sehle_render_context_render (&ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 1, 0);
        /* Clean up context */
        sehle_render_context_finish_frame(&ctx);
#endif
#if 1
        sehle_render_context_bind_gbuf_textures (&ctx, tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH],
            tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_NORMAL],
            tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_ALBEDO_AMBIENT],
            tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_SPECULAR_SHININESS], 0, 0, 1.0f, 1.0f);

        // Lightmaps
        // The problem is, that UINT8 buffers do not support subtractive blend
        // Thus we better render lightmap here so we do not have to create FP16 SSAO buffers
        sehle_render_context_display_frame (&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst,
            1, SEHLE_STAGE_LIGHTS);
        // Lighting for opaque geometry
        sehle_render_context_render_lights (&ctx);
        sehle_render_context_finish_frame (&ctx);
#endif
#if 1
        sehle_render_context_set_target (&ctx, tgt_viewport);
        sehle_render_context_set_viewport (&ctx, 0, 0, width, height);
        sehle_render_context_clear (&ctx, 1, 1, &EleaColor4fRed);
        static SehleProgram *prog_gamma = NULL;
		if (!prog_gamma) {
			prog_gamma = sehle_program_overlay_get_reference (engine, SEHLE_PROGRAM_OVERLAY_HAS_TEXTURE /*| SEHLE_PROGRAM_OVERLAY_HAS_EXPOSURE */, 0);
		}
		sehle_render_context_set_program (&ctx, prog_gamma);
		ctx.render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
		sehle_render_flags_clear (&ctx.render_state.flags, SEHLE_DEPTH_TEST | SEHLE_DEPTH_WRITE);
		sehle_render_flags_set (&ctx.render_state.flags, SEHLE_BLEND);
		sehle_render_context_set_texture_to_channel (&ctx, (SehleTexture *) tgt_fbuf->texcolor, 0);
		//sehle_render_context_set_texture_to_channel (&ctx, (SehleTexture *) tgt_gbuf->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH], 0);
		sehle_render_context_bind (&ctx);
		sehle_program_setUniform1i (prog_gamma, SEHLE_PROGRAM_OVERLAY_TEXTURE, 0);
		sehle_program_setUniform4fv (prog_gamma, SEHLE_PROGRAM_OVERLAY_PRIMARY, 1, EleaColor4fWhite.c);
		sehle_program_setUniform1f (prog_gamma, SEHLE_PROGRAM_OVERLAY_LWMAX, 1.0f);
		sehle_program_setUniform1f (prog_gamma, SEHLE_PROGRAM_OVERLAY_GAMMA, 1 / 2.2f);

		sehle_render_context_draw_overlay_rect_2d (&ctx, 0, 0, width, height, 0, 0, 1, 1);
#endif
#if 0
        /* Display renderable */
        sehle_render_context_display_frame(&ctx, SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION, &list.collection_inst.renderable_inst, LAYER_DEBUG, SEHLE_STAGE_FORWARD);
        /* Render everything that was submitted at display stage  */
        sehle_render_context_render_forward(&ctx);
        //sehle_render_context_render (&ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 1, 0);
        /* Clean up context */
        sehle_render_context_finish_frame(&ctx);
#endif
        SDL_GL_SwapWindow(window);
    }

    sehle_engine_delete(engine);

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(gl_context);
    SDL_Quit();

    return 0;
}
