#define SP_SELECTION_CHEMISTRY_C

/*
 * sp-selection
 *
 * selection handling functions
 *
 * TODO: ungrouping destroys group attributes - should inherit to children
 *
 */

#include <string.h>
#include "svg/svg.h"
#include "xml/repr-private.h"
#include "document.h"
#include "sodipodi.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection-chemistry.h"
#include "sp-item-transform.h" 
#include "sp-item-group.h"

/* fixme: find a better place */
GSList * clipboard = NULL;

void
sp_selection_delete (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * selected;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	if (sp_selection_is_empty (selection)) return;

	selected = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sp_selection_empty (selection);

	while (selected) {
		sp_document_del_repr (SP_DT_DOCUMENT (desktop), (SPRepr *) selected->data);
		selected = g_slist_remove (selected, selected->data);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

/* fixme */
void sp_selection_duplicate (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * selected, * newsel;
	SPRepr * copy;
	SPItem * item;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	if (sp_selection_is_empty (selection)) return;

	selected = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sp_selection_empty (selection);

	selected = g_slist_sort (selected, (GCompareFunc) sp_repr_compare_position);

	newsel = NULL;

	while (selected) {
		copy = sp_repr_duplicate ((SPRepr *) selected->data);
		item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		sp_repr_unref (copy);
		g_assert (item != NULL);
		newsel = g_slist_prepend (newsel, copy);
		selected = g_slist_remove (selected, selected->data);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_repr_list (SP_DT_SELECTION (desktop), newsel);

	g_slist_free (newsel);
}

void
sp_selection_group (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPRepr * current;
	SPRepr * group;
	SPItem * new;
	const GSList * l;
	GSList * p;

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	l = sp_selection_repr_list (selection);

	if (l->next == NULL) return;

	p = g_slist_copy ((GSList *) l);

	sp_selection_empty (SP_DT_SELECTION (desktop));

	p = g_slist_sort (p, (GCompareFunc) sp_repr_compare_position);

	group = sp_repr_new ("g");

	while (p) {
		SPRepr *new;
		current = (SPRepr *) p->data;
		new = sp_repr_duplicate (current);
		sp_document_del_repr (SP_DT_DOCUMENT (desktop), current);
		sp_repr_append_child (group, new);
		sp_repr_unref (new);
		p = g_slist_remove (p, current);
	}

	new = sp_document_add_repr (SP_DT_DOCUMENT (desktop), group);
	sp_repr_unref (group);
	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_repr (selection, group);
}

void sp_selection_ungroup (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPItem * item;
	SPRepr * current;
	SPRepr * child;
	const gchar * pastr, * castr;
	gdouble pa[6], ca[6];
	SPCSSAttr * css;
	gchar affinestr[80];

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	current = sp_selection_repr (selection);

	if (current == NULL) return;

	if (strcmp (sp_repr_name (current), "g") != 0)
		return;

	sp_selection_empty (selection);

	art_affine_identity (pa);
	pastr = sp_repr_attr (current, "transform");
	sp_svg_read_affine (pa, pastr);

	while (current->children) {
		child = current->children;

		art_affine_identity (ca);
		castr = sp_repr_attr (child, "transform");
		sp_svg_read_affine (ca, castr);
		art_affine_multiply (ca, ca, pa);

		css = sp_repr_css_attr_inherited (child, "style");
		sp_repr_ref (child);
		sp_repr_remove_child (current, child);
		
		sp_svg_write_affine (affinestr, 79, ca);
		affinestr[79] = '\0';
		sp_repr_set_attr (child, "transform", affinestr);

		sp_repr_css_set (child, css, "style");
		sp_repr_css_attr_unref (css);

		item = sp_document_add_repr (SP_DT_DOCUMENT (desktop), child);
		sp_repr_unref (child);
		sp_selection_add_item (selection, item);
	}

	sp_document_del_repr (SP_DT_DOCUMENT (desktop), current);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void sp_selection_raise (GtkWidget * widget)
{
	SPDocument * document;
	SPSelection * selection;
	SPDesktop * desktop;
	SPRepr * repr;
	GSList * rl;
	GSList * l;
	GSList * pl;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	document = SP_DT_DOCUMENT (desktop);
	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	pl = NULL;

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pl = g_slist_append (pl, GINT_TO_POINTER (sp_repr_position (repr)));
	}

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, GPOINTER_TO_INT (pl->data) + 1);
		pl = g_slist_remove (pl, pl->data);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void sp_selection_raise_to_top (GtkWidget * widget)
{
	SPDocument * document;
	SPSelection * selection;
	SPDesktop * desktop;
	SPRepr * repr;
	GSList * rl;
	GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	document = SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP);
	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sp_selection_is_empty (selection)) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, -1);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void sp_selection_lower (GtkWidget * widget)
{
	SPDocument * document;
	SPSelection * selection;
	SPDesktop * desktop;
	SPRepr * repr;
	GSList * rl;
	GSList * l;
	GSList * pl;
	gint pos;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	document = SP_DT_DOCUMENT (desktop);
	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	pl = NULL;

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pl = g_slist_append (pl, GINT_TO_POINTER (sp_repr_position (repr)));
	}

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pos = GPOINTER_TO_INT (pl->data) - 1;
		if (pos < 0) pos = 0;
		sp_repr_set_position_absolute (repr, pos);
		pl = g_slist_remove (pl, pl->data);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void sp_selection_lower_to_bottom (GtkWidget * widget)
{
	SPDocument * document;
	SPSelection * selection;
	SPDesktop * desktop;
	SPRepr * repr;
	GSList * rl;
	GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	document = SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP);
	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	if (sp_selection_is_empty (selection)) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	rl = g_slist_reverse (rl);

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, 0);
	}

	g_slist_free (rl);

	sp_document_done (document);
}

