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

#define noDESKTOP_VERBOSE

#include <math.h>
#include <gnome.h>
#include <glade/glade.h>
#include "helper/canvas-helper.h"
#include "forward.h"
#include "sodipodi-private.h"
#include "desktop.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "document.h"
#include "selection.h"
#include "select-context.h"
#include "event-context.h"
#include "sp-item.h"
#include "sp-ruler.h"

#include <gal/widgets/gtk-combo-text.h>
#include <gal/widgets/gtk-combo-stack.h>

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_destroy (GtkObject * object);

static void sp_desktop_realize (GtkWidget * widget);

static void sp_desktop_update_rulers (GtkWidget * widget, SPDesktop * desktop);
static void sp_desktop_resized (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop);
static void sp_desktop_update_scrollbars (Sodipodi * sodipodi, SPSelection * selection);
static void sp_desktop_set_viewport (SPDesktop * desktop, double x, double y);
static void sp_desktop_zoom_update (SPDesktop * desktop);

static gint select_set_id =0;

void sp_desktop_indicator_on (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);
void sp_desktop_indicator_off (Sodipodi * sodipodi, SPDesktop * desktop, gpointer data);
void sp_desktop_zoom (GtkEntry * caller,SPDesktop * desktop);

/* fixme: */
void sp_desktop_toggle_borders (GtkWidget * widget);

static void sp_desktop_menu_popup (GtkWidget * widget, GdkEventButton * event);

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

	widget_class->realize = sp_desktop_realize;
