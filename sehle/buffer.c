#define __SEHLE_BUFFER_C__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2018
*/

static const int debug = 1;

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "GL/glew.h"

#include "engine.h"
#include "buffer.h"

/*
* !DATA && !GL_UP_TO_DATE - no data set/not mapped
* !DATA &&  GL_UP_TO_DATE - not mapped
*  DATA && !GL_UP_TO_DATE - data is allocated array
*  DATA &&  GL_UP_TO_DATE - mapped
*/

static void buffer_class_init (SehleBufferClass *klass);
static void buffer_finalize (SehleBufferClass *klass, SehleBuffer *buf);

static void buffer_shutdown (AZObject *obj);
static void buffer_build (SehleResource *res);

static unsigned int buffer_type = 0;
static SehleBufferClass *buffer_class = NULL;
static SehleResourceClass *parent_class = NULL;

unsigned int
sehle_buffer_get_type (void)
{
	if (!buffer_type) {
		az_register_type (&buffer_type, (const unsigned char *) "SehleBuffer", SEHLE_TYPE_RESOURCE, sizeof (SehleBufferClass), sizeof (SehleBuffer), AZ_CLASS_IS_ABSTRACT,
			(void (*) (AZClass *)) buffer_class_init,
			NULL,
			(void (*) (const AZImplementation *, void *)) buffer_finalize);
		buffer_class = (SehleBufferClass *) az_type_get_class (buffer_type);
	}
	return buffer_type;
}

static void
buffer_class_init (SehleBufferClass *klass)
{
	parent_class = (SehleResourceClass *) ((AZClass *) klass)->parent;
	klass->resource_class.active_object_class.object_class.shutdown = buffer_shutdown;
	klass->resource_class.build = buffer_build;
}

static void
buffer_finalize (SehleBufferClass *klass, SehleBuffer *buf)
{
	if (buf->values && (buf->resource.state & SEHLE_BUFFER_PRIVATE_STORAGE)) free (buf->values);
}

