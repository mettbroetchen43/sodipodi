#define SP_DESKTOP_C

/*
 * SPDesktop
 *
 * This is infinite drawing board in device (gnome-print, PS) coords - i.e.
 *   Y grows UPWARDS, X to RIGHT
 * For both Canvas and SVG the coordinate space has to be flipped - this
 *   is normally done by SPRoot item.
 * SPRoot is the only child of SPDesktop->main canvas group and should be
 *   the parent of all "normal" drawing items - this is quite normal, as
 *   SPRoot is rendered from <svg> node.
 *
 */

#include <math.h>
#include <gnome.h>
#include <glade/glade.h>
#include "helper/canvas-helper.h"
#include "sodipodi-private.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "desktop.h"
#include "select-context.h"
#include "event-context.h"

#define SP_DESKTOP_SCROLL_LIMIT 4000.0

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_destroy (GtkObject * object);

static void sp_desktop_update_rulers (GtkWidget * widget, SPDesktop * desktop);
static void sp_desktop_set_viewport (SPDesktop * desktop, double x, double y);

void sp_desktop_indicator_on (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);
void sp_desktop_indicator_off (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);

/* fixme: */
void sp_desktop_toggle_borders (GtkWidget * widget);

GtkEventBoxClass * parent_class;

void zoom_any_update (gdouble any);
extern GtkWidget * zoom_any;


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

	widget_class->enter_notify_event = sp_desktop_enter_notify;
#if 0
	widget_class->button_press_event = sp_desktop_button_press;
	widget_class->button_release_event = sp_desktop_button_release;
	widget_class->motion_notify_event = sp_desktop_motion_notify;
#endif
}

static void
sp_desktop_init (SPDesktop * desktop)
{
  //	GtkWidget * menu_button;
	GtkWidget * menu_arrow;
	GtkWidget * hbox; 
	GtkWidget * eventbox;

	desktop->document = NULL;
	desktop->namedview = NULL;
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
	desktop->guides_active = FALSE;

	desktop->table = GTK_TABLE (gtk_table_new (3, 3, FALSE));
	gtk_widget_show (GTK_WIDGET (desktop->table));
	gtk_container_add (GTK_CONTAINER (desktop), GTK_WIDGET (desktop->table));

	/* Horizontal scrollbar */
	desktop->hscrollbar = GTK_SCROLLBAR (gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->hscrollbar));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->hscrollbar),
		1,2,2,3,
		GTK_EXPAND | GTK_FILL,
		GTK_FILL,
		0,0);
	/* Vertical scrollbar */
	desktop->vscrollbar = GTK_SCROLLBAR (gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->vscrollbar));
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->vscrollbar),
		2,3,1,2,
		GTK_FILL,
		GTK_EXPAND | GTK_FILL,
		0,0);
	/* Horizonatl ruler */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	desktop->hruler = GTK_RULER (gtk_hruler_new ());
	gtk_widget_show (GTK_WIDGET (desktop->hruler));
	gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (desktop->hruler));
	gtk_table_attach (desktop->table,
		eventbox,
		1,2,0,1,
		GTK_FILL,
		GTK_FILL,
		0,0);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_release_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "motion_notify_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	/* Vertical ruler */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	desktop->vruler = GTK_RULER (gtk_vruler_new ());
	gtk_widget_show (GTK_WIDGET (desktop->vruler));
	gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (desktop->vruler));
	gtk_table_attach (desktop->table,
		eventbox,
		0,1,1,2,
		GTK_FILL,
		GTK_FILL,
		0,0);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_release_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "motion_notify_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	/* Canvas */
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
	/* menu button */
	desktop->menubutton = gtk_button_new ();
	gtk_widget_show (desktop->menubutton);
	gtk_table_attach (desktop->table,
		GTK_WIDGET (desktop->menubutton),
		0,1,0,1,
		FALSE,
		FALSE,
		0,0);
	menu_arrow = gtk_arrow_new (GTK_ARROW_RIGHT,GTK_SHADOW_ETCHED_IN);
	gtk_widget_show (menu_arrow);
	gtk_container_add ((GtkContainer *)desktop->menubutton, menu_arrow);
	gtk_signal_connect (GTK_OBJECT (desktop-> menubutton), 
			    "button_press_event", 
			    GTK_SIGNAL_FUNC (sp_event_root_menu_popup), 
			    NULL);
	/* indicator for active desktop */
	hbox = gtk_vbox_new (FALSE, 0);
	gtk_table_attach (desktop->table,
		GTK_WIDGET (hbox),
		2,3,0,1,
		FALSE,
		FALSE,
		0,0);
	gtk_widget_show (hbox);
	desktop->active = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/dt_active.xpm");
	gtk_box_pack_start ((GtkBox *) hbox, desktop->active, TRUE, TRUE, 0);
	desktop->inactive = gnome_pixmap_new_from_file (SODIPODI_GLADEDIR "/dt_inactive.xpm");
	gtk_box_pack_start ((GtkBox *) hbox, desktop->inactive, TRUE, TRUE, 0);
       	gtk_widget_show (desktop->inactive);

	gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "activate_desktop",
					GTK_SIGNAL_FUNC (sp_desktop_indicator_on),
					NULL,
					GTK_OBJECT (desktop));

	gtk_signal_connect_while_alive (GTK_OBJECT (SODIPODI), "desactivate_desktop",
					GTK_SIGNAL_FUNC (sp_desktop_indicator_off),
					NULL,
					GTK_OBJECT (desktop));

}

