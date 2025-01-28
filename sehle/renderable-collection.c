#define __SEHLE_RENDERABLE_COLLECTION_C__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2015
*/

#include <sehle/renderable-collection.h>

unsigned int collection_type = 0;
static SehleRenderableCollectionClass *collection_class;

unsigned int
sehle_renderable_collection_get_type (void)
{
	if (!collection_type) {
		collection_class = (SehleRenderableCollectionClass *) az_register_interface_type (&collection_type, (const unsigned char *) "SehleRenderableCollection", SEHLE_TYPE_RENDERABLE,
			sizeof (SehleRenderableClass), sizeof (SehleRenderableCollectionImplementation), sizeof (SehleRenderableCollectionInstance), 0,
			NULL, NULL, NULL, NULL);
	}
	return collection_type;
}

void
sehle_renderable_collection_add_child (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	if (collection_impl->add_child) {
		collection_impl->add_child (collection_impl, collection_inst, impl, inst);
	}
}

void
sehle_renderable_collection_remove_child (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	if (collection_impl->remove_child) {
		collection_impl->remove_child (collection_impl, collection_inst, impl, inst);
	}
}

void
sehle_renderable_collection_update_visuals (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst)
{
	if (collection_impl->update_visuals) {
		collection_impl->update_visuals (collection_impl, collection_inst);
	}
}
