#define SP_SELTRANS_C

#include <math.h>
#include "svg/svg.h"
#include "sodipodi-private.h"
#include "sodipodi.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "select-context.h"
#include "seltrans-handles.h"
#include "seltrans.h"

static void sp_sel_trans_update_handles (SPSelTrans * seltrans);
static void sp_sel_trans_update_volatile_state (SPSelTrans * seltrans);
static void sp_sel_trans_d2n_affine_from_box (gdouble affine[], ArtDRect * box);

static void sp_remove_handles (SPKnot * knot[], gint num);
static void sp_show_handles (SPSelTrans * seltrans, SPKnot * knot[], SPSelTransHandle handle[], gint num);

static void sp_sel_trans_handle_grab (SPKnot * knot, guint state, gpointer data);
static void sp_sel_trans_handle_ungrab (SPKnot * knot, guint state, gpointer data);
static void sp_sel_trans_handle_new_event (SPKnot * knot, ArtPoint * position, guint32 state, gpointer data);
static gboolean sp_sel_trans_handle_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data);

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
			sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
			l = l->next;
		}
	}

	sp_document_done (SP_DT_DOCUMENT (seltrans->desktop));

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
			seltrans->chandle = sp_knot_new (seltrans->desktop);
			gtk_object_set (GTK_OBJECT (seltrans->chandle),
				"anchor", handle_center.anchor, NULL);
			gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "moved",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), &handle_center);
			gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "grabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_grab), &handle_center);
			gtk_signal_connect (GTK_OBJECT (seltrans->chandle), "ungrabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_ungrab), &handle_center);
		}
		sp_knot_show (seltrans->chandle);
		sp_knot_set_position (seltrans->chandle, &seltrans->center, 0);
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
sp_show_handles (SPSelTrans * seltrans, SPKnot * knot[], SPSelTransHandle handle[], gint num)
{
	ArtPoint p;
	gdouble d2n[6], n2d[6];
	gint i;

	sp_sel_trans_d2n_affine_from_box (d2n, &seltrans->box);
	art_affine_invert (n2d, d2n);

	for (i = 0; i < num; i++) {
		if (knot[i] == NULL) {
			knot[i] = sp_knot_new (seltrans->desktop);
			gtk_object_set (GTK_OBJECT (knot[i]),
				"anchor", handle[i].anchor, NULL);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "request",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_request), &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "moved",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_new_event), &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "grabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_grab), &handle[i]);
			gtk_signal_connect (GTK_OBJECT (knot[i]), "ungrabbed",
				GTK_SIGNAL_FUNC (sp_sel_trans_handle_ungrab), &handle[i]);
		}
		sp_knot_show (knot[i]);
		p.x = handle[i].x;
		p.y = handle[i].y;
		/* fixme: current2n */
		art_affine_point (&p, &p, seltrans->n2current);
		art_affine_point (&p, &p, n2d);

		sp_knot_set_position (knot[i], &p, 0);
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

	sp_sel_trans_grab (seltrans, handle->affine, p.x, p.y, FALSE);
}

static void
sp_sel_trans_handle_ungrab (SPKnot * knot, guint state, gpointer data)
{
	SPDesktop * desktop;
	SPSelTrans * seltrans;
	
	desktop = knot->desktop;
	seltrans = &SP_SELECT_CONTEXT (desktop->event_context)->seltrans;

	sp_sel_trans_ungrab (seltrans);
	sp_selection_changed (SP_DT_SELECTION (seltrans->desktop));

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

	handle->action (seltrans, handle, position->x, position->y, state);
}

/* fixme: Highly experimental test :) */

static gboolean
sp_sel_trans_handle_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data)
{
#if 0
	ArtPoint s;
#endif

	sp_desktop_free_snap (knot->desktop, p);
#if 0
	s.x = rint (p->x);
	s.y = rint (p->y);

	sp_knot_set_position (knot, &s, state);

	return TRUE;
#else
	return FALSE;
#endif
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

