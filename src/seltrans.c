#define __SP_SELTRANS_C__

/*
 * Helper object for transforming selected items
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <gdk/gdkkeysyms.h>

#include "svg/svg.h"
#include "sodipodi-private.h"
#include "sodipodi.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "selection.h"
#include "select-context.h"
#include "sp-item.h"
#include "seltrans-handles.h"
#include "seltrans.h"
#include "sp-metrics.h"
#include "helper/sp-ctrlline.h"

static void sp_sel_trans_update_handles (SPSelTrans * seltrans);
static void sp_sel_trans_update_volatile_state (SPSelTrans * seltrans);

static void sp_remove_handles (SPKnot * knot[], gint num);
static void sp_show_handles (SPSelTrans * seltrans, SPKnot * knot[], const SPSelTransHandle handle[], gint num);

static void sp_sel_trans_handle_grab (SPKnot * knot, guint state, gpointer data);
static void sp_sel_trans_handle_ungrab (SPKnot * knot, guint state, gpointer data);
static void sp_sel_trans_handle_new_event (SPKnot * knot, ArtPoint * position, guint32 state, gpointer data);
static gboolean sp_sel_trans_handle_request (SPKnot * knot, ArtPoint * p, guint state, gboolean * data);

static void sp_sel_trans_sel_changed (SPSelection * selection, gpointer data);
static void sp_sel_trans_sel_modified (SPSelection * selection, guint flags, gpointer data);

extern GdkCursor * CursorSelectMouseover, * CursorSelectDragging;
extern GdkPixbuf * handles[];

static gboolean
sp_seltrans_handle_event (SPKnot *knot, GdkEvent *event, gpointer data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	
	switch (event->type) {
	case GDK_MOTION_NOTIFY:
		break;
	case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_space) {
			/* stamping mode: both mode(show content and outline) operation with knot */
			if (!SP_KNOT_IS_GRABBED (knot)) return FALSE;
			desktop = knot->desktop;
			seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;
			sp_sel_trans_stamp (seltrans);
			return TRUE;
		}
		break;
	default:
		g_print ("Event type %d occured\n", event->type);
		break;
	}

	return FALSE;
}

void
sp_sel_trans_init (SPSelTrans * seltrans, SPDesktop * desktop)
{
	gint i;

	g_return_if_fail (seltrans != NULL);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	seltrans->desktop = desktop;

	seltrans->state = SP_SELTRANS_STATE_SCALE;
	seltrans->show = SP_SELTRANS_SHOW_CONTENT;
	seltrans->transform = SP_SELTRANS_TRANSFORM_OPTIMIZE;

	seltrans->grabbed = FALSE;
	seltrans->show_handles = TRUE;
	for (i = 0; i < 8; i++) seltrans->shandle[i] = NULL;
	for (i = 0; i < 8; i++) seltrans->rhandle[i] = NULL;
	seltrans->chandle = NULL;

	sp_sel_trans_update_volatile_state (seltrans);
	
	seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
	seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;

	sp_sel_trans_update_handles (seltrans);

	seltrans->selection = SP_DT_SELECTION (desktop);
	gtk_signal_connect (GTK_OBJECT (seltrans->selection), "changed", GTK_SIGNAL_FUNC (sp_sel_trans_sel_changed), seltrans);
	gtk_signal_connect (GTK_OBJECT (seltrans->selection), "modified", GTK_SIGNAL_FUNC (sp_sel_trans_sel_modified), seltrans);

	seltrans->norm = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
		SP_TYPE_CTRL,
		"anchor", GTK_ANCHOR_CENTER,
		"mode", SP_CTRL_MODE_COLOR,
		"shape", SP_CTRL_SHAPE_BITMAP,
		"size", 13.0,
		"filled", TRUE,
		"fill_color", 0x40ff40a0,
		"stroked", TRUE,
		"stroke_color", 0x000000a0,
		"pixbuf", handles[12],				
		NULL);
	seltrans->grip = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
		SP_TYPE_CTRL,
		"anchor", GTK_ANCHOR_CENTER,
		"mode", SP_CTRL_MODE_XOR,
		"shape", SP_CTRL_SHAPE_CROSS,
		"size", 7.0,
		"filled", TRUE,
		"fill_color", 0xff40ffa0,
		"stroked", TRUE,
		"stroke_color", 0xffffffFF,
		"pixbuf", handles[12],				
		NULL);
	gnome_canvas_item_hide (seltrans->grip);
	gnome_canvas_item_hide (seltrans->norm);

	seltrans->l1 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop), SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (seltrans->l1);
	seltrans->l2 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop), SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (seltrans->l2);
	seltrans->l3 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop), SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (seltrans->l3);
	seltrans->l4 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop), SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (seltrans->l4);
}

