#ifndef SP_OBJECTGROUP_H
#define SP_OBJECTGROUP_H

/*
 * SPObjectGroup
 *
 * Abstract base class for SPObjects with multiple children, who are not
 * SPItems themselves - such as NamedViews
 *
 * This should be kept in sync with SPGroup
 *
 */

#define SP_TYPE_OBJECTGROUP            (sp_objectgroup_get_type ())
#define SP_OBJECTGROUP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_OBJECTGROUP, SPObjectGroup))
#define SP_OBJECTGROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECTGROUP, SPObjectGroupClass))
#define SP_IS_OBJECTGROUP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_OBJECTGROUP))
#define SP_IS_OBJECTGROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECTGROUP))

#include "sp-object.h"

struct _SPObjectGroup {
	SPObject object;
	SPObject * children;
	gboolean transparent;
};

struct _SPObjectGroupClass {
	SPObjectClass parent_class;
};

GtkType sp_objectgroup_get_type (void);

#endif
