#ifndef __SP_ROOT_H__
#define __SP_ROOT_H__

/*
 * SVG <svg> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg-types.h"
#include "sp-item-group.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_ROOT (sp_root_get_type ())
#define SP_ROOT(o) (GTK_CHECK_CAST ((o), SP_TYPE_ROOT, SPRoot))
#define SP_ROOT_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_ROOT, SPRootClass))
#define SP_IS_ROOT(o) (GTK_CHECK_TYPE ((o), SP_TYPE_ROOT))
#define SP_IS_ROOT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_ROOT))

struct _SPRoot {
	SPGroup group;

	guint svg : 8;
	guint sodipodi : 8;
	guint original : 8;

	SPSVGLength width;
	SPSVGLength height;

	ArtDRect viewbox;
	/* List of namedviews */
	/* fixme: use single container instead */
	GSList *namedviews;
	/* Root-level <defs> node */
	SPDefs *defs;
};

struct _SPRootClass {
	SPGroupClass parent_class;
};

GtkType sp_root_get_type (void);

END_GNOME_DECLS

#endif
