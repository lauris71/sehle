#ifndef __SEHLE_VERTEXBUFFER_H__
#define __SEHLE_VERTEXBUFFER_H__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2018
*/

typedef struct _SehleVertexBuffer SehleVertexBuffer;
typedef struct _SehleVertexBufferClass SehleVertexBufferClass;

#define SEHLE_TYPE_VERTEX_BUFFER (sehle_vertex_buffer_get_type ())
#define SEHLE_VERTEX_BUFFER(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_VERTEX_BUFFER, SehleVertexBuffer))
#define SEHLE_IS_VERTEX_BUFFER(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_VERTEX_BUFFER))

#include <sehle/buffer.h>
#include <sehle/vertex-array.h>

/* OpenGL vertexbuffer object */

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleVertexBuffer {
	SehleBuffer buffer;

	/* Offsets of standard attributes */
	unsigned char offsets[SEHLE_NUM_VERTEX_BINDINGS];
	unsigned char sizes[SEHLE_NUM_VERTEX_BINDINGS];
};

struct _SehleVertexBufferClass {
	SehleBufferClass buffer_class;
};

unsigned int sehle_vertex_buffer_get_type (void);

SehleVertexBuffer *sehle_vertex_buffer_new (SehleEngine *engine, const char *id, unsigned int usage);

/* Stride is in 4-byte elements */
void sehle_vertex_buffer_setup (SehleVertexBuffer *vbuf, unsigned int n_vertices, unsigned int stride);
void sehle_vertex_buffer_setup_vnt (SehleVertexBuffer *vbuf, unsigned int n_vertices);
/* Set up vertex buffer from ATTR, VPV pairs, -1 in the end */
void sehle_vertex_buffer_setup_attrs (SehleVertexBuffer *vb, unsigned int n_vertices, int attr, ...);

ARIKKEI_INLINE
float *
sehle_vertex_buffer_map (SehleVertexBuffer *vbuf, unsigned int flags)
{
	return (float *) sehle_buffer_map (&vbuf->buffer, flags);
}

#define sehle_vertex_buffer_unmap(vb) sehle_buffer_unmap(&vb->buffer)
#define sehle_vertex_buffer_bind(vb) sehle_buffer_bind(&vb->buffer)

void sehle_vertex_buffer_bind_attributes (SehleVertexBuffer *vbuf, unsigned int loc, unsigned int size, unsigned int attr);

/* Convenience methods */

void sehle_vertex_buffer_set_value (SehleVertexBuffer *vbuf, unsigned int vidx, unsigned int attr, float val);
void sehle_vertex_buffer_set_values (SehleVertexBuffer *vbuf, unsigned int vidx, unsigned int attr, const float *values, unsigned int n_values);
void sehle_vertex_buffer_set_values_multi (SehleVertexBuffer *vbuf, unsigned int first, unsigned int n_vertices, unsigned int attr, const float *values, unsigned int n_values, unsigned int val_stride);

#ifdef __cplusplus
};
#endif

#endif

