#define __SP_FILL_STYLE_C__

/*
 * Display settings dialog
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

#include <config.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>

#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include "../svg/svg.h"
#include "../widgets/sp-widget.h"
#include "../widgets/paint-selector.h"
#include "../style.h"
#include "../gradient-chemistry.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-gradient.h"
#include "../sodipodi.h"

#include "fill-style.h"

static void sp_fill_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel);
static void sp_fill_style_widget_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel);
static void sp_fill_style_widget_update (SPWidget *spw, SPSelection *sel);

static void sp_fill_style_widget_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw);
static void sp_fill_style_widget_paint_dragged (SPPaintSelector *psel, SPWidget *spw);
static void sp_fill_style_widget_paint_changed (SPPaintSelector *psel, SPWidget *spw);

static void sp_fill_style_get_average_color_rgba (const GSList *objects, gfloat *c);
static void sp_fill_style_get_average_color_cmyka (const GSList *objects, gfloat *c);
static SPPaintSelectorMode sp_fill_style_determine_paint_selector_mode (SPObject *object);

static GtkWidget *dialog = NULL;

static void
sp_fill_style_dialog_destroy (GtkObject *object, gpointer data)
{
	dialog = NULL;
}

void
sp_fill_style_dialog (void)
{
	if (!dialog) {
		GtkWidget *w;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Fill style"));
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy", GTK_SIGNAL_FUNC (sp_fill_style_dialog_destroy), NULL);
		w = sp_fill_style_widget_new ();
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	}

	gtk_widget_show (dialog);
}

GtkWidget *
sp_fill_style_widget_new (void)
{
	GtkWidget *spw, *psel;

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	psel = sp_paint_selector_new ();
	gtk_widget_show (psel);
	gtk_container_add (GTK_CONTAINER (spw), psel);
	gtk_object_set_data (GTK_OBJECT (spw), "paint-selector", psel);

	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_fill_style_widget_modify_selection), psel);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_fill_style_widget_change_selection), psel);

	gtk_signal_connect (GTK_OBJECT (psel), "mode_changed", GTK_SIGNAL_FUNC (sp_fill_style_widget_paint_mode_changed), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "dragged", GTK_SIGNAL_FUNC (sp_fill_style_widget_paint_dragged), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "changed", GTK_SIGNAL_FUNC (sp_fill_style_widget_paint_changed), spw);

	sp_fill_style_widget_update (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

static void
sp_fill_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG)) {
		sp_fill_style_widget_update (spw, selection);
	}
}

static void
sp_fill_style_widget_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel)
{
	sp_fill_style_widget_update (spw, selection);
}

static void
sp_fill_style_widget_update (SPWidget *spw, SPSelection *sel)
{
	SPPaintSelector *psel;
	SPPaintSelectorMode pselmode;
	const GSList *objects, *l;
	SPObject *object;
	SPGradient *vector;
	gfloat c[5];
	ArtDRect bbox;
	SPLinearGradient *lg;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	psel = gtk_object_get_data (GTK_OBJECT (spw), "paint-selector");

	if (!sel || sp_selection_is_empty (sel)) {
		/* No objects, set empty */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_EMPTY);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}

	objects = sp_selection_item_list (sel);
	object = SP_OBJECT (objects->data);
	pselmode = sp_fill_style_determine_paint_selector_mode (object);

	for (l = objects->next; l != NULL; l = l->next) {
		SPPaintSelectorMode nextmode;
		nextmode = sp_fill_style_determine_paint_selector_mode (SP_OBJECT (l->data));
		if (nextmode != pselmode) {
			/* Multiple styles */
			sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
			gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
			return;
		}
	}

	g_print ("FillStyleWidget: paint selector mode %d\n", pselmode);

	switch (pselmode) {
	case SP_PAINT_SELECTOR_MODE_NONE:
		/* No paint at all */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_NONE);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_RGB);
		sp_fill_style_get_average_color_rgba (objects, c);
		sp_paint_selector_set_color_rgba_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_CMYK);
		sp_fill_style_get_average_color_cmyka (objects, c);
		sp_paint_selector_set_color_cmyka_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		object = SP_OBJECT (objects->data);
		/* We know that all objects have lineargradient fill style */
		vector = sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_FILL_SERVER (object)), FALSE);
		for (l = objects->next; l != NULL; l = l->next) {
			SPObject *next;
			next = SP_OBJECT (l->data);
			if (sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_FILL_SERVER (next)), FALSE) != vector) {
				/* Multiple vectors */
				sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
				gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
				return;
			}
		}
		/* fixme: Probably we should set multiple mode here too */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);
		sp_paint_selector_set_gradient_linear (psel, vector);
		sp_selection_bbox (sel, &bbox);
		sp_paint_selector_set_gradient_bbox (psel, bbox.x0, bbox.y0, bbox.x1, bbox.y1);
		/* fixme: This is plain wrong */
		lg = SP_LINEARGRADIENT (SP_OBJECT_STYLE_FILL_SERVER (object));
		/* fixme: Take units into account! */
		sp_paint_selector_set_gradient_position (psel, lg->x1.computed, lg->y1.computed, lg->x2.computed, lg->y2.computed);
		break;
	}

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

