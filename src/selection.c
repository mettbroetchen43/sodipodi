#define __SP_SELECTION_C__

/*
 * Per-desktop selection container
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define sp_debug(str, section) 	if (FALSE) printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); 

#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include "sodipodi-private.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "document.h"
#include "selection.h"

#define SP_SELECTION_UPDATE_PRIORITY 0

enum {
	CHANGED,
	MODIFIED,
	LAST_SIGNAL
};

static void sp_selection_class_init (SPSelectionClass * klass);
static void sp_selection_init (SPSelection * selection);
static void sp_selection_destroy (GtkObject * object);

static void sp_selection_private_changed (SPSelection * selection);

static void sp_selection_frozen_empty (SPSelection * selection);

static gint sp_selection_idle_handler (gpointer data);

static GtkObjectClass * parent_class;
static guint selection_signals[LAST_SIGNAL] = { 0 };

GtkType
sp_selection_get_type (void)
{
	static GtkType selection_type = 0;

	if (!selection_type) {

		static const GtkTypeInfo selection_info = {
			"SPSelection",
			sizeof (SPSelection),
			sizeof (SPSelectionClass),
			(GtkClassInitFunc) sp_selection_class_init,
			(GtkObjectInitFunc) sp_selection_init,
			NULL, NULL, NULL
		};

		selection_type = gtk_type_unique (gtk_object_get_type (), &selection_info);
	}
	return selection_type;
}

static void
sp_selection_class_init (SPSelectionClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	selection_signals [CHANGED] = gtk_signal_new ("changed",
						      GTK_RUN_FIRST,
						      GTK_CLASS_TYPE(object_class),
						      GTK_SIGNAL_OFFSET (SPSelectionClass, changed),
						      gtk_marshal_NONE__NONE,
						      GTK_TYPE_NONE, 0);
	selection_signals [MODIFIED] = gtk_signal_new ("modified",
						       GTK_RUN_FIRST,
						       GTK_CLASS_TYPE(object_class),
						       GTK_SIGNAL_OFFSET (SPSelectionClass, modified),
						       gtk_marshal_NONE__UINT,
						       GTK_TYPE_NONE, 1, GTK_TYPE_UINT);

	object_class->destroy = sp_selection_destroy;

	klass->changed = sp_selection_private_changed;
}

static void
sp_selection_init (SPSelection * selection)
{
	selection->reprs = NULL;
	selection->items = NULL;
	selection->idle = 0;
	selection->flags = 0;
}

static void
sp_selection_destroy (GtkObject *object)
{
	SPSelection * selection;

	selection = SP_SELECTION (object);

	sp_selection_frozen_empty (selection);

	if (selection->idle) {
		/* Clear pending idle request */
		gtk_idle_remove (selection->idle);
		selection->idle = 0;
	}

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
sp_selection_private_changed (SPSelection * selection)
{
	sodipodi_selection_set (selection);
}

static void
sp_selection_selected_item_release (SPItem * item, SPSelection * selection)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	selection->items = g_slist_remove (selection->items, item);

	sp_selection_changed (selection);
}

/* Handler for selected objects "modified" signal */

static void
sp_selection_selected_item_modified (SPItem *item, guint flags, SPSelection *selection)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (sp_selection_item_selected (selection, item));

	if (!selection->idle) {
		/* Request handling to be run in idle loop */
		selection->idle = gtk_idle_add_priority (SP_SELECTION_UPDATE_PRIORITY, sp_selection_idle_handler, selection);
	}

	/* Collect all flags */
	selection->flags |= flags;
}

/* Our idle loop handler */

static gint
sp_selection_idle_handler (gpointer data)
{
	SPSelection *selection;
	guint flags;

	selection = SP_SELECTION (data);

	/* Clear our id, so next request will be rescheduled */
	selection->idle = 0;
	flags = selection->flags;
	selection->flags = 0;
	/* Emit our own "modified" signal */
	gtk_signal_emit (GTK_OBJECT (selection), selection_signals [MODIFIED], flags);

	/* Request "selection_modified" signal on Sodipodi */
	sodipodi_selection_modified (selection, flags);

	return FALSE;
}

