#define __SP_BUTTON_C__

/*
 * Generic button widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <string.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock.h>
#include <libnr/nr-pixblock-pattern.h>
#include <libnr/nr-pixops.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkmenuitem.h>

#include "helper/sp-marshal.h"
#include "forward.h"
#include "sodipodi-private.h"
#include "document.h"
#include "sp-item.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"

#include "icon.h"
#include "button.h"

enum {PRESSED, RELEASED, CLICKED, TOGGLED, LAST_SIGNAL};

static void sp_button_class_init (SPButtonClass *klass);
static void sp_button_init (SPButton *button);
static void sp_button_destroy (GtkObject *object);

static void sp_button_realize (GtkWidget *widget);
static void sp_button_unrealize (GtkWidget *widget);
static void sp_button_map (GtkWidget *widget);
static void sp_button_unmap (GtkWidget *widget);
static void sp_button_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_button_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static int sp_button_expose (GtkWidget *widget, GdkEventExpose *event);
static int sp_button_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
static int sp_button_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static int sp_button_button_release (GtkWidget *widget, GdkEventButton *event);
static int sp_button_button_press (GtkWidget *widget, GdkEventButton *event);

static void sp_button_paint (SPButton *button, GdkRectangle *area);

static GtkWidgetClass *parent_class;
static guint button_signals[LAST_SIGNAL];

GtkType
sp_button_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPButton",
			sizeof (SPButton),
			sizeof (SPButtonClass),
			(GtkClassInitFunc) sp_button_class_init,
			(GtkObjectInitFunc) sp_button_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_button_class_init (SPButtonClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = sp_button_destroy;

	widget_class->realize = sp_button_realize;
	widget_class->unrealize = sp_button_unrealize;
	widget_class->map = sp_button_map;
	widget_class->unmap = sp_button_unmap;
	widget_class->size_request = sp_button_size_request;
	widget_class->size_allocate = sp_button_size_allocate;
	widget_class->expose_event = sp_button_expose;
	widget_class->button_press_event = sp_button_button_press;
	widget_class->button_release_event = sp_button_button_release;
	widget_class->enter_notify_event = sp_button_enter_notify;
	widget_class->leave_notify_event = sp_button_leave_notify;

	button_signals[PRESSED] = g_signal_new ("pressed",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SPButtonClass, pressed),
						NULL, NULL,
						sp_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
	button_signals[RELEASED] = g_signal_new ("released",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SPButtonClass, released),
						NULL, NULL,
						sp_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
	button_signals[CLICKED] = g_signal_new ("clicked",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SPButtonClass, clicked),
						NULL, NULL,
						sp_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
	button_signals[TOGGLED] = g_signal_new ("toggled",
						G_OBJECT_CLASS_TYPE (object_class),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (SPButtonClass, toggled),
						NULL, NULL,
						sp_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
}

static void
sp_button_init (SPButton *button)
{
	GTK_WIDGET_SET_FLAGS (button, GTK_NO_WINDOW);
}

static void
sp_button_destroy (GtkObject *object)
{
	SPButton *button;

	button = SP_BUTTON (object);

	if (button->timeout) {
		gtk_timeout_remove (button->timeout);
		button->timeout = 0;
	}

	if (button->menu) {
		gtk_widget_destroy (button->menu);
		button->menu = NULL;
	}

	if (button->options) {
		int i;
		for (i = 0; i < button->noptions; i++) {
			nr_free (button->options[i].px);
			g_free (button->options[i].tip);
		}
		g_free (button->options);
		button->options = NULL;
	}

	if (button->tooltips) {
		g_object_unref (G_OBJECT (button->tooltips));
		button->tooltips = NULL;
	}

	((GtkObjectClass *) (parent_class))->destroy (object);
}

static void
sp_button_realize (GtkWidget *widget)
{
	SPButton *button;
	GdkWindowAttr attributes;
	gint attributes_mask;

	button = SP_BUTTON (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
				  GDK_BUTTON_RELEASE_MASK |
				  GDK_ENTER_NOTIFY_MASK |
				  GDK_LEAVE_NOTIFY_MASK);
	attributes_mask = GDK_WA_X | GDK_WA_Y;

	widget->window = gtk_widget_get_parent_window (widget);
	g_object_ref (widget->window);

	button->event_window = gdk_window_new (widget->window,
					       &attributes,
					       attributes_mask);
	gdk_window_set_user_data (button->event_window, button);

	widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
sp_button_unrealize (GtkWidget *widget)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	if (button->event_window) {
		gdk_window_set_user_data (button->event_window, NULL);
		gdk_window_destroy (button->event_window);
		button->event_window = NULL;
	}

	GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
sp_button_map (GtkWidget *widget)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	GTK_WIDGET_CLASS (parent_class)->map (widget);

	if (button->event_window) {
		gdk_window_show (button->event_window);
	}
}

static void
sp_button_unmap (GtkWidget *widget)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	if (button->event_window) {
		gdk_window_hide (button->event_window);
	}

	GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
sp_button_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	requisition->width = button->size + 2 * widget->style->xthickness;
	requisition->height = button->size + 2 * widget->style->ythickness;
}

static void
sp_button_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (button->event_window,
					widget->allocation.x,
					widget->allocation.y,
					widget->allocation.width,
					widget->allocation.height);
	}

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gtk_widget_queue_draw (widget);
	}
}

static int
sp_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_button_paint (SP_BUTTON (widget), &event->area);
	}

	return TRUE;
}

static void
sp_button_update_state (SPButton *button)
{
	GtkStateType state;

	if (button->pressed) {
		state = GTK_STATE_ACTIVE;
	} else if (button->inside) {
		state = GTK_STATE_PRELIGHT;
	} else {
		state = GTK_STATE_NORMAL;
	}

	if (state != GTK_WIDGET (button)->state) {
		gtk_widget_set_state (GTK_WIDGET (button), state);
	}
}

static void
sp_button_menu_activate (GObject *object, SPButton *button)
{
	button->option = GPOINTER_TO_INT (g_object_get_data (object, "option"));
	if (button->tooltips) {
		gtk_tooltips_set_tip (button->tooltips, GTK_WIDGET (button), button->options[button->option].tip, NULL);
	}
	gtk_widget_queue_draw (GTK_WIDGET (button));
}

static void
sp_button_menu_selection_done (GObject *object, SPButton *button)
{
	/* Emulate button released */
	switch (button->type) {
	case SP_BUTTON_TYPE_NORMAL:
		button->pressed = 0;
		button->down = 0;
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		sp_button_update_state (button);
		g_signal_emit (button, button_signals[RELEASED], 0);
		g_signal_emit (button, button_signals[CLICKED], 0);
		break;
	case SP_BUTTON_TYPE_TOGGLE:
		button->pressed = 0;
		button->down = !button->initial;
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		sp_button_update_state (button);
		g_signal_emit (button, button_signals[RELEASED], 0);
		g_signal_emit (button, button_signals[TOGGLED], 0);
		break;
	default:
		break;
	}
}

