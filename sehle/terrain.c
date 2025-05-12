#define __SEHLE_TERRAIN_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2022
 */

#include <az/class.h>

#include <sehle/index-buffer.h>
#include <sehle/material.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include "terrain.h"

static void terrain_class_init (SehleTerrainClass *klass);
static void terrain_init (SehleTerrainClass *klass, SehleTerrain *terrain);
static void terrain_finalize (SehleTerrainClass *klass, SehleTerrain *terrain);
/* Renderable implementation */
static void terrain_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
static void terrain_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);

unsigned int sehle_terrain_type = 0;
SehleTerrainClass *sehle_terrain_class;

unsigned int
sehle_terrain_get_type (void)
{
	if (!sehle_terrain_type) {
		az_register_type (&sehle_terrain_type, (const unsigned char *) "SehleTerrain", AZ_TYPE_BLOCK, sizeof (SehleTerrainClass), sizeof (SehleTerrain), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) terrain_class_init,
			(void (*) (const AZImplementation *, void *)) terrain_init,
			(void (*) (const AZImplementation *, void *)) terrain_finalize);
		sehle_terrain_class = (SehleTerrainClass *) az_type_get_class (sehle_terrain_type);
	}
	return sehle_terrain_type;
}

static void
terrain_class_init (SehleTerrainClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE,
		ARIKKEI_OFFSET(SehleTerrainClass, renderable_impl),
		ARIKKEI_OFFSET(SehleTerrain, renderable_inst));
	klass->renderable_impl.display = terrain_display;
	klass->renderable_impl.render = terrain_render;
}

static void
terrain_init (SehleTerrainClass *klass, SehleTerrain *terrain)
{
}

static void
terrain_finalize (SehleTerrainClass *klass, SehleTerrain *terrain)
{
	if (terrain->va) {
		az_object_unref ((AZObject *) terrain->va);
	}
}

#define N_DIVS 16
#define N_VERTS (N_DIVS + 1)

void
sehle_terrain_setup (SehleTerrain *terrain, SehleEngine *engine, unsigned int layer_mask)
{
	unsigned int x, y;
	float *attrs;
	uint32_t *indices;
	terrain->renderable_inst.layer_mask = layer_mask;
	terrain->va = sehle_vertex_array_new_from_attrs (engine, (const unsigned char *) "Sehle::Terrain::Vertices", N_VERTS * N_VERTS, N_DIVS * N_DIVS * 6, SEHLE_ATTRIBUTE_VERTEX, 4, -1);
	attrs = sehle_vertex_buffer_map (terrain->va->vbufs[SEHLE_ATTRIBUTE_VERTEX], SEHLE_BUFFER_WRITE);
	for (y = 0; y < N_VERTS; y++) {
		for (x = 0; x < N_VERTS; x++) {
			attrs[(y * N_VERTS + x) * 4] = (float) x / N_DIVS;
			attrs[(y * N_VERTS + x) * 4 + 1] = (float) y / N_DIVS;
		}
	}
	sehle_vertex_buffer_unmap (terrain->va->vbufs[SEHLE_ATTRIBUTE_VERTEX]);
	indices = sehle_index_buffer_map (terrain->va->ibuf, SEHLE_BUFFER_WRITE);
	for (y = 0; y < N_DIVS; y++) {
		for (x = 0; x < N_DIVS; x++) {
			indices[(y * N_DIVS + x) * 6] = y * N_VERTS + x;
			indices[(y * N_DIVS + x) * 6 + 1] = y * N_VERTS + x + 1;
			indices[(y * N_DIVS + x) * 6 + 2] = (y + 1) * N_VERTS + x + 1;
			indices[(y * N_DIVS + x) * 6 + 3] = (y + 1) * N_VERTS + x + 1;
			indices[(y * N_DIVS + x) * 6 + 4] = (y + 1) * N_VERTS + x;
			indices[(y * N_DIVS + x) * 6 + 5] = y * N_VERTS + x;
		}
	}
	sehle_index_buffer_unmap (terrain->va->ibuf);
}

void
sehle_terrain_release (SehleTerrain *terrain)
{
}

static void
terrain_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{

}

static void
terrain_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{

}