void
sp_selection_frozen_empty (SPSelection * selection)
{
	SPItem * i;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	while (selection->items) {
		i = (SPItem *) selection->items->data;
/* 		gtk_signal_disconnect_by_data (GTK_OBJECT (i), selection); */
		g_signal_handlers_disconnect_matched (G_OBJECT(i), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, selection);
		selection->items = g_slist_remove (selection->items, i);
	}

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_update_statusbar (SPSelection * selection)
{
	gchar * message;
	gint len;

	len = g_slist_length (selection->items);

	if (len == 1) {
		message = sp_item_description (SP_ITEM (selection->items->data));
	} else {
		message = g_strdup_printf ("%i items selected",
			 g_slist_length (selection->items));
	}

	sp_view_set_status (SP_VIEW (SP_ACTIVE_DESKTOP), message, TRUE);
	
	g_free (message);
}

void
sp_selection_changed (SPSelection * selection)
{
	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	gtk_signal_emit (GTK_OBJECT (selection), selection_signals [CHANGED]);

	sp_selection_update_statusbar (selection);

	sp_debug ("end", SP_SELECTION);
}

SPSelection *
sp_selection_new (SPDesktop * desktop)
{
	SPSelection * selection;

	sp_debug ("start", SP_SELECTION);

	selection = gtk_type_new (SP_TYPE_SELECTION);
	g_assert (selection != NULL);

	selection->desktop = desktop;

	sp_debug ("end", SP_SELECTION);

	return selection;
}

gboolean
sp_selection_item_selected (SPSelection * selection, SPItem * item)
{
	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, FALSE);
	g_return_val_if_fail (SP_IS_SELECTION (selection), FALSE);
	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (SP_IS_ITEM (item), FALSE);

	sp_debug ("end", SP_SELECTION);

	return (g_slist_find (selection->items, item) != NULL);
}

gboolean
sp_selection_repr_selected (SPSelection * selection, SPRepr * repr)
{
	GSList * l;

	sp_debug ("start", SP_SELECTION);
	
	g_return_val_if_fail (selection != NULL, FALSE);
	g_return_val_if_fail (SP_IS_SELECTION (selection), FALSE);
	g_return_val_if_fail (repr != NULL, FALSE);

	for (l = selection->items; l != NULL; l = l->next) {
		if (((SPObject *)l->data)->repr == repr) return TRUE;
	}

	sp_debug ("end", SP_SELECTION);

	return FALSE;
}