#if 0
	SPGradient *gradient;
	GtkWidget *vectors, *position;
#if 0
	GtkWidget *menu;
	gint pos;
	GList *children, *l;
#endif
	const GSList *items, *i;
	ArtDRect bbox;
	SPLinearGradient *lg;

	if (!selection) return;

	items = sp_selection_item_list (selection);

	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->fill.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->fill.server)) {
			gradient = SP_GRADIENT (style->fill.server);
			while (gradient) {
				/* Search vector gradient */
				sp_gradient_ensure_vector (gradient);
				if (gradient->has_stops) break;
				gradient = gradient->href;
			}
			if (gradient) break;
		}
	}
	/* Return if no vector gradient */
	if (!gradient) return;

	vectors = gtk_object_get_data (GTK_OBJECT (spw), "vectors");

#if 0
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (vectors));
	children = gtk_container_children (GTK_CONTAINER (menu));

	pos = 0;
	for (l = children; l != NULL; l = l->next) {
		SPGradient *gr;
		gr = gtk_object_get_data (GTK_OBJECT (l->data), "gradient");
		if (gr == gradient) break;
		pos += 1;
	}

	g_list_free (children);

	gtk_option_menu_set_history (GTK_OPTION_MENU (vectors), pos);
#else
	sp_gradient_vector_selector_set_gradient (SP_GRADIENT_VECTOR_SELECTOR (vectors), (gradient) ? SP_OBJECT_DOCUMENT (gradient) : NULL, gradient);
#endif

	gtk_object_set_data (GTK_OBJECT (spw), "gradient", gradient);

	/* Update position */
	position = gtk_object_get_data (GTK_OBJECT (spw), "position");
	gradient = NULL;
	for (i = items; i != NULL; i = i->next) {
		SPStyle *style;
		style = SP_OBJECT_STYLE (i->data);
		if ((style->fill.type == SP_PAINT_TYPE_PAINTSERVER) && SP_IS_LINEARGRADIENT (style->fill.server)) {
			gradient = SP_GRADIENT (style->fill.server);
			break;
		}
	}
	g_assert (gradient != NULL);
	sp_gradient_position_set_gradient (SP_GRADIENT_POSITION (position), SP_GRADIENT (gradient));
	sp_selection_bbox (selection, &bbox);
	sp_gradient_position_set_bbox (SP_GRADIENT_POSITION (position), bbox.x0, bbox.y0, bbox.x1, bbox.y1);
	lg = SP_LINEARGRADIENT (gradient);
	sp_gradient_position_set_vector (SP_GRADIENT_POSITION (position), lg->x1.distance, lg->y1.distance, lg->x2.distance, lg->y2.distance);
#endif

static void
sp_fill_style_widget_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	/* fixme: Does this work? */
	/* fixme: Not really, here we have to get old color back from object */
	/* Instead of relying on paint widget having meaningful colors set */
	sp_fill_style_widget_paint_changed (psel, spw);
}

