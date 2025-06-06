#define __SHADERS_DICT_C__

#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "shaders-code.c"

struct StringData {
    const char *cdata;
    uint64_t csize;
};

static struct StringData strings[] = {
    { smaa_txt_0_data, sizeof (smaa_txt_0_data) - 1 },
    { control_fragment_txt_0_data, sizeof (control_fragment_txt_0_data) - 1 },
    { control_vertex_txt_0_data, sizeof (control_vertex_txt_0_data) - 1 },
    { decodegbuffer_txt_0_data, sizeof (decodegbuffer_txt_0_data) - 1 },
    { depth_fragment_txt_0_data, sizeof (depth_fragment_txt_0_data) - 1 },
    { depth_vertex_txt_0_data, sizeof (depth_vertex_txt_0_data) - 1 },
    { dns_fragment_glsl_0_data, sizeof (dns_fragment_glsl_0_data) - 1 },
    { dns_vertex_txt_0_data, sizeof (dns_vertex_txt_0_data) - 1 },
    { encodegbuffer_txt_0_data, sizeof (encodegbuffer_txt_0_data) - 1 },
    { exposure_txt_0_data, sizeof (exposure_txt_0_data) - 1 },
    { findtangentspace_txt_0_data, sizeof (findtangentspace_txt_0_data) - 1 },
    { foliage_gbuf_fragment_txt_0_data, sizeof (foliage_gbuf_fragment_txt_0_data) - 1 },
    { foliage_gbuf_vertex_txt_0_data, sizeof (foliage_gbuf_vertex_txt_0_data) - 1 },
    { light_fragment_glsl_0_data, sizeof (light_fragment_glsl_0_data) - 1 },
    { light_vertex_glsl_0_data, sizeof (light_vertex_glsl_0_data) - 1 },
    { lightvolume_fragment_txt_0_data, sizeof (lightvolume_fragment_txt_0_data) - 1 },
    { occlusion_fragment_txt_0_data, sizeof (occlusion_fragment_txt_0_data) - 1 },
    { occlusion_vertex_txt_0_data, sizeof (occlusion_vertex_txt_0_data) - 1 },
    { overlay_vertex_glsl_0_data, sizeof (overlay_vertex_glsl_0_data) - 1 },
    { overlay_fragment_glsl_0_data, sizeof (overlay_fragment_glsl_0_data) - 1 },
    { particles_fragment_txt_0_data, sizeof (particles_fragment_txt_0_data) - 1 },
    { particles_vertex_txt_0_data, sizeof (particles_vertex_txt_0_data) - 1 },
    { plain_fragment_txt_0_data, sizeof (plain_fragment_txt_0_data) - 1 },
    { plain_vertex_txt_0_data, sizeof (plain_vertex_txt_0_data) - 1 },
    { postprocess_blur4x4_fragment_txt_0_data, sizeof (postprocess_blur4x4_fragment_txt_0_data) - 1 },
    { postprocess_tonemap_fragment_txt_0_data, sizeof (postprocess_tonemap_fragment_txt_0_data) - 1 },
    { postprocess_vertex_txt_0_data, sizeof (postprocess_vertex_txt_0_data) - 1 },
    { sky_fragment_txt_0_data, sizeof (sky_fragment_txt_0_data) - 1 },
    { sky_vertex_txt_0_data, sizeof (sky_vertex_txt_0_data) - 1 },
    { stars_fragment_txt_0_data, sizeof (stars_fragment_txt_0_data) - 1 },
    { stars_vertex_txt_0_data, sizeof (stars_vertex_txt_0_data) - 1 },
    { terrain_gbuf_fragment_txt_0_data, sizeof (terrain_gbuf_fragment_txt_0_data) - 1 },
    { terrain_gbuf_vertex_txt_0_data, sizeof (terrain_gbuf_vertex_txt_0_data) - 1 },
    { water_fragment_txt_0_data, sizeof (water_fragment_txt_0_data) - 1 },
    { water_vertex_txt_0_data, sizeof (water_vertex_txt_0_data) - 1 },
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
    { "SMAA.txt", 0, 1, 13703, 55800, 0 },
    { "control-fragment.txt", 1, 1, 664, 2011, 0 },
    { "control-vertex.txt", 2, 1, 313, 830, 0 },
    { "decodeGBuffer.txt", 3, 1, 675, 1478, 0 },
    { "depth-fragment.txt", 4, 1, 381, 969, 0 },
    { "depth-vertex.txt", 5, 1, 465, 1275, 0 },
    { "dns-fragment.glsl", 6, 1, 1490, 5307, 0 },
    { "dns-vertex.txt", 7, 1, 582, 2110, 0 },
    { "encodeGBuffer.txt", 8, 1, 683, 1399, 0 },
    { "exposure.txt", 9, 1, 272, 504, 0 },
    { "findTangentSpace.txt", 10, 1, 243, 467, 0 },
    { "foliage-gbuf-fragment.txt", 11, 1, 398, 929, 0 },
    { "foliage-gbuf-vertex.txt", 12, 1, 482, 1274, 0 },
    { "light-fragment.glsl", 13, 1, 1789, 6036, 0 },
    { "light-vertex.glsl", 14, 1, 195, 299, 0 },
    { "lightvolume-fragment.txt", 15, 1, 477, 1004, 0 },
    { "occlusion-fragment.txt", 16, 1, 530, 1116, 0 },
    { "occlusion-vertex.txt", 17, 1, 211, 350, 0 },
    { "overlay-vertex.glsl", 18, 1, 359, 924, 0 },
    { "overlay-fragment.glsl", 19, 1, 460, 1362, 0 },
    { "particles-fragment.txt", 20, 1, 524, 1320, 0 },
    { "particles-vertex.txt", 21, 1, 240, 476, 0 },
    { "plain-fragment.txt", 22, 1, 97, 131, 0 },
    { "plain-vertex.txt", 23, 1, 186, 375, 0 },
    { "postprocess-blur4x4-fragment.txt", 24, 1, 419, 995, 0 },
    { "postprocess-tonemap-fragment.txt", 25, 1, 212, 348, 0 },
    { "postprocess-vertex.txt", 26, 1, 124, 171, 0 },
    { "sky-fragment.txt", 27, 1, 838, 1991, 0 },
    { "sky-vertex.txt", 28, 1, 814, 1860, 0 },
    { "stars-fragment.txt", 29, 1, 132, 167, 0 },
    { "stars-vertex.txt", 30, 1, 383, 777, 0 },
    { "terrain-gbuf-fragment.txt", 31, 1, 728, 2005, 0 },
    { "terrain-gbuf-vertex.txt", 32, 1, 214, 500, 0 },
    { "water-fragment.txt", 33, 1, 1546, 4685, 0 },
    { "water-vertex.txt", 34, 1, 248, 505, 0 },
};

