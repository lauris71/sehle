#define __SMAA_DICT_C__

#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "smaa-data.c"

struct StringData {
    const char *cdata;
    size_t csize;
};

static struct StringData strings[] = {
    { area_texture_0_data, sizeof (area_texture_0_data) - 1 },
    { search_texture_0_data, sizeof (search_texture_0_data) - 1 },
};

struct MapData {
    const char *mapname;
    int firststring;
    int numstrings;
    unsigned int zsize;
    unsigned int size;
    char *data;
};

static struct MapData maps[] = {
    { "area_texture", 0, 1, 44701, 268800, 0 },
    { "search_texture", 1, 1, 76, 3072, 0 },
};

const unsigned char *
smaa_mmap (const char *name, size_t *csize)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (!strcmp (maps[i].mapname, name)) {
            if (!maps[i].data) {;
                if (maps[i].zsize != 0) {
                    unsigned char *zdata;
                    size_t pos;
                    int j, zresult;
                    unsigned long dlen = (unsigned long) maps[i].size;
                    zdata = (unsigned char *) malloc (maps[i].zsize);
                    pos = 0;
                    for (j = 0; j < maps[i].numstrings; j++) {
                        memcpy (zdata + pos, strings[maps[i].firststring + j].cdata, strings[maps[i].firststring + j].csize);
                        pos += strings[maps[i].firststring + j].csize;
                    }
                    maps[i].data = (char *) malloc (maps[i].size);
                    zresult = uncompress ((unsigned char *) maps[i].data, &dlen, zdata, (unsigned long) maps[i].zsize);
                    free (zdata);
                } else {
                    size_t pos;
                    int j;
                    maps[i].data = (char *) malloc (maps[i].size);
                    pos = 0;
                    for (j = 0; j < maps[i].numstrings; j++) {
                        memcpy (maps[i].data + pos, strings[maps[i].firststring + j].cdata, strings[maps[i].firststring + j].csize);
                        pos += strings[maps[i].firststring + j].csize;
                    }
                }
            }
            if (csize) *csize = maps[i].size;
            return (const unsigned char *) maps[i].data;
        }
    }
    return NULL;
}
void
smaa_unmap (const unsigned char *data)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (maps[i].data == (char *) data) {
            free ((void *) maps[i].data);
            maps[i].data = NULL;
        }
    }
}