static void
sp_fill_style_widget_paint_dragged (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i;
	SPGradient *vector;
	gfloat c[5];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	g_print ("FillStyleWidget: paint dragged\n");

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
	case SP_PAINT_SELECTOR_MODE_NONE:
		g_warning ("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_get_rgba_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_fill_color_rgba (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], TRUE, TRUE);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_get_cmyka_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_fill_color_cmyka (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], c[4], TRUE, TRUE);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		vector = sp_paint_selector_get_gradient_vector (psel);
		vector = sp_gradient_ensure_vector_normalized (vector);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			gfloat p[4];
			sp_item_force_fill_lineargradient_vector (SP_ITEM (i->data), vector);
			sp_paint_selector_get_gradient_position_floatv (psel, p);
			sp_lineargradient_set_position (SP_LINEARGRADIENT (SP_OBJECT_STYLE_FILL_SERVER (i->data)), p[0], p[1], p[2], p[3]);
			/* fixme: Managing selection bbox/item bbox stuff is big mess */
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		g_warning ("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, psel->mode);
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}
}

static void
sp_fill_style_widget_paint_changed (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i;
	SPCSSAttr *css;
	gfloat rgba[4], cmyka[5];
	SPGradient *vector;
	guchar b[64];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	g_print ("FillStyleWidget: paint changed\n");

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
		g_warning ("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_NONE:
		css = sp_repr_css_attr_new ();
		sp_repr_css_set_property (css, "fill", "none");
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "fill-cmyk", NULL);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_rgba_floatv (psel, rgba);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "fill", b);
		g_snprintf (b, 64, "%g", rgba[3]);
		sp_repr_css_set_property (css, "fill-opacity", b);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			g_print ("FillStyleWidget: Set RGBA fill style\n");
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			g_print ("FillStyleWidget: Set RGBA fill-cmyk style\n");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "fill-cmyk", NULL);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_cmyka_floatv (psel, cmyka);
		sp_color_cmyk_to_rgb_floatv (rgba, cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "fill", b);
		g_snprintf (b, 64, "%g", cmyka[4]);
		sp_repr_css_set_property (css, "fill-opacity", b);
		g_snprintf (b, 64, "(%g %g %g %g)", cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			g_print ("FillStyleWidget: Set CMYK fill style\n");
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			g_print ("FillStyleWidget: Set CMYK fill-cmyk style\n");
			sp_repr_set_attr_recursive (SP_OBJECT_REPR (i->data), "fill-cmyk", b);
		}
		sp_repr_css_attr_unref (css);
		sp_document_done (spw->document);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		items = sp_widget_get_item_list (spw);
		if (items) {
			vector = sp_paint_selector_get_gradient_vector (psel);
			if (!vector) {
				g_warning ("SPFillStyleWidget: Got linearGradient mode but NULL gradient in 'changed' handler\n");
				vector = sp_document_default_gradient_vector (spw->document);
			}
			vector = sp_gradient_ensure_vector_normalized (vector);
			for (i = items; i != NULL; i = i->next) {
				gfloat p[4];
				g_print ("FillStyleWidget: Set LinearGradient %s fill style\n", SP_OBJECT_ID (vector));
				sp_item_force_fill_lineargradient_vector (SP_ITEM (i->data), vector);
				/* fixme: Managing selection bbox/item bbox stuff is big mess */
				sp_paint_selector_get_gradient_position_floatv (psel, p);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_FILL_SERVER (i->data)), "x1", p[0]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_FILL_SERVER (i->data)), "y1", p[1]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_FILL_SERVER (i->data)), "x2", p[2]);
				sp_repr_set_double (SP_OBJECT_REPR (SP_OBJECT_STYLE_FILL_SERVER (i->data)), "y2", p[3]);
			}
			sp_document_done (spw->document);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		g_warning ("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, psel->mode);
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}
}

static void
sp_fill_style_get_average_color_rgba (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[3];
		object = SP_OBJECT (objects->data);
		if (object->style->fill.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_rgb_floatv (&object->style->fill.value.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += SP_SCALE24_TO_FLOAT (object->style->fill_opacity.value);
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
}

static void
sp_fill_style_get_average_color_cmyka (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	c[4] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[4];
		object = SP_OBJECT (objects->data);
		if (object->style->fill.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_cmyk_floatv (&object->style->fill.value.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += d[3];
			c[4] += SP_SCALE24_TO_FLOAT (object->style->fill_opacity.value);
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
	c[4] /= num;
}

static SPPaintSelectorMode
sp_fill_style_determine_paint_selector_mode (SPObject *object)
{
	SPColorSpaceType cstype;

	switch (object->style->fill.type) {
	case SP_PAINT_TYPE_NONE:
		return SP_PAINT_SELECTOR_MODE_NONE;
	case SP_PAINT_TYPE_COLOR:
		cstype = sp_color_get_colorspace_type (&object->style->fill.value.color);
		switch (cstype) {
		case SP_COLORSPACE_TYPE_RGB:
			return SP_PAINT_SELECTOR_MODE_COLOR_RGB;
		case SP_COLORSPACE_TYPE_CMYK:
			return SP_PAINT_SELECTOR_MODE_COLOR_CMYK;
		default:
			g_warning ("file %s: line %d: Unknown colorspace type %d", __FILE__, __LINE__, cstype);
			return SP_PAINT_SELECTOR_MODE_NONE;
		}
	case SP_PAINT_TYPE_PAINTSERVER:
		if (SP_IS_LINEARGRADIENT (SP_OBJECT_STYLE_FILL_SERVER (object))) {
			return SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR;
		}
		g_warning ("file %s: line %d: Unknown paintserver", __FILE__, __LINE__);
		return SP_PAINT_SELECTOR_MODE_NONE;
	default:
		g_warning ("file %s: line %d: Unknown paint type %d", __FILE__, __LINE__, object->style->fill.type);
		break;
	}

	return SP_PAINT_SELECTOR_MODE_NONE;
}

