#define SP_SELTRANS_C

#include <math.h>
#include "svg/svg.h"
#include "mdi-desktop.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "select-context.h"
#include "seltrans-handles.h"
#include "seltrans.h"

static void sp_sel_trans_update_handles (SPSelTrans * seltrans);
static void sp_sel_trans_update_volatile_state (SPSelTrans * seltrans);
static void sp_sel_trans_d2n_affine_from_box (gdouble affine[], ArtDRect * box);

static void sp_remove_handles (SPCtrl * ctrl[], gint num);
static void sp_show_handles (SPSelTrans * seltrans, SPCtrl * ctrl[], SPSelTransHandle handle[], gint num);

static gint sp_sel_trans_handle_event (GnomeCanvasItem * item, GdkEvent * event, SPSelTransHandle * handle);

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

	sp_sel_trans_update_handles (seltrans);

	selection = SP_DT_SELECTION (desktop);

	seltrans->sel_changed_id = gtk_signal_connect (GTK_OBJECT (selection), "changed",
		GTK_SIGNAL_FUNC (sp_sel_trans_sel_changed), seltrans);
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

	if (seltrans->state > SP_SEL_TRANS_ROTATE) {
		seltrans->state = SP_SEL_TRANS_SCALE;
	} else {
		seltrans->center.x = (seltrans->box.x0 + seltrans->box.x1) / 2;
		seltrans->center.y = (seltrans->box.y0 + seltrans->box.y1) / 2;
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
sp_sel_trans_grab (SPSelTrans * seltrans, gdouble affine[], gdouble x, gdouble y, gboolean show_handles)
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

	if (affine != NULL) {
		sp_sel_trans_d2n_affine_from_box (seltrans->d2n, &seltrans->box);
		art_affine_multiply (seltrans->d2n, seltrans->d2n, affine);
	} else {
		art_affine_identity (seltrans->d2n);
		seltrans->d2n[4] -= seltrans->center.x;
		seltrans->d2n[5] -= seltrans->center.y;
	}

	seltrans->point.x = x;
	seltrans->point.y = y;

	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_transform (SPSelTrans * seltrans, gdouble affine[])
{
	SPItem * item;
	const GSList * l;
	double current2n[6], n2d[6], i2d[6], i2n[6], i2current[6], i2new[6], i2dnew[6];
	ArtPoint p;
	gint i;

	g_return_if_fail (seltrans->grabbed);
	g_return_if_fail (!seltrans->empty);

	/* We accept empty lists here, as something may well remove items */
	/* and seltrans does not update, if frozen */

	l = sp_selection_item_list (SP_DT_SELECTION (seltrans->desktop));

	art_affine_invert (current2n, seltrans->n2current);
	art_affine_invert (n2d, seltrans->d2n);

	while (l != NULL) {
		item = SP_ITEM (l->data);
		sp_item_i2d_affine (item, i2d);
		art_affine_multiply (i2current, i2d, seltrans->d2n);
		art_affine_multiply (i2n, i2current, current2n);
		art_affine_multiply (i2new, i2n, affine);
		art_affine_multiply (i2dnew, i2new, n2d);
		sp_item_set_i2d_affine (item, i2dnew);
		l = l->next;
	}

	p.x = seltrans->center.x;
	p.y = seltrans->center.y;
	art_affine_point (&p, &p, seltrans->d2n);
	art_affine_point (&p, &p, current2n);
	art_affine_point (&p, &p, affine);
	art_affine_point (&p, &p, n2d);
	seltrans->center.x = p.x;
	seltrans->center.y = p.y;

	for (i = 0; i < 6; i++) seltrans->n2current[i] = affine[i];

	seltrans->changed = TRUE;

	sp_sel_trans_update_handles (seltrans);
}

void
sp_sel_trans_ungrab (SPSelTrans * seltrans)
{
	SPItem * item;
	const GSList * l;
	gchar tstr[80];

	g_return_if_fail (seltrans->grabbed);

	if (!seltrans->empty && seltrans->changed) {
		l = sp_selection_item_list (SP_DT_SELECTION (seltrans->desktop));

		tstr[79] = '\0';

		while (l != NULL) {
			item = SP_ITEM (l->data);
			sp_svg_write_affine (tstr, 79, item->affine);
			sp_repr_set_attr (item->repr, "transform", tstr);
			l = l->next;
		}
	}

	seltrans->grabbed = FALSE;
	seltrans->show_handles = TRUE;

	sp_sel_trans_update_volatile_state (seltrans);

#if 0
	if (seltrans->sel_changed) {
		seltrans->state = SP_SEL_TRANS_SCALE;
	}
#endif

	sp_sel_trans_update_handles (seltrans);
}

ArtPoint *
sp_sel_trans_d2n_xy_point (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y)
{
	g_return_val_if_fail (p != NULL, NULL);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, seltrans->d2n);

	return p;
}

