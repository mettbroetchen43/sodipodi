#define SP_REPR_UTIL_C

#include <stdlib.h>
#include <stdio.h>
#include "repr.h"

SPRepr *
sp_repr_new_with_name (const gchar * name)
{
	SPRepr * repr;

	repr = sp_repr_new ();

	sp_repr_set_name (repr, name);

	return repr;
}

gint sp_repr_attr_is_set (SPRepr * repr, const gchar * key)
{
	gchar * result;

	result = (gchar *) sp_repr_attr (repr, key);

	return (result != NULL);
}

double sp_repr_get_double_attribute (SPRepr * repr, const gchar * key, double def)
{
	gchar * result;

	g_return_val_if_fail (repr != NULL, def);
	g_return_val_if_fail (key != NULL, def);

	result = (gchar *) sp_repr_attr (repr, key);

	if (result == NULL)
		return def;

	return atof (result);
}

void
sp_repr_set_double_attribute (SPRepr * repr, const gchar * key, double value)
{
	gchar c[128];

	g_return_if_fail (repr != NULL);
	g_return_if_fail (key != NULL);

	snprintf (c, 128, "%f", value);

	sp_repr_set_attr (repr, key, c);

	return;
}

const gchar *
sp_repr_doc_attr (SPRepr * repr, const gchar * key)
{
	SPRepr * p;

	p = sp_repr_parent (repr);

	while (p != NULL) {
		repr = p;
		p = sp_repr_parent (p);
	}

	return sp_repr_attr (repr, key);
}

gint
sp_repr_compare_position (SPRepr * first, SPRepr * second)
{
	SPRepr * parent;
	const GList * siblings;
	gint p1, p2;

	parent = sp_repr_parent (first);
	g_assert (parent == sp_repr_parent (second));

	siblings = sp_repr_children (parent);

	p1 = g_list_index ((GList *) siblings, first);
	p2 = g_list_index ((GList *) siblings, second);

	if (p1 > p2) return 1;
	if (p1 < p2) return -1;
	return 0;
}

gint
sp_repr_position (SPRepr * repr)
{
	SPRepr * parent;
	const GList * siblings;

	g_assert (repr != NULL);
	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	siblings = sp_repr_children (parent);

	return g_list_index ((GList *) siblings, repr);
}

void
sp_repr_set_position_absolute (SPRepr * repr, gint pos)
{
	SPRepr * parent;
	gint n_sib;

	g_assert (repr != NULL);
	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	n_sib = sp_repr_n_children (parent);
	if ((pos < 0) || (pos > (n_sib - 1)))
		pos = n_sib - 1;

	sp_repr_ref (repr);
	sp_repr_remove_child (parent, repr);
	sp_repr_add_child (parent, repr, pos);
	sp_repr_unref (repr);
}

void
sp_repr_set_position_relative (SPRepr * repr, gint pos)
{
	gint cpos;

	g_assert (repr != NULL);

	cpos = sp_repr_position (repr);
	sp_repr_set_position_absolute (repr, cpos + pos);
}

gint
sp_repr_n_children (SPRepr * repr)
{
	const GList * children;

	g_assert (repr != NULL);

	children = sp_repr_children (repr);

	return g_list_length ((GList *) children);
}

void
sp_repr_append_child (SPRepr * repr, SPRepr * child)
{
	g_assert (repr != NULL);
	g_assert (child != NULL);

	sp_repr_add_child (repr, child, sp_repr_n_children (repr));
}

void sp_repr_unparent_and_destroy (SPRepr * repr)
{
	SPRepr * parent;

	g_assert (repr != NULL);

	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	sp_repr_remove_child (parent, repr);
#if 0
	sp_repr_unref (repr);
#endif
}

SPRepr * sp_repr_duplicate_and_parent (SPRepr * repr)
{
	SPRepr * parent, * new;

	g_assert (repr != NULL);

	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	new = sp_repr_copy (repr);
	sp_repr_append_child (parent, new);
	sp_repr_unref (new);

	return new;
}

void
sp_repr_remove_signals (SPRepr * repr)
{
	g_assert (repr != NULL);

	sp_repr_set_signal (repr, "destroy", NULL, NULL);
	sp_repr_set_signal (repr, "child_added", NULL, NULL);
	sp_repr_set_signal (repr, "unparented", NULL, NULL);
	sp_repr_set_signal (repr, "attr_changed", NULL, NULL);
	sp_repr_set_signal (repr, "content_changed", NULL, NULL);
}

const gchar *
sp_repr_attr_inherited (SPRepr * repr, const gchar * key)
{
	SPRepr * current;
	const gchar * val;

	g_assert (repr != NULL);
	g_assert (key != NULL);

	for (current = repr; current != NULL; current = sp_repr_parent (current)) {
		val = sp_repr_attr (current, key);
		if (val != NULL)
			return val;
	}
	return NULL;
}

