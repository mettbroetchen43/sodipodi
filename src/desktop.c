#define SP_DESKTOP_C

#include <math.h>
#include <gnome.h>
#include <glade/glade.h>
#include "helper/canvas-helper.h"
#include "mdi-desktop.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "desktop.h"
#include "select-context.h"

#define SP_DESKTOP_SCROLL_LIMIT 4000.0

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_destroy (GtkObject * object);

static void sp_desktop_size_request (GtkWidget * widget, GtkRequisition * requisition);
static void sp_desktop_size_allocate (GtkWidget * widget, GtkAllocation * allocation);

static void sp_desktop_update_rulers (SPDesktop * desktop);
static void sp_desktop_set_viewport (SPDesktop * desktop, double x, double y);

GtkBoxClass * parent_class;

GtkType
sp_desktop_get_type (void)
{
	static GtkType desktop_type = 0;

	if (!desktop_type) {
		static const GtkTypeInfo desktop_info = {
			"SPDesktop",
			sizeof (SPDesktop),
			sizeof (SPDesktopClass),
			(GtkClassInitFunc) sp_desktop_class_init,
			(GtkObjectInitFunc) sp_desktop_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		desktop_type = gtk_type_unique (gtk_box_get_type (), &desktop_info);
	}

	return desktop_type;
}

static void
sp_desktop_class_init (SPDesktopClass * klass)
{
	GtkObjectClass * object_class;
	GtkWidgetClass * widget_class;

	parent_class = gtk_type_class (gtk_box_get_type ());

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	object_class->destroy = sp_desktop_destroy;

	widget_class->size_request = sp_desktop_size_request;
	widget_class->size_allocate = sp_desktop_size_allocate;
}

static void
sp_desktop_init (SPDesktop * desktop)
{
	desktop->document = NULL;
	desktop->selection = NULL;
	desktop->canvas = NULL;
	desktop->acetate = NULL;
	desktop->main = NULL;
	desktop->grid = NULL;
	desktop->guides = NULL;
	desktop->drawing = NULL;
	desktop->sketch = NULL;
	desktop->controls = NULL;
	desktop->hscrollbar = NULL;
	desktop->vscrollbar = NULL;
	desktop->hruler = NULL;
	desktop->vruler = NULL;
	art_affine_identity (desktop->d2w);
	art_affine_identity (desktop->w2d);
}

static void
sp_desktop_destroy (GtkObject * object)
{
	SPDesktop * desktop;

	desktop = SP_DESKTOP (object);

	if (desktop->selection)
		gtk_object_destroy (GTK_OBJECT (desktop->selection));

	if (desktop->event_context)
		gtk_object_destroy (GTK_OBJECT (desktop->event_context));

	if (desktop->document) {
		if (desktop->canvas)
			sp_item_hide (SP_ITEM (desktop->document), desktop->canvas);
		gtk_object_unref (GTK_OBJECT (desktop->document));
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_desktop_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
	GtkBox * box;
	GtkWidget * child;

	box = GTK_BOX (widget);

	if (box->children != NULL) {
		child = ((GtkBoxChild *) box->children->data)->widget;
		gtk_widget_size_request (child, requisition);
	} else {
		requisition->width = 600;
		requisition->height = 400;
	}

	requisition->width += GTK_CONTAINER (box)->border_width * 2;
	requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
sp_desktop_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	GtkBox * box;
	GtkWidget * child;
	GtkAllocation child_allocation;

	box = GTK_BOX (widget);

	widget->allocation = * allocation;

	if (box->children != NULL) {
		child = ((GtkBoxChild *) box->children->data)->widget;

		child_allocation.x = allocation->x + GTK_CONTAINER (widget)->border_width;
		child_allocation.y = allocation->y + GTK_CONTAINER (widget)->border_width;
		child_allocation.width = allocation->width - 2 * GTK_CONTAINER (widget)->border_width;
		child_allocation.height = allocation->height - 2 * GTK_CONTAINER (widget)->border_width;

		gtk_widget_size_allocate (child, &child_allocation);
	}
}

/* Constructor */

SPDesktop *
sp_desktop_new (SPDocument * document)
{
	SPDesktop * desktop;
	GtkWidget * dwidget;
	GladeXML * xml;
	GnomeCanvasGroup * root;
	GnomeCanvasItem * ci;
	GtkAdjustment * hadj, * vadj;
	gdouble dw, dh;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	/* Setup widget */

	xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "desktop");
	g_return_val_if_fail (xml != NULL, NULL);
	glade_xml_signal_autoconnect (xml);

	desktop = (SPDesktop *) gtk_type_new (sp_desktop_get_type ());

	GTK_BOX (desktop)->spacing = 4;
	GTK_BOX (desktop)->homogeneous = TRUE;

	dwidget = glade_xml_get_widget (xml, "desktop");
	gtk_object_set_data (GTK_OBJECT (dwidget), "SPDesktop", desktop);
	gtk_box_pack_start_defaults (GTK_BOX (desktop), dwidget);

	/* Setup document */

	desktop->document = document;
	gtk_object_ref (GTK_OBJECT (document));

	desktop->selection = sp_selection_new ();

	desktop->event_context = sp_event_context_new (desktop, SP_TYPE_SELECT_CONTEXT);

	desktop->hscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "hscrollbar");
	desktop->vscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "vscrollbar");
	desktop->hruler = (GtkRuler *) glade_xml_get_widget (xml, "hruler");
	desktop->vruler = (GtkRuler *) glade_xml_get_widget (xml, "vruler");

	desktop->canvas = (GnomeCanvas *) glade_xml_get_widget (xml, "canvas");

	root = gnome_canvas_root (desktop->canvas);

	desktop->acetate = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->acetate), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), desktop);
	desktop->main = (GnomeCanvasGroup *) gnome_canvas_item_new (root,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->grid = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->guides = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->drawing = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->sketch = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->controls = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);

	/* fixme: Setup display rectangle */

	dw = sp_document_page_width (document);
	dh = sp_document_page_height (document);

	gnome_canvas_item_new (desktop->grid, GNOME_TYPE_CANVAS_RECT,
		"x1", 0.0, "y1", 0.0, "x2", dw, "y2", dh,
		"outline_color", "black", "width_pixels", 2, NULL);

	/* Fixme: Setup initial zooming */

	art_affine_scale (desktop->d2w, 1.0, -1.0);
	desktop->d2w[5] = dw;
	art_affine_invert (desktop->w2d, desktop->d2w);
	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);

	gnome_canvas_set_scroll_region (desktop->canvas,
		-SP_DESKTOP_SCROLL_LIMIT,
		-SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);

	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);

	ci = sp_item_show (SP_ITEM (desktop->document), desktop->drawing, sp_desktop_item_handler);

	return desktop;
}

