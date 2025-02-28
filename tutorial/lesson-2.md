# Simple lighting

In this tutorial we learn how to:

- Set up RenderableList
- Create simple forward-lit material
- Create different light types

We start from the last tutorial and add, remove and modify the content as needed.

In the previous lesson we simply used render layer 1. But anticipating the future, let's
give it a descriptive name now (we still do not use more than one layer in this tutorial).

    #define LAYER_SCENE 1

As the main method is growing pretty long, let's move the geometry creation into separate
methods.

First we want to create 20mx20m flat platform under our scene.

    static void
    create_platform(SehleStaticMesh *mesh, SehleEngine *engine)
    {

Create a static mesh object.

        az_instance_init(mesh, SEHLE_TYPE_STATIC_MESH);
        sehle_static_mesh_setup(mesh, engine, LAYER_SCENE);

Instead of building VertexArray ourselves, we can use some pre-built ones provided by the Engine. For the
platform we can use 1mx1m flat grid.

        SehleVertexArray *va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_GRID_8x8);
        sehle_static_mesh_set_vertex_array(mesh, va);

SehleMaterialControl reacts only to directional lights. For more interesting lighting we use more
advanced MaterialDNS (Diffuse-Normal-Specular).

        SehlematerialDNS *mat = sehle_material_dns_new(engine);
        sehle_material_dns_set_has_colors(mat, 0);
        sehle_material_dns_set_transparent(mat, 0, 0);

Set up the StaticMesh like in the previous tutorial.

        sehle_static_mesh_resize_materials(mesh, 1);
        sehle_static_mesh_set_material(mesh, 0, SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION, &mat->material_inst);

        sehle_static_mesh_resize_fragments(mesh, 1);
        mesh->frags[0].first = 0;
        mesh->frags[0].n_indices = va->ibuf->buffer.n_elements;
        mesh->frags[0].mat_idx = 0;

And set the StaticMesh local coordinate system and bounding box (we have to scale to cover the needed area).

        EleaMat3x4f m;
        elea_mat3x4f_set_scale_xyz(&m, 20, 20, 20);
        elea_mat3x4f_translate_left_xyz(&mesh->r2w, &m, -10, -10, 0);

        elea_aabox3f_set_values(&mesh->renderable_inst.bbox, -10, -10, 0, 10, 10, 0);
    }

Also we add similar method to create cube. This one also takes the cube center as an argument.

    static void
    create_cube(SehleStaticMesh *mesh, SehleEngine *engine, EleaVec3f pos)

Everything is exactly the same as in the previous method, except we are using unit cube geometry. There are two versions of unit cube geometry - one with faces looking outside (the one we are using here) and another facing inside.

    SehleVertexArray *va = sehle_engine_get_standard_geometry(engine, SEHLE_GEOMETRY_UNIT_CUBE_OUTSIDE);

And set up the local coordinate system and the bounding box:

    elea_mat3x4f_set_translate(&mesh->r2w, &pos);
    elea_aabox3f_set_values(&mesh->renderable_inst.bbox, pos.x - 2, pos.y - 2, pos.z - 2, pos.x + 2, pos.y + 2, pos.z + 2);

In the main method, everything is the same up to and including the creation and set up of the render context.

In this tutorial we are creating more than one renderable. To avoid calling *display* and *render* methods individually for each
one, we can instead put the renderables into a collection.

SehleRenderableCollection is itself an interface type, but like with renderables and materials, there is a convenient
block type SehleRenderableList that implements a collection. It also handles bbox and layer based culling.

Renderable collection does not perform any memory management, it is strictly for grouping rendering calls. Thus the user has to ensure that all renderables remain valid during the collection lifetime.

    SehleRenderableList list;
    sehle_renderable_list_setup(&list, engine, LAYER_SCENE);

Now create a platform using our helper function.

    SehleStaticMesh platform;
    create_platform(&platform, engine);

And add it to the list.

    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &platform.renderable_inst);

Do the same with a cube.

    SehleStaticMesh mesh;
    create_cube(&mesh, engine, (EleaVec3f) {0, 0, 2});
    sehle_renderable_list_add_child(&list, SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION, &mesh.renderable_inst);

Now it is time to create some lights.

Light are renderables that can both:
 - render themselves to framebuffer in deferred lighting pass
 - give lighting information to materials in forward and transparent passes (we are using this feature here)

They are interfaces, and unlike with previous ones, there are no wrapper objects. This is because light
implementation can provide shadow maps and this has to remain implementable by user code. But in this
tutorial we can ignore shadows.

First a directional light:

Create and initialize both implementation and instance.

    SehleDirectionalLightImplementation dirl_impl;
    SehleDirectionalLightInstance dirl_inst;
    az_implementation_init((AZImplementation *) &dirl_impl, SEHLE_TYPE_DIRECTIONAL_LIGHT);
    az_interface_init((AZImplementation *) &dirl_impl, &dirl_inst);

Set up the instance.

    sehle_directional_light_setup(&dirl_inst, engine, 0, 0);

Set diffuse color (no need for other lighting types here).

    dirl_inst.light_inst.diffuse = EleaColor4fBlue;

Set up the light direction (negative z of the local coordinate system) and add the light to the collection.
Directional light always has an infinite bbox so no need to set bbox here.

    EleaVec3f light_pos = {-100, 100, 100};
    elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
    sehle_renderable_list_add_child(&list, &dirl_impl.light_impl.renderable_impl, &dirl_inst.light_inst.renderable_inst);

