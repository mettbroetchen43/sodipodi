#define SP_DOCUMENT_UNDO_C

#include <string.h>
#include <stdlib.h>
#include "xml/repr-private.h"
#include "sp-object.h"
#include "sp-item.h"
#include "document-private.h"

/* fixme: Implement in preferences */

#define MAX_UNDO 128

/*
 * Undo & redo
 */

void
sp_document_set_undo_sensitive (SPDocument * document, gboolean sensitive)
{
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (document->private != NULL);

	document->private->sensitive = sensitive;
}

void
sp_document_done (SPDocument * document)
{
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (document->private != NULL);
	g_assert (document->private->sensitive);

	if (document->private->actions == NULL) return;

	g_assert (document->private->redo == NULL);

	if (!sp_repr_attr (document->private->rroot, "sodipodi:modified")) {
		sp_repr_set_attr (document->private->rroot, "sodipodi:modified", "true");
	}

	if (g_slist_length (document->private->undo) >= MAX_UNDO) {
		GSList * last;
		last = g_slist_last (document->private->undo);
		document->private->undo = g_slist_remove (document->private->undo, last);
	}

	document->private->undo = g_slist_prepend (document->private->undo, document->private->actions);
	document->private->actions = NULL;
}

void
sp_document_maybe_done (SPDocument *document)
{
	sp_document_done (document);
}

void
sp_document_clear_actions (SPDocument * document)
{
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (document->private != NULL);
	g_assert (document->private->sensitive);

	while (document->private->actions) {
		sp_repr_unref ((SPRepr *) document->private->actions->data);
		document->private->actions = g_list_remove_link (document->private->actions, document->private->actions);
	}
}