void
sp_undo (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (SP_IS_DESKTOP(desktop)) {
		sp_document_undo (SP_DT_DOCUMENT (desktop));
	}
}

void
sp_redo (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (SP_IS_DESKTOP(desktop)) {
		sp_document_redo (SP_DT_DOCUMENT (desktop));
	}
}

void
sp_selection_cut (GtkWidget * widget)
{
	sp_selection_copy (widget);
	sp_selection_delete (widget);
}

void
sp_selection_copy (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPRepr * repr, * copy;
	SPCSSAttr * css;
	const GSList * sl, * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);
	if (sp_selection_is_empty (selection)) return;

	sl = sp_selection_repr_list (selection);

	while (clipboard) {
		sp_repr_unref ((SPRepr *) clipboard->data);
		clipboard = g_slist_remove_link (clipboard, clipboard);
	}

	for (l = sl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		css = sp_repr_css_attr_inherited (repr, "style");
		copy = sp_repr_duplicate (repr);
		sp_repr_css_set (copy, css, "style");
		sp_repr_css_attr_unref (css);

		clipboard = g_slist_append (clipboard, copy);
	}
}

void
sp_selection_paste (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * l;
	SPRepr * repr, * copy;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	sp_selection_empty (selection);

	for (l = clipboard; l != NULL; l = l->next) {
		repr = (SPRepr *) clipboard->data;
		copy = sp_repr_duplicate (repr);
		sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		sp_repr_unref (copy);
		sp_selection_add_repr (selection, copy);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void sp_selection_apply_affine (SPSelection * selection, double affine[6]) {
  SPItem * item;
  GSList * l;
  double curaff[6], newaff[6];
  char tstr[80];

  g_assert (SP_IS_SELECTION (selection));

    
  for (l = selection->items; l != NULL; l = l-> next) {
    item = SP_ITEM (l->data);

    sp_item_i2d_affine (item, curaff);
    art_affine_multiply (newaff,curaff,affine);
    sp_item_set_i2d_affine (item, newaff);    

    // update repr -  needed for undo 
    tstr[79] = '\0';
    sp_svg_write_affine (tstr, 79, item->affine);
    sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);

  }
}


void
sp_selection_remove_transform (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	const GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	selection = SP_DT_SELECTION (desktop);
	if (!SP_IS_SELECTION (selection)) return;

	l = sp_selection_repr_list (selection);

	while (l != NULL) {
		sp_repr_set_attr (l->data,"transform", NULL);
		l = l->next;
	}

	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}


void
sp_selection_scale_absolute (SPSelection * selection, double x0, double x1, double y0, double y1) {  
  ArtDRect  bbox;
  double p2o[6], o2n[6], scale[6], final[6], s[6];
  double dx, dy, nx, ny;
  
  g_assert (SP_IS_SELECTION (selection));

  sp_selection_bbox (selection, &bbox);

  art_affine_translate (p2o, -bbox.x0, -bbox.y0);

  dx = (x1-x0) / (bbox.x1 - bbox.x0);
  dy = (y1-y0) / (bbox.y1 - bbox.y0);
  art_affine_scale (scale, dx, dy);

  nx = x0;
  ny = y0;
  art_affine_translate (o2n, nx, ny);

  art_affine_multiply (s , p2o, scale);
  art_affine_multiply (final , s, o2n);

  sp_selection_apply_affine (selection, final);
}


void
sp_selection_scale_relative (SPSelection * selection, ArtPoint * align, double dx, double dy) {  
  double scale[6], n2d[6], d2n[6], final[6], s[6];

  art_affine_translate (n2d, -align->x, -align->y);
  art_affine_translate (d2n, align->x, align->y);
  art_affine_scale (scale, dx, dy);

  art_affine_multiply (s, n2d, scale);
  art_affine_multiply (final, s, d2n);

  sp_selection_apply_affine (selection, final);

}


void
sp_selection_rotate_relative (SPSelection * selection, ArtPoint * center, gdouble angle) {
  double rotate[6], n2d[6], d2n[6], final[6], s[6];
  
  art_affine_translate (n2d, -center->x, -center->y);
  art_affine_invert (d2n,n2d);
  art_affine_rotate (rotate, angle);

  art_affine_multiply (s, n2d, rotate);
  art_affine_multiply (final, s, d2n);

  sp_selection_apply_affine (selection, final);
}


void
sp_selection_skew_relative (SPSelection * selection, ArtPoint * align, double dx, double dy) {  
  double skew[6], n2d[6], d2n[6], final[6], s[6];
  
  art_affine_translate (n2d, -align->x, -align->y);
  art_affine_invert (d2n,n2d);

  skew[0] = 1;
  skew[1] = dy;
  skew[2] = dx;
  skew[3] = 1;
  skew[4] = 0;
  skew[5] = 0;

  art_affine_multiply (s, n2d, skew);
  art_affine_multiply (final, s, d2n);

  sp_selection_apply_affine (selection, final);
}


void
sp_selection_move_relative (SPSelection * selection, double dx, double dy) {  
  double move[6];
  
  art_affine_translate (move, dx, dy);

  sp_selection_apply_affine (selection, move);
}

void
sp_selection_rotate_90 (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPItem * item;
	GSList * l, * l2;

	desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP(desktop)) return;
	selection = SP_DT_SELECTION(desktop);
	if sp_selection_is_empty(selection) return;
	l = selection->items;  
	for (l2 = l; l2 != NULL; l2 = l2-> next) {
		item = SP_ITEM (l2->data);
		sp_item_rotate_rel (item,-90);
	}

	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_move_screen (gdouble sx, gdouble sy)
{
  SPDesktop * desktop;
  SPSelection * selection;
  gdouble dx,dy,zf;

  desktop = SP_ACTIVE_DESKTOP;
  g_return_if_fail(SP_IS_DESKTOP (desktop));
  selection = SP_DT_SELECTION (desktop);
  if (!SP_IS_SELECTION (selection)) return;
  if sp_selection_is_empty(selection) return;

  zf = sp_desktop_zoom_factor(desktop);
  dx = sx / zf;
  dy = sy / zf;
  sp_selection_move_relative (selection,dx,dy);

  //
  sp_selection_changed (selection);
  sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_item_next (void)
{
  SPDocument * document;
  SPDesktop * desktop;
  SPSelection * selection;
  GSList * children = NULL, * item = NULL;
  SPGroup * group;
  ArtDRect dbox,sbox;
  ArtPoint s,d;
  gint dx=0, dy=0;

  document = SP_ACTIVE_DOCUMENT;
  desktop = SP_ACTIVE_DESKTOP;
  g_return_if_fail(document != NULL);
  g_return_if_fail(desktop != NULL);
  if (!SP_IS_DESKTOP (desktop)) return;
  selection = SP_DT_SELECTION(desktop);
  g_return_if_fail(selection!=NULL);
  
  // get list of relevant items
  if (SP_CYCLING == SP_CYCLE_VISIBLE) {
    sp_desktop_get_visible_area (desktop, &dbox);
    children = sp_document_items_in_box (document, &dbox);
    g_print("get children \n");
  } else {
    group = SP_GROUP(sp_document_root(document));
    children = group->children;
  }
  if (children==NULL) return;

  // no selection -> take first
  if sp_selection_is_empty(selection) {
    sp_selection_set_item (selection,SP_ITEM(children->data));
    return;
  }

  item = g_slist_find(children,selection->items->data);

  // selection not in root or last element in list -> take first
  if ((item==NULL) || (item->next==NULL)) item = children;
  else item = item->next;

  sp_selection_set_item (selection,SP_ITEM(item->data));

  // adjust visible area to see whole new selection
  if (SP_CYCLING == SP_CYCLE_FOCUS) {
    sp_desktop_get_visible_area (desktop, &dbox);
    sp_item_bbox (SP_ITEM(item->data),&sbox);
    if (dbox.x0>sbox.x0 || dbox.y0>sbox.y0 || dbox.x1<sbox.x1 || dbox.y1<sbox.y1 ) {
      s.x = (sbox.x0+sbox.x1)/2;
      s.y = (sbox.y0+sbox.y1)/2;
      d.x = (dbox.x0+dbox.x1)/2;
      d.y = (dbox.y0+dbox.y1)/2;
      art_affine_point (&s, &s, desktop->d2w);
      art_affine_point (&d, &d, desktop->d2w);
      dx = (gint)(d.x-s.x);
      dy = (gint)(d.y-s.y);
      sp_desktop_scroll_world (desktop, dx, dy);
    }
  }

}
