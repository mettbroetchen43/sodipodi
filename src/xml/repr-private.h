#ifndef SP_REPR_PRIVATE_H
#define SP_REPR_PRIVATE_H

/*
 * TODO: Hash tables are overkill for attributes
 */

#include <gtk/gtkobject.h>
#include "repr.h"

struct _SPRepr {
	GtkObject object;
	SPRepr * parent;
	GQuark name;
	gchar * content;
	GHashTable * attr;
	GList * children;
};

struct _SPReprClass {
	GtkObjectClass object_class;
	void (* child_added) (SPRepr * repr, SPRepr * child);
	void (* remove_child) (SPRepr * repr, SPRepr * child);
	gint (* change_attr) (SPRepr * repr, const gchar * key, const gchar * value);
	void (* attr_changed) (SPRepr * repr, const gchar * key);
	gint (* change_content) (SPRepr * repr, const gchar * content);
	void (* content_changed) (SPRepr * repr);
	gint (* change_order) (SPRepr * repr, gint order);
	void (* order_changed) (SPRepr * repr);
};


#endif
