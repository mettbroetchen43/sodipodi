#define __SP_NAMEDVIEW_C__

/*
 * <sodipodi:namedview> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <math.h>
#include <string.h>
#include <ctype.h>
#include "helper/canvas-grid.h"
#include "svg/svg.h"
#include "document.h"
#include "desktop.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "sp-guide.h"
#include "sp-namedview.h"

#define PTPERMM (72.0 / 25.4)

#define DEFAULTTOLERANCE 5.0
#define DEFAULTGRIDCOLOR 0x3f3fff2f
#define DEFAULTGUIDECOLOR 0x0000ff7f
#define DEFAULTGUIDEHICOLOR 0xff00007f

static void sp_namedview_class_init (SPNamedViewClass * klass);
static void sp_namedview_init (SPNamedView * namedview);

static void sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_namedview_release (SPObject *object);
static void sp_namedview_read_attr (SPObject * object, const gchar * key);
static void sp_namedview_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_namedview_remove_child (SPObject *object, SPRepr *child);
static SPRepr *sp_namedview_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_namedview_setup_grid (SPNamedView * nv);
static void sp_namedview_setup_grid_item (SPNamedView * nv, SPCanvasItem * item);

static gboolean sp_str_to_bool (const guchar *str);
static gboolean sp_nv_read_length (const guchar *str, guint base, gdouble *val, const SPUnit **unit);
static gboolean sp_nv_read_opacity (const guchar *str, guint32 *color);

static SPObjectGroupClass * parent_class;

GType
sp_namedview_get_type (void)
{
	static GType namedview_type = 0;
	if (!namedview_type) {
		GTypeInfo namedview_info = {
			sizeof (SPNamedViewClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_namedview_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPNamedView),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_namedview_init,
		};
		namedview_type = g_type_register_static (SP_TYPE_OBJECTGROUP, "SPNamedView", &namedview_info, 0);
	}
	return namedview_type;
}

static void
sp_namedview_class_init (SPNamedViewClass * klass)
{
	GObjectClass * gobject_class;
	SPObjectClass * sp_object_class;

	gobject_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = g_type_class_ref (SP_TYPE_OBJECTGROUP);

	sp_object_class->build = sp_namedview_build;
	sp_object_class->release = sp_namedview_release;
	sp_object_class->read_attr = sp_namedview_read_attr;
	sp_object_class->child_added = sp_namedview_child_added;
	sp_object_class->remove_child = sp_namedview_remove_child;
	sp_object_class->write = sp_namedview_write;
}

static void
sp_namedview_init (SPNamedView * nv)
{
	nv->editable = TRUE;
	nv->showgrid = FALSE;
	nv->snaptogrid = FALSE;
	nv->showguides = FALSE;
	nv->snaptoguides = FALSE;
	nv->showborder = TRUE;

	nv->hguides = NULL;
	nv->vguides = NULL;
	nv->viewcount = 0;
}

static void
sp_namedview_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPNamedView * nv;
	SPObjectGroup * og;
        SPObject * o;

	nv = (SPNamedView *) object;
	og = (SPObjectGroup *) object;

	if (((SPObjectClass *) (parent_class))->build)
		(* ((SPObjectClass *) (parent_class))->build) (object, document, repr);

	sp_namedview_read_attr (object, "viewonly");
	sp_namedview_read_attr (object, "showgrid");
	sp_namedview_read_attr (object, "snaptogrid");
	sp_namedview_read_attr (object, "showguides");
	sp_namedview_read_attr (object, "snaptoguides");
	sp_namedview_read_attr (object, "gridtolerance");
	sp_namedview_read_attr (object, "guidetolerance");
	sp_namedview_read_attr (object, "gridoriginx");
	sp_namedview_read_attr (object, "gridoriginy");
	sp_namedview_read_attr (object, "gridspacingx");
	sp_namedview_read_attr (object, "gridspacingy");
	sp_namedview_read_attr (object, "gridcolor");
	sp_namedview_read_attr (object, "gridopacity");
	sp_namedview_read_attr (object, "guidecolor");
	sp_namedview_read_attr (object, "guideopacity");
	sp_namedview_read_attr (object, "guidehicolor");
	sp_namedview_read_attr (object, "guidehiopacity");
	sp_namedview_read_attr (object, "showborder");

	/* Construct guideline list */

	for (o = og->children; o != NULL; o = o->next) {
		if (SP_IS_GUIDE (o)) {
			SPGuide * g;
			g = SP_GUIDE (o);
			if (g->orientation == SP_GUIDE_HORIZONTAL) {
				nv->hguides = g_slist_prepend (nv->hguides, g);
			} else {
				nv->vguides = g_slist_prepend (nv->vguides, g);
			}
			g_object_set (G_OBJECT (g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
		}
	}
}

static void
sp_namedview_release (SPObject * object)
{
	SPNamedView *namedview;

	namedview = (SPNamedView *) object;

	if (namedview->hguides) {
		g_slist_free (namedview->hguides);
		namedview->hguides = NULL;
	}

	if (namedview->vguides) {
		g_slist_free (namedview->vguides);
		namedview->vguides = NULL;
	}

	g_assert (!namedview->views);

	while (namedview->gridviews) {
		gtk_object_destroy (GTK_OBJECT (namedview->gridviews->data));
		namedview->gridviews = g_slist_remove (namedview->gridviews, namedview->gridviews->data);
	}

	if (((SPObjectClass *) parent_class)->release)
		((SPObjectClass *) parent_class)->release (object);
}

static void
sp_namedview_read_attr (SPObject * object, const gchar * key)
{
	SPNamedView *nv;
	const gchar *astr;
	const SPUnit *pt = NULL;
	const SPUnit *px = NULL;
	const SPUnit *mm = NULL;
	GSList * l;

	nv = SP_NAMEDVIEW (object);

	if (!pt) pt = sp_unit_get_by_abbreviation ("pt");
	if (!px) px = sp_unit_get_by_abbreviation ("px");
	if (!mm) mm = sp_unit_get_by_abbreviation ("mm");

	astr = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "viewonly")) {
		nv->editable = (astr == NULL);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "showgrid")) {
		nv->showgrid = sp_str_to_bool (astr);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "snaptogrid")) {
		nv->snaptogrid = sp_str_to_bool (astr);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "showguides")) {
		nv->showguides = sp_str_to_bool (astr);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "snaptoguides")) {
		nv->snaptoguides = sp_str_to_bool (astr);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridtolerance")) {
		nv->gridtoleranceunit = px;
		nv->gridtolerance = DEFAULTTOLERANCE;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &nv->gridtolerance, &nv->gridtoleranceunit);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "guidetolerance")) {
		nv->guidetoleranceunit = px;
		nv->guidetolerance = DEFAULTTOLERANCE;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE | SP_UNIT_DEVICE, &nv->guidetolerance, &nv->guidetoleranceunit);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (strcmp (key, "gridoriginx") == 0) {
		nv->gridunit = mm;
		nv->gridoriginx = 0.0;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE, &nv->gridoriginx, &nv->gridunit);
		}
		sp_convert_distance (&nv->gridoriginx, nv->gridunit, pt);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridoriginy")) {
		nv->gridunit = mm;
		nv->gridoriginy = 0.0;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE, &nv->gridoriginy, &nv->gridunit);
		}
		sp_convert_distance (&nv->gridoriginy, nv->gridunit, pt);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridspacingx")) {
		nv->gridunit = mm;
		nv->gridspacingx = 5.0;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE, &nv->gridspacingx, &nv->gridunit);
		}
		sp_convert_distance (&nv->gridspacingx, nv->gridunit, pt);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridspacingy")) {
		nv->gridunit = mm;
		nv->gridspacingy = 5.0;
		if (astr) {
			sp_nv_read_length (astr, SP_UNIT_ABSOLUTE, &nv->gridspacingy, &nv->gridunit);
		}
		sp_convert_distance (&nv->gridspacingy, nv->gridunit, pt);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridcolor")) {
		nv->gridcolor = (nv->gridcolor & 0xff) | (DEFAULTGRIDCOLOR & 0xffffff00);
		if (astr) {
			nv->gridcolor = (nv->gridcolor & 0xff) | sp_svg_read_color (astr, nv->gridcolor);
		}
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "gridopacity")) {
		nv->gridcolor = (nv->gridcolor & 0xffffff00) | (DEFAULTGRIDCOLOR & 0xff);
		sp_nv_read_opacity (astr, &nv->gridcolor);
		sp_namedview_setup_grid (nv);
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "guidecolor")) {
		nv->guidecolor = (nv->guidecolor & 0xff) | (DEFAULTGUIDECOLOR & 0xffffff00);
		if (astr) {
			nv->guidecolor = (nv->guidecolor & 0xff) | sp_svg_read_color (astr, nv->guidecolor);
		}
		for (l = nv->hguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "color", nv->guidecolor, NULL);
		}
		for (l = nv->vguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "color", nv->guidecolor, NULL);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "guideopacity")) {
		nv->guidecolor = (nv->guidecolor & 0xffffff00) | (DEFAULTGUIDECOLOR & 0xff);
		sp_nv_read_opacity (astr, &nv->guidecolor);
		for (l = nv->hguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "color", nv->guidecolor, NULL);
		}
		for (l = nv->vguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "color", nv->guidecolor, NULL);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "guidehicolor")) {
		nv->guidehicolor = (nv->guidehicolor & 0xff) | (DEFAULTGUIDEHICOLOR & 0xffffff00);
		if (astr) {
			nv->guidehicolor = (nv->guidehicolor & 0xff) | sp_svg_read_color (astr, nv->guidehicolor);
		}
		for (l = nv->hguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "hicolor", nv->guidehicolor, NULL);
		}
		for (l = nv->vguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "hicolor", nv->guidehicolor, NULL);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "guidehiopacity")) {
		nv->guidehicolor = (nv->guidehicolor & 0xffffff00) | (DEFAULTGUIDEHICOLOR & 0xff);
		sp_nv_read_opacity (astr, &nv->guidehicolor);
		for (l = nv->hguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "hicolor", nv->guidehicolor, NULL);
		}
		for (l = nv->vguides; l != NULL; l = l->next) {
			g_object_set (G_OBJECT (l->data), "hicolor", nv->guidehicolor, NULL);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "showborder")) {
		nv->showborder = (astr) ? sp_str_to_bool (astr) : TRUE;
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (((SPObjectClass *) (parent_class))->read_attr)
			(* ((SPObjectClass *) (parent_class))->read_attr) (object, key);
	}
}

