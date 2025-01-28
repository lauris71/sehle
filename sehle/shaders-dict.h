#ifndef __SHADERS_DICT_H__
#define __SHADERS_DICT_H__

#ifdef __cplusplus
extern "C" {
#endif

const unsigned char *sehle_get_map (const unsigned char *name, unsigned int *csize);
void sehle_free_map (const unsigned char *data);

#ifdef __cplusplus
}; /* "C" */
#endif

#endif
