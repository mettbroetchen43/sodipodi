#define __SP_COLOR_PREVIEW_C__

/*
 * A simple color preview widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "helper/sp-intl.h"
#include "helper/nr-plain-stuff-gdk.h"

#include "sp-color-preview.h"

#define SPCP_DEFAULT_WIDTH 32
#define SPCP_DEFAULT_HEIGHT 16

static void sp_color_preview_class_init (SPColorPreviewClass *klass);
static void sp_color_preview_init (SPColorPreview *image);
static void sp_color_preview_destroy (GtkObject *object);

static void sp_color_preview_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_color_preview_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint sp_color_preview_expose (GtkWidget *widget, GdkEventExpose *event);

static void sp_color_preview_paint (SPColorPreview *cp, GdkRectangle *area);

static GtkWidgetClass *preview_parent_class;

GtkType
sp_color_preview_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPColorPreview",
			sizeof (SPColorPreview),
			sizeof (SPColorPreviewClass),
			(GtkClassInitFunc) sp_color_preview_class_init,
			(GtkObjectInitFunc) sp_color_preview_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_color_preview_class_init (SPColorPreviewClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	preview_parent_class = gtk_type_class (GTK_TYPE_WIDGET);

	object_class->destroy = sp_color_preview_destroy;

	widget_class->size_request = sp_color_preview_size_request;
	widget_class->size_allocate = sp_color_preview_size_allocate;
	widget_class->expose_event = sp_color_preview_expose;
}

static void
sp_color_preview_init (SPColorPreview *image)
{
	GTK_WIDGET_SET_FLAGS (image, GTK_NO_WINDOW);

	image->rgba = 0xffffffff;
}

static void
sp_color_preview_destroy (GtkObject *object)
{
	SPColorPreview *image;

	image = SP_COLOR_PREVIEW (object);

	if (((GtkObjectClass *) (preview_parent_class))->destroy)
		(* ((GtkObjectClass *) (preview_parent_class))->destroy) (object);
}

static void
sp_color_preview_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPColorPreview *slider;

	slider = SP_COLOR_PREVIEW (widget);

	requisition->width = SPCP_DEFAULT_WIDTH;
	requisition->height = SPCP_DEFAULT_HEIGHT;
}

static void
sp_color_preview_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPColorPreview *image;

	image = SP_COLOR_PREVIEW (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_DRAWABLE (image)) {
		gtk_widget_queue_draw (GTK_WIDGET (image));
	}
}

static gint
sp_color_preview_expose (GtkWidget *widget, GdkEventExpose *event)
{
	SPColorPreview *cp;

	cp = SP_COLOR_PREVIEW (widget);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_color_preview_paint (cp, &event->area);
	}

	return TRUE;
}

GtkWidget *
sp_color_preview_new (guint32 rgba, gboolean transparent, gboolean opaque)
{
	SPColorPreview *preview;

	preview = gtk_type_new (SP_TYPE_COLOR_PREVIEW);

	preview->transparent = transparent || (!transparent && !opaque);
	preview->opaque = opaque || (!transparent && !opaque);

	sp_color_preview_set_rgba32 (preview, rgba);

	return (GtkWidget *) preview;
}

void
sp_color_preview_set_rgba32 (SPColorPreview *cp, guint32 rgba)
{
	cp->rgba = rgba;

	if (GTK_WIDGET_DRAWABLE (cp)) {
		gtk_widget_queue_draw (GTK_WIDGET (cp));
	}
}

static void
sp_color_preview_paint (SPColorPreview *cp, GdkRectangle *area)
{
	GtkWidget *widget;
	GdkRectangle warea, carea;
	GdkRectangle wpaint, cpaint;
	gint bx, by, w2;

	widget = GTK_WIDGET (cp);

	warea.x = widget->allocation.x;
	warea.y = widget->allocation.y;
	warea.width = widget->allocation.width;
	warea.height = widget->allocation.height;

	if (!gdk_rectangle_intersect (area, &warea, &wpaint)) return;

	/* Draw shadow */
	gtk_draw_shadow (widget->style, widget->window,
			 widget->state, GTK_SHADOW_ETCHED_IN,
			 warea.x, warea.y,
			 warea.width, warea.height);

	bx = widget->style->xthickness;
	by = widget->style->ythickness;
	w2 = warea.width / 2;

	/* Transparent area */
	if (cp->transparent) {
		carea.x = warea.x + bx;
		carea.y = warea.y + by;
		if (cp->opaque) {
			carea.width = w2 - bx;
		} else {
			carea.width = warea.width - 2 * bx;
		}
		carea.height = warea.height - 2 * by;
		if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
			nr_gdk_draw_rgba32_solid (widget->window, widget->style->black_gc,
						  cpaint.x, cpaint.y,
						  cpaint.width, cpaint.height,
						  cp->rgba);
		}
	}

	/* Solid area */
	if (cp->opaque) {
		if (cp->transparent) {
			carea.x = warea.x + w2;
			carea.width = warea.width - bx - w2;
		} else {
			carea.x = warea.x + bx;
			carea.width = warea.width - 2 * bx;
		}
		carea.y = warea.y + by;
		carea.height = warea.height - 2 * by;
		if (gdk_rectangle_intersect (area, &carea, &cpaint)) {
			nr_gdk_draw_rgba32_solid (widget->window, widget->style->black_gc,
						  cpaint.x, cpaint.y,
						  cpaint.width, cpaint.height,
						  cp->rgba | 0xff);
		}
	}
}