#if 0
/* Private handler */
static void
sp_desktop_document_destroyed (SPDocument * document, SPDesktop * desktop)
{
	/* fixme: should we assign ->document = NULL before ? */
	gtk_object_destroy (GTK_OBJECT (desktop));
}
#endif

/* Private methods */

static void
sp_desktop_update_rulers (SPDesktop * desktop)
{
	ArtPoint p0, p1;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	gnome_canvas_window_to_world (desktop->canvas,
		0.0, 0.0, &p0.x, &p0.y);
	gnome_canvas_window_to_world (desktop->canvas,
		((GtkWidget *) desktop->canvas)->allocation.width,
		((GtkWidget *) desktop->canvas)->allocation.height,
		&p1.x, &p1.y);
	art_affine_point (&p0, &p0, desktop->w2d);
	art_affine_point (&p1, &p1, desktop->w2d);

	gtk_ruler_set_range (desktop->hruler, p0.x, p1.x, desktop->hruler->position, p1.x);
	gtk_ruler_set_range (desktop->vruler, p0.y, p1.y, desktop->vruler->position, p1.y);
}

/* Public methods */

#if 0
void
sp_desktop_set_focus (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = (SPDesktop *) gtk_object_get_data (GTK_OBJECT (widget), "SPDesktop");

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));
#if 0
	active_desktop = desktop;
#endif
}

gboolean
sp_desktop_close (GtkWidget * widget)
{
	SPDesktop * desktop;

	/* fixme: do it the right way - please!!! */

	desktop = (SPDesktop *) gtk_object_get_data (GTK_OBJECT (widget), "SPDesktop");

	g_return_val_if_fail (desktop != NULL, TRUE);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), TRUE);

	gtk_object_destroy (GTK_OBJECT (desktop));
#if 0
	num_desktops--;

	if (num_desktops < 1)
		gtk_main_quit ();
#endif
	return FALSE;
}

SPDesktop *
sp_desktop_widget_desktop (GtkWidget * widget)
{
	SPDesktop * desktop;

	g_return_val_if_fail (widget != NULL, NULL);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

	while (widget->parent != NULL) {
		widget = widget->parent;
	}
	desktop = (SPDesktop *) gtk_object_get_data (GTK_OBJECT (widget), "SPDesktop");
#if 0
	if (desktop == NULL) desktop = active_desktop;
#endif
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);

	return desktop;
}
#endif

void
sp_desktop_set_position (SPDesktop * desktop, double x, double y)
{
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	desktop->hruler->position = x;
	gtk_ruler_draw_pos (desktop->hruler);
	desktop->vruler->position = y;
	gtk_ruler_draw_pos (desktop->vruler);
}

