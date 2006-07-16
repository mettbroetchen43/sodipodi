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

#include <libarikkei/arikkei-strlib.h>

#include <glib.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef MAX
#define MAX(a,b) (((a) < (b)) ? (b) : (a))
#endif

#include "repr-private.h"
#include "repr.h"

static void sp_xml_ns_register_defaults ();
static char *sp_xml_ns_auto_prefix (const char *uri);

/* SPXMLNs */

static SPXMLNs *namespaces=NULL;

static void
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

static char *
sp_xml_ns_auto_prefix (const char *uri)
{
	const char *start, *end;
	char *new_prefix;
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
		char *temp;
		int counter=0;
		do {
			temp = g_strdup_printf ("%s%d", new_prefix, counter++);
		} while (sp_xml_ns_prefix_uri (temp));
		g_free (new_prefix);
		new_prefix = temp;
	}
	return new_prefix;
}

const unsigned char *
sp_xml_ns_uri_prefix (const unsigned char *uri, const unsigned char *suggested)
{
	unsigned int key;
	SPXMLNs *iter;
	const char *prefix;

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
		const char *new_prefix;
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
			g_free ((char *)new_prefix);
		}
	}
	return prefix;
}

const unsigned char *
sp_xml_ns_prefix_uri (const unsigned char *prefix)
{
	unsigned int key;
	SPXMLNs *iter;
	const char *uri;

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
sp_xml_document_createTextNode (SPXMLDocument *doc, const unsigned char *data)
{
	SPXMLText *text;

	text = sp_repr_new ("text");
	text->type = SP_XML_TEXT_NODE;
	sp_repr_set_content (text, data);

	return text;
}

SPXMLElement *
sp_xml_document_createElement (SPXMLDocument *doc, const unsigned char *name)
{
	return sp_repr_new (name);
}

SPXMLElement *
sp_xml_document_createElementNS (SPXMLDocument *doc, const unsigned char *ns, const unsigned char *qname)
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
sp_xml_element_setAttributeNS (SPXMLElement *element, const unsigned char *nr, const unsigned char *qname, const unsigned char *val)
{
	if (!strncmp (qname, "svg:", 4)) qname += 4;

	/* fixme: return value (Exception?) */
	sp_repr_set_attr (element, qname, val);
}

SPRepr *
sp_repr_get_children (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return repr->children;
}

SPRepr *
sp_repr_get_next (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return repr->next;
}

int sp_repr_attr_is_set (SPRepr * repr, const char * key)
{
	char * result;

	result = (char *) sp_repr_get_attr (repr, key);

	return (result != NULL);
}

SPReprAttr *
sp_repr_attr_get_first (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return repr->attributes;
}

SPReprAttr *
sp_repr_attr_get_next (SPRepr *repr, SPReprAttr *ref)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return (ref) ? ref->next : repr->attributes;
}

const unsigned char *
sp_repr_attr_get_key (SPRepr *repr, SPReprAttr *attr)
{
	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return SP_REPR_ATTRIBUTE_KEY (attr);
}

const unsigned char *
sp_repr_attr_get_value (SPRepr *repr, SPReprAttr *attr)
{
	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (attr != NULL, NULL);

	return SP_REPR_ATTRIBUTE_VALUE (attr);
}

SPReprXmlSpaceType
sp_repr_get_xml_space (SPRepr *repr)
{
	const unsigned char *space;

	space = sp_repr_attr_inherited (repr, "xml:space");

	if (space && !strcmp(space, "preserve"))
		return SP_REPR_XML_SPACE_PRESERVE;

	return SP_REPR_XML_SPACE_DEFAULT;
}

void
sp_repr_set_xml_space (SPRepr *repr, SPReprXmlSpaceType space)
{
	const char *space_str;
	switch (space) {
	case SP_REPR_XML_SPACE_PRESERVE:
		space_str = "preserve";
		break;
	case SP_REPR_XML_SPACE_DEFAULT:
		space_str = "default";
		break;
	case SP_REPR_XML_SPACE_NONE:
	default:
		space_str = NULL;
		break;
	}

	sp_repr_set_attr (repr, "xml:space", space_str);
}

const char *
sp_repr_doc_attr (SPRepr * repr, const char * key)
{
	SPRepr * p;

	p = sp_repr_get_parent (repr);

	while (p != NULL) {
		repr = p;
		p = sp_repr_get_parent (p);
	}

	return sp_repr_get_attr (repr, key);
}

int
sp_repr_compare_position (SPRepr * first, SPRepr * second)
{
	SPRepr * parent;
	int p1, p2;

	parent = sp_repr_get_parent (first);
	g_assert (parent == sp_repr_get_parent (second));

	p1 = sp_repr_position (first);
	p2 = sp_repr_position (second);

	if (p1 > p2) return 1;
	if (p1 < p2) return -1;
	return 0;
}

