#define __SEHLE_RENDERABLE_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

#include <arikkei/arikkei-utils.h>

#include <sehle/render-context.h>

#include "renderable.h"

static void renderable_class_init (SehleRenderableClass *klass);
static void renderable_instance_init (SehleRenderableImplementation *impl, SehleRenderable *renderable);

unsigned int
sehle_renderable_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_interface_type (&type, (const unsigned char *) "SehleRenderable", AZ_TYPE_INTERFACE,
			sizeof (SehleRenderableClass), sizeof (SehleRenderableImplementation), sizeof (SehleRenderable), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) renderable_class_init,
			NULL,
			(void (*) (const AZImplementation *, void *)) renderable_instance_init,
			NULL);
	}
	return type;
}

static void
renderable_class_init (SehleRenderableClass *klass)
{
}

static void
renderable_instance_init (SehleRenderableImplementation *impl, SehleRenderable *renderable)
{
	renderable->render_stages = SEHLE_RENDER_STAGES_ALL;
	renderable->layer_mask = 0xffffffff;
	renderable->bbox = EleaAABox3fEmpty;
}
