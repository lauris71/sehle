#ifndef __NR_TYPES_H__
#define __NR_TYPES_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NRMatrixf NRMatrixf;
typedef struct _NRPointd NRPointd;
typedef struct _NRPointf NRPointf;
typedef struct _NRPointl NRPointl;
typedef struct _NRPoints NRPoints;
typedef struct _NRRectf NRRectf;
typedef struct _NRRectl NRRectl;
typedef struct _NRRects NRRects;

typedef struct _NRPixBlock NRPixBlock;

struct _NRMatrixf {
	float c[6];
};

struct _NRPointd {
	union {
		struct {
			double x, y;
		};
		double c[2];
	};
};

struct _NRPointf {
	union {
		struct {
			float x, y;
		};
		float c[2];
	};
};

struct _NRPointl {
	union {
		struct {
			int32_t x, y;
		};
		int32_t c[2];
	};
};

struct _NRPoints {
	union {
		struct {
			int16_t x, y;
		};
		int16_t c[2];
	};
};

struct _NRRectf {
	union {
		struct {
			float x0, y0, x1, y1;
		};
		struct {
			NRPointf min, max;
		};
		float c[4];
	};
};

struct _NRRectl {
	union {
		struct {
			int32_t x0, y0, x1, y1;
		};
		struct {
			NRPointl min, max;
		};
		int32_t c[4];
	};
};

struct _NRRects {
	union {
		struct {
			int16_t x0, y0, x1, y1;
		};
		struct {
			NRPoints min, max;
		};
		int16_t c[4];
	};
};

typedef struct _NRImage NRImage;

#ifdef __cplusplus
};
#endif

#endif