void
sp_document_undo (SPDocument * document)
{
	GList * l;
	SPRepr * action;
	const gchar * name;
	SPRepr * repr, * copy;
	const gchar * id, * key, * value, * str;
	SPObject * object;
	gint position;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (document->private != NULL);
	g_assert (document->private->sensitive);
	g_assert (document->private->actions == NULL);

	if (document->private->undo == NULL) return;

	sp_document_set_undo_sensitive (document, FALSE);

	document->private->actions = (GList *) document->private->undo->data;
	document->private->undo = g_slist_remove_link (document->private->undo, document->private->undo);

	for (l = document->private->actions; l != NULL; l = l->next) {
		action = (SPRepr *) l->data;
		name = sp_repr_name (action);
		if (strcmp (name, "add") == 0) {
			/* Undoing add is del */
			repr = action->children;
			id = sp_repr_attr (repr, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			sp_repr_unparent (object->repr);
		}
		if (strcmp (name, "del") == 0) {
			id = sp_repr_attr (action, "parent");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_return_if_fail (object != NULL);
			str = sp_repr_attr (action, "position");
			g_assert (str != NULL);
			position = atoi (str);
			repr = action->children;
			copy = sp_repr_duplicate (repr);
			g_assert (copy != NULL);
			sp_repr_add_child (object->repr, copy, NULL);
			sp_repr_set_position_absolute (copy, position);
			sp_repr_unref (copy);
		}
		if (strcmp (name, "attr") == 0) {
			key = sp_repr_attr (action, "key");
			g_assert (key != NULL);
			if (strcmp (key, "id") != 0) {
				id = sp_repr_attr (action, "id");
				g_assert (id != NULL);
				object = sp_document_lookup_id (document, id);
			} else {
				id = sp_repr_attr (action, "new");
				g_assert (id != NULL);
				object = sp_document_lookup_id (document, id);
			}
			g_assert (object != NULL);
			value = sp_repr_attr (action, "old");
			sp_repr_set_attr (object->repr, key, value);
		}
		if (strcmp (name, "content") == 0) {
			id = sp_repr_attr (action, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			value = sp_repr_attr (action, "old");
			sp_repr_set_content (object->repr, value);
		}
		if (strcmp (name, "order") == 0) {
			SPObject *object, *child, *ref;
			const guchar *id, *cid, *rid;
			id = sp_repr_attr (action, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			cid = sp_repr_attr (action, "child");
			g_assert (cid != NULL);
			child = sp_document_lookup_id (document, cid);
			rid = sp_repr_attr (action, "old");
			if (rid) {
				ref = sp_document_lookup_id (document, rid);
				sp_repr_change_order (object->repr, child->repr, ref->repr);
			} else {
				sp_repr_change_order (object->repr, child->repr, NULL);
			}
		}
	}

	document->private->redo = g_slist_prepend (document->private->redo, document->private->actions);
	document->private->actions = NULL;

	sp_document_set_undo_sensitive (document, TRUE);
}

void
sp_document_redo (SPDocument * document)
{
	GList * l;
	SPRepr * action;
	const gchar * name;
	SPRepr * repr, * copy;
	const gchar * id, * key, * value;
	SPObject * object;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (document->private != NULL);
	g_assert (document->private->sensitive);
	g_assert (document->private->actions == NULL);

	if (document->private->redo == NULL) return;

	sp_document_set_undo_sensitive (document, FALSE);

	document->private->actions = (GList *) document->private->redo->data;
	document->private->redo = g_slist_remove_link (document->private->redo, document->private->redo);

	for (l = g_list_last (document->private->actions); l != NULL; l = l->prev) {
		action = (SPRepr *) l->data;
		name = sp_repr_name (action);
		if (strcmp (name, "add") == 0) {
			repr = action->children;
			copy = sp_repr_duplicate (repr);
			g_assert (copy != NULL);
			/* fixme: order! */
			sp_repr_append_child (document->private->rroot, copy);
			sp_repr_unref (copy);
		}
		if (strcmp (name, "del") == 0) {
			repr = action->children;
			id = sp_repr_attr (repr, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (id != NULL);
			sp_repr_unparent (object->repr);
		}
		if (strcmp (name, "attr") == 0) {
			id = sp_repr_attr (action, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			key = sp_repr_attr (action, "key");
			g_assert (key != NULL);
			value = sp_repr_attr (action, "new");
			sp_document_set_undo_sensitive (document, FALSE);
			sp_repr_set_attr (object->repr, key, value);
			sp_document_set_undo_sensitive (document, TRUE);
		}
		if (strcmp (name, "content") == 0) {
			id = sp_repr_attr (action, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			value = sp_repr_attr (action, "new");
			sp_document_set_undo_sensitive (document, FALSE);
			sp_repr_set_content (object->repr, value);
			sp_document_set_undo_sensitive (document, TRUE);
		}
		if (strcmp (name, "order") == 0) {
			SPObject *object, *child, *ref;
			const guchar *id, *cid, *rid;
			id = sp_repr_attr (action, "id");
			g_assert (id != NULL);
			object = sp_document_lookup_id (document, id);
			g_assert (object != NULL);
			cid = sp_repr_attr (action, "child");
			g_assert (cid != NULL);
			child = sp_document_lookup_id (document, cid);
			rid = sp_repr_attr (action, "new");
			if (rid) {
				ref = sp_document_lookup_id (document, rid);
				sp_repr_change_order (object->repr, child->repr, ref->repr);
			} else {
				sp_repr_change_order (object->repr, child->repr, NULL);
			}
		}
	}

	document->private->undo = g_slist_prepend (document->private->undo, document->private->actions);
	document->private->actions = NULL;

	sp_document_set_undo_sensitive (document, TRUE);
}

/*
 * Actions
 */

/*
 * <add><added repr></add>
 */

SPItem *
sp_document_add_repr (SPDocument * document, SPRepr * repr)
{
	SPRepr * action, * copy;
	const gchar * id;
	SPObject * object;

	sp_repr_append_child (document->private->rroot, repr);

	sp_document_clear_redo (document);

	if (document->private->sensitive) {
		action = sp_repr_new ("add");
		copy = sp_repr_duplicate (repr);
		sp_repr_append_child (action, copy);
		sp_repr_unref (copy);

		document->private->actions = g_list_prepend (document->private->actions, action);
	}

	id = sp_repr_attr (repr, "id");
	g_assert (id != NULL);

	object = sp_document_lookup_id (document, id);
	g_assert (object != NULL);
#if 0
	g_assert (SP_IS_ITEM (object));
#endif

	return SP_ITEM (object);
}

/*
 * <del parent=parentid position=position><deleted repr></del>
 */

void
sp_document_del_repr (SPDocument * document, SPRepr * repr)
{
	SPRepr * action;
	SPRepr * parent;
	const gchar * parentid;
	gint position;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);

	parent = sp_repr_parent (repr);
	g_assert (parent != NULL);
	parentid = sp_repr_attr (parent, "id");
	g_assert (parentid != NULL);
	position = sp_repr_position (repr);

	sp_repr_ref (repr);
	sp_repr_unparent (repr);

	sp_document_clear_redo (document);

	if (document->private->sensitive) {
		action = sp_repr_new ("del");
		sp_repr_set_attr (action, "parent", parentid);
		sp_repr_set_int_attribute (action, "position", position);
		sp_repr_append_child (action, repr);

		document->private->actions = g_list_prepend (document->private->actions, action);
	}

	sp_repr_unref (repr);
}


/*
 * <attr id="id" key="key" old="old" new="new">
 */

gboolean
sp_document_attr_changed (SPDocument * document, SPObject * object, const gchar * key, const gchar * oldval, const gchar * newval)
{
	if (document->private->sensitive) {
		SPRepr * action;

		sp_document_clear_redo (document);

		action = sp_repr_new ("attr");
		sp_repr_set_attr (action, "id", object->id);
		sp_repr_set_attr (action, "key", key);
		sp_repr_set_attr (action, "old", oldval);
		sp_repr_set_attr (action, "new", newval);

		document->private->actions = g_list_prepend (document->private->actions, action);
	}

	return TRUE;
}

/*
 * <content id="id" old="old" new="new">
 */

gboolean
sp_document_change_content_requested (SPDocument * document, SPObject * object, const gchar * value)
{
	SPRepr * action;
	const gchar * oldvalue;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (object->document == document);

	if (document->private->sensitive) {
		sp_document_clear_redo (document);

		oldvalue = sp_repr_content (object->repr);

		action = sp_repr_new ("content");
		sp_repr_set_attr (action, "id", object->id);
		sp_repr_set_attr (action, "old", oldvalue);
		sp_repr_set_attr (action, "new", value);

		document->private->actions = g_list_prepend (document->private->actions, action);
	}

	return TRUE;
}

/*
 * <order id="id" old="old" new="new">
 */

gboolean
sp_document_order_changed (SPDocument * document, SPObject * object, SPRepr * child, SPRepr * old, SPRepr * new)
{
	SPRepr * action;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (object != NULL);
	g_assert (SP_IS_OBJECT (object));
	g_assert (object->document == document);

	if (document->private->sensitive) {
		sp_document_clear_redo (document);

		action = sp_repr_new ("order");
		sp_repr_set_attr (action, "id", object->id);
		sp_repr_set_attr (action, "child", sp_repr_attr (child, "id"));
		if (old) sp_repr_set_attr (action, "old", sp_repr_attr (old, "id"));
		if (new) sp_repr_set_attr (action, "new", sp_repr_attr (new, "id"));

		document->private->actions = g_list_prepend (document->private->actions, action);
	}

	return TRUE;
}

void
sp_document_clear_undo (SPDocument * document)
{
	GList * l;

	while (document->private->undo) {
		l = (GList *) document->private->undo->data;
		while (l) {
			sp_repr_unref ((SPRepr *) l->data);
			l = g_list_remove (l, l->data);
		}
		document->private->undo = g_slist_remove_link (document->private->undo, document->private->undo);
	}
}

void
sp_document_clear_redo (SPDocument * document)
{
	GList * l;

	while (document->private->redo) {
		l = (GList *) document->private->redo->data;
		while (l) {
			sp_repr_unref ((SPRepr *) l->data);
			l = g_list_remove (l, l->data);
		}
		document->private->redo = g_slist_remove_link (document->private->redo, document->private->redo);
	}
}