static void
sp_namedview_child_added (SPObject * object, SPRepr * child, SPRepr * ref)
{
	SPNamedView * nv;
	SPObject * no;
	const gchar * id;
	GSList * l;

	nv = (SPNamedView *) object;

	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	id = sp_repr_attr (child, "id");
	no = sp_document_lookup_id (object->document, id);
	g_assert (SP_IS_OBJECT (no));

	if (SP_IS_GUIDE (no)) {
		SPGuide * g;
		g = (SPGuide *) no;
		if (g->orientation == SP_GUIDE_HORIZONTAL) {
			nv->hguides = g_slist_prepend (nv->hguides, g);
		} else {
			nv->vguides = g_slist_prepend (nv->vguides, g);
		}
		g_object_set (G_OBJECT (g), "color", nv->guidecolor, "hicolor", nv->guidehicolor, NULL);
		if (nv->editable) {
			for (l = nv->views; l != NULL; l = l->next) {
				sp_guide_show (g, SP_DESKTOP (l->data)->guides, sp_dt_guide_event);
				if (SP_DESKTOP (l->data)->guides_active) sp_guide_sensitize (g, SP_DT_CANVAS (l->data), TRUE);
			}
		}
	}
}

static void
sp_namedview_remove_child (SPObject * object, SPRepr * child)
{
	SPNamedView * nv;
	SPObject * no;
	const gchar * id;

	nv = (SPNamedView *) object;

	id = sp_repr_attr (child, "id");
	no = sp_document_lookup_id (object->document, id);
	g_assert (no != NULL);

	if (SP_IS_GUIDE (no)) {
		SPGuide * g;
		g = (SPGuide *) no;
		if (g->orientation == SP_GUIDE_HORIZONTAL) {
			nv->hguides = g_slist_remove (nv->hguides, g);
		} else {
			nv->vguides = g_slist_remove (nv->vguides, g);
		}
	}

	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);
}

