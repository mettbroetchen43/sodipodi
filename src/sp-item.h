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
 * read - reads item params from representation, creating children if
 *        necessary
 * read_attr
 * show
 * hide
 * paint
 */

/*
 * NB! Currently update is intended for changing affine transforms
 * For other changes there should be other methods
 */

#include <gtk/gtk.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeui/gnome-canvas.h>
#include <libart_lgpl/art_pixbuf.h>
#include "sp-object.h"

#ifndef SP_ITEM_DEFINED
#define SP_ITEM_DEFINED
typedef struct _SPItem SPItem;
typedef struct _SPItemClass SPItemClass;
#endif

#ifndef SP_GROUP_DEFINED
#define SP_GROUP_DEFINED
typedef struct _SPGroup SPGroup;
typedef struct _SPGroupClass SPGroupClass;
#endif

#define SP_TYPE_ITEM            (sp_item_get_type ())
#define SP_ITEM(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ITEM, SPItem))
#define SP_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ITEM, SPItemClass))
#define SP_IS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ITEM))
#define SP_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ITEM))

struct _SPItem {
	SPObject object;
	guint stop_paint: 1;	/* If set, ::paint returns TRUE */
	double affine[6];
	GSList * display;
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
	GnomeCanvasItem * (* show) (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
	void (* hide) (SPItem * item, GnomeCanvas * canvas);

	/* Who finds better name?
	 * Basically same as render, but draws to rgba buf,
	 * has separate affine & can be stopped during execution
	 * Is used for rendering buffer fills & exporting raster images
	 */

	gboolean (* paint) (SPItem * item, ArtPixBuf * buf, gdouble * affine);
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

GnomeCanvasItem * sp_item_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
void sp_item_hide (SPItem * item, GnomeCanvas * canvas);
gboolean sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

/* Utility */

GnomeCanvasItem * sp_item_canvas_item (SPItem * item, GnomeCanvas * canvas);

void sp_item_request_canvas_update (SPItem * item);

gdouble * sp_item_i2d_affine (SPItem * item, gdouble affine[]);
void sp_item_set_i2d_affine (SPItem * item, gdouble affine[]);
gdouble * sp_item_i2doc_affine (SPItem * item, gdouble affine[]);

void sp_item_change_canvasitem_position (SPItem * item, gint delta);
void sp_item_raise_canvasitem_to_top (SPItem * item);

/* group */

#define SP_TYPE_GROUP            (sp_group_get_type ())
#define SP_GROUP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GROUP, SPGroup))
#define SP_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GROUP, SPGroupClass))
#define SP_IS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GROUP))
#define SP_IS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GROUP))

struct _SPGroup {
	SPItem item;
	GSList * children;
	gboolean transparent;
};

struct _SPGroupClass {
	SPItemClass parent_class;
};


/* Standard Gtk function */

GtkType sp_group_get_type (void);

#endif
