#define SP_DESKTOP_C

#include <math.h>
#include <gnome.h>
#include <glade/glade.h>
#include "helper/canvas-helper.h"
#include "event-broker.h"
#include "desktop-affine.h"
#include "desktop.h"

#define SP_DESKTOP_SCROLL_LIMIT 4000.0

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_destroy (GtkObject * object);

static void sp_desktop_size_request (GtkWidget * widget, GtkRequisition * requisition);
static void sp_desktop_size_allocate (GtkWidget * widget, GtkAllocation * allocation);

static void sp_desktop_document_destroyed (SPDocument * document, SPDesktop * desktop);

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
	if ((desktop->document) && (desktop->canvas))
		sp_item_hide (SP_ITEM (desktop->document), desktop->canvas);
#if 0
	if (desktop->window)
		gtk_object_destroy (GTK_OBJECT (desktop->window));
#endif
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

	xml = glade_xml_new (SODIPODI_GLADEDIR "/sodipodi.glade", "desktop");
	g_return_val_if_fail (xml != NULL, NULL);
	glade_xml_signal_autoconnect (xml);

	desktop = (SPDesktop *) gtk_type_new (sp_desktop_get_type ());

	GTK_BOX (desktop)->spacing = 4;
	GTK_BOX (desktop)->homogeneous = TRUE;

	desktop->document = document;
#if 0
	gtk_signal_connect (GTK_OBJECT (document), "destroy",
		GTK_SIGNAL_FUNC (sp_desktop_document_destroyed), desktop);
#endif

	desktop->selection = sp_selection_new ();

#if 0
	desktop->window = glade_xml_get_widget (xml, "sodipodi");
#endif
	dwidget = glade_xml_get_widget (xml, "desktop");
	gtk_object_set_data (GTK_OBJECT (dwidget), "SPDesktop", desktop);
	gtk_box_pack_start_defaults (GTK_BOX (desktop), dwidget);
#if 0
	desktop->statusbar = glade_xml_get_widget (xml, "status_bar");
#endif
	desktop->hscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "hscrollbar");
	desktop->vscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "vscrollbar");
	desktop->hruler = (GtkRuler *) glade_xml_get_widget (xml, "hruler");
	desktop->vruler = (GtkRuler *) glade_xml_get_widget (xml, "vruler");

	desktop->canvas = (GnomeCanvas *) glade_xml_get_widget (xml, "canvas");

	root = gnome_canvas_root (desktop->canvas);

	desktop->acetate = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->acetate), "event",
		GTK_SIGNAL_FUNC (sp_root_handler), desktop);
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

	/* fixme */
	dw = sp_document_page_width (document);
	dh = sp_document_page_height (document);
	gnome_canvas_item_new (desktop->grid, GNOME_TYPE_CANVAS_RECT,
		"x1", 0.0, "y1", 0.0, "x2", dw, "y2", dh,
		"outline_color", "black", "width_pixels", 2, NULL);

#if 0
	art_affine_scale (desktop->d2w, 0.4, -0.4);
	art_affine_invert (desktop->w2d, desktop->d2w);
	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);
#endif
	gnome_canvas_set_scroll_region (desktop->canvas,
		-SP_DESKTOP_SCROLL_LIMIT,
		-SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);
#if 0
	gnome_canvas_scroll_to (desktop->canvas,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);
	sp_desktop_set_position (desktop, 0.0, 0.0);
	sp_desktop_update_rulers (desktop);
#else
	/* sp_desktop_show_region (desktop, 0.0, 0.0, dw, dh); */
#endif

	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);
#if 1
	sp_desktop_show_region (desktop, 0.0, 0.0, dw, dh);
#endif
	ci = sp_item_show (SP_ITEM (desktop->document), desktop->drawing, sp_event_handler);
#if 0
	gtk_signal_connect (GTK_OBJECT (ci), "event",
		GTK_SIGNAL_FUNC (sp_event_handler), document);
#endif
#if 0
	/* fixme: this seems the right way */
	sp_context_set (desktop, SP_CONTEXT_SELECT);
#endif

#if 0
	active_desktop = desktop;

	num_desktops++;
#endif

	return desktop;
}

/* Private handler */

static void
sp_desktop_document_destroyed (SPDocument * document, SPDesktop * desktop)
{
	/* fixme: should we assign ->document = NULL before ? */
	gtk_object_destroy (GTK_OBJECT (desktop));
}

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



