#ifndef __SP_ANCHOR_H__
#define __SP_ANCHOR_H__

/*
 * SVG <a> element implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-item-group.h"

#define SP_TYPE_ANCHOR (sp_anchor_get_type ())
#define SP_ANCHOR(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_ANCHOR, SPAnchor))
#define SP_ANCHOR_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ANCHOR, SPAnchorClass))
#define SP_IS_ANCHOR(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_ANCHOR))
#define SP_IS_ANCHOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ANCHOR))

struct _SPAnchor {
	SPGroup group;

	guchar *href;
};

struct _SPAnchorClass {
	SPGroupClass parent_class;
};

GtkType sp_anchor_get_type (void);

#endif
