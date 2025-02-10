#ifndef __NR_PIXBLOCK_H__
#define __NR_PIXBLOCK_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define NR_TYPE_PIXBLOCK (nr_pixblock_get_type ())

typedef struct _NRPixBlockClass NRPixBlockClass;

#include <assert.h>

#include <az/class.h>

#include <nr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Channel type */
#define NR_PIXBLOCK_U8 0
#define NR_PIXBLOCK_U16 1
#define NR_PIXBLOCK_F32 2

/* Memory allocation profile */
#define NR_PIXBLOCK_STANDARD 0
#define NR_PIXBLOCK_TRANSIENT 1
#define NR_PIXBLOCK_EXTERNAL 2

/* Colorspace */
#define NR_PIXBLOCK_LINEAR 0
#define NR_PIXBLOCK_SRGB 1

/* Predefined types */
#define NR_PIXBLOCK_MODE_G8 0
/* 8 bit RGB */
#define NR_PIXBLOCK_MODE_R8G8B8 1
/* Normal 8 bit RGBA */
#define NR_PIXBLOCK_MODE_R8G8B8A8N 2
/* Premultiplied 8 bit RGBA */
#define NR_PIXBLOCK_MODE_R8G8B8A8P 3

// Automatically allocated rowstride is aligned at 4 bytes

struct _NRPixBlock {
	/* Channel type (u8, u16, f32) */
	unsigned int ch_type : 2;
	/* Number of channels (1..4) */
	unsigned int n_channels : 3;
	/* Color space (linear, srgb) */
	unsigned int colorspace : 2;
	/* Channels are premultiplied with alpha */
	unsigned int premultiplied : 1;

	/* Pixblock is empty (marker for faster pixel operations */
	unsigned int empty : 1;
	/* Pixel allocation_profile */
	unsigned int storage : 2;

	/* Channel width in bytes (automatically calculated) */
	unsigned int ch_width : 2;

	unsigned int rs;
	NRRectl area;
	uint8_t *px;
};

struct _NRPixBlockClass {
	AZClass klass;
};

unsigned int nr_pixblock_get_type (void);

static inline
uint8_t *nr_pixblock_get_row (const NRPixBlock *pb, unsigned int r)
{
	return pb->px + r * pb->rs;
}

#define PB_MODE(pb) ((pb)->n_channels == 1) ? NR_PIXBLOCK_MODE_G8 : ((pb)->n_channels == 3) ? NR_PIXBLOCK_MODE_R8G8B8 : ((pb)->premultiplied) ? NR_PIXBLOCK_MODE_R8G8B8A8P : NR_PIXBLOCK_MODE_R8G8B8A8N

#define NR_PIXBLOCK_MODE_BPP(m) ((m == NR_PIXBLOCK_MODE_G8) ? 1 : (m == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4)
#define NR_PIXBLOCK_PX(pb) ((pb)->px)
#define NR_PIXBLOCK_ROW(pb,r) nr_pixblock_get_row(pb, r)

/* Newly created pixblocks always have 4-byte row alignment */
void nr_pixblock_setup_full (NRPixBlock *pb, unsigned int ch_type, unsigned int n_channels, unsigned int colorspace, unsigned int premul, int x0, int y0, int x1, int y1);
void nr_pixblock_setup_transient_full (NRPixBlock *pb, unsigned int ch_type, unsigned int n_channels, unsigned int colorspace, unsigned int premul, int x0, int y0, int x1, int y1);
void nr_pixblock_setup_extern_full (NRPixBlock *pb, unsigned int ch_type, unsigned int n_channels, unsigned int colorspace, unsigned int premul, int x0, int y0, int x1, int y1, uint8_t *px, int rs, int empty);

static inline void
nr_pixblock_release (NRPixBlock *pb)
{
	az_instance_finalize (pb, NR_TYPE_PIXBLOCK);
}

/* Set up pixblock with one predefined mode */
void nr_pixblock_setup (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear);
void nr_pixblock_setup_transient (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear);
void nr_pixblock_setup_extern (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, uint8_t *px, int rs, int empty, int clear);

/* Clone or duplicate pixblock ensuring 4-aligned row packing */
void nr_pixblock_clone_packed (NRPixBlock *dst, const NRPixBlock *src);

/* Fill pixblock storage with 0 */
void nr_pixblock_clear (NRPixBlock *pb);

/* Helpers */

static inline unsigned int
nr_pixblock_has_alpha (const NRPixBlock *pb)
{
	/* LA and RGBA (1 & 3) */
	return pb->n_channels & 1;
}

void nr_pixblock_get_channel_limits (const NRPixBlock *pb, unsigned int minv[], unsigned int maxv[]);
void nr_pixblock_get_histogram (const NRPixBlock *pb, unsigned int histogram[][256]);
unsigned int nr_pixblock_get_crc32 (const NRPixBlock *pb);
unsigned long long nr_pixblock_get_crc64 (const NRPixBlock *pb);
unsigned int nr_pixblock_get_hash (const NRPixBlock *pb);
unsigned int nr_pixblock_is_equal (const NRPixBlock *a, const NRPixBlock *b);

#ifdef __cplusplus
};
#endif

#endif
