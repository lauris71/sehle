# Rotating colored cube

- Setting up Sehle engine and RenderContext
- Creating simple geometry
- Creating material
- Display and render

But first we have to include a handful of headers from arikkei, elea and sehle libraries. You can,
of course, skip these for now and add one-by-one as needed.

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

We also need some method to create GL context and provide rendering viewport. In this tutorial we use SDL2 library, but
any other will do as well.

    #include <SDL2/SDL.h>

SDL2 needs certain amount of initialization code itself.

But first we create variables to store window size. We need them later to manage window resize events.

    static int width = 800;
    static int height = 600;

Now SDL initialization:

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

Now hopefully the base system is set up and we can progress to the actual sehle rendering system.

The first thing is always to create an instance of SehleEngine. This is an opaque object that supervises
the whole rendering system. It provides:

- OpenGL libary initialization
- resource allocation and sharing
- render state bookkeeping

Starting engine initializes OpenGL and makes the context accessible to sehle resource objects.
Although most objects can be created while the engine is stopped, the actual graphics data will
not be built before it is started.

    SehleEngine *engine = sehle_engine_new();
    sehle_engine_ensure_started(engine);
    if (!engine->running) {
        fprintf (stderr, "main:Cannot start sehle engine (invalid/missing OpenGL context?)");
        exit(1);
    }

Next we need render target to draw on. In simpler cases it can be viewport target - i.e. the one
rendering directly to the screen. RenderTarget is a resource - a heavyweight reference-counted
object. Other resource types are textures, vertex buffers and shader programs.

We create the target the same size as previously created SDL window. Whenever window size changes, viewport
has to be resized manually (later we use appropriate SDL event for that).

    SehleRenderTarget *tgt = sehle_render_target_new_viewport(engine, 800, 600);

Now when the render target is set up we create render context.

Render context holds the data related to one rendering task, both low and high-level.

- engine state
- viewport rectangle
- active render target
- active shader program
- render lists
- camera matrixes
- data for high-level materials

If renderables, material system and renderers are not used it is not needed

    SehleRenderContext ctx;
    sehle_render_context_setup(&ctx, engine);
    ctx.global_ambient = EleaColor4fWhite;
    sehle_render_context_set_target (&ctx, tgt);
    sehle_render_context_set_viewport (&ctx, 0, 0, width, height);

Render context is plain block type object and thus has to be managed externally.

Now it is time to create some geometry. First we create vertex and index buffers and map these to CPU memory.

Vertex and index buffers are another example of resource class. The engine caches resources by name - i.e. if
a resource of the same class with the same name is already created it will be re-used.

Vertex buffer contains an array of (tuples of) floats. For our purpose we need 9 floats per tuple (vertex):

- Coordinate: 3 floats
- TexCoord: 2 floats
- Color: 4 floats

Index buffer contains an array of 32-bit integers.

