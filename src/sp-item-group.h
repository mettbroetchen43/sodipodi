#ifndef __SP_ITEM_GROUP_H__
#define __SP_ITEM_GROUP_H__

/*
 * SVG <g> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define SP_TYPE_GROUP (sp_group_get_type ())
#define SP_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_GROUP, SPGroup))
#define SP_IS_GROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_GROUP))

typedef struct _SPVPGroup SPVPGroup;
typedef struct _SPVPGroupClass SPVPGroupClass;

#define SP_TYPE_VPGROUP (sp_vpgroup_get_type ())
#define SP_VPGROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_GROUP, SPVPGroup))
#define SP_IS_VPGROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_VPGROUP))

#include <libnr/nr-matrix.h>
#include "svg/svg-types.h"

#include "sp-item.h"

/* Group */

struct _SPGroup {
	SPItem item;
	unsigned int transparent : 1;
};

struct _SPGroupClass {
	SPItemClass parent_class;
};

GType sp_group_get_type (void);

/* fixme: This is potentially dangerous (Lauris) */
/* Be extra careful what happens, if playing with <svg> */
void sp_group_set_transparent (SPGroup *group, unsigned int transparent);

void sp_item_group_ungroup (SPGroup *group, GSList **children);
GSList *sp_item_group_item_list (SPGroup *group);
SPObject *sp_item_group_get_child_by_name (SPGroup *group, SPObject *ref, const unsigned char *name);

/* VPGroup */

struct _SPVPGroup {
	SPGroup group;

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
};

struct _SPVPGroupClass {
	SPGroupClass parent_class;
};

GType sp_vpgroup_get_type (void);

#endif
