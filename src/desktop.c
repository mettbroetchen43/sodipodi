#define __SP_DESKTOP_C__

/*
 * Editable view and widget implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#define noDESKTOP_VERBOSE

#include <config.h>
#include <math.h>
#include <gnome.h>
#include "helper/canvas-helper.h"
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

#include <gal/widgets/gtk-combo-text.h>
#include <gal/widgets/gtk-combo-stack.h>

#define SP_CANVAS_STICKY_FLAG (1 << 16)

enum {
	ACTIVATE,
	DESACTIVATE,
	MODIFIED,
	LAST_SIGNAL
};

static void sp_desktop_class_init (SPDesktopClass * klass);
static void sp_desktop_init (SPDesktop * desktop);
static void sp_desktop_destroy (GtkObject * object);

static void sp_desktop_set_document (SPView *view, SPDocument *doc);
static void sp_desktop_document_resized (SPView *view, SPDocument *doc, gdouble width, gdouble height);

/* Constructor */

static SPView *sp_desktop_new (SPNamedView *nv, SPDesktopWidget *owner);

static void sp_desktop_size_allocate (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop);
static void sp_desktop_update_scrollbars (Sodipodi * sodipodi, SPSelection * selection);
static void sp_desktop_set_viewport (SPDesktop * desktop, double x, double y);
static void sp_desktop_zoom_update (SPDesktop * desktop);

static gint select_set_id =0;

void sp_desktop_zoom (GtkEntry * caller, SPDesktopWidget *dtw);

/* fixme: */
void sp_desktop_toggle_borders (GtkWidget * widget);

static void sp_desktop_menu_popup (GtkWidget * widget, GdkEventButton * event, gpointer data);

/* fixme: These are widget forward decls, that are here, until things will be sorted out */
static void sp_desktop_widget_update_rulers (GtkWidget * widget, SPDesktopWidget *dtw);

SPViewClass * parent_class;
static guint signals[LAST_SIGNAL] = { 0 };

GtkType
sp_desktop_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		static const GtkTypeInfo info = {
			"SPDesktop",
			sizeof (SPDesktop),
			sizeof (SPDesktopClass),
			(GtkClassInitFunc) sp_desktop_class_init,
			(GtkObjectInitFunc) sp_desktop_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_VIEW, &info);
	}
	return type;
}

static void
sp_desktop_class_init (SPDesktopClass *klass)
{
	GtkObjectClass *object_class;
	SPViewClass *view_class;

	object_class = GTK_OBJECT_CLASS (klass);
	view_class = SP_VIEW_CLASS (klass);

	parent_class = gtk_type_class (SP_TYPE_VIEW);

	signals[ACTIVATE] = gtk_signal_new ("activate",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDesktopClass, activate),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	signals[DESACTIVATE] = gtk_signal_new ("desactivate",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDesktopClass, desactivate),
					    gtk_marshal_NONE__NONE,
					    GTK_TYPE_NONE, 0);
	signals[MODIFIED] = gtk_signal_new ("modified",
					    GTK_RUN_FIRST,
					    object_class->type,
					    GTK_SIGNAL_OFFSET (SPDesktopClass, modified),
					    gtk_marshal_NONE__UINT,
					    GTK_TYPE_NONE, 1, GTK_TYPE_UINT);
	gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

	object_class->destroy = sp_desktop_destroy;

	view_class->set_document = sp_desktop_set_document;
	view_class->document_resized = sp_desktop_document_resized;
}

static void
sp_desktop_init (SPDesktop *desktop)
{
	desktop->owner = NULL;

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
	art_affine_identity (desktop->doc2dt);
}

static void
sp_desktop_destroy (GtkObject *object)
{
	SPDesktop *desktop;

	desktop = SP_DESKTOP (object);

	sodipodi_remove_desktop (desktop);

	if (desktop->event_context) {
		gtk_object_unref (GTK_OBJECT (desktop->event_context));
		desktop->event_context = NULL;
	}

	if (desktop->selection) {
		gtk_object_unref (GTK_OBJECT (desktop->selection));
		desktop->selection = NULL;
	}

	if (desktop->drawing) {
		sp_namedview_hide (desktop->namedview, desktop);
		sp_item_hide (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (desktop))), SP_CANVAS_ARENA (desktop->drawing)->arena);
		desktop->drawing = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_desktop_document_resized (SPView *view, SPDocument *doc, gdouble width, gdouble height)
{
	SPDesktop *desktop;

	desktop = SP_DESKTOP (view);

	desktop->doc2dt[3] = -1.0;
	desktop->doc2dt[5] = height;

	gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (desktop->drawing), desktop->doc2dt);

	sp_ctrlrect_set_area (SP_CTRLRECT (desktop->page), 0.0, 0.0, width, height);
}

