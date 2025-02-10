#define __NR_PIXBLOCK_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <string.h>
#include <stdlib.h>

#include "pixblock.h"

/* Memory management */

uint8_t *nr_pixelstore_4K_new (int clear, unsigned char val);
void nr_pixelstore_4K_free (uint8_t *px);
uint8_t *nr_pixelstore_16K_new (int clear, unsigned char val);
void nr_pixelstore_16K_free (uint8_t *px);
uint8_t *nr_pixelstore_64K_new (int clear, unsigned char val);
void nr_pixelstore_64K_free (uint8_t *px);

static void nr_pixblock_class_init (NRPixBlockClass *klass);
static void nr_pixblock_finalize (NRPixBlockClass *klass, NRPixBlock *pb);

unsigned int
nr_pixblock_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const uint8_t *) "NRPixBlock", AZ_TYPE_BLOCK, sizeof (NRPixBlockClass), sizeof (NRPixBlock), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) nr_pixblock_class_init,
			NULL,
			(void (*) (const AZImplementation *, void *)) nr_pixblock_finalize);
	}
	return type;
}

static void
nr_pixblock_class_init (NRPixBlockClass *klass)
{
}

static void
nr_pixblock_finalize (NRPixBlockClass *klass, NRPixBlock *pb)
{
	if (pb->storage == NR_PIXBLOCK_STANDARD) {
		if (pb->px) free (pb->px);
	} else if (pb->storage == NR_PIXBLOCK_TRANSIENT) {
		int size = (pb->area.x1 - pb->area.x0) * pb->rs;
		if (size > 65536) {
			free (pb->px);
		} else if (size > 16384) {
			nr_pixelstore_64K_free (pb->px);
		} else if (size > 4096) {
			nr_pixelstore_16K_free (pb->px);
		} else if (size > 0) {
			nr_pixelstore_4K_free (pb->px);
		}
	}
}

static const unsigned int ch_widths[] = { 1, 2, 4 };

void
nr_pixblock_setup_full (NRPixBlock *pb, unsigned int type, unsigned int n_channels, unsigned int colorspace, unsigned int premultiplied, int x0, int y0, int x1, int y1)
{
	unsigned int size;

	az_instance_init (pb, NR_TYPE_PIXBLOCK);

	pb->ch_type = type;
	pb->n_channels = n_channels;
	pb->colorspace = colorspace;
	pb->premultiplied = premultiplied;

	pb->ch_width = ch_widths[pb->ch_type];
	pb->empty = 1;
	pb->storage = NR_PIXBLOCK_STANDARD;

	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->rs = ((x1 - x0) * pb->n_channels * pb->ch_width + 3) & 0xfffffffc;
	size = (y1 - y0) * pb->rs;
	if (size) {
		pb->px = (uint8_t *) malloc(size);
	}
}

void
nr_pixblock_setup_transient_full (NRPixBlock *pb, unsigned int type, unsigned int n_channels, unsigned int colorspace, unsigned int premultiplied, int x0, int y0, int x1, int y1)
{
	unsigned int size;

	az_instance_init (pb, NR_TYPE_PIXBLOCK);

	pb->ch_type = type;
	pb->n_channels = n_channels;
	pb->colorspace = colorspace;
	pb->premultiplied = premultiplied;

	pb->ch_width = ch_widths[pb->ch_type];
	pb->empty = 1;

	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->rs = ((x1 - x0) * pb->n_channels * pb->ch_width + 3) & 0xfffffffc;
	size = (y1 - y0) * pb->rs;
	if (size <= 4096) {
		pb->storage = NR_PIXBLOCK_TRANSIENT;
		pb->px = nr_pixelstore_4K_new (0, 0);
	} else if (size <= 16384) {
		pb->storage = NR_PIXBLOCK_TRANSIENT;
		pb->px = nr_pixelstore_16K_new (0, 0);
	} else if (size <= 65536) {
		pb->storage = NR_PIXBLOCK_TRANSIENT;
		pb->px = nr_pixelstore_64K_new (0, 0);
	} else {
		pb->storage = NR_PIXBLOCK_STANDARD;
		pb->px = (uint8_t *) malloc (size);
	}
	size = (x1 - x0) * pb->rs;
}

