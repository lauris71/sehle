#ifndef __SEHLE_RESOURCE_H__
#define __SEHLE_RESOURCE_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2018
 */

/*
 * Base class for shared engine objects
 *
 * Each created resource that has ID is registered by engine and can be retrieved by name
*/

typedef struct _SehleResource SehleResource;
typedef struct _SehleResourceClass SehleResourceClass;

#define SEHLE_TYPE_RESOURCE (sehle_resource_get_type ())
#define SEHLE_RESOURCE(r) (AZ_CHECK_INSTANCE_CAST ((r), SEHLE_TYPE_RESOURCE, SehleResource))
#define SEHLE_IS_RESOURCE(r) (AZ_CHECK_INSTANCE_TYPE ((r), SEHLE_TYPE_RESOURCE))

/*
 * Base states
 * CREATED - resource is newly created, owner has to set up date
 * MODIFIED - data has changed, need to rebuild OpneGL object
 * READY - everything is set up
 * IVALID - data is in incorrect state
 */

#define SEHLE_RESOURCE_STATE_CREATED 0
#define SEHLE_RESOURCE_STATE_MODIFIED 1
#define SEHLE_RESOURCE_STATE_READY 2
#define SEHLE_RESOURCE_STATE_INVALID 3

#include <az/active-object.h>

#include <sehle/sehle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleResource {
	AZActiveObject active_object;
	SehleEngine *engine;
	/* For resource sharing */
	unsigned char *id;
	/* State for subclass use */
	unsigned int state;
	/* OpenGL object */
	/* Only valid if READY */
	unsigned int gl_handle;
};

struct _SehleResourceClass {
	AZActiveObjectClass active_object_class;
	/* (Re)create engine resources */
	/* Invariant: state == MODIFIED */
	void (*build) (SehleResource *res);
};

unsigned int sehle_resource_get_type (void);

/* Get OpenGL handle, (re)creating engine resources */
/* Invariant: state == READY || state == MODIFIED */
unsigned int sehle_resource_get_handle (SehleResource *res);

ARIKKEI_INLINE
void sehle_resource_set_sate (SehleResource *res, unsigned int state) {
	res->state = (res->state & 0xfffffffc) | state;
}

ARIKKEI_INLINE
unsigned int sehle_resource_get_sate (SehleResource *res) {
	return res->state & 3;
}

ARIKKEI_INLINE
unsigned int sehle_resource_is_initialized (SehleResource *res) {
	return (res->state & 3) != SEHLE_RESOURCE_STATE_CREATED;
}

ARIKKEI_INLINE
unsigned int sehle_resource_is_modified (SehleResource *res) {
	return (res->state & 3) == SEHLE_RESOURCE_STATE_MODIFIED;
}

ARIKKEI_INLINE
unsigned int sehle_resource_is_ready (SehleResource *res) {
	return (res->state & 3) == SEHLE_RESOURCE_STATE_READY;
}

ARIKKEI_INLINE
unsigned int sehle_resource_is_invalid (SehleResource *res) {
	return (res->state & 3) == SEHLE_RESOURCE_STATE_INVALID;
}

/* For subclass implementations */
/* Initializes engine link and registers resource in engine for reuse */
/* Invariant: state == CREATED */
void sehle_resource_setup (SehleResource *res, SehleEngine *engine, const unsigned char *id);
/* (Re)create engine resources */
/* Invariant: state == MODIFIED */
void sehle_resource_build (SehleResource *res);

#ifdef __cplusplus
};
#endif

#endif

