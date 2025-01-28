#define __SEHLE_RESOURCEMANAGER_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012
//

static const int debug = 0;

#include <string.h>

#include <sehle/resource.h>

#include <sehle/resource-manager.h>

static void resource_manager_init (SehleResourceManagerClass *klass, SehleResourceManager *manager);
static void resource_manager_finalize (SehleResourceManagerClass *klass, SehleResourceManager *manager);

unsigned int resource_manager_type = 0;
SehleResourceManagerClass *resource_manager_class = NULL;

unsigned int
sehle_resource_manager_get_type (void)
{
	if (!resource_manager_type) {
		az_register_type (&resource_manager_type, (const unsigned char *) "SehleResourcemanager", AZ_TYPE_BLOCK, sizeof (SehleResourceManagerClass), sizeof (SehleResourceManager), 0,
			NULL,
			(void (*) (const AZImplementation *, void *)) resource_manager_init,
			(void (*) (const AZImplementation *, void *)) resource_manager_finalize);
		resource_manager_class = (SehleResourceManagerClass *) az_type_get_class (resource_manager_type);
	}
	return resource_manager_type;
}

static unsigned int
resource_hash (const void *key)
{
	SehleResource *res = (SehleResource *) key;
	return arikkei_string_hash (res->id) ^ arikkei_int32_hash (ARIKKEI_INT_TO_POINTER(res->active_object.object.klass->reference_klass.klass.implementation.type));
}

static unsigned int
resource_equal (const void *lhs, const void *rhs)
{
	SehleResource *lres = (SehleResource *) lhs;
	SehleResource *rres = (SehleResource *) rhs;
	return (lres->active_object.object.klass == rres->active_object.object.klass) && !strcmp ((const char *) lres->id, (const char *) rres->id);
}

static void
resource_manager_init (SehleResourceManagerClass *klass, SehleResourceManager *manager)
{
	arikkei_dict_setup_full (&manager->instance_dict, 701, resource_hash, resource_equal);
}

static void
resource_manager_finalize (SehleResourceManagerClass *klass, SehleResourceManager *manager)
{
	arikkei_dict_release (&manager->instance_dict);
}

SehleResourceManager *
sehle_resource_manager_new (void)
{
	return (SehleResourceManager *) az_instance_new (SEHLE_TYPE_RESOURCE_MANAGER);
}

struct Lookup {
	unsigned int type;
	const unsigned char *key;
};

static unsigned int
lookup_hash (const void *key)
{
	struct Lookup *lookup = (struct Lookup *) key;
	return arikkei_string_hash (lookup->key) ^ arikkei_int32_hash (ARIKKEI_INT_TO_POINTER(lookup->type));
}

static unsigned int
lookup_equal (const void *lhs, const void *rhs)
{
	struct Lookup *lookup = (struct Lookup *) lhs;
	SehleResource *res = (SehleResource *) rhs;
	return (lookup->type == res->active_object.object.klass->reference_klass.klass.implementation.type) && !strcmp ((const char *) lookup->key, (const char *) res->id);
}

SehleResource *
sehle_resource_manager_lookup (SehleResourceManager *manager, unsigned int type, const unsigned char *key)
{
	struct Lookup lookup;
	lookup.key = key;
	lookup.type = type;
	return (SehleResource *) arikkei_dict_lookup_foreign (&manager->instance_dict, &lookup, lookup_hash, lookup_equal);
}

void
sehle_resource_manager_add_resource (SehleResourceManager *manager, SehleResource *res, unsigned int replace)
{
	SehleResource *old = (SehleResource *) arikkei_dict_lookup (&manager->instance_dict, res);
	if (old) {
		if (!replace) return;
		arikkei_dict_remove (&manager->instance_dict, old);
	}
	arikkei_dict_insert (&manager->instance_dict, res, res);
}

void
sehle_resource_manager_remove_resource (SehleResourceManager *manager, SehleResource *res)
{
	arikkei_return_if_fail (manager != NULL);
	arikkei_return_if_fail (res != NULL);
	arikkei_return_if_fail (SEHLE_IS_RESOURCE (res));
	arikkei_dict_remove (&manager->instance_dict, res);
}