void
sp_desktop_set_active (SPDesktop *desktop, gboolean active)
{
	if (active != desktop->active) {
		desktop->active = active;
		if (active) {
			gtk_signal_emit (GTK_OBJECT (desktop), signals[ACTIVATE]);
		} else {
			gtk_signal_emit (GTK_OBJECT (desktop), signals[DESACTIVATE]);
		}
	}
}

/* fixme: */

static gint
arena_handler (SPCanvasArena *arena, NRArenaItem *item, GdkEvent *event, SPDesktop *desktop)
{
	if (item) {
		SPItem *spi;
		spi = gtk_object_get_user_data (GTK_OBJECT (item));
		sp_event_context_item_handler (desktop->event_context, spi, event);
	} else {
		sp_event_context_root_handler (desktop->event_context, event);
	}

	return TRUE;
}

/* Constructor */

static SPView *
sp_desktop_new (SPNamedView *namedview, SPDesktopWidget *owner)
{
	SPDesktop *desktop;
	GnomeCanvasGroup *root, *page;
	NRArenaItem *ai;
	gdouble dw, dh;
	SPDocument *document;

	document = SP_OBJECT_DOCUMENT (namedview);

	/* Setup widget */

	desktop = (SPDesktop *) gtk_type_new (sp_desktop_get_type ());

	desktop->owner = owner;
	desktop->owner->desktop = desktop;

	/* Connect document */
	sp_view_set_document (SP_VIEW (desktop), document);

	desktop->namedview = namedview;
	desktop->number = sp_namedview_viewcount (namedview);

	/* Setup Canvas */
	gtk_object_set_data (GTK_OBJECT (desktop->owner->canvas), "SPDesktop", desktop);

	root = gnome_canvas_root (desktop->owner->canvas);

	desktop->acetate = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_ACETATE, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->acetate), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), desktop);
	/* Setup adminstrative layers */
	desktop->main = (GnomeCanvasGroup *) gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_GROUP, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->main), "event",
		GTK_SIGNAL_FUNC (sp_desktop_root_handler), desktop);
	/* fixme: */
	page = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main, GNOME_TYPE_CANVAS_GROUP, NULL);

	desktop->drawing = (GnomeCanvasGroup *) gnome_canvas_item_new (desktop->main, SP_TYPE_CANVAS_ARENA, NULL);
	gtk_signal_connect (GTK_OBJECT (desktop->drawing), "arena_event", GTK_SIGNAL_FUNC (arena_handler), desktop);

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

	desktop->page = gnome_canvas_item_new (page, SP_TYPE_CTRLRECT, NULL);
	sp_ctrlrect_set_area (SP_CTRLRECT (desktop->page), 0.0, 0.0, sp_document_width (document), sp_document_height (document));
	sp_ctrlrect_set_shadow (SP_CTRLRECT (desktop->page), 5, 0x3f3f3fff);

	/* Connect event for page resize */
	art_affine_identity (desktop->doc2dt);
	desktop->doc2dt[3] = -1.0;
	desktop->doc2dt[5] = sp_document_height (document);
	gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (desktop->drawing), desktop->doc2dt);

	/* Fixme: Setup initial zooming */

	art_affine_scale (desktop->d2w, 1.0, -1.0);
	desktop->d2w[5] = dw;
	art_affine_invert (desktop->w2d, desktop->d2w);
	gnome_canvas_item_affine_absolute ((GnomeCanvasItem *) desktop->main, desktop->d2w);

	gnome_canvas_set_scroll_region (desktop->owner->canvas,
		-SP_DESKTOP_SCROLL_LIMIT,
		-SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT,
		SP_DESKTOP_SCROLL_LIMIT);

	gtk_signal_connect (GTK_OBJECT (desktop->owner->canvas), "size-allocate", GTK_SIGNAL_FUNC (sp_desktop_size_allocate), desktop);
	if (select_set_id<1) select_set_id = gtk_signal_connect (GTK_OBJECT (SODIPODI), "change_selection", 
								 GTK_SIGNAL_FUNC (sp_desktop_update_scrollbars),NULL);

	ai = sp_item_show (SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (desktop))), SP_CANVAS_ARENA (desktop->drawing)->arena);
	if (ai) {
		nr_arena_item_add_child (SP_CANVAS_ARENA (desktop->drawing)->root, ai, NULL);
		gtk_object_unref (GTK_OBJECT(ai));
	}

	sp_namedview_show (desktop->namedview, desktop);
	/* Ugly hack */
	sp_desktop_activate_guides (desktop, TRUE);

	// ?
	// sp_active_desktop_set (desktop);
	sodipodi_add_desktop (desktop);

	return SP_VIEW (desktop);
}