ArtPoint *
sp_sel_trans_n2d_xy_point (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y)
{
	gdouble n2d[6];

	g_return_val_if_fail (p != NULL, NULL);

	art_affine_invert (n2d, seltrans->d2n);
	p->x = x;
	p->y = y;
	art_affine_point (p, p, n2d);

	return p;
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
sp_sel_trans_point_normal (SPSelTrans * seltrans, ArtPoint * p)
{
	g_return_val_if_fail (p != NULL, NULL);

	art_affine_point (p, &seltrans->point, seltrans->d2n);

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
		sp_show_handles (seltrans, seltrans->rhandle, handles_rotate, 8);
		if (seltrans->chandle == NULL) {
			seltrans->chandle = (SPCtrl *) gnome_canvas_item_new (SP_DT_CONTROLS (seltrans->desktop),
				SP_TYPE_CTRL, "anchor", handle_center.anchor, NULL);
			gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "event",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_event), &handle_center);
		}
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (seltrans->chandle));
		sp_ctrl_moveto (seltrans->chandle, seltrans->center.x, seltrans->center.y);
	}
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
	sp_sel_trans_d2n_affine_from_box (seltrans->d2n, &seltrans->box);
	art_affine_identity (seltrans->n2current);
}

static void
sp_sel_trans_d2n_affine_from_box (gdouble affine[], ArtDRect * box)
{
	g_return_if_fail (affine != NULL);
	g_return_if_fail (box != NULL);
	g_return_if_fail (fabs (box->x1 - box->x0) > 1e-18);
	g_return_if_fail (fabs (box->y1 - box->y0) > 1e-18);

	affine[0] = 1 / (box->x1 - box->x0);
	affine[1] = affine[2] = 0;
	affine[3] = 1 / (box->y1 - box->y0);
	affine[4] = - box->x0 * affine[0];
	affine[5] = - box->y0 * affine[3];
}

static void
sp_remove_handles (SPCtrl * ctrl[], gint num)
{
	gint i;

	for (i = 0; i < num; i++) {
		if (ctrl[i] != NULL) {
			gnome_canvas_item_hide (GNOME_CANVAS_ITEM (ctrl[i]));
		}
	}
}

static void
sp_show_handles (SPSelTrans * seltrans, SPCtrl * ctrl[], SPSelTransHandle handle[], gint num)
{
	ArtPoint p;
	gdouble d2n[6], n2d[6];
	gint i;

	sp_sel_trans_d2n_affine_from_box (d2n, &seltrans->box);
	art_affine_invert (n2d, d2n);

	for (i = 0; i < num; i++) {
		if (ctrl[i] == NULL) {
			ctrl[i] = (SPCtrl *) gnome_canvas_item_new (SP_DT_CONTROLS (seltrans->desktop),
				SP_TYPE_CTRL, "anchor", handle[i].anchor, NULL);
			gtk_signal_connect (GTK_OBJECT (ctrl[i]), "event",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_event), &handle[i]);
		}
		gnome_canvas_item_show (GNOME_CANVAS_ITEM (ctrl[i]));
		p.x = handle[i].x;
		p.y = handle[i].y;
		/* fixme: current2n */
		art_affine_point (&p, &p, seltrans->n2current);
		art_affine_point (&p, &p, n2d);
		sp_ctrl_moveto (ctrl[i], p.x, p.y);
	}
}

static gint
sp_sel_trans_handle_event (GnomeCanvasItem * item, GdkEvent * event, SPSelTransHandle * handle)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	ArtPoint p;
	static int dragging = FALSE;
	GdkCursor * cursor;
	SPCtrl * control;
	
	g_return_val_if_fail (SP_IS_CTRL (item), FALSE);
	control = SP_CTRL (item);
	desktop = SP_ACTIVE_DESKTOP;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			cursor = gdk_cursor_new (GDK_HAND2);
			gnome_canvas_item_grab (item,
				GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				cursor, event->button.time);
			gdk_cursor_destroy (cursor);
			dragging = TRUE;
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_sel_trans_grab (seltrans, handle->affine, p.x, p.y, FALSE);
			break;
		default:
			break;
		}
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			handle->action (seltrans, handle, p.x, p.y, event->button.state);
		}
		break;
	case GDK_BUTTON_RELEASE:
		gnome_canvas_item_ungrab (item, event->button.time);
		dragging = FALSE;
		sp_sel_trans_ungrab (seltrans);
		break;
	case GDK_ENTER_NOTIFY:
		cursor = gdk_cursor_new (handle->cursor);
		gdk_window_set_cursor (item->canvas->layout.container.widget.window, cursor);
		gdk_cursor_destroy (cursor);
		break;
	case GDK_LEAVE_NOTIFY:
		gdk_window_set_cursor (item->canvas->layout.container.widget.window, NULL);
		break;
	default:
		break;
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
		sp_sel_trans_update_handles (seltrans);
	}
}

