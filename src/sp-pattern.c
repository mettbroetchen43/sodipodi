#define __SP_PATTERN_C__

/*
 * SVG <pattern> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>
#include <libnr/nr-matrix.h>
#include <gtk/gtksignal.h>
#include "helper/nr-plain-stuff.h"
#include "svg/svg.h"
#include "document.h"
#include "sp-pattern.h"

/*
 * Pattern
 */

static void sp_pattern_class_init (SPPatternClass *klass);
static void sp_pattern_init (SPPattern *gr);
static void sp_pattern_finalize (GtkObject *object);

static void sp_pattern_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_pattern_read_attr (SPObject *object, const gchar *key);
static void sp_pattern_child_added (SPObject *object, SPRepr *child, SPRepr *ref);
static void sp_pattern_remove_child (SPObject *object, SPRepr *child);
static void sp_pattern_modified (SPObject *object, guint flags);

static void sp_pattern_href_destroy (SPObject *href, SPPattern *pattern);
static void sp_pattern_href_modified (SPObject *href, guint flags, SPPattern *pattern);

static SPPainter *sp_pattern_painter_new (SPPaintServer *ps, const gdouble *affine, const ArtDRect *bbox);
static void sp_pattern_painter_free (SPPaintServer *ps, SPPainter *painter);

static SPPaintServerClass * pattern_parent_class;

GtkType
sp_pattern_get_type (void)
{
	static GtkType pattern_type = 0;
	if (!pattern_type) {
		GtkTypeInfo pattern_info = {
			"SPPattern",
			sizeof (SPPattern),
			sizeof (SPPatternClass),
			(GtkClassInitFunc) sp_pattern_class_init,
			(GtkObjectInitFunc) sp_pattern_init,
			NULL, NULL, NULL
		};
		pattern_type = gtk_type_unique (SP_TYPE_PAINT_SERVER, &pattern_info);
	}
	return pattern_type;
}

static void
sp_pattern_class_init (SPPatternClass *klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;
	SPPaintServerClass *ps_class;

	gtk_object_class = (GtkObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;
	ps_class = (SPPaintServerClass *) klass;

	pattern_parent_class = gtk_type_class (SP_TYPE_PAINT_SERVER);

	gtk_object_class->finalize = sp_pattern_finalize;

	sp_object_class->build = sp_pattern_build;
	sp_object_class->read_attr = sp_pattern_read_attr;
	sp_object_class->child_added = sp_pattern_child_added;
	sp_object_class->remove_child = sp_pattern_remove_child;
	sp_object_class->modified = sp_pattern_modified;

	ps_class->painter_new = sp_pattern_painter_new;
	ps_class->painter_free = sp_pattern_painter_free;
}

static void
sp_pattern_init (SPPattern *pat)
{
	pat->patternUnits = SP_PATTERN_UNITS_OBJECTBOUNDINGBOX;
	pat->patternUnits = SP_PATTERN_UNITS_USERSPACEONUSE;

	nr_matrix_d_set_identity (&pat->patternTransform);

	sp_svg_length_unset (&pat->x, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&pat->y, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&pat->width, SP_SVG_UNIT_NONE, 0.0, 0.0);
	sp_svg_length_unset (&pat->height, SP_SVG_UNIT_NONE, 0.0, 0.0);
}

static void
sp_pattern_finalize (GtkObject *object)
{
	SPPattern *pat;

	pat = (SPPattern *) object;

	if (SP_OBJECT_DOCUMENT (object)) {
		/* Unregister ourselves */
		sp_document_remove_resource (SP_OBJECT_DOCUMENT (object), "pattern", SP_OBJECT (object));
	}

	if (pat->href) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (pat->href), pat);
		sp_object_hunref (SP_OBJECT (pat->href), object);
	}

	if (GTK_OBJECT_CLASS (pattern_parent_class)->finalize)
		(* GTK_OBJECT_CLASS (pattern_parent_class)->finalize) (object);
}