void
nr_pixblock_setup_extern_full (NRPixBlock *pb, unsigned int type, unsigned int n_channels, unsigned int colorspace, unsigned int premultiplied, int x0, int y0, int x1, int y1, uint8_t *px, int rs, int empty)
{
	az_instance_init (pb, NR_TYPE_PIXBLOCK);

	pb->ch_type = type;
	pb->n_channels = n_channels;
	pb->colorspace = colorspace;
	pb->premultiplied = premultiplied;

	pb->ch_width = ch_widths[pb->ch_type];
	pb->empty = empty;
	pb->storage = NR_PIXBLOCK_EXTERNAL;

	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->rs = rs;
	pb->px = px;
}

void
nr_pixblock_clear (NRPixBlock *pb)
{
	if (pb->rs == (((pb->area.x1 - pb->area.x0) * pb->n_channels * pb->ch_width + 3) & 0xfffffffc)) {
		/* Packed */
		unsigned int size = (pb->area.y1 - pb->area.y0) * pb->rs;
		memset (pb->px, 0x0, size);
	} else {
		/* Unpacked */
		int y;
		unsigned int size = (pb->area.x1 - pb->area.x0) * pb->n_channels * pb->ch_width;
		for (y = pb->area.y0; y < pb->area.y1; y++) {
			memset (pb->px + (y - pb->area.y0) * pb->rs, 0x0, size);
		}
	}
}

void
nr_pixblock_setup (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear)
{
	nr_pixblock_setup_full (pb, NR_PIXBLOCK_U8, NR_PIXBLOCK_MODE_BPP (mode), NR_PIXBLOCK_LINEAR, (mode == NR_PIXBLOCK_MODE_R8G8B8A8P), x0, y0, x1, y1);
	if (clear) nr_pixblock_clear (pb);
	pb->empty = 1;
}

void
nr_pixblock_setup_transient (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, int clear)
{
	nr_pixblock_setup_transient_full (pb, NR_PIXBLOCK_U8, NR_PIXBLOCK_MODE_BPP (mode), NR_PIXBLOCK_LINEAR, (mode == NR_PIXBLOCK_MODE_R8G8B8A8P), x0, y0, x1, y1);
	if (clear) nr_pixblock_clear (pb);
	pb->empty = 1;
}

void
nr_pixblock_setup_extern (NRPixBlock *pb, int mode, int x0, int y0, int x1, int y1, uint8_t *px, int rs, int empty, int clear)
{
	int w, bpp;

	az_instance_init (pb, NR_TYPE_PIXBLOCK);

	w = x1 - x0;
	bpp = (mode == NR_PIXBLOCK_MODE_G8) ? 1 : (mode == NR_PIXBLOCK_MODE_R8G8B8) ? 3 : 4;

	pb->ch_type = NR_PIXBLOCK_U8;
	pb->ch_width = ch_widths[pb->ch_type];
	pb->n_channels = NR_PIXBLOCK_MODE_BPP (mode);
	pb->premultiplied = (mode == NR_PIXBLOCK_MODE_R8G8B8A8P);
	pb->storage = NR_PIXBLOCK_EXTERNAL;
	pb->empty = empty;
	pb->area.x0 = x0;
	pb->area.y0 = y0;
	pb->area.x1 = x1;
	pb->area.y1 = y1;
	pb->px = px;
	pb->rs = rs;

	if (clear) {
		if (rs == bpp * w) {
			memset (pb->px, 0x0, bpp * (y1 - y0) * w);
		} else {
			int y;
			for (y = y0; y < y1; y++) {
				memset (pb->px + (y - y0) * rs, 0x0, bpp * w);
			}
		}
	}
}

