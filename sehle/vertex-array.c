#define __SEHLE_VERTEX_ARRAY_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2020
 *
 */

static const int debug = 0;

#include <stdarg.h>

#include <arikkei/arikkei-utils.h>

#include "GL/glew.h"

#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/vertex-buffer.h>

#include <sehle/vertex-array.h>

static void vertex_array_class_init (SehleVertexArrayClass *klass);

/* AZObject implementation */
static void vertex_array_shutdown (AZObject *object);
/* SehleResource implementation */
static void vertex_array_build (SehleResource *res);

static SehleResourceClass *parent_class;

unsigned int
sehle_vertex_array_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleVertexArray", SEHLE_TYPE_RESOURCE, sizeof (SehleVertexArrayClass), sizeof (SehleVertexArray), 0,
			(void (*) (AZClass *)) vertex_array_class_init,
			NULL, NULL);
	}
	return type;
}

static void
vertex_array_class_init (SehleVertexArrayClass *klass)
{
	parent_class = (SehleResourceClass *) ((AZClass *) klass)->parent;
	klass->resource_klass.active_object_class.object_class.shutdown = vertex_array_shutdown;
	klass->resource_klass.build = vertex_array_build;
}

static void
vertex_array_shutdown (AZObject *object)
{
	SehleVertexArray *va = (SehleVertexArray  *) object;
	if (va->resource.gl_handle) {
		glDeleteVertexArrays (1, &va->resource.gl_handle);
		va->resource.gl_handle = 0;
	}
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		if (va->vbufs[i]) {
			az_object_unref ((AZObject *) va->vbufs[i]);
			va->vbufs[i] = NULL;
		}
	}
	if (va->ibuf) {
		az_object_unref ((AZObject*) va->ibuf);
		va->ibuf = NULL;
	}
	if (((AZObjectClass *) parent_class)->shutdown) {
		((AZObjectClass *) parent_class)->shutdown (object);
	}
}

static void
vertex_array_build (SehleResource* res)
{
	SehleVertexArray *va = (SehleVertexArray *) res;
	if (!va->resource.gl_handle) {
		glGenVertexArrays (1, &va->resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	glBindVertexArray(va->resource.gl_handle);
	SEHLE_CHECK_ERRORS (0);
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		if (va->vbufs[i]) {
			sehle_vertex_buffer_bind_attributes (va->vbufs[i], i, va->vbufs[i]->sizes[i], i);
		} else {
			glDisableVertexAttribArray (i);
		}
	}
	SEHLE_CHECK_ERRORS (0);
	if (va->ibuf) {
		sehle_index_buffer_bind (va->ibuf);
	}
	/*
	 * We have to keep VertexArray unbound whenever it is not in use
	 * Otherwise arbitrary Vertex/Index buffer bind (e.g. mapping) replaces the corresponding array
	 */
	glBindVertexArray(0);
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
}

SehleVertexArray *
sehle_vertex_array_new (SehleEngine* engine, const unsigned char* id)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	arikkei_return_val_if_fail (SEHLE_IS_ENGINE (engine), NULL);
	SehleVertexArray *va = (SehleVertexArray *) az_object_new (SEHLE_TYPE_VERTEX_ARRAY);
	sehle_resource_setup (&va->resource, engine, id);
	return va;
}

SehleVertexArray *
sehle_vertex_array_new_from_buffers (SehleEngine *engine, const unsigned char *id, SehleVertexBuffer *vb, SehleIndexBuffer *ib)
{
	SehleVertexArray *va = sehle_vertex_array_new (engine, id);
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		if (vb->sizes[i]) sehle_vertex_array_set_vertex_data (va, i, vb);
	}
	sehle_vertex_array_set_index_data (va, ib);
	return va;
}

