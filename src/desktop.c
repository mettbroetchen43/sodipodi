#define SP_DESKTOP_C

/*
 * SPDesktop
 *
 * This is infinite drawing board in device (gnome-print, PS) coords - i.e.
 *   Y grows UPWARDS, X to RIGHT
 * For both Canvas and SVG the coordinate space has to be flipped - this
 *   is normally done by SPRoot item.
 * SPRoot is the only child of SPDesktop->main canvas group and should be
 *   the parent of all "normal" drawing items
 *
 */

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

static void sp_desktop_update_rulers (SPDesktop * desktop);
static void sp_desktop_set_viewport (SPDesktop * desktop, double x, double y);

/* Signal handlers */

gint sp_desktop_activate (GtkWidget * widget, GdkEventCrossing * event, gpointer data);

GtkEventBoxClass * parent_class;

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

		desktop_type = gtk_type_unique (GTK_TYPE_EVENT_BOX, &desktop_info);
	}

	return desktop_type;
}

static void
sp_desktop_class_init (SPDesktopClass * klass)
{
	GtkObjectClass * object_class;
	GtkWidgetClass * widget_class;

	parent_class = gtk_type_class (GTK_TYPE_EVENT_BOX);

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	object_class->destroy = sp_desktop_destroy;
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
	art_affine_identity (desktop->d2w);
	art_affine_identity (desktop->w2d);

	desktop->decorations = TRUE;

	desktop->table = GTK_TABLE (gtk_table_new (3, 3, FALSE));
	gtk_widget_show (GTK_WIDGET (desktop->table));
	gtk_container_add (GTK_CONTAINER (desktop), GTK_WIDGET (desktop->table));

	desktop->hscrollbar = GTK_SCROLLBAR (gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->hscrollbar));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->hscrollbar),
		1,2,2,3,
		GTK_EXPAND | GTK_FILL,
		GTK_FILL,
		0,0);
	desktop->vscrollbar = GTK_SCROLLBAR (gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->vscrollbar));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->vscrollbar),
		2,3,1,2,
		GTK_FILL,
		GTK_EXPAND | GTK_FILL,
		0,0);
	desktop->hruler = GTK_RULER (gtk_hruler_new ());
	gtk_widget_show (GTK_WIDGET (desktop->hruler));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->hruler),
		1,2,0,1,
		GTK_FILL,
		GTK_FILL,
		0,0);
	desktop->vruler = GTK_RULER (gtk_vruler_new ());
	gtk_widget_show (GTK_WIDGET (desktop->vruler));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->vruler),
		0,1,1,2,
		GTK_FILL,
		GTK_FILL,
		0,0);
	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());
	desktop->canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	gtk_widget_show (GTK_WIDGET (desktop->canvas));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->canvas),
		1,2,1,2,
		GTK_EXPAND | GTK_FILL,
		GTK_EXPAND | GTK_FILL,
		0,0);

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
			sp_item_hide (SP_ITEM (desktop->document->root), desktop->canvas);
		gtk_object_unref (GTK_OBJECT (desktop->document));
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

#if 0
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
#endif

#if 0
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
#endif

/* Constructor */

SPDesktop *
sp_desktop_new (SPDocument * document)
{
	SPDesktop * desktop;
	GtkWidget * dwidget;
	GladeXML * xml;
	GtkStyle * style;
	GnomeCanvasGroup * root;
	GnomeCanvasItem * ci;
	GtkAdjustment * hadj, * vadj;
	gdouble dw, dh;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	/* Setup widget */

	desktop = (SPDesktop *) gtk_type_new (sp_desktop_get_type ());

	gtk_signal_connect (GTK_OBJECT (desktop), "enter_notify_event",
		GTK_SIGNAL_FUNC (sp_desktop_activate), desktop);

	/* Setup document */

	desktop->document = document;
	gtk_object_ref (GTK_OBJECT (document));

	/* Setup Canvas */
	gtk_object_set_data (GTK_OBJECT (desktop->canvas), "SPDesktop", desktop);

	style = gtk_style_copy (GTK_WIDGET (desktop->canvas)->style);
	style->bg[GTK_STATE_NORMAL] = style->white;
	gtk_widget_set_style (GTK_WIDGET (desktop->canvas), style);

	root = gnome_canvas_root (desktop->canvas);

	desktop->acetate = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->acetate), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), desktop);
	desktop->main = (GnomeCanvasGroup *) gnome_canvas_item_new (root,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->main), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), desktop);
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

	desktop->selection = sp_selection_new (desktop);

	desktop->event_context = sp_event_context_new (desktop, SP_TYPE_SELECT_CONTEXT);

	/* fixme: Setup display rectangle */

	dw = sp_document_width (document);
	dh = sp_document_height (document);
#if 0
	gnome_canvas_item_new (desktop->grid, GNOME_TYPE_CANVAS_RECT,
		"x1", 0.0, "y1", 0.0, "x2", dw, "y2", dh,
		"outline_color", "black", "width_pixels", 2, NULL);
#endif
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

	ci = sp_item_show (SP_ITEM (desktop->document->root), desktop->drawing, sp_desktop_item_handler);

	return desktop;
}

void
sp_desktop_show_decorations (SPDesktop * desktop, gboolean show)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	if (desktop->decorations == show) return;

	desktop->decorations = show;

	if (show) {
		gtk_widget_show (GTK_WIDGET (desktop->hscrollbar));
		gtk_widget_show (GTK_WIDGET (desktop->vscrollbar));
		gtk_widget_show (GTK_WIDGET (desktop->hruler));
		gtk_widget_show (GTK_WIDGET (desktop->vruler));
	} else {
		gtk_widget_hide (GTK_WIDGET (desktop->hscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->vscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->hruler));
		gtk_widget_hide (GTK_WIDGET (desktop->vruler));
	}
}

void
sp_desktop_change_document (SPDesktop * desktop, SPDocument * document)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));

	sp_item_hide (SP_ITEM (desktop->document->root), desktop->canvas);

	gtk_object_ref (GTK_OBJECT (document));
	gtk_object_unref (GTK_OBJECT (desktop->document));

	desktop->document = document;

	sp_item_show (SP_ITEM (desktop->document->root), desktop->drawing, sp_desktop_item_handler);
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

/*
 * fixme: this conatins a hack, to deal with deleting a view, which is
 * completely on another view, in which case active_desktop will not be updated
 *
 */

void
sp_desktop_item_handler (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	gpointer ddata;
	SPDesktop * desktop;

	ddata = gtk_object_get_data (GTK_OBJECT (item->canvas), "SPDesktop");
	g_return_if_fail (ddata != NULL);

	desktop = SP_DESKTOP (ddata);

	sp_event_context_item_handler (desktop->event_context, SP_ITEM (data), event);
}

/* Signal handlers */

gint
sp_desktop_activate (GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
	g_assert (data != NULL);
	g_assert (SP_IS_DESKTOP (data));

	sp_active_desktop_set (SP_DESKTOP (data));

	return FALSE;
}


/* fixme: this is UI functions - find a better place */

void
sp_desktop_toggle_borders (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	sp_desktop_show_decorations (desktop, !desktop->decorations);
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
