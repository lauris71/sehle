#ifndef __SEHLE_BUFFER_H__
#define __SEHLE_BUFFER_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2018
 */

typedef struct _SehleBuffer SehleBuffer;
typedef struct _SehleBufferClass SehleBufferClass;

#define SEHLE_TYPE_BUFFER (sehle_buffer_get_type ())
#define SEHLE_BUFFER(p) (ARIKKEI_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_BUFFER, SehleBuffer))
#define SEHLE_IS_BUFFER(p) (ARIKKEI_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_BUFFER))

#include <sehle/resource.h>

/* OpenGL buffer object */

/* Storage is allocated by buffer */
#define SEHLE_BUFFER_PRIVATE_STORAGE 4
/* Mapping modes */
#define SEHLE_BUFFER_READ 8
#define SEHLE_BUFFER_WRITE 16
#define SEHLE_BUFFER_READ_WRITE (SEHLE_BUFFER_READ | SEHLE_BUFFER_WRITE)

/* Usage hints */
#define SEHLE_BUFFER_DYNAMIC 0
#define SEHLE_BUFFER_STATIC 1
#define SEHLE_BUFFER_STREAM 2

#define SEHLE_BUFFER_IS_MAPPED(b) (((b)->resource.state & SEHLE_BUFFER_READ_WRITE) != 0)

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleBuffer {
	SehleResource resource;
	/* OpenGL buffer type */
	unsigned int gl_type;
	unsigned int element_size : 4;
	/* Stride is in elements */
	unsigned int stride : 8;
	/* Usage hint */
	unsigned int usage : 2;
	/* Mapping */
	unsigned int n_elements;
	/* Data is either private backing store or pointer to mapped buffer */
	void *values;
};

struct _SehleBufferClass {
	SehleResourceClass resource_class;
};

unsigned int sehle_buffer_get_type (void);

void *sehle_buffer_map (SehleBuffer *buf, unsigned int flags);
void sehle_buffer_unmap (SehleBuffer *buf);

void sehle_buffer_bind (SehleBuffer *buf);

/* For subclasses */
void sehle_buffer_setup (SehleBuffer *buf, SehleEngine *engine, const char *id, unsigned int gl_type, unsigned int element_size, unsigned int usage);
void sehle_buffer_resize (SehleBuffer *buf, unsigned int n_elements, unsigned int stride);

#ifdef __cplusplus
};
#endif

#endif