void
sp_sel_trans_shutdown (SPSelTrans *seltrans)
{
	gint i;
#if 0
	seltrans->show_handles = FALSE;

	sp_sel_trans_update_handles (seltrans);
#else
	for (i = 0; i < 8; i++) {
		if (seltrans->shandle[i]) {
			gtk_object_unref (GTK_OBJECT (seltrans->shandle[i]));
			seltrans->shandle[i] = NULL;
		}
		if (seltrans->rhandle[i]) {
			gtk_object_unref (GTK_OBJECT (seltrans->rhandle[i]));
			seltrans->rhandle[i] = NULL;
		}
	}
	if (seltrans->chandle) {
		gtk_object_unref (GTK_OBJECT (seltrans->chandle));
		seltrans->chandle = NULL;
	}
#endif

	if (seltrans->norm) {
		gtk_object_destroy (GTK_OBJECT (seltrans->norm));
		seltrans->norm = NULL;
	}
	if (seltrans->grip) {
		gtk_object_destroy (GTK_OBJECT (seltrans->grip));
		seltrans->grip = NULL;
	}
	if (seltrans->l1) {
		gtk_object_destroy (GTK_OBJECT (seltrans->l1));
		seltrans->l1 = NULL;
	}
	if (seltrans->l2) {
		gtk_object_destroy (GTK_OBJECT (seltrans->l2));
		seltrans->l2 = NULL;
	}
	if (seltrans->l3) {
		gtk_object_destroy (GTK_OBJECT (seltrans->l3));
		seltrans->l3 = NULL;
	}
	if (seltrans->l4) {
		gtk_object_destroy (GTK_OBJECT (seltrans->l4));
		seltrans->l4 = NULL;
	}

	if (seltrans->selection) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (seltrans->selection), seltrans);
	}
}

void
sp_sel_trans_reset_state (SPSelTrans * seltrans)
{
	seltrans->state = SP_SELTRANS_STATE_SCALE;
}

