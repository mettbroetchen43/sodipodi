#define __SP_REPR_C__

#include <string.h>
#include "repr-private.h"

typedef struct _SPReprListener SPListener;

static SPRepr * sp_repr_new_from_code (gint code);
static void sp_repr_remove_attribute (SPRepr *repr, SPReprAttr *attr);
static void sp_node_remove_listener (SPXMLNode *node, SPListener *listener);

static SPXMLAttribute *sp_attribute_duplicate (const SPXMLAttribute *attr);
static SPXMLAttribute *sp_attribute_new_from_quark (gint key, const guchar *value);

/* Helpers */
static SPXMLNode * sp_node_alloc (void);
static void sp_node_free (SPXMLNode *node);
static SPXMLAttribute * sp_attribute_alloc (void);
static void sp_attribute_free (SPXMLAttribute *attribute);
static SPListener *sp_listener_alloc (void);
static void sp_listener_free (SPListener *listener);

SPRepr *
sp_repr_new_from_code (gint code)
{
	SPRepr * repr;

	repr = sp_node_alloc ();

	repr->refcount = 1;
	repr->name = code;
	repr->type = SP_XML_ELEMENT_NODE;
	repr->parent = repr->next = repr->children = NULL;
	repr->attributes = NULL;
	repr->listeners = NULL;
	repr->content = NULL;

	return repr;
}

SPRepr *
sp_repr_new (const guchar *name)
{
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (*name != '\0', NULL);

	return sp_repr_new_from_code (g_quark_from_string (name));
}

SPRepr *
sp_repr_ref (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (repr->refcount > 0, NULL);

	repr->refcount += 1;

	return repr;
}

SPRepr *
sp_repr_unref (SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (repr->refcount > 0, NULL);

	repr->refcount -= 1;

	if (repr->refcount < 1) {
		/* Parents reference children */
		g_assert (repr->parent == NULL);
		g_assert (repr->next == NULL);
		while (repr->children) sp_repr_remove_child (repr, repr->children);
		while (repr->attributes) sp_repr_remove_attribute (repr, repr->attributes);
		if (repr->content) g_free (repr->content);
		while (repr->listeners) sp_node_remove_listener (repr, repr->listeners);
		sp_node_free (repr);
	}

	return NULL;
}

static SPRepr *
sp_repr_attach (SPRepr *parent, SPRepr *child)
{
	g_assert (parent != NULL);
	g_assert (child != NULL);
	g_assert (child->parent == NULL);

	child->parent = parent;

	return child;
}

SPRepr *
sp_repr_duplicate (const SPRepr *repr)
{
	SPRepr *new;
	SPRepr *child, *lastchild;
	SPReprAttr *attr, *lastattr;

	g_return_val_if_fail (repr != NULL, NULL);

	new = sp_repr_new_from_code (repr->name);
	new->type = repr->type;

	if (repr->content != NULL) new->content = g_strdup (repr->content);

	lastchild = NULL;
	for (child = repr->children; child != NULL; child = child->next) {
		if (lastchild) {
			lastchild = lastchild->next = sp_repr_attach (new, sp_repr_duplicate (child));
		} else {
			lastchild = new->children = sp_repr_attach (new, sp_repr_duplicate (child));
		}
	}

	lastattr = NULL;
	for (attr = repr->attributes; attr != NULL; attr = attr->next) {
		if (lastattr) {
			lastattr = lastattr->next = sp_attribute_duplicate (attr);
		} else {
			lastattr = new->attributes = sp_attribute_duplicate (attr);
		}
	}

	return new;
}

const guchar *
sp_repr_name (const SPRepr *repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return SP_REPR_NAME (repr);
}

const guchar *
sp_repr_content (const SPRepr *repr)
{
	g_assert (repr != NULL);

	return SP_REPR_CONTENT (repr);
}

const guchar *
sp_repr_attr (const SPRepr *repr, const guchar *key)
{
	SPReprAttr *ra;
	GQuark q;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	q = g_quark_from_string (key);

	for (ra = repr->attributes; ra != NULL; ra = ra->next) if (ra->key == q) return ra->value;

	return NULL;
}

