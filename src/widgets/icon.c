#define __SP_ICON_C__

/*
 * Generic icon widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <string.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock.h>
#include <libnr/nr-pixblock-pattern.h>
#include <libnr/nr-pixops.h>

#include "forward.h"
#include "sodipodi-private.h"
#include "document.h"
#include "sp-item.h"
#include "display/nr-arena.h"
#include "display/nr-arena-item.h"

#include "icon.h"

static void sp_icon_class_init (SPIconClass *klass);
static void sp_icon_init (SPIcon *icon);
static void sp_icon_destroy (GtkObject *object);

static void sp_icon_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void sp_icon_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static int sp_icon_expose (GtkWidget *widget, GdkEventExpose *event);

static void sp_icon_paint (SPIcon *icon, GdkRectangle *area);

static unsigned char *sp_icon_image_load_pixmap (const unsigned char *name, unsigned int size);
static unsigned char *sp_icon_image_load_svg (const unsigned char *name, unsigned int size);

static GtkWidgetClass *parent_class;

GtkType
sp_icon_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPIcon",
			sizeof (SPIcon),
			sizeof (SPIconClass),
			(GtkClassInitFunc) sp_icon_class_init,
			(GtkObjectInitFunc) sp_icon_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_WIDGET, &info);
	}
	return type;
}

static void
sp_icon_class_init (SPIconClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	object_class->destroy = sp_icon_destroy;

	widget_class->size_request = sp_icon_size_request;
	widget_class->size_allocate = sp_icon_size_allocate;
	widget_class->expose_event = sp_icon_expose;
}

static void
sp_icon_init (SPIcon *icon)
{
	GTK_WIDGET_SET_FLAGS (icon, GTK_NO_WINDOW);
}

static void
sp_icon_destroy (GtkObject *object)
{
	SPIcon *icon;

	icon = SP_ICON (object);

	if (icon->px) {
		if (!(GTK_OBJECT_FLAGS (icon) & SP_ICON_FLAG_STATIC_DATA)) {
			nr_free (icon->px);
		}
		icon->px = NULL;
	}

	((GtkObjectClass *) (parent_class))->destroy (object);
}

static void
sp_icon_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	SPIcon *icon;

	icon = SP_ICON (widget);

	requisition->width = icon->size;
	requisition->height = icon->size;
}

static void
sp_icon_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	widget->allocation = *allocation;

	if (GTK_WIDGET_DRAWABLE (widget)) {
		gtk_widget_queue_draw (widget);
	}
}

static int
sp_icon_expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (GTK_WIDGET_DRAWABLE (widget)) {
		sp_icon_paint (SP_ICON (widget), &event->area);
	}

	return TRUE;
}

GtkWidget *
sp_icon_new (unsigned int size, const unsigned char *name)
{
	SPIcon *icon;

	icon = g_object_new (SP_TYPE_ICON, NULL);

	icon->size = CLAMP (size, 1, 128);

	GTK_OBJECT_UNSET_FLAGS (icon, SP_ICON_FLAG_STATIC_DATA);
	icon->px = sp_icon_image_load (name, icon->size);

	return (GtkWidget *) icon;
}

GtkWidget *
sp_icon_new_from_data (unsigned int size, const unsigned char *px)
{
	SPIcon *icon;

	icon = g_object_new (SP_TYPE_ICON, NULL);

	icon->size = CLAMP (size, 1, 128);

	GTK_OBJECT_SET_FLAGS (icon, SP_ICON_FLAG_STATIC_DATA);
	icon->px = (unsigned char *) px;

	return (GtkWidget *) icon;
}

unsigned char *
sp_icon_image_load (const unsigned char *name, unsigned int size)
{
	unsigned char *px;

	px = sp_icon_image_load_pixmap (name, size);
	if (!px) px = sp_icon_image_load_svg (name, size);

	return px;
}


static void
sp_icon_paint (SPIcon *icon, GdkRectangle *area)
{
	GtkWidget *widget;
	int padx, pady;
	int x0, y0, x1, y1, x, y;

	widget = GTK_WIDGET (icon);

	padx = (widget->allocation.width - icon->size) / 2;
	pady = (widget->allocation.height - icon->size) / 2;

	x0 = MAX (area->x, widget->allocation.x + padx);
	y0 = MAX (area->y, widget->allocation.y + pady);
	x1 = MIN (area->x + area->width, widget->allocation.x + padx + icon->size);
	y1 = MIN (area->y + area->height, widget->allocation.y + pady + icon->size);

	for (y = y0; y < y1; y += 64) {
		for (x = x0; x < x1; x += 64) {
			NRPixBlock bpb;
			int xe, ye;
			xe = MIN (x + 64, x1);
			ye = MIN (y + 64, y1);
			nr_pixblock_setup_fast (&bpb, NR_PIXBLOCK_MODE_R8G8B8, x, y, xe, ye, FALSE);

			if (icon->px) {
				GdkColor *color;
				unsigned int br, bg, bb;
				int xx, yy;

				/* fixme: We support only plain-color themes */
				/* fixme: But who needs other ones anyways? (Lauris) */
				color = &widget->style->bg[widget->state];
				br = (color->red & 0xff00) >> 8;
				bg = (color->green & 0xff00) >> 8;
				bb = (color->blue & 0xff00) >> 8;

				for (yy = y; yy < ye; yy++) {
					const unsigned char *s;
					unsigned char *d;
					d = NR_PIXBLOCK_PX (&bpb) + (yy - y) * bpb.rs;
					s = icon->px + 4 * (yy - pady - widget->allocation.y) * icon->size + 4 * (x - padx - widget->allocation.x);
					for (xx = x; xx < xe; xx++) {
						d[0] = NR_COMPOSEN11 (s[0], s[3], br);
						d[1] = NR_COMPOSEN11 (s[1], s[3], bg);
						d[2] = NR_COMPOSEN11 (s[2], s[3], bb);
						s += 4;
						d += 3;
					}
				}
			} else {
				nr_pixblock_render_gray_noise (&bpb, NULL);
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

static unsigned char *
sp_icon_image_load_pixmap (const unsigned char *name, unsigned int size)
{
	unsigned char *path;
	unsigned char *px;
	GdkPixbuf *pb;

	path = g_strdup_printf ("%s/%s.png", SODIPODI_PIXMAPDIR, name);
	pb = gdk_pixbuf_new_from_file (path, NULL);
	g_free (path);
	if (!pb) {
		path = g_strdup_printf ("%s/%s.xpm", SODIPODI_PIXMAPDIR, name);
		pb = gdk_pixbuf_new_from_file (path, NULL);
		g_free (path);
	}
	if (pb) {
		unsigned char *spx;
		int srs, y;
		if (!gdk_pixbuf_get_has_alpha (pb)) gdk_pixbuf_add_alpha (pb, FALSE, 0, 0, 0);
		if ((gdk_pixbuf_get_width (pb) != size) || (gdk_pixbuf_get_height (pb) != size)) {
			GdkPixbuf *spb;
			spb = gdk_pixbuf_scale_simple (pb, size, size, GDK_INTERP_HYPER);
			g_object_unref (G_OBJECT (pb));
			pb = spb;
		}
		spx = gdk_pixbuf_get_pixels (pb);
		srs = gdk_pixbuf_get_rowstride (pb);
		px = nr_new (unsigned char, 4 * size * size);
		for (y = 0; y < size; y++) {
			memcpy (px + 4 * y * size, spx + y * srs, 4 * size);
		}
		g_object_unref (G_OBJECT (pb));

		return px;
	}

	return NULL;
}

static unsigned char *
sp_icon_image_load_svg (const unsigned char *name, unsigned int size)
{
	static SPDocument *doc = NULL;
	static NRArena *arena = NULL;
	static NRArenaItem *root = NULL;
	static unsigned int edoc = FALSE;
	unsigned char *px;

	/* Try to load from document */
	if (!edoc && !doc) {
		doc = sp_document_new (SODIPODI_PIXMAPDIR "/icons.svg", FALSE, FALSE);
		if (!doc) doc = sp_document_new ("glade/icons.svg", FALSE, FALSE);
		if (doc) {
			NRMatrixF affine;
			unsigned int visionkey;

			sp_document_ensure_up_to_date (doc);

			/* Create new arena */
			arena = g_object_new (NR_TYPE_ARENA, NULL);
			/* Create ArenaItem and set transform */
			visionkey = sp_item_display_key_new ();
			root = sp_item_show (SP_ITEM (SP_DOCUMENT_ROOT (doc)), arena, visionkey);
			/* Set up matrix */
			nr_matrix_f_set_scale (&affine, 0.8, 0.8);
			nr_arena_item_set_transform (root, &affine);
		} else {
			edoc = TRUE;
		}
	}

	if (!edoc && doc) {
		SPObject *object;
		object = sp_document_lookup_id (doc, name);
		if (object && SP_IS_ITEM (object)) {
			NRRectF area;
			sp_item_bbox_desktop (SP_ITEM (object), &area);
			if (!nr_rect_f_test_empty (&area)) {
				NRRectF bbox;
				NRGC gc;
				NRPixBlock B;
				NRRectL ua;
				px = nr_new (unsigned char, 4 * size * size);
				memset (px, 0x00, 4 * size * size);
				/* Set up area of interest */
				bbox.x0 = area.x0 * 1.0;
				bbox.y0 = (sp_document_height (doc) - area.y1) * 1.0;
				bbox.x1 = area.x1 * 1.0;
				bbox.y1 = (sp_document_height (doc) - area.y0) * 1.0;
				/* Update to renderable state */
				nr_matrix_d_set_identity (&gc.transform);
				ua.x0 = (bbox.x0 + 0.0625);
				ua.y0 = (bbox.y0 + 0.0625);
				ua.x1 = ua.x0 + size;
				ua.y1 = ua.y0 + size;
				nr_arena_item_invoke_update (root, NULL, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);

				/* Render */
				nr_pixblock_setup_extern (&B, NR_PIXBLOCK_MODE_R8G8B8A8N, ua.x0, ua.y0, ua.x1, ua.y1, px, 4 * size, FALSE, FALSE);
				nr_arena_item_invoke_render (root, &ua, &B, NR_ARENA_ITEM_RENDER_NO_CACHE);
				nr_pixblock_release (&B);
				return px;
			}
		}
	}

	return NULL;
}