void
sp_sel_trans_increase_state (SPSelTrans * seltrans)
{
	if (seltrans->state == SP_SELTRANS_STATE_SCALE) {
		seltrans->state = SP_SELTRANS_STATE_ROTATE;
	} else {
		seltrans->state = SP_SELTRANS_STATE_SCALE;
	}

	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_set_center (SPSelTrans * seltrans, gdouble x, gdouble y)
{
	seltrans->center.x = x;
	seltrans->center.y = y;

	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_grab (SPSelTrans * seltrans, NRPointF *p, gdouble x, gdouble y, gboolean show_handles)
{
	SPSelection * selection;

	selection = SP_DT_SELECTION (seltrans->desktop);

	g_return_if_fail (!seltrans->grabbed);

	seltrans->grabbed = TRUE;
	seltrans->show_handles = show_handles;
	sp_sel_trans_update_volatile_state (seltrans);

	seltrans->changed = FALSE;
	seltrans->sel_changed = FALSE;

	if (seltrans->empty) return;

	art_affine_identity (seltrans->current);

	seltrans->point.x = p->x;
	seltrans->point.y = p->y;

	seltrans->snappoints = sp_selection_snappoints (selection);

	seltrans->opposit.x = seltrans->box.x0 + (1 - x) * fabs (seltrans->box.x1 - seltrans->box.x0);
	seltrans->opposit.y = seltrans->box.y0 + (1 - y) * fabs (seltrans->box.y1 - seltrans->box.y0);

	if ((x != -1) && (y != -1)) {
		gnome_canvas_item_show (seltrans->norm);
		gnome_canvas_item_show (seltrans->grip);
	}

	if (seltrans->show == SP_SELTRANS_SHOW_OUTLINE) {
		gnome_canvas_item_show (seltrans->l1);
		gnome_canvas_item_show (seltrans->l2);
		gnome_canvas_item_show (seltrans->l3);
		gnome_canvas_item_show (seltrans->l4);
	}


	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_transform (SPSelTrans * seltrans, gdouble affine[], ArtPoint * norm)
{
	SPItem * item;
	const GSList * l;
	gdouble current_[6], i2d[6], i2current[6], i2dnew[6], n2p[6], p2n[6];
	ArtPoint p1, p2, p3, p4;
	gint i;

	g_return_if_fail (seltrans->grabbed);
	g_return_if_fail (!seltrans->empty);

	art_affine_invert (current_, seltrans->current);
	art_affine_translate (n2p, norm->x, norm->y);
	art_affine_invert (p2n, n2p);
	art_affine_multiply (affine, p2n, affine);
	art_affine_multiply (affine, affine, n2p);

	if (seltrans->show == SP_SELTRANS_SHOW_CONTENT) {
	        // update the content

         	/* We accept empty lists here, as something may well remove items */
	        /* and seltrans does not update, if frozen */
	        l = sp_selection_item_list (SP_DT_SELECTION (seltrans->desktop));
	        while (l != NULL) {
		        item = SP_ITEM (l->data);
			sp_item_i2d_affine (item, i2d);
			art_affine_multiply (i2current, i2d, current_);
			art_affine_multiply (i2dnew, i2current, affine);
			sp_item_set_i2d_affine (item, i2dnew);
			l = l->next;
		}
	} else {
        	// update the outline
                p1.x = seltrans->box.x0;
		p1.y = seltrans->box.y0;
		p2.x = seltrans->box.x0;
		p2.y = seltrans->box.y1;
		p3.x = seltrans->box.x1;
		p3.y = seltrans->box.y1;
		p4.x = seltrans->box.x1;
		p4.y = seltrans->box.y0;
		art_affine_point (&p1, &p1, affine);
		art_affine_point (&p2, &p2, affine);
		art_affine_point (&p3, &p3, affine);
		art_affine_point (&p4, &p4, affine);
		sp_ctrlline_set_coords (SP_CTRLLINE (seltrans->l1), p1.x, p1.y, p2.x, p2.y);
		sp_ctrlline_set_coords (SP_CTRLLINE (seltrans->l2), p2.x, p2.y, p3.x, p3.y);
		sp_ctrlline_set_coords (SP_CTRLLINE (seltrans->l3), p3.x, p3.y, p4.x, p4.y);
		sp_ctrlline_set_coords (SP_CTRLLINE (seltrans->l4), p4.x, p4.y, p1.x, p1.y);
	}

	for (i = 0; i < 6; i++) seltrans->current[i] = affine[i];

	seltrans->changed = TRUE;

	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_ungrab (SPSelTrans * seltrans)
{
	SPItem * item;
	const GSList * l;
	gchar tstr[80];
	ArtPoint p;
	gdouble i2d[6], i2dnew[6];

	g_return_if_fail (seltrans->grabbed);

	if (!seltrans->empty && seltrans->changed) {
		l = sp_selection_item_list (SP_DT_SELECTION (seltrans->desktop));

		tstr[79] = '\0';

		while (l != NULL) {
			item = SP_ITEM (l->data);
			/* fixme: We do not have to set it here (Lauris) */
			if (seltrans->show == SP_SELTRANS_SHOW_OUTLINE) {
				sp_item_i2d_affine (item, i2d);
				art_affine_multiply (i2dnew, i2d, seltrans->current);
				sp_item_set_i2d_affine (item, i2dnew);
			}
			if (seltrans->transform == SP_SELTRANS_TRANSFORM_OPTIMIZE) {
				sp_item_write_transform (item, SP_OBJECT_REPR (item), item->affine);
				/* because item/repr affines may be out of sync, invoke reread */
				/* fixme: We should test equality to avoid unnecessary rereads */
				/* fixme: This probably is not needed (Lauris) */
				sp_object_invoke_read_attr (SP_OBJECT (item), "transform");
			} else {
				if (sp_svg_write_affine (tstr, 79, item->affine)) {
					sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
				} else {
					sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
				}
			}
			l = l->next;
		}
		p.x = seltrans->center.x;
		p.y = seltrans->center.y;
		art_affine_point (&p, &p, seltrans->current);
		seltrans->center.x = p.x;
		seltrans->center.y = p.y;
		
		sp_document_done (SP_DT_DOCUMENT (seltrans->desktop));
		sp_selection_changed (SP_DT_SELECTION (seltrans->desktop));
	}

	seltrans->grabbed = FALSE;
	seltrans->show_handles = TRUE;
	
	/* free snappoints */
	while (seltrans->snappoints) {
		g_free (seltrans->snappoints->data);
		seltrans->snappoints = g_slist_remove (seltrans->snappoints, seltrans->snappoints->data);
	}

#if 0
	if (seltrans->sel_changed) {
		seltrans->state = SP_SEL_TRANS_SCALE;
	}
#endif

	gnome_canvas_item_hide (seltrans->norm);
	gnome_canvas_item_hide (seltrans->grip);

        if (seltrans->show == SP_SELTRANS_SHOW_OUTLINE) {
		gnome_canvas_item_hide (seltrans->l1);
		gnome_canvas_item_hide (seltrans->l2);
		gnome_canvas_item_hide (seltrans->l3);
		gnome_canvas_item_hide (seltrans->l4);
	}

	sp_sel_trans_update_volatile_state (seltrans);
	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_stamp (SPSelTrans * seltrans)
{
	/* stamping mode */
	SPItem * original_item, * copy_item;
	SPRepr * original_repr, * copy_repr;
	const GSList * l;

	gchar tstr[80];
	gdouble i2d[6], i2dnew[6];
	
	gdouble * new_affine;

	tstr[79] = '\0';
	
	if (!seltrans->empty) {
		l = sp_selection_item_list (SP_DT_SELECTION (seltrans->desktop));
		while (l) {
			original_item = SP_ITEM(l->data);
			original_repr = (SPRepr *)(SP_OBJECT (original_item)->repr);
			copy_repr = sp_repr_duplicate (original_repr);
			copy_item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (seltrans->desktop), 
								     copy_repr);
			
			if (seltrans->show == SP_SELTRANS_SHOW_OUTLINE) {
				sp_item_i2d_affine (original_item, i2d);
				art_affine_multiply (i2dnew, i2d, seltrans->current);
				sp_item_set_i2d_affine (copy_item, i2dnew);
				new_affine = copy_item->affine;
			} else 
				new_affine = original_item->affine;

			if (sp_svg_write_affine (tstr, 79, new_affine)) 
				sp_repr_set_attr (copy_repr, "transform", tstr);
			else
				sp_repr_set_attr (copy_repr, "transform", NULL);
			sp_repr_unref (copy_repr);
			l = l->next;
		}
		sp_document_done (SP_DT_DOCUMENT (seltrans->desktop));
	}
}

ArtPoint *
sp_sel_trans_point_desktop (SPSelTrans * seltrans, ArtPoint * p)
{
	g_return_val_if_fail (p != NULL, NULL);

	p->x = seltrans->point.x;
	p->y = seltrans->point.y;

	return p;
}

ArtPoint *
sp_sel_trans_origin_desktop (SPSelTrans * seltrans, ArtPoint * p)
{
	g_return_val_if_fail (p != NULL, NULL);

	p->x = seltrans->origin.x;
	p->y = seltrans->origin.y;

	return p;
}

static void
sp_sel_trans_update_handles (SPSelTrans * seltrans)
{
	g_return_if_fail (seltrans != NULL);

      	if ((!seltrans->show_handles) || (seltrans->empty)) {
		sp_remove_handles (seltrans->shandle, 8);
		sp_remove_handles (seltrans->rhandle, 8);
		sp_remove_handles (&seltrans->chandle, 1);
		return;
	}
	
	if (seltrans->state == SP_SELTRANS_STATE_SCALE) {
		sp_remove_handles (seltrans->rhandle, 8);
		sp_remove_handles (&seltrans->chandle, 1);
		sp_show_handles (seltrans, seltrans->shandle, handles_scale, 8);
	} else {
		sp_remove_handles (seltrans->shandle, 8);
		sp_remove_handles (&seltrans->chandle, 1);
		sp_show_handles (seltrans, seltrans->rhandle, handles_rotate, 8);
	}

	// center handle
	if (seltrans->chandle == NULL) {
	  seltrans->chandle = sp_knot_new (seltrans->desktop);
	  gtk_object_set (GTK_OBJECT (seltrans->chandle),
			  "anchor", handle_center.anchor, 
			  "shape", SP_CTRL_SHAPE_BITMAP,
			  "size", 13,
			  "mode", SP_CTRL_MODE_COLOR,
					"fill", 0x40ff40a0,
					"fill_mouseover", 0xff4040f0,
					"stroke", 0x000000a0,
					"stroke_mouseover", 0x000000FF,
			  //"fill", 0xff40ffa0,
			  //"fill_mouseover", 0x40ffffa0,
			  //"stroke", 0xFFb0b0ff,
			  //"stroke_mouseover", 0xffffffFF,
			  "pixbuf", handles[handle_center.control],
			  "cursor_mouseover", CursorSelectMouseover,
			  "cursor_dragging", CursorSelectDragging,
			  NULL);
	  gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "request",
			      GTK_SIGNAL_FUNC (sp_sel_trans_handle_request), (gpointer) &handle_center);
	  gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "moved",
	  		      GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), (gpointer) &handle_center);
	  gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "grabbed",
			      GTK_SIGNAL_FUNC (sp_sel_trans_handle_grab), (gpointer) &handle_center);
	  gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "ungrabbed",
			      GTK_SIGNAL_FUNC (sp_sel_trans_handle_ungrab), (gpointer) &handle_center);
	}
	sp_knot_show (seltrans->chandle);
	sp_knot_set_position (seltrans->chandle, &seltrans->center, 0);
}

