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

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "repr-private.h"

static void sp_repr_transfer_ids (SPRepr *new_repr, SPRepr *repr);
static void sp_xml_ns_register_defaults ();
static gchar *sp_xml_ns_auto_prefix (const gchar *uri);

/* SPXMLNs */

static SPXMLNs *namespaces=NULL;

void
sp_xml_ns_register_defaults ()
{
	static SPXMLNs defaults[3];
	defaults[0].uri = g_quark_from_static_string (SP_SODIPODI_NS_URI);
	defaults[0].prefix = g_quark_from_static_string ("sodipodi");
	defaults[0].next = &defaults[1];
	defaults[1].uri = g_quark_from_static_string (SP_XLINK_NS_URI);
	defaults[1].prefix = g_quark_from_static_string ("xlink");
	defaults[1].next = &defaults[2];
	defaults[2].uri = g_quark_from_static_string (SP_SVG_NS_URI);
	defaults[2].prefix = g_quark_from_static_string ("svg");
	defaults[2].next = NULL;
	namespaces = &defaults[0];
}

gchar *
sp_xml_ns_auto_prefix (const gchar *uri)
{
	const gchar *start, *end;
	gchar *new_prefix;
	start = uri;
	while ((end = strpbrk (start, ":/"))) {
		start = end + 1;
	}
	end = start + strspn (start, "abcdefghijklmnopqrstuvwxyz");
	if (end == start) {
		start = "ns";
		end = start + 2;
	}
	new_prefix = g_strndup (start, end - start);
	if (sp_xml_ns_prefix_uri (new_prefix)) {
		gchar *temp;
		int counter=0;
		do {
			temp = g_strdup_printf ("%s%d", new_prefix, counter++);
		} while (sp_xml_ns_prefix_uri (temp));
		g_free (new_prefix);
		new_prefix = temp;
	}
	return new_prefix;
}

const gchar *
sp_xml_ns_uri_prefix (const gchar *uri, const gchar *suggested)
{
	GQuark key;
	SPXMLNs *iter;
	const gchar *prefix;

	if (!uri) return NULL;

	if (!namespaces) {
		sp_xml_ns_register_defaults ();
	}

	key = g_quark_from_string (uri);
	prefix = NULL;
	for ( iter = namespaces ; iter ; iter = iter->next ) {
		if ( iter->uri == key ) {
			prefix = g_quark_to_string (iter->prefix);
			break;
		}
	}
	if (!prefix) {
		const gchar *new_prefix;
		SPXMLNs *ns;
		if (suggested) {
			new_prefix = suggested;
		} else {
			new_prefix = sp_xml_ns_auto_prefix (uri);
		}
		ns = g_new (SPXMLNs, 1);
		if (ns) {
			ns->uri = g_quark_from_string (uri);
			ns->prefix = g_quark_from_string (new_prefix);
			ns->next = namespaces;
			namespaces = ns;
			prefix = g_quark_to_string (ns->prefix);
		}
		if (!suggested) {
			g_free ((gchar *)new_prefix);
		}
	}
	return prefix;
}

const gchar *
sp_xml_ns_prefix_uri (const gchar *prefix)
{
	GQuark key;
	SPXMLNs *iter;
	const gchar *uri;

	if (!prefix) return NULL;

	if (!namespaces) {
		sp_xml_ns_register_defaults ();
	}

	key = g_quark_from_string (prefix);
	uri = NULL;
	for ( iter = namespaces ; iter ; iter = iter->next ) {
		if ( iter->prefix == key ) {
			uri = g_quark_to_string (iter->uri);
			break;
		}
	}
	return uri;
}

/* SPXMLDocument */

SPXMLText *
sp_xml_document_createTextNode (SPXMLDocument *doc, const guchar *data)
{
	SPXMLText *text;

	text = sp_repr_new ("text");
	text->type = SP_XML_TEXT_NODE;
	sp_repr_set_content (text, data);

	return text;
}

SPXMLElement *
sp_xml_document_createElement (SPXMLDocument *doc, const guchar *name)
{
	return sp_repr_new (name);
}

SPXMLElement *
sp_xml_document_createElementNS (SPXMLDocument *doc, const guchar *ns, const guchar *qname)
{
	if (!strncmp (qname, "svg:", 4)) qname += 4;

	return sp_repr_new (qname);
}

/* SPXMLNode */

SPXMLDocument *
sp_xml_node_get_Document (SPXMLNode *node)
{
	g_warning ("sp_xml_node_get_Document: unimplemented");

	return NULL;
}

/* SPXMLElement */

