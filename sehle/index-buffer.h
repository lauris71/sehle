#ifndef __SEHLE_INDEXBUFFER_H__
#define __SEHLE_INDEXBUFFER_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2018
//

typedef struct _SehleIndexBuffer SehleIndexBuffer;
typedef struct _SehleIndexBufferClass SehleIndexBufferClass;

#define SEHLE_TYPE_INDEX_BUFFER (sehle_index_buffer_get_type ())
#define SEHLE_INDEX_BUFFER(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_INDEXBUFFER, SehleIndexBuffer))
#define SEHLE_IS_INDEX_BUFFER(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_INDEX_BUFFER))

#include <sehle/buffer.h>

/* OpenGL indexbuffer object */

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleIndexBuffer {
	SehleBuffer buffer;
};

struct _SehleIndexBufferClass {
	SehleBufferClass resource_class;
};

unsigned int sehle_index_buffer_get_type (void);

SehleIndexBuffer *sehle_index_buffer_new (SehleEngine *engine, const char *id, unsigned int usage);

void sehle_index_buffer_resize (SehleIndexBuffer *ibuf, unsigned int size);

static inline uint32_t *
sehle_index_buffer_map (SehleIndexBuffer *ibuf, unsigned int flags)
{
	return (uint32_t *) sehle_buffer_map (&ibuf->buffer, flags);
}

#define sehle_index_buffer_unmap(ib) sehle_buffer_unmap(&(ib)->buffer)
#define sehle_index_buffer_bind(ib) sehle_buffer_bind(&(ib)->buffer)

#ifdef __cplusplus
};
#endif

#endif