static void
sp_sel_trans_update_volatile_state (SPSelTrans * seltrans)
{
	SPSelection * selection;

	g_return_if_fail (seltrans != NULL);

	selection = SP_DT_SELECTION (seltrans->desktop);

	seltrans->empty = sp_selection_is_empty (selection);

	if (seltrans->empty) return;

	sp_selection_bbox (SP_DT_SELECTION (seltrans->desktop), &seltrans->box);

	if ((seltrans->box.x0 > seltrans->box.x1) || (seltrans->box.y0 > seltrans->box.y1)) {
		seltrans->empty = TRUE;
		return;
	}

	art_affine_identity (seltrans->current);
}

static void
sp_remove_handles (SPKnot * knot[], gint num)
{
	gint i;

	for (i = 0; i < num; i++) {
		if (knot[i] != NULL) {
			sp_knot_hide (knot[i]);
		}
	}
}

static void
sp_show_handles (SPSelTrans * seltrans, SPKnot * knot[], const SPSelTransHandle handle[], gint num)
{
	ArtPoint p;
	gint i;

	for (i = 0; i < num; i++) {
		if (knot[i] == NULL) {
			knot[i] = sp_knot_new (seltrans->desktop);
			gtk_object_set (GTK_OBJECT (knot[i]),
					"anchor", handle[i].anchor, 
					"shape", SP_CTRL_SHAPE_BITMAP,
					"size", 13,
					"mode", SP_KNOT_MODE_COLOR,
					"fill", 0x4040ffa0,
					"fill_mouseover", 0xff4040f0,
					"stroke", 0x000000a0,
					"stroke_mouseover", 0x000000FF,
					//"fill", 0xffff0080,
					//"fill_mouseover", 0x00ffff80,
					//"stroke", 0xFFFFFFff,
					//"stroke_mouseover", 0xb0b0b0FF,
					"pixbuf", handles[handle[i].control],
					"cursor_mouseover", CursorSelectMouseover,
					"cursor_dragging", CursorSelectDragging,
					NULL);

			gtk_signal_connect (GTK_OBJECT (knot[i]), "request",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_request), (gpointer) &handle[i]);
 			gtk_signal_connect (GTK_OBJECT (knot[i]), "moved",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), (gpointer) &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "grabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_grab), (gpointer) &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "ungrabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_ungrab), (gpointer) &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "event", GTK_SIGNAL_FUNC (sp_seltrans_handle_event), (gpointer) &handle[i]);
		}
		sp_knot_show (knot[i]);

		p.x = seltrans->box.x0 + handle[i].x * fabs (seltrans->box.x1 - seltrans->box.x0);
		p.y = seltrans->box.y0 + handle[i].y * fabs (seltrans->box.y1 - seltrans->box.y0);

		//gtk_signal_handler_block_by_func (GTK_OBJECT (knot[i]),
		//				  GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), &handle[i]);
		sp_knot_set_position (knot[i], &p, 0);	    
		//gtk_signal_handler_unblock_by_func (GTK_OBJECT (knot[i]),
		//				    GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), &handle[i]);

	}
}