#if 0
namespace Sehle {

Terrain::Terrain (Engine *engine, unsigned int contextmask, unsigned int levels, float pchunksize)
: SehleRenderable(engine, contextmask),
vbuffer(NULL), ibuffer(NULL), material(NULL), root(NULL)
{
	static const unsigned int cpc = 1 << chunklevel;
	nrows = 1 << levels;
	ncols = 1 << levels;
	chunksize = pchunksize;
	cellsize = chunksize / cpc;

	nztextures = 1;
	ztextures = (Texture2D **) malloc (1 * sizeof (Texture2D *));
	ztextures[0] = engine->getBasicTexture (Engine::TEXTURE_NOISE_16X16);

	root = addNode (0, 0, 0, nrows, ncols);

	// Generate buffers
	unsigned int nvertices = 0;
	unsigned int nindices = 0;
	unsigned int div = 1 << chunklevel;
	while (div) {
		nvertices += (div + 1) * (div + 1);
		nindices += 6 * div * div;
		div = div >> 1;
	}
	vbuffer = engine->getVertexBuffer ("Terrain");
	vbuffer->setUp (nvertices, 2);
	f32 *attribs = vbuffer->map (VertexBuffer::STATE_WRITE);
	ibuffer = engine->getIndexBuffer ("Terrain");
	ibuffer->resize (nindices);
	u32 *indices = ibuffer->map (IndexBuffer::WRITE);
	unsigned int vidx = 0;
	unsigned int iidx = 0;
	div = 1 << chunklevel;
	for (unsigned int lod = 0; lod <= chunklevel; lod++) {
		firstindices[lod] = iidx;
		this->nindices[lod] = 6 * div * div;
		for (unsigned int row = 0; row <= div; row++) {
			for (unsigned int col = 0; col <= div; col++) {
				vbuffer->setValues (vidx + row * (div + 1) + col, 0, (float) col / div, (float) row / div);
			}
		}
		for (unsigned int row = 0; row < div; row++) {
			for (unsigned int col = 0; col < div; col++) {
				unsigned int v = vidx + row * (div + 1) + col;
				indices[iidx++] = v;
				indices[iidx++] = v + 1;
				indices[iidx++] = v + (div + 1) + 1;
				indices[iidx++] = v;
				indices[iidx++] = v + (div + 1) + 1;
				indices[iidx++] = v + (div + 1);
			}
		}
		vidx += (div + 1) * (div + 1);
		div = div >> 1;
	}
	vbuffer->unMap ();
	ibuffer->unMap ();

	material = MaterialTerrain::newMaterialTerrain(engine, "test");
}

Terrain::~Terrain (void)
{
	if (material) material->unRef ();
	if (ibuffer) ibuffer->unRef ();
	if (vbuffer) vbuffer->unRef ();
	// Free Z textures
	for (unsigned int i = 0; i < nztextures; i++) {
		ztextures[i]->unRef ();
	}
	free (ztextures);
}

void
Terrain::display (RenderContext *ctx, DisplayContext *displaycontext, Node *node)
{
	if (node->bbox.testPosition (ctx->viewSpace) == Elea::ALL_OUT) return;
	// fixme: Do somethng with r2w (Lauris)
	float dist = node->bbox.distance (ctx->v2w.getTranslation ());
	if (dist < 1) dist = 1;
	// int lod = (int) sqrt (dist / 100);
	int level = (int) maxlevel - (int) (log (dist) - 1);
	if (level < 0) level = 0;
	if (level > (int) maxlevel) level = maxlevel;
	if ((level <= (int) node->level) || (node->leaf)) {
		// Display this node
		node->batch.terrain = this;
		node->batch.ztex = 0;
		node->batch.size = chunksize * (node->area[2] - node->area[0]);
		node->batch.positions = &node->positions;
		node->batch.biases = &node->biases;
		node->batch.ztexpos = &node->ztexpos;
		node->positions.set (node->area[0] * chunksize, node->area[1] * chunksize);
		node->biases.set (64, 64, 64, 64);
		ctx->scheduleRenderInstanced (vbuffer, vbuffer, ibuffer, firstindices[0], nindices[0], material, 1, &node->batch, STAGE_SOLID);
	} else {
		for (int i = 0; i < 4; i++) {
			display (ctx, displaycontext, node->children[i]);
		}
	}
}

void
Terrain::display (RenderContext *ctx, DisplayContext *displaycontext)
{
	display (ctx, displaycontext, root);
}

Terrain::Node *
Terrain::addNode (unsigned int level, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	static const unsigned int cpc = 1 << chunklevel;
	if ((x0 >= x1) || (y0 >= y1)) return NULL;
	Node *node = (Node *) malloc (sizeof (Node));
	node->level = level;
	node->area[0] = x0;
	node->area[1] = y0;
	node->area[2] = x1;
	node->area[3] = y1;
	node->bbox.set (x0 * chunksize, y0 * chunksize, 0, x1 * chunksize, y1 * chunksize, 0.1f);
	if (((x1 - x0) > 1) || ((y1 - y0) > 1)) {
		node->leaf = false;
		unsigned int xmid = (x0 + x1) / 2;
		unsigned int ymid = (y0 + y1) / 2;
		node->children[0] = addNode (level + 1, x0, y0, xmid, ymid);
		node->children[1] = addNode (level + 1, xmid, y0, x1, ymid);
		node->children[2] = addNode (level + 1, x0, ymid, xmid, y1);
		node->children[3] = addNode (level + 1, xmid, ymid, x1, y1);
	} else {
		node->leaf = true;
		for (unsigned int i = 0; i < 4; i++) node->children[i] = NULL;
	}
	node->ztex = 0;
	node->ctex = 0;
	if (level < 4) {
		node->ztexpos.set ((float) node->area[0] / ncols, (float) node->area[1] / nrows, (float) node->area[2] / ncols, (float) node->area[3] / nrows);
	} else {
		node->ztexpos.set ((float) (node->area[0] & 0x1f) * 16 / ncols, (float) (node->area[1] & 0x1f) * 16 / nrows, (float) (node->area[2] & 0x1f) * 16 / ncols, (float) (node->area[3] & 0x1f) * 16 / nrows);
	}
	return node;
}

void
Terrain::deleteNode (Node *node)
{
	for (unsigned int i = 0; i < 4; i++) if (node->children[i]) deleteNode (node->children[i]);
	free (node);
}

void
Terrain::setNZTextures (unsigned int pnztextures)
{
	if (pnztextures < 1) pnztextures = 1;
	for (unsigned int i = pnztextures; i < nztextures; i++) ztextures[i]->unRef ();
	ztextures = (Texture2D **) malloc (pnztextures * sizeof (Texture2D *));
	for (unsigned int i = nztextures; i < pnztextures; i++) ztextures[i] = engine->getBasicTexture (Engine::TEXTURE_NOISE_16X16);
	nztextures = pnztextures;
}

void
Terrain::setZTexture (unsigned int idx, Texture2D *texture)
{
	if (idx >= nztextures) return;
	if (!texture) return;
	ztextures[idx]->unRef ();
	ztextures[idx] = texture;
}

} // Namespace Sehle
#endif
