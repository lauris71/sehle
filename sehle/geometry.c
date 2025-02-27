#define __SEHLE_GEOMETRY_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <elea/geometry.h>
#include <elea/vector2.h>
#include <elea/vector3.h>

#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/vertex-buffer.h>

SehleVertexArray *
sehle_build_geometry (SehleEngine *engine, unsigned int type)
{
	SehleVertexBuffer *vb;
	SehleIndexBuffer *ib;
	SehleVertexArray *va = NULL;
	static const EleaVec3f p0 = { -1, -1, -1 }, p1 = { 1, 1, 1 };
	if (type == SEHLE_GEOMETRY_TEXTURE_RECT) {
		vb = sehle_engine_get_vertex_buffer (engine, "TextureRect", SEHLE_BUFFER_STATIC);
		ib = sehle_engine_get_index_buffer (engine, "TextureRect", SEHLE_BUFFER_STATIC);
		if (sehle_resource_get_sate (&vb->buffer.resource) == SEHLE_RESOURCE_STATE_CREATED) {
			static const float c[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
			static const unsigned int p[] = { 0, 1, 2, 0, 2, 3 };
			sehle_vertex_buffer_setup_attrs (vb, 4, SEHLE_ATTRIBUTE_VERTEX, 0, SEHLE_ATTRIBUTE_TEXCOORD, 2, -1);
			float *attribs = (float *) sehle_vertex_buffer_map (vb, SEHLE_BUFFER_WRITE);
			memcpy (attribs, c, 8 * 4);
			sehle_vertex_buffer_unmap (vb);
			sehle_index_buffer_resize (ib, 36);
			uint32_t *indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
			memcpy (indices, p, 6 * 4);
			sehle_index_buffer_unmap (ib);
		}
		va = sehle_vertex_array_new_from_buffers (engine, (const unsigned char *) "SehleStdTextureRect", vb, ib);
		sehle_vertex_array_set_vertex_data (va, SEHLE_ATTRIBUTE_VERTEX, vb);
		sehle_vertex_array_set_vertex_data (va, SEHLE_ATTRIBUTE_TEXCOORD, vb);
		sehle_vertex_array_set_index_data (va, ib);
	} else if (type == SEHLE_GEOMETRY_UNIT_CUBE_OUTSIDE) {
		vb = sehle_engine_get_vertex_buffer (engine, "UnitCubeOutside", SEHLE_BUFFER_STATIC);
		ib = sehle_engine_get_index_buffer (engine, "UnitCubeOutside", SEHLE_BUFFER_STATIC);
		if (sehle_resource_get_sate (&vb->buffer.resource) == SEHLE_RESOURCE_STATE_CREATED) {
			sehle_vertex_buffer_setup_vnt (vb, 24);
			float *attribs = ( float *) sehle_vertex_buffer_map (vb, SEHLE_BUFFER_WRITE);
			sehle_index_buffer_resize (ib, 36);
			uint32_t *indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
			elea_generate_box (attribs, 32, attribs + 3, 32, attribs + 6, 32, indices, &p0, &p1, 1);
			sehle_index_buffer_unmap (ib);
			sehle_vertex_buffer_unmap (vb);
		}
		va = sehle_vertex_array_new (engine, (const unsigned char *) "SehleStdUnitCubeOutside");
		sehle_vertex_array_set_vertex_data (va, 0, vb);
		sehle_vertex_array_set_vertex_data (va, 1, vb);
		sehle_vertex_array_set_vertex_data (va, 2, vb);
		sehle_vertex_array_set_index_data (va, ib);
	} else if (type == SEHLE_GEOMETRY_UNIT_CUBE_INSIDE) {
		vb = sehle_engine_get_vertex_buffer (engine, "UnitCubeInside", SEHLE_BUFFER_STATIC);
		ib = sehle_engine_get_index_buffer (engine, "UnitCubeInside", SEHLE_BUFFER_STATIC);
		if (sehle_resource_get_sate (&vb->buffer.resource) == SEHLE_RESOURCE_STATE_CREATED) {
			sehle_vertex_buffer_setup_vnt (vb, 24);
			float *attribs = ( float *) sehle_vertex_buffer_map (vb, SEHLE_BUFFER_WRITE);
			sehle_index_buffer_resize (ib, 36);
			uint32_t *indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
			elea_generate_box (attribs, 32, attribs + 3, 32, attribs + 6, 32, indices, &p0, &p1, -1);
			sehle_index_buffer_unmap (ib);
			sehle_vertex_buffer_unmap (vb);
		}
		va = sehle_vertex_array_new (engine, (const unsigned char *) "SehleStdUnitCubeInside");
		sehle_vertex_array_set_vertex_data (va, 0, vb);
		sehle_vertex_array_set_vertex_data (va, 1, vb);
		sehle_vertex_array_set_vertex_data (va, 2, vb);
		sehle_vertex_array_set_index_data (va, ib);
	} else if (type == SEHLE_GEOMETRY_STREAM_256) {
		vb = sehle_engine_get_vertex_buffer (engine, NULL, SEHLE_BUFFER_STREAM);
		ib = sehle_engine_get_index_buffer (engine, NULL, SEHLE_BUFFER_STATIC);
		if (sehle_resource_get_sate (&vb->buffer.resource) == SEHLE_RESOURCE_STATE_CREATED) {
			unsigned int *indices, i, j;
			static const unsigned int p[] = { 0, 1, 2, 0, 2, 3 };
			sehle_vertex_buffer_setup_attrs (vb, 256, SEHLE_ATTRIBUTE_VERTEX, 2, SEHLE_ATTRIBUTE_TEXCOORD, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
			sehle_index_buffer_resize (ib, 384);
			indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
			for (i = 0; i < 64; i++) {
				for (j = 0; j < 6; j++) indices[6 * i + j] = 4 * i + p[j];
			}
			sehle_index_buffer_unmap (ib);
		}
		va = sehle_vertex_array_new (engine, NULL);
		sehle_vertex_array_set_vertex_data (va, 0, vb);
		sehle_vertex_array_set_vertex_data (va, 2, vb);
		sehle_vertex_array_set_vertex_data (va, SEHLE_ATTRIBUTE_COLOR, vb);
		sehle_vertex_array_set_index_data (va, ib);
	} else if (type == SEHLE_GEOMETRY_GRID_8x8) {
		float *attrs;
		unsigned int *indices, x, y;
		static const unsigned int p[] = { 0, 1, 2, 0, 2, 3 };
		va = sehle_vertex_array_new_from_attrs (engine, NULL, 81, 384, SEHLE_ATTRIBUTE_VERTEX, 3, SEHLE_ATTRIBUTE_TEXCOORD, 2, -1);
		attrs = (float *) sehle_vertex_buffer_map (va->vbufs[0], SEHLE_BUFFER_WRITE);
		for (y = 0; y <= 8; y++) {
			for (x = 0; x <= 8; x++) {
				attrs[2 * (9 * y + x)] = x / 8.0f;
				attrs[2 * (9 * y + x) + 1] = y / 8.0f;
			}
		}
		sehle_vertex_buffer_unmap (va->vbufs[0]);
		indices = sehle_index_buffer_map (va->ibuf, SEHLE_BUFFER_WRITE);
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				indices[6 * (8 * y + x)] = 9 * y + x;
				indices[6 * (8 * y + x) + 1] = 9 * y + x + 1;
				indices[6 * (8 * y + x) + 2] = 9 * (y + 1) + x;
				indices[6 * (8 * y + x) + 3] = 9 * y + x;
				indices[6 * (8 * y + x) + 4] = 9 * (y + 1) + x;
				indices[6 * (8 * y + x) + 5] = 9 * (y + 1) + x + 1;
			}
		}
		sehle_index_buffer_unmap (va->ibuf);
	}
	return va;
}