static int
sp_button_timeout (gpointer data)
{
	SPButton *button;
	int i;

	button = SP_BUTTON (data);

	button->timeout = 0;

	if (button->menu) {
		gtk_widget_destroy (button->menu);
		button->menu = NULL;
	}
	button->menu = gtk_menu_new ();
	gtk_widget_show (button->menu);
	for (i = 0; i < button->noptions; i++) {
		GtkWidget *icon, *mi;
		icon = sp_icon_new_from_data (button->size, button->options[i].px);
		gtk_widget_show (icon);
		mi = gtk_menu_item_new ();
		gtk_widget_show (mi);
		gtk_container_add (GTK_CONTAINER (mi), icon);
		gtk_menu_append (GTK_MENU (button->menu), mi);
		g_object_set_data (G_OBJECT (mi), "option", GINT_TO_POINTER (i));
		if (button->tooltips) {
			gtk_tooltips_set_tip (button->tooltips, mi, button->options[i].tip, NULL);
		}
		g_signal_connect (G_OBJECT (mi), "activate", G_CALLBACK (sp_button_menu_activate), button);
	}
	g_signal_connect (G_OBJECT (button->menu), "selection_done", G_CALLBACK (sp_button_menu_selection_done), button);

	gtk_menu_popup (GTK_MENU (button->menu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);

	return FALSE;
}

static int
sp_button_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	if (event->button == 1) {
		if (button->timeout) {
			gtk_timeout_remove (button->timeout);
			button->timeout = 0;
		}
		if (button->noptions > 1) {
			button->timeout = gtk_timeout_add (200, sp_button_timeout, button);
		}
		switch (button->type) {
		case SP_BUTTON_TYPE_NORMAL:
			gdk_pointer_grab (button->event_window, FALSE, GDK_BUTTON_RELEASE_MASK, NULL, NULL, GDK_CURRENT_TIME); 
			button->pressed = 1;
			button->down = 1;
			sp_button_update_state (button);
			g_signal_emit (button, button_signals[PRESSED], 0);
			break;
		case SP_BUTTON_TYPE_TOGGLE:
			gdk_pointer_grab (button->event_window, FALSE, GDK_BUTTON_RELEASE_MASK, NULL, NULL, GDK_CURRENT_TIME);
			button->pressed = 1;
			button->initial = button->down;
			button->down = 1;
			sp_button_update_state (button);
			g_signal_emit (button, button_signals[PRESSED], 0);
			break;
		default:
			break;
		}
	}

	return TRUE;
}

