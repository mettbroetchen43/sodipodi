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
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		g_print ("FillStyleWidget: selection modified\n");

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
		vector = sp_gradient_get_vector (SP_GRADIENT (object->style->fill.server), FALSE);
		for (l = objects->next; l != NULL; l = l->next) {
			SPObject *next;
			next = SP_OBJECT (l->data);
			if (sp_gradient_get_vector (SP_GRADIENT (next->style->fill.server), FALSE) != vector) {
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
		sp_paint_selector_set_gradient_position (psel, lg->x1.distance, lg->y1.distance, lg->x2.distance, lg->y2.distance);
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
			sp_object_style_changed (SP_OBJECT (i->data), SP_OBJECT_MODIFIED_FLAG);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_get_cmyka_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_fill_color_cmyka (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], c[4], TRUE, TRUE);
			sp_object_style_changed (SP_OBJECT (i->data), SP_OBJECT_MODIFIED_FLAG);
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
			sp_color_get_rgb_floatv (&object->style->fill.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += object->style->fill_opacity;
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
			sp_color_get_cmyk_floatv (&object->style->fill.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += d[3];
			c[4] += object->style->fill_opacity;
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
		cstype = sp_color_get_colorspace_type (&object->style->fill.color);
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
		if (SP_IS_LINEARGRADIENT (object->style->fill.server)) {
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

#if 0
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkframe.h>
#include <gtk/gtksignal.h>
#include <libgnomeui/gnome-pixmap.h>
#include "../widgets/sp-color-selector.h"
#include "../svg/svg.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-item.h"
#include "../sp-gradient.h"
#include "../style.h"
#include "../gradient-chemistry.h"
#include "sp-widget.h"
#include "gradient-selector.h"
#include "fill-style.h"

typedef enum {
	FSW_EMPTY,
	FSW_NONE,
	FSW_SOLID,
	FSW_GRADIENT,
	FSW_PATTERN,
	FSW_FRACTAL
} FSWFillType;

#define FSW_ALL (FSW_NONE | FSW_COLOR | FSW_GRADIENT | FSW_PATTERN | FSW_FRACTAL)

static void sp_fill_style_widget_destroy (SPWidget *spw, gpointer data);
static void sp_fill_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data);
static void sp_fill_style_widget_change_selection (SPWidget *spw, SPSelection *selection, gpointer data);
static void sp_fill_style_widget_type_toggled (GtkToggleButton *b, FSWFillType type);
static void sp_fill_style_widget_set_type (SPWidget *spw, FSWFillType type);
static void sp_fill_style_set_empty (SPWidget *spw);
static void sp_fill_style_set_none (SPWidget *spw);
static void sp_fill_style_set_solid (SPWidget *spw);
static void sp_fill_style_set_gradient (SPWidget *spw);
static void sp_fill_style_widget_reread (SPWidget *spw, SPSelection *selection);
static void sp_fill_style_widget_rgba_changed (SPColorSelector *csel, SPWidget *spw);
static void sp_fill_style_widget_rgba_dragged (SPColorSelector *csel, SPWidget *spw);

void sp_fill_style_dialog_close (GtkWidget *widget);
static gint sp_fill_style_dialog_delete (GtkWidget *widget, GdkEvent *event);

static gboolean blocked = FALSE;
static SPColorSelectorMode lastmode = SP_COLOR_SELECTOR_MODE_RGB;
static SPColor lastprocesscolor;
static gdouble lastalpha;
static GtkWidget *dialog = NULL;

/* Creates new instance of item properties widget */

GtkWidget *
sp_fill_style_widget_new (void)
{
	static gboolean colorinitialized = FALSE;
	GtkWidget *spw, *vb, *hb, *b, *f, *w;

	if (!colorinitialized) {
		/* Set up last used process color values */
		sp_color_set_rgb_float (&lastprocesscolor, 1.0, 1.0, 1.0);
		lastalpha = 1.0;
	}

	spw = sp_widget_new (SODIPODI, SP_ACTIVE_DESKTOP, SP_ACTIVE_DOCUMENT);

	vb = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vb);
	hb = gtk_hbox_new (TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hb), 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	/* fill:none */
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/fill_none.xpm");
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (b), FALSE);
	gtk_object_set_data (GTK_OBJECT (spw), "fill-none", b);
	gtk_object_set_data (GTK_OBJECT (b), "SPWidget", spw);
	gtk_signal_connect (GTK_OBJECT (b), "toggled",
			    GTK_SIGNAL_FUNC (sp_fill_style_widget_type_toggled), GINT_TO_POINTER (FSW_NONE));
	/* fill:color */
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/fill_solid.xpm");
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (b), FALSE);
	gtk_object_set_data (GTK_OBJECT (spw), "fill-solid", b);
	gtk_object_set_data (GTK_OBJECT (b), "SPWidget", spw);
	gtk_signal_connect (GTK_OBJECT (b), "toggled",
			    GTK_SIGNAL_FUNC (sp_fill_style_widget_type_toggled), GINT_TO_POINTER (FSW_SOLID));
	/* fill:gradient */
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/fill_gradient.xpm");
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (b), FALSE);
	gtk_object_set_data (GTK_OBJECT (spw), "fill-gradient", b);
	gtk_object_set_data (GTK_OBJECT (b), "SPWidget", spw);
	gtk_signal_connect (GTK_OBJECT (b), "toggled",
			    GTK_SIGNAL_FUNC (sp_fill_style_widget_type_toggled), GINT_TO_POINTER (FSW_GRADIENT));
	/* fill:pattern */
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/fill_pattern.xpm");
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (b), FALSE);
	/* fill:fractal */
	b = gtk_toggle_button_new ();
	gtk_widget_show (b);
	w = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/fill_fractal.xpm");
	gtk_widget_show (w);
	gtk_container_add (GTK_CONTAINER (b), w);
	gtk_box_pack_start (GTK_BOX (hb), b, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (GTK_WIDGET (b), FALSE);

	/* Horizontal separator */
	w = gtk_hseparator_new ();
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vb), w, FALSE, FALSE, 0);

	/* Frame */
	f = gtk_frame_new (_("Empty"));
	gtk_widget_show (f);
	gtk_container_set_border_width (GTK_CONTAINER (f), 4);
	gtk_box_pack_start (GTK_BOX (vb), f, TRUE, TRUE, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "type-frame", f);

	/* Add toplevel to container */
	gtk_container_add (GTK_CONTAINER (spw), vb);

	/* Set initial style */
	gtk_object_set_data (GTK_OBJECT (spw), "fill-style", GINT_TO_POINTER (FSW_EMPTY));

	/* Connect basic listeners */
	gtk_signal_connect (GTK_OBJECT (spw), "destroy", GTK_SIGNAL_FUNC (sp_fill_style_widget_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_fill_style_widget_modify_selection), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_fill_style_widget_change_selection), NULL);

	sp_fill_style_widget_reread (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

static void
sp_fill_style_widget_destroy (SPWidget *spw, gpointer data)
{
}

static void
sp_fill_style_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data)
{
	if (flags != SP_OBJECT_CHILD_MODIFIED_FLAG) {
		sp_fill_style_widget_reread (spw, selection);
	}
}

