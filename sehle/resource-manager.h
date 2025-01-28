#ifndef __SEHLE_RESOURCEMANAGER_H__
#define __SEHLE_RESOURCEMANAGER_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012
//

//
// ResourceManager
//
// An internal class that keeps track of managed resource classes and instances
//

typedef struct _SehleResourceManager SehleResourceManager;
typedef struct _SehleResourceManagerClass SehleResourceManagerClass;

#define SEHLE_TYPE_RESOURCE_MANAGER (sehle_resource_manager_get_type ())

#include <arikkei/arikkei-dict.h>
#include <az/class.h>

#include <sehle/sehle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleResourceManager {
	ArikkeiDict instance_dict;
};

struct _SehleResourceManagerClass {
	AZClass klass;
};

unsigned int sehle_resource_manager_get_type (void);

SehleResourceManager *sehle_resource_manager_new (void);

/* Return resource or NULL, no reference added */
SehleResource *sehle_resource_manager_lookup (SehleResourceManager *manager, unsigned int type, const unsigned char *key);
/* Register resource, replacing old if present, no references dropped or added */
void sehle_resource_manager_add_resource (SehleResourceManager *manager, SehleResource *res, unsigned int replace);
/* Unregisters resource, no references dropped */
void sehle_resource_manager_remove_resource (SehleResourceManager *manager, SehleResource *res);

#ifdef __cplusplus
};
#endif

#endif

