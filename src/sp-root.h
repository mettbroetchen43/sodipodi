#ifndef SP_ROOT_H
#define SP_ROOT_H

/*
 * SPRoot is child of SPGroup, its main reason is to have a object
 * which renders itself to <svg></svg>
 * It does not have any attributes
 * It's parend DOES NOT have to be SPGroup
 *
 * Idea: should we derive SPGroup from SPRoot?
 */

#include "sp-item.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_ROOT            (sp_root_get_type ())
#define SP_ROOT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_ROOT, SPRoot))
#define SP_ROOT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_ROOT, SPRootClass))
#define SP_IS_ROOT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_ROOT))
#define SP_IS_ROOT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_ROOT))

#ifndef SP_ROOT_DEFINED
#define SP_ROOT_DEFINED
typedef struct _SPRoot SPRoot;
typedef struct _SPRootClass SPRootClass;
#endif

struct _SPRoot {
	SPGroup group;
	double width, height;
	ArtDRect viewbox;
	GSList * namedviews;
};

struct _SPRootClass {
	SPGroupClass parent_class;
};


/* Standard Gtk function */

GtkType sp_root_get_type (void);

END_GNOME_DECLS

#endif