static void
sp_sel_trans_handle_grab (SPKnot * knot, guint state, gpointer data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	SPSelTransHandle * handle;
	ArtPoint p;
	NRPointF pf;

	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;
	handle = (SPSelTransHandle *) data;

	switch (handle->anchor) {
	case GTK_ANCHOR_CENTER:
	  gtk_object_set (GTK_OBJECT (seltrans->grip),
			  "shape", SP_CTRL_SHAPE_BITMAP,
			  "size", 13.0,
			  NULL);
	  gnome_canvas_item_show (seltrans->grip);
	  break;
	default:
	  gtk_object_set (GTK_OBJECT (seltrans->grip),
			  "shape", SP_CTRL_SHAPE_CROSS,
			  "size", 7.0,
			  NULL);
	  gnome_canvas_item_show (seltrans->norm);
	  gnome_canvas_item_show (seltrans->grip);

	  break;
	}

	sp_knot_position (knot, &p);

	pf.x = p.x;
	pf.y = p.y;
	sp_sel_trans_grab (seltrans, &pf, handle->x, handle->y, FALSE);
}

static void
sp_sel_trans_handle_ungrab (SPKnot * knot, guint state, gpointer data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	
	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;

	sp_sel_trans_ungrab (seltrans);
	//sp_selection_changed (SP_DT_SELECTION (seltrans->desktop));
	
}

static void
sp_sel_trans_handle_new_event (SPKnot * knot, ArtPoint * position, guint state, gpointer data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	SPSelTransHandle * handle;

	if (!SP_KNOT_IS_GRABBED (knot)) return;

	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;
	handle = (SPSelTransHandle *) data;
	handle->action (seltrans, handle, position, state);

	//sp_desktop_coordinate_status (desktop, position->x, position->y, 4);
}

/* fixme: Highly experimental test :) */

static gboolean
sp_sel_trans_handle_request (SPKnot * knot, ArtPoint * position, guint state, gboolean * data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	SPSelTransHandle * handle;
	ArtPoint s, point;

	if (!SP_KNOT_IS_GRABBED (knot)) return TRUE;

	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;
	handle = (SPSelTransHandle *) data;

	sp_desktop_set_coordinate_status (desktop, position->x, position->y, 0);

	if (state & GDK_MOD1_MASK) {
	  sp_sel_trans_point_desktop (seltrans, &point);
	  position->x = point.x + (position->x - point.x) / 10;
	  position->y = point.y + (position->y - point.y) / 10;
	}
	if (state & GDK_SHIFT_MASK) seltrans->origin = seltrans->center;
	else seltrans->origin = seltrans->opposit;

	if (handle->request (seltrans, handle, position, state)) {
	  sp_knot_set_position (knot, position, state);
	  s.x = rint (position->x);
	  s.y = rint (position->y);	  
	  sp_ctrl_moveto (SP_CTRL (seltrans->grip), s.x, s.y);
	  sp_ctrl_moveto (SP_CTRL (seltrans->norm), seltrans->origin.x, seltrans->origin.y);
	}
	
	return TRUE;

}

static void
sp_sel_trans_sel_changed (SPSelection * selection, gpointer data)
{
	SPSelTrans * seltrans;

	seltrans = (SPSelTrans *) data;

	if (seltrans->grabbed) {
		seltrans->sel_changed = TRUE;
	} else {
		sp_sel_trans_update_volatile_state (seltrans);
		seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
		seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;
		sp_sel_trans_update_handles (seltrans);
	}

}

static void
sp_sel_trans_sel_modified (SPSelection *selection, guint flags, gpointer data)
{
	SPSelTrans *seltrans;

	seltrans = (SPSelTrans *) data;

	if (seltrans->grabbed) {
		seltrans->sel_changed = TRUE;
	} else {
		sp_sel_trans_update_volatile_state (seltrans);
		seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
		seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;
		sp_sel_trans_update_handles (seltrans);
	}

}

