#ifndef SP_ITEM_H
#define SP_ITEM_H

/*
 * SPItem - the base item of new hierarchy
 *
 * Methods:
 * bbox - return tight bbox in document coordinates
 * print - prints item, using gnome-print API
 * description - gives textual description of item
 *               for example "group of 4 items"
 *               caller has to free string
 * show
 * hide
 * paint
 */

/*
 * NB! Currently update is intended for changing affine transforms
 * For other changes there should be other methods
 */

#include <gtk/gtkmenu.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeui/gnome-canvas.h>
#include <libart_lgpl/art_pixbuf.h>
#include "forward.h"
#include "sp-object.h"

#define SP_TYPE_ITEM            (sp_item_get_type ())
#define SP_ITEM(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ITEM, SPItem))
#define SP_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ITEM, SPItemClass))
#define SP_IS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ITEM))
#define SP_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ITEM))

typedef struct _SPItemView SPItemView;

struct _SPItemView {
	SPItemView * next;
	SPItemView * prev;
	SPItem * item;
	SPDesktop * desktop;
	GnomeCanvasItem * canvasitem;
};

struct _SPItem {
	SPObject object;
	guint stop_paint: 1;	/* If set, ::paint returns TRUE */
	gdouble affine[6];
	SPItemView * display;
};

struct _SPItemClass {
	SPObjectClass parent_class;

	void (* update) (SPItem * item, gdouble affine[]);

	void (* bbox) (SPItem * item, ArtDRect * bbox);

	/* Printing method. Assumes ctm is set to item affine matrix */

	void (* print) (SPItem * item, GnomePrintContext * gpc);

	/* Give short description of item (for status display) */

	gchar * (* description) (SPItem * item);

	/* Silly, silly. We should assign handlers a more intelligent way */
	GnomeCanvasItem * (* show) (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
	void (* hide) (SPItem * item, SPDesktop * desktop);

	/* Who finds better name?
	 * Basically same as render, but draws to rgba buf,
	 * has separate affine & can be stopped during execution
	 * Is used for rendering buffer fills & exporting raster images
	 */

	gboolean (* paint) (SPItem * item, ArtPixBuf * buf, gdouble * affine);

	/* Append to context menu */
	void (* menu) (SPItem * item, SPDesktop *desktop, GtkMenu * menu);
        /* give list of points for item to be considered for snapping */ 
        GSList * (* snappoints) (SPItem * item, GSList * points);
};

/* Flag testing macros */

#define SP_ITEM_STOP_PAINT(i) (SP_ITEM (i)->stop_paint)

/* Standard Gtk function */

GtkType sp_item_get_type (void);

/* Methods */

void sp_item_update (SPItem * item, gdouble affine[]);
void sp_item_bbox (SPItem * item, ArtDRect * bbox);
gchar * sp_item_description (SPItem * item);
void sp_item_print (SPItem * item, GnomePrintContext * gpc);

GnomeCanvasItem * sp_item_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
void sp_item_hide (SPItem * item, SPDesktop * desktop);
gboolean sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

GSList * sp_item_snappoints (SPItem * item);

void sp_item_set_item_transform (SPItem *item, const gdouble *transform);

/* Utility */

GnomeCanvasItem * sp_item_canvas_item (SPItem * item, SPDesktop * desktop);

void sp_item_request_canvas_update (SPItem * item);

gdouble * sp_item_i2d_affine (SPItem * item, gdouble affine[]);
void sp_item_set_i2d_affine (SPItem * item, gdouble affine[]);
gdouble * sp_item_i2doc_affine (SPItem * item, gdouble affine[]);

void sp_item_change_canvasitem_position (SPItem * item, gint delta);
void sp_item_raise_canvasitem_to_top (SPItem * item);

/* Context menu stuff */

void sp_item_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

SPItemView * sp_item_view_new_prepend (SPItemView * list, SPItem * item, SPDesktop * desktop, GnomeCanvasItem * canvasitem);
SPItemView * sp_item_view_list_remove (SPItemView * list, SPItemView * view);

#endif