#if 0
      	widget_class->enter_notify_event = sp_desktop_enter_notify;

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
	GtkWidget * hbox, * widget, * zoom, * entry; 
	GtkWidget * eventbox;
	GtkTable * table;
	//	GList * zoom_list = NULL;

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

	desktop->table = GTK_BOX (gtk_vbox_new (FALSE,0));
	gtk_widget_show (GTK_WIDGET (desktop->table));
	gtk_box_set_spacing (GTK_BOX (desktop->table), 0);
	gtk_container_add (GTK_CONTAINER (desktop), GTK_WIDGET (desktop->table));

	table = GTK_TABLE (gtk_table_new (3, 3, FALSE));
	gtk_widget_show (GTK_WIDGET (table));
        gtk_box_pack_start (GTK_BOX (desktop->table), 
				GTK_WIDGET (table),
				TRUE,
				TRUE,
       				0);

	/* Horizontal scrollbar */
	desktop->hscrollbar = GTK_SCROLLBAR (gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->hscrollbar));
	gtk_table_attach (table,
		GTK_WIDGET (desktop->hscrollbar),
		1,2,2,3,
		GTK_EXPAND | GTK_FILL,
		GTK_FILL,
		0,1);
	/* Vertical scrollbar */
	desktop->vscrollbar = GTK_SCROLLBAR (gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (4000.0,
		0.0, 8000.0, 10.0, 100.0, 4.0))));
	gtk_widget_show (GTK_WIDGET (desktop->vscrollbar));
	gtk_table_attach (table,
		GTK_WIDGET (desktop->vscrollbar),
		2,3,1,2,
		GTK_FILL,
		GTK_EXPAND | GTK_FILL,
		1,0);
	/* Canvas */
	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());
	desktop->canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();
	gtk_widget_show (GTK_WIDGET (desktop->canvas));
	widget = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (widget), GTK_SHADOW_IN);
	gtk_widget_show (widget);
	gtk_table_attach (table,
		GTK_WIDGET (widget),
		1,2,1,2,
		GTK_EXPAND | GTK_FILL,
		GTK_EXPAND | GTK_FILL,
		0,0);
      	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (desktop->canvas));
	/* Horizonatl ruler */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	desktop->hruler = GTK_RULER (sp_hruler_new ());
	sp_ruler_set_metric (desktop->hruler, SP_DEFAULT_METRIC);
	gtk_widget_show (GTK_WIDGET (desktop->hruler));
	gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (desktop->hruler));
	gtk_table_attach (table,
		eventbox,
		1,2,0,1,
		GTK_FILL,
		GTK_FILL,
		widget->style->klass->xthickness,0);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_release_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "motion_notify_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	/* Vertical ruler */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	desktop->vruler = GTK_RULER (sp_vruler_new ());
	sp_ruler_set_metric (desktop->vruler, SP_DEFAULT_METRIC);
	gtk_widget_show (GTK_WIDGET (desktop->vruler));
	gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (desktop->vruler));
	gtk_table_attach (table,
		eventbox,
		0,1,1,2,
		GTK_FILL,
		GTK_FILL,
		0,widget->style->klass->ythickness);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_release_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "motion_notify_event",
			    GTK_SIGNAL_FUNC (sp_dt_vruler_event), desktop);
	/* menu button */
	desktop->menubutton = gtk_button_new ();
      	gtk_button_set_relief (GTK_BUTTON(desktop->menubutton), GTK_RELIEF_NONE);
	gtk_widget_show (desktop->menubutton);
	gtk_table_attach (table,
		GTK_WIDGET (desktop->menubutton),
		0,1,0,1,
		GTK_FILL,
		GTK_FILL,
		0,0);
	menu_arrow = gtk_arrow_new (GTK_ARROW_RIGHT,GTK_SHADOW_IN);
	gtk_widget_show (menu_arrow);
	gtk_container_add ((GtkContainer *)desktop->menubutton, menu_arrow);
	gtk_signal_connect (GTK_OBJECT (desktop-> menubutton), 
			    "button_press_event", 
			    GTK_SIGNAL_FUNC (sp_desktop_menu_popup), 
			    NULL);
        GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET(desktop->menubutton), GTK_CAN_FOCUS);
	/* indicator for active desktop */
	hbox = gtk_vbox_new (FALSE, 0);
	gtk_table_attach (table,
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
       // status bars
       	hbox = gtk_hbox_new (FALSE,0);
	gtk_box_set_spacing (GTK_BOX (hbox), 2);
       	gtk_box_pack_start (GTK_BOX (desktop->table),
       	                      GTK_WIDGET (hbox),
                              FALSE,
       	                      FALSE,
                              0);
        gtk_widget_show (hbox);

        desktop->coord_status = GNOME_APPBAR(gnome_appbar_new (FALSE, TRUE,GNOME_PREFERENCES_NEVER));
	gtk_misc_set_alignment (GTK_MISC (desktop->coord_status->status), 0.5, 0.5);
	gtk_widget_show (GTK_WIDGET(desktop->coord_status));	
	gtk_box_pack_start (GTK_BOX (hbox),
			    GTK_WIDGET(desktop->coord_status),
			    FALSE,
			    TRUE,
			    0);
	gtk_widget_set_usize (GTK_WIDGET (desktop->coord_status),120,0);

        desktop->select_status = GNOME_APPBAR(gnome_appbar_new (FALSE, TRUE,GNOME_PREFERENCES_NEVER));
        gtk_misc_set_alignment (GTK_MISC (desktop->select_status->status), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (desktop->select_status->status), 5, 0);
        //gtk_label_set_line_wrap (GTK_LABEL (desktop->select_status->label), TRUE);
        gtk_widget_show (GTK_WIDGET (desktop->select_status));
        gtk_box_pack_start (GTK_BOX (hbox),
                            GTK_WIDGET (desktop->select_status),
                            TRUE,
                            TRUE,
                            0);

	//desktop->coord_status_id = gtk_statusbar_get_context_id (desktop->coord_status, "mouse coordinates");
	//desktop->select_status_id = gtk_statusbar_get_context_id (desktop->coord_status, "selection stuff");
        gnome_appbar_set_status (desktop->select_status, "Wellcome !");

        // zoom combo
        zoom = desktop->zoom = gtk_combo_text_new (FALSE);
        if (!gnome_preferences_get_toolbar_relief_btn ())
                gtk_combo_box_set_arrow_relief (GTK_COMBO_BOX (zoom), GTK_RELIEF_NONE);
        entry = GTK_COMBO_TEXT (zoom)->entry;
        gtk_signal_connect (GTK_OBJECT (entry), "activate", GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop);
        gtk_combo_box_set_title (GTK_COMBO_BOX (zoom), _("Zoom"));
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "25%", "25%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "50%", "50%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "75%", "75%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "100%", "100%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "150%", "150%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "200%", "200%");
        gtk_combo_text_add_item(GTK_COMBO_TEXT (zoom), "400%", "400%");
        gtk_widget_set_usize (GTK_WIDGET (zoom),57,0);
        gtk_widget_show (GTK_WIDGET (zoom));
        gtk_box_pack_end (GTK_BOX (hbox),
                            GTK_WIDGET (zoom),
                            FALSE,
                            TRUE,
                          0);

        gtk_widget_grab_focus (GTK_WIDGET (desktop->canvas));
}

