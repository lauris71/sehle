#ifndef __SEHLE_STATICMESH_H__
#define __SEHLE_STATICMESH_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

/*
 * SehleStaticMesh
 *
 * Simple geometry using static buffers and multiple materials
 * It vertex and index buffer are assigned it claims a reference to these
 * StaticMesh is STANDALONE objects that implements renderable
 */

typedef struct _SehleStaticMesh SehleStaticMesh;
typedef struct _SehleStaticMeshClass SehleStaticMeshClass;
typedef struct _SehleStaticMeshFragment SehleStaticMeshFragment;

#define SEHLE_TYPE_STATIC_MESH sehle_static_mesh_get_type ()

#define SEHLE_STATIC_MESH_FROM_RENDERABLE_INSTANCE(i) (SehleStaticMesh *) ARIKKEI_BASE_ADDRESS (SehleStaticMesh, renderable_inst, i)
#define SEHLE_STATIC_MESH_RENDERABLE_IMPLEMENTATION (&sehle_static_mesh_class->renderable_impl)

#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleStaticMeshFragment {
	unsigned int lod : 8;
	unsigned int mat_idx : 8;
	unsigned int first;
	unsigned int n_indices;
};

/**
 * @brief A simple mesh renderable that supports multiple materials and LOD levels
 * 
 */
struct _SehleStaticMesh {
	SehleRenderable renderable_inst;

	EleaMat3x4f r2w;

	SehleVertexArray *va;

	// Data for sorted rendering
	unsigned int nsortedindices;
	uint32_t *sortedindices;

	unsigned int nmaterials;
	SehleMaterialHandle *materials;
	unsigned int nfrags;
	SehleStaticMeshFragment *frags;
	unsigned int ninstances;
	EleaMat3x4f *instances;
	unsigned int nlods;
	float lods[8];
};

struct _SehleStaticMeshClass {
	AZClass klass;
	SehleRenderableImplementation renderable_impl;
};

unsigned int sehle_static_mesh_get_type (void);

#ifndef __SEHLE_STATIC_MESH_C__
extern unsigned int sehle_static_mesh_type;
extern SehleStaticMeshClass *sehle_static_mesh_class;
#endif

/**
 * @brief Release all references and make mesh invisible
 * 
 * @param mesh a static mesh
 */
void sehle_static_mesh_clear (SehleStaticMesh *mesh);

/**
 * @brief Grab reference of vertex array
 * 
 * @param mesh a static mesh
 * @param va vertex array
 */
void sehle_static_mesh_set_vertex_array (SehleStaticMesh *mesh, SehleVertexArray *va);
/*
 * 
 * Attribute is present if size > 0
 */

/**
 * @brief Grab reference of vertex buffer
 * 
 * Grab a reference of vertex buffer and set the vertex array from it's attributes that
 * have size > 0.
 * @param mesh a static mesh
 * @param vbuf vertex buffer
 */
void sehle_static_mesh_set_vertex_data (SehleStaticMesh *mesh, SehleVertexBuffer *vbuf);
/**
 * @brief Grab reference of index buffer
 * 
 * @param mesh a static mesh
 * @param ibuf index buffer
 */
void sehle_static_mesh_set_index_data (SehleStaticMesh *mesh, SehleIndexBuffer *ibuf);


/**
 * @brief Allocate space for required number of materials
 * 
 * If hte current number of materials is > length, the references of remaining materials
 * are dropped.
 * @param mesh a static mesh
 * @param length the number of materials
 */
void sehle_static_mesh_resize_materials (SehleStaticMesh *mesh, unsigned int length);
// Set material and grab reference
void sehle_static_mesh_set_material (SehleStaticMesh *mesh, unsigned int idx, SehleMaterialImplementation *impl, SehleMaterialInstance *inst);
// Allocate space for required number of fragments
void sehle_static_mesh_resize_fragments (SehleStaticMesh *mesh, unsigned int length);
// Allocate space for required number of instances
void sehle_static_mesh_resize_instances (SehleStaticMesh *mesh, unsigned int ninstances);

void sehle_static_mesh_setup (SehleStaticMesh *mesh, SehleEngine *engine, unsigned int layer_mask);
void sehle_static_mesh_release (SehleStaticMesh *mesh);
SehleStaticMesh *sehle_static_mesh_new (SehleEngine *engine, unsigned int layer_mask);
void sehle_static_mesh_delete (SehleStaticMesh *mesh);

#ifdef __cplusplus
};
#endif

#endif