void
sp_desktop_show_decorations (SPDesktop * desktop, gboolean show)
{
	g_assert (desktop != NULL);
	g_assert (SP_IS_DESKTOP (desktop));

	if (desktop->owner->decorations == show) return;

	desktop->owner->decorations = show;

	if (show) {
		gtk_widget_show (GTK_WIDGET (desktop->owner->hscrollbar));
		gtk_widget_show (GTK_WIDGET (desktop->owner->vscrollbar));
		gtk_widget_show (GTK_WIDGET (desktop->owner->hruler));
		gtk_widget_show (GTK_WIDGET (desktop->owner->vruler));
		gtk_widget_show (GTK_WIDGET (desktop->owner->menubutton));
		gtk_widget_show (GTK_WIDGET (desktop->owner->active));		
		gtk_widget_show (GTK_WIDGET (desktop->owner->zoom)->parent);		
	} else {
		gtk_widget_hide (GTK_WIDGET (desktop->owner->hscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->vscrollbar));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->hruler));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->vruler));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->menubutton));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->active));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->inactive));
		gtk_widget_hide (GTK_WIDGET (desktop->owner->zoom)->parent);		
	}
}

void
sp_desktop_activate_guides (SPDesktop * desktop, gboolean activate)
{
	desktop->owner->guides_active = activate;
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
			gtk_object_unref (GTK_OBJECT(ai));
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

// redraw the scrollbars in the desktop window and set the adjustments to <reasonable> values
static 
void sp_desktop_update_scrollbars (Sodipodi * sodipodi, SPSelection * selection)
{
  ArtPoint wc0, wc1, dc0, dc1, p0, p1;
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

  g_return_if_fail (SP_IS_SELECTION (selection));
  desktop = selection->desktop;
  g_return_if_fail (SP_IS_DESKTOP (desktop));
  docitem = SP_ITEM (sp_document_root (SP_VIEW_DOCUMENT (desktop)));
  g_return_if_fail (docitem != NULL);

  hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->owner->hscrollbar));
  vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->owner->vscrollbar));

  zf = sp_desktop_zoom_factor (desktop);
  cw = GTK_WIDGET (desktop->owner->canvas)->allocation.width;
  ch = GTK_WIDGET (desktop->owner->canvas)->allocation.height;
  // drawing / document coordinates
  sp_item_bbox (docitem, &d);
  // add document area / document coordinates
  if (d.x0 > 0) d.x0 = 0;
  if (d.y0 > 0) d.y0 = 0;
  dw = sp_document_width (SP_VIEW_DOCUMENT (desktop));
  dh = sp_document_height (SP_VIEW_DOCUMENT (desktop));
  if (d.x1 < dw) d.x1 = dw;
  if (d.y1 < dh) d.y1 = dh;
  // add border around drawing / document coordinates
  dc0.x = d.x0 - cw/zf;
  dc0.y = d.y1 + ch/zf;
  dc1.x = d.x1 + cw/zf;
  dc1.y = d.y0 - ch/zf;
  // >> world coordinates
  art_affine_point (&wc0, &dc0, desktop->d2w);
  art_affine_point (&wc1, &dc1, desktop->d2w);
  // enlarge to fit desktop / world coordinates 
  gnome_canvas_window_to_world (desktop->owner->canvas, 0.0, 0.0, &p0.x, &p0.y);
  gnome_canvas_window_to_world (desktop->owner->canvas, cw, ch, &p1.x, &p1.y);
  if (p0.x < wc0.x) wc0.x = p0.x;
  if (p0.y < wc0.y) wc0.y = p0.y;
  if (p1.x > wc1.x) wc1.x = p1.x;
  if (p1.y > wc1.y) wc1.y = p1.y;
  // add border arround desktop / world coordinates
  wc0.x -= 10/zf;
  wc0.y -= 10/zf;
  wc1.x += 10/zf;
  wc1.y += 10/zf;  
  // set new adjustment / world coordinates
  hadj->lower = wc0.x + SP_DESKTOP_SCROLL_LIMIT;
  hadj->upper = wc1.x + SP_DESKTOP_SCROLL_LIMIT;
  vadj->lower = wc0.y + SP_DESKTOP_SCROLL_LIMIT;
  vadj->upper = wc1.y + SP_DESKTOP_SCROLL_LIMIT;//-abs(p1.y - p0.y);
  //restrict to canvas scroll limit / world coordinates
  if (hadj->lower < 0) hadj->lower = 0;
  if (hadj->upper > 2*SP_DESKTOP_SCROLL_LIMIT) hadj->upper = 2*SP_DESKTOP_SCROLL_LIMIT;
  if (vadj->lower < 0) vadj->lower = 0;
  if (vadj->upper > 2*SP_DESKTOP_SCROLL_LIMIT) vadj->upper = 2*SP_DESKTOP_SCROLL_LIMIT;

  gtk_adjustment_changed (hadj);
  gtk_adjustment_changed (vadj);
}


