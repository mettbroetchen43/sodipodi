#ifndef __NR_RENDER_H__
#define __NR_RENDER_H__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _NRRenderer NRRenderer;

#include <libnr/nr-pixblock.h>

typedef void (* NRRenderFunc) (NRRenderer *r, NRPixBlock *pb, NRPixBlock *m);

struct _NRRenderer {
	NRRenderFunc render;
};

#define nr_render(r,pb,m) ((NRRenderer *) (r))->render ((NRRenderer *) (r), (pb), (m))

#endif
