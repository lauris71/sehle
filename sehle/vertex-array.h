#ifndef __SEHLE_VERTEX_ARRAY_H__
#define __SEHLE_VERTEX_ARRAY_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2020
 *
 */

#define SEHLE_TYPE_VERTEX_ARRAY (sehle_vertex_array_get_type ())
#define SEHLE_VERTEX_ARRAY(r) (AZ_CHECK_INSTANCE_CAST ((r), SEHLE_TYPE_VERTEX_ARRAY, SehleVertexArray))
#define SEHLE_IS_VERTEX_ARRAY(r) (AZ_CHECK_INSTANCE_TYPE ((r), SEHLE_TYPE_VERTEX_ARRAY))

typedef struct _SehleVertexArray SehleVertexArray;
typedef struct _SehleVertexArrayClass SehleVertexArrayClass;

#include <sehle/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleVertexArray {
	SehleResource resource;
	SehleVertexBuffer *vbufs[SEHLE_NUM_VERTEX_BINDINGS];
	SehleIndexBuffer *ibuf;
};

struct _SehleVertexArrayClass {
	SehleResourceClass resource_klass;
};

unsigned int sehle_vertex_array_get_type (void);

SehleVertexArray *sehle_vertex_array_new (SehleEngine *engine, const unsigned char *id);
SehleVertexArray *sehle_vertex_array_new_from_buffers (SehleEngine *engine, const unsigned char *id, SehleVertexBuffer *vb, SehleIndexBuffer *ib);
/* Setup buffers from attr/vpv pairs, -1 is end */
SehleVertexArray *sehle_vertex_array_new_from_attrs (SehleEngine *engine, const unsigned char *id, unsigned int n_vertices, unsigned int n_indices, int attr, ...);

void sehle_vertex_array_bind (SehleVertexArray *va);

void sehle_vertex_array_set_vertex_data (SehleVertexArray *va, unsigned int attr, SehleVertexBuffer *vbuf);
void sehle_vertex_array_set_index_data (SehleVertexArray *va, SehleIndexBuffer *ibuf);

void sehle_vertex_array_render_triangles (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices);
void sehle_vertex_array_render_triangles_instanced (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices, unsigned int n_instances);
void sehle_vertex_array_render_lines (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices);
void sehle_vertex_array_render_points (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices);

#ifdef __cplusplus
};
#endif

#endif