const unsigned char *
sehle_get_map (const unsigned char *name, unsigned int *csize)
{
    unsigned int i;
    for (i = 0; i < 35; i++) {
        if (!strcmp (maps[i].mapname, (const char *) name)) {
            if (!maps[i].data) {;
                maps[i].data = (char *) malloc (maps[i].size + 1);
                if (maps[i].zsize != 0) {
                    unsigned char *zdata;
                    uint64_t pos;
                    int j, zresult;
                    unsigned long dlen = (unsigned long) maps[i].size;
                    zdata = (unsigned char *) malloc (maps[i].zsize);
                    pos = 0;
                    for (j = 0; j < maps[i].numstrings; j++) {
                        memcpy (zdata + pos, strings[maps[i].firststring + j].cdata, strings[maps[i].firststring + j].csize);
                        pos += strings[maps[i].firststring + j].csize;
                    }
                    zresult = uncompress ((unsigned char *) maps[i].data, &dlen, zdata, (unsigned long) maps[i].zsize);
                    free (zdata);
                } else {
                    uint64_t pos;
                    int j;
                    pos = 0;
                    for (j = 0; j < maps[i].numstrings; j++) {
                        memcpy (maps[i].data + pos, strings[maps[i].firststring + j].cdata, strings[maps[i].firststring + j].csize);
                        pos += strings[maps[i].firststring + j].csize;
                    }
                }
                maps[i].data[maps[i].size] = 0;
            }
            if (csize) *csize = maps[i].size;
            return (const unsigned char *) maps[i].data;
        }
    }
    return NULL;
}
void
sehle_free_map (const unsigned char *data)
{
    unsigned int i;
    for (i = 0; i < 35; i++) {
        if (maps[i].data == (char *) data) {
            free ((void *) maps[i].data);
            maps[i].data = NULL;
        }
    }
}