Now a point light:

    EleaVec3f point_pos = {5, -5, 2};
    SehlePointLightImplementation point_impl;
    SehlePointLightInstance point_inst;
    az_implementation_init((AZImplementation *) &point_impl, SEHLE_TYPE_POINT_LIGHT);
    az_interface_init((AZImplementation *) &point_impl, &point_inst);
    sehle_point_light_setup(&point_inst, engine, 0);

Here we want both ambient and diffuse lighting. Ambient light lits up even the parts that are not directly
facing the light as long as these are in light's influence area.

    point_inst.light_inst.ambient = elea_color4f_div(EleaColor4fRed, 3);
    point_inst.light_inst.diffuse = EleaColor4fRed;

Set up lighting parameters.

Point attenuation describes how the lighting intensity changes by distance
  - min_distance is the length of unlit zone immediately next to light
  - radius is the maximum distance where light reaches
  - falloff describes how fast the light decays by distance

The min_distance is not very useful for point lights, but is needed fro spotlights that share the
attenuatin parameters.

    sehle_point_light_set_point_attenuation (&point_inst, 2.0f, 10.0f, 2);

Set the local coordinate system (only position is used).

    elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);

Like all renderables, lights have bounding boxes. These have to be recalculated whenever
the local coordinate system or attenuation parameters change (the exception is directional
light that always has infinite bounding box).

    sehle_point_light_update_visuals(&point_inst);
    sehle_renderable_list_add_child(&list, &point_impl.light_impl.renderable_impl, &point_inst.light_inst.renderable_inst);

And now a spot light:

    EleaVec3f spot_pos = {-2, 2, 4};
    SehleSpotLightImplementation spot_impl;
    SehleSpotLightInstance spot_inst;
    az_implementation_init((AZImplementation *) &spot_impl, SEHLE_TYPE_SPOT_LIGHT);
    az_interface_init((AZImplementation *) &spot_impl, &spot_inst);
    sehle_spot_light_setup(&spot_inst, engine, 0);
    spot_inst.light_inst.ambient = elea_color4f_div(EleaColor4fGreen, 3);
    spot_inst.light_inst.diffuse = EleaColor4fGreen;

While a pointlight intensity depends only on the distance frome the light, spotlight
intensity depends also on the angle from the light center line.

Spot attenuation describes how the lighting intensity changes by angle
  - inner angle is the maximum angle (from center line, in radians) where light still has full intensity
  - outer_angle is the maximum angle where light reaches
  - falloff describes how fast the light decays by angle from inner to outer

Spotlight also has poinlight attenuation parameters to describe distance-based intensity falloff.

    sehle_spot_light_set_point_attenuation (&spot_inst, 0, 20.0f, 1);
    sehle_spot_light_set_spot_attenuation (&spot_inst, 0.6f, 0.75f, 1);

Spot light has complex light cone that has to be rebuilt whenever either spot or point
attenuation parameters change. It is not needed for forward lighting, but we include it here
for completeness.

    sehle_spot_light_update_geometry(&spot_inst);

Set up local coordinate system. For spotlight local negative z determines the light direction.

    elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &EleaVec3f0, &EleaVec3fZ);
    sehle_spot_light_update_visuals(&spot_inst);
    sehle_renderable_list_add_child(&list, &spot_impl.light_impl.renderable_impl, &spot_inst.light_inst.renderable_inst);

Once all renderables have been added to the collection, we let the collection to
update mask and bbox to cover all the content.

    sehle_renderable_list_update_visuals(&list);

Set up view and projection matrices

    EleaVec3f viewpoint = {10, 20, 20};
    EleaMat3x4f v2w;
    elea_mat3x4f_set_look_at(&v2w, &viewpoint, &EleaVec3f0, &EleaVec3fZ);
    EleaMat4x4f proj;
    elea_mat4x4f_set_frustum_fov(&proj, 0.01f, (float) width / height, 0.01f, 100.0f);
    sehle_render_context_set_view (&ctx, &v2w, &proj);

The event loop part is identical up to the animation part.

Rotate the cube:

                elea_mat3x4f_set_rotation_axis_angle(&mesh.r2w, &EleaVec3fZ, fmod(time / 10, 2 * M_PI));

Rotate the directional light and change it's intensity:

                light_pos.x = 100 * cos(time);
                light_pos.y = 100 * sin(time);
                elea_mat3x4f_set_look_at(&dirl_inst.light_inst.l2w, &light_pos, &EleaVec3f0, &EleaVec3fZ);
                dirl_inst.light_inst.diffuse = elea_color4f_mul(EleaColor4fBlue, (1 + sin(time / 2)) / 2);

Rotate the position of the point light around the center:

                point_pos.x = -5 * cos(-time);
                point_pos.y = -5 * sin(-time);
                elea_mat3x4f_set_translate(&point_inst.light_inst.l2w, &point_pos);

Rotate the direction of spot light:

                EleaVec3f spot_dst;
                spot_dst.x = spot_pos.x + 2 * cos(time);
                spot_dst.y = spot_pos.y + 2 * sin(time);
                spot_dst.z = 0;
                elea_mat3x4f_set_look_at(&spot_inst.light_inst.l2w, &spot_pos, &spot_dst, &EleaVec3fZ);
