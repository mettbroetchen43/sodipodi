#define KNOT_HOLDER_C
#define noKNOT_HOLDER_DEBUG


#include <glib.h>
#include "sp-item.h"
#include "sp-shape.h"
#include "knotholder.h"


static void knot_moved_handler (SPKnot      *knot,
				ArtPoint    *p,
				guint	state,
				gpointer	data);

#ifdef KNOT_HOLDER_DEBUG
#include <gtk/gtk.h>

static void
sp_knot_holder_debug (GtkObject *object,
		gpointer data)
{
	g_print ("sp-knot-holder-debug: [type=%s] [data=%s]\n",
		 gtk_type_name (GTK_OBJECT_TYPE(object)),
		 (const gchar *)data);
}
#endif

SPKnotHolder *
sp_knot_holder_new	(SPDesktop     *desktop,
			 SPItem	       *item)
{
	SPKnotHolder *knot_holder;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP(desktop), NULL);
	g_return_val_if_fail (item != NULL, NULL);
	g_return_val_if_fail (SP_IS_ITEM(item), NULL);

	knot_holder = g_new (SPKnotHolder, 1);
	knot_holder->desktop = desktop;
	gtk_object_ref (GTK_OBJECT (desktop));
	knot_holder->item    = item;
	gtk_object_ref (GTK_OBJECT (item));
	knot_holder->entity   = NULL;

#ifdef KNOT_HOLDER_DEBUG
	gtk_signal_connect (GTK_OBJECT(desktop), "destroy",
			    sp_knot_holder_debug, (gpointer)"SPKnotHolder::desktop");
	gtk_signal_connect (GTK_OBJECT(item), "destroy",
			    sp_knot_holder_debug, (gpointer)"SPKnotHolder::item");
#endif

	return knot_holder;
}

void
sp_knot_holder_destroy	(SPKnotHolder       *knot_holder)
{
	if (knot_holder) {
		GSList *el;

		gtk_object_unref (GTK_OBJECT (knot_holder->desktop));
		gtk_object_unref (GTK_OBJECT (knot_holder->item));

		for (el = knot_holder->entity; el; ) {
			SPKnotHolderEntity *e;

			e = (SPKnotHolderEntity *)el->data;
			/* unref should call destroy */
			gtk_object_unref (GTK_OBJECT(e->knot));
			g_free (e);
			el = g_slist_remove_link (el, el);
		}

		g_free (knot_holder);
	}
}

void
sp_knot_holder_add	(SPKnotHolder       *knot_holder,
			 SPKnotHolderSetFunc knot_set,
			 SPKnotHolderGetFunc knot_get)
{
	SPKnotHolderEntity *e;
	SPItem        *item;
	ArtPoint       p;
	gdouble        affine[6];
	GtkObject     *kob;

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
	knot_holder->entity = g_slist_append (knot_holder->entity, e);
	kob = GTK_OBJECT (e->knot);
#if 0	/* it's useless, if we don't use sink at all */
	gtk_object_ref (kob);
	gtk_object_sink (kob);
#endif

	/* move to current point */
	e->knot_get (item, &p);
	sp_item_i2d_affine(item, affine);
	art_affine_point (&p, &p, affine);
	sp_knot_set_position (e->knot, &p, SP_KNOT_STATE_NORMAL);

	e->handler_id = gtk_signal_connect (kob, "moved",
					    knot_moved_handler, knot_holder);

#ifdef KNOT_HOLDER_DEBUG
	gtk_signal_connect (kob, "destroy",
			    sp_knot_holder_debug, "SPKnotHolder::knot");
#endif
	sp_knot_show (e->knot);
}

#if 0
void
sp_knot_holder_select		(SPKnotHolder       *knot_holder)
{
	GSList *el;

	g_return_if_fail (knot_holder != NULL);
	
	for (el = knot_holder->entity; el; el = el->next) {
		sp_knot_show (SP_KNOT(e->data));
	}
}
#endif

static void
knot_moved_handler (SPKnot   *knot,
		    ArtPoint *p,
		    guint     state,
		    gpointer  data)
{
	SPKnotHolder *knot_holder;
	SPItem  *item;
	gdouble  affine[6];
	GSList  *el;

	knot_holder = (SPKnotHolder *)data;
	item  = SP_ITEM (knot_holder->item);

	for (el = knot_holder->entity; el; el = el->next) {
		SPKnotHolderEntity *e = (SPKnotHolderEntity *)el->data;
		if (e->knot == knot) {
/*  			sp_knot_position (knot, p, SP_KNOT_STATE_NORMAL); */

			sp_item_i2d_affine(item, affine);
			art_affine_invert (affine, affine);
			art_affine_point (p, p, affine);

			e->knot_set (item, p, state);

			break;
		}
	}
	
	sp_shape_set_shape (SP_SHAPE (item));
	
	sp_item_i2d_affine(item, affine);

	for (el = knot_holder->entity; el; el = el->next) {
		SPKnotHolderEntity *e = (SPKnotHolderEntity *)el->data;

		/*if (e->knot != knot)*/ {
			ArtPoint p1;
			GtkObject *kob;

			e->knot_get (item, &p1);

			art_affine_point (&p1, &p1, affine);

			kob = GTK_OBJECT (e->knot);
			gtk_signal_handler_block (kob, e->handler_id);
			sp_knot_set_position (e->knot, &p1,
					      SP_KNOT_STATE_NORMAL);
			gtk_signal_handler_unblock (kob, e->handler_id);
		}
	}
	sp_shape_set_shape (SP_SHAPE (item));
}