static void
sp_desktop_destroy (GtkObject * object)
{
	SPDesktop * desktop;

	desktop = SP_DESKTOP (object);

	sodipodi_remove_desktop (desktop);

	if (desktop->event_context)
		gtk_object_unref (GTK_OBJECT (desktop->event_context));

	if (desktop->selection)
		gtk_object_unref (GTK_OBJECT (desktop->selection));

	if (desktop->document) {
		if (desktop->canvas) {
			sp_namedview_hide (desktop->namedview, desktop);
			sp_item_hide (SP_ITEM (sp_document_root (desktop->document)), desktop);
		}
		gtk_object_unref (GTK_OBJECT (desktop->document));
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_desktop_realize (GtkWidget * widget)
{
	SPDesktop * dt;
	ArtDRect d;

	dt = SP_DESKTOP (widget);

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	d.x0 = 0.0;
	d.y0 = 0.0;
	d.x1 = sp_document_width (dt->document);
	d.y1 = sp_document_height (dt->document);

	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;

	sp_desktop_show_region (dt, d.x0, d.y0, d.x1, d.y1, 10);
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

	// connecting canvas, scrollbars, rulers, statusbar
	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);
	gtk_signal_connect (GTK_OBJECT (hadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (vadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (desktop->canvas), "size-allocate", GTK_SIGNAL_FUNC (sp_desktop_resized), desktop);
	if (select_set_id<1) select_set_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "change_selection", 
								 GTK_SIGNAL_FUNC (sp_desktop_update_scrollbars),NULL);
	/* fixme!
	 * i wonder why this doesn't work with canvas
	 * mouse coordinate handler should be called before all other handlers */
	gtk_signal_connect (GTK_OBJECT (gnome_canvas_root (desktop->canvas)), "event",
			    GTK_SIGNAL_FUNC (sp_canvas_root_handler), desktop);

	ci = sp_item_show (SP_ITEM (sp_document_root (desktop->document)), desktop, desktop->drawing);

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
		gtk_widget_show (GTK_WIDGET (desktop->zoom)->parent);		
	} else {
		gtk_widget_hide (GTK_WIDGET (desktop->hscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->vscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->hruler));
		gtk_widget_hide (GTK_WIDGET (desktop->vruler));
		gtk_widget_hide (GTK_WIDGET (desktop->menubutton));
		gtk_widget_hide (GTK_WIDGET (desktop->active));
		gtk_widget_hide (GTK_WIDGET (desktop->inactive));
		gtk_widget_hide (GTK_WIDGET (desktop->zoom)->parent);		
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

	sp_namedview_hide (desktop->namedview, desktop);
	sp_item_hide (SP_ITEM (sp_document_root (desktop->document)), desktop);

	gtk_object_ref (GTK_OBJECT (document));
	gtk_object_unref (GTK_OBJECT (desktop->document));

	desktop->document = document;
	desktop->namedview = sp_document_namedview (document, NULL);

	sp_item_show (SP_ITEM (sp_document_root (desktop->document)), desktop, desktop->drawing);
	sp_namedview_show (desktop->namedview, desktop);
	/* Ugly hack */
	sp_desktop_activate_guides (desktop, TRUE);
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

// redraw the scrollbars in the desktop window
static 
void sp_desktop_update_scrollbars (Sodipodi * sodipodi, SPSelection * selection)
{
  ArtPoint p0, p1;
  SPItem * docitem;
  ArtDRect d;
  gdouble dw, dh, zf, cw, ch;
  GtkAdjustment * vadj, * hadj;
  SPDesktop * desktop;

  if (selection == NULL) {
    if (select_set_id>0) {
      gtk_signal_disconnect (GTK_OBJECT (sodipodi), select_set_id);
      select_set_id = 0;
    }
    return;
  }

  g_print ("update scrollbars\n");
  g_return_if_fail (SP_IS_SELECTION (selection));
  desktop = selection->desktop;
  g_return_if_fail (SP_IS_DESKTOP (desktop));
  docitem = SP_ITEM (sp_document_root (desktop->document));
  g_return_if_fail (docitem != NULL);

  hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
  vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));

  zf = sp_desktop_zoom_factor (desktop);
  cw = GTK_WIDGET (desktop->canvas)->allocation.width;
  ch = GTK_WIDGET (desktop->canvas)->allocation.height;
  // drawing
  sp_item_bbox (docitem, &d);
  // add document
  if (d.x0 > 0) d.x0 = 0;
  if (d.y0 > 0) d.y0 = 0;
  dw = sp_document_width (desktop->document);
  dh = sp_document_height (desktop->document);
  if (d.x1 < dw) d.x1 = dw;
  if (d.y1 < dh) d.y1 = dh;
  // add border around drawing 
  d.x0 -= cw/zf;
  d.x1 += cw/zf;
  d.y0 -= ch/zf;
  d.y1 += ch/zf;
  // enlarge to fit desktop 
  gnome_canvas_window_to_world (desktop->canvas, 0.0, 0.0, &p0.x, &p0.y);
  gnome_canvas_window_to_world (desktop->canvas, cw, ch, &p1.x, &p1.y);
  art_affine_point (&p0, &p0, desktop->w2d);
  art_affine_point (&p1, &p1, desktop->w2d);
  if (d.x0 < p0.x) p0.x = d.x0;
  if (d.y0 < p0.y) p0.y = d.y0;
  if (d.x1 > p1.x) p1.x = d.x1;
  if (d.y1 > p1.y) p1.y = d.y1;
  art_affine_point (&p0, &p0, desktop->d2w);
  art_affine_point (&p1, &p1, desktop->d2w);
  gnome_canvas_world_to_window (desktop->canvas, p0.x, p0.y, &d.x0, &d.y0);
  gnome_canvas_world_to_window (desktop->canvas, p1.x, p1.y, &d.x1, &d.y1);
  // set new adjustment  
  hadj->lower = p0.x + SP_DESKTOP_SCROLL_LIMIT;
  hadj->upper = p1.x + SP_DESKTOP_SCROLL_LIMIT;
  vadj->lower = p1.y + SP_DESKTOP_SCROLL_LIMIT;
  vadj->upper = p0.y + SP_DESKTOP_SCROLL_LIMIT;
  //restrict to canvas scroll limit
  if (hadj->lower < 0) hadj->lower = 0;
  if (hadj->upper > 2*SP_DESKTOP_SCROLL_LIMIT) hadj->upper = 2*SP_DESKTOP_SCROLL_LIMIT;
  if (vadj->lower < 0) vadj->lower = 0;
  if (vadj->upper > 2*SP_DESKTOP_SCROLL_LIMIT) vadj->upper = 2*SP_DESKTOP_SCROLL_LIMIT;

  gtk_adjustment_changed (hadj);
  gtk_adjustment_changed (vadj);
}


// is called when the cavas is resized, connected in sp_desktop_new
static 
void sp_desktop_resized (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop) 
{
  sp_desktop_update_rulers (widget, desktop);
  sp_desktop_update_scrollbars (SODIPODI, SP_DT_SELECTION (desktop));
}


// update the zoomfactor in the combobox of the desktop window
static 
void sp_desktop_zoom_update (SPDesktop * desktop)
{
  GString * str = NULL;
  gint * pos, p0 = 0;
  gint iany;
  gchar * format = "%d%c";
  gdouble zf;

  zf = sp_desktop_zoom_factor (desktop);
  //g_print ("new zoom: %f\n",zf);
  iany = (gint)(zf*100+0.5);
  str = g_string_new("");

  g_string_sprintf (str,format,iany,'%');
  pos = &p0;

  gtk_signal_handler_block_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->zoom)->entry),
                                    GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop);
  gtk_combo_text_set_text (GTK_COMBO_TEXT (desktop->zoom), str->str);
  gtk_signal_handler_unblock_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->zoom)->entry),
                                      GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop);

  g_string_free(str, TRUE);
}


