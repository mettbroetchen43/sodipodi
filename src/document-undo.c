#define __SP_DOCUMENT_UNDO_C__

/*
 * Undo/Redo stack implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <stdlib.h>
#include "xml/repr-private.h"
#include "sp-object.h"
#include "sp-item.h"
#include "document-private.h"

/* fixme: Implement in preferences */

#define MAX_UNDO 128

static void sp_action_list_undo (SPDocument *doc, SPAction *action);
static void sp_action_undo (SPDocument *doc, SPAction *action);
static void sp_action_undo_add (SPDocument *doc, SPAction *action);
static void sp_action_undo_del (SPDocument *doc, SPAction *action);
static void sp_action_undo_chgattr (SPDocument *doc, SPAction *action);
static void sp_action_undo_chgcontent (SPDocument *doc, SPAction *action);
static void sp_action_undo_chgorder (SPDocument *doc, SPAction *action);
static void sp_action_list_redo (SPDocument *doc, SPAction *action);
static void sp_action_redo (SPDocument *doc, SPAction *action);
static void sp_action_redo_add (SPDocument *doc, SPAction *action);
static void sp_action_redo_del (SPDocument *doc, SPAction *action);
static void sp_action_redo_chgattr (SPDocument *doc, SPAction *action);
static void sp_action_redo_chgcontent (SPDocument *doc, SPAction *action);
static void sp_action_redo_chgorder (SPDocument *doc, SPAction *action);

static void sp_action_print_list (SPAction *action);

static SPAction *sp_action_new (SPActionType type, const guchar *id);
static void sp_action_free (SPAction *action);

/*
 * Undo & redo
 */

void
sp_document_set_undo_sensitive (SPDocument *doc, gboolean sensitive)
{
	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->priv != NULL);

	doc->priv->sensitive = sensitive;
}

static void
sp_document_priv_done (SPDocument *doc, const guchar *key)
{
	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->priv != NULL);
	g_assert (doc->priv->sensitive);

	/* Set modal undo key */
	doc->actionkey = key;

	g_assert (doc->priv->redo == NULL);

	if (!sp_repr_attr (doc->rroot, "sodipodi:modified")) {
		sp_repr_set_attr (doc->rroot, "sodipodi:modified", "true");
	}

	if (g_slist_length (doc->priv->undo) >= MAX_UNDO) {
		GSList *last;
		last = g_slist_last (doc->priv->undo);
		sp_action_free_list ((SPAction *) last->data);
		doc->priv->undo = g_slist_remove (doc->priv->undo, last->data);
	}

	doc->priv->undo = g_slist_prepend (doc->priv->undo, doc->priv->actions);
	doc->priv->actions = NULL;
}

void
sp_document_done (SPDocument *doc)
{
	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->priv != NULL);
	g_assert (doc->priv->sensitive);

	if (!doc->priv->actions) return;

	sp_document_priv_done (doc, NULL);
}

void
sp_document_maybe_done (SPDocument *doc, const guchar *key)
{
	if (!doc->priv->actions) return;

	if (key && doc->actionkey && !strcmp (key, doc->actionkey) && doc->priv->undo) {
		SPAction *last, *current;
		current = doc->priv->actions;
		last = (SPAction *) doc->priv->undo->data;
		while (last && current) {
			if (current->type != last->type) break;
			if ((current->type != SP_ACTION_CHGATTR) && (current->type != SP_ACTION_CHGCONTENT)) break;
			if (strcmp (current->id, last->id)) break;
			current = current->next;
			last = last->next;
		}
		if (!last && !current) {
			/* Merge current into last */
			current = doc->priv->actions;
			last = (SPAction *) doc->priv->undo->data;
			while (last && current) {
				switch (current->type) {
				case SP_ACTION_CHGATTR:
					if (last->act.chgattr.newval) g_free (last->act.chgattr.newval);
					last->act.chgattr.newval = current->act.chgattr.newval;
					current->act.chgattr.newval = NULL;
					break;
				case SP_ACTION_CHGCONTENT:
					if (last->act.chgcontent.newval) g_free (last->act.chgcontent.newval);
					last->act.chgcontent.newval = current->act.chgcontent.newval;
					current->act.chgcontent.newval = NULL;
					break;
				default:
					g_warning ("file %s: line %d: Action %d should not be in merge list", __FILE__, __LINE__, current->type);
					break;
				}
				current = current->next;
				last = last->next;
			}
			/* fixme: IMHO NOP is handled by repr itself */
			if (!sp_repr_attr (doc->rroot, "sodipodi:modified")) {
				sp_repr_set_attr (doc->rroot, "sodipodi:modified", "true");
			}
			sp_action_free_list (doc->priv->actions);
			doc->priv->actions = NULL;
			return;
		}
	}

	sp_document_priv_done (doc, key);
}

