#define SELECT_CONTEXT_C

#include <math.h>
#include "rubberband.h"
#include "desktop-affine.h"
#include "seltrans-handles.h"
#include "seltrans.h"
#include "sp-select-context.h"

static void sp_selection_moveto (SPDesktop * desktop, double x, double y, guint state);

#if 0
static gchar * sp_select_context_status (void);
#endif

#if 0
static void sp_selection_increase_state (void)
{
	sel.state++;
	if (sel.state > STATE_ROTATE)
		sel.state = STATE_SCALE;
}
#endif

gint
sp_select_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event)
{
	SPSelection * selection;
	GdkCursor * cursor;
	ArtPoint p;
	static int dragging;
	gint consumed = FALSE;

	g_return_val_if_fail (desktop != NULL, TRUE);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), TRUE);
	g_return_val_if_fail (item != NULL, TRUE);
	g_return_val_if_fail (SP_IS_ITEM (item), TRUE);

	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			if (!sp_selection_is_empty (selection)) {
			  if (event->button.state & GDK_SHIFT_MASK) {
			    if (sp_selection_item_selected (selection, item)) {
			      sp_selection_remove_item (selection, item);
			    } else {
			      sp_selection_add_item (selection, item);
			    }
			  } else {
			    if (sp_selection_item_selected (selection, item)) {
			      sp_sel_trans_increase_state (desktop);
			    } else {
			      sp_selection_set_item (selection, item);
			    }
			  }
			} else {
				sp_selection_set_item (selection, item);
			}
			if (!sp_selection_is_empty (selection)) {
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_sel_trans_grab (desktop, NULL, p.x, p.y, FALSE);
				dragging = TRUE;
				consumed = TRUE;
			}
			consumed = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_selection_moveto (desktop, p.x, p.y, event->button.state);
			consumed = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			sp_sel_trans_ungrab (desktop);
			dragging = FALSE;
			consumed = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_ENTER_NOTIFY:
		cursor = gdk_cursor_new (GDK_FLEUR);
		gdk_window_set_cursor (GTK_WIDGET (SP_DT_CANVAS (desktop))->window, cursor);
		gdk_cursor_destroy (cursor);
		break;
	case GDK_LEAVE_NOTIFY:
		gdk_window_set_cursor (GTK_WIDGET (SP_DT_CANVAS (desktop))->window, NULL);
		break;
	default:
		break;
	}

	return consumed;
}

gint
sp_select_root_handler (SPDesktop * desktop, GdkEvent * event)
{
	SPSelection * selection;
	SPItem * item;
	gint ret = FALSE;
	ArtPoint p;
	ArtDRect b;
	GSList * l;

	g_return_val_if_fail (desktop != NULL, TRUE);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), TRUE);

	selection = SP_DT_SELECTION (desktop);

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
			case 1:
				sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
				sp_sel_trans_grab (desktop, NULL, p.x, p.y, FALSE);
				sp_rubberband_start (desktop, p.x, p.y);
				break;
			default:
				break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				l = sp_document_items_in_box (SP_DT_DOCUMENT (desktop), &b);
				if (event->button.state & GDK_SHIFT_MASK) {
					while (l) {
						item = SP_ITEM (l->data);
						if (sp_selection_item_selected (selection, item)) {
							sp_selection_remove_item (selection, item);
						} else {
							sp_selection_add_item (selection, item);
						}
						l = g_slist_remove (l, item);
					}
				} else {
					sp_selection_set_item_list (selection, l);
				}
			} else {
				if (!sp_selection_is_empty (selection)) {
					sp_selection_empty (selection);
					ret = TRUE;
				}
			}
			sp_sel_trans_ungrab (desktop);
			break;
		default:
			break;
		}
		break;
	case GDK_KEY_PRESS:
		g_print ("key %d pressed\n", event->key.keyval);
		break;
	default:
		break;
	}

	return ret;
}

#if 0
static void
sp_selection_save_state (SPDesktop * desktop, double affine[], double x, double y)
{
	double d2n[6];
	ArtDRect bbox;

	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	g_assert (!sp_selection_is_empty (SP_DT_SELECTION (desktop)));
	art_affine_identity (sel.n2current);

	sp_selection_bbox (SP_DT_SELECTION (desktop), &bbox);

	if (affine != NULL) {
		sp_d2n_affine_from_bbox (&bbox, d2n);
		art_affine_multiply (sel.d2n, d2n, affine);
	} else {
		art_affine_identity (sel.d2n);
		sel.d2n[4] -= sel.center.x;
		sel.d2n[5] -= sel.center.y;
	}
	sel.point.x = x;
	sel.point.y = y;
	sel.desktop = desktop;
}

