#ifndef SP_REPR_PRIVATE_H
#define SP_REPR_PRIVATE_H

#include "repr.h"

struct _SPRepr {
	gint ref_count;
	SPRepr * parent;
	GQuark name;
	gchar * content;
	GHashTable * attr;
	GList * children;
	gpointer data;
	void (* destroy) (SPRepr *, gpointer);
	gpointer destroy_data;
	void (* child_added)(SPRepr *, SPRepr *, gpointer);
	gpointer child_added_data;
	void (* child_removed)(SPRepr *, SPRepr *, gpointer);
	gpointer child_removed_data;
	void (* attr_changed)(SPRepr *, const gchar *, gpointer);
	gpointer attr_changed_data;
	void (* content_changed)(SPRepr *, gpointer);
	gpointer content_changed_data;
	void (* order_changed)(SPRepr *, gpointer);
	gpointer order_changed_data;
};

/* Returns list of attribute strings
 * List should be freed by caller, but attributes not */

GList * sp_repr_attributes (SPRepr * repr);

#endif