void
sp_document_undo (SPDocument *doc)
{
	SPAction *actions, *reverse;

	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->priv != NULL);
	g_assert (doc->priv->sensitive);

	/* Set modal undo key */
	doc->actionkey = NULL;

	if (doc->priv->actions) {
		g_warning ("Document Undo: Last operation did not flush cache");
		sp_action_print_list (doc->priv->actions);
		return;
	}

	if (doc->priv->undo == NULL) return;

	/* Get last action list */
	actions = (SPAction *) doc->priv->undo->data;
	doc->priv->undo = g_slist_remove (doc->priv->undo, actions);
	/* Replay it */
	sp_action_list_undo (doc, actions);
	/* Reverse it */
	reverse = NULL;
	while (actions != NULL) {
		SPAction *a;
		a = actions;
		actions = actions->next;
		a->next = reverse;
		reverse = a;
	}
	/* Prepend it to redo */
	doc->priv->redo = g_slist_prepend (doc->priv->redo, reverse);
}

void
sp_document_redo (SPDocument *doc)
{
	SPAction *actions, *reverse;

	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->priv != NULL);
	g_assert (doc->priv->sensitive);
	g_assert (doc->priv->actions == NULL);

	/* Set modal undo key */
	doc->actionkey = NULL;

	if (doc->priv->redo == NULL) return;

	/* Get last action list */
	actions = (SPAction *) doc->priv->redo->data;
	doc->priv->redo = g_slist_remove (doc->priv->redo, actions);
	/* Replay it */
	sp_action_list_redo (doc, actions);
	/* Reverse it */
	reverse = NULL;
	while (actions != NULL) {
		SPAction *a;
		a = actions;
		actions = actions->next;
		a->next = reverse;
		reverse = a;
	}
	/* Prepend it to undo */
	doc->priv->undo = g_slist_prepend (doc->priv->undo, reverse);
}

/*
 * <add id="parent_id" ref="ref_id">
 *   <added repr>
 * </add>
 */

void
sp_document_child_added (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *ref)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));
	g_assert (child != NULL);

	if (doc->priv->sensitive) {
		SPAction *action;
		const guchar *refid;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_ADD, object->id);
		action->act.add.child = sp_repr_duplicate (child);
		refid = (ref) ? sp_repr_attr (ref, "id") : NULL;
		action->act.add.ref = (refid) ? g_strdup (refid) : NULL;

		action->next = doc->priv->actions;
		doc->priv->actions = action;
	}
}

/*
 * <del id="parent_id" ref="ref_id">
 *   <deleted repr>
 * </del>
 */

void
sp_document_child_removed (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *ref)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));
	g_assert (child != NULL);

	if (doc->priv->sensitive) {
		SPAction *action;
		const guchar *refid;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_DEL, object->id);
		action->act.del.child = sp_repr_duplicate (child);
		refid = (ref) ? sp_repr_attr (ref, "id") : NULL;
		action->act.del.ref = (refid) ? g_strdup (refid) : NULL;

		action->next = doc->priv->actions;
		doc->priv->actions = action;
	}
}

