#define __SP_DESKTOP_C__

/*
 * Editable view and widget implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noDESKTOP_VERBOSE

#include <config.h>

#include <math.h>

#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gtk/gtk.h>

#include "helper/sp-intl.h"
#include "helper/sp-marshal.h"
#include "helper/gnome-canvas-acetate.h"
#include "helper/sodipodi-ctrlrect.h"
#include "helper/units.h"
#include "helper/sp-intl.h"
#include "display/canvas-arena.h"
#include "forward.h"
#include "sodipodi-private.h"
#include "desktop.h"
#include "desktop-events.h"
#include "desktop-affine.h"
#include "document.h"
#include "selection.h"
#include "select-context.h"
#include "event-context.h"
#include "sp-namedview.h"
#include "sp-item.h"
#include "sp-root.h"
#include "sp-ruler.h"

/* fixme: Lauris */
#include "file.h"

#define SP_CANVAS_STICKY_FLAG (1 << 16)

enum {
	ACTIVATE,
	DESACTIVATE,
	MODIFIED,
	LAST_SIGNAL
};

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_dispose (GObject *object);

static void sp_desktop_request_redraw (SPView *view);
static void sp_desktop_set_document (SPView *view, SPDocument *doc);
static void sp_desktop_document_resized (SPView *view, SPDocument *doc, gdouble width, gdouble height);

/* Constructor */

static SPView *sp_desktop_new (SPNamedView *nv, SPCanvas *canvas);

static void sp_desktop_prepare_shutdown (SPDesktop *dt);

static void sp_dt_namedview_modified (SPNamedView *nv, guint flags, SPDesktop *desktop);
static void sp_desktop_selection_modified (SPSelection *selection, guint flags, SPDesktop *desktop);

static void sp_dt_update_snap_distances (SPDesktop *desktop);

static void sp_desktop_size_allocate (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop);
static void sp_desktop_zoom_update (SPDesktop * desktop);

void sp_desktop_zoom (GtkEntry *caller, SPDesktopWidget *dtw);

static gint sp_desktop_menu_popup (GtkWidget * widget, GdkEventButton * event, gpointer data);

/* fixme: This is here, but shouldn't in theory (Lauris) */
static void sp_desktop_widget_update_rulers (SPDesktopWidget *dtw);
static void sp_desktop_update_scrollbars (SPDesktop *desktop);

SPViewClass * parent_class;
static guint signals[LAST_SIGNAL] = { 0 };

unsigned int
sp_desktop_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPDesktopClass),
			NULL, NULL,
			(GClassInitFunc) sp_desktop_class_init,
			NULL, NULL,
			sizeof (SPDesktop),
			4,
			(GInstanceInitFunc) sp_desktop_init,
		};
		type = g_type_register_static (SP_TYPE_VIEW, "SPDesktop", &info, 0);
	}
	return type;
}

static void
sp_desktop_class_init (SPDesktopClass *klass)
{
	GObjectClass *object_class;
	SPViewClass *view_class;

	object_class = G_OBJECT_CLASS (klass);
	view_class = (SPViewClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	signals[ACTIVATE] = g_signal_new ("activate",
					  G_TYPE_FROM_CLASS(klass),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (SPDesktopClass, activate),
					  NULL, NULL,
					  sp_marshal_NONE__NONE,
					  G_TYPE_NONE, 0);
	signals[DESACTIVATE] = g_signal_new ("desactivate",
					  G_TYPE_FROM_CLASS(klass),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (SPDesktopClass, desactivate),
					  NULL, NULL,
					  sp_marshal_NONE__NONE,
					  G_TYPE_NONE, 0);
	signals[MODIFIED] = g_signal_new ("modified",
					  G_TYPE_FROM_CLASS(klass),
					  G_SIGNAL_RUN_FIRST,
					  G_STRUCT_OFFSET (SPDesktopClass, modified),
					  NULL, NULL,
					  sp_marshal_NONE__UINT,
					  G_TYPE_NONE, 1,
					  G_TYPE_UINT);

	object_class->dispose = sp_desktop_dispose;

	view_class->request_redraw = sp_desktop_request_redraw;
	view_class->set_document = sp_desktop_set_document;
	view_class->document_resized = sp_desktop_document_resized;
}

static void
sp_desktop_init (SPDesktop *desktop)
{
	desktop->namedview = NULL;
	desktop->selection = NULL;
	desktop->event_context = NULL;
	desktop->acetate = NULL;
	desktop->main = NULL;
	desktop->grid = NULL;
	desktop->guides = NULL;
	desktop->drawing = NULL;
	desktop->sketch = NULL;
	desktop->controls = NULL;

	art_affine_identity (desktop->d2w);
	art_affine_identity (desktop->w2d);
	art_affine_scale (desktop->doc2dt, 0.8, -0.8);

	desktop->guides_active = FALSE;
}

static void
sp_desktop_dispose (GObject *object)
{
	SPDesktop *dt;

	dt = SP_DESKTOP (object);

	if (dt->sodipodi) {
		sodipodi_remove_desktop (dt);
		dt->sodipodi = NULL;
	}

	if (dt->event_context) {
		sp_event_context_finish (dt->event_context);
		g_object_unref (G_OBJECT (dt->event_context));
		dt->event_context = NULL;
	}

	if (dt->selection) {
		g_object_unref (G_OBJECT (dt->selection));
		dt->selection = NULL;
	}

	if (dt->drawing) {
		sp_namedview_hide (dt->namedview, dt);
		sp_item_hide (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (dt))), SP_CANVAS_ARENA (dt->drawing)->arena);
		dt->drawing = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sp_desktop_request_redraw (SPView *view)
{
	SPDesktop *dt;

	dt = SP_DESKTOP (view);

	if (dt->main) {
		gtk_widget_queue_draw (GTK_WIDGET (SP_CANVAS_ITEM (dt->main)->canvas));
	}
}

static void
sp_desktop_document_resized (SPView *view, SPDocument *doc, gdouble width, gdouble height)
{
	SPDesktop *desktop;

	desktop = SP_DESKTOP (view);

	desktop->doc2dt[5] = height;

	sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (desktop->drawing), desktop->doc2dt);

	sp_ctrlrect_set_area (SP_CTRLRECT (desktop->page), 0.0, 0.0, width, height);
}

