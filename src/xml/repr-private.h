#ifndef SP_REPR_PRIVATE_H
#define SP_REPR_PRIVATE_H

#include "repr.h"

/* fixme: move callbacks to separate structure, so we can share it */

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
	gint (* attr_changed_pre)(SPRepr * repr, const gchar * key, const gchar * value, gpointer);
	gpointer attr_changed_pre_data;
	void (* attr_changed)(SPRepr *, const gchar *, gpointer);
	gpointer attr_changed_data;
	gint (* content_changed_pre)(SPRepr * repr, const gchar * content, gpointer data);
	gpointer content_changed_pre_data;
	void (* content_changed)(SPRepr *, gpointer);
	gpointer content_changed_data;
	gint (* order_changed_pre)(SPRepr * repr, gint order, gpointer data);
	gpointer order_changed_pre_data;
	void (* order_changed)(SPRepr *, gpointer);
	gpointer order_changed_data;
};

/* Returns list of attribute strings
 * List should be freed by caller, but attributes not */

GList * sp_repr_attributes (SPRepr * repr);

#endif