static void
sp_fill_style_widget_change_selection (SPWidget *spw, SPSelection *selection, gpointer data)
{
	sp_fill_style_widget_reread (spw, selection);
}

void
sp_fill_style_dialog (SPItem *item)
{
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	if (dialog == NULL) {
		GtkWidget *w;
		dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Fill style"));
		gtk_signal_connect (GTK_OBJECT (dialog), "delete_event", GTK_SIGNAL_FUNC (sp_fill_style_dialog_delete), NULL);
		w = sp_fill_style_widget_new ();
		/* Connect signals */
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	}

	if (!GTK_WIDGET_VISIBLE (dialog)) gtk_widget_show (dialog);

#if 0
	sp_item_dialog_setup (item);
#endif
}

void
sp_fill_style_dialog_close (GtkWidget *widget)
{
	g_assert (dialog != NULL);

	if (GTK_WIDGET_VISIBLE (dialog)) gtk_widget_hide (dialog);
}

static gint
sp_fill_style_dialog_delete (GtkWidget *widget, GdkEvent *event)
{
	sp_fill_style_dialog_close (widget);

	return TRUE;
}

/*
 * Change object style according to button toggled
 */

static void
sp_fill_style_widget_type_toggled (GtkToggleButton *b, FSWFillType type)
{
	static gboolean localblocked = FALSE;
	SPWidget *spw;
	FSWFillType oldtype;
	const GSList *reprs;

	if (blocked) return;
	if (localblocked) return;

	spw = gtk_object_get_data (GTK_OBJECT (b), "SPWidget");

	/* Return, if type is not changed */
	oldtype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (spw), "fill-type"));
	if (oldtype == type) return;

	if (!spw->desktop) return;
	if (!SP_DT_SELECTION (spw->desktop)) return;
	reprs = sp_selection_repr_list (SP_DT_SELECTION (spw->desktop));

	/* Switch the style of all selected items */
	if (reprs) {
		SPCSSAttr *css;
		const GSList *l;
		guchar c[64];

		localblocked = TRUE;

		css = sp_repr_css_attr_new ();

		switch (type) {
		case FSW_EMPTY:
			break;
		case FSW_NONE:
			sp_repr_css_set_property (css, "fill", "none");
			break;
		case FSW_SOLID:
			sp_svg_write_color (c, 64, sp_color_get_rgba32_ualpha (&lastprocesscolor, 0));
			sp_repr_css_set_property (css, "fill", c);
			g_snprintf (c, 64, "%g", lastalpha);
			sp_repr_css_set_property (css, "fill-opacity", c);
			break;
		case FSW_GRADIENT: {
			SPGradient *gr;
			const GSList *items;
			g_warning ("Gradient fill specified");
			gr = sp_document_default_gradient_vector (spw->document);
			items = sp_selection_item_list (SP_DT_SELECTION (spw->desktop));
			for (l = items; l != NULL; l = l->next) {
				sp_item_force_fill_lineargradient_vector (SP_ITEM (l->data), gr);
			}
			sp_document_done (spw->document);
			sp_repr_css_attr_unref (css);
			localblocked = FALSE;
			return;
			break;
		}
		default:
			g_assert_not_reached ();
			break;
		}

		for (l = reprs; l != NULL; l = l->next) {
			sp_repr_css_change_recursive (((SPRepr *) l->data), css, "style");
		}

		sp_document_done (spw->document);

		sp_repr_css_attr_unref (css);

		localblocked = FALSE;
	}
}

