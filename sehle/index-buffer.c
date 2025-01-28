#define __SEHLE_INDEX_BUFFER_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2018
//

static const int debug = 1;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "GL/glew.h"

#include "engine.h"
#include "index-buffer.h"

unsigned int
sehle_index_buffer_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleIndexBuffer", SEHLE_TYPE_BUFFER, sizeof (SehleIndexBufferClass), sizeof (SehleIndexBuffer), 0, NULL, NULL, NULL);
	}
	return type;
}

SehleIndexBuffer *
sehle_index_buffer_new (SehleEngine *engine, const char *id, unsigned int usage)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	arikkei_return_val_if_fail (SEHLE_IS_ENGINE (engine), NULL);
	arikkei_return_val_if_fail (usage <= SEHLE_BUFFER_STREAM, NULL);
	SehleIndexBuffer *ibuf = (SehleIndexBuffer *) az_object_new (SEHLE_TYPE_INDEX_BUFFER);
	sehle_buffer_setup (&ibuf->buffer, engine, id, GL_ELEMENT_ARRAY_BUFFER, 4, usage);
	return ibuf;
}

void
sehle_index_buffer_resize (SehleIndexBuffer *ibuf, unsigned int size)
{
	arikkei_return_if_fail (ibuf != NULL);
	arikkei_return_if_fail (SEHLE_IS_INDEX_BUFFER (ibuf));
	arikkei_return_if_fail (!SEHLE_BUFFER_IS_MAPPED ((SehleBuffer *) ibuf));
	sehle_buffer_resize (&ibuf->buffer, size, 1);
}