We need 24 vertices (4 per side) and 36 indices (6 per side). For smooth objects the vertices can be shared
between neighbouring faces, but not for sharp-edged cube.

    SehleVertexBuffer *vbuf = sehle_vertex_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_vertex_buffer_setup_attrs(vbuf, 24, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_TEXCOORD0, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
    SehleIndexBuffer *ibuf = sehle_index_buffer_new(engine, "HelloCube", SEHLE_BUFFER_STATIC);
    sehle_index_buffer_resize(ibuf, 36);
    float *vertices = sehle_vertex_buffer_map(vbuf, SEHLE_BUFFER_WRITE);
    uint32_t *indices = sehle_index_buffer_map(ibuf, SEHLE_BUFFER_WRITE);

Fortunately Elea has a method to generate both cube vertices (coordinates and normals) and indices. We
have to specify the cube size (lower and upper bounds), vertex layout and whether we need normals to
face outwards on inwards.

    EleaVec3f p0 = {-1, -1, -1};
    EleaVec3f p1 = {1, 1, 1};
    elea_generate_box(vertices, 9 * 4, NULL, 0, vertices + 3, 9 * 4, indices, &p0, &p1, 1);

The cube generator did not assign colors so we have to do that manually. We just set color channels to
(normalized) coordinate values for each vertex.

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

At last we unmap buffers, i.e. tell OpenGL that the data can be moved to video memory.

    sehle_vertex_buffer_unmap(vbuf);
    sehle_index_buffer_unmap(ibuf);

And attach vertex and index buffers to SehleVertexArray. The latter is simply and ocntainer of one or
more vertex buffers and one index buffer that simplifies (and speeds up) vertex binding for render calls.

    SehleVertexArray *va = sehle_vertex_array_new_from_buffers(engine, (const uint8_t *) "HelloCube", vbuf, ibuf);

To draw anything using the standard pipeline we need at least one SehleRenderable instance.

SehleRenderable is an interface, i.e. it has to be instantiated somewhere and it's behaviour specified
by creating Implementation. Renderables have two methods in implementation:

- display - submit renderable to display list
- render - draw the geometry (called from the displa list)

For simple purposes we can use SehleStaticMesh - a block type that implements renderable interface. It is
plain block type and thus has to be initialized and destroyed by hand. For the purpose of current tutorial
we just allocate it from stack.

Renderable interface supports 32 distinct render layers - basically a user-defined mask that allows one
to switch renderable display on and off.

We also attach created vertex array to static mesh and specify it's bounding box for fast visibility culling.

    SehleStaticMesh mesh;
    az_instance_init(&mesh, SEHLE_TYPE_STATIC_MESH);
    /* Bind static mesh to engine and set render layers */
    sehle_static_mesh_setup(&mesh, engine, 1);
    /* Set up static mesh geometry and bounding box */
    sehle_static_mesh_set_vertex_array(&mesh, va);
    elea_aabox3f_set_values(&mesh.renderable_inst.bbox, -2, -2, -2, 2, 2, 2);

RenderContext requires material, so we have to set up one. We use the most simple forward-rendered SehleMaterialControl.
It is intended for gizmos, controls and similar things but for our tutorial it is enough.
SehleMaterial is actually interface type, but SehleMaterialControl implements it so we do not have to set up the
implementation ourselves.

    SehleMaterialControl mat;
    sehle_material_control_init(&mat, engine);
    sehle_material_control_set_has_colors(&mat, 1);
    mat.ambient = EleaColor4fWhite;
    mat.color = EleaColor4fWhite;

Now we add created material to static mesh material slot 0.

    sehle_static_mesh_resize_materials(&mesh, 1);
    sehle_static_mesh_set_material(&mesh, 0, SEHLE_MATERIAL_CONTROL_MATERIAL_IMPLEMENTATION, &mat.material_inst);

Single static mesh can have multile fragments with multiple materials, LOD levels and so on. We just create single fragment,
assign all indices to it and set it's material index to 0.

    sehle_static_mesh_resize_fragments(&mesh, 1);
    mesh.frags[0].first = 0;
    mesh.frags[0].n_indices = va->ibuf->buffer.n_elements;
    mesh.frags[0].mat_idx = 0;

Now set up view matrices in SehleRenderContexxt.

    EleaVec3f viewpoint = {2, 4, 3};
    EleaMat3x4f v2w;
    elea_mat3x4f_set_look_at(&v2w, &viewpoint, &EleaVec3f0, &EleaVec3fZ);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 0.01f, (float) width / height, 0.1f, 100.0f);
    /* Update view parameters in render context  */
    sehle_render_context_set_view (&ctx, &v2w, &proj);

Now start the main rendering loop

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

Resize the render target to match window. This is only needed for proper framebuffers (not window taget) but we keep it here

                        sehle_render_target_resize(tgt, width, height);

Resize render context (needed to keep the engine state up to date)

                        sehle_render_context_set_viewport(&ctx, 0, 0, width, height);
                        break;
                }
                break;
            case SDL_QUIT:
                alive = 0;
                break;
            default:
                time = arikkei_get_time() - start;

Rotate the static mesh. Normally whenever the local coordinate system changes, bbox should be updated, but we
set the original bbox large enough to contain the rotated version.

                elea_mat3x4f_set_rotation_axis_angle(&mesh.r2w, &EleaVec3fZ, fmod(time, 2 * M_PI));
                break;
        }

Now come the actual rendering calls.

Render context should be bound before rendering - i.e. update OpenGL render state to match the one of RenderContext.

        sehle_render_context_bind(&ctx);

Clear frame to uniform color.

        EleaColor4f bg = {0.32f, 0.3f, 0.27f};
        sehle_render_context_clear (&ctx, 1, 1, &bg);

Display all our renderables. This calls *display* virtual method of renderables, submitting geometry for subsequent render calls.

Display uses the concept of render stages. A stage one semantic part of multi-pass rendering (e.g. STAGE_SOLID, STAGE_TRANSPARENT,
STAGE_LIGHTING). Here we use forward stage.

        sehle_render_context_display_frame(&ctx, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &mesh.renderable_inst, 1, SEHLE_STAGE_FORWARD | SEHLE_STAGE_SOLID);

Render everything that was submitted in display pass. This calls the *render* virtual method of renderables, sending submitted
data to OpenGL engine.

Render uses both display stage and render type. Render type defines, which parts and properties of renderale are relevant and how they should be handles (e.g. RENDER_DEPTH, RENDER_DENSITY, RENDER_AMBIENT). Here we use RENDER_FORWARD type.

        sehle_render_context_render (&ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 1, 0);

Clean up RenderContext (free list etc.).

        sehle_render_context_finish_frame(&ctx);
        
        SDL_GL_SwapWindow(window);
    }

When we finish it is good idea to delete engine.

    sehle_engine_delete(engine);

And shut down SDL

    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(gl_context);
    SDL_Quit();
