#ifndef SP_REPR_PRIVATE_H
#define SP_REPR_PRIVATE_H

/*
 * TODO: Hash tables are overkill for attributes
 */

#include "repr.h"

typedef struct _SPRepr SPNode;
typedef struct _SPReprAttr SPAttribute;
typedef struct _SPReprListener SPListener;

typedef struct _SPReprAttr SPReprAttr;
typedef struct _SPReprListener SPReprListener;
typedef struct _SPReprEventVector SPReprEventVector;

struct _SPReprAttr {
	SPReprAttr * next;
	gint key;
	gchar * value;
};

struct _SPReprListener {
	SPReprListener * next;
	SPReprEventVector * vector;
	gpointer data;
};

struct _SPReprEventVector {
	void (* child_added) (SPRepr * repr, SPRepr * child, SPRepr * ref, gpointer data);
	void (* remove_child) (SPRepr * repr, SPRepr * child, gpointer data);
	gint (* change_attr) (SPRepr * repr, const gchar * key, const gchar * oldval, const gchar * newval, gpointer data);
	void (* attr_changed) (SPRepr * repr, const gchar * key, const gchar * oldval, const gchar * newval, gpointer data);
	gint (* change_content) (SPRepr * repr, const gchar * content, gpointer data);
	void (* content_changed) (SPRepr * repr, gpointer data);
	gint (* change_order) (SPRepr * repr, SPRepr * child, SPRepr * old, SPRepr * new, gpointer data);
	void (* order_changed) (SPRepr * repr, SPRepr * child, SPRepr * old, SPRepr * new, gpointer data);
};

struct _SPRepr {
	gint refcount;
	gint name;
	SPRepr * parent;
	SPRepr * next;
	SPRepr * children;
	SPReprAttr * attributes;
	SPReprListener * listeners;
	gchar * content;
};

#define SP_REPR_ATTRIBUTE_KEY(a) g_quark_to_string (a->key)
#define SP_REPR_ATTRIBUTE_VALUE(a) (a->value)

SPRepr * sp_repr_nth_child (SPRepr * repr, gint n);

void sp_repr_change_order (SPRepr * repr, SPRepr * child, SPRepr * ref);

void sp_repr_add_listener (SPRepr * repr, SPReprEventVector * vector, gpointer data);
void sp_repr_remove_listener_by_data (SPRepr * repr, gpointer data);

void sp_repr_document_set_root (SPReprDoc * doc, SPRepr * repr);


#endif
