#define SP_DOCUMENT_UNDO_C

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
	g_assert (doc->private != NULL);

	doc->private->sensitive = sensitive;
}

void
sp_document_done (SPDocument *doc)
{
	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->private != NULL);
	g_assert (doc->private->sensitive);

	/* Clear modal undo key */
	doc->private->key = NULL;

	if (doc->private->actions == NULL) return;

	g_assert (doc->private->redo == NULL);

	if (!sp_repr_attr (doc->private->rroot, "sodipodi:modified")) {
		sp_repr_set_attr (doc->private->rroot, "sodipodi:modified", "true");
	}

	if (g_slist_length (doc->private->undo) >= MAX_UNDO) {
		GSList *last;
		last = g_slist_last (doc->private->undo);
		sp_action_free_list ((SPAction *) last->data);
		doc->private->undo = g_slist_remove (doc->private->undo, last->data);
	}

	doc->private->undo = g_slist_prepend (doc->private->undo, doc->private->actions);
	doc->private->actions = NULL;
}

void
sp_document_maybe_done (SPDocument *doc, const guchar *key)
{
	sp_document_done (doc);
}

void
sp_document_undo (SPDocument *doc)
{
	SPAction *actions, *reverse;

	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->private != NULL);
	g_assert (doc->private->sensitive);
	g_assert (doc->private->actions == NULL);

	if (doc->private->undo == NULL) return;

	/* Get last action list */
	actions = (SPAction *) doc->private->undo->data;
	doc->private->undo = g_slist_remove (doc->private->undo, actions);
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
	doc->private->redo = g_slist_prepend (doc->private->redo, reverse);
}

void
sp_document_redo (SPDocument *doc)
{
	SPAction *actions, *reverse;

	g_assert (doc != NULL);
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (doc->private != NULL);
	g_assert (doc->private->sensitive);
	g_assert (doc->private->actions == NULL);

	if (doc->private->redo == NULL) return;

	/* Get last action list */
	actions = (SPAction *) doc->private->redo->data;
	doc->private->redo = g_slist_remove (doc->private->redo, actions);
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
	doc->private->undo = g_slist_prepend (doc->private->undo, reverse);
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

	if (doc->private->sensitive) {
		SPAction *action;
		const guchar *refid;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_ADD, object->id);
		action->add.child = sp_repr_duplicate (child);
		refid = (ref) ? sp_repr_attr (ref, "id") : NULL;
		action->add.ref = (refid) ? g_strdup (refid) : NULL;

		action->next = doc->private->actions;
		doc->private->actions = action;
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

	if (doc->private->sensitive) {
		SPAction *action;
		const guchar *refid;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_DEL, object->id);
		action->del.child = sp_repr_duplicate (child);
		refid = (ref) ? sp_repr_attr (ref, "id") : NULL;
		action->del.ref = (refid) ? g_strdup (refid) : NULL;

		action->next = doc->private->actions;
		doc->private->actions = action;
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

	if (doc->private->sensitive) {
		SPAction *action;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGATTR, strcmp (key, "id") ? object->id : oldval);
		action->chgattr.key = g_quark_from_string (key);
		if (oldval) action->chgattr.oldval = g_strdup (oldval);
		if (newval) action->chgattr.newval = g_strdup (newval);

		action->next = doc->private->actions;
		doc->private->actions = action;
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

	if (doc->private->sensitive) {
		SPAction *action;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGCONTENT, object->id);
		if (oldcontent) action->chgcontent.oldval = g_strdup (oldcontent);
		if (newcontent) action->chgcontent.newval = g_strdup (newcontent);

		action->next = doc->private->actions;
		doc->private->actions = action;
	}

}

/*
 * <order id="parent_id" id="repr_id" old="oldref_id" new="newref_id">
 */

void
sp_document_order_changed (SPDocument *doc, SPObject *object, SPRepr *child, SPRepr *oldref, SPRepr *newref)
{
	g_assert (SP_IS_DOCUMENT (doc));
	g_assert (SP_IS_OBJECT (object));
	g_assert (child != NULL);

	if (doc->private->sensitive) {
		SPAction *action;
		const guchar *id;

		sp_document_clear_redo (doc);

		action = sp_action_new (SP_ACTION_CHGCONTENT, object->id);
		id = sp_repr_attr (child, "id");
		action->chgorder.child = g_strdup (id);
		id = (oldref) ? sp_repr_attr (oldref, "id") : NULL;
		if (id) action->chgorder.oldref = g_strdup (id);
		id = (newref) ? sp_repr_attr (newref, "id") : NULL;
		if (id) action->chgorder.newref = g_strdup (id);

		action->next = doc->private->actions;
		doc->private->actions = action;
	}
}

SPItem *
sp_document_add_repr (SPDocument * document, SPRepr * repr)
{
	const gchar *id;
	SPObject *object;

	sp_repr_append_child (document->private->rroot, repr);

	id = sp_repr_attr (repr, "id");
	g_assert (id != NULL);

	object = sp_document_lookup_id (document, id);
	g_assert (object != NULL);
#if 0
	g_assert (SP_IS_ITEM (object));
#endif

	return SP_ITEM (object);
}

