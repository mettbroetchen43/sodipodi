#define __SP_FONT_SELECTOR_C__

/*
 * Font selection widgets
 *
 * Authors:
 *   Chris Lahey <clahey@ximian.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "config.h"

#include <stdlib.h>

#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>

#include <libnrtype/nr-type-directory.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkdrawingarea.h>

#include "../helper/nr-plain-stuff-gdk.h"

#include "font-selector.h"

/* SPFontSelector */

struct _SPFontSelector
{
	GtkHBox hbox;
  
	GtkWidget *family;
	GtkWidget *style;
	GtkWidget *size;

	guchar *familyname;
	guchar *stylename;
	gfloat fontsize;
	NRFont *font;
};


struct _SPFontSelectorClass
{
	GtkHBoxClass parent_class;

	void (* font_set) (SPFontSelector *fsel, NRFont *font);
};

enum {FONT_SET, LAST_SIGNAL};

static void sp_font_selector_class_init (SPFontSelectorClass *klass);
static void sp_font_selector_init (SPFontSelector *fsel);
static void sp_font_selector_destroy (GtkObject *object);

static void sp_font_selector_family_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, SPFontSelector *fsel);
static void sp_font_selector_style_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, SPFontSelector *fsel);
static void sp_font_selector_size_changed (GtkEditable *editable, SPFontSelector *fsel);

static void sp_font_selector_emit_set (SPFontSelector *fsel);

static const guchar *sizes[] = {
	"8", "9", "10", "11", "12", "13", "14",
	"16", "18", "20", "22", "24", "26", "28",
	"32", "36", "40", "48", "56", "64", "72",
	NULL
};

static GtkHBoxClass *fs_parent_class = NULL;
static guint fs_signals[LAST_SIGNAL] = {0};

guint
sp_font_selector_get_type ()
{
	static guint type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPFontSelector",
			sizeof (SPFontSelector),
			sizeof (SPFontSelectorClass),
			(GtkClassInitFunc) sp_font_selector_class_init,
			(GtkObjectInitFunc) sp_font_selector_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_HBOX, &info);
	}
	return type;
}

static void
sp_font_selector_class_init (SPFontSelectorClass *klass)
{
	GtkObjectClass *object_class;
  
	object_class = (GtkObjectClass *) klass;
  
	fs_parent_class = gtk_type_class (GTK_TYPE_HBOX);

	fs_signals[FONT_SET] = gtk_signal_new ("font_set",
					       GTK_RUN_FIRST,
					       object_class->type,
					       GTK_SIGNAL_OFFSET (SPFontSelectorClass, font_set),
					       gtk_marshal_NONE__POINTER,
					       GTK_TYPE_NONE,
					       1, GTK_TYPE_POINTER);
	gtk_object_class_add_signals (object_class, fs_signals, LAST_SIGNAL);

	object_class->destroy = sp_font_selector_destroy;
}