// read and set the zoom factor from the combo box in the desktop window
void 
sp_desktop_zoom (GtkEntry * caller,SPDesktop * desktop) {
  gchar * zoom_str;
  ArtDRect d;
  gdouble any;

  g_return_if_fail (SP_IS_DESKTOP (desktop));

  zoom_str = gtk_entry_get_text (GTK_ENTRY(caller));

  //  zoom_str[5] = '\0';
  any = strtod(zoom_str,NULL) /100;
  if (any < SP_DESKTOP_ZOOM_MIN/2) return;
  
  sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);
  sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, any, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2);
  // give focus back to canvas
  gtk_widget_grab_focus (GTK_WIDGET (desktop->canvas));
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

	cw = GTK_WIDGET (desktop->canvas)->allocation.width+1;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height+1;

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


gdouble sp_desktop_zoom_factor (SPDesktop * desktop)
{
  g_return_val_if_fail (SP_IS_DESKTOP (desktop), 0.0);

  return sqrt (fabs(desktop->d2w[0] * desktop->d2w[3]));
}

void
sp_desktop_show_region (SPDesktop * desktop, gdouble x0, gdouble y0, gdouble x1, gdouble y1, gint border)
{
	ArtPoint c, p;
	double w, h, cw, ch, sw, sh, scale;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	c.x = (x0 + x1) / 2;
	c.y = (y0 + y1) / 2;

	w = fabs (x1 - x0);
	h = fabs (y1 - y0);

	cw = GTK_WIDGET (desktop->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->canvas)->allocation.height;

	sw = (cw-2*border) / w;
	sh = (ch-2*border) / h;

	scale = MIN (sw, sh);
        if (scale < SP_DESKTOP_ZOOM_MIN) scale = SP_DESKTOP_ZOOM_MIN;
        if (scale > SP_DESKTOP_ZOOM_MAX) scale = SP_DESKTOP_ZOOM_MAX;
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
       	sp_desktop_zoom_update (desktop);
}