static void
sp_fill_style_widget_set_type (SPWidget *spw, FSWFillType type)
{
	GtkWidget *b;

	blocked = TRUE;

	b = gtk_object_get_data (GTK_OBJECT (spw), "fill-none");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), type == FSW_NONE);
	gtk_widget_set_sensitive (b, !(type == FSW_EMPTY));
	b = gtk_object_get_data (GTK_OBJECT (spw), "fill-solid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), type == FSW_SOLID);
	gtk_widget_set_sensitive (b, !(type == FSW_EMPTY));
	b = gtk_object_get_data (GTK_OBJECT (spw), "fill-gradient");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (b), type == FSW_GRADIENT);
	gtk_widget_set_sensitive (b, !(type == FSW_EMPTY));

	switch (type) {
	case FSW_EMPTY:
		sp_fill_style_set_empty (spw);
		break;
	case FSW_NONE:
		sp_fill_style_set_none (spw);
		break;
	case FSW_SOLID:
		sp_fill_style_set_solid (spw);
		break;
	case FSW_GRADIENT:
		sp_fill_style_set_gradient (spw);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	gtk_object_set_data (GTK_OBJECT (spw), "fill-type", GINT_TO_POINTER (type));

	blocked = FALSE;
}

/*
 * fill:none
 */

static void
sp_fill_style_set_empty (SPWidget *spw)
{
	FSWFillType oldtype;
	GtkWidget *frame;
	GList *children;

	oldtype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (spw), "fill-type"));
	if (oldtype != FSW_EMPTY) {
		frame = gtk_object_get_data (GTK_OBJECT (spw), "type-frame");

		children = gtk_container_children (GTK_CONTAINER (frame));
		while (children) {
			gtk_container_remove (GTK_CONTAINER (frame), children->data);
			children = g_list_remove (children, children->data);
		}
		gtk_object_remove_data (GTK_OBJECT (spw), "rgb-selector");

		gtk_frame_set_label (GTK_FRAME (frame), _("Empty"));
	}
}

/*
 * fill:none
 */

static void
sp_fill_style_set_none (SPWidget *spw)
{
	FSWFillType oldtype;
	GtkWidget *frame;
	GList *children;

	oldtype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (spw), "fill-type"));
	if (oldtype != FSW_NONE) {
		frame = gtk_object_get_data (GTK_OBJECT (spw), "type-frame");

		children = gtk_container_children (GTK_CONTAINER (frame));
		while (children) {
			gtk_container_remove (GTK_CONTAINER (frame), children->data);
			children = g_list_remove (children, children->data);
		}
		gtk_object_remove_data (GTK_OBJECT (spw), "rgb-selector");

		gtk_frame_set_label (GTK_FRAME (frame), _("None"));
	}
}