static void
sp_font_selector_init (SPFontSelector *fsel)
{
	GtkWidget *f, *sw, *vb, *hb, *l;
	NRNameList families;
	GList *sl;
	int i;

	gtk_box_set_homogeneous (GTK_BOX (fsel), TRUE);
	gtk_box_set_spacing (GTK_BOX (fsel), 4);

	/* Family frame */
	f = gtk_frame_new (_("Font family"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (fsel), f, TRUE, TRUE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (sw);
	gtk_container_set_border_width (GTK_CONTAINER (sw), 4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (f), sw);

	fsel->family = gtk_clist_new (1);
	gtk_widget_show (fsel->family);
	gtk_clist_set_selection_mode (GTK_CLIST (fsel->family), GTK_SELECTION_SINGLE);
	gtk_clist_column_titles_hide (GTK_CLIST (fsel->family));
	gtk_signal_connect (GTK_OBJECT (fsel->family), "select_row", GTK_SIGNAL_FUNC (sp_font_selector_family_select_row), fsel);
	gtk_container_add (GTK_CONTAINER (sw), fsel->family);

	if (nr_type_directory_family_list_get (&families)) {
		gint i;
		gtk_clist_freeze (GTK_CLIST (fsel->family));
		for (i = 0; i < families.length; i++) {
			gtk_clist_append (GTK_CLIST (fsel->family), (gchar **) families.names + i);
		}
		gtk_clist_thaw (GTK_CLIST (fsel->family));
		nr_name_list_release (&families);
	}

	/* Style frame */
	f = gtk_frame_new (_("Style"));
	gtk_widget_show (f);
	gtk_box_pack_start (GTK_BOX (fsel), f, TRUE, TRUE, 0);

	vb = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vb);
	gtk_container_set_border_width (GTK_CONTAINER (vb), 4);
	gtk_container_add (GTK_CONTAINER (f), vb);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (sw);
	gtk_container_set_border_width (GTK_CONTAINER (sw), 4);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vb), sw, TRUE, TRUE, 0);

	fsel->style = gtk_clist_new (1);
	gtk_widget_show (fsel->style);
	gtk_clist_set_selection_mode (GTK_CLIST (fsel->style), GTK_SELECTION_SINGLE);
	gtk_clist_column_titles_hide (GTK_CLIST (fsel->style));
	gtk_signal_connect (GTK_OBJECT (fsel->style), "select_row", GTK_SIGNAL_FUNC (sp_font_selector_style_select_row), fsel);
	gtk_container_add (GTK_CONTAINER (sw), fsel->style);

	hb = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hb);
	gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

	fsel->size = gtk_combo_new ();
	gtk_widget_show (fsel->size);
	gtk_combo_set_value_in_list (GTK_COMBO (fsel->size), FALSE, FALSE);
	gtk_combo_set_use_arrows (GTK_COMBO (fsel->size), TRUE);
	gtk_combo_set_use_arrows_always (GTK_COMBO (fsel->size), TRUE);
	gtk_widget_set_usize (fsel->size, 64, -1);
	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (fsel->size)->entry), "changed", GTK_SIGNAL_FUNC (sp_font_selector_size_changed), fsel);
	gtk_box_pack_end (GTK_BOX (hb), fsel->size, FALSE, FALSE, 0);

	l = gtk_label_new (_("Font size:"));
	gtk_widget_show (l);
	gtk_box_pack_end (GTK_BOX (hb), l, FALSE, FALSE, 0);

	/* Setup strings */
	sl = NULL;
	for (i = 0; sizes[i] != NULL; i++) {
		sl = g_list_prepend (sl, (gpointer) sizes[i]);
	}
	sl = g_list_reverse (sl);
	gtk_combo_set_popdown_strings (GTK_COMBO (fsel->size), sl);
	g_list_free (sl);

	fsel->familyname = NULL;
	fsel->stylename = NULL;
	fsel->fontsize = 10.0;
	fsel->font = NULL;
}

static void
sp_font_selector_destroy (GtkObject *object)
{
	SPFontSelector *fsel;

	fsel = SP_FONT_SELECTOR (object);

	if (fsel->font) {
		fsel->font = nr_font_unref (fsel->font);
	}

	if (fsel->stylename) {
		g_free (fsel->stylename);
		fsel->stylename = NULL;
	}

	if (fsel->familyname) {
		g_free (fsel->familyname);
		fsel->familyname = NULL;
	}

  	if (GTK_OBJECT_CLASS (fs_parent_class)->destroy)
		GTK_OBJECT_CLASS (fs_parent_class)->destroy (object);
}

static void
sp_font_selector_family_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, SPFontSelector *fsel)
{
	gchar *family;
	NRNameList styles;

	if (fsel->familyname) {
		g_free (fsel->familyname);
		fsel->familyname = NULL;
	}

	gtk_clist_get_text (clist, row, column, &family);

	if (family && nr_type_directory_style_list_get (family, &styles)) {
		gint i;
		fsel->familyname = g_strdup (family);
		gtk_clist_freeze (GTK_CLIST (fsel->style));
		gtk_clist_clear (GTK_CLIST (fsel->style));
		for (i = 0; i < styles.length; i++) {
			gtk_clist_append (GTK_CLIST (fsel->style), (gchar **) styles.names + i);
		}
		gtk_clist_thaw (GTK_CLIST (fsel->style));
		nr_name_list_release (&styles);
		gtk_clist_select_row (GTK_CLIST (fsel->style), 0, 0);
	} else {
		gtk_clist_clear (GTK_CLIST (fsel->style));
	}
}

