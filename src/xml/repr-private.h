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
	int key;
	unsigned char *value;
};

struct _SPReprListener {
	SPReprListener *next;
	const SPReprEventVector *vector;
	void * data;
};

struct _SPReprEventVector {
	/* Immediate signals */
	void (* destroy) (SPRepr *repr, void * data);
	unsigned int (* add_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, void * data);
	void (* child_added) (SPRepr *repr, SPRepr *child, SPRepr *ref, void * data);
	unsigned int (* remove_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, void * data);
	void (* child_removed) (SPRepr *repr, SPRepr *child, SPRepr *ref, void * data);
	unsigned int (* change_attr) (SPRepr *repr, const unsigned char *key, const unsigned char *oldval, const unsigned char *newval, void * data);
	void (* attr_changed) (SPRepr *repr, const unsigned char *key, const unsigned char *oldval, const unsigned char *newval, void * data);
	unsigned int (* change_content) (SPRepr *repr, const unsigned char *oldcontent, const unsigned char *newcontent, void * data);
	void (* content_changed) (SPRepr *repr, const unsigned char *oldcontent, const unsigned char *newcontent, void * data);
	unsigned int (* change_order) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, void * data);
	void (* order_changed) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, void * data);
};

struct _SPRepr {
	int refcount;
	int name;
	int type;
	SPRepr *parent;
	SPRepr *next;
	SPRepr *children;
	SPReprAttr *attributes;
	SPReprListener *listeners;
	unsigned char *content;
};

struct _SPXMLNs {
	SPXMLNs *next;
	unsigned int uri;
	unsigned int prefix;
};

#define SP_REPR_NAME(r) g_quark_to_string ((r)->name)
#define SP_REPR_TYPE(r) ((r)->type)
#define SP_REPR_CONTENT(r) ((r)->content)
#define SP_REPR_ATTRIBUTE_KEY(a) g_quark_to_string ((a)->key)
#define SP_REPR_ATTRIBUTE_VALUE(a) ((a)->value)

SPRepr *sp_repr_nth_child (const SPRepr *repr, int n);

unsigned int sp_repr_change_order (SPRepr *repr, SPRepr *child, SPRepr *ref);

void sp_repr_synthesize_events (SPRepr *repr, const SPReprEventVector *vector, void * data);

void sp_repr_add_listener (SPRepr *repr, const SPReprEventVector *vector, void * data);
void sp_repr_remove_listener_by_data (SPRepr *repr, void * data);

void sp_repr_document_set_root (SPReprDoc *doc, SPRepr *repr);


#endif