// is called when the cavas is resized, connected in sp_desktop_new
static 
void sp_desktop_size_allocate (GtkWidget * widget, GtkRequisition *requisition, SPDesktop * desktop) 
{
  sp_desktop_widget_update_rulers (widget, desktop->owner);
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

  gtk_signal_handler_block_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->owner->zoom)->entry),
                                    GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop->owner);
  gtk_combo_text_set_text (GTK_COMBO_TEXT (desktop->owner->zoom), str->str);
  gtk_signal_handler_unblock_by_func (GTK_OBJECT (GTK_COMBO_TEXT (desktop->owner->zoom)->entry),
                                      GTK_SIGNAL_FUNC (sp_desktop_zoom), desktop->owner);

  g_string_free(str, TRUE);
}


// read and set the zoom factor from the combo box in the desktop window
void 
sp_desktop_zoom (GtkEntry * caller, SPDesktopWidget *dtw) {
  gchar * zoom_str;
  ArtDRect d;
  gdouble any;

  g_return_if_fail (SP_IS_DESKTOP_WIDGET (dtw));

  zoom_str = gtk_entry_get_text (GTK_ENTRY(caller));

  //  zoom_str[5] = '\0';
  any = strtod(zoom_str,NULL) /100;
  if (any < SP_DESKTOP_ZOOM_MIN/2) return;
  
  sp_desktop_get_visible_area (SP_ACTIVE_DESKTOP, &d);
  sp_desktop_zoom_absolute (SP_ACTIVE_DESKTOP, any, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2);
  // give focus back to canvas
#if 1
  gtk_widget_grab_focus (GTK_WIDGET (dtw->canvas));
#endif
}


/* Public methods */

void
sp_desktop_set_position (SPDesktop * desktop, double x, double y)
{
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	desktop->owner->hruler->position = x;
	gtk_ruler_draw_pos (desktop->owner->hruler);
	desktop->owner->vruler->position = y;
	gtk_ruler_draw_pos (desktop->owner->vruler);
}

void
sp_desktop_scroll_world (SPDesktop * desktop, gint dx, gint dy)
{
	gint x, y;

	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	gnome_canvas_get_scroll_offsets (desktop->owner->canvas, &x, &y);
	gnome_canvas_scroll_to (desktop->owner->canvas, x - dx, y - dy);

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

	cw = GTK_WIDGET (desktop->owner->canvas)->allocation.width+1;
	ch = GTK_WIDGET (desktop->owner->canvas)->allocation.height+1;

	gnome_canvas_window_to_world (desktop->owner->canvas, 0.0, 0.0, &p0.x, &p0.y);
	gnome_canvas_window_to_world (desktop->owner->canvas, cw, ch, &p1.x, &p1.y);

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

	cw = GTK_WIDGET (desktop->owner->canvas)->allocation.width;
	ch = GTK_WIDGET (desktop->owner->canvas)->allocation.height;

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
    
    cw = GTK_WIDGET (desktop->owner->canvas)->allocation.width + 1;
    ch = GTK_WIDGET (desktop->owner->canvas)->allocation.height + 1;
    gnome_canvas_scroll_to (desktop->owner->canvas, x - cw / 2, y - ch / 2);

    sp_desktop_widget_update_rulers (NULL, desktop->owner);
    sp_desktop_update_scrollbars (SODIPODI, SP_DT_SELECTION (desktop));
}

/* fixme: this are UI functions - find a better place for them */