static void
sp_font_selector_style_select_row (GtkCList *clist, gint row, gint column, GdkEvent *event, SPFontSelector *fsel)
{
	gchar *style;

	if (fsel->stylename) {
		g_free (fsel->stylename);
		fsel->stylename = NULL;
	}

	gtk_clist_get_text (clist, row, column, &style);

	if (style) {
		fsel->stylename = g_strdup (style);
	}

	sp_font_selector_emit_set (fsel);
}

static void
sp_font_selector_size_changed (GtkEditable *editable, SPFontSelector *fsel)
{
	gchar *sstr;

	sstr = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (fsel->size)->entry));
	fsel->fontsize = MAX (atof (sstr), 0.1);

	sp_font_selector_emit_set (fsel);
}

static void
sp_font_selector_emit_set (SPFontSelector *fsel)
{
	NRTypeFace *tf;
	NRFont *font;

	tf = nr_type_directory_lookup_fuzzy (fsel->familyname, fsel->stylename);
	font = nr_font_new_default (tf, NR_TYPEFACE_METRICS_DEFAULT, fsel->fontsize);
	nr_typeface_unref (tf);
	if (font != fsel->font) {
		if (fsel->font) fsel->font = nr_font_unref (fsel->font);
		if (font) fsel->font = nr_font_ref (font);
		gtk_signal_emit (GTK_OBJECT (fsel), fs_signals[FONT_SET], fsel->font);
	}
	if (font) nr_font_unref (font);
}

GtkWidget *
sp_font_selector_new (void)
{
	SPFontSelector *fsel;
  
	fsel = gtk_type_new (SP_TYPE_FONT_SELECTOR);
  
	gtk_clist_select_row (GTK_CLIST (fsel->family), 0, 0);

	return (GtkWidget *) fsel;
}

void
sp_font_selector_set_font (SPFontSelector *fsel, NRFont *font)
{
}

void
sp_font_selector_set_font_fuzzy (SPFontSelector *fsel, const guchar *family, const guchar *style)
{
}

NRFont*
sp_font_selector_get_font (SPFontSelector *fsel)
{
	if (fsel->font) nr_font_ref (fsel->font);

	return fsel->font;
}

/* SPFontPreview */

struct _SPFontPreview
{
	GtkDrawingArea darea;
};


struct _SPFontPreviewClass
{
	GtkDrawingAreaClass parent_class;
};

static void sp_font_preview_class_init (SPFontPreviewClass *klass);
static void sp_font_preview_init (SPFontPreview *fsel);
static void sp_font_preview_destroy (GtkObject *object);

static gint sp_font_preview_expose (GtkWidget *widget, GdkEventExpose *event);

static GtkDrawingAreaClass *fp_parent_class = NULL;

guint
sp_font_preview_get_type ()
{
	static guint type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPFontPreview",
			sizeof (SPFontPreview),
			sizeof (SPFontPreviewClass),
			(GtkClassInitFunc) sp_font_preview_class_init,
			(GtkObjectInitFunc) sp_font_preview_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_DRAWING_AREA, &info);
	}
	return type;
}

static void
sp_font_preview_class_init (SPFontPreviewClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
  
	fp_parent_class = gtk_type_class (GTK_TYPE_DRAWING_AREA);

	object_class->destroy = sp_font_preview_destroy;

	widget_class->expose_event = sp_font_preview_expose;
}

static void
sp_font_preview_init (SPFontPreview *fsel)
{
}

static void
sp_font_preview_destroy (GtkObject *object)
{
	SPFontPreview *fprev;

	fprev = SP_FONT_PREVIEW (object);

  	if (GTK_OBJECT_CLASS (fp_parent_class)->destroy)
		GTK_OBJECT_CLASS (fp_parent_class)->destroy (object);
}