/*
 * handlers for handle move-request
 */

gboolean
sp_sel_trans_scale_request (SPSelTrans *seltrans, SPSelTransHandle *handle, ArtPoint *p, guint state)
{
	ArtPoint norm, point;
	gdouble sx, sy;
	gchar status[80];
	SPDesktop *desktop;
	int xd, yd;

	desktop = seltrans->desktop;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	if (fabs (point.x - norm.x) > 0.0625) {
		sx = (p->x - norm.x) / (point.x - norm.x);
		if (fabs (sx) < 1e-9) sx = 1e-9;
		xd = TRUE;
	} else {
		sx = 0.0;
		xd = FALSE;
	}
	if (fabs (point.y - norm.y) > 0.0625) {
		sy = (p->y - norm.y) / (point.y - norm.y);
		if (fabs (sy) < 1e-9) sy = 1e-9;
		yd = TRUE;
	} else {
		sy = 0.0;
		yd = FALSE;
	}

	if (state & GDK_CONTROL_MASK) {
		double r;
		if (!xd || !yd) return FALSE;
	        if (fabs (sy) > fabs (sx)) sy = fabs (sx) * sy / fabs (sy);
		if (fabs (sx) > fabs (sy)) sx = fabs (sy) * sx / fabs (sx);
		r = sp_desktop_vector_snap_list (desktop, seltrans->snappoints, &norm, sx, sy);
		sx = fabs (r) * sx / fabs (sx);
		sy = fabs (r) * sy / fabs (sy);
	} else {
		if (xd) sx = sp_desktop_horizontal_snap_list_scale (desktop, seltrans->snappoints, &norm, sx);
		if (yd) sy = sp_desktop_vertical_snap_list_scale (desktop, seltrans->snappoints, &norm, sy);
	}

	p->x = (point.x - norm.x) * sx + norm.x;
	p->y = (point.y - norm.y) * sy + norm.y;

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * sx, '%', 100 * sy, '%');
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);

	return TRUE;
}

gboolean
sp_sel_trans_stretch_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	double sx = 1.0, sy = 1.0, ratio;
	gchar status[80];	
	ArtPoint norm, point;
	SPDesktop * desktop;

	desktop = seltrans->desktop;
 
	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	switch (handle->cursor) {
	case GDK_TOP_SIDE:
	case GDK_BOTTOM_SIDE:
		if (fabs (point.y - norm.y) < 1e-15) return FALSE;
		sy = (p->y - norm.y) / (point.y - norm.y);	    
		if (fabs (sy) < 1e-15) sy = 1e-15;
		if (state & GDK_CONTROL_MASK) {
			if (fabs (sy) > 1) sy = sy / fabs (sy);
			sx = fabs (sy);
			ratio = sp_desktop_vector_snap_list (desktop, seltrans->snappoints, &norm, sx, sy);
			sy = (fabs (ratio) < 1) ? fabs (ratio) * sy / fabs (sy) : sy / fabs (sy);
			sx = fabs (sy);
		} else {
			sy = sp_desktop_vertical_snap_list_scale (desktop, seltrans->snappoints, &norm, sy); 
		}
		break;
	case GDK_LEFT_SIDE:
	case GDK_RIGHT_SIDE:
		if (fabs (point.x - norm.x) < 1e-5) return FALSE;
		sx = (p->x - norm.x) / (point.x - norm.x);
		if (fabs (sx) < 1e-5) sx = 1e-15;
		if (state & GDK_CONTROL_MASK) {
			if (fabs (sx) > 1) sx = sx / fabs (sx);
			sy = fabs (sx);
			ratio = sp_desktop_vector_snap_list (desktop, seltrans->snappoints, &norm, sx, sy);
			sx = (fabs (sx) < 1) ? fabs (ratio) * sx / fabs (sx) : sx / fabs (sx);
			sy = fabs (ratio);
		} else {
			sx = sp_desktop_horizontal_snap_list_scale (desktop, seltrans->snappoints, &norm, sx);
		}
		break;
	default:
		break;
	}

	p->x = (point.x - norm.x)* sx + norm.x;
	p->y = (point.y - norm.y)* sy + norm.y;

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * sx, '%', 100 * sy, '%');
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);

	return TRUE;
}

