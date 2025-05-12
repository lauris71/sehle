#define __SEHLE_RENDERABLE_LIST_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

static const int debug = 1;

#include <az/class.h>

#include <stdlib.h>
#include <stdio.h>

#include <arikkei/arikkei-utils.h>
#include <az/types.h>

#include <sehle/renderable-list.h>

void renderable_list_class_init (SehleRenderableListClass *klass);
void renderable_list_finalize (SehleRenderableListClass *klass, SehleRenderableList *list);

/* SehleRenderable implementation */
static void renderable_list_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
/* SehleRenderableCollection implementation */
static void renderable_list_add_child (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
static void renderable_list_remove_child (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
static void renderable_list_update_visuals (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst);

static unsigned int group_type = 0;
SehleRenderableListClass *sehle_renderable_list_class = NULL;

unsigned int
sehle_renderable_list_get_type (void)
{
	if (!group_type) {
		az_register_type (&group_type, (const unsigned char *) "SehleRenderableList", AZ_TYPE_STRUCT, sizeof (SehleRenderableListClass), sizeof (SehleRenderableList), AZ_FLAG_ZERO_MEMORY,
						(void (*) (AZClass *)) renderable_list_class_init,
						NULL,
						(void (*) (const AZImplementation *, void *)) renderable_list_finalize);
		sehle_renderable_list_class = (SehleRenderableListClass *) az_type_get_class (group_type);
	}
	return group_type;
}

void
renderable_list_class_init (SehleRenderableListClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE_COLLECTION,
		ARIKKEI_OFFSET (SehleRenderableListClass, collection_impl),
		ARIKKEI_OFFSET (SehleRenderableList, collection_inst));
	klass->collection_impl.add_child = renderable_list_add_child;
	klass->collection_impl.remove_child = renderable_list_remove_child;
	klass->collection_impl.update_visuals = renderable_list_update_visuals;
	klass->collection_impl.renderable_impl.display = renderable_list_display;
}

void
renderable_list_finalize (SehleRenderableListClass *klass, SehleRenderableList *list)
{
	if (debug) {
		if (list->n_children) {
			fprintf (stderr, "renderable_list_finalize: destroying list with children, possible memory leak\n");
		}
	}
}

static void
renderable_list_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleRenderableList *list = SEHLE_RENDERABLE_LIST_FROM_RENDERABLE_INSTANCE (inst);
	for (unsigned int i = 0; i < list->n_children; i++) {
		sehle_renderable_display (list->children[i].impl, list->children[i].inst, ctx, displayctx);
	}
}

static void
renderable_list_add_child (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	SehleRenderableList *list = SEHLE_RENDERABLE_LIST_FROM_COLLECTION_INSTANCE (coll_inst);
	if (list->n_children == list->children_size) {
		if (list->children_size == 0) {
			list->children_size = 16;
			list->children = (SehleRenderableHandle *) malloc (list->children_size * sizeof (SehleRenderableHandle));
		} else {
			list->children_size = list->children_size * 2;
			list->children = (SehleRenderableHandle *) realloc (list->children, list->children_size * sizeof (SehleRenderableHandle));
		}
	}
	list->children[list->n_children].impl = impl;
	list->children[list->n_children].inst = inst;
	list->n_children += 1;
}

static void
renderable_list_remove_child (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	SehleRenderableList *list = SEHLE_RENDERABLE_LIST_FROM_COLLECTION_INSTANCE (coll_inst);
	for (unsigned int i = 0; i < list->n_children; i++) {
		if ((list->children[i].impl == impl) && (list->children[i].inst == inst)) {
			list->n_children -= 1;
			if (i < list->n_children) {
				list->children[i] = list->children[list->n_children];
			}
			return;
		}
	}
	fprintf (stderr, "renderable_list_remove_child: Trying to remove renderable not in list\n");
}

static void
renderable_list_update_visuals (SehleRenderableCollectionImplementation *coll_impl, SehleRenderableCollectionInstance *coll_inst)
{
	SehleRenderableList *list = SEHLE_RENDERABLE_LIST_FROM_COLLECTION_INSTANCE (coll_inst);
	coll_inst->renderable_inst.layer_mask = 0;
	coll_inst->renderable_inst.bbox = EleaAABox3fEmpty;
	for (unsigned int i = 0; i < list->n_children; i++) {
		coll_inst->renderable_inst.layer_mask |= list->children[i].inst->layer_mask;
		elea_aabox3f_union (&coll_inst->renderable_inst.bbox, &coll_inst->renderable_inst.bbox, &list->children[i].inst->bbox);
	}
}

SehleRenderableList *
sehle_renderable_list_new (SehleEngine *engine, unsigned int layer_mask)
{
	SehleRenderableList *list = (SehleRenderableList *) az_instance_new (SEHLE_TYPE_RENDERABLE_LIST);
	list->collection_inst.renderable_inst.layer_mask = layer_mask;
	return list;
}

void
sehle_renderable_list_delete (SehleRenderableList *list)
{
	az_instance_delete (SEHLE_TYPE_RENDERABLE_LIST, list);
}

void
sehle_renderable_list_setup (SehleRenderableList *list, SehleEngine *engine, unsigned int layer_mask)
{
	az_instance_init (list, SEHLE_TYPE_RENDERABLE_LIST);
	list->collection_inst.renderable_inst.layer_mask = layer_mask;
}

void
sehle_renderable_list_release (SehleRenderableList *list)
{
	az_instance_finalize (list, SEHLE_TYPE_RENDERABLE_LIST);
}

void
sehle_renderable_list_invoke_display (SehleRenderableList *list, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	sehle_renderable_display (&sehle_renderable_list_class->collection_impl.renderable_impl, &list->collection_inst.renderable_inst, ctx, displayctx);
}

void
sehle_renderable_list_add_child (SehleRenderableList *list, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	arikkei_return_if_fail (list != NULL);
	arikkei_return_if_fail (impl != NULL);
	arikkei_return_if_fail (inst != NULL);
	sehle_renderable_collection_add_child (&sehle_renderable_list_class->collection_impl, &list->collection_inst, impl, inst);
}

void
sehle_renderable_list_remove_child (SehleRenderableList *list, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	arikkei_return_if_fail (list != NULL);
	arikkei_return_if_fail (impl != NULL);
	arikkei_return_if_fail (inst != NULL);
	sehle_renderable_collection_remove_child (&sehle_renderable_list_class->collection_impl, &list->collection_inst, impl, inst);
}

void
sehle_renderable_list_update_visuals (SehleRenderableList *list)
{
	arikkei_return_if_fail (list != NULL);
	sehle_renderable_collection_update_visuals (&sehle_renderable_list_class->collection_impl, &list->collection_inst);
}

void
sehle_renderable_list_clear (SehleRenderableList *list)
{
	arikkei_return_if_fail (list != NULL);
	list->n_children = 0;
	list->collection_inst.renderable_inst.bbox = EleaAABox3fEmpty;
}