static void
sp_desktop_destroy (GtkObject * object)
{
	SPDesktop * desktop;

	desktop = SP_DESKTOP (object);

	sodipodi_remove_desktop (desktop);

	if (desktop->selection)
		gtk_object_destroy (GTK_OBJECT (desktop->selection));

	if (desktop->event_context)
		gtk_object_destroy (GTK_OBJECT (desktop->event_context));

	if (desktop->document) {
		if (desktop->canvas) {
			sp_namedview_hide (desktop->namedview, desktop);
			sp_item_hide (SP_ITEM (sp_document_root (desktop->document)), desktop->canvas);
		}
		gtk_object_unref (GTK_OBJECT (desktop->document));
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Constructor */

SPDesktop *
sp_desktop_new (SPDocument * document, SPNamedView * namedview)
{
	SPDesktop * desktop;
	GtkStyle * style;
	GnomeCanvasGroup * root;
	GnomeCanvasItem * ci;
	GtkAdjustment * hadj, * vadj;
	gdouble dw, dh;
	ArtDRect pdim;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (namedview != NULL, NULL);
	g_return_val_if_fail (SP_IS_NAMEDVIEW (namedview), NULL);

	/* Setup widget */

	desktop = (SPDesktop *) gtk_type_new (sp_desktop_get_type ());

	/* Setup document */

	desktop->document = document;
	gtk_object_ref (GTK_OBJECT (document));

	desktop->namedview = namedview;

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
	desktop->drawing = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->grid = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
		GNOME_TYPE_CANVAS_GROUP, NULL);
	desktop->guides = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main,
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

	desktop->page = gnome_canvas_item_new (desktop->grid, SP_TYPE_CTRLRECT, NULL);
	pdim.x0 = pdim.y0 = 0.0;
	pdim.x1 = dw;
	pdim.y1 = dh;
	sp_ctrlrect_set_rect ((SPCtrlRect *) desktop->page, &pdim);

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


	// connecting canvas, scrollbars, rulers
	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);
	gtk_signal_connect (GTK_OBJECT (hadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (vadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (hadj), "changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (vadj), "changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);

	ci = sp_item_show (SP_ITEM (sp_document_root (desktop->document)), desktop->drawing, sp_desktop_item_handler);

	sp_namedview_show (desktop->namedview, desktop);
	/* Ugly hack */
	sp_desktop_activate_guides (desktop, TRUE);

	// ?
	// sp_active_desktop_set (desktop);
	sodipodi_add_desktop (desktop);

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
		gtk_widget_show (GTK_WIDGET (desktop->menubutton));
		gtk_widget_show (GTK_WIDGET (desktop->active));		
	} else {
		gtk_widget_hide (GTK_WIDGET (desktop->hscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->vscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->hruler));
		gtk_widget_hide (GTK_WIDGET (desktop->vruler));
		gtk_widget_hide (GTK_WIDGET (desktop->menubutton));
		gtk_widget_hide (GTK_WIDGET (desktop->active));
		gtk_widget_hide (GTK_WIDGET (desktop->inactive));
	}
}

void
sp_desktop_indicator_on (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	
	gtk_widget_show (GTK_WIDGET (desktop->active));		
	gtk_widget_hide (GTK_WIDGET (desktop->inactive));		
}

void
sp_desktop_indicator_off (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	
	gtk_widget_hide (GTK_WIDGET (desktop->active));		
	gtk_widget_show (GTK_WIDGET (desktop->inactive));		
}

void
sp_desktop_activate_guides (SPDesktop * desktop, gboolean activate)
{
	desktop->guides_active = activate;
	sp_namedview_activate_guides (desktop->namedview, desktop, activate);
}

void
sp_desktop_change_document (SPDesktop * desktop, SPDocument * document)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));

	sp_item_hide (SP_ITEM (sp_document_root (desktop->document)), desktop->canvas);

	gtk_object_ref (GTK_OBJECT (document));
	gtk_object_unref (GTK_OBJECT (desktop->document));

	desktop->document = document;

	sp_item_show (SP_ITEM (sp_document_root (desktop->document)), desktop->drawing, sp_desktop_item_handler);
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
sp_desktop_update_rulers (GtkWidget * widget, SPDesktop * desktop)
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

	//	sp_desktop_update_rulers (NULL, desktop);
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

	w = fabs (x1 - x0) +10;
	h = fabs (y1 - y0) +10;

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
	zoom_any_update (scale);
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

      	sp_desktop_update_rulers (NULL, desktop);
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

      	sp_desktop_update_rulers (NULL, desktop);

	zoom_any_update (zoom);
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

	sp_desktop_update_rulers (NULL, desktop);
}

/* fixme: this are UI functions - find a better place for them */

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

#endif


void sp_desktop_set_status (SPDesktop *desktop, const gchar * text)
{
#if 0
	if (!sodipodi->active_window) return;
	if (!sodipodi->active_window->statusbar) return;

	gnome_appbar_set_status (GNOME_APPBAR (sodipodi->active_window->statusbar), text);
#endif
}

/*
 * updates the zoom toolbox
 *
 * fixme to update allways correctly !
 */

void
zoom_any_update (gdouble any) {
  GString * str = NULL;
  gint * pos, p0 = 0;
  gint iany;
  gchar * format = "%d%c";
  
  iany = (gint)(any*100);
  str = g_string_new("");
  g_print("\n    %d  to  ",iany);
  g_string_sprintf (str,format,iany,'%');
  pos = &p0;

  g_print(str->str);
  /*
  gtk_editable_delete_text ((GtkEditable *) zoom_any, 0 ,-1);  
  gtk_editable_insert_text ((GtkEditable *) zoom_any, str->str, str->len, pos);
  */
}










