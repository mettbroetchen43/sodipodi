#ifndef __SP_CLIPPATH_H__
#define __SP_CLIPPATH_H__

/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define SP_TYPE_CLIPPATH (sp_clippath_get_type ())
#define SP_CLIPPATH(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CLIPPATH, SPClipPath))
#define SP_CLIPPATH_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CLIPPATH, SPClipPathClass))
#define SP_IS_CLIPPATH(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CLIPPATH))
#define SP_IS_CLIPPATH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CLIPPATH))

#include "display/nr-arena-forward.h"
#include "sp-object-group.h"

struct _SPClipPath {
	SPObjectGroup group;
};

struct _SPClipPathClass {
	SPObjectGroupClass parent_class;
};

GtkType sp_clippath_get_type (void);

NRArenaItem *sp_clippath_show (SPClipPath *cp, NRArena *arena);

#endif