static void
sp_pattern_build (SPObject *object, SPDocument *document, SPRepr *repr)
{
	SPPattern *pat;

	pat = SP_PATTERN (object);

	if (SP_OBJECT_CLASS (pattern_parent_class)->build)
		(* SP_OBJECT_CLASS (pattern_parent_class)->build) (object, document, repr);

	sp_pattern_read_attr (object, "patternUnits");
	sp_pattern_read_attr (object, "patternContentUnits");
	sp_pattern_read_attr (object, "patternTransform");
	sp_pattern_read_attr (object, "x");
	sp_pattern_read_attr (object, "y");
	sp_pattern_read_attr (object, "width");
	sp_pattern_read_attr (object, "height");
	sp_pattern_read_attr (object, "viewBox");
	sp_pattern_read_attr (object, "xlink:href");

	/* Register ourselves */
	sp_document_add_resource (document, "pattern", object);
}

static void
sp_pattern_read_attr (SPObject *object, const gchar *key)
{
	SPPattern *pat;
	const guchar *val;

	pat = SP_PATTERN (object);

	val = sp_repr_attr (object->repr, key);

	/* fixme: We should unset properties, if val == NULL */
	if (!strcmp (key, "patternUnits")) {
		if (val) {
			if (!strcmp (val, "userSpaceOnUse")) {
				pat->patternUnits = SP_PATTERN_UNITS_USERSPACEONUSE;
			} else {
				pat->patternUnits = SP_PATTERN_UNITS_OBJECTBOUNDINGBOX;
			}
			pat->patternUnits_set = TRUE;
		} else {
			pat->patternUnits_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "patternContentUnits")) {
		if (val) {
			if (!strcmp (val, "userSpaceOnUse")) {
				pat->patternContentUnits = SP_PATTERN_UNITS_USERSPACEONUSE;
			} else {
				pat->patternContentUnits = SP_PATTERN_UNITS_OBJECTBOUNDINGBOX;
			}
			pat->patternContentUnits_set = TRUE;
		} else {
			pat->patternContentUnits_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "patternTransform")) {
		nr_matrix_d_set_identity (&pat->patternTransform);
		if (val) {
			sp_svg_read_affine (pat->patternTransform.c, val);
			pat->patternTransform_set = TRUE;
		} else {
			pat->patternTransform_set = FALSE;
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "xlink:href")) {
		if (pat->href) {
			gtk_signal_disconnect_by_data (GTK_OBJECT (pat->href), pat);
			pat->href = (SPPattern *) sp_object_hunref (SP_OBJECT (pat->href), object);
		}
		if (val && *val == '#') {
			SPObject *href;
			href = sp_document_lookup_id (object->document, val + 1);
			if (SP_IS_PATTERN (href)) {
				pat->href = (SPPattern *) sp_object_href (href, object);
				gtk_signal_connect (GTK_OBJECT (href), "destroy", GTK_SIGNAL_FUNC (sp_pattern_href_destroy), pat);
				gtk_signal_connect (GTK_OBJECT (href), "modified", GTK_SIGNAL_FUNC (sp_pattern_href_modified), pat);
			}
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
		return;
	} else {
		if (SP_OBJECT_CLASS (pattern_parent_class)->read_attr)
			(* SP_OBJECT_CLASS (pattern_parent_class)->read_attr) (object, key);
	}
}

static void
sp_pattern_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
}

static void
sp_pattern_remove_child (SPObject *object, SPRepr *child)
{
}

static void
sp_pattern_modified (SPObject *object, guint flags)
{
}