void
sp_desktop_zoom_relative (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy)
{
        gdouble scale;

	scale = sp_desktop_zoom_factor (desktop) * zoom;

        if (scale < SP_DESKTOP_ZOOM_MIN) scale = SP_DESKTOP_ZOOM_MIN;
        if (scale > SP_DESKTOP_ZOOM_MAX) scale = SP_DESKTOP_ZOOM_MAX;

	art_affine_scale (desktop->d2w, scale, -scale);

        /*
        desktop->d2w[0] *= zoom;
        desktop->d2w[1] *= zoom;
        desktop->d2w[2] *= zoom;
        desktop->d2w[3] *= zoom;
        */
        desktop->d2w[4] = -cx * desktop->d2w[0];
        desktop->d2w[5] = -cy * desktop->d2w[3];
        art_affine_invert (desktop->w2d, desktop->d2w);

	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);
	sp_desktop_set_viewport (desktop, SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT);
	sp_desktop_zoom_update (desktop);
}

void
sp_desktop_zoom_absolute (SPDesktop * desktop, gdouble zoom, gdouble cx, gdouble cy)
{
        if (zoom < SP_DESKTOP_ZOOM_MIN) zoom = SP_DESKTOP_ZOOM_MIN;
        if (zoom > SP_DESKTOP_ZOOM_MAX) zoom = SP_DESKTOP_ZOOM_MAX;

	desktop->d2w[0] = zoom;
	desktop->d2w[1] = 0.0;
	desktop->d2w[2] = 0.0;
	desktop->d2w[3] = -zoom;

	desktop->d2w[4] = -cx * desktop->d2w[0];
	desktop->d2w[5] = -cy * desktop->d2w[3];
	art_affine_invert (desktop->w2d, desktop->d2w);
	
	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);
      	sp_desktop_set_viewport (desktop, SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT);
	sp_desktop_zoom_update (desktop);
}

/* Context switching */

void
sp_desktop_set_event_context (SPDesktop * desktop, GtkType type)
{
	if (desktop->event_context)
		gtk_object_unref (GTK_OBJECT (desktop->event_context));

	desktop->event_context = sp_event_context_new (desktop, type);
}

/* Private helpers */

/*
 * Set viewport center to x,y in world coordinates
 */

static void
sp_desktop_set_viewport (SPDesktop * desktop, double x, double y)
{
    gdouble cw, ch;
    
    cw = GTK_WIDGET (desktop->canvas)->allocation.width + 1;
    ch = GTK_WIDGET (desktop->canvas)->allocation.height + 1;
    gnome_canvas_scroll_to (desktop->canvas, x - cw / 2, y - ch / 2);

    sp_desktop_update_rulers (NULL, desktop);
    sp_desktop_update_scrollbars (SODIPODI, SP_DT_SELECTION (desktop));
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


// set the select status bar 
void 
sp_desktop_set_status (SPDesktop *desktop, const gchar * stat)
{
  gint b =0;
  GString * text;

  g_return_if_fail (SP_IS_DESKTOP (desktop));

  text = g_string_new(stat);
  // remove newlines 
  for (b=0; text->str[b]!=0; b+=1) if (text->str[b]=='\n') text->str[b]=' ';
  gnome_appbar_set_status (desktop->select_status, text->str);
  g_string_free(text,FALSE);
}


// we make the desktop window with focus active, signal is connected in interface.c
gint
sp_desktop_set_focus (GtkWidget *widget, GtkWidget *widget2, SPDesktop * desktop)
{
  sodipodi_activate_desktop (desktop);
  // give focus to canvas widget
  gtk_widget_grab_focus (GTK_WIDGET (desktop->canvas));
  return FALSE;
}
 
static void
sp_desktop_menu_popup (GtkWidget * widget, GdkEventButton * event)
{
	sp_event_root_menu_popup (widget, NULL, event);
}

void
sp_desktop_connect_item (SPDesktop * desktop, SPItem * item, GnomeCanvasItem * canvasitem)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));
	g_assert (item != NULL);
	g_assert (SP_IS_ITEM (item));
	g_assert (canvasitem != NULL);
	g_assert (GNOME_IS_CANVAS_ITEM (canvasitem));

	gtk_signal_connect (GTK_OBJECT (canvasitem), "event",
			    GTK_SIGNAL_FUNC (sp_desktop_item_handler), item);
}

