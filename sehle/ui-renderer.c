#define __SEHLE_UI_RENDERER_CPP__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2018
*
*/

#define UI_RENDERER_BUFFER_SIZE 256

#include <az/extend.h>

#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/program-overlay.h>
#include <sehle/program.h>
#include <sehle/render-target.h>
#include <sehle/texture-2d.h>
#include <sehle/vertex-buffer.h>

#include <sehle/ui-renderer.h>

static void ui_renderer_class_init (SehleUIRendererClass *klass);
static void ui_renderer_init (SehleUIRendererClass *klass, SehleUIRenderer *rend);
/* AZObject implementation */
static void ui_renderer_shutdown (AZObject *object);

static void sehle_ui_renderer_draw (SehleUIRenderer *rend);

unsigned int
sehle_ui_renderer_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleUIRenderer", AZ_TYPE_OBJECT, sizeof (SehleUIRendererClass), sizeof (SehleUIRenderer), 0,
			(void (*) (AZClass *)) ui_renderer_class_init,
			(void (*) (const AZImplementation *, void *)) ui_renderer_init,
			NULL);
	}
	return type;
}

static void
ui_renderer_class_init (SehleUIRendererClass *klass)
{
	klass->object_class.shutdown = ui_renderer_shutdown;
}

static void
ui_renderer_init (SehleUIRendererClass *klass, SehleUIRenderer *rend)
{
	rend->render_flags = SEHLE_UI_RENDERER_DEFAULT_FLAGS;
}

static void
ui_renderer_shutdown (AZObject *obj)
{
	SehleUIRenderer *rend = (SehleUIRenderer *) obj;
	rend->n_rects = 0;
	if (rend->attribs) {
		sehle_vertex_buffer_unmap (rend->va[0]->vbufs[0]);
		rend->attribs = NULL;
	}
	az_object_unref (AZ_OBJECT (rend->va[0]));
	rend->va[0] = NULL;
	az_object_unref (AZ_OBJECT (rend->va[1]));
	rend->va[1] = NULL;
	if (rend->prog) {
		az_object_unref ((AZObject *) rend->prog);
		rend->prog = NULL;
	}
	if (rend->tex) {
		az_object_unref ((AZObject *) rend->tex);
		rend->tex = NULL;
	}
	az_object_unref (AZ_OBJECT (rend->target));
	rend->target = NULL;
}

SehleUIRenderer *
sehle_ui_renderer_new (SehleRenderTarget *target)
{
	SehleUIRenderer *rend;
	unsigned int *indices, i, j;
	static const unsigned int p[] = { 0, 1, 2, 0, 2, 3 };

	arikkei_return_val_if_fail (target != NULL, NULL);
	arikkei_return_val_if_fail (SEHLE_IS_RENDER_TARGET (target), NULL);
	rend = (SehleUIRenderer *) az_object_new (SEHLE_TYPE_UI_RENDERER);
	rend->engine = target->resource.engine;

	rend->target = target;
	az_object_ref (AZ_OBJECT (rend->target));
	rend->vport.x0 = 0;
	rend->vport.y0 = 0;
	rend->vport.x1 = target->width;
	rend->vport.y1 = target->height;

	SehleIndexBuffer *ibuf = sehle_engine_get_index_buffer (rend->engine, NULL, SEHLE_BUFFER_STATIC);
	sehle_index_buffer_resize (ibuf, UI_RENDERER_BUFFER_SIZE * 6);
	indices = sehle_index_buffer_map (ibuf, SEHLE_BUFFER_WRITE);
	for (i = 0; i < UI_RENDERER_BUFFER_SIZE; i++) {
		for (j = 0; j < 6; j++) indices[6 * i + j] = 4 * i + p[j];
	}
	sehle_index_buffer_unmap (ibuf);
	for (i = 0; i < 2; i++) {
		SehleVertexBuffer *vbuf = sehle_engine_get_vertex_buffer (rend->engine, NULL, SEHLE_BUFFER_STREAM);
		sehle_vertex_buffer_setup_attrs (vbuf, UI_RENDERER_BUFFER_SIZE * 4, SEHLE_ATTRIBUTE_VERTEX, 2, SEHLE_ATTRIBUTE_TEXCOORD, 2, SEHLE_ATTRIBUTE_COLOR, 4, -1);
		rend->va[i] = sehle_vertex_array_new (rend->engine, 0);
		sehle_vertex_array_set_vertex_data (rend->va[i], SEHLE_ATTRIBUTE_VERTEX, vbuf);
		sehle_vertex_array_set_vertex_data (rend->va[i], SEHLE_ATTRIBUTE_TEXCOORD, vbuf);
		sehle_vertex_array_set_vertex_data (rend->va[i], SEHLE_ATTRIBUTE_COLOR, vbuf);
		sehle_vertex_array_set_index_data (rend->va[i], ibuf);
		az_object_unref ((AZObject *) vbuf);
	}
	az_object_unref ((AZObject *) ibuf);
	return rend;
}

