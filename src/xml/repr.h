#ifndef __SP_REPR_H__
#define __SP_REPR_H__

/*
 * Fuzzy DOM-like tree implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <stdio.h>

#define SP_SODIPODI_NS_URI "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
#define SP_XLINK_NS_URI "http://www.w3.org/1999/xlink"
#define SP_SVG_NS_URI "http://www.w3.org/2000/svg"

/* NB! Unless explicitly stated all methods are noref/nostrcpy */

typedef struct _SPRepr SPRepr;
typedef struct _SPReprDoc SPReprDoc;
typedef struct _SPReprAttr SPReprAttr;

/* SPRepr */

SPRepr *sp_repr_new (const unsigned char *name);
SPRepr *sp_repr_new_text (const unsigned char *content);
SPRepr *sp_repr_ref (SPRepr *repr);
SPRepr *sp_repr_unref (SPRepr *repr);
SPRepr *sp_repr_duplicate (SPRepr *repr);

SPRepr *sp_repr_get_parent (SPRepr *repr);
SPRepr *sp_repr_get_children (SPRepr *repr);
SPRepr *sp_repr_get_next (SPRepr *repr);

const unsigned char *sp_repr_get_name (SPRepr *repr);
const unsigned char *sp_repr_get_content (SPRepr *repr);
const unsigned char *sp_repr_get_attr (SPRepr *repr, const unsigned char *key);

unsigned int sp_repr_is_element (SPRepr *repr);
unsigned int sp_repr_is_text (SPRepr *repr);

/* Return TRUE is change succeeded */
unsigned int sp_repr_set_content (SPRepr *repr, const unsigned char *content);
unsigned int sp_repr_set_attr (SPRepr *repr, const unsigned char *key, const unsigned char *value);
unsigned int sp_repr_add_child (SPRepr *repr, SPRepr *child, SPRepr *ref);
unsigned int sp_repr_remove_child (SPRepr *repr, SPRepr *child);
unsigned int sp_repr_change_order (SPRepr *repr, SPRepr *child, SPRepr *ref);

/* SPReprDoc */

SPReprDoc *sp_repr_doc_new (const unsigned char *rootname);
void sp_repr_doc_ref (SPReprDoc * doc);
void sp_repr_doc_unref (SPReprDoc * doc);
SPRepr *sp_repr_doc_get_root (SPReprDoc *doc);
SPReprDoc *sp_repr_get_doc (SPRepr *repr);

/* IO */

SPReprDoc *sp_repr_doc_new_from_file (const unsigned char *filename, const unsigned char *default_ns);
SPReprDoc *sp_repr_doc_new_from_mem (const unsigned char *data, unsigned int length, const unsigned char *default_ns);
unsigned int sp_repr_doc_write_stream (SPReprDoc *doc, FILE *stream);
unsigned int sp_repr_doc_write_file (SPReprDoc *doc, const unsigned char *filename);
unsigned int sp_repr_write_stream (SPRepr *repr, FILE *stream, unsigned int ident_level);

/* Attributes */

SPReprAttr *sp_repr_attr_get_first (SPRepr *repr);
SPReprAttr *sp_repr_attr_get_next (SPRepr *repr, SPReprAttr *ref);
const unsigned char *sp_repr_attr_get_key (SPRepr *repr, SPReprAttr *attr);
const unsigned char *sp_repr_attr_get_value (SPRepr *repr, SPReprAttr *attr);

/* Convenience */

unsigned int sp_repr_get_boolean (SPRepr *repr, const unsigned char *key, unsigned int *val);
unsigned int sp_repr_get_int (SPRepr *repr, const unsigned char *key, int *val);
unsigned int sp_repr_get_double (SPRepr *repr, const unsigned char *key, double *val);
unsigned int sp_repr_set_boolean (SPRepr *repr, const unsigned char *key, unsigned int val);
unsigned int sp_repr_set_int (SPRepr *repr, const unsigned char *key, int val);
unsigned int sp_repr_set_double (SPRepr *repr, const unsigned char *key, double val);

SPRepr *sp_repr_lookup_child (SPRepr *repr, const unsigned char *key, const unsigned char *value);
SPRepr *sp_repr_lookup_child_by_name (SPRepr *repr, const unsigned char *name);

unsigned int sp_repr_doc_merge (SPReprDoc *doc, SPReprDoc *src, const unsigned char *key);
unsigned int sp_repr_merge (SPRepr *repr, SPRepr *src, const unsigned char *key);

unsigned int sp_repr_append_child (SPRepr *repr, SPRepr *child);
unsigned int sp_repr_unparent (SPRepr *repr);

/* Event listeners */

typedef struct _SPReprEventVector SPReprEventVector;

struct _SPReprEventVector {
	/* Happens if recounts becomes zero and cannot be vetoed */
	void (* destroy) (SPRepr *repr, void *data);
	/* Returning FALSE vetoes mutation */
	/* Ref is child next to which new child will be added/moved */
	unsigned int (* add_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, void *data);
	void (* child_added) (SPRepr *repr, SPRepr *child, SPRepr *ref, void *data);
	unsigned int (* remove_child) (SPRepr *repr, SPRepr *child, SPRepr *ref, void *data);
	void (* child_removed) (SPRepr *repr, SPRepr *child, SPRepr *ref, void *data);
	unsigned int (* change_attr) (SPRepr *repr, const unsigned char *key,
				      const unsigned char *oldval, const unsigned char *newval, void *data);
	void (* attr_changed) (SPRepr *repr, const unsigned char *key,
			       const unsigned char *oldval, const unsigned char *newval, void *data);
	unsigned int (* change_content) (SPRepr *repr, const unsigned char *oldcontent,
					 const unsigned char *newcontent, void *data);
	void (* content_changed) (SPRepr *repr, const unsigned char *oldcontent,
				  const unsigned char *newcontent, void *data);
	unsigned int (* change_order) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, void *data);
	void (* order_changed) (SPRepr *repr, SPRepr *child, SPRepr *oldref, SPRepr *newref, void *data);
};

void sp_repr_add_listener (SPRepr *repr, const SPReprEventVector *vector, void *data);
void sp_repr_remove_listener_by_data (SPRepr *repr, void *data);

#endif