void
sp_desktop_set_active (SPDesktop *desktop, gboolean active)
{
	if (active != desktop->active) {
		desktop->active = active;
		if (active) {
			g_signal_emit (G_OBJECT (desktop), signals[ACTIVATE], 0);
		} else {
			g_signal_emit (G_OBJECT (desktop), signals[DESACTIVATE], 0);
		}
	}
}

/* fixme: */

static gint
arena_handler (SPCanvasArena *arena, NRArenaItem *item, GdkEvent *event, SPDesktop *desktop)
{
	if (item) {
		SPItem *spi;
		spi = g_object_get_data (G_OBJECT (item), "sp-item");
		sp_event_context_item_handler (desktop->event_context, spi, event);
	} else {
		sp_event_context_root_handler (desktop->event_context, event);
	}

	return TRUE;
}

/* Constructor */

static SPView *
sp_desktop_new (SPNamedView *namedview, SPCanvas *canvas)
{
	SPDesktop *desktop;
	SPCanvasGroup *root, *page;
	NRArenaItem *ai;
	gdouble dw, dh;
	SPDocument *document;

	document = SP_OBJECT_DOCUMENT (namedview);

	/* Setup widget */

	desktop = (SPDesktop *) g_object_new (SP_TYPE_DESKTOP, NULL);

	/* Connect document */
	sp_view_set_document (SP_VIEW (desktop), document);

	desktop->namedview = namedview;
	g_signal_connect (G_OBJECT (namedview), "modified", G_CALLBACK (sp_dt_namedview_modified), desktop);
	desktop->number = sp_namedview_viewcount (namedview);

	/* Setup Canvas */
	g_object_set_data (G_OBJECT (canvas), "SPDesktop", desktop);

	root = sp_canvas_root (canvas);

	desktop->acetate = sp_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	g_signal_connect (G_OBJECT (desktop->acetate), "event", G_CALLBACK (sp_desktop_root_handler), desktop);
	/* Setup adminstrative layers */
	desktop->main = (SPCanvasGroup *) sp_canvas_item_new (root, SP_TYPE_CANVAS_GROUP, NULL);
	g_signal_connect (G_OBJECT (desktop->main), "event", G_CALLBACK (sp_desktop_root_handler), desktop);
	/* fixme: */
	page = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_GROUP, NULL);

	desktop->drawing = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_ARENA, NULL);
	g_signal_connect (G_OBJECT (desktop->drawing), "arena_event", G_CALLBACK (arena_handler), desktop);

	desktop->grid = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_GROUP, NULL);
	desktop->guides = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_GROUP, NULL);
	desktop->sketch = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_GROUP, NULL);
	desktop->controls = (SPCanvasGroup *) sp_canvas_item_new (desktop->main, SP_TYPE_CANVAS_GROUP, NULL);

	desktop->selection = sp_selection_new (desktop);

#if 0
	desktop->event_context = sp_event_context_new (SP_TYPE_SELECT_CONTEXT, desktop, NULL);
#else
	/* fixme: I think it is safe, but is it nice? (Lauris) */
	sp_desktop_set_event_context (desktop, SP_TYPE_SELECT_CONTEXT, "tools.select");
#endif

	/* fixme: Setup display rectangle */
	dw = sp_document_width (document);
	dh = sp_document_height (document);

	desktop->page = sp_canvas_item_new (page, SP_TYPE_CTRLRECT, NULL);
	sp_ctrlrect_set_area (SP_CTRLRECT (desktop->page), 0.0, 0.0, sp_document_width (document), sp_document_height (document));
	sp_ctrlrect_set_shadow (SP_CTRLRECT (desktop->page), 5, 0x3f3f3fff);

	/* Connect event for page resize */
	desktop->doc2dt[5] = sp_document_height (document);
	sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (desktop->drawing), desktop->doc2dt);

	/* Fixme: Setup initial zooming */
	art_affine_scale (desktop->d2w, 1.0, -1.0);
	desktop->d2w[5] = dh;
	art_affine_invert (desktop->w2d, desktop->d2w);
	sp_canvas_item_affine_absolute ((SPCanvasItem *) desktop->main, desktop->d2w);

#if 0
	sp_canvas_set_scroll_region (canvas, -SP_DESKTOP_SCROLL_LIMIT, -SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT, SP_DESKTOP_SCROLL_LIMIT);
#endif

	g_signal_connect (G_OBJECT (canvas), "size-allocate", G_CALLBACK (sp_desktop_size_allocate), desktop);
	g_signal_connect (G_OBJECT (desktop->selection), "modified", G_CALLBACK (sp_desktop_selection_modified), desktop);

	ai = sp_item_show (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (desktop))), SP_CANVAS_ARENA (desktop->drawing)->arena);
	if (ai) {
		nr_arena_item_add_child (SP_CANVAS_ARENA (desktop->drawing)->root, ai, NULL);
		g_object_unref (G_OBJECT(ai));
	}

	sp_namedview_show (desktop->namedview, desktop);
	/* Ugly hack */
	sp_desktop_activate_guides (desktop, TRUE);

	// ?
	// sp_active_desktop_set (desktop);
	sodipodi_add_desktop (desktop);
	desktop->sodipodi = SODIPODI;

	return SP_VIEW (desktop);
}

static void
sp_desktop_prepare_shutdown (SPDesktop *dt)
{
	if (dt->sodipodi) {
		sodipodi_remove_desktop (dt);
		dt->sodipodi = NULL;
	}

	if (dt->event_context) {
		sp_event_context_finish (dt->event_context);
		g_object_unref (G_OBJECT (dt->event_context));
		dt->event_context = NULL;
	}

	if (dt->selection) {
		g_object_unref (G_OBJECT (dt->selection));
		dt->selection = NULL;
	}

	if (dt->drawing) {
		sp_namedview_hide (dt->namedview, dt);
		sp_item_hide (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (dt))), SP_CANVAS_ARENA (dt->drawing)->arena);
		dt->drawing = NULL;
	}
}

