#ifndef __SEHLE_RENDERABLE_LIST_H__
#define __SEHLE_RENDERABLE_LIST_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

//
// Sehle::RenderableGroup
//
// A very simple implementation of child type agnostic renderable list object
//

typedef struct _SehleRenderableList SehleRenderableList;
typedef struct _SehleRenderableListClass SehleRenderableListClass;

#define SEHLE_TYPE_RENDERABLE_LIST sehle_renderable_list_get_type ()

#define SEHLE_RENDERABLE_LIST_FROM_RENDERABLE_INSTANCE(i) ((SehleRenderableList *) (i))
#define SEHLE_RENDERABLE_LIST_RENDERABLE_IMPLEMENTATION (&sehle_renderable_list_class->collection_impl.renderable_impl)
#define SEHLE_RENDERABLE_LIST_FROM_COLLECTION_INSTANCE(i) ((SehleRenderableList *) (i))
#define SEHLE_RENDERABLE_LIST_COLLECTION_IMPLEMENTATION (&sehle_renderable_list_class->collection_impl)

#include <sehle/renderable-collection.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderableList {
	SehleRenderableCollectionInstance collection_inst;

	unsigned int children_size;
	unsigned int n_children;
	SehleRenderableHandle *children;
};

struct _SehleRenderableListClass {
	AZClass klass;
	SehleRenderableCollectionImplementation collection_impl;
};

unsigned int sehle_renderable_list_get_type (void);

#ifndef __SEHLE_RENDERABLE_LIST_CPP__
extern SehleRenderableListClass *sehle_renderable_list_class;
#endif

SehleRenderableList *sehle_renderable_list_new (SehleEngine *engine, unsigned int layer_mask);
void sehle_renderable_list_delete (SehleRenderableList *list);
void sehle_renderable_list_setup (SehleRenderableList *list, SehleEngine *engine, unsigned int layer_mask);
void sehle_renderable_list_release (SehleRenderableList *list);

void sehle_renderable_list_invoke_display (SehleRenderableList *list, SehleRenderContext *ctx, SehleDisplayContext *displayctx);

void sehle_renderable_list_add_child (SehleRenderableList *list, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
void sehle_renderable_list_remove_child (SehleRenderableList *list, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);

/* Set list bbox to the union of child boxes and mask to the bitwise OR-ed mask */
void sehle_renderable_list_update_visuals (SehleRenderableList *list);
/* Erases all children and resets bbox */
void sehle_renderable_list_clear (SehleRenderableList *list);

#ifdef __cplusplus
};
#endif

#endif

