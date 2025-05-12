#define __SEHLE_RESOURCE_C__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2018
*/

static const int debug = 0;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sehle/engine.h>
#include <sehle/resource.h>

static void resource_class_init (SehleResourceClass *klass);
/* AZObject implementation */
static void resource_shutdown (AZObject *obj);

AZActiveObjectClass *parent_class;

unsigned int
sehle_resource_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		AZClass *klass = az_register_type (&type, (const unsigned char *) "SehleResource", AZ_TYPE_ACTIVE_OBJECT, sizeof (SehleResourceClass), sizeof (SehleResource), AZ_FLAG_ABSTRACT,
			(void (*) (AZClass *)) resource_class_init,
			NULL, NULL);
		parent_class = (AZActiveObjectClass *) az_class_parent(klass);
	}
	return type;
}

static void
resource_class_init (SehleResourceClass *klass)
{
	((AZObjectClass *) klass)->shutdown = resource_shutdown;
}

static void
resource_shutdown (AZObject *obj)
{
	SehleResource *res = (SehleResource *) obj;
	if (res->gl_handle) {
		fprintf (stderr, "resource_shutdown: %s %s OpenGL object not released\n", res->id, ((AZObject *) res)->klass->reference_klass.klass.name);
	}
	if (res->id) {
		sehle_engine_remove_resource (res->engine, res);
		free (res->id);
		res->id = NULL;
	}
	res->engine = NULL;
	parent_class->object_class.shutdown (obj);
}

unsigned int
sehle_resource_get_handle (SehleResource *res)
{
	arikkei_return_val_if_fail (res != NULL, 0);
	arikkei_return_val_if_fail (SEHLE_IS_RESOURCE (res), 0);
	arikkei_return_val_if_fail (sehle_resource_get_sate(res) != SEHLE_RESOURCE_STATE_CREATED, 0);
	arikkei_return_val_if_fail (sehle_resource_get_sate (res) != SEHLE_RESOURCE_STATE_INVALID, 0);
	if (sehle_resource_get_sate (res) == SEHLE_RESOURCE_STATE_MODIFIED) sehle_resource_build (res);
	return res->gl_handle;
}

void
sehle_resource_setup (SehleResource *res, SehleEngine *engine, const unsigned char *id)
{
	arikkei_return_if_fail (!sehle_resource_is_initialized (res));
	res->engine = engine;
	if (id) {
		res->id = (unsigned char *) strdup ((const char *) id);
		sehle_engine_add_resource (engine, res, 0);
	}
}

void
sehle_resource_build (SehleResource *res)
{
	arikkei_return_if_fail (res->engine->running);
	arikkei_return_if_fail (sehle_resource_is_modified (res));
	arikkei_return_if_fail (!res->gl_handle || (sehle_resource_get_sate (res) == SEHLE_RESOURCE_STATE_MODIFIED));
	if (((SehleResourceClass *) ((AZObject *) res)->klass)->build) {
		((SehleResourceClass *) ((AZObject *) res)->klass)->build (res);
	}
}