static void
sp_dt_namedview_modified (SPNamedView *nv, guint flags, SPDesktop *desktop)
{
	if (flags && SP_OBJECT_MODIFIED_FLAG) {
		/* Recalculate snap distances */
		sp_dt_update_snap_distances (desktop);
		/* Show/hide page border */
		if (nv->showborder) {
			sp_canvas_item_show (desktop->page);
		} else {
			sp_canvas_item_hide (desktop->page);
		}
	}
}

static void
sp_dt_update_snap_distances (SPDesktop *desktop)
{
	static const SPUnit *px = NULL;
	gdouble px2doc;

	if (!px) px = sp_unit_get_by_abbreviation ("px");

	px2doc = sqrt (fabs (desktop->w2d[0] * desktop->w2d[3]));
	desktop->gridsnap = (desktop->namedview->snaptogrid) ? desktop->namedview->gridtolerance : 0.0;
	sp_convert_distance_full (&desktop->gridsnap, desktop->namedview->gridtoleranceunit, px, 1.0, px2doc);
	desktop->guidesnap = (desktop->namedview->snaptoguides) ? desktop->namedview->guidetolerance : 0.0;
	sp_convert_distance_full (&desktop->guidesnap, desktop->namedview->guidetoleranceunit, px, 1.0, px2doc);
	g_print ("Grid %g guides %g\n", desktop->gridsnap, desktop->guidesnap);
}

void
sp_desktop_activate_guides (SPDesktop * desktop, gboolean activate)
{
	desktop->guides_active = activate;
	sp_namedview_activate_guides (desktop->namedview, desktop, activate);
}

static void
sp_desktop_set_document (SPView *view, SPDocument *doc)
{
	SPDesktop *desktop;

	desktop = SP_DESKTOP (view);

	if (view->doc) {
		sp_namedview_hide (desktop->namedview, desktop);
		sp_item_hide (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (desktop))), SP_CANVAS_ARENA (desktop->drawing)->arena);
	}

	/* fixme: */
	if (desktop->drawing) {
		NRArenaItem *ai;

		desktop->namedview = sp_document_namedview (doc, NULL);

		ai = sp_item_show (SP_ITEM (sp_document_root (doc)), SP_CANVAS_ARENA (desktop->drawing)->arena);
		if (ai) {
			nr_arena_item_add_child (SP_CANVAS_ARENA (desktop->drawing)->root, ai, NULL);
			g_object_unref (G_OBJECT(ai));
		}
		sp_namedview_show (desktop->namedview, desktop);
		/* Ugly hack */
		sp_desktop_activate_guides (desktop, TRUE);
	}
}

void
sp_desktop_change_document (SPDesktop *desktop, SPDocument *document)
{
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	sp_view_set_document (SP_VIEW (desktop), document);
}

/* Private methods */

static void
sp_desktop_selection_modified (SPSelection *selection, guint flags, SPDesktop *desktop)
{
	sp_desktop_update_scrollbars (desktop);
}

// is called when the cavas is resized, connected in sp_desktop_new
static void
sp_desktop_size_allocate (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop) 
{
	sp_desktop_widget_update_rulers (desktop->owner);
	sp_desktop_update_scrollbars (desktop);
}


// update the zoomfactor in the combobox of the desktop window
static 
void sp_desktop_zoom_update (SPDesktop *desktop)
{
	GString * str = NULL;
	gint * pos, p0 = 0;
	gint iany;
	gchar * format = "%d%c";
	gdouble zf;
	
	zf = SP_DESKTOP_ZOOM (desktop);
	//g_print ("new zoom: %f\n",zf);
	iany = (gint)(zf*100+0.5);
	str = g_string_new("");
	
	g_string_sprintf (str,format,iany,'%');
	pos = &p0;
	
#if 0
	gtk_signal_handler_block_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->owner->zoom)->entry),
					  GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop->owner);
	gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (desktop->owner->zoom)->entry), str->str);
	gtk_signal_handler_unblock_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->owner->zoom)->entry),
					    GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop->owner);
#endif
	
	g_string_free(str, TRUE);
}


// read and set the zoom factor from the combo box in the desktop window
void 
sp_desktop_zoom (GtkEntry * caller, SPDesktopWidget *dtw) {
	const gchar * zoom_str;
	ArtDRect d;
	gdouble any;
	
	g_return_if_fail (SP_IS_DESKTOP_WIDGET (dtw));
	
	zoom_str = gtk_entry_get_text (GTK_ENTRY(caller));
	
	//  zoom_str[5] = '\0';
	any = strtod(zoom_str,NULL) /100;
	if (any < SP_DESKTOP_ZOOM_MIN/2) return;
	
	sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);
	sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, any);
	// give focus back to canvas
#if 1
	gtk_widget_grab_focus (GTK_WIDGET (dtw->canvas));
#endif
}


/* Public methods */

/* Context switching */

void
sp_desktop_set_event_context (SPDesktop *desktop, GtkType type, const guchar *config)
{
	SPRepr *repr;

	if (desktop->event_context) {
		sp_event_context_finish (desktop->event_context);
		g_object_unref (G_OBJECT (desktop->event_context));
	}

	repr = (config) ? sodipodi_get_repr (SODIPODI, config) : NULL;

	desktop->event_context = sp_event_context_new (type, desktop, repr);
}

/* Private helpers */

/* fixme: The idea to have underlines is good, but have to fit it into desktop/widget framework (Lauris) */

/* set the coordinate statusbar underline single coordinates with undeline-mask 
 * x and y are document coordinates
 * underline :
 *   0 - don't underline, 1 - underlines x, 2 - underlines y
 *   3 - underline both, 4 - underline none  */

