#define __KNOT_HOLDER_C__

/*
 * Container for SPKnot visual handles
 *
 * Authors:
 *   Mitsuru Oka
 *
 * Copyright (C) 2001-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noKNOT_HOLDER_DEBUG

#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtksignal.h>
#include "document.h"
#include "desktop.h"
#include "sp-item.h"
#include "sp-shape.h"
#include "knotholder.h"

static void knot_moved_handler (SPKnot *knot, ArtPoint *p, guint state, gpointer data);
static void knot_ungrabbed_handler (SPKnot *knot, unsigned int state, SPKnotHolder *kh);

#ifdef KNOT_HOLDER_DEBUG
#include <gtk/gtk.h>

static void
sp_knot_holder_debug (GtkObject *object, gpointer data)
{
	g_print ("sp-knot-holder-debug: [type=%s] [data=%s]\n", gtk_type_name (GTK_OBJECT_TYPE(object)), (const gchar *)data);
}
#endif

SPKnotHolder *
sp_knot_holder_new (SPDesktop *desktop, SPItem *item, SPKnotHolderReleasedFunc relhandler)
{
	SPKnotHolder *knot_holder;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP(desktop), NULL);
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM(item), NULL);

	knot_holder = g_new (SPKnotHolder, 1);
	knot_holder->desktop = desktop;
	knot_holder->item = item;
	gtk_object_ref (GTK_OBJECT (item));
	knot_holder->entity = NULL;

	knot_holder->released = relhandler;

#ifdef KNOT_HOLDER_DEBUG
	gtk_signal_connect (GTK_OBJECT(desktop), "destroy",
			    sp_knot_holder_debug, (gpointer)"SPKnotHolder::item");
#endif

	return knot_holder;
}

void
sp_knot_holder_destroy	(SPKnotHolder *kh)
{
	if (kh) {
		gtk_object_unref (GTK_OBJECT (kh->item));
		while (kh->entity) {
			SPKnotHolderEntity *e;
			e = (SPKnotHolderEntity *) kh->entity->data;
			/* unref should call destroy */
			gtk_object_unref (GTK_OBJECT (e->knot));
			g_free (e);
			kh->entity = g_slist_remove (kh->entity, e);
		}

		g_free (kh);
	}
}

void
sp_knot_holder_add (SPKnotHolder *knot_holder, SPKnotHolderSetFunc knot_set, SPKnotHolderGetFunc knot_get)
{
	sp_knot_holder_add_full (knot_holder, knot_set, knot_get, SP_KNOT_SHAPE_DIAMOND, SP_KNOT_MODE_COLOR);
}

void
sp_knot_holder_add_full	(SPKnotHolder       *knot_holder,
			 SPKnotHolderSetFunc knot_set,
			 SPKnotHolderGetFunc knot_get,
			 SPKnotShapeType     shape,
			 SPKnotModeType      mode)
{
	SPKnotHolderEntity *e;
	SPItem        *item;
	ArtPoint       p;
	gdouble        affine[6];
	GtkObject     *ob;

	g_return_if_fail (knot_holder != NULL);
	g_return_if_fail (knot_set != NULL);
	g_return_if_fail (knot_get != NULL);
	
	item = SP_ITEM (knot_holder->item);
#if 0
#define KH_EPSILON 1e-6
	/* Precondition for knot_set and knot_get */
	{
		ArtPoint p1, p2;
		knot_get (item, &p1);
		knot_set (item, &p1, 0);
		knot_get (item, &p2);
		g_assert (fabs(p1.x - p2.x) < KH_EPSILON &&
			  fabs(p1.y - p2.y) < KH_EPSILON);
	}
#endif
	/* create new SPKnotHolderEntry */
	e = g_new (SPKnotHolderEntity, 1);
	e->knot = sp_knot_new (knot_holder->desktop);
	e->knot_set = knot_set;
	e->knot_get = knot_get;
	ob = GTK_OBJECT (e->knot->item);
	gtk_object_set (ob, "shape", shape, NULL);
	gtk_object_set (ob, "mode", mode, NULL);
	knot_holder->entity = g_slist_append (knot_holder->entity, e);

	ob = GTK_OBJECT (e->knot);
	/* move to current point */
	e->knot_get (item, &p);
	sp_item_i2d_affine(item, affine);
	art_affine_point (&p, &p, affine);
	sp_knot_set_position (e->knot, &p, SP_KNOT_STATE_NORMAL);

	e->handler_id = gtk_signal_connect (ob, "moved", knot_moved_handler, knot_holder);

	gtk_signal_connect (ob, "ungrabbed", GTK_SIGNAL_FUNC (knot_ungrabbed_handler), knot_holder);

#ifdef KNOT_HOLDER_DEBUG
	gtk_signal_connect (ob, "destroy",
			    sp_knot_holder_debug, "SPKnotHolder::knot");
#endif
	sp_knot_show (e->knot);
}

static void
knot_moved_handler (SPKnot *knot, ArtPoint *p, guint state, gpointer data)
{
	SPKnotHolder *knot_holder;
	SPItem *item;
	SPObject *object;
	gdouble affine[6];
	GSList *el;

	knot_holder = (SPKnotHolder *) data;
	item  = SP_ITEM (knot_holder->item);
	object = SP_OBJECT (item);

	for (el = knot_holder->entity; el; el = el->next) {
		SPKnotHolderEntity *e = (SPKnotHolderEntity *)el->data;
		if (e->knot == knot) {
			ArtPoint q;
			double d2i[6];

			sp_item_i2d_affine(item, affine);
			art_affine_invert (d2i, affine);
			art_affine_point (&q, p, d2i);

			e->knot_set (item, &q, state);

			break;
		}
	}
	
	sp_shape_set_shape (SP_SHAPE (item));

	sp_object_invoke_write (object, object->repr, SP_OBJECT_WRITE_SODIPODI);
	
	sp_item_i2d_affine(item, affine);

	for (el = knot_holder->entity; el; el = el->next) {
		SPKnotHolderEntity *e = (SPKnotHolderEntity *)el->data;
		ArtPoint p1;
		GtkObject *kob;
		
		kob = GTK_OBJECT (e->knot);

		e->knot_get (item, &p1);
		
		art_affine_point (&p1, &p1, affine);

		gtk_signal_handler_block (kob, e->handler_id);
		sp_knot_set_position (e->knot, &p1,
				      SP_KNOT_STATE_NORMAL);
		gtk_signal_handler_unblock (kob, e->handler_id);
	}
}

static void
knot_ungrabbed_handler (SPKnot *knot, unsigned int state, SPKnotHolder *kh)
{
	if (kh->released) {
		kh->released (kh->item);
	} else {
		sp_document_done (SP_OBJECT_DOCUMENT (kh->item));
	}
}

