#ifndef __NR_ARENA_FORWARD_H__
#define __NR_ARENA_FORWARD_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

typedef struct _NRArena NRArena;
typedef struct _NRArenaClass NRArenaClass;

typedef struct _NRArenaItem NRArenaItem;
typedef struct _NRArenaItemClass NRArenaItemClass;

typedef struct _NRArenaGroup NRArenaGroup;
typedef struct _NRArenaGroupClass NRArenaGroupClass;

typedef struct _NRArenaShape NRArenaShape;
typedef struct _NRArenaShapeClass NRArenaShapeClass;

typedef struct _NRArenaShapeGroup NRArenaShapeGroup;
typedef struct _NRArenaShapeGroupClass NRArenaShapeGroupClass;

typedef struct _NRArenaImage NRArenaImage;
typedef struct _NRArenaImageClass NRArenaImageClass;

typedef struct _NRArenaGlyphs NRArenaGlyphs;
typedef struct _NRArenaGlyphsClass NRArenaGlyphsClass;

#endif