static gint
sp_font_preview_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPFontPreview *fprev;

	fprev = SP_FONT_PREVIEW (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		nr_gdk_draw_gray_garbage (widget->window, widget->style->black_gc,
					  event->area.x, event->area.y,
					  event->area.width, event->area.height);
	}

	return TRUE;
}

GtkWidget *
sp_font_preview_new (void)
{
	GtkWidget *w;

	w = gtk_type_new (SP_TYPE_FONT_PREVIEW);

	return w;
}

void
sp_font_preview_set_font (SPFontPreview *fprev, NRFont *font)
{
}

void
sp_font_preview_set_rgba32 (SPFontPreview *fprev, guint32 rgba)
{
}

void
sp_font_preview_set_phrase (SPFontPreview *fprev, const guchar *phrase)
{
}

#if 0
/*****************************************************************************
 * These functions are the main public interface for getting/setting the font.
 *****************************************************************************/

gdouble
gnome_font_selection_get_size (GnomeFontSelection * fontsel)
{
	return fontsel->selectedsize;
}

void
gnome_font_selection_set_font (GnomeFontSelection * fontsel, GnomeFont * font)
{
	const GnomeFontFace * face;
	const gchar * familyname, * stylename;
	gdouble size;
	gchar b[32];
	gint rows, row;

	g_return_if_fail (fontsel != NULL);
	g_return_if_fail (GNOME_IS_FONT_SELECTION (fontsel));
	g_return_if_fail (font != NULL);
	g_return_if_fail (GNOME_IS_FONT (font));

	face = gnome_font_get_face (font);
	familyname = gnome_font_face_get_family_name (face);
	stylename = gnome_font_face_get_species_name (face);
	size = gnome_font_get_size (font);

	rows = ((GtkCList *) fontsel->family)->rows;
	for (row = 0; row < rows; row++) {
		gchar * text;
		gtk_clist_get_text ((GtkCList *) fontsel->family, row, 0, &text);
		if (strcmp (text, familyname) == 0) break;
	}
	gtk_clist_select_row ((GtkCList *) fontsel->family, row, 0);

	rows = ((GtkCList *) fontsel->style)->rows;
	for (row = 0; row < rows; row++) {
		gchar * text;
		gtk_clist_get_text ((GtkCList *) fontsel->style, row, 0, &text);
		if (strcmp (text, stylename) == 0) break;
	}
	gtk_clist_select_row ((GtkCList *) fontsel->style, row, 0);

	g_snprintf (b, 32, "%2.1f", size);
	b[31] = '\0';
	gtk_entry_set_text ((GtkEntry *) ((GtkCombo *) fontsel->size)->entry, b);
	fontsel->selectedsize = size;
}

/********************************************
 * GnomeFontPreview
 ********************************************/

struct _GnomeFontPreview
{
	GnomeCanvas canvas;

	GtkObject * phrase;

	gchar * text;
	GnomeFont * font;
	guint32 color;
};


struct _GnomeFontPreviewClass
{
	GnomeCanvasClass parent_class;
};

static void gnome_font_preview_class_init (GnomeFontPreviewClass *klass);
static void gnome_font_preview_init (GnomeFontPreview * preview);
static void gnome_font_preview_destroy (GtkObject *object);
static void gnome_font_preview_update (GnomeFontPreview * preview);

static GnomeCanvasClass *gfp_parent_class = NULL;

GtkType
gnome_font_preview_get_type()
{
	static GtkType font_preview_type = 0;
	if (!font_preview_type) {
		static const GtkTypeInfo gfp_type_info = {
			"GnomeFontPreview",
			sizeof (GnomeFontPreview),
			sizeof (GnomeFontPreviewClass),
			(GtkClassInitFunc) gnome_font_preview_class_init,
			(GtkObjectInitFunc) gnome_font_preview_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL,
		};
		font_preview_type = gtk_type_unique (GNOME_TYPE_CANVAS, &gfp_type_info);
	}
	return font_preview_type;
}