/*
 * fill:color
 */

static void
sp_fill_style_widget_solid_color_mode_activated (GtkWidget *widget, gpointer data)
{
	SPColorSelector *csel;
	SPColorSelectorMode mode, oldmode;

	csel = gtk_object_get_data (GTK_OBJECT (widget), "color-selector");
	mode = (SPColorSelectorMode) GPOINTER_TO_INT (data);
	oldmode = sp_color_selector_get_mode (csel);

	lastmode = mode;

	sp_color_selector_set_mode (csel, GPOINTER_TO_INT (data));
}

static void
sp_fill_style_widget_get_collective_color (SPWidget *spw, SPColor *color, gdouble *alpha)
{
	const GSList *objects, *l;
	gdouble r, g, b, a, c, m, y, k;
	gint items;
	SPColorSpaceType colorspace;

	if (!spw->desktop || !SP_DT_SELECTION (spw->desktop)) {
		/* Noting interesting, use last value */
		sp_color_copy (color, &lastprocesscolor);
		*alpha = lastalpha;
		return;
	}

	objects = sp_selection_item_list (SP_DT_SELECTION (spw->desktop));

	/* Try to determine colorspace */
	colorspace = SP_COLORSPACE_TYPE_NONE;
	for (l = objects; l != NULL; l = l->next) {
		SPObject *o;
		o = SP_OBJECT (l->data);
		if (o->style->fill.type == SP_PAINT_TYPE_COLOR) {
			SPColorSpaceType sct;
			sct = sp_color_get_colorspace_type (&o->style->fill.color);
			if (colorspace == SP_COLORSPACE_TYPE_NONE) {
				colorspace = sct;
			} else if (colorspace != sct) {
				colorspace = SP_COLORSPACE_TYPE_UNKNOWN;
			}
		}
	}

	switch (colorspace) {
	case SP_COLORSPACE_TYPE_NONE:
		/* Noting interesting, use last value */
		sp_color_copy (color, &lastprocesscolor);
		*alpha = lastalpha;
	case SP_COLORSPACE_TYPE_CMYK:
		/* RGB has precendence over CMYK */
		/* Find average */
		items = 0;
		c = m = y = k = a = 0.0;
		for (l = objects; l != NULL; l = l->next) {
			SPObject *o;
			o = SP_OBJECT (l->data);
			if (o->style->fill.type == SP_PAINT_TYPE_COLOR) {
				gfloat cmyk[4];
				sp_color_get_cmyk_floatv (&o->style->fill.color, cmyk);
				c += cmyk[0];
				m += cmyk[1];
				y += cmyk[2];
				k += cmyk[3];
				a += o->style->fill_opacity;
				items += 1;
			}
		}
		if (items > 0) {
			c = c / items;
			m = m / items;
			y = y / items;
			k = k / items;
		}
		sp_color_set_cmyk_float (color, c, m, y, k);
		*alpha = a;
		break;
	default:
		/* RGB has precendence over CMYK */
		/* Find average */
		items = 0;
		r = g = b = a = 0.0;
		for (l = objects; l != NULL; l = l->next) {
			SPObject *o;
			o = SP_OBJECT (l->data);
			if (o->style->fill.type == SP_PAINT_TYPE_COLOR) {
				gfloat rgb[4];
				sp_color_get_rgb_floatv (&o->style->fill.color, rgb);
				r += rgb[0];
				g += rgb[1];
				b += rgb[2];
				a += o->style->fill_opacity;
				items += 1;
			}
		}
		if (items > 0) {
			r = r / items;
			g = g / items;
			b = b / items;
			a = a / items;
			sp_color_set_rgb_float (color, r, g, b);
			*alpha = a;
		} else {
		/* Noting interesting, use last value */
			sp_color_copy (color, &lastprocesscolor);
			*alpha = lastalpha;
		}
		break;
	}
}