gboolean
sp_repr_set_content (SPRepr *repr, const guchar *newcontent)
{
	SPReprListener *rl;
	guchar *oldcontent;
	gboolean allowed;

	g_return_val_if_fail (repr != NULL, FALSE);

	oldcontent = SP_REPR_CONTENT (repr);

	allowed = TRUE;
	for (rl = repr->listeners; rl && allowed; rl = rl->next) {
		if (rl->vector->change_content) allowed = (* rl->vector->change_content) (repr, oldcontent, newcontent, rl->data);
	}

	if (allowed) {
		if (newcontent) {
			newcontent = repr->content = g_strdup (newcontent);
		} else {
			repr->content = NULL;
		}
		for (rl = repr->listeners; rl != NULL; rl = rl->next) {
			if (rl->vector->content_changed) (* rl->vector->content_changed) (repr, oldcontent, newcontent, rl->data);
		}
		if (oldcontent) g_free (oldcontent);
	}

	return allowed;
}

static gboolean
sp_repr_del_attr (SPRepr *repr, const guchar *key)
{
	SPReprAttr *prev, *attr;
	SPReprListener *rl;
	gboolean allowed;
	GQuark q;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);

	allowed = TRUE;

	q = g_quark_from_string (key);
	prev = NULL;
	for (attr = repr->attributes; attr && (attr->key != q); attr = attr->next) prev = attr;

	if (attr) {
		guchar *oldval;

		oldval = attr->value;

		for (rl = repr->listeners; rl && allowed; rl = rl->next) {
			if (rl->vector->change_attr) allowed = (* rl->vector->change_attr) (repr, key, oldval, NULL, rl->data);
		}

		if (allowed) {
			(prev) ? prev->next : repr->attributes = attr->next;

			for (rl = repr->listeners; rl != NULL; rl = rl->next) {
				if (rl->vector->attr_changed) (* rl->vector->attr_changed) (repr, key, oldval, NULL, rl->data);
			}

			sp_attribute_free (attr);
		}
	}

	return allowed;
}

static gboolean
sp_repr_chg_attr (SPRepr *repr, const guchar *key, const guchar *value)
{
	SPReprAttr *prev, *attr;
	SPReprListener *rl;
	gboolean allowed;
	gchar *oldval;
	GQuark q;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);

	oldval = NULL;
	q = g_quark_from_string (key);
	prev = NULL;
	for (attr = repr->attributes; attr && (attr->key != q); attr = attr->next) prev = attr;
	if (attr) {
		if (!strcmp (attr->value, value)) return TRUE;
		oldval = attr->value;
	}

	allowed = TRUE;
	for (rl = repr->listeners; rl && allowed; rl = rl->next) {
		if (rl->vector->change_attr) allowed = (* rl->vector->change_attr) (repr, key, oldval, value, rl->data);
	}

	if (allowed) {
		if (attr) {
			attr->value = g_strdup (value);
		} else {
			attr = sp_attribute_new_from_quark (q, value);

			(prev) ? prev->next : repr->attributes = attr;
		}

		for (rl = repr->listeners; rl != NULL; rl = rl->next) {
			if (rl->vector->attr_changed) (* rl->vector->attr_changed) (repr, key, oldval, value, rl->data);
		}

		if (oldval) g_free (oldval);
	}

	return allowed;
}

gboolean
sp_repr_set_attr (SPRepr *repr, const guchar *key, const guchar *value)
{
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*key != '\0', FALSE);

	if (!value) return sp_repr_del_attr (repr, key);

	return sp_repr_chg_attr (repr, key, value);
}


static void
sp_repr_remove_attribute (SPRepr *repr, SPReprAttr *attr)
{
	g_assert (repr != NULL);
	g_assert (attr != NULL);

	if (attr == repr->attributes) {
		repr->attributes = attr->next;
	} else {
		SPReprAttr * prev;
		prev = repr->attributes;
		while (prev->next != attr) {
			prev = prev->next;
			g_assert (prev != NULL);
		}
		prev->next = attr->next;
	}

	g_free (attr->value);
	sp_attribute_free (attr);
}