SehleVertexArray *
sehle_vertex_array_new_from_attrs (SehleEngine *engine, const unsigned char *id, unsigned int n_vertices, unsigned int n_indices, int attr, ...)
{
	va_list ap;
	unsigned int attrs[SEHLE_NUM_VERTEX_BINDINGS] = { 0 }, vpv[SEHLE_NUM_VERTEX_BINDINGS] = { 0 }, stride = 0, n_attrs = 0;
	va_start (ap, attr);
	while ((attr >= 0) && (n_attrs <= SEHLE_NUM_VERTEX_BINDINGS)) {
		attrs[n_attrs] = attr;
		vpv[n_attrs] = va_arg (ap, int);
		stride += vpv[n_attrs];
		n_attrs += 1;
		attr = va_arg (ap, int);
	}
	va_end (ap);
	SehleVertexArray* va = sehle_vertex_array_new (engine, id);
	SehleVertexBuffer *vb = sehle_engine_get_vertex_buffer (engine, (const char *) id, SEHLE_BUFFER_STATIC);
	sehle_vertex_buffer_setup (vb, n_vertices, stride);
	for (unsigned int i = 0; i < n_attrs; i++) {
		vb->sizes[attrs[i]] = vpv[i];
	}
	unsigned int offset = 0;
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		vb->offsets[i] = offset;
		offset += vb->sizes[i];
	}
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		if (vb->sizes[i]) sehle_vertex_array_set_vertex_data (va, i, vb);
	}
	az_object_unref ((AZObject *) vb);
	SehleIndexBuffer *ib = sehle_engine_get_index_buffer (engine, (const char *) id, SEHLE_BUFFER_STATIC);
	sehle_index_buffer_resize (ib, n_indices);
	sehle_vertex_array_set_index_data (va, ib);
	az_object_unref ((AZObject *) ib);
	return va;
}

void
sehle_vertex_array_bind (SehleVertexArray* va)
{
	arikkei_return_if_fail (va->resource.engine->running);
	SEHLE_CHECK_ERRORS (0);
	if (!va->resource.gl_handle || (sehle_resource_get_sate (&va->resource) == SEHLE_RESOURCE_STATE_MODIFIED)) {
		/* Building vertex array leaves it bound */
		sehle_resource_build (&va->resource);
	}
	glBindVertexArray (va->resource.gl_handle);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_vertex_array_set_vertex_data (SehleVertexArray *va, unsigned int attr, SehleVertexBuffer *vbuf)
{
	if (va->vbufs[attr]) az_object_unref ((AZObject *) va->vbufs[attr]);
	va->vbufs[attr] = vbuf;
	if (va->vbufs[attr]) az_object_ref ((AZObject *) va->vbufs[attr]);
	sehle_resource_set_sate (&va->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

void
sehle_vertex_array_set_index_data (SehleVertexArray *va, SehleIndexBuffer *ibuf)
{
	if (va->ibuf) az_object_unref ((AZObject *) va->ibuf);
	va->ibuf = ibuf;
	if (va->ibuf) az_object_ref ((AZObject *) va->ibuf);
	sehle_resource_set_sate (&va->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

void
sehle_vertex_array_render_triangles (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices)
{
	SEHLE_CHECK_ERRORS (0);
	if (bind) {
		sehle_vertex_array_bind (va);
		SEHLE_CHECK_ERRORS (0);
	}
	glDrawElements (GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + first_index);
	if (bind) {
		glBindVertexArray(0);
	}
#ifdef SEHLE_PERFORMANCE_MONITOR
	va->resource.engine->counter.indices += n_indices;
	va->resource.engine->counter.indexinstances += n_indices;
	va->resource.engine->counter.geometry_batches += 1;
#endif
}

void
sehle_vertex_array_render_triangles_instanced (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices, unsigned int n_instances)
{
	SEHLE_CHECK_ERRORS (0);
	if (bind) {
		sehle_vertex_array_bind (va);
		SEHLE_CHECK_ERRORS (0);
	}
	glDrawElementsInstanced (GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + first_index, n_instances);
	if (bind) {
		glBindVertexArray(0);
	}
#ifdef SEHLE_PERFORMANCE_MONITOR
	va->resource.engine->counter.indices += n_indices;
	va->resource.engine->counter.indexinstances += n_instances * n_indices;
	va->resource.engine->counter.geometry_batches += n_instances;
#endif
}

void
sehle_vertex_array_render_lines (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices)
{
	SEHLE_CHECK_ERRORS (0);
	if (bind) {
		sehle_vertex_array_bind (va);
		SEHLE_CHECK_ERRORS (0);
	}
	glDrawElements (GL_LINES, n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + first_index);
	if (bind) {
		glBindVertexArray(0);
	}
#ifdef SEHLE_PERFORMANCE_MONITOR
	va->resource.engine->counter.geometry_batches += 1;
#endif
}

void
sehle_vertex_array_render_points (SehleVertexArray *va, unsigned int bind, unsigned int first_index, unsigned int n_indices)
{
	SEHLE_CHECK_ERRORS (0);
	if (bind) {
		sehle_vertex_array_bind (va);
		SEHLE_CHECK_ERRORS (0);
	}
	glDrawElements (GL_POINTS, n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + first_index);
	//glBindVertexArray (0);
#ifdef SEHLE_PERFORMANCE_MONITOR
	va->resource.engine->counter.geometry_batches += 1;
#endif
}