#if 0

static void sp_desktop_update_rulers (void);

GtkWidget * status_bar;

GnomeCanvasGroup * sp_desktop_create (GladeXML * xml)
{
	GnomeCanvasGroup * root;
	GnomeCanvasItem * ac;
	GtkAdjustment * hadj, * vadj;

	desktop.canvas = (GnomeCanvas *) glade_xml_get_widget (xml, "canvas");
	g_return_val_if_fail (GNOME_IS_CANVAS (desktop.canvas), NULL);

	root = gnome_canvas_root (desktop.canvas);

	ac = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	gtk_signal_connect (GTK_OBJECT (ac), "event",
		GTK_SIGNAL_FUNC (sp_root_handler), ac);

	desktop.self = (GnomeCanvasGroup *) gnome_canvas_item_new (root,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop.grid = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop.self,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop.guides = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop.self,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop.drawarea = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop.self,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop.sketch = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop.self,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop.controls = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop.self,
		GNOME_TYPE_CANVAS_GROUP, NULL);

	desktop.hscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "hscrollbar");
	desktop.vscrollbar = (GtkScrollbar *) glade_xml_get_widget (xml, "vscrollbar");
	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop.hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop.vscrollbar));
	desktop.hruler = (GtkRuler *) glade_xml_get_widget (xml, "hruler");
	desktop.vruler = (GtkRuler *) glade_xml_get_widget (xml, "vruler");

	art_affine_scale (desktop.d2w, 0.4, -0.4);
	art_affine_invert (desktop.w2d, desktop.d2w);
	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop.self, desktop.d2w);

	gnome_canvas_set_scroll_region (desktop.canvas,
		-SP_DESKTOP_SCROLL_LIMIT,
		-SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);
	gnome_canvas_scroll_to (desktop.canvas,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);

	sp_desktop_set_position (0.0, 0.0);
	sp_desktop_update_rulers ();

	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop.canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop.canvas), vadj);

	main_window = (GtkWindow *) glade_xml_get_widget (xml, "sodipodi");
	status_bar = glade_xml_get_widget (xml, "status_bar");

	return desktop.self;
}

GnomeCanvasGroup * sp_desktop (void)
{
	return desktop.drawarea;
}

GnomeCanvasGroup * sp_desktop_controls (void)
{
	return desktop.controls;
}

GnomeCanvasGroup * sp_desktop_grid (void)
{
	return desktop.grid;
}

GnomeCanvasGroup * sp_desktop_guides (void)
{
	return desktop.guides;
}

GnomeCanvasGroup * sp_desktop_sketch (void)
{
	return desktop.sketch;
}


void
sp_desktop_zoom (double zoom, double x, double y)
{
	double cw, ch;

	cw = GTK_WIDGET (desktop.canvas)->allocation.width;
	ch = GTK_WIDGET (desktop.canvas)->allocation.height;

	desktop.d2w[0] *= zoom;
	desktop.d2w[1] *= zoom;
	desktop.d2w[2] *= zoom;
	desktop.d2w[3] *= zoom;

	desktop.d2w[4] = -x * desktop.d2w[0];
	desktop.d2w[5] = -y * desktop.d2w[3];
	art_affine_invert (desktop.w2d, desktop.d2w);

	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop.self, desktop.d2w);

	gnome_canvas_scroll_to (desktop.canvas, SP_DESKTOP_SCROLL_LIMIT - cw / 2, SP_DESKTOP_SCROLL_LIMIT - ch / 2);

	sp_desktop_update_rulers ();
}

void
sp_w2d (ArtPoint * d, ArtPoint * s)
{
	art_affine_point (d, s, desktop.w2d);
}

ArtDRect *
sp_w2d_drect_drect (ArtDRect * d, ArtDRect * s)
{
	ArtPoint p0, p1;

	p0.x = s->x0;
	p0.y = s->y0;
	art_affine_point (&p0, &p0, desktop.w2d);
	p1.x = s->x1;
	p1.y = s->y1;
	art_affine_point (&p1, &p1, desktop.w2d);

	d->x0 = p0.x;
	d->y0 = p0.y;
	d->x1 = p1.x;
	d->y1 = p1.y;

	return d;
}

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
