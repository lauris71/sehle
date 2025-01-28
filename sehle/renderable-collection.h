#ifndef __SEHLE_RENDERABLE_COLLECTION_H__
#define __SEHLE_RENDERABLE_COLLECTION_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2015
 */

#define SEHLE_TYPE_RENDERABLE_COLLECTION sehle_renderable_collection_get_type ()
typedef struct _SehleRenderableCollectionInstance SehleRenderableCollectionInstance;
typedef struct _SehleRenderableCollectionImplementation SehleRenderableCollectionImplementation;
typedef struct _SehleRenderableCollectionClass SehleRenderableCollectionClass;

#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderableCollectionInstance {
	SehleRenderableInstance renderable_inst;
};

struct _SehleRenderableCollectionImplementation {
	SehleRenderableImplementation renderable_impl;
	void (*add_child) (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
	void (*remove_child) (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
	/* Set list bbox to the union of child boxes and mask to the bitwise OR-ed mask */
	void (*update_visuals) (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst);
};

struct _SehleRenderableCollectionClass {
	SehleRenderableClass renderable_klass;
};

unsigned int sehle_renderable_collection_get_type (void);

void sehle_renderable_collection_add_child (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
void sehle_renderable_collection_remove_child (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);
void sehle_renderable_collection_update_visuals (SehleRenderableCollectionImplementation *collection_impl, SehleRenderableCollectionInstance *collection_inst);

#ifdef __cplusplus
};
#endif

#endif
