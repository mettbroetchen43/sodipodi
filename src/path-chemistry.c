#define SP_PATH_CHEMISTRY_C

#include "xml/repr.h"
#include "svg/svg.h"
#include "sp-path.h"
#include "mdi-desktop.h"
#include "document.h"
#include "selection.h"
#include "desktop-handles.h"
#include "path-chemistry.h"

void
sp_selected_path_combine (void)
{
	SPSelection * selection;
	GSList * il;
	GSList * l;
	SPRepr * repr;
	SPItem * item;
	SPPath * path;
	ArtBpath * bp, * abp;
	gdouble i2doc[6];
	gchar * d, * str, * style;

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	il = (GSList *) sp_selection_item_list (selection);

	if (g_slist_length (il) < 2) return;

	for (l = il; l != NULL; l = l->next) {
		item = (SPItem *) l->data;
		if (!SP_IS_PATH (item)) return;
		if (!((SPPath *) item)->independent) return;
	}

	il = g_slist_copy (il);

	d = "";
	style = g_strdup (sp_repr_attr ((SP_ITEM (il->data))->repr, "style"));

	for (l = il; l != NULL; l = l->next) {
		path = (SPPath *) l->data;
		bp = sp_path_normalized_bpath (path);
		sp_item_i2doc_affine (SP_ITEM (path), i2doc);
		abp = art_bpath_affine_transform (bp, i2doc);
		art_free (bp);
		str = sp_svg_write_path (abp);
		art_free (abp);
		d = g_strconcat (d, str, NULL);
		g_free (str);
		sp_repr_unparent_and_destroy (SP_ITEM (path)->repr);
	}

	g_slist_free (il);

	repr = sp_repr_new_with_name ("path");
	sp_repr_set_attr (repr, "style", style);
	g_free (style);
	sp_repr_set_attr (repr, "d", d);
	g_free (d);
	item = sp_document_add_repr (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP), repr);
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
	ArtBpath * bpath, * abp, * nbp;
	double i2doc[6];
	gchar * style, * str;
	gint len, pos, newpos;

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	item = sp_selection_item (selection);

	if (item == NULL) return;
	if (!SP_IS_PATH (item)) return;

	path = SP_PATH (item);
	if (!sp_path_independent (path)) return;

	bpath = sp_path_normalized_bpath (path);
	if (bpath == NULL) return;

	sp_item_i2doc_affine (SP_ITEM (path), i2doc);
	style = g_strdup (sp_repr_attr (item->repr, "style"));

	sp_repr_unparent_and_destroy (item->repr);

	abp = art_bpath_affine_transform (bpath, i2doc);
	art_free (bpath);
	for (len = 0; abp[len].code != ART_END; len++);
	nbp = art_new (ArtBpath, len);

	pos = 0;
	while (pos < len) {
		newpos = 0;
		do {
			nbp[newpos] = abp[pos];
			pos++;
			newpos++;
		} while ((abp[pos].code != ART_MOVETO) &&
			(abp[pos].code != ART_MOVETO_OPEN) &&
			(abp[pos].code != ART_END));
		nbp[newpos].code = ART_END;

		repr = sp_repr_new_with_name ("path");
		sp_repr_set_attr (repr, "style", style);
		str = sp_svg_write_path (nbp);
		sp_repr_set_attr (repr, "d", str);
		g_free (str);
		item = sp_document_add_repr (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP), repr);
		sp_repr_unref (repr);
		sp_selection_add_item (selection, item);
	}

	art_free (nbp);
	art_free (abp);
	g_free (style);
}

void
sp_selected_path_to_curves (void)
{
	SPSelection * selection;
	SPRepr * new;
	SPItem * item;
	SPPath * path;
	ArtBpath * bpath;
	gchar * str;
	const gchar * transform, * style;

	selection = SP_DT_SELECTION (SP_ACTIVE_DESKTOP);

	item = sp_selection_item (selection);
	if (item == NULL) return;
	if (!SP_IS_PATH (item)) return;

	path = SP_PATH (item);

	bpath = sp_path_normalized_bpath (path);
	str = sp_svg_write_path (bpath);
	art_free (bpath);
	transform = sp_repr_attr (item->repr, "transform");
	style = sp_repr_attr (item->repr, "style");

	new = sp_repr_new_with_name ("path");
	sp_repr_set_attr (new, "transform", transform);
	sp_repr_set_attr (new, "style", style);
	sp_repr_set_attr (new, "d", str);

	g_free (str);

	sp_repr_unparent_and_destroy (item->repr);
	item = sp_document_add_repr (SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP), new);
	sp_repr_unref (new);

	sp_selection_set_item (selection, item);
}
