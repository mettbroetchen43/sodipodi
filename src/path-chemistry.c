#define SP_PATH_CHEMISTRY_C

#include "xml/repr.h"
#include "svg/svg.h"
#if 0
#include "canvas-helper/sp-canvas-util.h"
#include "sp-repr-item.h"
#endif
#if 0
#include "sp-path.h"
#include "drawing.h"
#include "sp-selection.h"
#include "event-broker.h"
#include "path-chemistry.h"
#endif

void
sp_selected_path_combine (void)
{
#if 0
	GList * l;
	SPRepr * repr, * parent_repr;
	SPItem * item, * parent_group;
	SPPath * path;
	ArtBpath * bp, * abp;
	double i2g[6];
	gchar * d, * str, * style;

	l = sp_selection_list ();
	if (g_list_length (l) < 2)
		return;

	for (l = sp_selection_list (); l != NULL; l = l->next) {
		item = (SPItem *) l->data;
		if (!SP_IS_PATH (item))
			return;
		if (!((SPPath *) item)->independent)
			return;
	}

	sp_event_context_set_select ();

	parent_group = sp_drawing_current_group ();
	d = "";
	style = g_strdup (sp_repr_attr (((SPItem *) sp_selection_list()->data)->repr, "style"));

	for (l = sp_selection_list (); l != NULL; l = l->next) {
		path = (SPPath *) l->data;
		bp = sp_path_normalize (path);
		gnome_canvas_item_i2i_affine ((GnomeCanvasItem *) path, (GnomeCanvasItem *) parent_group, i2g);
		abp = art_bpath_affine_transform (bp, i2g);
		str = sp_svg_write_path (abp);
		art_free (abp);
		d = g_strconcat (d, str, NULL);
		g_free (str);
		sp_repr_unparent_and_destroy (repr);
	}

	repr = sp_repr_new ();
	sp_repr_set_name (repr, "path");
	sp_repr_set_attr (repr, "style", style);
	g_free (style);
	sp_repr_set_attr (repr, "d", d);
	g_free (d);
	sp_repr_append_child (parent_repr, repr);
	sp_repr_unref (repr);

	sp_selection_set_repr (repr);
#endif
}

void
sp_selected_path_break_apart (void)
{
#if 0
	SPRepr * repr, * parent_repr;
	SPItem * item, * parent_group;
	SPPath * path;
	ArtBpath * bpath, * abp, * nbp;
	double i2g[6];
	gchar * style, * str;
	gint len, pos, newpos;

	repr = sp_selection_repr ();
	if (repr == NULL)
		return;
	item = sp_repr_item (repr);
	if (!SP_IS_PATH (item))
		return;
	path = SP_PATH (item);
	if (!sp_path_independent (path))
		return;

	sp_selection_empty ();

	parent_repr = sp_drawing_current_group ();
	parent_group = sp_repr_item (parent_repr);

	bpath = sp_path_normalized_bpath (path);
	if (bpath == NULL)
		return;
	gnome_canvas_item_i2i_affine ((GnomeCanvasItem *) path, (GnomeCanvasItem *) parent_group, i2g);
	style = g_strdup (sp_repr_attr (repr, "style"));

	sp_repr_unparent_and_destroy (repr);

	abp = art_bpath_affine_transform (bpath, i2g);
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
		sp_repr_append_child (parent_repr, repr);
		sp_repr_unref (repr);
	}

	art_free (nbp);
	art_free (abp);
	g_free (style);
#endif
}

void
sp_selected_path_to_curves (void)
{
#if 0
	SPRepr * repr, * parent, * new;
	SPPath * path;
	ArtBpath * bpath;
	gchar * str;
	const gchar * transform, * style;
	gint pos;

	repr = sp_selection_repr ();
	if (repr == NULL)
		return;

	sp_selection_empty ();

	path = (SPPath *) sp_repr_item (repr);
	if (!SP_IS_PATH (path))
		return;

	parent = sp_repr_parent (repr);
	pos = sp_repr_position (repr);

	bpath = sp_path_normalized_bpath (path);
	str = sp_svg_write_path (bpath);
	art_free (bpath);
	transform = sp_repr_attr (repr, "transform");
	style = sp_repr_attr (repr, "style");

	new = sp_repr_new_with_name ("path");
	sp_repr_set_attr (new, "transform", transform);
	sp_repr_set_attr (new, "style", style);
	sp_repr_set_attr (new, "d", str);

	g_free (str);

	sp_repr_unparent_and_destroy (repr);
	sp_repr_add_child (parent, new, pos);
	sp_repr_unref (new);

	sp_selection_set_repr (new);
#endif
}