SPRepr *
sp_repr_parent (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->parent;
}

gboolean
sp_repr_add_child (SPRepr *repr, SPRepr *child, SPRepr *ref)
{
	SPReprListener * rl;
	gboolean allowed;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (!ref || ref->parent == repr);
	g_assert (child->parent == NULL);

	allowed = TRUE;
	for (rl = repr->listeners; rl != NULL; rl = rl->next) {
		if (rl->vector->add_child) allowed = (* rl->vector->add_child) (repr, child, ref, rl->data);
	}

	if (allowed) {
		if (ref != NULL) {
			child->next = ref->next;
			ref->next = child;
		} else {
			child->next = repr->children;
			repr->children = child;
		}

		child->parent = repr;
		sp_repr_ref (child);

		for (rl = repr->listeners; rl != NULL; rl = rl->next) {
			if (rl->vector->child_added) (* rl->vector->child_added) (repr, child, ref, rl->data);
		}
	}

	return allowed;
}

gboolean
sp_repr_remove_child (SPRepr *repr, SPRepr *child)
{
	SPReprListener *rl;
	SPRepr *ref;
	gboolean allowed;

	g_assert (repr != NULL);
	g_assert (child != NULL);
	g_assert (child->parent == repr);

	ref = NULL;
	if (child != repr->children) {
		ref = repr->children;
		while (ref->next != child) ref = ref->next;
	}

	allowed = TRUE;
	for (rl = repr->listeners; rl != NULL; rl = rl->next) {
		if (rl->vector->remove_child) allowed = (* rl->vector->remove_child) (repr, child, ref, rl->data);
	}

	if (allowed) {
		(ref) ? ref->next : repr->children = child->next;
		child->parent = NULL;
		child->next = NULL;

		for (rl = repr->listeners; rl != NULL; rl = rl->next) {
			if (rl->vector->child_removed) (* rl->vector->child_removed) (repr, child, ref, rl->data);
		}

		sp_repr_unref (child);
	}

	return allowed;
}

gboolean
sp_repr_change_order (SPRepr *repr, SPRepr *child, SPRepr *ref)
{
	SPRepr * prev;
	SPReprListener * rl;
	gint allowed;

	prev = NULL;
	if (child != repr->children) {
		SPRepr *cur;
		for (cur = repr->children; cur != child; cur = cur->next) prev = cur;
	}

	if (prev == ref) return TRUE;

	allowed = TRUE;
	for (rl = repr->listeners; rl && allowed; rl = rl->next) {
		if (rl->vector->change_order) allowed = (* rl->vector->change_order) (repr, child, prev, ref, rl->data);
	}

	if (allowed) {
		if (prev) {
			prev->next = child->next;
		} else {
			repr->children = child->next;
		}
		if (ref) {
			child->next = ref->next;
			ref->next = child;
		} else {
			child->next = repr->children;
			repr->children = child;
		}

		for (rl = repr->listeners; rl != NULL; rl = rl->next) {
			if (rl->vector->order_changed) (* rl->vector->order_changed) (repr, child, prev, ref, rl->data);
		}
	}

	return allowed;
}

void
sp_repr_set_position_absolute (SPRepr * repr, gint pos)
{
	SPRepr * parent;
	SPRepr * ref, * cur;

	parent = repr->parent;

	if (pos < 0) pos = G_MAXINT;

	ref = NULL;
	cur = parent->children;
	while (pos > 0 && cur) {
		ref = cur;
		cur = cur->next;
		pos -= 1;
	}

	if (ref == repr) {
		ref = repr->next;
		if (!ref) return;
	}

	sp_repr_change_order (parent, repr, ref);
}

void
sp_repr_add_listener (SPRepr *repr, const SPReprEventVector *vector, gpointer data)
{
	SPReprListener *rl, *last;

	g_assert (repr != NULL);
	g_assert (vector != NULL);

	last = NULL;
	if (repr->listeners) {
		last = repr->listeners;
		while (last->next) last = last->next;
	}

	rl = sp_listener_alloc ();
	rl->next = NULL;
	rl->vector = vector;
	rl->data = data;

	if (last) {
		last->next = rl;
	} else {
		repr->listeners = rl;
	}
}