/*
 * <attr id="id" key="key" old="old" new="new">
 */

void
sp_document_attr_changed (SPDocument *doc, SPObject *object, const guchar *key, const guchar *oldval, const guchar *newval)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));
	g_assert (key != NULL);

	if (doc->priv->sensitive) {
		SPAction *action;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGATTR, strcmp (key, "id") ? (const guchar *) object->id : oldval);
		action->act.chgattr.key = g_quark_from_string (key);
		if (oldval) action->act.chgattr.oldval = g_strdup (oldval);
		if (newval) action->act.chgattr.newval = g_strdup (newval);

		action->next = doc->priv->actions;
		doc->priv->actions = action;
	}
}

/*
 * <content id="id" old="old" new="new">
 */

void
sp_document_content_changed (SPDocument *doc, SPObject *object, const guchar *oldcontent, const guchar *newcontent)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));

	if (doc->priv->sensitive) {
		SPAction *action;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGCONTENT, object->id);
		if (oldcontent) action->act.chgcontent.oldval = g_strdup (oldcontent);
		if (newcontent) action->act.chgcontent.newval = g_strdup (newcontent);

		action->next = doc->priv->actions;
		doc->priv->actions = action;
	}

}

/*
 * <order id="parent_id" child="repr_id" oldref="oldref_id" newref="newref_id">
 */

void
sp_document_order_changed (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *oldref, SPRepr *newref)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));
	g_assert (child != NULL);

	if (doc->priv->sensitive) {
		SPAction *action;
		const guchar *id;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGORDER, object->id);
		id = sp_repr_attr (child, "id");
		action->act.chgorder.child = g_strdup (id);
		id = (oldref) ? sp_repr_attr (oldref, "id") : NULL;
		if (id) action->act.chgorder.oldref = g_strdup (id);
		id = (newref) ? sp_repr_attr (newref, "id") : NULL;
		if (id) action->act.chgorder.newref = g_strdup (id);

		action->next = doc->priv->actions;
		doc->priv->actions = action;
	}
}

void
sp_document_clear_undo (SPDocument *doc)
{
	while (doc->priv->undo) {
		SPAction *actions;
		actions = (SPAction *) doc->priv->undo->data;
		doc->priv->undo = g_slist_remove (doc->priv->undo, actions);
		sp_action_free_list (actions);
	}
}

void
sp_document_clear_redo (SPDocument *doc)
{
	while (doc->priv->redo) {
		SPAction *actions;
		actions = (SPAction *) doc->priv->redo->data;
		doc->priv->redo = g_slist_remove (doc->priv->redo, actions);
		sp_action_free_list (actions);
	}
}

/* Action helpers */

static void
sp_action_list_undo (SPDocument *doc, SPAction *action)
{
	SPAction *a;

	sp_document_set_undo_sensitive (doc, FALSE);

	for (a = action; a != NULL; a = a->next) {
		sp_action_undo (doc, a);
	}

	sp_document_set_undo_sensitive (doc, TRUE);
}

