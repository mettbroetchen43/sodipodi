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

#include <libnr/nr-matrix.h>
#include "svg/svg-types.h"
#include "sp-item-group.h"

G_BEGIN_DECLS

#define SP_TYPE_ROOT (sp_root_get_type ())
#define SP_ROOT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_ROOT, SPRoot))
#define SP_ROOT_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), SP_TYPE_ROOT, SPRootClass))
#define SP_IS_ROOT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_ROOT))
#define SP_IS_ROOT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SP_TYPE_ROOT))

struct _SPRoot {
	SPGroup group;

	guint svg : 8;
	guint sodipodi : 8;
	guint original : 8;

	SPSVGLength width;
	SPSVGLength height;

	NRMatrixD viewbox;

	/* List of namedviews */
	/* fixme: use single container instead */
	GSList *namedviews;
	/* Root-level <defs> node */
	SPDefs *defs;
};

struct _SPRootClass {
	SPGroupClass parent_class;
};

GType sp_root_get_type (void);

G_BEGIN_DECLS

#endif