static void
sp_node_remove_listener (SPXMLNode *node, SPListener *listener)
{
	if (listener == node->listeners) {
		node->listeners = listener->next;
	} else {
		SPListener *prev;
		prev = node->listeners;
		while (prev->next != listener) prev = prev->next;
		prev->next = listener->next;
	}

	sp_listener_free (listener);
}

void
sp_repr_remove_listener_by_data (SPRepr * repr, gpointer data)
{
	SPReprListener * last, * rl;

	last = NULL;
	for (rl = repr->listeners; rl != NULL; rl = rl->next) {
		if (rl->data == data) {
			if (last) {
				last->next = rl->next;
			} else {
				repr->listeners = rl->next;
			}
			sp_listener_free (rl);
			return;
		}
		last = rl;
		rl = rl->next;
	}
}

SPRepr *
sp_repr_nth_child (const SPRepr * repr, gint n)
{
	SPRepr * child;

	child = repr->children;

	while (n > 0 && child) {
		child = child->next;
		n -= 1;
	}

	return child;
}

/* Documents - 1st step in migrating to real XML */
/* fixme: Do this somewhere, somehow The Right Way (TM) */

SPReprDoc *
sp_repr_document_new (const gchar * rootname)
{
	SPRepr * repr, * root;

	repr = sp_repr_new ("?xml");
	if (!strcmp (rootname, "svg")) {
		sp_repr_set_attr (repr, "version", "1.0");
		sp_repr_set_attr (repr, "standalone", "no");
		sp_repr_set_attr (repr, "doctype",
				  "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG August 1999//EN\"\n"
				  "\"http://www.w3.org/Graphics/SVG/SVG-19990812.dtd\">");
	}

	root = sp_repr_new (rootname);
	sp_repr_add_child (repr, root, 0);

	return (SPReprDoc *) repr;
}

void
sp_repr_document_ref (SPReprDoc * doc)
{
	sp_repr_ref ((SPRepr *) doc);
}

void
sp_repr_document_unref (SPReprDoc * doc)
{
	sp_repr_unref ((SPRepr *) doc);
}

void
sp_repr_document_set_root (SPReprDoc * doc, SPRepr * repr)
{
	SPRepr * rdoc;

	g_assert (doc != NULL);
	g_assert (repr != NULL);

	rdoc = (SPRepr *) doc;

	g_assert (rdoc->children != NULL);

	sp_repr_remove_child (rdoc, rdoc->children);
	sp_repr_add_child (rdoc, repr, NULL);
}

SPReprDoc *
sp_repr_document (SPRepr * repr)
{
	/* fixme: */
	while (repr->parent) repr = repr->parent;

	return (SPReprDoc *) repr;
}

SPRepr *
sp_repr_document_root (SPReprDoc * doc)
{
	SPRepr * repr;

	repr = (SPRepr *) doc;
	g_return_val_if_fail (repr->children != NULL, NULL);

	return repr->children;
}

/**
 * sp_repr_document_overwrite:
 * @doc: overwrite into
 * @source: overwrite it
 * @key: identify each node by
 *
 * Copy all @source subtree into @doc.
 */
void
sp_repr_document_overwrite (SPReprDoc       *doc,
			    const SPReprDoc *source,
			    const guchar    *key)
{
	SPRepr       *rdoc;
	const SPRepr *rsrc;
	
	rdoc = sp_repr_document_root (doc);
	rsrc = sp_repr_document_root ((SPReprDoc *) source);
	
	sp_repr_overwrite (rdoc->children, rsrc->children, key);
}

/**
 * sp_repr_overwrite:
 * @repr: overwrite into
 * @src: overwrite it
 * @key: identify each node by
 *
 * Copy all @src subtree into @doc.
 */