gboolean
sp_sel_trans_skew_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
        double skew[6], sx = 1.0, sy = 1.0;
        gchar status[80];
	ArtPoint norm, point;
	SPDesktop * desktop;

	desktop = seltrans->desktop;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

  
	skew[4]=0;
	skew[5]=0;
	skew[0]=1;
	skew[3]=1;

	switch (handle->cursor) {
	case GDK_SB_V_DOUBLE_ARROW:
	  if (fabs (point.x - norm.x) < 1e-15) return FALSE;
	  skew[1] = (p->y - point.y) / (point.x - norm.x);
	  skew[1] = sp_desktop_vertical_snap_list_skew (desktop, seltrans->snappoints, &norm, skew[1]);
	  p->y = (point.x - norm.x) * skew[1] + point.y; 
	  sx = (p->x - norm.x) / (point.x - norm.x);
	  if (state & GDK_CONTROL_MASK) {
	    sx = sp_desktop_horizontal_snap_list_scale (desktop, seltrans->snappoints, &norm, sx);
	  } else {
	    if (fabs (sx) < 1e-15) sx = 1e-15;
	    if (fabs (sx) < 1) sx = fabs (sx) / sx;
	    sx = (double)((int)(sx + 0.5*(fabs(sx)/sx)));
	  }
	  if (fabs (sx) < 1e-15) sx = 1e-15;
	  p->x = (point.x - norm.x) * sx + norm.x;
	  
	  skew[2] = 0;
	  break;
	case GDK_SB_H_DOUBLE_ARROW:
	  if (fabs (point.y - norm.y) < 1e-15) return FALSE;
	  skew[2] = (p->x - point.x) / (point.y - norm.y);
	  skew[2] = sp_desktop_horizontal_snap_list_skew (desktop, seltrans->snappoints, &norm, skew[2]);
	  p->x = (point.y - norm.y) * skew[2] + point.x; 
	  sy = (p->y - norm.y) / (point.y - norm.y);
	  
	  if (state & GDK_CONTROL_MASK) {
	    sy = sp_desktop_vertical_snap_list_scale (desktop, seltrans->snappoints, &norm, sy);
	  } else {
	    if (fabs (sy) < 1e-15) sy = 1e-15;
	    if (fabs (sy) < 1) sy = fabs (sy) / sy;
	    sy = (double)((int)(sy + 0.5*(fabs(sy)/sy)));
	  }
	  if (fabs (sy) < 1e-15) sy = 1e-15;
	  p->y = (point.y - norm.y) * sy + norm.y;
	  
	  skew[1] = 0;
	  break;
	default:
		break;
	}
	// status text
	sprintf (status, "Skew  %0.2f%c %0.2f%c", 100 * fabs(skew[2]), '%', 100 * fabs(skew[1]), '%');
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);

	return TRUE;
}

gboolean
sp_sel_trans_rotate_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	ArtPoint norm, point, q1, q2;
	gdouble h1, h2, r1[6], r2[6], rotate[6], n2p[6], p2n[6], dx1, dx2, dy1, dy2, sg, angle;
	SPDesktop * desktop;
	gchar status[80];

	desktop = seltrans->desktop;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);
	
	// rotate affine in rotate
	dx1 = point.x - norm.x;
	dy1 = point.y - norm.y;
	dx2 = p->x    - norm.x;
	dy2 = p->y    - norm.y;
	h1 = hypot (dx1, dy1);
	if (fabs (h1) < 1e-15) return FALSE;
	q1.x = (dx1) / h1;
	q1.y = (dy1) / h1;
	h2 = hypot (dx2, dy2);
	if (fabs (h2) < 1e-15) return FALSE;
	q2.x = (dx2) / h2;
	q2.y = (dy2) / h2;
	r1[0] = q1.x;  r1[1] = -q1.y;  r1[2] =  q1.y;  r1[3] = q1.x;  r1[4] = 0;  r1[5] = 0;
	r2[0] = q2.x;  r2[1] =  q2.y;  r2[2] = -q2.y;  r2[3] = q2.x;  r2[4] = 0;  r2[5] = 0;
	art_affine_translate (n2p, norm.x, norm.y);
	art_affine_invert (p2n, n2p);
	art_affine_multiply (rotate, p2n, r1);
	art_affine_multiply (rotate, rotate, r2);
	art_affine_multiply (rotate, rotate, n2p);

	// snap
	sp_desktop_circular_snap_list (desktop, seltrans->snappoints, &norm, rotate);
	art_affine_point (p, &point, rotate);

	// status text
	dx2 = p->x    - norm.x;
	dy2 = p->y    - norm.y;
	h2 = hypot (dx2, dy2);
	if (fabs (h2) < 1e-15) return FALSE;
	angle = 180 / M_PI * acos ((dx1*dx2 + dy1*dy2) / (h1 * h2));
	sg = (dx1 * dy2 + dy1 * dx2) / (dx1*dx1 + dy1*dy1);
	if (fabs (sg) > 1e-15) angle *= sg / fabs (sg);
	
	sprintf (status, "Rotate by %0.2f deg", angle);
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);

	return TRUE;
}

gboolean
sp_sel_trans_center_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	gchar status[80];
	GString * xs, * ys;
	SPDesktop * desktop;
	ArtPoint point;
	
	desktop  = seltrans->desktop;
	sp_desktop_free_snap (desktop, p);

	if (state & GDK_CONTROL_MASK) {
	        sp_sel_trans_point_desktop (seltrans, &point);
	        if (fabs (point.x - p->x) > fabs (point.y - p->y)) 
		  p->y = point.y;
		else 
		  p->x = point.x;
	}

	if (state & GDK_SHIFT_MASK) {
		  p->x = (seltrans->box.x0 + seltrans->box.x1) / 2;
		  p->y = (seltrans->box.y0 + seltrans->box.y1) / 2;
	}

	// status text
	xs = SP_PT_TO_METRIC_STRING (p->x, SP_DEFAULT_METRIC);
	ys = SP_PT_TO_METRIC_STRING (p->y, SP_DEFAULT_METRIC);
	sprintf (status, "Center  %s, %s", xs->str, ys->str);
	sp_view_set_status (SP_VIEW (seltrans->desktop), status, FALSE);
	g_string_free (xs, FALSE);
	g_string_free (ys, FALSE);

	return TRUE;
}

