#ifndef SP_PATH_H
#define SP_PATH_H

#include "sp-path-component.h"
#include "sp-item.h"

#define SP_TYPE_PATH            (sp_path_get_type ())
#define SP_PATH(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_PATH, SPPath))
#define SP_PATH_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_PATH, SPPathClass))
#define SP_IS_PATH(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_PATH))
#define SP_IS_PATH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_PATH))

struct _SPPath {
	SPItem item;
	GSList * comp;
	gboolean independent;
};

struct _SPPathClass {
	SPItemClass item_class;
	void (* remove_comp) (SPPath * path, SPPathComp * comp);
	void (* add_comp) (SPPath * path, SPPathComp * comp);
	void (* change_bpath) (SPPath * path, SPPathComp * comp, SPCurve * curve);
};


/* Standard Gtk function */
GtkType sp_path_get_type (void);

#define sp_path_independent(p) (p->independent)

/* methods */

void sp_path_remove_comp (SPPath * path, SPPathComp * comp);
void sp_path_add_comp (SPPath * path, SPPathComp * comp);
void sp_path_change_bpath (SPPath * path, SPPathComp * comp, SPCurve * curve);

/* Utility */

void sp_path_clear (SPPath * path);
void sp_path_add_bpath (SPPath * path, SPCurve * curve, gboolean private, gdouble affine[]);

/* normalizes path - i.e. concatenates all component bpaths. */
/* returns newly allocated bpath - caller has to manage it */

SPCurve * sp_path_normalized_bpath (SPPath * path);

/* creates single component normalized path with private component */

SPCurve * sp_path_normalize (SPPath * path);

/* fixme: it works only for single component (normalized) private paths? */
void sp_path_bpath_modified (SPPath * path, SPCurve * curve);

#endif
