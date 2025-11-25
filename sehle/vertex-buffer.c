#define __SEHLE_VERTEX_BUFFER_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

static const int debug = 1;

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "GL/glew.h"

#include <az/extend.h>

#include "engine.h"
#include "vertex-buffer.h"

unsigned int
sehle_vertex_buffer_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleVertexBuffer", SEHLE_TYPE_BUFFER, sizeof (SehleVertexBufferClass), sizeof (SehleVertexBuffer), 0,
			NULL, NULL, NULL);
	}
	return type;
}

SehleVertexBuffer *
sehle_vertex_buffer_new (SehleEngine *engine, const char *id, unsigned int usage)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	arikkei_return_val_if_fail (SEHLE_IS_ENGINE (engine), NULL);
	arikkei_return_val_if_fail (usage <= SEHLE_BUFFER_STREAM, NULL);
	SehleVertexBuffer *vbuf = (SehleVertexBuffer *) az_object_new (SEHLE_TYPE_VERTEX_BUFFER);
	sehle_buffer_setup (&vbuf->buffer, engine, id, GL_ARRAY_BUFFER, 4, usage);
	return vbuf;
}

void
sehle_vertex_buffer_setup (SehleVertexBuffer *vbuf, unsigned int n_vertices, unsigned int stride)
{
	arikkei_return_if_fail (vbuf != NULL);
	arikkei_return_if_fail (SEHLE_IS_VERTEX_BUFFER (vbuf));
	arikkei_return_if_fail (!SEHLE_BUFFER_IS_MAPPED (&vbuf->buffer));
	memset (vbuf->offsets, 0, sizeof (vbuf->offsets));
	memset (vbuf->sizes, 0, sizeof (vbuf->sizes));
	sehle_buffer_resize (&vbuf->buffer, n_vertices, stride);
}

void
sehle_vertex_buffer_setup_vnt (SehleVertexBuffer *vbuf, unsigned int n_vertices)
{
	arikkei_return_if_fail (vbuf != NULL);
	arikkei_return_if_fail (SEHLE_IS_VERTEX_BUFFER (vbuf));
	arikkei_return_if_fail (!SEHLE_BUFFER_IS_MAPPED (&vbuf->buffer));
	sehle_vertex_buffer_setup_attrs (vbuf, n_vertices, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_NORMAL, 3, SEHLE_ATTRIBUTE_TEXCOORD, 2, -1);
}

void
sehle_vertex_buffer_setup_attrs (SehleVertexBuffer *vb, unsigned int n_vertices, int attr, ...)
{
	va_list ap;
	unsigned int attrs[SEHLE_NUM_VERTEX_BINDINGS] = { 0 }, vpv[SEHLE_NUM_VERTEX_BINDINGS] = { 0 }, stride = 0, n_attrs = 0;
	arikkei_return_if_fail (vb != NULL);
	arikkei_return_if_fail (SEHLE_IS_VERTEX_BUFFER (vb));
	arikkei_return_if_fail (!SEHLE_BUFFER_IS_MAPPED (&vb->buffer));
	va_start (ap, attr);
	while ((attr >= 0) && (n_attrs <= SEHLE_NUM_VERTEX_BINDINGS)) {
		attrs[n_attrs] = attr;
		vpv[n_attrs] = va_arg (ap, int);
		stride += vpv[n_attrs];
		n_attrs += 1;
		attr = va_arg (ap, int);
	}
	va_end (ap);
	sehle_vertex_buffer_setup (vb, n_vertices, stride);
	for (unsigned int i = 0; i < n_attrs; i++) {
		vb->sizes[attrs[i]] = vpv[i];
	}
	unsigned int offset = 0;
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		vb->offsets[i] = offset;
		offset += vb->sizes[i];
	}
}

void
sehle_vertex_buffer_bind_attributes (SehleVertexBuffer *vbuf, unsigned int loc, unsigned int size, unsigned int attr)
{
	arikkei_return_if_fail (vbuf != NULL);
	arikkei_return_if_fail (SEHLE_IS_VERTEX_BUFFER (vbuf));
	arikkei_return_if_fail (!SEHLE_BUFFER_IS_MAPPED (&vbuf->buffer));
	SEHLE_CHECK_ERRORS (0);
	sehle_buffer_bind (&vbuf->buffer);
	SEHLE_CHECK_ERRORS (0);
	glVertexAttribPointer (loc, size, GL_FLOAT, 1, vbuf->buffer.stride * 4, (float *) NULL + vbuf->offsets[attr]);
	SEHLE_CHECK_ERRORS (0);
	glEnableVertexAttribArray (loc);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_vertex_buffer_set_value (SehleVertexBuffer *vbuf, unsigned int vidx, unsigned int attr, float val)
{
	arikkei_return_if_fail (vbuf->buffer.resource.state & SEHLE_BUFFER_WRITE);
	((float *) vbuf->buffer.values)[vidx * vbuf->buffer.stride + attr] = val;
}

void
sehle_vertex_buffer_set_values (SehleVertexBuffer *vbuf, unsigned int vidx, unsigned int attr, const float *values, unsigned int n_values)
{
	arikkei_return_if_fail (vbuf->buffer.resource.state & SEHLE_BUFFER_WRITE);
	memcpy ((float *) vbuf->buffer.values + vidx * vbuf->buffer.stride + attr, values, n_values * 4);
}

void
sehle_vertex_buffer_set_values_multi (SehleVertexBuffer *vbuf, unsigned int first, unsigned int n_vertices, unsigned int attr, const float *values, unsigned int n_values, unsigned int val_stride)
{
	arikkei_return_if_fail (vbuf->buffer.resource.state & SEHLE_BUFFER_WRITE);
	for (unsigned int i = 0; i < n_vertices; i++) {
		sehle_vertex_buffer_set_values (vbuf, first + i, attr, (float *) ((char *) values + i * val_stride), n_values);
	}
}