static SPRepr *
sp_namedview_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPNamedView *nv;

	nv = SP_NAMEDVIEW (object);

	if (flags & SP_OBJECT_WRITE_SODIPODI) {
		if (repr) {
			sp_repr_merge (repr, SP_OBJECT_REPR (object), "id");
		} else {
			repr = sp_repr_duplicate (SP_OBJECT_REPR (object));
		}
	}

	return repr;
}

void
sp_namedview_show (SPNamedView * nv, gpointer desktop)
{
	SPDesktop * dt;
	GSList * l;
	SPCanvasItem * item;

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), SP_DT_CANVAS (dt), TRUE);
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_show (SP_GUIDE (l->data), dt->guides, sp_dt_guide_event);
		if (dt->guides_active) sp_guide_sensitize (SP_GUIDE (l->data), SP_DT_CANVAS (dt), TRUE);
	}

	nv->views = g_slist_prepend (nv->views, desktop);

	item = sp_canvas_item_new (SP_DT_GRID (dt), SP_TYPE_CGRID, NULL);
	nv->gridviews = g_slist_prepend (nv->gridviews, item);
	sp_namedview_setup_grid_item (nv, item);
}

void
sp_namedview_hide (SPNamedView * nv, gpointer desktop)
{
	SPDesktop * dt;
	GSList * l;

	g_assert (nv != NULL);
	g_assert (SP_IS_NAMEDVIEW (nv));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (g_slist_find (nv->views, desktop));

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_hide (SP_GUIDE (l->data), SP_DT_CANVAS (dt));
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_hide (SP_GUIDE (l->data), SP_DT_CANVAS (dt));
	}

	nv->views = g_slist_remove (nv->views, desktop);

	for (l = nv->gridviews; l != NULL; l = l->next) {
		if (SP_CANVAS_ITEM (l->data)->canvas == SP_DT_CANVAS (dt)) break;
	}

	g_assert (l);

	g_object_unref (G_OBJECT (l->data));
	nv->gridviews = g_slist_remove (nv->gridviews, l->data);
}