static void
sp_selection_arrange_handles (SPDesktop * desktop)
{
	ArtDRect bbox;
	double d2n[6], n2d[6];
	ArtPoint p;
	int i;

	g_return_if_fail (!sp_selection_is_empty (SP_DT_SELECTION (desktop)));

	if (!handle) return;

	sp_selection_bbox (SP_DT_SELECTION (desktop), &bbox);
	sp_d2n_affine_from_bbox (&bbox, d2n);
	art_affine_invert (n2d, d2n);

	for (i=0; i<8; i++) {
		p.x = (handle + i)->x;
		p.y = (handle + i)->y;
		art_affine_point (&p, &p, n2d);
		sp_ctrl_moveto ((handle + i)->control, p.x, p.y);
	}

	if (sel.state == STATE_ROTATE) {
		sp_ctrl_moveto (center.control, sel.center.x, sel.center.y);
	}

}
#endif

#if 0
static void sp_selection_remember_affine (double affine[])
{
	g_assert (!sp_selection_is_empty (SP_DT_SELECTION (sel.desktop)));

	sel.n2current[0] = affine[0];
	sel.n2current[1] = affine[1];
	sel.n2current[2] = affine[2];
	sel.n2current[3] = affine[3];
	sel.n2current[4] = affine[4];
	sel.n2current[5] = affine[5];
}
#endif

#if 0
static void
sp_selection_normal_transform (double affine[])
{
	const GSList * l;
	SPItem * item;
	double current2n[6], n2d[6], i2d[6], i2n[6], i2current[6], i2new[6], i2dnew[6];
	ArtPoint p;

	art_affine_invert (current2n, sel.n2current);
	art_affine_invert (n2d, sel.d2n);

	l = sp_selection_item_list (SP_DT_SELECTION (sel.desktop));
	g_return_if_fail (l != NULL);

	while (l != NULL) {
		item = SP_ITEM (l->data);
		sp_item_i2d_affine (item, i2d);
		art_affine_multiply (i2current, i2d, sel.d2n);
		art_affine_multiply (i2n, i2current, current2n);
		art_affine_multiply (i2new, i2n, affine);
		art_affine_multiply (i2dnew, i2new, n2d);
		sp_item_set_i2d_affine (item, i2dnew);
		l = l->next;
	}

	p.x = sel.center.x;
	p.y = sel.center.y;
	art_affine_point (&p, &p, sel.d2n);
	art_affine_point (&p, &p, current2n);
	art_affine_point (&p, &p, affine);
	art_affine_point (&p, &p, n2d);
	sel.center.x = p.x;
	sel.center.y = p.y;

	sp_selection_remember_affine (affine);
}
#endif

void
sp_handle_stretch (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state)
{
	ArtPoint p;
	double stretch[6], n2p[6], p2n[6];

	sp_sel_trans_d2n_xy_point (desktop, &p, x, y);
	p.x = 1.0;

	if (state & GDK_SHIFT_MASK) {
		n2p[0] = 1.0;
		n2p[1] = 0.0;
		n2p[2] = 0.0;
		n2p[3] = 2.0;
		n2p[4] = 0.0;
		n2p[5] = -1.0;
		art_affine_invert (p2n, n2p);
		art_affine_point (&p, &p, n2p);
	} else {
		art_affine_identity (n2p);
		art_affine_identity (p2n);
	}

	if (state & GDK_CONTROL_MASK) {
		if (fabs (p.y) > fabs (p.x)) p.y = p.y / fabs (p.y);
		if (fabs (p.y) < fabs (p.x)) p.x = fabs (p.y) * p.x / fabs (p.x);
	}

	art_affine_scale (stretch, p.x, p.y);
	art_affine_multiply (stretch, n2p, stretch);
	art_affine_multiply (stretch, stretch, p2n);

	sp_sel_trans_transform (desktop, stretch);
#if 0
	sp_selection_arrange_handles (sel.desktop);
#endif
}

void
sp_handle_scale (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state) {
	ArtPoint p;
	double scale[6], n2p[6], p2n[6];

	sp_sel_trans_d2n_xy_point (desktop, &p, x, y);

	if (state & GDK_SHIFT_MASK) {
		n2p[0] = 2.0;
		n2p[1] = 0;
		n2p[2] = 0;
		n2p[3] = 2.0;
		n2p[4] = -1.0;
		n2p[5] = -1.0;
		art_affine_invert (p2n, n2p);
		art_affine_point (&p, &p, n2p);
	} else {
		art_affine_identity (n2p);
		art_affine_identity (p2n);
	}

	if (state & GDK_CONTROL_MASK) {
		if (fabs (p.y) > fabs (p.x)) p.y = fabs (p.x) * p.y / fabs (p.y);
		if (fabs (p.x) > fabs (p.y)) p.x = fabs (p.y) * p.x / fabs (p.x);
	}
	art_affine_scale (scale, p.x, p.y);
	art_affine_multiply (scale, n2p, scale);
	art_affine_multiply (scale, scale, p2n);

	sp_sel_trans_transform (desktop, scale);
#if 0
	sp_selection_arrange_handles (sel.desktop);
#endif
}