gboolean
sp_repr_overwrite (SPRepr       *repr,
		   const SPRepr *src,
		   const guchar *key)
{
	SPRepr *child;
	SPReprAttr *attr, *lastattr;
	
	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (src != NULL, FALSE);

	/* Fixme: we should use hash table for faster lookup? */
	
	if (src->content != NULL) {
		if (repr->content != NULL) g_free (repr->content);
		repr->content = g_strdup (src->content);
	}
	
	for (child = src->children; child != NULL; child = child->next) {
		SPRepr *rchild;
		const guchar *value;
		
		value = sp_repr_attr (child, key);
		rchild = sp_repr_lookup_child (repr, key, value);
		if (rchild) {
			sp_repr_overwrite (rchild, child, key);
		} else {
			rchild = sp_repr_duplicate (child);
			sp_repr_append_child (repr, rchild);
		}
	}
	
	/* fixme: We probably should merge, not replace here (Lauris) */
	while (repr->attributes) sp_repr_remove_attribute (repr, repr->attributes);

	lastattr = NULL;
	for (attr = src->attributes; attr != NULL; attr = attr->next) {
		if (lastattr) {
			lastattr = lastattr->next = sp_attribute_duplicate (attr);
		} else {
			lastattr = repr->attributes = sp_attribute_duplicate (attr);
		}
	}

	return TRUE;
}

static SPXMLAttribute *
sp_attribute_duplicate (const SPXMLAttribute *attr)
{
	SPXMLAttribute *new;

	new = sp_attribute_alloc ();

	new->next = NULL;
	new->key = attr->key;
	new->value = g_strdup (attr->value);

	return new;
}

static SPXMLAttribute *
sp_attribute_new_from_quark (gint key, const guchar *value)
{
	SPXMLAttribute *new;

	new = sp_attribute_alloc ();

	new->next = NULL;
	new->key = key;
	new->value = g_strdup (value);

	return new;
}

/* Helpers */
#define SP_NODE_ALLOC_SIZE 32
static SPXMLNode *free_node = NULL;

static SPXMLNode *
sp_node_alloc (void)
{
	SPXMLNode *node;
	gint i;

	if (free_node) {
		node = free_node;
		free_node = free_node->next;
	} else {
		node = g_new (SPXMLNode, SP_NODE_ALLOC_SIZE);
		for (i = 1; i < SP_NODE_ALLOC_SIZE - 1; i++) node[i].next = node + i + 1;
		node[SP_NODE_ALLOC_SIZE - 1].next = NULL;
		free_node = node + 1;
	}

	return node;
}

static void
sp_node_free (SPXMLNode *node)
{
	node->next = free_node;
	free_node = node;
}

#define SP_ATTRIBUTE_ALLOC_SIZE 32
static SPXMLAttribute *free_attribute = NULL;

static SPXMLAttribute *
sp_attribute_alloc (void)
{
	SPXMLAttribute *attribute;
	gint i;

	if (free_attribute) {
		attribute = free_attribute;
		free_attribute = free_attribute->next;
	} else {
		attribute = g_new (SPXMLAttribute, SP_ATTRIBUTE_ALLOC_SIZE);
		for (i = 1; i < SP_ATTRIBUTE_ALLOC_SIZE - 1; i++) attribute[i].next = attribute + i + 1;
		attribute[SP_ATTRIBUTE_ALLOC_SIZE - 1].next = NULL;
		free_attribute = attribute + 1;
	}

	return attribute;
}

static void
sp_attribute_free (SPXMLAttribute *attribute)
{
	attribute->next = free_attribute;
	free_attribute = attribute;
}

#define SP_LISTENER_ALLOC_SIZE 32
static SPListener *free_listener = NULL;

static SPListener *
sp_listener_alloc (void)
{
	SPListener *listener;
	gint i;

	if (free_listener) {
		listener = free_listener;
		free_listener = free_listener->next;
	} else {
		listener = g_new (SPListener, SP_LISTENER_ALLOC_SIZE);
		for (i = 1; i < SP_LISTENER_ALLOC_SIZE - 1; i++) listener[i].next = listener + i + 1;
		listener[SP_LISTENER_ALLOC_SIZE - 1].next = NULL;
		free_listener = listener + 1;
	}

	return listener;
}

static void
sp_listener_free (SPListener *listener)
{
	listener->next = free_listener;
	free_listener = listener;
}