static void
gnome_font_preview_class_init (GnomeFontPreviewClass *klass)
{
	GtkObjectClass *object_class;
  
	object_class = (GtkObjectClass *) klass;
  
	gfp_parent_class = gtk_type_class (GNOME_TYPE_CANVAS);
  
	object_class->destroy = gnome_font_preview_destroy;
}

static void
gnome_font_preview_init (GnomeFontPreview * preview)
{
	GtkWidget * w;
	GtkStyle * style;
	GnomeCanvasItem * phrase;

	w = (GtkWidget *) preview;

	preview->text = NULL;
	preview->font = NULL;
	preview->color = 0x000000ff;

	style = gtk_style_copy (w->style);
	style->bg[GTK_STATE_NORMAL] = style->white;
	gtk_widget_set_style (w, style);

	gtk_widget_set_usize (w, 64, MIN_PREVIEW_HEIGHT);

	phrase = gnome_canvas_item_new (gnome_canvas_root ((GnomeCanvas *) preview),
					GNOME_TYPE_CANVAS_HACKTEXT,
					"fill_color_rgba", preview->color,
					NULL);

	preview->phrase = (GtkObject *) phrase;
}

static void
gnome_font_preview_destroy (GtkObject *object)
{
	GnomeFontPreview *preview;
  
	preview = (GnomeFontPreview *) object;

	if (preview->text) {
		g_free (preview->text);
		preview->text = NULL;
	}

	if (preview->font) {
		gnome_font_unref (preview->font);
		preview->font = NULL;
	}

	preview->phrase = NULL;

  	if (GTK_OBJECT_CLASS (gfp_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (gfp_parent_class)->destroy) (object);
}

GtkWidget *
gnome_font_preview_new (void)
{
	GnomeFontPreview * preview;
  
	preview = gtk_type_new (GNOME_TYPE_FONT_PREVIEW);

	return GTK_WIDGET (preview);
}

void
gnome_font_preview_set_phrase (GnomeFontPreview * preview, const gchar *phrase)
{
	g_return_if_fail (preview != NULL);
	g_return_if_fail (GNOME_IS_FONT_PREVIEW (preview));

	if (preview->text) g_free (preview->text);

	if (phrase) {
		preview->text = g_strdup (phrase);
	} else {
		preview->text = NULL;
	}

	gnome_font_preview_update (preview);
}


void
gnome_font_preview_set_font (GnomeFontPreview * preview, GnomeFont * font)
{
	g_return_if_fail (preview != NULL);
	g_return_if_fail (GNOME_IS_FONT_PREVIEW (preview));
	g_return_if_fail (font != NULL);
	g_return_if_fail (GNOME_IS_FONT (font));

	gnome_font_ref (font);

	if (preview->font) gnome_font_unref (preview->font);

	preview->font = font;

	gnome_font_preview_update (preview);
}

void
gnome_font_preview_set_color (GnomeFontPreview * preview, guint32 color)
{
	g_return_if_fail (preview != NULL);
	g_return_if_fail (GNOME_IS_FONT_PREVIEW (preview));

	preview->color = color;

	gnome_font_preview_update (preview);
}

void
gnome_font_preview_bind_editable_enters (GnomeFontPreview * gfp, GnomeDialog * dialog)
{
}

void
gnome_font_preview_bind_accel_group (GnomeFontPreview * gfp, GtkWindow * window)
{
}

static void
gnome_font_preview_update (GnomeFontPreview * preview)
{
	const gchar * sample;
	gdouble ascender, descender, width;

	if (!preview->font) return;

	if (preview->text) {
		sample = preview->text;
	} else {
		sample = gnome_font_face_get_sample (gnome_font_get_face (preview->font));
	}

	ascender = gnome_font_get_ascender (preview->font);
	descender = gnome_font_get_descender (preview->font);
	width = gnome_font_get_width_string (preview->font, sample);

	gnome_canvas_set_scroll_region (GNOME_CANVAS (preview),
					-16.0, -ascender, width + 16.0, descender);

	gnome_canvas_item_set ((GnomeCanvasItem *) preview->phrase,
			       "font", preview->font,
			       "text", sample,
			       "fill_color_rgba", preview->color,
			       NULL);
}

#endif