void
nr_pixblock_clone_packed (NRPixBlock *dst, const NRPixBlock *src)
{
	if (src->rs == (((src->area.x1 - src->area.x0) * src->n_channels * src->ch_width + 3) & 0xfffffffc)) {
		nr_pixblock_setup_extern_full (dst, src->ch_type, src->n_channels, src->colorspace, src->premultiplied, src->area.x0, src->area.y0, src->area.x1, src->area.y1, src->px, src->rs, src->empty);
	} else {
		int y;
		unsigned int rowsize = (src->area.x1 - src->area.x0) * src->n_channels * src->ch_width;
		nr_pixblock_setup_full (dst, src->ch_type, src->n_channels, src->colorspace, src->premultiplied, src->area.x0, src->area.y0, src->area.x1, src->area.y1);
		for (y = src->area.y0; y < src->area.y1; y++) {
			memcpy (nr_pixblock_get_row (dst, y - src->area.y0), nr_pixblock_get_row (src, y - src->area.y0), rowsize);
		}
	}
}

/* Helpers */

void
nr_pixblock_get_channel_limits (const NRPixBlock *pb, unsigned int minv[], unsigned int maxv[])
{
	int x, y, bpp, c;
	bpp = pb->n_channels;
	for (c = 0; c < bpp; c++) {
		minv[c] = 255;
		maxv[c] = 0;
	}
	for (y = pb->area.y0; y < pb->area.y1; y++) {
		const uint8_t *s;
		s = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs;
		for (x = pb->area.x0; x < pb->area.x1; x++) {
			for (c = 0; c < bpp; c++) {
				if (s[c] < minv[c]) minv[c] = s[c];
				if (s[c] > maxv[c]) maxv[c] = s[c];
			}
			s += bpp;
		}
	}
}

void
nr_pixblock_get_histogram (const NRPixBlock *pb, unsigned int histogram[][256])
{
	int x, y, bpp, c, v;
	bpp = pb->n_channels;
	for (c = 0; c < bpp; c++) {
		for (v = 0; v < 256; v++) histogram[c][v] = 0;
	}
	for (y = pb->area.y0; y < pb->area.y1; y++) {
		const uint8_t *s;
		s = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs;
		for (x = pb->area.x0; x < pb->area.x1; x++) {
			for (c = 0; c < bpp; c++) {
				histogram[c][s[c]] += 1;
			}
			s += bpp;
		}
	}
}

unsigned int
nr_pixblock_get_crc32 (const NRPixBlock *pb)
{
	unsigned int crc32, acc;
	unsigned int width, height, bpp, x, y, c, cnt;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = pb->n_channels;
	crc32 = 0;
	acc = 0;
	cnt = 0;
	for (y = 0; y < height; y++) {
		const uint8_t *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				acc <<= 8;
				acc |= *s++;
				if (++cnt >= 4) {
					crc32 ^= acc;
					acc = cnt = 0;
				}
			}
		}
		crc32 ^= acc;
		acc = cnt = 0;
	}
	return crc32;
}

unsigned long long
nr_pixblock_get_crc64 (const NRPixBlock *pb)
{
	unsigned long long crc64, acc, carry;
	unsigned int width, height, bpp, x, y, c, cnt;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = pb->n_channels;
	crc64 = 0;
	acc = 0;
	carry = 0;
	cnt = 0;
	for (y = 0; y < height; y++) {
		const uint8_t *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				acc <<= 8;
				acc |= *s++;
				if (++cnt >= 8) {
					if ((crc64 & 0x8000000000000000) && (acc & 0x8000000000000000)) carry = 1;
					crc64 += acc;
					crc64 += carry;
					acc = carry = 0;
					cnt = 0;
				}
			}
		}
		crc64 += acc;
		acc = cnt = 0;
	}
	return crc64;
}

unsigned int
nr_pixblock_get_hash (const NRPixBlock *pb)
{
	unsigned int hval;
	unsigned int width, height, bpp, x, y, c;
	width = pb->area.x1 - pb->area.x0;
	height = pb->area.y1 - pb->area.y0;
	bpp = pb->n_channels;
	hval = pb->ch_type;
	hval = (hval << 5) - hval + pb->n_channels;
	hval = (hval << 5) - hval + pb->empty;
	hval = (hval << 5) - hval + pb->premultiplied;
	hval = (hval << 5) - hval + pb->area.x0;
	hval = (hval << 5) - hval + pb->area.y0;
	hval = (hval << 5) - hval + pb->area.x1;
	hval = (hval << 5) - hval + pb->area.y1;
	for (y = 0; y < height; y++) {
		const uint8_t *s = NR_PIXBLOCK_ROW(pb, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				hval = (hval << 5) - hval + *s++;
			}
		}
	}
	return hval;
}