/*
 * handlers for handle movement
 *
 */

void
sp_sel_trans_stretch (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	double stretch[6], sx = 1.0, sy = 1.0;
	ArtPoint norm, point;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	switch (handle->cursor) {
	case GDK_TOP_SIDE:
	case GDK_BOTTOM_SIDE:
		if (fabs (point.y - norm.y) < 1e-15) return;
		sy = (p->y - norm.y) / (point.y - norm.y);
		if (fabs (sy) < 1e-15) sy = 1e-15;
		if (state & GDK_CONTROL_MASK) {
			if (fabs (sy) > fabs (sx)) sy = sy / fabs (sy);
			if (fabs (sy) < fabs (sx)) sx = fabs (sy) * sx / fabs (sx);
		}
		break;
	case GDK_LEFT_SIDE:
	case GDK_RIGHT_SIDE:
		if (fabs (point.x - norm.x) < 1e-15) return;
		sx = (p->x - norm.x) / (point.x - norm.x);
		if (fabs (sx) < 1e-15) sx = 1e-15;
		if (state & GDK_CONTROL_MASK) {
			if (fabs (sx) > fabs (sy)) sx = sx / fabs (sx);
			if (fabs (sx) < fabs (sy)) sy = fabs (sx) * sy / fabs (sy);
		}
		break;
	default:
		break;
	}

	if (fabs (sx) < 1e-15) sx = 1e-15; 
	if (fabs (sy) < 1e-15) sy = 1e-15;
	art_affine_scale (stretch, sx, sy);
	sp_sel_trans_transform (seltrans, stretch, &norm);
}

void
sp_sel_trans_scale (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state) 
{
	double scale[6], sx, sy;
	ArtPoint norm, point;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	if (fabs (point.x - norm.x) > 1e-9) {
		sx = (p->x - norm.x) / (point.x - norm.x);
	} else {
		sx = 1.0;
	}

	if (fabs (point.y - norm.y) > 1e-9) {
		sy = (p->y - norm.y) / (point.y - norm.y);
	} else {
		sy = 1.0;
	}

	if (fabs (sx) < 1e-9) sx = 1e-9; 
	if (fabs (sy) < 1e-9) sy = 1e-9;

	art_affine_scale (scale, sx, sy);
	sp_sel_trans_transform (seltrans, scale, &norm);
}

void
sp_sel_trans_skew (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
        double skew[6];
	ArtPoint norm, point;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);
  
	skew[4]=0;
	skew[5]=0;
	
	switch (handle->cursor) {
	case GDK_SB_H_DOUBLE_ARROW:
	  if (fabs (point.y - norm.y) < 1e-15) return;
	  skew[2] = (p->x - point.x) / (point.y - norm.y);
	  skew[3] = (p->y - norm.y) / (point.y - norm.y);
	  skew[1] = 0;
	  skew[0] = 1;
	  break;
	case GDK_SB_V_DOUBLE_ARROW:
	  if (fabs (point.x - norm.x) < 1e-15) return;
	  skew[1] = (p->y - point.y) / (point.x - norm.x);
	  skew[0] = (p->x - norm.x) / (point.x - norm.x);
	  skew[2] = 0;
	  skew[3] = 1;
	  break;
	default:
		break;
	}

	if (fabs (skew[0]) < 1e-15) skew[0] = 1e-15;
	if (fabs (skew[3]) < 1e-15) skew[3] = 1e-15;
	sp_sel_trans_transform (seltrans, skew, &norm);
}

void
sp_sel_trans_rotate (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
        ArtPoint q1, q2, point, norm;
	gdouble rotate[6], r1[6], r2[6], h1, h2;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	h1 = hypot (point.x - norm.x, point.y - norm.y);
	if (fabs (h1) < 1e-15) return;
	q1.x = (point.x - norm.x) / h1;
	q1.y = (point.y - norm.y) / h1;
	h2 = hypot (p->x - norm.x, p->y - norm.y);
	if (fabs (h2) < 1e-15) return;
	q2.x = (p->x - norm.x) / h2;
	q2.y = (p->y - norm.y) / h2;
	r1[0] = q1.x;  r1[1] = -q1.y;  r1[2] =  q1.y;  r1[3] = q1.x;  r1[4] = 0;  r1[5] = 0;
	r2[0] = q2.x;  r2[1] =  q2.y;  r2[2] = -q2.y;  r2[3] = q2.x;  r2[4] = 0;  r2[5] = 0;

	art_affine_multiply (rotate, r1, r2);
	sp_sel_trans_transform (seltrans, rotate, &norm);
}

void
sp_sel_trans_center (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	sp_sel_trans_set_center (seltrans, p->x, p->y);
}