static void
sp_fill_style_set_solid (SPWidget *spw)
{
	FSWFillType oldtype;
	GtkWidget *csel;
	SPColor color;
	gdouble alpha;

	oldtype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (spw), "fill-type"));
	if (oldtype != FSW_SOLID) {
		GtkWidget *frame, *vb, *hb, *m, *i, *w;
		GList *children;
		/* Create solid fill widget */
		frame = gtk_object_get_data (GTK_OBJECT (spw), "type-frame");
		/* Clear frame contents */
		children = gtk_container_children (GTK_CONTAINER (frame));
		while (children) {
			gtk_container_remove (GTK_CONTAINER (frame), children->data);
			children = g_list_remove (children, children->data);
		}
		/* Set frame label */
		gtk_frame_set_label (GTK_FRAME (frame), _("Solid color"));
		/* Create color selector for later reference */
		csel = sp_color_selector_new ();
		/* Create vbox */
		vb = gtk_vbox_new (FALSE, 4);
		gtk_widget_show (vb);
		/* Create hbox */
		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 4);
		/* Label */
		w = gtk_label_new (_("Mode:"));
		gtk_misc_set_alignment (GTK_MISC (w), 1.0, 0.5);
		gtk_widget_show (w);
		gtk_box_pack_start (GTK_BOX (hb), w, TRUE, TRUE, 4);
		/* Create menu */
		m = gtk_menu_new ();
		gtk_widget_show (m);
		i = gtk_menu_item_new_with_label (_("RGB"));
		gtk_object_set_data (GTK_OBJECT (i), "color-selector", csel);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_fill_style_widget_solid_color_mode_activated),
				    GINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_RGB));
		gtk_widget_show (i);
		gtk_menu_append (GTK_MENU (m), i);
		i = gtk_menu_item_new_with_label (_("HSV"));
		gtk_object_set_data (GTK_OBJECT (i), "color-selector", csel);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_fill_style_widget_solid_color_mode_activated),
				    GINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_HSV));
		gtk_widget_show (i);
		gtk_menu_append (GTK_MENU (m), i);
		i = gtk_menu_item_new_with_label (_("CMYK"));
		gtk_object_set_data (GTK_OBJECT (i), "color-selector", csel);
		gtk_signal_connect (GTK_OBJECT (i), "activate",
				    GTK_SIGNAL_FUNC (sp_fill_style_widget_solid_color_mode_activated),
				    GINT_TO_POINTER (SP_COLOR_SELECTOR_MODE_CMYK));
		gtk_widget_show (i);
		gtk_menu_append (GTK_MENU (m), i);
		/* Create option menu */
		w = gtk_option_menu_new ();
		gtk_widget_show (w);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (w), m);
		gtk_object_set_data (GTK_OBJECT (csel), "mode-menu", w);
		gtk_box_pack_start (GTK_BOX (hb), w, FALSE, FALSE, 0);
		/* Color selector */
		gtk_widget_show (csel);
		gtk_box_pack_start (GTK_BOX (vb), csel, FALSE, FALSE, 0);
		gtk_object_set_data (GTK_OBJECT (spw), "rgb-selector", csel);
		gtk_signal_connect (GTK_OBJECT (csel), "changed", GTK_SIGNAL_FUNC (sp_fill_style_widget_rgba_changed), spw);
		gtk_signal_connect (GTK_OBJECT (csel), "dragged", GTK_SIGNAL_FUNC (sp_fill_style_widget_rgba_dragged), spw);
		/* Pack everything to frame */
		gtk_container_add (GTK_CONTAINER (frame), vb);
	} else {
		csel = gtk_object_get_data (GTK_OBJECT (spw), "rgb-selector");
		g_assert (csel != NULL);
		g_assert (SP_IS_COLOR_SELECTOR (csel));
	}

	sp_fill_style_widget_get_collective_color (spw, &color, &alpha);

	if (sp_color_get_colorspace_type (&color) == SP_COLORSPACE_TYPE_CMYK) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (gtk_object_get_data (GTK_OBJECT (csel), "mode-menu")), 2);
		sp_color_selector_set_mode (SP_COLOR_SELECTOR (csel), SP_COLOR_SELECTOR_MODE_CMYK);
		sp_color_selector_set_any_cmyka_float (SP_COLOR_SELECTOR (csel), color.v.c[0], color.v.c[1], color.v.c[2], color.v.c[3], alpha);
	} else {
		/* fixme: preserve RGB/HSV mode */
		if (lastmode == SP_COLOR_SELECTOR_MODE_HSV) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (gtk_object_get_data (GTK_OBJECT (csel), "mode-menu")), 1);
			sp_color_selector_set_mode (SP_COLOR_SELECTOR (csel), SP_COLOR_SELECTOR_MODE_HSV);
		} else {
		gtk_option_menu_set_history (GTK_OPTION_MENU (gtk_object_get_data (GTK_OBJECT (csel), "mode-menu")), 0);
			sp_color_selector_set_mode (SP_COLOR_SELECTOR (csel), SP_COLOR_SELECTOR_MODE_RGB);
		}
		sp_color_selector_set_any_rgba_float (SP_COLOR_SELECTOR (csel), color.v.c[0], color.v.c[1], color.v.c[2], alpha);
	}
	
	/* Save colors used */
	sp_color_copy (&lastprocesscolor, &color);
	lastalpha = alpha;
}