unsigned int
nr_pixblock_is_equal (const NRPixBlock *a, const NRPixBlock *b)
{
	unsigned int width, height, bpp, x, y, c;
	if (a == b) return 1;
	if (a->ch_type != b->ch_type) return 0;
	if (a->n_channels != b->n_channels) return 0;
	if (a->premultiplied != b->premultiplied) return 0;
	if (a->empty != b->empty) return 0;
	if (a->area.x0 - b->area.x0) return 0;
	if (a->area.y0 - b->area.y0) return 0;
	if (a->area.x1 - b->area.x1) return 0;
	if (a->area.y1 - b->area.y1) return 0;
	width = a->area.x1 - a->area.x0;
	height = a->area.y1 - a->area.y0;
	bpp = a->n_channels;
	for (y = 0; y < height; y++) {
		const uint8_t *pa = NR_PIXBLOCK_ROW(a, y);
		const uint8_t *pb = NR_PIXBLOCK_ROW(b, y);
		for (x = 0; x < width; x++) {
			for (c = 0; c < bpp; c++) {
				if (*pa++ != *pb++) return 0;
			}
		}
	}
	return 1;
}

/* PixelStore operations */

#define NR_4K_BLOCK 32
static uint8_t **nr_4K_px = NULL;
static unsigned int nr_4K_len = 0;
static unsigned int nr_4K_size = 0;

uint8_t *
nr_pixelstore_4K_new (int clear, unsigned char val)
{
	uint8_t *px;

	if (nr_4K_len != 0) {
		nr_4K_len -= 1;
		px = nr_4K_px[nr_4K_len];
	} else {
		px = (uint8_t *) malloc(4096);
	}
	
	if (clear) memset (px, val, 4096);

	return px;
}

void
nr_pixelstore_4K_free (uint8_t *px)
{
	if (nr_4K_len == nr_4K_size) {
		nr_4K_size += NR_4K_BLOCK;
		nr_4K_px = (uint8_t **) realloc(nr_4K_px, nr_4K_size * sizeof(uint8_t *));
	}

	nr_4K_px[nr_4K_len] = px;
	nr_4K_len += 1;
}

#define NR_16K_BLOCK 32
static uint8_t **nr_16K_px = NULL;
static unsigned int nr_16K_len = 0;
static unsigned int nr_16K_size = 0;

uint8_t *
nr_pixelstore_16K_new (int clear, unsigned char val)
{
	uint8_t *px;

	if (nr_16K_len != 0) {
		nr_16K_len -= 1;
		px = nr_16K_px[nr_16K_len];
	} else {
		px = (uint8_t *) malloc(16384);
	}
	
	if (clear) memset (px, val, 16384);

	return px;
}

void
nr_pixelstore_16K_free (uint8_t *px)
{
	if (nr_16K_len == nr_16K_size) {
		nr_16K_size += NR_16K_BLOCK;
		nr_16K_px = (uint8_t **) realloc(nr_16K_px, nr_16K_size * sizeof(uint8_t *));
	}

	nr_16K_px[nr_16K_len] = px;
	nr_16K_len += 1;
}

#define NR_64K_BLOCK 32
static uint8_t **nr_64K_px = NULL;
static unsigned int nr_64K_len = 0;
static unsigned int nr_64K_size = 0;

uint8_t *
nr_pixelstore_64K_new (int clear, unsigned char val)
{
	uint8_t *px;

	if (nr_64K_len != 0) {
		nr_64K_len -= 1;
		px = nr_64K_px[nr_64K_len];
	} else {
		px = (uint8_t *) malloc(65536);
	}

	if (clear) memset (px, val, 65536);

	return px;
}

void
nr_pixelstore_64K_free (uint8_t *px)
{
	if (nr_64K_len == nr_64K_size) {
		nr_64K_size += NR_64K_BLOCK;
		nr_64K_px = (uint8_t **) realloc(nr_64K_px, nr_64K_size * sizeof(uint8_t *));
	}

	nr_64K_px[nr_64K_len] = px;
	nr_64K_len += 1;
}