static void
sp_pattern_href_destroy (SPObject *href, SPPattern *pattern)
{
	pattern->href = (SPPattern *) sp_object_hunref (href, pattern);
	sp_object_request_modified (SP_OBJECT (pattern), SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_pattern_href_modified (SPObject *href, guint flags, SPPattern *pattern)
{
	sp_object_request_modified (SP_OBJECT (pattern), SP_OBJECT_MODIFIED_FLAG);
}

/* Painter */

typedef struct _SPPatPainter SPPatPainter;

struct _SPPatPainter {
	SPPainter painter;
	SPPattern *pat;
};

static void
sp_pat_fill (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride)
{
#if 0
	SPLGPainter *lgp;
	SPLinearPattern *lg;
	SPPattern *g;
	gdouble pos;
	gint x, y;
	guchar *p;

	lgp = (SPLGPainter *) painter;
	lg = lgp->lg;
	g = (SPPattern *) lg;

	if (!g->color) {
		/* fixme: This is forbidden, so we should paint some mishmesh instead */
		nr_render_r8g8b8a8_gray_garbage (px, width, height, rowstride);
		return;
	}

	for (y = 0; y < height; y++) {
		p = px + y * rowstride;
		pos = (y + y0 - lgp->y0) * lgp->dy + (0 + x0 - lgp->x0) * lgp->dx;
		for (x = 0; x < width; x++) {
			gint idx;
			if (g->spread == SP_PATTERN_SPREAD_REFLECT) {
				idx = ((gint) pos) & (2 * NCOLORS - 1);
				if (idx & NCOLORS) idx = (2 * NCOLORS) - idx;
			} else if (g->spread == SP_PATTERN_SPREAD_REPEAT) {
				idx = ((gint) pos) & (NCOLORS - 1);
			} else {
				idx = CLAMP (((gint) pos), 0, (NCOLORS - 1));
			}
			idx = idx * 4;
			*p++ = g->color[idx];
			*p++ = g->color[idx + 1];
			*p++ = g->color[idx + 2];
			*p++ = g->color[idx + 3];
			pos += lgp->dx;
		}
	}
#else
	nr_render_r8g8b8a8_gray_garbage (px, width, height, rowstride);
#endif
}

static SPPainter *
sp_pattern_painter_new (SPPaintServer *ps, const gdouble *ctm, const ArtDRect *bbox)
{
	SPPattern *pat;
	SPPatPainter *pp;

	pat = SP_PATTERN (ps);

	pp = g_new (SPPatPainter, 1);

	pp->painter.type = SP_PAINTER_IND;
	pp->painter.fill = sp_pat_fill;

	pp->pat = pat;

#if 0
	/* fixme: Technically speaking, we map NCOLORS on line [start,end] onto line [0,1] (Lauris) */
	/* fixme: I almost think, we should fill color array start and end in that case (Lauris) */
	/* fixme: The alternative would be to leave these just empty garbage or something similar (Lauris) */
	/* fixme: Originally I had 1023.9999 here - not sure, whether we have really to cut out ceil int (Lauris) */
	art_affine_scale (color2norm, gr->len / (gdouble) NCOLORS, gr->len / (gdouble) NCOLORS);
	SP_PRINT_TRANSFORM ("color2norm", color2norm);
	/* Now we have normalized vector */

	if (gr->units == SP_PATTERN_UNITS_OBJECTBOUNDINGBOX) {
		gdouble norm2pos[6], bbox2user[6];
		gdouble color2pos[6], color2tpos[6], color2user[6], color2px[6], px2color[6];

		/* This is easy case, as we can just ignore percenting here */
		/* fixme: Still somewhat tricky, but I think I got it correct (lauris) */
		norm2pos[0] = lg->x2.computed - lg->x1.computed;
		norm2pos[1] = lg->y2.computed - lg->y1.computed;
		norm2pos[2] = lg->y2.computed - lg->y1.computed;
		norm2pos[3] = lg->x1.computed - lg->x2.computed;
		norm2pos[4] = lg->x1.computed;
		norm2pos[5] = lg->y1.computed;
		SP_PRINT_TRANSFORM ("norm2pos", norm2pos);

		/* patternTransform goes here (Lauris) */
		SP_PRINT_TRANSFORM ("patternTransform", gr->transform);

		/* BBox to user coordinate system */
		bbox2user[0] = bbox->x1 - bbox->x0;
		bbox2user[1] = 0.0;
		bbox2user[2] = 0.0;
		bbox2user[3] = bbox->y1 - bbox->y0;
		bbox2user[4] = bbox->x0;
		bbox2user[5] = bbox->y0;
		SP_PRINT_TRANSFORM ("bbox2user", bbox2user);

		/* CTM goes here */
		SP_PRINT_TRANSFORM ("ctm", ctm);

		art_affine_multiply (color2pos, color2norm, norm2pos);
		SP_PRINT_TRANSFORM ("color2pos", color2pos);
		art_affine_multiply (color2tpos, color2pos, gr->transform);
		SP_PRINT_TRANSFORM ("color2tpos", color2tpos);
		art_affine_multiply (color2user, color2tpos, bbox2user);
		SP_PRINT_TRANSFORM ("color2user", color2user);
		art_affine_multiply (color2px, color2user, ctm);
		SP_PRINT_TRANSFORM ("color2px", color2px);

		art_affine_invert (px2color, color2px);
		SP_PRINT_TRANSFORM ("px2color", px2color);

		lgp->x0 = color2px[4];
		lgp->y0 = color2px[5];
		lgp->dx = px2color[0];
		lgp->dy = px2color[2];
	} else {
		gdouble norm2pos[6];
		gdouble color2pos[6], color2tpos[6], color2px[6], px2color[6];
		/* Problem: What to do, if we have mixed lengths and percentages? */
		/* Currently we do ignore percentages at all, but that is not good (lauris) */

		/* fixme: Do percentages (Lauris) */
		norm2pos[0] = lg->x2.computed - lg->x1.computed;
		norm2pos[1] = lg->y2.computed - lg->y1.computed;
		norm2pos[2] = lg->y2.computed - lg->y1.computed;
		norm2pos[3] = lg->x1.computed - lg->x2.computed;
		norm2pos[4] = lg->x1.computed;
		norm2pos[5] = lg->y1.computed;
		SP_PRINT_TRANSFORM ("norm2pos", norm2pos);

		/* patternTransform goes here (Lauris) */
		SP_PRINT_TRANSFORM ("patternTransform", gr->transform);

		/* CTM goes here */
		SP_PRINT_TRANSFORM ("ctm", ctm);

		art_affine_multiply (color2pos, color2norm, norm2pos);
		SP_PRINT_TRANSFORM ("color2pos", color2pos);
		art_affine_multiply (color2tpos, color2pos, gr->transform);
		SP_PRINT_TRANSFORM ("color2tpos", color2tpos);
		art_affine_multiply (color2px, color2tpos, ctm);
		SP_PRINT_TRANSFORM ("color2px", color2px);

		art_affine_invert (px2color, color2px);
		SP_PRINT_TRANSFORM ("px2color", px2color);

		lgp->x0 = color2px[4];
		lgp->y0 = color2px[5];
		lgp->dx = px2color[0];
		lgp->dy = px2color[2];
	}
#endif

	return (SPPainter *) pp;
}

static void
sp_pattern_painter_free (SPPaintServer *ps, SPPainter *painter)
{
	SPPatPainter *pp;

	pp = (SPPatPainter *) painter;

	g_free (pp);
}

#if 0
/*
 * Linear Pattern
 */


static void
sp_linearpattern_read_attr (SPObject * object, const gchar * key)
{
	SPLinearPattern * lg;
	const guchar *str;

	lg = SP_LINEARPATTERN (object);

	str = sp_repr_attr (object->repr, key);

	if (!strcmp (key, "x1")) {
		if (!sp_svg_length_read (str, &lg->x1)) {
			sp_svg_length_unset (&lg->x1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "y1")) {
		if (!sp_svg_length_read (str, &lg->y1)) {
			sp_svg_length_unset (&lg->y1, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "x2")) {
		if (!sp_svg_length_read (str, &lg->x2)) {
			sp_svg_length_unset (&lg->x2, SP_SVG_UNIT_PERCENT, 1.0, 1.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "y2")) {
		if (!sp_svg_length_read (str, &lg->y2)) {
			sp_svg_length_unset (&lg->y2, SP_SVG_UNIT_PERCENT, 0.0, 0.0);
		}
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (SP_OBJECT_CLASS (lg_parent_class)->read_attr)
			(* SP_OBJECT_CLASS (lg_parent_class)->read_attr) (object, key);
	}
}

#endif