int
sp_repr_position (SPRepr * repr)
{
	SPRepr * parent;
	SPRepr * sibling;
	int pos;

	g_assert (repr != NULL);
	parent = sp_repr_get_parent (repr);
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
sp_repr_set_position_relative (SPRepr * repr, int pos)
{
	int cpos;

	g_assert (repr != NULL);

	cpos = sp_repr_position (repr);
	sp_repr_set_position_absolute (repr, cpos + pos);
}

int
sp_repr_n_children (SPRepr * repr)
{
	SPRepr * child;
	int n;

	g_assert (repr != NULL);

	n = 0;
	for (child = repr->children; child != NULL; child = child->next) n++;

	return n;
}

unsigned int
sp_repr_append_child (SPRepr * repr, SPRepr * child)
{
	SPRepr * ref;

	g_return_val_if_fail (repr != NULL, 0);
	g_return_val_if_fail (child != NULL, 0);

	ref = NULL;
	if (repr->children) {
		ref = repr->children;
		while (ref->next) ref = ref->next;
	}

	return sp_repr_add_child (repr, child, ref);
}

unsigned int
sp_repr_unparent (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, 0);
	g_return_val_if_fail (repr->parent != NULL, 0);

	return sp_repr_remove_child (repr->parent, repr);
}

SPRepr * sp_repr_duplicate_and_parent (SPRepr * repr)
{
	SPRepr * parent, * new;

	g_assert (repr != NULL);

	parent = sp_repr_get_parent (repr);
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

const unsigned char *
sp_repr_attr_inherited (SPRepr *repr, const unsigned char *key)
{
	SPRepr *current;
	const char *val;

	g_assert (repr != NULL);
	g_assert (key != NULL);

	for (current = repr; current != NULL; current = sp_repr_get_parent (current)) {
		val = sp_repr_get_attr (current, key);
		if (val != NULL)
			return val;
	}
	return NULL;
}

unsigned int
sp_repr_set_attr_recursive (SPRepr *repr, const unsigned char *key, const unsigned char *value)
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
sp_repr_lookup_child (SPRepr *repr, const unsigned char *key, const unsigned char *value)
{
	SPRepr *child;
	SPReprAttr *attr;
	unsigned int quark;

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

SPRepr *
sp_repr_lookup_child_by_name (SPRepr *repr, const unsigned char *name)
{
	SPRepr *child;
	unsigned int quark;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	quark = g_quark_from_string (name);

	for (child = repr->children; child != NULL; child = child->next) {
		if (child->name == quark) return child;
	}

	return NULL;
}

/* Convenience */
unsigned int
sp_repr_get_boolean (SPRepr *repr, const unsigned char *key, unsigned int *val)
{
	const unsigned char *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_get_attr (repr, key);

	if (v != NULL) {
		if (!g_strcasecmp (v, "true") ||
		    !g_strcasecmp (v, "yes") ||
		    (atoi (v) != 0)) {
			*val = TRUE;
		} else {
			*val = FALSE;
		}
		return TRUE;
	}

	return FALSE;
}

unsigned int
sp_repr_get_int (SPRepr *repr, const unsigned char *key, int *val)
{
	const unsigned char *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_get_attr (repr, key);

	if (v != NULL) {
		*val = atoi (v);
		return TRUE;
	}

	return FALSE;
}

unsigned int
sp_repr_get_double (SPRepr *repr, const unsigned char *key, double *val)
{
	const unsigned char *v;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (val != NULL, FALSE);

	v = sp_repr_get_attr (repr, key);

	if (v != NULL) {
		return arikkei_strtod_exp (v, 256, val);
	}

	return FALSE;
}

unsigned int
sp_repr_set_boolean (SPRepr *repr, const unsigned char *key, unsigned int val)
{
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	return sp_repr_set_attr (repr, key, (val) ? "true" : "false");
}

unsigned int
sp_repr_set_int (SPRepr *repr, const unsigned char *key, int val)
{
	unsigned char c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	g_snprintf (c, 32, "%d", val);

	return sp_repr_set_attr (repr, key, c);
}

unsigned int
sp_repr_set_double (SPRepr *repr, const unsigned char *key, double val)
{
	unsigned char c[32];

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	arikkei_dtoa_exp (c, 256, val, 8, FALSE);

	return sp_repr_set_attr (repr, key, c);
}

unsigned int
sp_repr_set_double_default (SPRepr *repr, const unsigned char *key, double val, double def, double e)
{
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	if (fabs (val - def) <= e) {
		return sp_repr_set_attr (repr, key, NULL);
	} else {
		return sp_repr_set_double (repr, key, val);
	}
}