void
sp_desktop_toggle_borders (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;

	if (desktop == NULL) return;

	sp_desktop_show_decorations (desktop, !desktop->owner->decorations);
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

void 
sp_desktop_default_status (SPDesktop *desktop, const gchar * stat)
{
  gint b =0;
  GString * text;

  g_return_if_fail (SP_IS_DESKTOP (desktop));

  text = g_string_new(stat);
  // remove newlines 
  for (b=0; text->str[b]!=0; b+=1) if (text->str[b]=='\n') text->str[b]=' ';
  gnome_appbar_set_default (desktop->owner->select_status, text->str);
  g_string_free(text,FALSE);
}

void
sp_desktop_set_status (SPDesktop * desktop, const gchar * stat)
{
  gnome_appbar_set_status (desktop->owner->select_status, stat);
}

void
sp_desktop_clear_status(SPDesktop * desktop)
{
  gnome_appbar_clear_stack (desktop->owner->select_status);
}

/* set the coordinate statusbar underline single coordinates with undeline-mask 
 * x and y are document coordinates
 * underline :
 *   0 - don't underline, 1 - underlines x, 2 - underlines y
 *   3 - underline both, 4 - underline none  */
void
sp_desktop_coordinate_status (SPDesktop * desktop, gdouble x, gdouble y, gint8 underline)
{
  static gchar coord_str[40];
  gchar coord_pattern [20]= "                    ";
  GString * x_str, * y_str;
  gint i=0,j=0;

  x_str = SP_PT_TO_STRING (x, SP_DEFAULT_METRIC);
  y_str = SP_PT_TO_STRING (y, SP_DEFAULT_METRIC);
  sprintf (coord_str, "%s, %s",
	     x_str->str,
	     y_str->str);
  gnome_appbar_set_status (desktop->owner->coord_status, coord_str);
  // set underline
  if (underline & 0x01) for (; i<x_str->len; i++) coord_pattern[i]='_';
  i = x_str->len + 2;
  if (underline & 0x02) for (; j<y_str->len; j++,i++) coord_pattern[i]='_';
  if (underline) gtk_label_set_pattern(GTK_LABEL(desktop->owner->coord_status->status), coord_pattern);

  g_string_free (x_str, TRUE);
  g_string_free (y_str, TRUE);
}


// we make the desktop window with focus active, signal is connected in interface.c
gint
sp_desktop_set_focus (GtkWidget *widget, GtkWidget *widget2, SPDesktop * desktop)
{
  sodipodi_activate_desktop (desktop);
  // give focus to canvas widget
#if 1
  gtk_widget_grab_focus (GTK_WIDGET (desktop->owner->canvas));
  gnome_canvas_item_grab_focus ((GnomeCanvasItem *) desktop->main); 
#endif

  return FALSE;
}
 
static void
sp_desktop_menu_popup (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	sp_event_root_menu_popup (SP_DESKTOP_WIDGET (data)->desktop, NULL, (GdkEvent *)event);
}

/* SPDesktopWidget */

static void sp_desktop_widget_class_init (SPDesktopWidgetClass *klass);
static void sp_desktop_widget_init (SPDesktopWidget *widget);
static void sp_desktop_widget_destroy (GtkObject *object);

static void sp_desktop_widget_realize (GtkWidget *widget);

static gint sp_desktop_widget_event (GtkWidget *widget, GdkEvent *event, SPDesktopWidget *dtw);

static gboolean sp_desktop_widget_shutdown (SPViewWidget *vw);

static void sp_dtw_desktop_activate (SPDesktop *desktop, SPDesktopWidget *dtw);
static void sp_dtw_desktop_desactivate (SPDesktop *desktop, SPDesktopWidget *dtw);

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

	vw_class->shutdown = sp_desktop_widget_shutdown;
}