void
sehle_ui_renderer_set_viewport (SehleUIRenderer *rend, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_UI_RENDERER (rend));
	rend->vport.x0 = x0;
	rend->vport.y0 = y0;
	rend->vport.x1 = x1;
	rend->vport.y1 = y1;
}

void
sehle_ui_renderer_begin (SehleUIRenderer *rend, SehleProgram *prog, unsigned int render_flags)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_UI_RENDERER (rend));
	arikkei_return_if_fail (prog != NULL);
	arikkei_return_if_fail (SEHLE_IS_PROGRAM (prog));
	if ((prog != rend->prog) || (render_flags != rend->render_flags)) {
		/* Draw leaves buffer unmapped */
		if (rend->n_rects) sehle_ui_renderer_draw (rend);
		if (rend->prog) az_object_unref ((AZObject *) rend->prog);
		rend->prog = prog;
		if (rend->prog) az_object_ref ((AZObject *) rend->prog);
		rend->render_flags = render_flags;
	}
	sehle_engine_set_render_target (rend->engine, rend->target);
	sehle_engine_set_viewport (rend->engine, &rend->vport);
	sehle_engine_set_program (rend->engine, rend->prog);
	sehle_engine_set_render_flags (rend->engine, rend->render_flags);
	if (!rend->attribs) {
		rend->attribs = sehle_vertex_buffer_map (rend->va[0]->vbufs[0], SEHLE_BUFFER_WRITE);
	}
	//rend->n_rects = 0;
}

void
sehle_ui_renderer_set_texture (SehleUIRenderer *rend, SehleTexture2D *tex)
{
	if (tex != rend->tex) {
		if (rend->n_rects) sehle_ui_renderer_draw (rend);
		if (rend->tex) az_object_unref ((AZObject *) rend->tex);
		rend->tex = tex;
		if (rend->tex) az_object_ref ((AZObject *) rend->tex);
		if (!rend->attribs) {
			rend->attribs = sehle_vertex_buffer_map (rend->va[0]->vbufs[0], SEHLE_BUFFER_WRITE);
		}
	}
}

void
sehle_ui_renderer_finish (SehleUIRenderer *rend)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_UI_RENDERER (rend));
	if (rend->n_rects) {
		/* Draw leaves buffer unmapped */
		sehle_ui_renderer_draw (rend);
	} else if (rend->attribs) {
		sehle_vertex_buffer_unmap (rend->va[0]->vbufs[0]);
		rend->attribs = NULL;
	}
	rend->n_rects = 0;
}