static void
sp_action_undo (SPDocument *doc, SPAction *action)
{
	switch (action->type) {
	case SP_ACTION_ADD:
		sp_action_undo_add (doc, action);
		break;
	case SP_ACTION_DEL:
		sp_action_undo_del (doc, action);
		break;
	case SP_ACTION_CHGATTR:
		sp_action_undo_chgattr (doc, action);
		break;
	case SP_ACTION_CHGCONTENT:
		sp_action_undo_chgcontent (doc, action);
		break;
	case SP_ACTION_CHGORDER:
		sp_action_undo_chgorder (doc, action);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
sp_action_undo_add (SPDocument *doc, SPAction *action)
{
	SPObject *child;

	child = sp_document_lookup_id (doc, sp_repr_attr (action->act.add.child, "id"));
	g_assert (child != NULL);

	sp_repr_unparent (child->repr);
}

static void
sp_action_undo_del (SPDocument *doc, SPAction *action)
{
	SPObject *object, *ref;
	SPRepr *new;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	ref = (action->act.del.ref) ? sp_document_lookup_id (doc, action->act.del.ref) : NULL;

	new = sp_repr_duplicate (action->act.del.child);
	sp_repr_add_child (object->repr, new, (ref) ? ref->repr : NULL);
	sp_repr_unref (new);
}

static void
sp_action_undo_chgattr (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	/* fixme: This is suboptimal */
	if (action->act.chgattr.key == g_quark_from_string ("id")) {
		object = sp_document_lookup_id (doc, action->act.chgattr.newval);
		g_assert (object != NULL);
	} else {
		object = sp_document_lookup_id (doc, action->id);
		g_assert (object != NULL);
	}
	sp_repr_set_attr (object->repr, g_quark_to_string (action->act.chgattr.key), action->act.chgattr.oldval);
}

static void
sp_action_undo_chgcontent (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_content (object->repr, action->act.chgcontent.oldval);
}

static void
sp_action_undo_chgorder (SPDocument *doc, SPAction *action)
{
	SPObject *object, *child, *ref;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	child = sp_document_lookup_id (doc, action->act.chgorder.child);
	g_assert (child != NULL);

	ref = (action->act.chgorder.oldref) ? sp_document_lookup_id (doc, action->act.chgorder.oldref) : NULL;

	sp_repr_change_order (object->repr, child->repr, (ref) ? ref->repr : NULL);
}

static void
sp_action_list_redo (SPDocument *doc, SPAction *action)
{
	SPAction *a;

	sp_document_set_undo_sensitive (doc, FALSE);

	for (a = action; a != NULL; a = a->next) {
		sp_action_redo (doc, a);
	}

	sp_document_set_undo_sensitive (doc, TRUE);
}

static void
sp_action_redo (SPDocument *doc, SPAction *action)
{
	switch (action->type) {
	case SP_ACTION_ADD:
		sp_action_redo_add (doc, action);
		break;
	case SP_ACTION_DEL:
		sp_action_redo_del (doc, action);
		break;
	case SP_ACTION_CHGATTR:
		sp_action_redo_chgattr (doc, action);
		break;
	case SP_ACTION_CHGCONTENT:
		sp_action_redo_chgcontent (doc, action);
		break;
	case SP_ACTION_CHGORDER:
		sp_action_redo_chgorder (doc, action);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
sp_action_redo_add (SPDocument *doc, SPAction *action)
{
	SPObject *object, *ref;
	SPRepr *new;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	ref = (action->act.add.ref) ? sp_document_lookup_id (doc, action->act.add.ref) : NULL;

	new = sp_repr_duplicate (action->act.add.child);
	sp_repr_add_child (object->repr, new, (ref) ? ref->repr : NULL);
	sp_repr_unref (new);
}

static void
sp_action_redo_del (SPDocument *doc, SPAction *action)
{
	SPObject *child;

	child = sp_document_lookup_id (doc, sp_repr_attr (action->act.del.child, "id"));
	g_assert (child != NULL);

	sp_repr_unparent (child->repr);
}

static void
sp_action_redo_chgattr (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_attr (object->repr, g_quark_to_string (action->act.chgattr.key), action->act.chgattr.newval);
}

static void
sp_action_redo_chgcontent (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_content (object->repr, action->act.chgcontent.newval);
}

static void
sp_action_redo_chgorder (SPDocument *doc, SPAction *action)
{
	SPObject *object, *child, *ref;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	child = sp_document_lookup_id (doc, action->act.chgorder.child);
	g_assert (child != NULL);

	ref = (action->act.chgorder.newref) ? sp_document_lookup_id (doc, action->act.chgorder.newref) : NULL;

	sp_repr_change_order (object->repr, child->repr, (ref) ? ref->repr : NULL);
}

static void
sp_action_print_list (SPAction *action)
{
	while (action) {
		g_print ("SPAction: Id %s\n", action->id);
		switch (action->type) {
		case SP_ACTION_ADD:
			g_print ("SPAction: Add ref %s\n", action->act.add.ref);
			break;
		case SP_ACTION_DEL:
			g_print ("SPAction: Del ref %s\n", action->act.del.ref);
			break;
		case SP_ACTION_CHGATTR:
			g_print ("SPAction: ChgAttr %s: %s -> %s\n",
				 g_quark_to_string (action->act.chgattr.key),
				 action->act.chgattr.oldval,
				 action->act.chgattr.newval);
			break;
		case SP_ACTION_CHGCONTENT:
			g_print ("SPAction: ChgContent %s -> %s\n",
				 action->act.chgcontent.oldval,
				 action->act.chgcontent.newval);
			break;
		case SP_ACTION_CHGORDER:
			g_print ("SPAction: ChgOrder %s: %s -> %s\n",
				 action->act.chgorder.child,
				 action->act.chgorder.oldref,
				 action->act.chgorder.newref);
			break;
		default:
			g_print ("SPAction: Invalid action type %d\n", action->type);
			break;
		}
		action = action->next;
	}
}

#define SP_ACTION_ALLOC_SIZE 256
static SPAction *free_action = NULL;

static SPAction *
sp_action_new (SPActionType type, const guchar *id)
{
	SPAction *action;
	gint i;

	if (free_action) {
		action = free_action;
		free_action = free_action->next;
	} else {
		action = g_new (SPAction, SP_ACTION_ALLOC_SIZE);
		for (i = 1; i < SP_ACTION_ALLOC_SIZE - 1; i++) action[i].next = action + i + 1;
		action[SP_ACTION_ALLOC_SIZE - 1].next = NULL;
		free_action = action + 1;
	}

	action->next = NULL;
	action->type = type;
	action->id = g_strdup (id);

	switch (type) {
	case SP_ACTION_ADD:
		action->act.add.child = NULL;
		action->act.add.ref = NULL;
		break;
	case SP_ACTION_DEL:
		action->act.del.child = NULL;
		action->act.del.ref = NULL;
		break;
	case SP_ACTION_CHGATTR:
		action->act.chgattr.key = 0;
		action->act.chgattr.oldval = NULL;
		action->act.chgattr.newval = NULL;
		break;
	case SP_ACTION_CHGCONTENT:
		action->act.chgcontent.oldval = NULL;
		action->act.chgcontent.newval = NULL;
		break;
	case SP_ACTION_CHGORDER:
		action->act.chgorder.child = NULL;
		action->act.chgorder.oldref = NULL;
		action->act.chgorder.newref = NULL;
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	return action;
}

static void
sp_action_free (SPAction *action)
{
	switch (action->type) {
	case SP_ACTION_ADD:
		sp_repr_unref (action->act.add.child);
		if (action->act.add.ref) g_free (action->act.add.ref);
		break;
	case SP_ACTION_DEL:
		sp_repr_unref (action->act.del.child);
		if (action->act.del.ref) g_free (action->act.del.ref);
		break;
	case SP_ACTION_CHGATTR:
		if (action->act.chgattr.oldval) g_free (action->act.chgattr.oldval);
		if (action->act.chgattr.newval) g_free (action->act.chgattr.newval);
		break;
	case SP_ACTION_CHGCONTENT:
		if (action->act.chgcontent.oldval) g_free (action->act.chgcontent.oldval);
		if (action->act.chgcontent.newval) g_free (action->act.chgcontent.newval);
		break;
	case SP_ACTION_CHGORDER:
		g_free (action->act.chgorder.child);
		if (action->act.chgorder.oldref) g_free (action->act.chgorder.oldref);
		if (action->act.chgorder.newref) g_free (action->act.chgorder.newref);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	g_free (action->id);
	action->type = SP_ACTION_INVALID;

	action->next = free_action;
	free_action = action;
}

void
sp_action_free_list (SPAction *action)
{
	while (action) {
		SPAction *a;
		a = action;
		action = action->next;
		sp_action_free (a);
	}
}