void
sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy)
{
	gint x, y;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	gnome_canvas_get_scroll_offsets (desktop->canvas, &x, &y);
	gnome_canvas_scroll_to (desktop->canvas, x - dx, y - dy);

	sp_desktop_update_rulers (desktop);
}

/* Zooming & display */

ArtDRect *
sp_desktop_get_visible_area (SPDesktop * desktop, ArtDRect * area)
{
	gdouble cw, ch;
	ArtPoint p0, p1;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (area != NULL, NULL);

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	gnome_canvas_window_to_world (desktop->canvas, 0.0, 0.0, &p0.x, &p0.y);
	gnome_canvas_window_to_world (desktop->canvas, cw, ch, &p1.x, &p1.y);

	art_affine_point (&p0, &p0, desktop->w2d);
	art_affine_point (&p1, &p1, desktop->w2d);

	area->x0 = MIN (p0.x, p1.x);
	area->y0 = MIN (p0.y, p1.y);
	area->x1 = MAX (p0.x, p1.x);
	area->y1 = MAX (p0.y, p1.y);

	return area;
}

void
sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1)
{
	ArtPoint c, p;
	double w, h, cw, ch, sw, sh, scale;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	c.x = (x0 + x1) / 2;
	c.y = (y0 + y1) / 2;

	w = fabs (x1 - x0) + 10.0;
	h = fabs (y1 - y0) + 10.0;

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	sw = cw / w;
	sh = ch / h;
	scale = MIN (sw, sh);
	w = cw / scale;
	h = ch / scale;

	p.x = c.x - w / 2;
	p.y = c.y - h / 2;

	art_affine_scale (desktop->d2w, scale, -scale);
	desktop->d2w[4] = - p.x * scale - cw / 2;
	desktop->d2w[5] = p.y * scale + ch / 2;
	art_affine_invert (desktop->w2d, desktop->d2w);

	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);

	sp_desktop_set_viewport (desktop, SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT);
}

void
sp_desktop_zoom_relative (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy)
{
	double cw, ch;

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	desktop->d2w[0] *= zoom;
	desktop->d2w[1] *= zoom;
	desktop->d2w[2] *= zoom;
	desktop->d2w[3] *= zoom;

	desktop->d2w[4] = -cx * desktop->d2w[0];
	desktop->d2w[5] = -cy * desktop->d2w[3];
	art_affine_invert (desktop->w2d, desktop->d2w);

	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);

	gnome_canvas_scroll_to (desktop->canvas, SP_DESKTOP_SCROLL_LIMIT - cw / 2, SP_DESKTOP_SCROLL_LIMIT - ch / 2);

	sp_desktop_update_rulers (desktop);
}

void
sp_desktop_zoom_absolute (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy)
{
	double cw, ch;

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	desktop->d2w[0] = zoom;
	desktop->d2w[1] = 0.0;
	desktop->d2w[2] = 0.0;
	desktop->d2w[3] = -zoom;

	desktop->d2w[4] = -cx * desktop->d2w[0];
	desktop->d2w[5] = -cy * desktop->d2w[3];
	art_affine_invert (desktop->w2d, desktop->d2w);

	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);

	gnome_canvas_scroll_to (desktop->canvas, SP_DESKTOP_SCROLL_LIMIT - cw / 2, SP_DESKTOP_SCROLL_LIMIT - ch / 2);

	sp_desktop_update_rulers (desktop);
}

/* Context switching */

void
sp_desktop_set_event_context (SPDesktop * desktop, GtkType type)
{
	if (desktop->event_context)
		gtk_object_destroy (GTK_OBJECT (desktop->event_context));

	desktop->event_context = sp_event_context_new (desktop, type);
}

/* Private helpers */

/*
 * Set viewport center to x,y in world coordinates
 */

static void
sp_desktop_set_viewport (SPDesktop * desktop, double x, double y)
{
	double cw, ch;

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	gnome_canvas_scroll_to (desktop->canvas, x - cw / 2, y - ch / 2);

	sp_desktop_update_rulers (desktop);
}

void
sp_desktop_root_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	sp_event_context_root_handler (SP_DESKTOP (data)->event_context, event);
}

void
sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	sp_event_context_item_handler ((SP_ACTIVE_DESKTOP)->event_context, SP_ITEM (data), event);
}

#if 0

/* fixme: */

void
sp_desktop_set_title (const gchar * title)
{
	gtk_window_set_title (main_window, title);
}

void sp_desktop_set_status (const gchar * text)
{
	gnome_appbar_set_status (GNOME_APPBAR (status_bar), text);
}

#endif
