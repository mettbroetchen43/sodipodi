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

#ifndef SP_OBJECT_DEFINED
#define SP_OBJECT_DEFINED
typedef struct _SPObject SPObject;
typedef struct _SPObjectClass SPObjectClass;
#endif

#ifndef SP_DOCUMENT_DEFINED
#define SP_DOCUMENT_DEFINED
typedef struct _SPDocument SPDocument;
typedef struct _SPDocumentClass SPDocumentClass;
#endif

#define SP_TYPE_OBJECT            (sp_object_get_type ())
#define SP_OBJECT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_OBJECT, SPObject))
#define SP_OBJECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECT, SPObjectClass))
#define SP_IS_OBJECT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_OBJECT))
#define SP_IS_OBJECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECT))

#define SP_OBJECT_CLONED_FLAG (1 << 4)
#define SP_OBJECT_IS_CLONED(o) (GTK_OBJECT_FLAGS (o) & SP_OBJECT_CLONED_FLAG)

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>
#include "xml/repr.h"

struct _SPObject {
	GtkObject object;
	SPDocument * document;		/* Document we are part of */
	SPObject * parent;		/* Our parent (only one allowed) */
	SPRepr * repr;			/* Our xml representation */
	gchar * id;			/* Our very own unique id */
	const gchar * title;		/* Our title, if any */
	const gchar * description;	/* Our description, if any */
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

void sp_object_invoke_build (SPObject * object, SPDocument * document, SPRepr * repr, gboolean cloned);
void sp_object_invoke_read_attr (SPObject * object, const gchar * key);


#endif