void
sp_selection_add_item (SPSelection * selection, SPItem * item)
{
	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (!sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	selection->items = g_slist_prepend (selection->items, item);
	g_signal_connect (G_OBJECT (item), "release",
			  G_CALLBACK (sp_selection_selected_item_release), selection);
	g_signal_connect (G_OBJECT (item), "modified",
			  G_CALLBACK (sp_selection_selected_item_modified), selection);

	sp_selection_changed (selection);

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_add_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_add_item (selection, SP_ITEM (object));

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_set_item (SPSelection * selection, SPItem * item)
{
	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	sp_selection_frozen_empty (selection);

	sp_selection_add_item (selection, item);

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_set_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_set_item (selection, SP_ITEM (object));

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_remove_item (SPSelection * selection, SPItem * item)
{
	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

/* 	gtk_signal_disconnect_by_data (GTK_OBJECT (item), selection); */
	g_signal_handlers_disconnect_matched (G_OBJECT(item), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, selection);
	selection->items = g_slist_remove (selection->items, item);

	sp_selection_changed (selection);

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_remove_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_remove_item (selection, SP_ITEM (object));

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_set_item_list (SPSelection * selection, const GSList * list)
{
	SPItem * i;
	const GSList * l;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	sp_selection_frozen_empty (selection);

	if (list != NULL) {
		for (l = list; l != NULL; l = l->next) {
			i = (SPItem *) l->data;
			if (!SP_IS_ITEM (i)) break;
			selection->items = g_slist_prepend (selection->items, i);
			g_signal_connect (G_OBJECT (i), "release",
					  G_CALLBACK (sp_selection_selected_item_release), selection);
			g_signal_connect (G_OBJECT (i), "modified",
					  G_CALLBACK (sp_selection_selected_item_modified), selection);
		}
	}

	sp_selection_changed (selection);

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_set_repr_list (SPSelection * selection, const GSList * list)
{
	GSList * itemlist;
	const GSList * l;
	SPRepr * repr;
	const gchar * id;
	SPObject * object;

	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	itemlist = NULL;

	for (l = list; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		g_return_if_fail (repr != NULL);
		id = sp_repr_attr (repr, "id");
		g_return_if_fail (id != NULL);

		object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
		g_return_if_fail (object != NULL);
		g_return_if_fail (SP_IS_ITEM (object));

		itemlist = g_slist_prepend (itemlist, object);
	}

	sp_selection_set_item_list (selection, itemlist);

	g_slist_free (itemlist);

	sp_debug ("end", SP_SELECTION);
}

void
sp_selection_empty (SPSelection * selection)
{
	sp_debug ("start", SP_SELECTION);

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	sp_selection_frozen_empty (selection);

	sp_selection_changed (selection);

	sp_debug ("end", SP_SELECTION);
}

const GSList *
sp_selection_item_list (SPSelection * selection)
{
	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	sp_debug ("end", SP_SELECTION);

	return selection->items;
}

const GSList *
sp_selection_repr_list (SPSelection * selection)
{
	SPItem * i;
	GSList * l;

	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	for (l = selection->items; l != NULL; l = l->next) {
		i = (SPItem *) l->data;
		selection->reprs = g_slist_prepend (selection->reprs, SP_OBJECT (i)->repr);
	}

	sp_debug ("end", SP_SELECTION);

	return selection->reprs;
}

SPItem *
sp_selection_item (SPSelection * selection)
{
	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	if (selection->items == NULL) return NULL;
	if (selection->items->next != NULL) return NULL;

	sp_debug ("end", SP_SELECTION);

	return SP_ITEM (selection->items->data);
}

SPRepr *
sp_selection_repr (SPSelection * selection)
{
	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	if (selection->items == NULL) return NULL;
	if (selection->items->next != NULL) return NULL;

	sp_debug ("end", SP_SELECTION);

	return SP_OBJECT (selection->items->data)->repr;
}

ArtDRect *
sp_selection_bbox (SPSelection *selection, ArtDRect *bbox)
{
	SPItem *item;
	ArtDRect b;
	GSList *l;

	sp_debug ("start", SP_SELECTION);

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);
	g_return_val_if_fail (bbox != NULL, NULL);

	if (sp_selection_is_empty (selection)) {
		bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;
		return bbox;
	}

	bbox->x0 = bbox->y0 = 1e18;
	bbox->x1 = bbox->y1 = -1e18;

	for (l = selection->items; l != NULL; l = l-> next) {
		item = SP_ITEM (l->data);
		sp_item_bbox_desktop (item, &b);
		if (b.x0 < bbox->x0) bbox->x0 = b.x0;
		if (b.y0 < bbox->y0) bbox->y0 = b.y0;
		if (b.x1 > bbox->x1) bbox->x1 = b.x1;
		if (b.y1 > bbox->y1) bbox->y1 = b.y1;
	}

	sp_debug ("end", SP_SELECTION);
	return bbox;
}

ArtDRect *
sp_selection_bbox_document (SPSelection *selection, ArtDRect *bbox)
{
	GSList *l;

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);
	g_return_val_if_fail (bbox != NULL, NULL);

	if (sp_selection_is_empty (selection)) {
		bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;
		return bbox;
	}

	bbox->x0 = bbox->y0 = 1e18;
	bbox->x1 = bbox->y1 = -1e18;

	for (l = selection->items; l != NULL; l = l-> next) {
		gdouble i2doc[6];

		sp_item_i2doc_affine (SP_ITEM (l->data), i2doc);
		sp_item_invoke_bbox (SP_ITEM (l->data), bbox, i2doc, FALSE);
	}

	return bbox;
}

GSList * 
sp_selection_snappoints (SPSelection * selection)
/* compute the list of points in the selection 
 * which are to be considered for snapping */
{
        GSList * points = NULL, * l;
	ArtPoint * p;
	ArtDRect bbox;

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	l = selection->items;

	if (l == NULL) {
	  points = NULL;
	} else if (l->next == NULL) {
	  /* selection has only one item -> take snappoints of item */
	  points = sp_item_snappoints (SP_ITEM (l->data));
	} else {
	  /* selection has more than one item -> take corners of selection */
	  sp_selection_bbox (selection, &bbox);
	  p = g_new (ArtPoint,1);
	  p->x = bbox.x0;
	  p->y = bbox.y0;
	  points = g_slist_append (points, p);
	  p = g_new (ArtPoint,1);
	  p->x = bbox.x1;
	  p->y = bbox.y0;
	  points = g_slist_append (points, p);
	  p = g_new (ArtPoint,1);
	  p->x = bbox.x1;
	  p->y = bbox.y1;
	  points = g_slist_append (points, p);
	  p = g_new (ArtPoint,1);
	  p->x = bbox.x0;
	  p->y = bbox.y1;
	  points = g_slist_append (points, p);
	} 

	return points;
}
