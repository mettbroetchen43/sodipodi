#define __SP_REPR_UTIL_C__

/*
 * Miscellaneous helpers for reprs
 *
 * Authors:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "repr-private.h"

gint sp_repr_attr_is_set (SPRepr * repr, const gchar * key)
{
	gchar * result;

	result = (gchar *) sp_repr_attr (repr, key);

	return (result != NULL);
}

gdouble sp_repr_get_double_attribute (SPRepr * repr, const gchar * key, gdouble def)
{
	gchar * result;

	g_return_val_if_fail (repr != NULL, def);
	g_return_val_if_fail (key != NULL, def);

	result = (gchar *) sp_repr_attr (repr, key);

	if (result == NULL) return def;

	return atof (result);
}

gint sp_repr_get_int_attribute (SPRepr * repr, const gchar * key, gint def)
{
	gchar * result;

	g_return_val_if_fail (repr != NULL, def);
	g_return_val_if_fail (key != NULL, def);

	result = (gchar *) sp_repr_attr (repr, key);

	if (result == NULL) return def;

	return atoi (result);
}

gboolean
sp_repr_set_double_attribute (SPRepr * repr, const gchar * key, gdouble value)
{
	gchar c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	g_snprintf (c, 32, "%f", value);

	return sp_repr_set_attr (repr, key, c);
}

gboolean
sp_repr_set_int_attribute (SPRepr * repr, const gchar * key, gint value)
{
	gchar c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	g_snprintf (c, 32, "%d", value);

	return sp_repr_set_attr (repr, key, c);
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
	gint p1, p2;

	parent = sp_repr_parent (first);
	g_assert (parent == sp_repr_parent (second));

	p1 = sp_repr_position (first);
	p2 = sp_repr_position (second);

	if (p1 > p2) return 1;
	if (p1 < p2) return -1;
	return 0;
}

gint
sp_repr_position (SPRepr * repr)
{
	SPRepr * parent;
	SPRepr * sibling;
	gint pos;

	g_assert (repr != NULL);
	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	pos = 0;
	for (sibling = parent->children; sibling != NULL; sibling = sibling->next) {
		if (repr == sibling) return pos;
		pos += 1;
	}
	
	g_assert_not_reached ();

	return -1;
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
	SPRepr * child;
	gint n;

	g_assert (repr != NULL);

	n = 0;
	for (child = repr->children; child != NULL; child = child->next) n++;

	return n;
}

void
sp_repr_append_child (SPRepr * repr, SPRepr * child)
{
	SPRepr * ref;

	g_assert (repr != NULL);
	g_assert (child != NULL);

	ref = NULL;
	if (repr->children) {
		ref = repr->children;
		while (ref->next) ref = ref->next;
	}

	sp_repr_add_child (repr, child, ref);
}

void sp_repr_unparent (SPRepr * repr)
{
	SPRepr * parent;

	g_assert (repr != NULL);

	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	sp_repr_remove_child (parent, repr);
}

SPRepr * sp_repr_duplicate_and_parent (SPRepr * repr)
{
	SPRepr * parent, * new;

	g_assert (repr != NULL);

	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);

	new = sp_repr_duplicate (repr);
	sp_repr_append_child (parent, new);
	sp_repr_unref (new);

	return new;
}

void
sp_repr_remove_signals (SPRepr * repr)
{
	g_assert (repr != NULL);

	g_warning ("need to remove signal handlers by hand");
#if 0
	sp_repr_set_signal (repr, "destroy", NULL, NULL);
	sp_repr_set_signal (repr, "child_added", NULL, NULL);
	sp_repr_set_signal (repr, "child_removed", NULL, NULL);
	sp_repr_set_signal (repr, "attr_changed_pre", NULL, NULL);
	sp_repr_set_signal (repr, "attr_changed", NULL, NULL);
	sp_repr_set_signal (repr, "content_changed_pre", NULL, NULL);
	sp_repr_set_signal (repr, "content_changed", NULL, NULL);
	sp_repr_set_signal (repr, "order_changed", NULL, NULL);
#endif
}

const guchar *
sp_repr_attr_inherited (SPRepr *repr, const guchar *key)
{
	SPRepr *current;
	const gchar *val;

	g_assert (repr != NULL);
	g_assert (key != NULL);

	for (current = repr; current != NULL; current = sp_repr_parent (current)) {
		val = sp_repr_attr (current, key);
		if (val != NULL)
			return val;
	}
	return NULL;
}

gboolean
sp_repr_set_attr_recursive (SPRepr *repr, const guchar *key, const guchar *value)
{
	SPRepr *child;

	if (!sp_repr_set_attr (repr, key, value)) return FALSE;

	for (child = repr->children; child != NULL; child = child->next) {
		sp_repr_set_attr (child, key, NULL);
	}

	return TRUE;
}

/**
 * sp_repr_lookup_child:
 * @repr: 
 * @key: 
 * @value: 
 *
 * lookup child by (@key, @value)
 */
SPRepr *
sp_repr_lookup_child (SPRepr       *repr,
                      const guchar *key,
                      const guchar *value)
{
	SPRepr *child;
	SPReprAttr *attr;
	GQuark quark;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (value != NULL, NULL);

	quark = g_quark_from_string (key);

	/* Fixme: we should use hash table for faster lookup? */
	
	for (child = repr->children; child != NULL; child = child->next) {
		for (attr = child->attributes; attr != NULL; attr = attr->next) {
			if ((attr->key == quark) && !strcmp (attr->value, value)) return child;
		}
	}

	return NULL;
}

/* Convenience */
gboolean
sp_repr_get_boolean (SPRepr *repr, const guchar *key, gboolean *val)
{
	const guchar *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_attr (repr, key);

	if (v != NULL) {
		if (!strcasecmp (v, "true") ||
		    !strcasecmp (v, "yes") ||
		    !strcasecmp (v, "y") ||
		    (atoi (v) != 0)) {
			*val = TRUE;
		} else {
			*val = FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

gboolean
sp_repr_get_int (SPRepr *repr, const guchar *key, gint *val)
{
	const guchar *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_attr (repr, key);

	if (v != NULL) {
		*val = atoi (v);
		return TRUE;
	}

	return FALSE;
}

gboolean
sp_repr_get_double (SPRepr *repr, const guchar *key, gdouble *val)
{
	const guchar *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_attr (repr, key);

	if (v != NULL) {
		*val = atof (v);
		return TRUE;
	}

	return FALSE;
}

gboolean
sp_repr_set_boolean (SPRepr *repr, const guchar *key, gboolean val)
{
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	return sp_repr_set_attr (repr, key, (val) ? "true" : "false");
}

gboolean
sp_repr_set_int (SPRepr *repr, const guchar *key, gint val)
{
	guchar c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	g_snprintf (c, 32, "%d", val);

	return sp_repr_set_attr (repr, key, c);
}

gboolean
sp_repr_set_double (SPRepr *repr, const guchar *key, gdouble val)
{
	guchar c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	g_snprintf (c, 32, "%g", val);

	return sp_repr_set_attr (repr, key, c);
}