void
sp_desktop_set_coordinate_status (SPDesktop *desktop, gdouble x, gdouble y, guint underline)
{
#if 0
	static guchar cstr[64];
	gchar coord_pattern [20]= "                    ";
	GString * x_str, * y_str;
	gint i=0,j=0;

	g_snprintf (cstr, 64, "%.2g%s, %.2g%s", x, y);
	x_str = SP_PT_TO_STRING (x, SP_DEFAULT_METRIC);
	y_str = SP_PT_TO_STRING (y, SP_DEFAULT_METRIC);
	sprintf (coord_str, "%s, %s", x_str->str, y_str->str);
	gnome_appbar_set_status (GNOME_APPBAR (desktop->owner->coord_status), coord_str);
	// set underline
	if (underline & SP_COORDINATES_UNDERLINE_X) for (; i<x_str->len; i++) coord_pattern[i]='_';
	i = x_str->len + 2;
	if (underline & SP_COORDINATES_UNDERLINE_Y) for (; j<y_str->len; j++,i++) coord_pattern[i]='_';
	if (underline) gtk_label_set_pattern(GTK_LABEL(gnome_appbar_get_status(GNOME_APPBAR (desktop->owner->coord_status))), coord_pattern);

	g_string_free (x_str, TRUE);
	g_string_free (y_str, TRUE);
#endif
}

static gint
sp_desktop_menu_popup (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	sp_event_root_menu_popup (SP_DESKTOP_WIDGET (data)->desktop, NULL, (GdkEvent *)event);

	return FALSE;
}

/* SPDesktopWidget */

static void sp_desktop_widget_class_init (SPDesktopWidgetClass *klass);
static void sp_desktop_widget_init (SPDesktopWidget *widget);
static void sp_desktop_widget_destroy (GtkObject *object);

static void sp_desktop_widget_realize (GtkWidget *widget);

static gint sp_desktop_widget_event (GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw);

static void sp_desktop_widget_view_position_set (SPView *view, gdouble x, gdouble y, SPDesktopWidget *dtw);
static void sp_desktop_widget_view_status_set (SPView *view, const guchar *status, gboolean isdefault, SPDesktopWidget *dtw);
static void sp_dtw_desktop_activate (SPDesktop *desktop, SPDesktopWidget *dtw);
static void sp_dtw_desktop_desactivate (SPDesktop *desktop, SPDesktopWidget *dtw);

static void sp_desktop_widget_adjustment_value_changed (GtkAdjustment *adj, SPDesktopWidget *dtw);
static void sp_desktop_widget_namedview_modified (SPNamedView *nv, guint flags, SPDesktopWidget *dtw);

SPViewWidgetClass *dtw_parent_class;

GtkType
sp_desktop_widget_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPDesktopWidget",
			sizeof (SPDesktopWidget),
			sizeof (SPDesktopWidgetClass),
			(GtkClassInitFunc) sp_desktop_widget_class_init,
			(GtkObjectInitFunc) sp_desktop_widget_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_VIEW_WIDGET, &info);
	}
	return type;
}

static void
sp_desktop_widget_class_init (SPDesktopWidgetClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	SPViewWidgetClass *vw_class;

	dtw_parent_class = gtk_type_class (SP_TYPE_VIEW_WIDGET);

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;
	vw_class = SP_VIEW_WIDGET_CLASS (klass);

	object_class->destroy = sp_desktop_widget_destroy;

	widget_class->realize = sp_desktop_widget_realize;
}

static void
sp_desktop_widget_init (SPDesktopWidget *dtw)
{
	SPDesktopWidget *desktop;
	GtkWidget * menu_arrow;
	GtkWidget * hbox, * widget;
#if 0
	GtkWidget *zoom, *entry; 
#endif
	GtkWidget * eventbox;
	GtkTable * table;
	GtkStyle *style;

	desktop = dtw;

	desktop->desktop = NULL;

	desktop->canvas = NULL;

	desktop->decorations = TRUE;

	desktop->table = gtk_vbox_new (FALSE, 0);
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
	desktop->hscrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0)));
	gtk_widget_show (GTK_WIDGET (desktop->hscrollbar));
	gtk_table_attach (table, desktop->hscrollbar, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	/* Vertical scrollbar */
	desktop->vscrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -4000.0, 4000.0, 10.0, 100.0, 4.0)));
	gtk_widget_show (GTK_WIDGET (desktop->vscrollbar));
	gtk_table_attach (table, desktop->vscrollbar, 2, 3, 1, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	/* Canvas */
	desktop->canvas = SP_CANVAS (sp_canvas_new_aa ());

	style = gtk_style_copy (GTK_WIDGET (desktop->canvas)->style);
	style->bg[GTK_STATE_NORMAL] = style->white;
	gtk_widget_set_style (GTK_WIDGET (desktop->canvas), style);

	gtk_signal_connect (GTK_OBJECT (desktop->canvas), "event",
			    GTK_SIGNAL_FUNC (sp_desktop_widget_event), desktop);

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
	desktop->hruler = sp_hruler_new ();
	sp_ruler_set_metric (GTK_RULER (desktop->hruler), SP_PT);
	gtk_widget_show (desktop->hruler);
	gtk_container_add (GTK_CONTAINER (eventbox), desktop->hruler);
	gtk_table_attach (table,
		eventbox,
		1,2,0,1,
		GTK_FILL,
		GTK_FILL,
		widget->style->xthickness,0);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "button_release_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	gtk_signal_connect (GTK_OBJECT (eventbox), "motion_notify_event",
			    GTK_SIGNAL_FUNC (sp_dt_hruler_event), desktop);
	/* Vertical ruler */
	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	desktop->vruler = sp_vruler_new ();
	sp_ruler_set_metric (GTK_RULER (desktop->vruler), SP_PT);
	gtk_widget_show (GTK_WIDGET (desktop->vruler));
	gtk_container_add (GTK_CONTAINER (eventbox), GTK_WIDGET (desktop->vruler));
	gtk_table_attach (table,
		eventbox,
		0,1,1,2,
		GTK_FILL,
		GTK_FILL,
		0,widget->style->ythickness);
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
	gtk_signal_connect (GTK_OBJECT (desktop->menubutton), 
			    "button_press_event", 
			    GTK_SIGNAL_FUNC (sp_desktop_menu_popup), 
			    desktop);
        GTK_WIDGET_UNSET_FLAGS (GTK_WIDGET(desktop->menubutton), GTK_CAN_FOCUS);
	/* indicator for active desktop */
	hbox = gtk_vbox_new (FALSE, 0);
	gtk_table_attach (table, GTK_WIDGET (hbox), 2,3,0,1, FALSE, FALSE, 0,0);
	gtk_widget_show (hbox);
	desktop->active = gtk_image_new_from_file (SODIPODI_GLADEDIR "/dt_active.xpm");
	gtk_box_pack_start ((GtkBox *) hbox, desktop->active, TRUE, TRUE, 0);
	desktop->inactive = gtk_image_new_from_file (SODIPODI_GLADEDIR "/dt_inactive.xpm");
	gtk_box_pack_start ((GtkBox *) hbox, desktop->inactive, TRUE, TRUE, 0);
       	gtk_widget_show (desktop->inactive);

       // status bars
       	hbox = gtk_hbox_new (FALSE,0);
	gtk_box_set_spacing (GTK_BOX (hbox), 2);
       	gtk_box_pack_start (GTK_BOX (desktop->table),
       	                      GTK_WIDGET (hbox),
                              FALSE,
       	                      FALSE,
                              0);
        gtk_widget_show (hbox);