static void
sp_fill_style_set_gradient (SPWidget *spw)
{
	FSWFillType oldtype;

	oldtype = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (spw), "fill-type"));
	if (oldtype != FSW_GRADIENT) {
		GtkWidget *frame, *w;
		GList *children;
		/* Create solid fill widget */
		frame = gtk_object_get_data (GTK_OBJECT (spw), "type-frame");
		/* Clear frame contents */
		children = gtk_container_children (GTK_CONTAINER (frame));
		while (children) {
			gtk_container_remove (GTK_CONTAINER (frame), children->data);
			children = g_list_remove (children, children->data);
		}
		/* Set frame label */
		gtk_frame_set_label (GTK_FRAME (frame), _("Gradient"));
		/* Create gradient widget */
		w = sp_gradient_widget_new ();
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (frame), w);
	}

	/* fixme: Everything */
}

static void
sp_fill_style_widget_reread (SPWidget *spw, SPSelection *selection)
{
	FSWFillType type;
	SPObject *object;
	const GSList *objects, *l;

	if (!selection || sp_selection_is_empty (selection)) {
		sp_fill_style_widget_set_type (spw, FSW_EMPTY);
		return;
	}

	objects = sp_selection_item_list (selection);
	type = FSW_NONE;

	for (l = objects; l != NULL; l = l->next) {
		object = SP_OBJECT (l->data);
		switch (object->style->fill.type) {
		case SP_PAINT_TYPE_NONE:
			if (type < FSW_NONE) type = FSW_NONE;
			break;
		case SP_PAINT_TYPE_COLOR:
			if (type < FSW_SOLID) type = FSW_SOLID;
			break;
		case SP_PAINT_TYPE_PAINTSERVER:
			if (SP_IS_GRADIENT (object->style->fill.server)) {
				if (type < FSW_GRADIENT) type = FSW_GRADIENT;
			}
			break;
		default:
			break;
		}
	}

	sp_fill_style_widget_set_type (spw, type);
}

static void
sp_fill_style_widget_rgba_changed (SPColorSelector *csel, SPWidget *spw)
{
	const GSList *reprs;

	if (blocked) return;

	if (!spw->desktop) return;
	if (!SP_DT_SELECTION (spw->desktop)) return;
	reprs = sp_selection_repr_list (SP_DT_SELECTION (spw->desktop));

	if (reprs) {
		SPCSSAttr *css;
		const GSList *l;
		guchar c[64];
		gboolean fill_cmyk;
		guchar *cmykstr;

		css = sp_repr_css_attr_new ();

		fill_cmyk = (sp_color_selector_get_mode (csel) == SP_COLOR_SELECTOR_MODE_CMYK);
		sp_svg_write_color (c, 64, sp_color_selector_get_rgba32 (csel));
		sp_repr_css_set_property (css, "fill", c);
		g_snprintf (c, 64, "%g", sp_color_selector_get_a (csel));
		sp_repr_css_set_property (css, "fill-opacity", c);
		if (fill_cmyk) {
			gfloat cmyk[5];
			sp_color_selector_get_cmyka_floatv (csel, cmyk);
			cmykstr = g_strdup_printf ("(%g %g %g %g)", cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
		} else {
			cmykstr = NULL;
		}

		for (l = reprs; l != NULL; l = l->next) {
			sp_repr_css_change_recursive (((SPRepr *) l->data), css, "style");
			sp_repr_set_attr_recursive (((SPRepr *) l->data), "fill-cmyk", cmykstr);
		}

		if (cmykstr) g_free (cmykstr);

		sp_document_done (spw->document);

		sp_repr_css_attr_unref (css);
	}
}

#endif
