#ifndef SP_MDI_CHILD_H
#define SP_MDI_CHILD_H

/*
 * SPMDIChild
 *
 * A per view object
 *
 */

#include <libgnomeui/gnome-mdi-child.h>
#include "document.h"

#define SP_TYPE_MDI_CHILD            (sp_mdi_child_get_type ())
#define SP_MDI_CHILD(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_MDI_CHILD, SPMDIChild))
#define SP_MDI_CHILD_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_MDI_CHILD, SPMDIChildClass))
#define SP_IS_MDI_CHILD(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_MDI_CHILD))
#define SP_IS_MDI_CHILD_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_MDI_CHILD))

typedef struct _SPMDIChild SPMDIChild;
typedef struct _SPMDIChildClass SPMDIChildClass;

struct _SPMDIChild {
	GnomeMDIChild mdi_child;
	SPDocument * document;
};

struct _SPMDIChildClass {
	GnomeMDIChildClass parent_class;
};

/* Standard Gtk function */

GtkType sp_mdi_child_get_type (void);

/* Constructor */

SPMDIChild * sp_mdi_child_new (SPDocument * document);

#endif
