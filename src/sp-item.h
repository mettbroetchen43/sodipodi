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
#include "xml/repr.h"

typedef struct _SPItem SPItem;
typedef struct _SPItemClass SPItemClass;

typedef struct _SPGroup SPGroup;
typedef struct _SPGroupClass SPGroupClass;

#define SP_TYPE_ITEM            (sp_item_get_type ())
#define SP_ITEM(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ITEM, SPItem))
#define SP_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ITEM, SPItemClass))
#define SP_IS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ITEM))
#define SP_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ITEM))

struct _SPItem {
	GtkObject object;
	SPGroup * parent;
	SPRepr * repr;
	double affine[6];
	GSList * display;
};

struct _SPItemClass {
	GtkObjectClass parent_class;

	void (* update) (SPItem * item, gdouble affine[]);

	void (* bbox) (SPItem * item, ArtDRect * bbox);

	/* Printing method. Assumes ctm is set to item affine matrix */

	void (* print) (SPItem * item, GnomePrintContext * gpc);

	/* Give short description of item (for status display) */

	gchar * (* description) (SPItem * item);

	/* Read item settings from SPRepr item */

	void (* read) (SPItem * item, SPRepr * repr);
	void (* read_attr) (SPItem * item, SPRepr * repr, const gchar * key);

	/* Silly, silly. We should assign handlers a more intelligent way */
	GnomeCanvasItem * (* show) (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
	void (* hide) (SPItem * item, GnomeCanvas * canvas);

	/* Who finds better name?
	 * Basically same as render, but draws to rgba buf,
	 * has separate affine & can be stopped during execution
	 * Is used for rendering buffer fills & exporting raster images
	 */

	void (* paint) (SPItem * item, ArtPixBuf * buf, gdouble * affine);
};

/* Standard Gtk function */

GtkType sp_item_get_type (void);

/* Constructors */

SPItem * sp_item_new_root (SPRepr * repr);
SPItem * sp_item_new (SPRepr * repr, SPGroup * parent);

/* Methods */

void sp_item_update (SPItem * item, gdouble affine[]);
void sp_item_bbox (SPItem * item, ArtDRect * bbox);
gchar * sp_item_description (SPItem * item);
void sp_item_print (SPItem * item, GnomePrintContext * gpc);
void sp_item_read (SPItem * item, SPRepr * repr);
void sp_item_read_attr (SPItem * item, SPRepr * repr, const gchar * key);
GnomeCanvasItem * sp_item_show (SPItem * item, GnomeCanvasGroup * canvas_group, gpointer handler);
void sp_item_hide (SPItem * item, GnomeCanvas * canvas);
void sp_item_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

/* Utility */

GnomeCanvasItem * sp_item_canvas_item (SPItem * item, GnomeCanvas * canvas);

void sp_item_request_canvas_update (SPItem * item);

gdouble * sp_item_i2d_affine (SPItem * item, gdouble affine[]);
void sp_item_set_i2d_affine (SPItem * item, gdouble affine[]);

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