static int
sp_button_button_release (GtkWidget *widget, GdkEventButton *event)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	if (event->button == 1) {
		if (button->timeout) {
			gtk_timeout_remove (button->timeout);
			button->timeout = 0;
		}
		switch (button->type) {
		case SP_BUTTON_TYPE_NORMAL:
			button->pressed = 0;
			button->down = 0;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
			sp_button_update_state (button);
			g_signal_emit (button, button_signals[RELEASED], 0);
			g_signal_emit (button, button_signals[CLICKED], 0);
			break;
		case SP_BUTTON_TYPE_TOGGLE:
			button->pressed = 0;
			button->down = !button->initial;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
			sp_button_update_state (button);
			g_signal_emit (button, button_signals[RELEASED], 0);
			g_signal_emit (button, button_signals[TOGGLED], 0);
			break;
		default:
			break;
		}
	}

	return TRUE;
}

static int
sp_button_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	button->inside = 1;
	sp_button_update_state (button);

	return FALSE;
}

static int
sp_button_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
	SPButton *button;

	button = SP_BUTTON (widget);

	button->inside = 0;
	sp_button_update_state (button);

	return FALSE;
}

GtkWidget *
sp_button_new (unsigned int size, const unsigned char *name, const unsigned char *tip)
{
	SPButton *button;

	button = g_object_new (SP_TYPE_BUTTON, NULL);

	button->noptions = 1;
	button->size = CLAMP (size, 1, 128);
	button->options = g_new (SPBImageData, 1);
	button->options[0].px = sp_icon_image_load (name, button->size);
	button->options[0].tip = g_strdup (tip);

	return (GtkWidget *) button;
}

GtkWidget *
sp_button_toggle_new (unsigned int size, const unsigned char *name, const unsigned char *tip)
{
	SPButton *button;

	button = g_object_new (SP_TYPE_BUTTON, NULL);

	button->noptions = 1;
	button->type = SP_BUTTON_TYPE_TOGGLE;
	button->size = CLAMP (size, 1, 128);
	button->options = g_new (SPBImageData, 1);
	button->options[0].px = sp_icon_image_load (name, button->size);
	button->options[0].tip = g_strdup (tip);

	return (GtkWidget *) button;
}

GtkWidget *
sp_button_menu_new (unsigned int size, unsigned int noptions)
{
	SPButton *button;

	button = g_object_new (SP_TYPE_BUTTON, NULL);

	button->noptions = CLAMP (noptions, 1, 16);
	button->size = CLAMP (size, 1, 128);
	button->options = g_new0 (SPBImageData, button->noptions);

	return (GtkWidget *) button;
}

GtkWidget *
sp_button_toggle_menu_new (unsigned int size, unsigned int noptions)
{
	SPButton *button;

	button = g_object_new (SP_TYPE_BUTTON, NULL);

	button->noptions = CLAMP (noptions, 1, 16);
	button->type = SP_BUTTON_TYPE_TOGGLE;
	button->size = CLAMP (size, 1, 128);
	button->options = g_new0 (SPBImageData, button->noptions);

	return (GtkWidget *) button;
}

void
sp_button_set_tooltips (SPButton *button, GtkTooltips *tooltips)
{
	g_object_ref (G_OBJECT (tooltips));
	button->tooltips = tooltips;
	gtk_tooltips_set_tip (button->tooltips, GTK_WIDGET (button), button->options[button->option].tip, NULL);
}

void
sp_button_toggle_set_down (SPButton *button, unsigned int down, unsigned int signal)
{
	down = (down != 0);

	if (button->down != down) {
		button->down = down;
		g_signal_emit (button, button_signals[TOGGLED], 0);
		gtk_widget_queue_draw (GTK_WIDGET (button));
	}
}

void
sp_button_add_option (SPButton *button, unsigned int option, const unsigned char *name, const unsigned char *tip)
{
	button->options[option].px = sp_icon_image_load (name, button->size);
	button->options[option].tip = g_strdup (tip);

	if ((option == button->option) && button->tooltips) {
		gtk_tooltips_set_tip (button->tooltips, GTK_WIDGET (button), tip, NULL);
	}
}

