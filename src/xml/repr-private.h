#ifndef __SP_REPR_PRIVATE_H__
#define __SP_REPR_PRIVATE_H__

/*
 * Fuzzy DOM-like tree implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "repr.h"

typedef struct _SPReprAttr SPReprAttr;
typedef struct _SPReprListener SPReprListener;
typedef struct _SPReprEventVector SPReprEventVector;

typedef enum {
	SP_XML_INVALID_NODE,
	SP_XML_ELEMENT_NODE,
	SP_XML_TEXT_NODE
} SPXMLNodeType;

struct _SPReprAttr {
	SPReprAttr *next;
	gint key;
	guchar *value;
};

struct _SPReprListener {
	SPReprListener *next;
	const SPReprEventVector *vector;
	gpointer data;
};

struct _SPReprEventVector {
	/* Immediate signals */
	void (* destroy) (SPRepr *repr, gpointer data);
	gboolean (* add_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
	void (* child_added) (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
	gboolean (* remove_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
	void (* child_removed) (SPRepr *repr, SPRepr *child, SPRepr *ref, gpointer data);
	gboolean (* change_attr) (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);
	void (* attr_changed) (SPRepr *repr, const guchar *key, const guchar *oldval, const guchar *newval, gpointer data);
	gboolean (* change_content) (SPRepr *repr, const guchar *oldcontent, const guchar *newcontent, gpointer data);
	void (* content_changed) (SPRepr *repr, const guchar *oldcontent, const guchar *newcontent, gpointer data);
	gboolean (* change_order) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, gpointer data);
	void (* order_changed) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, gpointer data);
};

struct _SPRepr {
	gint refcount;
	gint name;
	gint type;
	SPRepr *parent;
	SPRepr *next;
	SPRepr *children;
	SPReprAttr *attributes;
	SPReprListener *listeners;
	guchar *content;
};

#define SP_REPR_NAME(r) g_quark_to_string ((r)->name)
#define SP_REPR_TYPE(r) ((r)->type)
#define SP_REPR_CONTENT(r) ((r)->content)
#define SP_REPR_ATTRIBUTE_KEY(a) g_quark_to_string ((a)->key)
#define SP_REPR_ATTRIBUTE_VALUE(a) ((a)->value)

SPRepr *sp_repr_nth_child (const SPRepr *repr, gint n);

gboolean sp_repr_change_order (SPRepr *repr, SPRepr *child, SPRepr *ref);

void sp_repr_synthesize_events (SPRepr *repr, const SPReprEventVector *vector, gpointer data);

void sp_repr_add_listener (SPRepr *repr, const SPReprEventVector *vector, gpointer data);
void sp_repr_remove_listener_by_data (SPRepr *repr, gpointer data);

void sp_repr_document_set_root (SPReprDoc *doc, SPRepr *repr);


#endif
