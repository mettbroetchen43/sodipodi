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
#include "document.h"
#include "sodipodi.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection-chemistry.h"

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
		copy = sp_repr_copy ((SPRepr *) selected->data);
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
	SPRepr * parent;
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
	GList * list;
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
		copy = sp_repr_copy (repr);
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
		copy = sp_repr_copy (repr);
		sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		sp_repr_unref (copy);
		sp_selection_add_repr (selection, copy);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