#if 0
        desktop->coord_status = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_NEVER);
#else
        desktop->coord_status = gtk_statusbar_new ();
#endif
#if 0
	gtk_misc_set_alignment (GTK_MISC (gnome_appbar_get_status(GNOME_APPBAR (desktop->coord_status))), 0.5, 0.5);
#endif
	gtk_widget_show (GTK_WIDGET(desktop->coord_status));	
	gtk_box_pack_start (GTK_BOX (hbox),
			    GTK_WIDGET(desktop->coord_status),
			    FALSE,
			    TRUE,
			    0);
	gtk_widget_set_usize (GTK_WIDGET (desktop->coord_status),120,0);

#if 0
        desktop->select_status = gnome_appbar_new (FALSE, TRUE,GNOME_PREFERENCES_NEVER);
#else
        desktop->select_status = gtk_statusbar_new ();
#endif
#if 0
        gtk_misc_set_alignment (GTK_MISC (gnome_appbar_get_status(GNOME_APPBAR (desktop->select_status))), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (gnome_appbar_get_status(GNOME_APPBAR (desktop->select_status))), 5, 0);
#endif
        //gtk_label_set_line_wrap (GTK_LABEL (desktop->select_status->label), TRUE);
        gtk_widget_show (GTK_WIDGET (desktop->select_status));
        gtk_box_pack_start (GTK_BOX (hbox),
                            GTK_WIDGET (desktop->select_status),
                            TRUE,
                            TRUE,
                            0);

	//desktop->coord_status_id = gtk_statusbar_get_context_id (desktop->coord_status, "mouse coordinates");
	//desktop->select_status_id = gtk_statusbar_get_context_id (desktop->coord_status, "selection stuff");
        gtk_statusbar_push (GTK_STATUSBAR (desktop->select_status), 0, _("Welcome !"));

#if 0
        // zoom combo
        zoom = desktop->zoom = gtk_combo_text_new (FALSE);
/*          if (!gnome_preferences_get_toolbar_relief_btn ()) */
/*                  gtk_combo_box_set_arrow_relief (GTK_COMBO_BOX (zoom), GTK_RELIEF_NONE); */
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
#endif

	// connecting canvas, scrollbars, rulers, statusbar
	dtw->hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	dtw->vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
#if 0
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);
#endif
	gtk_signal_connect (GTK_OBJECT (dtw->hadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_widget_adjustment_value_changed), dtw);
	gtk_signal_connect (GTK_OBJECT (dtw->vadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_widget_adjustment_value_changed), dtw);

#if 1
        gtk_widget_grab_focus (GTK_WIDGET (desktop->canvas));
#endif
}

static void
sp_desktop_widget_destroy (GtkObject *object)
{
	SPDesktopWidget *dtw;

	dtw = SP_DESKTOP_WIDGET (object);

	if (dtw->desktop) {
		g_object_unref (G_OBJECT (dtw->desktop));
		dtw->desktop = NULL;
	}

	if (GTK_OBJECT_CLASS (dtw_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (dtw_parent_class)->destroy) (object);
}

/*
 * set the title in the desktop-window (if desktop has an own window)
 * the title has form sodipodi: file name: namedview name: desktop number
 * the file name is read from the respective document
 */
static void
sp_desktop_widget_set_title (SPDesktopWidget *dtw)
{
	GString *name;
	const gchar *nv_name, *uri, *fname;
	GtkWindow *window;

	window = GTK_WINDOW (gtk_object_get_data (GTK_OBJECT(dtw), "window"));
	if (window) {
		nv_name = sp_namedview_get_name (dtw->desktop->namedview);
		uri = SP_DOCUMENT_NAME (SP_VIEW_WIDGET_DOCUMENT (dtw));
		if (SPShowFullFielName) fname = uri;
		else fname = g_basename (uri);
		name = g_string_new ("");
		g_string_sprintf (name, _("Sodipodi: %s: %s: %d"), fname, nv_name, dtw->desktop->number);
		gtk_window_set_title (window, name->str);
		g_string_free (name, TRUE);
	}
}

static void
sp_desktop_widget_realize (GtkWidget *widget)
{
	SPDesktopWidget *dtw;
	ArtDRect d;

	dtw = SP_DESKTOP_WIDGET (widget);

	if (GTK_WIDGET_CLASS (dtw_parent_class)->realize)
		(* GTK_WIDGET_CLASS (dtw_parent_class)->realize) (widget);

	d.x0 = 0.0;
	d.y0 = 0.0;
	d.x1 = sp_document_width (SP_VIEW_WIDGET_DOCUMENT (dtw));
	d.y1 = sp_document_height (SP_VIEW_WIDGET_DOCUMENT (dtw));

	if ((fabs (d.x1 - d.x0) < 1.0) || (fabs (d.y1 - d.y0) < 1.0)) return;

	sp_desktop_set_display_area (dtw->desktop, d.x0, d.y0, d.x1, d.y1, 10);

	sp_desktop_widget_set_title (dtw);
}

static gint
sp_desktop_widget_event (GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw)
{
	if ((event->type == GDK_BUTTON_PRESS) && (event->button.button == 3)) {
		if (event->button.state & GDK_SHIFT_MASK) {
			sp_canvas_arena_set_sticky (SP_CANVAS_ARENA (dtw->desktop->drawing), TRUE);
		} else {
			sp_canvas_arena_set_sticky (SP_CANVAS_ARENA (dtw->desktop->drawing), FALSE);
		}
	}

	if (GTK_WIDGET_CLASS (dtw_parent_class)->event)
		return (* GTK_WIDGET_CLASS (dtw_parent_class)->event) (widget, event);

	return FALSE;
}

void
sp_dtw_desktop_activate (SPDesktop *desktop, SPDesktopWidget *dtw)
{
	if (dtw->decorations) {
		gtk_widget_show (GTK_WIDGET (dtw->active));
		gtk_widget_hide (GTK_WIDGET (dtw->inactive));
	}
}

void
sp_dtw_desktop_desactivate (SPDesktop *desktop, SPDesktopWidget *dtw)
{
	if (dtw->decorations) {
		gtk_widget_hide (GTK_WIDGET (dtw->active));
		gtk_widget_show (GTK_WIDGET (dtw->inactive));
	}
}

static gboolean
sp_dtw_desktop_shutdown (SPView *view, SPDesktopWidget *dtw)
{
	SPDocument *doc;

	doc = SP_VIEW_DOCUMENT (view);

	if (doc && (((GObject *) doc)->ref_count == 1)) {
		if (sp_repr_attr (sp_document_repr_root (doc), "sodipodi:modified") != NULL) {
			GtkWidget *dlg;
			gchar *msg;
			gint b;
			msg = g_strdup_printf (_("Document %s has unsaved changes, save them?"), SP_DOCUMENT_NAME (doc));
			dlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, msg);
			gtk_dialog_add_button (GTK_DIALOG(dlg), GTK_STOCK_CANCEL, GTK_BUTTONS_CANCEL);
			g_free (msg);
			b = gtk_dialog_run (GTK_DIALOG(dlg));
			gtk_widget_destroy(dlg);
			switch (b) {
			case GTK_RESPONSE_YES:
				sp_file_save_document (doc);
				break;
			case GTK_RESPONSE_NO:
				break;
			case GTK_RESPONSE_CANCEL:
				return TRUE;
				break;
			}
		}
	}

	sp_desktop_prepare_shutdown (SP_DESKTOP (view));

	return FALSE;
}

