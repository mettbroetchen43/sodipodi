#define SP_SELECTION_CHEMISTRY_C

/*
 * sp-selection
 *
 * selection handling functions
 *
 * TODO: ungrouping destroys group attributes - should inherit to children
 *
 */

#include "svg/svg.h"
#include "desktop.h"
#include "selection-chemistry.h"

void
sp_selection_delete (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * sel;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	desktop = SP_WIDGET_DESKTOP (widget);

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	sel = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
#if 0
	sp_selection_empty (selection);
#endif
	while (sel) {
		sp_repr_unparent_and_destroy ((SPRepr *) sel->data);
		sel = g_slist_remove (sel, sel->data);
	}
}

/* fixme */
void sp_selection_duplicate (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * sel;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	desktop = SP_WIDGET_DESKTOP (widget);

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	sel = g_slist_copy ((GSList *) sp_selection_repr_list (selection));

	sp_selection_empty (selection);

	sel = g_slist_sort (sel, (GCompareFunc) sp_repr_compare_position);

	/* fixme: use sp_doc_add_repr */
	while (sel) {
		sp_repr_duplicate_and_parent ((SPRepr *) sel->data);
		sel = g_slist_remove (sel, sel->data);
	}
}

void
sp_selection_group (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPRepr * current;
	SPRepr * parent;
	SPRepr * group;
	SPItem * new;
	const GSList * l;
	GSList * p;

	desktop = SP_WIDGET_DESKTOP (widget);

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	l = sp_selection_repr_list (selection);

	if (l->next == NULL) return;

	p = g_slist_copy ((GSList *) l);

	sp_selection_empty (SP_DT_SELECTION (desktop));

	p = g_slist_sort (p, (GCompareFunc) sp_repr_compare_position);

	group = sp_repr_new_with_name ("g");

	while (p) {
		current = (SPRepr *) p->data;
		parent = sp_repr_parent (current);
		sp_repr_ref (current);
		sp_repr_remove_child (parent, current);
		sp_repr_append_child (group, current);
		sp_repr_unref (current);
		p = g_slist_remove (p, current);
	}

	new = sp_document_add_repr (SP_DT_DOCUMENT (desktop), group);
	sp_repr_unref (group);

	sp_selection_set_item (selection, new);
}

void sp_selection_ungroup (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPItem * item;
	SPRepr * current;
	SPRepr * child;
	GList * list;
	const gchar * pastr, * castr;
	gdouble pa[6], ca[6];
	SPCSSAttr * css;
	gchar affinestr[80];

	desktop = SP_WIDGET_DESKTOP (widget);

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

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

	while ((list = (GList *) sp_repr_children (current))) {
		child = (SPRepr *) list->data;

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

	sp_repr_unparent_and_destroy (current);
}

void sp_selection_raise (GtkWidget * widget)
{
#if 0
	SPRepr * repr;
	GList * l;
	GList * pl;

	if (selected == NULL)
		return;

	sp_event_context_set_select ();

	pl = NULL;

	for (l = selected; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pl = g_list_append (pl, GINT_TO_POINTER (sp_repr_position (repr)));
	}

	for (l = selected; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, GPOINTER_TO_INT (pl->data) + 1);
		pl = g_list_remove (pl, pl->data);
	}
#endif
}

void sp_selection_raise_to_top (GtkWidget * widget)
{
#if 0
	SPRepr * repr;
	GList * l;

	if (selected == NULL)
		return;

	sp_event_context_set_select ();

	for (l = selected; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, -1);
	}
#endif
}

void sp_selection_lower (GtkWidget * widget)
{
#if 0
	SPRepr * repr;
	GList * l;
	GList * pl;
	gint pos;

	if (selected == NULL)
		return;

	sp_event_context_set_select ();

	pl = NULL;

	for (l = selected; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pl = g_list_append (pl, GINT_TO_POINTER (sp_repr_position (repr)));
	}

	for (l = selected; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		pos = GPOINTER_TO_INT (pl->data) - 1;
		if (pos < 0) pos = 0;
		sp_repr_set_position_absolute (repr, pos);
		pl = g_list_remove (pl, pl->data);
	}
#endif
}

void sp_selection_lower_to_bottom (GtkWidget * widget)
{
#if 0
	SPRepr * repr;
	GList * l;

	if (selected == NULL)
		return;

	sp_event_context_set_select ();

	for (l = g_list_last (selected); l != NULL; l = l->prev) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, 0);
	}
#endif
}


#if 0
void
sp_selection_bbox (ArtDRect * bbox)
{
	ArtDRect b;
	GList * list;
	SPRepr * repr;

	list = selected;

	if (list == NULL)
		return;

	repr = (SPRepr *) list->data;
	sp_editable_bbox (sp_repr_editable (repr), bbox);

	for (list = list->next; list != NULL; list = list->next) {
		repr = (SPRepr *) list->data;
		sp_editable_bbox (sp_repr_editable (repr), &b);
		art_drect_union (bbox, bbox, &b);
	}
}

#endif