static void
sp_desktop_widget_init (SPDesktopWidget *desktop)
{
  //	GtkWidget * menu_button;
	GtkWidget * menu_arrow;
	GtkWidget * hbox, * widget, * zoom, * entry; 
	GtkWidget * eventbox;
	GtkTable * table;
	GtkStyle *style;
	GtkAdjustment *hadj, *vadj;
	//	GList * zoom_list = NULL;
#if 0
        GTK_WIDGET_SET_FLAGS (GTK_WIDGET(desktop), GTK_CAN_FOCUS);
#endif
	desktop->desktop = NULL;

	desktop->canvas = NULL;

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
	gtk_signal_connect (GTK_OBJECT (desktop->menubutton), 
			    "button_press_event", 
			    GTK_SIGNAL_FUNC (sp_desktop_menu_popup), 
			    desktop);
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
        gnome_appbar_set_status (desktop->select_status, _("Welcome !"));

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

	// connecting canvas, scrollbars, rulers, statusbar
	hadj = gtk_range_get_adjustment (GTK_RANGE (desktop->hscrollbar));
	vadj = gtk_range_get_adjustment (GTK_RANGE (desktop->vscrollbar));
	gtk_layout_set_hadjustment (GTK_LAYOUT (desktop->canvas), hadj);
	gtk_layout_set_vadjustment (GTK_LAYOUT (desktop->canvas), vadj);
	gtk_signal_connect (GTK_OBJECT (hadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_widget_update_rulers), desktop);
	gtk_signal_connect (GTK_OBJECT (vadj), "value-changed", GTK_SIGNAL_FUNC (sp_desktop_widget_update_rulers), desktop);

#if 1
        gtk_widget_grab_focus (GTK_WIDGET (desktop->canvas));
#endif
}

static void
sp_desktop_widget_destroy (GtkObject *object)
{
	SPDesktopWidget *dtw;

	dtw = SP_DESKTOP_WIDGET (object);

	gtk_signal_disconnect_by_data (GTK_OBJECT (sodipodi), object);

	if (dtw->desktop) {
		gtk_object_unref (GTK_OBJECT (dtw->desktop));
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
		uri = sp_document_uri (SP_VIEW_WIDGET_DOCUMENT (dtw));
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

	sp_desktop_show_region (dtw->desktop, d.x0, d.y0, d.x1, d.y1, 10);

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

/* fixme: This is not very nice place for doing dialog and saving (Lauris) */

static gboolean
sp_desktop_widget_shutdown (SPViewWidget *vw)
{
	SPDocument *doc;

	doc = SP_VIEW_WIDGET_DOCUMENT (vw);

	if (doc && (((GtkObject *) doc)->ref_count == 1)) {
		if (sp_repr_attr (sp_document_repr_root (doc), "sodipodi:modified") != NULL) {
			GtkWidget *dlg;
			gchar *msg;
			gint b;
			msg = g_strdup_printf (_("Document %s has unsaved changes, save them?"), sp_document_uri (doc));
			dlg = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_WARNING, _("Save"), _("Don't save"), GNOME_STOCK_BUTTON_CANCEL, NULL);
			g_free (msg);
			b = gnome_dialog_run_and_close (GNOME_DIALOG (dlg));
			switch (b) {
			case 0:
				sp_file_save_document (doc);
				break;
			case 1:
				break;
			case 2:
				return TRUE;
				break;
			}
		}
	}
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

	dtw->desktop = (SPDesktop *) sp_desktop_new (namedview, dtw);
	gtk_signal_connect (GTK_OBJECT (dtw->desktop), "uri_set", GTK_SIGNAL_FUNC (sp_desktop_uri_set), dtw);
	sp_view_widget_set_view (SP_VIEW_WIDGET (dtw), SP_VIEW (dtw->desktop));

	/* Connect activation signals to update indicator */
	gtk_signal_connect (GTK_OBJECT (dtw->desktop), "activate",
			    GTK_SIGNAL_FUNC (sp_dtw_desktop_activate), dtw);
	gtk_signal_connect (GTK_OBJECT (dtw->desktop), "desactivate",
			    GTK_SIGNAL_FUNC (sp_dtw_desktop_desactivate), dtw);

	return SP_VIEW_WIDGET (dtw);
}

static void
sp_desktop_widget_update_rulers (GtkWidget * widget, SPDesktopWidget *dtw)
{
	ArtPoint p0, p1;

	gnome_canvas_window_to_world (dtw->canvas, 0.0, 0.0, &p0.x, &p0.y);
	gnome_canvas_window_to_world (dtw->canvas,
		((GtkWidget *) dtw->canvas)->allocation.width,
		((GtkWidget *) dtw->canvas)->allocation.height,
		&p1.x, &p1.y);
	art_affine_point (&p0, &p0, dtw->desktop->w2d);
	art_affine_point (&p1, &p1, dtw->desktop->w2d);

	gtk_ruler_set_range (dtw->hruler, p0.x, p1.x, dtw->hruler->position, p1.x);
	gtk_ruler_set_range (dtw->vruler, p0.y, p1.y, dtw->vruler->position, p1.y);
}


