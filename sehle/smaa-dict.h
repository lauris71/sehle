#ifndef __SMAA_DICT_H__
#define __SMAA_DICT_H__

#ifdef __cplusplus
extern "C" {
#endif

const unsigned char *smaa_mmap (const char *name, size_t *csize);
void smaa_unmap (const unsigned char *data);

#ifdef __cplusplus
}; /* "C" */
#endif

#endif