void
sp_xml_element_setAttributeNS (SPXMLElement *element, const guchar *nr, const guchar *qname, const guchar *val)
{
	if (!strncmp (qname, "svg:", 4)) qname += 4;

	/* fixme: return value (Exception?) */
	sp_repr_set_attr (element, qname, val);
}

SPRepr *
sp_repr_children (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return repr->children;
}

SPRepr *
sp_repr_next (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return repr->children;
}

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
#ifndef WIN32
		if (!strcasecmp (v, "true") ||
		    !strcasecmp (v, "yes") ||
		    !strcasecmp (v, "y") ||
		    (atoi (v) != 0)) {
			*val = TRUE;
		} else {
			*val = FALSE;
		}
#else
		if (!stricmp (v, "true") ||
		    !stricmp (v, "yes") ||
		    !stricmp (v, "y") ||
		    (atoi (v) != 0)) {
			*val = TRUE;
		} else {
			*val = FALSE;
		}
#endif
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

unsigned int
sp_xml_dtoa (unsigned char *buf, double val, unsigned int tprec, unsigned int fprec, unsigned int padf)
{
	double dival, fval;
	int ival, i;
	i = 0;
	if (val < 0.0) {
		buf[i++] = '-';
		val = fabs (val);
	}
	/* Extract integral and fractional parts */
	dival = floor (val);
	ival = (int) dival;
	fval = val - dival;
	/* Write integra */
	if (ival > 0) {
		char c[32];
		int j;
		j = 0;
		while (ival > 0) {
			c[32 - (++j)] = '0' + (ival % 10);
			ival /= 10;
		}
		memcpy (buf + i, &c[32 - j], j);
		i += j;
		tprec -= j;
	} else {
		buf[i++] = '0';
		tprec -= 1;
	}
	fprec = MAX (tprec, fprec);
	if ((fprec > 0) && (padf || (fval > 0.0))) {
		fval += 0.5 * pow (10.0, -((double) fprec));
		buf[i++] = '.';
		while ((fprec > 0) && (padf || (fval > 0.0))) {
			fval *= 10.0;
			dival = floor (fval);
			fval -= dival;
			buf[i++] = '0' + (int) dival;
			fprec -= 1;
		}

	}
	buf[i] = 0;
	return i;
}

gboolean
sp_repr_set_double (SPRepr *repr, const guchar *key, gdouble val)
{
	unsigned char c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	sp_xml_dtoa (c, val, 8, 0, FALSE);

	return sp_repr_set_attr (repr, key, c);
}

gboolean
sp_repr_set_double_default (SPRepr *repr, const guchar *key, gdouble val, gdouble def, gdouble e)
{
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	if (fabs (val - def) <= e) {
		return sp_repr_set_attr (repr, key, NULL);
	} else {
		return sp_repr_set_double (repr, key, val);
	}
}

void
sp_repr_transfer_ids (SPRepr *new_repr, SPRepr *repr)
{
	SPRepr *new_child, *child;

	for ( new_child = new_repr->children, child = repr->children ;
	      new_child && child ;
	      new_child = new_child->next, child = child->next )
	{
		sp_repr_transfer_ids (new_child, child);
	}

	sp_repr_set_attr (new_repr, "id", sp_repr_attr (repr, "id"));
}

SPRepr *
sp_repr_move (SPRepr *to, SPRepr *repr, SPRepr *ref)
{
	SPRepr * new_repr;

	g_assert (repr != NULL);

	if ( to == repr->parent ) {
		if (sp_repr_change_order(repr->parent, repr, ref)) {
			return repr;
		} else {
			return NULL;
		}
	} else if ( to == NULL ) {
		if (sp_repr_remove_child(repr->parent, repr)) {
			return repr;
		} else {
			return NULL;
		}
	}

	new_repr = sp_repr_duplicate (repr);
	g_return_val_if_fail (new_repr != NULL, NULL);

	if (!sp_repr_add_child(to, new_repr, ref)) {
		sp_repr_unref (new_repr);
		return NULL;
	}
	sp_repr_unref (new_repr);

	if ( repr->parent != NULL ) {
		sp_repr_ref (repr);
		if (!sp_repr_remove_child (repr->parent, repr)) {
			sp_repr_remove_child (to, new_repr);
			sp_repr_unref (repr);
			return NULL;
		}
		sp_repr_transfer_ids (new_repr, repr);
		sp_repr_unref (repr);
	} else {
		sp_repr_transfer_ids (new_repr, repr);
	}

	g_assert ( new_repr->refcount > 0 );
	return new_repr;
}

