#define __SP_SELTRANS_C__

#include <math.h>
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

void
sp_sel_trans_init (SPSelTrans * seltrans, SPDesktop * desktop)
{
	SPSelection * selection;
	gint i;

	g_return_if_fail (seltrans != NULL);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	seltrans->desktop = desktop;
	seltrans->grabbed = FALSE;
	seltrans->show_handles = TRUE;
	seltrans->state = SP_SEL_TRANS_SCALE;
	for (i = 0; i < 8; i++) seltrans->shandle[i] = NULL;
	for (i = 0; i < 8; i++) seltrans->rhandle[i] = NULL;
	seltrans->chandle = NULL;

	sp_sel_trans_update_volatile_state (seltrans);
	
	seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
	seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;

	sp_sel_trans_update_handles (seltrans);

	selection = SP_DT_SELECTION (desktop);

	seltrans->sel_changed_id = gtk_signal_connect (GTK_OBJECT (selection), "changed",
		GTK_SIGNAL_FUNC (sp_sel_trans_sel_changed), seltrans);

	seltrans->norm = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
		SP_TYPE_CTRL,
		"anchor", GTK_ANCHOR_CENTER,
		"shape", SP_CTRL_SHAPE_DIAMOND,
		"size", 6.0,
		"filled", FALSE,
		"fill_color", 0x000000ff,
		"stroked", TRUE,
		"stroke_color", 0x000000ff,
		NULL);
	seltrans->grip = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
		SP_TYPE_CTRL,
		"anchor", GTK_ANCHOR_CENTER,
		"shape", SP_CTRL_SHAPE_SQUARE,
		"size", 6.0,
		"filled", FALSE,
		"fill_color", 0x000000ff,
		"stroked", TRUE,
		"stroke_color", 0x000000ff,
		NULL);
	gnome_canvas_item_hide (seltrans->grip);
	gnome_canvas_item_hide (seltrans->norm);

	seltrans->l1 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
					      SP_TYPE_CTRLLINE, 
					      NULL);
	gnome_canvas_item_hide (seltrans->l1);
	seltrans->l2 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
					      SP_TYPE_CTRLLINE, 
					      NULL);
	gnome_canvas_item_hide (seltrans->l2);
	seltrans->l3 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
					      SP_TYPE_CTRLLINE, 
					      NULL);
	gnome_canvas_item_hide (seltrans->l3);
	seltrans->l4 = gnome_canvas_item_new (SP_DT_CONTROLS (desktop),
					      SP_TYPE_CTRLLINE, 
					      NULL);
	gnome_canvas_item_hide (seltrans->l4);
}

void
sp_sel_trans_shutdown (SPSelTrans * seltrans)
{
	seltrans->show_handles = FALSE;

	sp_sel_trans_update_handles (seltrans);

	if (seltrans->sel_changed_id > 0)
		gtk_signal_disconnect (GTK_OBJECT (SP_DT_SELECTION (seltrans->desktop)), seltrans->sel_changed_id);


}

void
sp_sel_trans_reset_state (SPSelTrans * seltrans)
{
	seltrans->state = SP_SEL_TRANS_SCALE;
}