void
sehle_ui_renderer_add_rect (SehleUIRenderer *rend, const NRRectf *dst, const NRRectf *src, unsigned int src_width, unsigned int src_height, const EleaColor4f *colors)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_UI_RENDERER (rend));
	arikkei_return_if_fail (dst != NULL);

	if (rend->n_rects >= UI_RENDERER_BUFFER_SIZE) {
		/* Draw leaves buffer unmapped */
		sehle_ui_renderer_draw (rend);
		rend->attribs = sehle_vertex_buffer_map (rend->va[0]->vbufs[0], SEHLE_BUFFER_WRITE);
	}

	float dxscale = 2.0f / (rend->vport.x1 - rend->vport.x0);
	float dyscale = 2.0f / (rend->vport.y1 - rend->vport.y0);
	rend->attribs[(rend->n_rects * 4 + 0) * 8 + 0] = dst->x0 * dxscale - 1;
	rend->attribs[(rend->n_rects * 4 + 0) * 8 + 1] = dst->y0 * dyscale - 1;
	rend->attribs[(rend->n_rects * 4 + 1) * 8 + 0] = dst->x1 * dxscale - 1;
	rend->attribs[(rend->n_rects * 4 + 1) * 8 + 1] = dst->y0 * dyscale - 1;
	rend->attribs[(rend->n_rects * 4 + 2) * 8 + 0] = dst->x1 * dxscale - 1;
	rend->attribs[(rend->n_rects * 4 + 2) * 8 + 1] = dst->y1 * dyscale - 1;
	rend->attribs[(rend->n_rects * 4 + 3) * 8 + 0] = dst->x0 * dxscale - 1;
	rend->attribs[(rend->n_rects * 4 + 3) * 8 + 1] = dst->y1 * dyscale - 1;

	if (src) {
		float sxscale = 1.0f / src_width;
		float syscale = 1.0f / src_height;
		float dx = (src->x1 >= src->x0) ? 0.5f : -0.5f;
		float dy = (src->y1 >= src->y0) ? 0.5f : -0.5f;
		rend->attribs[(rend->n_rects * 4 + 0) * 8 + 2 + 0] = (src->x0 + dx) * sxscale;
		rend->attribs[(rend->n_rects * 4 + 0) * 8 + 2 + 1] = (src->y0 + dy) * syscale;
		rend->attribs[(rend->n_rects * 4 + 1) * 8 + 2 + 0] = (src->x1 - dx) * sxscale;
		rend->attribs[(rend->n_rects * 4 + 1) * 8 + 2 + 1] = (src->y0 + dy) * syscale;
		rend->attribs[(rend->n_rects * 4 + 2) * 8 + 2 + 0] = (src->x1 - dx) * sxscale;
		rend->attribs[(rend->n_rects * 4 + 2) * 8 + 2 + 1] = (src->y1 - dy) * syscale;
		rend->attribs[(rend->n_rects * 4 + 3) * 8 + 2 + 0] = (src->x0 + dx) * sxscale;
		rend->attribs[(rend->n_rects * 4 + 3) * 8 + 2 + 1] = (src->y1 - dy) * syscale;
	}

	if (colors) {
		memcpy (&rend->attribs[(rend->n_rects * 4 + 0) * 8 + 4 + 0], colors[0].c, sizeof (EleaColor4f));
		memcpy (&rend->attribs[(rend->n_rects * 4 + 1) * 8 + 4 + 0], colors[1].c, sizeof (EleaColor4f));
		memcpy (&rend->attribs[(rend->n_rects * 4 + 2) * 8 + 4 + 0], colors[2].c, sizeof (EleaColor4f));
		memcpy (&rend->attribs[(rend->n_rects * 4 + 3) * 8 + 4 + 0], colors[3].c, sizeof (EleaColor4f));
	}

	rend->n_rects += 1;
}

static void
sehle_ui_renderer_draw (SehleUIRenderer *rend)
{
	sehle_vertex_buffer_unmap (rend->va[0]->vbufs[0]);
	rend->attribs = NULL;
	SehleVertexArray *t = rend->va[0];
	rend->va[0] = rend->va[1];
	rend->va[1] = t;
	if (rend->tex) {
		sehle_engine_set_texture (rend->engine, 0, &rend->tex->texture);
	}
	sehle_vertex_array_render_triangles (rend->va[1], 1, 0, rend->n_rects * 6);
	rend->n_rects = 0;
}

