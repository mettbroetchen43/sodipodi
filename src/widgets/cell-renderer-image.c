#define __SP_CELL_RENDERER_IMAGE_C__

/*
 * Multi image cellrenderer
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <string.h>
#include <malloc.h>

#include <libnr/nr-pixblock.h>
#include <libnr/nr-pixblock-pattern.h>
#include <libnr/nr-pixops.h>

#include "helper/sp-marshal.h"

#include "icon.h"

#include "cell-renderer-image.h"

#define DEFAULT_SIZE  16

enum { CLICKED, TOGGLED, LAST_SIGNAL };

enum { PROP_0, PROP_VALUE, PROP_ACTIVATABLE };


static void sp_cell_renderer_image_init (SPCellRendererImage *cri);
static void sp_cell_renderer_image_class_init (SPCellRendererImageClass *klass);
static void sp_cell_renderer_image_finalize (GObject *object);
static void sp_cell_renderer_image_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void sp_cell_renderer_image_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

static void sp_cell_renderer_image_get_size (GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *rectangle,
					     gint *x_offset, gint *y_offset, gint *width, gint *height);
static void sp_cell_renderer_image_render (GtkCellRenderer *cell, GdkWindow *window, GtkWidget *widget,
					   GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area,
					   GtkCellRendererState flags);
static gboolean sp_cell_renderer_image_activate (GtkCellRenderer *cell, GdkEvent *event, GtkWidget *widget, const gchar *path,
						 GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags);

static unsigned int signals[LAST_SIGNAL] = { 0 };

static GtkCellRendererClass *parent_class = NULL;

GType
sp_cell_renderer_image_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (SPCellRendererImageClass),
			NULL, NULL,
			(GClassInitFunc) sp_cell_renderer_image_class_init,
			NULL, NULL,
			sizeof (SPCellRendererImage),
			0,
			(GInstanceInitFunc) sp_cell_renderer_image_init,
		};
		type = g_type_register_static (GTK_TYPE_CELL_RENDERER, "SPCellRendererImage", &info, 0);
	}
	return type;
}

static void
sp_cell_renderer_image_class_init (SPCellRendererImageClass *klass)
{
	GObjectClass *object_class;
	GtkCellRendererClass *cr_class;

	object_class = (GObjectClass *) klass;
	cr_class = (GtkCellRendererClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	signals[CLICKED] = g_signal_new ("clicked",
					 G_OBJECT_CLASS_TYPE (object_class),
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET (SPCellRendererImageClass, clicked),
					 NULL, NULL,
					 sp_marshal_VOID__POINTER,
					 G_TYPE_NONE, 1, G_TYPE_STRING);
	signals[TOGGLED] = g_signal_new ("toggled",
					 G_OBJECT_CLASS_TYPE (object_class),
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET (SPCellRendererImageClass, toggled),
					 NULL, NULL,
					 sp_marshal_VOID__POINTER,
					 G_TYPE_NONE, 1, G_TYPE_STRING);

	object_class->finalize = sp_cell_renderer_image_finalize;
	object_class->get_property = sp_cell_renderer_image_get_property;
	object_class->set_property = sp_cell_renderer_image_set_property;

	cr_class->get_size = sp_cell_renderer_image_get_size;
	cr_class->render = sp_cell_renderer_image_render;
	cr_class->activate = sp_cell_renderer_image_activate;

	g_object_class_install_property (object_class, PROP_VALUE,
					 g_param_spec_int ("value", NULL, NULL, 0, 15, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class, PROP_ACTIVATABLE,
					 g_param_spec_boolean ("activatable", NULL, NULL, TRUE, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void
sp_cell_renderer_image_init (SPCellRendererImage *cri)
{
	GtkCellRenderer *cr;

	cr = (GtkCellRenderer *) cri;

	cr->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
}

static void
sp_cell_renderer_image_finalize (GObject *object)
{
	SPCellRendererImage *cri;

	cri = (SPCellRendererImage *) object;

	if (cri->images) {
		int i;
		/* Free pixbufs */
		for (i = 0; i < cri->numimages; i++) if (cri->images[i]) g_object_unref ((GObject *) cri->images[i]);
		free (cri->images);
	}
	((GObjectClass *) (parent_class))->finalize (object);
}