void
sp_sel_trans_increase_state (SPSelTrans * seltrans)
{
	seltrans->state++;

	if (seltrans->state > SP_SEL_TRANS_ROTATE)// {
		seltrans->state = SP_SEL_TRANS_SCALE; /*
	} else {
		seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
		seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;
		}*/
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
sp_sel_trans_grab (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y, gboolean show_handles)
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

	seltrans->point = *p;

	seltrans->snappoints = sp_selection_snappoints (selection);

	seltrans->opposit.x = seltrans->box.x0 + (1 - x) * fabs (seltrans->box.x1 - seltrans->box.x0);
	seltrans->opposit.y = seltrans->box.y0 + (1 - y) * fabs (seltrans->box.y1 - seltrans->box.y0);

	if ((x != -1) && (y != -1)) {
	  gnome_canvas_item_show (seltrans->norm);
	  gnome_canvas_item_show (seltrans->grip);
	}

	if (SelTransViewMode == SP_SELTRANS_OUTLINE) {
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
	gdouble current_[6], i2d[6], i2current[6], i2dnew[6], in2current[6], in2dnew[6], n2p[6], p2n[6];
	ArtPoint p1, p2, p3, p4;
	gint i;

	g_return_if_fail (seltrans->grabbed);
	g_return_if_fail (!seltrans->empty);

	art_affine_invert (current_, seltrans->current);
	art_affine_translate (n2p, norm->x, norm->y);
	art_affine_invert (p2n, n2p);
	art_affine_multiply (affine, p2n, affine);
	art_affine_multiply (affine, affine, n2p);

	if (SelTransViewMode == SP_SELTRANS_CONTENT) {
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
			if (SelTransViewMode == SP_SELTRANS_OUTLINE) {
			  sp_item_i2d_affine (item, i2d);
			  art_affine_multiply (i2dnew, i2d, seltrans->current);
			  sp_item_set_i2d_affine (item, i2dnew);
			}
			sp_svg_write_affine (tstr, 79, item->affine);
			sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
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
	
	// free snappoints
	g_slist_free (seltrans->snappoints);
	seltrans->snappoints = NULL;
	

#if 0
	if (seltrans->sel_changed) {
		seltrans->state = SP_SEL_TRANS_SCALE;
	}
#endif

	gnome_canvas_item_hide (seltrans->norm);
	gnome_canvas_item_hide (seltrans->grip);

        if (SelTransViewMode == SP_SELTRANS_OUTLINE) {
	  gnome_canvas_item_hide (seltrans->l1);
	  gnome_canvas_item_hide (seltrans->l2);
	  gnome_canvas_item_hide (seltrans->l3);
	  gnome_canvas_item_hide (seltrans->l4);
	}

	sp_sel_trans_update_volatile_state (seltrans);
	sp_sel_trans_update_handles (seltrans);
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
	
	if (seltrans->state == SP_SEL_TRANS_SCALE) {
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
			  "shape", handle_center.control,
			  "fill", 0x00ff00a0,
			  "fill_mouseover", 0xff0000a0,
			  "stroke", 0x000000ff,
			  "stroke_mouseover", 0x000000FF,
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
					"shape", handle[i].control,
					"mode", SP_KNOT_MODE_COLOR,
					"fill", 0x0000ffa0,
					"fill_mouseover", 0xff0000a0,
					"stroke", 0x000000ff,
					"stroke_mouseover", 0x000000FF,
					NULL);

			gtk_signal_connect (GTK_OBJECT (knot[i]), "request",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_request), (gpointer) &handle[i]);
 			gtk_signal_connect (GTK_OBJECT (knot[i]), "moved",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), (gpointer) &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "grabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_grab), (gpointer) &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "ungrabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_ungrab), (gpointer) &handle[i]);
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
						
	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;
	handle = (SPSelTransHandle *) data;

	sp_knot_position (knot, &p);

	sp_sel_trans_grab (seltrans, &p, handle->x, handle->y, FALSE);
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

	sp_desktop_coordinate_status (desktop, position->x, position->y, 4);

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

/*
 * handlers for handle move-request
 */

gboolean
sp_sel_trans_scale_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state)
{
	ArtPoint norm, point;
	gdouble sx, sy, ratio;
	gchar status[80];
	SPDesktop * desktop;

	desktop = seltrans->desktop;

	sp_sel_trans_point_desktop (seltrans, &point);
	sp_sel_trans_origin_desktop (seltrans, &norm);

	if ((fabs(point.x - norm.x)<1e-15) || (fabs(point.x - norm.x)<1e-15)) return FALSE;

	sx = (p->x - norm.x) / (point.x - norm.x);
	sy = (p->y - norm.y) / (point.y - norm.y);

	if (state & GDK_CONTROL_MASK) {
        	if (fabs (sx)<1e-15) sx = 1e-15;
	        if (fabs (sy)<1e-15) sy = 1e-15;
	        if (fabs (sy) > fabs (sx)) sy = fabs (sx) * sy / fabs (sy);
		if (fabs (sx) > fabs (sy)) sx = fabs (sy) * sx / fabs (sx);
		ratio = sp_desktop_vector_snap_list (desktop, seltrans->snappoints, &norm, sx, sy);
		sx = fabs (ratio) * sx / fabs (sx);
		sy = fabs (ratio) * sy / fabs (sy);
	} else {
	  sx = sp_desktop_horizontal_snap_list_scale (desktop, seltrans->snappoints, &norm, sx);
	  sy = sp_desktop_vertical_snap_list_scale (desktop, seltrans->snappoints, &norm, sy);
	}

	if (fabs (sx)<1e-15) sx = 1e-15;
	if (fabs (sy)<1e-15) sy = 1e-15;
	p->x = (point.x - norm.x)* sx + norm.x;
	p->y = (point.y - norm.y)* sy + norm.y;

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * sx, '%', 100 * sy, '%');
	sp_desktop_set_status (seltrans->desktop, status);

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
	}

	p->x = (point.x - norm.x)* sx + norm.x;
	p->y = (point.y - norm.y)* sy + norm.y;

	// status text
	sprintf (status, "Scale  %0.2f%c, %0.2f%c", 100 * sx, '%', 100 * sy, '%');
	sp_desktop_set_status (seltrans->desktop, status);

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
	}
	// status text
	sprintf (status, "Skew  %0.2f%c %0.2f%c", 100 * fabs(skew[2]), '%', 100 * fabs(skew[1]), '%');
	sp_desktop_set_status (seltrans->desktop, status);

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
	sp_desktop_set_status (seltrans->desktop, status);

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
	sp_desktop_set_status (seltrans->desktop, status);
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

	if ((fabs(point.x - norm.x)<1e-15) || (fabs(point.x - norm.x)<1e-15)) return;

	sx = (p->x - norm.x) / (point.x - norm.x);
	sy = (p->y - norm.y) / (point.y - norm.y);

	if (fabs (sx) < 1e-15) sx = 1e-15; 
	if (fabs (sy) < 1e-15) sy = 1e-15;
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