/* Constructor */


static void
sp_desktop_uri_set (SPView *view, const guchar *uri, SPDesktopWidget *dtw)
{
	sp_desktop_widget_set_title (dtw);
}

SPViewWidget *
sp_desktop_widget_new (SPNamedView *namedview)
{
	SPDesktopWidget *dtw;

	dtw = gtk_type_new (SP_TYPE_DESKTOP_WIDGET);

	dtw->desktop = (SPDesktop *) sp_desktop_new (namedview, dtw->canvas);
	dtw->desktop->owner = dtw;
	g_object_set_data (G_OBJECT (dtw->desktop), "widget", dtw);

	g_signal_connect (G_OBJECT (dtw->desktop), "uri_set", G_CALLBACK (sp_desktop_uri_set), dtw);
	sp_view_widget_set_view (SP_VIEW_WIDGET (dtw), SP_VIEW (dtw->desktop));

	g_signal_connect (G_OBJECT (dtw->desktop), "position_set", G_CALLBACK (sp_desktop_widget_view_position_set), dtw);
	g_signal_connect (G_OBJECT (dtw->desktop), "status_set", G_CALLBACK (sp_desktop_widget_view_status_set), dtw);

	/* Connect activation signals to update indicator */
	g_signal_connect (G_OBJECT (dtw->desktop), "activate", G_CALLBACK (sp_dtw_desktop_activate), dtw);
	g_signal_connect (G_OBJECT (dtw->desktop), "desactivate", G_CALLBACK (sp_dtw_desktop_desactivate), dtw);

	g_signal_connect (G_OBJECT (dtw->desktop), "shutdown", G_CALLBACK (sp_dtw_desktop_shutdown), dtw);

	/* Listen on namedview modification */
	g_signal_connect (G_OBJECT (namedview), "modified", G_CALLBACK (sp_desktop_widget_namedview_modified), dtw);

	if (dtw->decorations) {
		if (SP_DESKTOP (dtw->desktop) == SP_ACTIVE_DESKTOP) {
			gtk_widget_show (GTK_WIDGET (dtw->active));
			gtk_widget_hide (GTK_WIDGET (dtw->inactive));
		} else {
			gtk_widget_hide (GTK_WIDGET (dtw->active));
			gtk_widget_show (GTK_WIDGET (dtw->inactive));
		}
	}

	return SP_VIEW_WIDGET (dtw);
}

static void
sp_desktop_widget_view_position_set (SPView *view, gdouble x, gdouble y, SPDesktopWidget *dtw)
{
	guchar cstr[64];

	/* fixme: */
	GTK_RULER (dtw->hruler)->position = dtw->dt2r * (x - dtw->rx0);
	gtk_ruler_draw_pos (GTK_RULER (dtw->hruler));
	GTK_RULER (dtw->vruler)->position = dtw->dt2r * (y - dtw->ry0);
	gtk_ruler_draw_pos (GTK_RULER (dtw->vruler));

	g_snprintf (cstr, 64, "%6.1f, %6.1f", dtw->dt2r * (x - dtw->rx0), dtw->dt2r * (y - dtw->ry0));
#if 0
	gnome_appbar_set_status (GNOME_APPBAR (dtw->coord_status), cstr);
#else
	gtk_statusbar_pop (GTK_STATUSBAR (dtw->coord_status), 0);
	gtk_statusbar_push (GTK_STATUSBAR (dtw->coord_status), 0, cstr);
#endif
}

/*
 * the statusbars
 *
 * we have 
 * - coordinate status   set with sp_desktop_coordinate_status which is currently not unset
 * - selection status    which is used in two ways:
 *    * sp_desktop_default_status sets the default status text which is visible
 *      if no other text is displayed
 *    * sp_desktop_set_status sets the status text and can be cleared
        with sp_desktop_clear_status making the default visible
 */