static void
sp_cell_renderer_image_get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
	SPCellRendererImage *cri;

	cri = (SPCellRendererImage *) object;

	switch (param_id) {
	case PROP_VALUE:
		g_value_set_int (value, cri->value);
		break;
	case PROP_ACTIVATABLE:
		g_value_set_boolean (value, cri->activatable);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
sp_cell_renderer_image_set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	SPCellRendererImage *cri;

	cri = (SPCellRendererImage *) object;

	switch (param_id) {
	case PROP_VALUE:
		cri->value = g_value_get_int (value);
		break;
	case PROP_ACTIVATABLE:
		cri->activatable = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
sp_cell_renderer_image_get_size (GtkCellRenderer *cr, GtkWidget *widget, GdkRectangle *cell_area,
				 gint *x_offset, gint *y_offset, gint *width, gint *height)
{
	SPCellRendererImage *cri;
	int cwidth, cheight;

	cri = (SPCellRendererImage *) cr;

	cwidth  = cri->size + (int) cr->xpad * 2 + widget->style->xthickness * 2;
	cheight = cri->size + (int) cr->ypad * 2 + widget->style->ythickness * 2;

	if (x_offset) *x_offset = 0;
	if (y_offset) *y_offset = 0;

	if (cell_area) {
		if (x_offset) {
			float ralign;
			ralign = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ? (1.0 - cr->xalign) : cr->xalign;
			*x_offset = ralign * (cell_area->width - cwidth);
			*x_offset = MAX (*x_offset, 0);
		}
		if (y_offset) {
			*y_offset = cr->yalign * (cell_area->height - cheight);
			*y_offset = MAX (*y_offset, 0);
		}
	}

	if (width) *width = cwidth;
	if (height) *height = cheight;
}

static void
sp_cell_renderer_image_render (GtkCellRenderer *cr, GdkWindow *window, GtkWidget *widget,
                                  GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area,
                                  GtkCellRendererState flags)
{
	SPCellRendererImage *cri;
	GdkRectangle fullarea;
	GdkRectangle draw_rect;

	cri = (SPCellRendererImage *) cr;

	/* fixme: Is background already painted? (Lauris) */
	if (!cri->activatable) return;

	sp_cell_renderer_image_get_size (cr, widget, cell_area, &fullarea.x, &fullarea.y, &fullarea.width, &fullarea.height);

	fullarea.x += cell_area->x + cr->xpad;
	fullarea.y += cell_area->y + cr->ypad;
	fullarea.width -= cr->xpad * 2;
	fullarea.height -= cr->ypad * 2;

	if (fullarea.width <= 0 || fullarea.height <= 0) return;

	if (cri->images && cri->images[cri->value]) {
		fullarea.x += widget->style->xthickness;
		fullarea.y += widget->style->ythickness;
		fullarea.width -= widget->style->xthickness * 2;
		fullarea.height -= widget->style->ythickness * 2;

		if (gdk_rectangle_intersect (expose_area, &fullarea, &draw_rect)) {
			gdk_draw_pixbuf (window, widget->style->black_gc,
					 (GdkPixbuf *) cri->images[cri->value],
					 draw_rect.x - fullarea.x,
					 draw_rect.y - fullarea.y,
					 draw_rect.x,
					 draw_rect.y,
					 draw_rect.width,
					 draw_rect.height,
					 GDK_RGB_DITHER_MAX,
					 0, 0);
		}
	}
}

static gboolean
sp_cell_renderer_image_activate (GtkCellRenderer *cr, GdkEvent *event, GtkWidget *widget, const gchar *path,
                                    GdkRectangle *background_area, GdkRectangle *cell_area, GtkCellRendererState flags)
{
	SPCellRendererImage *cri;

	cri = (SPCellRendererImage *) cr;

	if (cri->activatable) {
		cri->value = (cri->value + 1) % cri->numimages;
		g_signal_emit (cr, signals[CLICKED], 0, path);
		g_signal_emit (cr, signals[TOGGLED], 0, path);
		return TRUE;
	}
	return FALSE;
}

GtkCellRenderer *
sp_cell_renderer_image_new (unsigned int numimages, unsigned int size)
{
	SPCellRendererImage *cri;

	cri = (SPCellRendererImage *) g_object_new (SP_TYPE_CELL_RENDERER_IMAGE, NULL);
	cri->numimages = numimages;
	cri->images = (void **) malloc (cri->numimages * sizeof (void *));
	memset (cri->images, 0, cri->numimages * sizeof (void *));
	cri->size = size;

	return (GtkCellRenderer *) cri;
}

void
sp_cell_renderer_image_set_image (SPCellRendererImage *cri, int imageid, const unsigned char *pxname)
{
	unsigned char *px;
	/* fixme: Memleak */
	px = sp_icon_image_load (pxname, cri->size, 24);
	if (px) {
		if (cri->images[imageid]) g_object_unref ((GObject *) cri->images[imageid]);
		cri->images[imageid] = gdk_pixbuf_new_from_data (px, GDK_COLORSPACE_RGB, TRUE, 8,
								 cri->size, cri->size, 4 * cri->size,
								 NULL, NULL);
	}
}