void
sp_namedview_activate_guides (SPNamedView * nv, gpointer desktop, gboolean active)
{
	SPDesktop * dt;
	GSList * l;

	g_assert (nv != NULL);
	g_assert (SP_IS_NAMEDVIEW (nv));
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (g_slist_find (nv->views, desktop));

	dt = SP_DESKTOP (desktop);

	for (l = nv->hguides; l != NULL; l = l->next) {
		sp_guide_sensitize (SP_GUIDE (l->data), SP_DT_CANVAS (dt), active);
	}

	for (l = nv->vguides; l != NULL; l = l->next) {
		sp_guide_sensitize (SP_GUIDE (l->data), SP_DT_CANVAS (dt), active);
	}
}

static void
sp_namedview_setup_grid (SPNamedView * nv)
{
	GSList * l;

	for (l = nv->gridviews; l != NULL; l = l->next) {
		sp_namedview_setup_grid_item (nv, SP_CANVAS_ITEM (l->data));
	}
}

static void
sp_namedview_setup_grid_item (SPNamedView * nv, SPCanvasItem * item)
{
	const SPUnit *pt = NULL;

	if (nv->showgrid) {
		sp_canvas_item_show (item);
	} else {
		sp_canvas_item_hide (item);
	}

	if (!pt) pt = sp_unit_get_identity (SP_UNIT_ABSOLUTE);

	sp_canvas_item_set ((GtkObject *) item,
			       "color", nv->gridcolor,
			       "originx", nv->gridoriginx,
			       "originy", nv->gridoriginy,
			       "spacingx", nv->gridspacingx,
			       "spacingy", nv->gridspacingy,
			       NULL);
}

