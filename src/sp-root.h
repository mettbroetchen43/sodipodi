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

#define SP_TYPE_ROOT (sp_root_get_type ())
#define SP_ROOT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_ROOT, SPRoot))
#define SP_IS_ROOT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_ROOT))

#if 0
#include <libnr/nr-matrix.h>
#include "svg/svg-types.h"
#include "enums.h"
#endif
#include "sp-item-group.h"

struct _SPRoot {
	SPVPGroup group;

	float version;

	guint svg : 8;
	guint sodipodi : 8;
	guint original : 8;

#if 0
	SPSVGLength x;
	SPSVGLength y;
	SPSVGLength width;
	SPSVGLength height;

	SPSVGViewBox viewBox;

	/* preserveAspectRatio */
	unsigned int aspect_set : 1;
	unsigned int aspect_align : 4;
	unsigned int aspect_clip : 1;

	/* Child to parent additional transform */
	NRMatrixD c2p;
#endif

	/* List of namedviews */
	/* fixme: use single container instead */
	/* GSList *namedviews; */
	/* Root-level <defs> node */
	SPDefs *defs;
};

struct _SPRootClass {
	SPVPGroupClass parent_class;
};

GType sp_root_get_type (void);

#endif