/* SPColorPicker */

#include <gtk/gtkwindow.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtksignal.h>

#include "sp-color-selector.h"

/* Like everything else, it has signals grabbed, dragged, released and changed */

struct _SPColorPicker {
	GtkButton button;
	SPColorPreview *preview;
	GtkWindow *window;
	SPColorSelector *csel;
	guint32 rgba;
	guchar *title;
};

struct _SPColorPickerClass {
	GtkButtonClass button_class;

	void (* grabbed) (SPColorSlider *slider);
	void (* dragged) (SPColorSlider *slider);
	void (* released) (SPColorSlider *slider);
	void (* changed) (SPColorSlider *slider);
};

enum {GRABBED, DRAGGED, /* RELEASED, */ CHANGED, LAST_SIGNAL};

static void sp_color_picker_class_init (SPColorPickerClass *klass);
static void sp_color_picker_init (SPColorPicker *picker);
static void sp_color_picker_destroy (GtkObject *object);

static void sp_color_picker_clicked (GtkButton *button);

static GtkWidgetClass *picker_parent_class;
static guint picker_signals[LAST_SIGNAL] = {0};

GType
sp_color_picker_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPColorPicker",
			sizeof (SPColorPicker),
			sizeof (SPColorPickerClass),
			(GtkClassInitFunc) sp_color_picker_class_init,
			(GtkObjectInitFunc) sp_color_picker_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_BUTTON, &info);
	}
	return type;
}

static void
sp_color_picker_class_init (SPColorPickerClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	button_class = (GtkButtonClass *) klass;

	picker_parent_class = g_type_class_peek_parent (klass);

	picker_signals[GRABBED] = gtk_signal_new ("grabbed",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorPickerClass, grabbed),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
	picker_signals[DRAGGED] = gtk_signal_new ("dragged",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorPickerClass, dragged),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
#if 0
	picker_signals[RELEASED] = gtk_signal_new ("released",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorPickerClass, released),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);
#endif
	picker_signals[CHANGED] = gtk_signal_new ("changed",
						  GTK_RUN_FIRST | GTK_RUN_NO_RECURSE,
						  GTK_CLASS_TYPE(object_class),
						  GTK_SIGNAL_OFFSET (SPColorPickerClass, changed),
						  gtk_marshal_NONE__NONE,
						  GTK_TYPE_NONE, 0);

	object_class->destroy = sp_color_picker_destroy;

	button_class->clicked = sp_color_picker_clicked;
}

static void
sp_color_picker_init (SPColorPicker *picker)
{
	picker->preview = (SPColorPreview *) sp_color_preview_new (0x00000000, TRUE, FALSE);
	gtk_container_add ((GtkContainer *) picker, (GtkWidget *) picker->preview);
}

static void
sp_color_picker_destroy (GtkObject *object)
{
	SPColorPicker *picker;
	picker = (SPColorPicker *) object;
	if (picker->window) {
		gtk_signal_disconnect_by_data ((GtkObject *) picker->window, object);
		gtk_widget_destroy ((GtkWidget *) picker->window);
		picker->window = NULL;
	}
	if (picker->title) {
		g_free (picker->title);
		picker->title = NULL;
	}
	if (((GtkObjectClass *) (picker_parent_class))->destroy)
		(* ((GtkObjectClass *) (picker_parent_class))->destroy) (object);
}