const gchar *
sp_namedview_get_name (SPNamedView * nv)
{
	SPException ex;
	gchar * name;

	SP_EXCEPTION_INIT (&ex);
  
	name = (gchar *)sp_object_getAttribute (SP_OBJECT (nv), "id", &ex);

	return name;
}

guint
sp_namedview_viewcount (SPNamedView * nv)
{
 g_assert (SP_IS_NAMEDVIEW (nv));

 return ++nv->viewcount;
}

const GSList *
sp_namedview_view_list (SPNamedView * nv)
{
 g_assert (SP_IS_NAMEDVIEW (nv));

 return nv->views;
}

/* This should be moved somewhere */

static gboolean
sp_str_to_bool (const guchar *str)
{
	if (str) {
		if (!strcasecmp (str, "true") ||
		    !strcasecmp (str, "yes") ||
		    !strcasecmp (str, "y") ||
		    (atoi (str) != 0)) return TRUE;
	}

	return FALSE;
}

/* fixme: Collect all these length parsing methods and think common sane API */

static gboolean
sp_nv_read_length (const guchar *str, guint base, gdouble *val, const SPUnit **unit)
{
	gdouble v;
	gchar *u;

	if (!str) return FALSE;

	v = strtod (str, &u);
	if (!u) return FALSE;
	while (isspace (*u)) u += 1;

	if (!*u) {
		/* No unit specified - keep default */
		*val = v;
		return TRUE;
	}

	if (base & SP_UNIT_DEVICE) {
		if (u[0] && u[1] && !isalnum (u[2]) && !strncmp (u, "px", 2)) {
			static const SPUnit *device = NULL;
			if (!device) device = sp_unit_get_identity (SP_UNIT_DEVICE);
			*unit = device;
			*val = v;
			return TRUE;
		}
	}

	if (base & SP_UNIT_ABSOLUTE) {
		static const SPUnit *pt = NULL;
		static const SPUnit *mm = NULL;
		static const SPUnit *cm = NULL;
		static const SPUnit *m = NULL;
		static const SPUnit *in = NULL;
		if (!pt) {
			pt = sp_unit_get_by_abbreviation ("pt");
			mm = sp_unit_get_by_abbreviation ("mm");
			cm = sp_unit_get_by_abbreviation ("cm");
			m = sp_unit_get_by_abbreviation ("m");
			in = sp_unit_get_by_abbreviation ("in");
		}
		if (!strncmp (u, "pt", 2)) {
			*unit = pt;
		} else if (!strncmp (u, "mm", 2)) {
			*unit = mm;
		} else if (!strncmp (u, "cm", 2)) {
			*unit = cm;
		} else if (!strncmp (u, "m", 1)) {
			*unit = m;
		} else if (!strncmp (u, "in", 2)) {
			*unit = in;
		} else {
			return FALSE;
		}
		*val = v;
		return TRUE;
	}

	return FALSE;
}

static gboolean
sp_nv_read_opacity (const guchar *str, guint32 *color)
{
	gdouble v;
	gchar *u;

	if (!str) return FALSE;

	v = strtod (str, &u);
	if (!u) return FALSE;
	v = CLAMP (v, 0.0, 1.0);

	*color = (*color & 0xffffff00) | (guint32) floor (v * 255.9999);

	return TRUE;
}
