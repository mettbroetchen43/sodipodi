#define __SP_PATH_CHEMISTRY_C__

/*
 * Here are handlers for modifying selections, specific to paths
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "xml/repr.h"
#include "svg/svg.h"
#include "sp-path.h"
#include "sp-text.h"
#include "style.h"
#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "path-chemistry.h"
#include "desktop.h"

void
sp_selected_path_combine (void)
{
  SPDesktop * desktop;
  SPSelection * selection;
  GSList * il;
  GSList * l;
  SPRepr * repr;
  SPItem * item;
  SPPath * path;
  SPCurve * c;
  ArtBpath * abp;
  gdouble i2doc[6];
  gchar * d, * str, * style;

  desktop = SP_ACTIVE_DESKTOP;
  if (!SP_IS_DESKTOP(desktop)) return;
  selection = SP_DT_SELECTION (desktop);

	il = (GSList *) sp_selection_item_list (selection);

	if (g_slist_length (il) < 2) return;

	for (l = il; l != NULL; l = l->next) {
		item = (SPItem *) l->data;
		if (!SP_IS_PATH (item)) return;
		if (!((SPPath *) item)->independent) return;
	}

	il = g_slist_copy (il);

	d = "";
	style = g_strdup (sp_repr_attr ((SP_OBJECT (il->data))->repr, "style"));

	for (l = il; l != NULL; l = l->next) {
		path = (SPPath *) l->data;
		c = sp_path_normalized_bpath (path);
		sp_item_i2doc_affine (SP_ITEM (path), i2doc);
		abp = art_bpath_affine_transform (c->bpath, i2doc);
		sp_curve_unref (c);
		str = sp_svg_write_path (abp);
		art_free (abp);
		d = g_strconcat (d, str, NULL);
		g_free (str);
		sp_repr_unparent (SP_OBJECT_REPR (path));
	}

	g_slist_free (il);

	repr = sp_repr_new ("path");
	sp_repr_set_attr (repr, "style", style);
	g_free (style);
	sp_repr_set_attr (repr, "d", d);
	g_free (d);
	item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
	sp_document_done (SP_DT_DOCUMENT (desktop));
	sp_repr_unref (repr);

	sp_selection_set_item (selection, item);
}

void
sp_selected_path_break_apart (void)
{
	SPSelection * selection;
	SPRepr * repr;
	SPItem * item;
	SPPath * path;
	SPCurve * curve;
	ArtBpath * abp;
	double i2doc[6];
	gchar * style, * str;
	GSList * list, * l;
	SPDesktop * desktop;
	
	desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP(desktop)) return;

	selection = SP_DT_SELECTION (desktop);

	item = sp_selection_item (selection);

	if (item == NULL) return;
	if (!SP_IS_PATH (item)) return;

	path = SP_PATH (item);
	if (!sp_path_independent (path)) return;

	curve = sp_path_normalized_bpath (path);
	if (curve == NULL) return;

	sp_item_i2doc_affine (SP_ITEM (path), i2doc);
	style = g_strdup (sp_repr_attr (SP_OBJECT (item)->repr, "style"));

	abp = art_bpath_affine_transform (curve->bpath, i2doc);

	sp_curve_unref (curve);
	sp_repr_unparent (SP_OBJECT_REPR (item));

	curve = sp_curve_new_from_bpath (abp);

	list = sp_curve_split (curve);

	sp_curve_unref (curve);

	for (l = list; l != NULL; l = l->next) {
		curve = (SPCurve *) l->data;

		repr = sp_repr_new ("path");
		sp_repr_set_attr (repr, "style", style);
		str = sp_svg_write_path (curve->bpath);
		sp_repr_set_attr (repr, "d", str);
		g_free (str);
		item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), repr);
		sp_repr_unref (repr);
		sp_selection_add_item (selection, item);
	}
	sp_document_done (SP_DT_DOCUMENT (desktop));

	g_slist_free (list);
	g_free (style);
}

void
sp_selected_path_to_curves (void)
{
	SPDesktop *dt;
	SPItem *item;
	SPCurve *curve;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	item = sp_selection_item (SP_DT_SELECTION (dt));
	if (!item) return;

	if (SP_IS_PATH (item) && !SP_PATH (item)->independent) {
		curve = sp_path_normalized_bpath (SP_PATH (item));
	} else if (SP_IS_TEXT (item)) {
		curve = sp_text_normalized_bpath (SP_TEXT (item));
	} else {
		curve = NULL;
	}

	if (curve) {
		SPObject *parent;
		SPRepr *new;
		guchar *str;

		parent = SP_OBJECT_PARENT (item);

		new = sp_repr_new ("path");
		/* Transformation */
		sp_repr_set_attr (new, "transform", sp_repr_attr (SP_OBJECT_REPR (item), "transform"));
		/* Style */
		str = sp_style_write_difference (SP_OBJECT_STYLE (item), SP_OBJECT_STYLE (SP_OBJECT_PARENT (item)));
		sp_repr_set_attr (new, "style", str);
		g_free (str);
		/* Definition */
		str = sp_svg_write_path (curve->bpath);
		sp_repr_set_attr (new, "d", str);
		g_free (str);
		sp_curve_unref (curve);

		sp_repr_add_child (SP_OBJECT_REPR (parent), new, SP_OBJECT_REPR (item));
		sp_repr_unparent (SP_OBJECT_REPR (item));

		sp_document_done (SP_DT_DOCUMENT (dt));
		sp_selection_set_repr (SP_DT_SELECTION (dt), new);
		sp_repr_unref (new);
	}
}

#if 0
	SPSelection * selection;
	SPRepr * new;
	SPPath * path;
	gchar * str;
	const gchar * transform, * style;
	GSList * l, * sl = NULL, * nl = NULL;
	
	selection = SP_DT_SELECTION (desktop);

	sl = (GSList *) sp_selection_item_list (selection);
	l = g_slist_copy (sl);
	sp_selection_empty (selection);

	for (; l != NULL; l = l->next) {
		item = (SPItem *) l->data;
		
		if (item == NULL) continue;
		if (!SP_IS_PATH (item)) continue;

		path = SP_PATH (item);

		curve = sp_path_normalized_bpath (path);
		str = sp_svg_write_path (curve->bpath);
		sp_curve_unref (curve);
		transform = sp_repr_attr (SP_OBJECT (item)->repr, "transform");
		style = sp_repr_attr (SP_OBJECT (item)->repr, "style");

		new = sp_repr_new ("path");
		sp_repr_set_attr (new, "transform", transform);
		sp_repr_set_attr (new, "style", style);
		sp_repr_set_attr (new, "d", str);

		g_free (str);

		sp_repr_unparent (SP_OBJECT_REPR (item));
		item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), new);
		sp_repr_unref (new);
		
		nl = g_slist_append (nl, (gpointer)item);
	}
	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_item_list (selection, nl);
}
#endif