static void
sp_color_picker_window_destroy (GtkObject *object, SPColorPicker *picker)
{
	picker->window = NULL;
}

static void
sp_color_picker_csel_grabbed (SPColorSelector *csel, SPColorPicker *picker)
{
	gtk_signal_emit ((GtkObject *) picker, picker_signals[GRABBED]);
}

static void
sp_color_picker_csel_dragged (SPColorSelector *csel, SPColorPicker *picker)
{
	picker->rgba = sp_color_selector_get_rgba32 (csel);
	sp_color_preview_set_rgba32 (picker->preview, picker->rgba);
	gtk_signal_emit ((GtkObject *) picker, picker_signals[DRAGGED]);
}

static void
sp_color_picker_csel_released (SPColorSelector *csel, SPColorPicker *picker)
{
	/* gtk_signal_emit ((GtkObject *) picker, picker_signals[RELEASED]); */
	gtk_button_released ((GtkButton *) picker);
}

static void
sp_color_picker_csel_changed (SPColorSelector *csel, SPColorPicker *picker)
{
	picker->rgba = sp_color_selector_get_rgba32 (csel);
	sp_color_preview_set_rgba32 (picker->preview, picker->rgba);
	gtk_signal_emit ((GtkObject *) picker, picker_signals[CHANGED]);
}

static void
sp_color_picker_window_close (GtkObject *object, SPColorPicker *picker)
{
	gtk_object_destroy ((GtkObject *) picker->window);
}

static void
sp_color_picker_clicked (GtkButton *button)
{
	SPColorPicker *picker;
	picker = (SPColorPicker *) button;
	if (!picker->window) {
		GtkWidget *vb, *hs, *hb, *b;

		picker->window = (GtkWindow *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (picker->window, picker->title);
		gtk_container_set_border_width ((GtkContainer *) picker->window, 4);
		g_signal_connect ((GObject *) picker->window, "destroy",
				  (GCallback) sp_color_picker_window_destroy, picker);

		vb = gtk_vbox_new (FALSE, 4);
		gtk_container_add ((GtkContainer *) picker->window, vb);

		picker->csel = (SPColorSelector *) sp_color_selector_new ();
		gtk_box_pack_start ((GtkBox *) vb, (GtkWidget *) picker->csel, TRUE, TRUE, 0);
		sp_color_selector_set_rgba32 (picker->csel, picker->rgba);
		g_signal_connect ((GObject *) picker->csel, "grabbed",
				  (GCallback) sp_color_picker_csel_grabbed, picker);
		g_signal_connect ((GObject *) picker->csel, "dragged",
				  (GCallback) sp_color_picker_csel_dragged, picker);
		g_signal_connect ((GObject *) picker->csel, "released",
				  (GCallback) sp_color_picker_csel_released, picker);
		g_signal_connect ((GObject *) picker->csel, "changed",
				  (GCallback) sp_color_picker_csel_changed, picker);

		hs = gtk_hseparator_new ();
		gtk_box_pack_start ((GtkBox *) vb, hs, FALSE, FALSE, 0);

		hb = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start ((GtkBox *) vb, hb, FALSE, FALSE, 0);

		b = gtk_button_new_with_label (_("Close"));
		gtk_box_pack_end ((GtkBox *) hb, b, FALSE, FALSE, 0);
		g_signal_connect ((GObject *) b, "clicked",
				  (GCallback) sp_color_picker_window_close, picker);

		gtk_widget_show_all ((GtkWidget *) picker->window);
	} else {
		gtk_window_present (picker->window);
	}
}

GtkWidget *
sp_color_picker_new (guint32 rgba, const guchar *title)
{
	SPColorPicker *picker;
	picker = (SPColorPicker *) g_object_new (SP_TYPE_COLOR_PICKER, NULL);
	picker->rgba = rgba;
	picker->title = g_strdup (title);
	return (GtkWidget *) picker;
}

guint32
sp_color_picker_get_rgba32 (SPColorPicker *picker)
{
	return picker->rgba;
}

