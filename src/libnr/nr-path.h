#ifndef __NR_PATH_H__
#define __NR_PATH_H__

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

typedef struct _NRVPath NRVPath;
typedef struct _NRBPath NRBPath;

#include <libnr/nr-matrix.h>

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_bpath.h>

struct _NRVPath {
	ArtVpath *path;
};

struct _NRBPath {
	ArtBpath *path;
};

NRBPath *nr_path_duplicate_transform (NRBPath *d, NRBPath *s, NRMatrixF *transform);

#endif