static void
buffer_shutdown (AZObject *obj)
{
	SehleBuffer *buf = (SehleBuffer *) obj;
	if (debug && SEHLE_BUFFER_IS_MAPPED (buf)) {
		fprintf (stderr, "buffer_finalize: deleting mapped buffer: %s (state %x)\n", buf->resource.id, buf->resource.state);
	}
	if (buf->resource.gl_handle) {
		glDeleteBuffers (1, &buf->resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
		buf->resource.gl_handle = 0;
	}
	parent_class->active_object_class.object_class.shutdown (obj);
}

static void
buffer_build (SehleResource *res)
{
	SehleBuffer *buf = (SehleBuffer *) res;
	static const int gl_usage[] = { GL_DYNAMIC_DRAW, GL_STATIC_DRAW, GL_STREAM_DRAW };
	if (!buf->resource.gl_handle) {
		glGenBuffers (1, &buf->resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	glBindBuffer (buf->gl_type, buf->resource.gl_handle);
	SEHLE_CHECK_ERRORS (0);
	glBufferData (buf->gl_type, buf->n_elements * buf->stride * buf->element_size, buf->values, gl_usage[buf->usage]);
	SEHLE_CHECK_ERRORS (0);
	if (buf->values && (buf->resource.state & SEHLE_BUFFER_PRIVATE_STORAGE)) {
		free (buf->values);
		buf->values = NULL;
		buf->resource.state &= ~SEHLE_BUFFER_PRIVATE_STORAGE;
	}
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
}

void *
sehle_buffer_map (SehleBuffer *buf, unsigned int flags)
{
	arikkei_return_val_if_fail (sehle_resource_get_sate (&buf->resource) != SEHLE_RESOURCE_STATE_CREATED, NULL);
	arikkei_return_val_if_fail (sehle_resource_get_sate (&buf->resource) != SEHLE_RESOURCE_STATE_INVALID, NULL);
	arikkei_return_val_if_fail (!SEHLE_BUFFER_IS_MAPPED (buf), NULL);
	arikkei_return_val_if_fail (!(flags & ~(SEHLE_BUFFER_READ | SEHLE_BUFFER_WRITE)), NULL);
	SEHLE_CHECK_ERRORS (0);
	buf->resource.state |= flags;
	if (!buf->resource.engine->running) {
		/* If engine is not running create backing store */
		if (!buf->values) buf->values = malloc (buf->n_elements * buf->stride * buf->element_size);
		buf->resource.state |= SEHLE_BUFFER_PRIVATE_STORAGE;
		return buf->values;
	} else {
		static const int gl_flags[] = { 0, GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE };
		/* Ensure buffer is created and existing backing store flushed */
		if (!buf->resource.gl_handle || (sehle_resource_get_sate (&buf->resource) == SEHLE_RESOURCE_STATE_MODIFIED)) {
			/* Building buffer leaves it bound */
			sehle_resource_build (&buf->resource);
		} else {
			glBindBuffer (buf->gl_type, buf->resource.gl_handle);
			SEHLE_CHECK_ERRORS (0);
		}
		/* Map actual buffer */
		buf->values = glMapBuffer (buf->gl_type, gl_flags[flags >> 3]);
		SEHLE_CHECK_ERRORS (0);
		return buf->values;
	}
}

void
sehle_buffer_unmap (SehleBuffer *buf)
{
	arikkei_return_if_fail (sehle_resource_get_sate (&buf->resource) != SEHLE_RESOURCE_STATE_CREATED);
	arikkei_return_if_fail (sehle_resource_get_sate (&buf->resource) != SEHLE_RESOURCE_STATE_INVALID);
	arikkei_return_if_fail (SEHLE_BUFFER_IS_MAPPED (buf));
	SEHLE_CHECK_ERRORS (0);
	if (buf->resource.engine->running) {
		if (!buf->resource.gl_handle || (sehle_resource_get_sate (&buf->resource) == SEHLE_RESOURCE_STATE_MODIFIED)) {
			/* Building buffer leaves it bound */
			sehle_resource_build (&buf->resource);
		} else {
			glBindBuffer (buf->gl_type, buf->resource.gl_handle);
			SEHLE_CHECK_ERRORS (0);
			glUnmapBuffer (buf->gl_type);
			SEHLE_CHECK_ERRORS (0);
			buf->values = NULL;
		}
	}
	buf->resource.state &= ~SEHLE_BUFFER_READ_WRITE;
}

void
sehle_buffer_bind (SehleBuffer *buf)
{
	arikkei_return_if_fail (buf->resource.engine->running);
	SEHLE_CHECK_ERRORS (0);
	if (!buf->resource.gl_handle || (sehle_resource_get_sate (&buf->resource) == SEHLE_RESOURCE_STATE_MODIFIED)) {
		/* Building buffer leaves it bound */
		sehle_resource_build (&buf->resource);
	} else {
		glBindBuffer (buf->gl_type, buf->resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
}

void
sehle_buffer_setup (SehleBuffer *buf, SehleEngine *engine, const char *id, unsigned int gl_type, unsigned int element_size, unsigned int usage)
{
	sehle_resource_setup (&buf->resource, engine, (const unsigned char *) id);
	buf->gl_type = gl_type;
	buf->element_size = element_size;
	buf->stride = 1;
	buf->usage = usage;
}

void
sehle_buffer_resize (SehleBuffer *buf, unsigned int n_elements, unsigned int stride)
{
	if ((buf->n_elements == n_elements) && (buf->stride == stride)) return;
	if (buf->values && (buf->resource.state & SEHLE_BUFFER_PRIVATE_STORAGE)) {
		free (buf->values);
		buf->values = NULL;
		buf->resource.state &= ~SEHLE_BUFFER_PRIVATE_STORAGE;
	}
	buf->n_elements = n_elements;
	buf->stride = stride;
	sehle_resource_set_sate (&buf->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}
