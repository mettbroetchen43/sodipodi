#ifndef SP_ITEM_GROUP_H
#define SP_ITEM_GROUP_H

/*
 * SPItemGroup
 *
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


/* Standard Gtk function */

GtkType sp_group_get_type (void);

void sp_item_group_ungroup (SPGroup *group);

GSList * sp_item_group_item_list (SPGroup * group);

#endif