static void 
sp_desktop_widget_view_status_set (SPView *view, const guchar *status, gboolean isdefault, SPDesktopWidget *dtw)
{
	if (isdefault) {
		gtk_statusbar_pop (GTK_STATUSBAR (dtw->coord_status), 0);
		gtk_statusbar_push (GTK_STATUSBAR (dtw->coord_status), 0, status);
	} else {
		if (status) {
			gtk_statusbar_pop (GTK_STATUSBAR (dtw->coord_status), 0);
			gtk_statusbar_push (GTK_STATUSBAR (dtw->coord_status), 0, status);
		} else {
			gtk_statusbar_pop (GTK_STATUSBAR (dtw->coord_status), 0);
			gtk_statusbar_push (GTK_STATUSBAR (dtw->coord_status), 0, "");
		}
	}
}

static void
sp_desktop_widget_namedview_modified (SPNamedView *nv, guint flags, SPDesktopWidget *dtw)
{
	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		dtw->dt2r = 1.0 / nv->gridunit->unittobase;
		dtw->rx0 = nv->gridoriginx;
		dtw->ry0 = nv->gridoriginy;
		sp_desktop_widget_update_rulers (dtw);
	}
}

static void
sp_desktop_widget_adjustment_value_changed (GtkAdjustment *adj, SPDesktopWidget *dtw)
{
	if (dtw->update) return;

	dtw->update = 1;

	sp_canvas_scroll_to (dtw->canvas, dtw->hadj->value, dtw->vadj->value, FALSE);
	sp_desktop_widget_update_rulers (dtw);

	dtw->update = 0;
}

/* we make the desktop window with focus active, signal is connected in interface.c */

gint
sp_desktop_widget_set_focus (GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw)
{
	sodipodi_activate_desktop (dtw->desktop);

	/* give focus to canvas widget */
	gtk_widget_grab_focus (GTK_WIDGET (dtw->canvas));
#if 0
	sp_canvas_item_grab_focus ((SPCanvasItem *) dtw->desktop->main); 
#endif

	return FALSE;
}
 
void
sp_desktop_widget_show_decorations (SPDesktopWidget *dtw, gboolean show)
{
	g_return_if_fail (dtw != NULL);
	g_return_if_fail (SP_IS_DESKTOP_WIDGET (dtw));

	if (!dtw->decorations != !show) {
		dtw->decorations = show;
		if (show) {
			gtk_widget_show (GTK_WIDGET (dtw->hscrollbar));
			gtk_widget_show (GTK_WIDGET (dtw->vscrollbar));
			gtk_widget_show (GTK_WIDGET (dtw->hruler));
			gtk_widget_show (GTK_WIDGET (dtw->vruler));
			gtk_widget_show (GTK_WIDGET (dtw->menubutton));
			gtk_widget_show (GTK_WIDGET (dtw->active));		
			gtk_widget_show (GTK_WIDGET (dtw->zoom)->parent);		
		} else {
			gtk_widget_hide (GTK_WIDGET (dtw->hscrollbar));
			gtk_widget_hide (GTK_WIDGET (dtw->vscrollbar));
			gtk_widget_hide (GTK_WIDGET (dtw->hruler));
			gtk_widget_hide (GTK_WIDGET (dtw->vruler));
			gtk_widget_hide (GTK_WIDGET (dtw->menubutton));
			gtk_widget_hide (GTK_WIDGET (dtw->active));
			gtk_widget_hide (GTK_WIDGET (dtw->inactive));
			gtk_widget_hide (GTK_WIDGET (dtw->zoom)->parent);
		}
	}
}

/* fixme: this are UI functions - find a better place for them */

void
sp_desktop_toggle_borders (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	sp_desktop_widget_show_decorations (SP_DESKTOP_WIDGET (desktop->owner), !desktop->owner->decorations);
}

/*
 * Sooner or later we want to implement two sets of methods
 * sp_desktop_... and sp_desktop_widget_...
 * To jump from first to second we probably want to
 * add single VEPV instead of filling desktop with signals
 * (Lauris)
 */

void
sp_desktop_set_display_area (SPDesktop *dt, float x0, float y0, float x1, float y1, float border)
{
	SPDesktopWidget *dtw;
	NRRectF viewbox;
	float cx, cy;
	double scale, newscale;
	int clear;

	dtw = g_object_get_data (G_OBJECT (dt), "widget");
	if (!dtw) return;

	cx = 0.5 * (x0 + x1);
	cy = 0.5 * (y0 + y1);

	sp_canvas_get_viewbox (dtw->canvas, &viewbox);

	viewbox.x0 += border;
	viewbox.y0 += border;
	viewbox.x1 -= border;
	viewbox.y1 -= border;

	scale = SP_DESKTOP_ZOOM (dt);

	if (((x1 - x0) * (viewbox.y1 - viewbox.y0)) > ((y1 - y0) * (viewbox.x1 - viewbox.x0))) {
		newscale = (viewbox.x1 - viewbox.x0) / (x1 - x0);
	} else {
		newscale = (viewbox.y1 - viewbox.y0) / (y1 - y0);
	}

	newscale = CLAMP (newscale, SP_DESKTOP_ZOOM_MIN, SP_DESKTOP_ZOOM_MAX);

	if (!NR_DF_TEST_CLOSE (newscale, scale, 1e-4 * scale)) {
		/* Set zoom factors */
		art_affine_scale (dt->d2w, newscale, -newscale);
		art_affine_invert (dt->w2d, dt->d2w);
		sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (dt->main), dt->d2w);
		clear = TRUE;
	} else {
		clear = FALSE;
	}

	/* Calculate top left corner */
	x0 = cx - 0.5 * (viewbox.x1 - viewbox.x0) / newscale;
	y1 = cy + 0.5 * (viewbox.y1 - viewbox.y0) / newscale;

	/* Scroll */
	sp_canvas_scroll_to (dtw->canvas, x0 * newscale - border, y1 * -newscale - border, clear);

	sp_desktop_widget_update_rulers (dtw);
	sp_desktop_update_scrollbars (dt);

#if 0
       	sp_desktop_zoom_update (desktop);
	sp_dt_update_snap_distances (desktop);
#endif
}