void
sp_handle_skew (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state)
{
	ArtPoint p;
	double skew[6];

	sp_sel_trans_d2n_xy_point (desktop, &p, x, y);

	skew[0] = 1.0;
	skew[1] = 0.0;
	skew[2] = p.x;
	skew[3] = 1.0;
	skew[4] = 0.0;
	skew[5] = 0.0;

	sp_sel_trans_transform (desktop, skew);
#if 0
	sp_selection_arrange_handles (sel.desktop);
#endif
}

void
sp_handle_rotate (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state) {
	ArtPoint p;
	double s[6], rotate[6];
	double d;

	sp_sel_trans_point_normal (desktop, &p);
	d = sqrt (p.x * p.x + p.y * p.y);
	if (d < 1e-15) return;
	p.x /= d;
	p.y /= d;
	s[0] = p.x;
	s[1] = -p.y;
	s[2] = p.y;
	s[3] = p.x;
	s[4] = 0.0;
	s[5] = 0.0;

	sp_sel_trans_d2n_xy_point (desktop, &p, x, y);
	art_affine_point (&p, &p, s);
	d = sqrt (p.x * p.x + p.y * p.y);
	if (d < 1e-15) return;
	p.x /= d;
	p.y /= d;
	rotate[0] = p.x;
	rotate[1] = p.y;
	rotate[2] = -p.y;
	rotate[3] = p.x;
	rotate[4] = 0.0;
	rotate[5] = 0.0;

	sp_sel_trans_transform (desktop, rotate);
#if 0
	sp_selection_arrange_handles (sel.desktop);
#endif
}

void
sp_handle_center (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state)
{
	sp_sel_trans_set_center (desktop, x, y);
}

static void
sp_selection_moveto (SPDesktop * desktop, double x, double y, guint state)
{
	double dx, dy;
	double move[6];
	ArtPoint p;

	sp_sel_trans_point_desktop (desktop, &p);
	dx = x - p.x;
	dy = y - p.y;

	if (state & GDK_CONTROL_MASK) {
		if (fabs (dx) > fabs (dy)) dy = 0.0;
		else dx = 0.0;
	}

	art_affine_translate (move, dx, dy);
	sp_sel_trans_transform (desktop, move);
}


void sp_select_context_release (SPDesktop * desktop)
{
	sp_sel_trans_set_active (desktop, FALSE);
#if 0
	if (handle != NULL)
		sp_select_context_hide_handles ();
#endif
#if 0
	sp_desktop_set_status ("");
#endif
}

void sp_select_context_set (SPDesktop * desktop)
{
	sp_sel_trans_set_active (desktop, TRUE);
#if 0
	g_assert (handle == NULL);

	sel.state = STATE_SCALE;

	if (sp_selection_is_empty (SP_DT_SELECTION (desktop)))
		return;

	sp_selection_save_state (desktop, NULL, 0.0, 0.0);
	sp_select_context_show_handles (desktop);
#endif
#if 0
	sp_desktop_set_status (sp_select_context_status ());
#endif
}

void sp_select_context_selection_changed (SPDesktop * desktop)
{
#if 0
	const GSList * list;

	list = sp_selection_item_list (SP_DT_SELECTION (desktop));

	if (list == NULL) {
		if (handle)
			sp_select_context_hide_handles ();
		sel.state = STATE_SCALE;
	} else {
		sp_selection_save_state (desktop, NULL, 0.0, 0.0);
		if (handle)
			sp_selection_arrange_handles (desktop);
	}
#endif
#if 0
	sp_desktop_set_status (sp_select_context_status ());
#endif
}

#if 0
gchar * sp_select_context_status (void)
{
#if 0
	GList * list;
	gint len;
	char s[128];

	list = sp_selection_list ();

	if (list == NULL) {
		return g_strdup ("Shift toggles");
	} else {
		len = g_list_length (list);
		if (len == 1) {
			return sp_editable_description (SP_EDITABLE (sp_repr_editable ((SPRepr *) list->data)));
		} else {
			sprintf (s, "%d objects selected", len);
			return g_strdup (s);
		}
	}
#endif
	return NULL;
}
#endif