unsigned int
sp_button_get_option (SPButton *button)
{
	return button->option;
}

void
sp_button_set_option (SPButton *button, unsigned int option)
{
	button->option = option;
	gtk_widget_queue_draw (GTK_WIDGET (button));
}

static void
sp_button_paint (SPButton *button, GdkRectangle *area)
{
	GtkWidget *widget;
	NRRectL iarea;
	int padx, pady;
	int x0, y0, x1, y1, x, y;

	widget = GTK_WIDGET (button);

	iarea.x0 = widget->allocation.x + widget->style->xthickness;
	iarea.y0 = widget->allocation.y + widget->style->ythickness;
	iarea.x1 = widget->allocation.x + widget->allocation.width - widget->style->xthickness;
	iarea.y1 = widget->allocation.y + widget->allocation.height - widget->style->ythickness;

	padx = (iarea.x1 - iarea.x0 - button->size) / 2;
	pady = (iarea.y1 - iarea.y0 - button->size) / 2;

	x0 = MAX (area->x, iarea.x0 + padx);
	y0 = MAX (area->y, iarea.y0 + pady);
	x1 = MIN (area->x + area->width, iarea.x0 + padx + button->size);
	y1 = MIN (area->y + area->height, iarea.y0 + pady + button->size);

	gtk_paint_box (widget->style, widget->window,
		       widget->state,
		       (button->down) ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
		       area, widget, "button",
		       widget->allocation.x,
		       widget->allocation.y,
		       widget->allocation.width,
		       widget->allocation.height);

	for (y = y0; y < y1; y += 64) {
		for (x = x0; x < x1; x += 64) {
			NRPixBlock bpb;
			unsigned char *px;
			int xe, ye;
			xe = MIN (x + 64, x1);
			ye = MIN (y + 64, y1);
			nr_pixblock_setup_fast (&bpb, NR_PIXBLOCK_MODE_R8G8B8, x, y, xe, ye, FALSE);

			px = button->options[button->option].px;
			if (px) {
				GdkColor *color;
				unsigned int br, bg, bb;
				int xx, yy;

				/* fixme: We support only plain-color themes */
				/* fixme: But who needs other ones anyways? (Lauris) */
				color = &widget->style->bg[widget->state];
				br = (color->red & 0xff00) >> 8;
				bg = (color->green & 0xff00) >> 8;
				bb = (color->blue & 0xff00) >> 8;

				if (GTK_WIDGET_SENSITIVE (button) && GTK_WIDGET_PARENT_SENSITIVE (button)) {
					for (yy = y; yy < ye; yy++) {
						const unsigned char *s;
						unsigned char *d;
						d = NR_PIXBLOCK_PX (&bpb) + (yy - y) * bpb.rs;
						s = px + 4 * (yy - pady - iarea.y0) * button->size + 4 * (x - padx - iarea.x0);
						for (xx = x; xx < xe; xx++) {
							d[0] = NR_COMPOSEN11 (s[0], s[3], br);
							d[1] = NR_COMPOSEN11 (s[1], s[3], bg);
							d[2] = NR_COMPOSEN11 (s[2], s[3], bb);
							s += 4;
							d += 3;
						}
					}
				} else {
					for (yy = y; yy < ye; yy++) {
						const unsigned char *s;
						unsigned char *d;
						unsigned int r, g, b;
						d = NR_PIXBLOCK_PX (&bpb) + (yy - y) * bpb.rs;
						s = px + 4 * (yy - pady - iarea.y0) * button->size + 4 * (x - padx - iarea.x0);
						for (xx = x; xx < xe; xx++) {
							r = br + ((int) s[0] - (int) br) / 2;
							g = bg + ((int) s[1] - (int) bg) / 2;
							b = bb + ((int) s[2] - (int) bb) / 2;
							d[0] = NR_COMPOSEN11 (r, s[3], br);
							d[1] = NR_COMPOSEN11 (g, s[3], bg);
							d[2] = NR_COMPOSEN11 (b, s[3], bb);
							s += 4;
							d += 3;
						}
					}
				}
			} else {
				nr_pixblock_render_gray_noise (&bpb, NULL);
			}

			if (button->noptions > 0) {
				/* Render arrow */
			}

			gdk_draw_rgb_image (widget->window, widget->style->black_gc,
					    x, y,
					    xe - x, ye - y,
					    GDK_RGB_DITHER_MAX,
					    NR_PIXBLOCK_PX (&bpb), bpb.rs);

			nr_pixblock_release (&bpb);
		}
	}
}