void
sp_document_del_repr (SPDocument * document, SPRepr * repr)
{
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);

	sp_repr_unparent (repr);
}

void
sp_document_clear_undo (SPDocument *doc)
{
	while (doc->private->undo) {
		SPAction *actions;
		actions = (SPAction *) doc->private->undo->data;
		doc->private->undo = g_slist_remove (doc->private->undo, actions);
		sp_action_free_list (actions);
	}
}

void
sp_document_clear_redo (SPDocument *doc)
{
	while (doc->private->redo) {
		SPAction *actions;
		actions = (SPAction *) doc->private->redo->data;
		doc->private->redo = g_slist_remove (doc->private->redo, actions);
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

	child = sp_document_lookup_id (doc, sp_repr_attr (action->add.child, "id"));
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
	ref = (action->del.ref) ? sp_document_lookup_id (doc, action->del.ref) : NULL;

	new = sp_repr_duplicate (action->del.child);
	sp_repr_add_child (object->repr, new, (ref) ? ref->repr : NULL);
	sp_repr_unref (new);
}

static void
sp_action_undo_chgattr (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	/* fixme: This is suboptimal */
	if (action->chgattr.key == g_quark_from_string ("id")) {
		object = sp_document_lookup_id (doc, action->chgattr.newval);
		g_assert (object != NULL);
	} else {
		object = sp_document_lookup_id (doc, action->id);
		g_assert (object != NULL);
	}
	sp_repr_set_attr (object->repr, g_quark_to_string (action->chgattr.key), action->chgattr.oldval);
}

static void
sp_action_undo_chgcontent (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_content (object->repr, action->chgcontent.oldval);
}

static void
sp_action_undo_chgorder (SPDocument *doc, SPAction *action)
{
	SPObject *object, *child, *ref;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	child = sp_document_lookup_id (doc, action->chgorder.child);
	g_assert (child != NULL);

	ref = (action->chgorder.oldref) ? sp_document_lookup_id (doc, action->chgorder.oldref) : NULL;

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
	ref = (action->add.ref) ? sp_document_lookup_id (doc, action->add.ref) : NULL;

	new = sp_repr_duplicate (action->add.child);
	sp_repr_add_child (object->repr, new, (ref) ? ref->repr : NULL);
	sp_repr_unref (new);
}

static void
sp_action_redo_del (SPDocument *doc, SPAction *action)
{
	SPObject *child;

	child = sp_document_lookup_id (doc, sp_repr_attr (action->del.child, "id"));
	g_assert (child != NULL);

	sp_repr_unparent (child->repr);
}

static void
sp_action_redo_chgattr (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_attr (object->repr, g_quark_to_string (action->chgattr.key), action->chgattr.newval);
}

static void
sp_action_redo_chgcontent (SPDocument *doc, SPAction *action)
{
	SPObject *object;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);

	sp_repr_set_content (object->repr, action->chgcontent.newval);
}

static void
sp_action_redo_chgorder (SPDocument *doc, SPAction *action)
{
	SPObject *object, *child, *ref;

	object = sp_document_lookup_id (doc, action->id);
	g_assert (object != NULL);
	child = sp_document_lookup_id (doc, action->chgorder.child);
	g_assert (child != NULL);

	ref = (action->chgorder.newref) ? sp_document_lookup_id (doc, action->chgorder.newref) : NULL;

	sp_repr_change_order (object->repr, child->repr, (ref) ? ref->repr : NULL);
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
		action->add.child = NULL;
		action->add.ref = NULL;
		break;
	case SP_ACTION_DEL:
		action->del.child = NULL;
		action->del.ref = NULL;
		break;
	case SP_ACTION_CHGATTR:
		action->chgattr.key = 0;
		action->chgattr.oldval = NULL;
		action->chgattr.newval = NULL;
		break;
	case SP_ACTION_CHGCONTENT:
		action->chgcontent.oldval = NULL;
		action->chgcontent.newval = NULL;
		break;
	case SP_ACTION_CHGORDER:
		action->chgorder.child = NULL;
		action->chgorder.oldref = NULL;
		action->chgorder.newref = NULL;
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
		sp_repr_unref (action->add.child);
		if (action->add.ref) g_free (action->add.ref);
		break;
	case SP_ACTION_DEL:
		sp_repr_unref (action->del.child);
		if (action->del.ref) g_free (action->del.ref);
		break;
	case SP_ACTION_CHGATTR:
		if (action->chgattr.oldval) g_free (action->chgattr.oldval);
		if (action->chgattr.newval) g_free (action->chgattr.newval);
		break;
	case SP_ACTION_CHGCONTENT:
		if (action->chgcontent.oldval) g_free (action->chgcontent.oldval);
		if (action->chgcontent.newval) g_free (action->chgcontent.newval);
		break;
	case SP_ACTION_CHGORDER:
		g_free (action->chgorder.child);
		if (action->chgorder.oldref) g_free (action->chgorder.oldref);
		if (action->chgorder.newref) g_free (action->chgorder.newref);
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

