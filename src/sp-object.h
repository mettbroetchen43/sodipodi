#ifndef SP_OBJECT_H
#define SP_OBJECT_H

/*
 * SPObject
 *
 * This is most abstract of all typed objects
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include <gtk/gtktypeutils.h>
#include "xml/repr.h"
#include "document.h"

#ifndef SP_OBJECT_DEFINED
#define SP_OBJECT_DEFINED
typedef struct _SPObject SPObject;
typedef struct _SPObjectClass SPObjectClass;
#endif

#define SP_TYPE_OBJECT            (sp_object_get_type ())
#define SP_OBJECT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_OBJECT, SPObject))
#define SP_OBJECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECT, SPObjectClass))
#define SP_IS_OBJECT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_OBJECT))
#define SP_IS_OBJECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECT))

struct _SPObject {
	GtkObject object;
	SPDocument * document;		/* Document we are part of */
	SPObject * parent;		/* Our immediate parent */
	gint order;			/* Our order among siblings */
	SPRepr * repr;			/* Our xml representation */
	gchar * id;			/* Our very own unique id */
};

struct _SPObjectClass {
	GtkObjectClass parent_class;
	void (* build) (SPObject * object, SPDocument * document, SPRepr * repr);

	/* Virtual handlers of repr signals */
	void (* read_attr) (SPObject * object, const gchar * key);
	void (* read_content) (SPObject * object);
	void (* set_order) (SPObject * object);
	void (* add_child) (SPObject * object, SPRepr * child);
	void (* remove_child) (SPObject * object, SPRepr * child);
};

GtkType sp_object_get_type (void);

void sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr);
void sp_object_invoke_read_attr (SPObject * object, const gchar * key);

#if 0
/* Constructors */

SPObject * sp_object_new_root (SPRepr * repr);
SPObject * sp_object_new (SPRepr * repr, SPGroup * parent);

/* Methods */

void sp_object_update (SPObject * object, gdouble affine[]);
void sp_object_bbox (SPObject * object, ArtDRect * bbox);
gchar * sp_object_description (SPObject * object);
void sp_object_print (SPObject * object, GnomePrintContext * gpc);
void sp_object_read (SPObject * object, SPRepr * repr);
void sp_object_read_attr (SPObject * object, SPRepr * repr, const gchar * key);
GnomeCanvasObject * sp_object_show (SPObject * object, GnomeCanvasGroup * canvas_group, gpointer handler);
void sp_object_hide (SPObject * object, GnomeCanvas * canvas);
void sp_object_paint (SPObject * object, ArtPixBuf * buf, gdouble affine[]);

/* Utility */

GnomeCanvasObject * sp_object_canvas_object (SPObject * object, GnomeCanvas * canvas);

void sp_object_request_canvas_update (SPObject * object);

gdouble * sp_object_i2d_affine (SPObject * object, gdouble affine[]);
void sp_object_set_i2d_affine (SPObject * object, gdouble affine[]);
gdouble * sp_object_i2doc_affine (SPObject * object, gdouble affine[]);

/* group */

#define SP_TYPE_GROUP            (sp_group_get_type ())
#define SP_GROUP(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_GROUP, SPGroup))
#define SP_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_GROUP, SPGroupClass))
#define SP_IS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_GROUP))
#define SP_IS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_GROUP))

struct _SPGroup {
	SPObject object;
	GSList * children;
	gboolean transparent;
};

struct _SPGroupClass {
	SPObjectClass parent_class;
};


/* Standard Gtk function */

GtkType sp_group_get_type (void);
#endif

#endif
