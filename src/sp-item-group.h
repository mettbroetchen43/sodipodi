#ifndef __SP_ITEM_GROUP_H__
#define __SP_ITEM_GROUP_H__

/*
 * SPItemGroup
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include "sp-item.h"

#define SP_TYPE_GROUP            (sp_group_get_type ())
#define SP_GROUP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GROUP, SPGroup))
#define SP_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GROUP, SPGroupClass))
#define SP_IS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GROUP))
#define SP_IS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GROUP))

struct _SPGroup {
	SPItem item;
	SPObject * children;
	gboolean transparent;
};

struct _SPGroupClass {
	SPItemClass parent_class;
};

GtkType sp_group_get_type (void);

void sp_item_group_ungroup (SPGroup *group, GSList **children);

GSList *sp_item_group_item_list (SPGroup * group);

#endif