static void
sp_desktop_get_display_area (SPDesktop *dt, NRRectF *area)
{
	SPDesktopWidget *dtw;
	NRRectF viewbox;
	float scale;

	dtw = g_object_get_data (G_OBJECT (dt), "widget");
	if (!dtw) return;

	sp_canvas_get_viewbox (dtw->canvas, &viewbox);

	scale = dt->d2w[0];

	area->x0 = viewbox.x0 / scale;
	area->y0 = viewbox.y1 / -scale;
	area->x1 = viewbox.x1 / scale;
	area->y1 = viewbox.y0 / -scale;
}

ArtDRect *
sp_desktop_get_visible_area (SPDesktop * desktop, ArtDRect * area)
{
	NRRectF da;

	sp_desktop_get_display_area (desktop, &da);

	area->x0 = da.x0;
	area->y0 = da.y0;
	area->x1 = da.x1;
	area->y1 = da.y1;

	return area;
}


void
sp_desktop_zoom_absolute (SPDesktop *dt, float cx, float cy, float zoom)
{
	SPDesktopWidget *dtw;
	NRRectF viewbox;
	float width2, height2;

	dtw = g_object_get_data (G_OBJECT (dt), "widget");
	if (!dtw) return;

	zoom = CLAMP (zoom, SP_DESKTOP_ZOOM_MIN, SP_DESKTOP_ZOOM_MAX);

	sp_canvas_get_viewbox (dtw->canvas, &viewbox);

	width2 = 0.5 * (viewbox.x1 - viewbox.x0) / zoom;
	height2 = 0.5 * (viewbox.y1 - viewbox.y0) / zoom;

	sp_desktop_set_display_area (dt, cx - width2, cy - height2, cx + width2, cy + height2, 0.0);
}

void
sp_desktop_zoom_relative (SPDesktop *dt, float cx, float cy, float zoom)
{
        gdouble scale;

	scale = SP_DESKTOP_ZOOM (dt) * zoom;

	sp_desktop_zoom_absolute (dt, cx, cy, scale);
}

void
sp_desktop_scroll_world (SPDesktop *dt, float dx, float dy)
{
	SPDesktopWidget *dtw;
	NRRectF viewbox;

	dtw = g_object_get_data (G_OBJECT (dt), "widget");
	if (!dtw) return;

	sp_canvas_get_viewbox (dtw->canvas, &viewbox);

	sp_canvas_scroll_to (dtw->canvas, viewbox.x0 - dx, viewbox.y0 - dy, FALSE);

	sp_desktop_widget_update_rulers (dtw);
	sp_desktop_update_scrollbars (dt);
}

static void
sp_desktop_widget_update_rulers (SPDesktopWidget *dtw)
{
	NRRectF viewbox;
	double scale, s, e;
	sp_canvas_get_viewbox (dtw->canvas, &viewbox);
	scale = SP_DESKTOP_ZOOM (dtw->desktop);
	s = viewbox.x0 / scale - dtw->rx0;
	e = viewbox.x1 / scale - dtw->rx0;
	gtk_ruler_set_range (GTK_RULER (dtw->hruler), dtw->dt2r * s, dtw->dt2r * e, GTK_RULER (dtw->hruler)->position, dtw->dt2r * (e - s));
	s = viewbox.y0 / -scale - dtw->ry0;
	e = viewbox.y1 / -scale - dtw->ry0;
	gtk_ruler_set_range (GTK_RULER (dtw->vruler), dtw->dt2r * s, dtw->dt2r * e, GTK_RULER (dtw->vruler)->position, dtw->dt2r * (e - s));
}

static void
set_adjustment (GtkAdjustment *adj, float l, float u, float ps, float si, float pi)
{
	if ((l != adj->lower) ||
	    (u != adj->upper) ||
	    (ps != adj->page_size) ||
	    (si != adj->step_increment) ||
	    (pi != adj->page_increment)) {
		adj->lower = l;
		adj->upper = u;
		adj->page_size = ps;
		adj->step_increment = si;
		adj->page_increment = pi;
		gtk_adjustment_changed (adj);
	}
}

static 
void sp_desktop_update_scrollbars (SPDesktop *dt)
{
	SPDesktopWidget *dtw;
	SPDocument *doc;
	NRRectF darea, carea, viewbox;
	double scale;

	dtw = g_object_get_data (G_OBJECT (dt), "widget");
	if (!dtw) return;

	if (dtw->update) return;
	dtw->update = 1;

	doc = SP_VIEW_DOCUMENT (dt);
	scale = SP_DESKTOP_ZOOM (dt);

	/* The desktop region we always show unconditionally */
	sp_item_bbox_desktop (SP_ITEM (SP_DOCUMENT_ROOT (doc)), &darea);
	darea.x0 = MIN (darea.x0, -sp_document_width (doc));
	darea.y0 = MIN (darea.y0, -sp_document_height (doc));
	darea.x1 = MAX (darea.x1, 2 * sp_document_width (doc));
	darea.y1 = MAX (darea.y1, 2 * sp_document_height (doc));

	/* Canvas region we always show unconditionally */
	carea.x0 = darea.x0 * scale - 64;
	carea.y0 = darea.y1 * -scale - 64;
	carea.x1 = darea.x1 * scale + 64;
	carea.y1 = darea.y0 * -scale + 64;

	sp_canvas_get_viewbox (dtw->canvas, &viewbox);

	/* Viewbox is always included into scrollable region */
	nr_rect_f_union (&carea, &carea, &viewbox);

	set_adjustment (dtw->hadj, carea.x0, carea.x1,
			(viewbox.x1 - viewbox.x0),
			0.1 * (viewbox.x1 - viewbox.x0),
			(viewbox.x1 - viewbox.x0));
	gtk_adjustment_set_value (dtw->hadj, viewbox.x0);

	set_adjustment (dtw->vadj, carea.y0, carea.y1,
			(viewbox.y1 - viewbox.y0),
			0.1 * (viewbox.y1 - viewbox.y0),
			(viewbox.y1 - viewbox.y0));
	gtk_adjustment_set_value (dtw->vadj, viewbox.y0);

	dtw->update = 0;
}


