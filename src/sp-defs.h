#ifndef SP_DEFS_H
#define SP_DEFS_H

/*
 * SPDefs
 *
 * SVG <defs> node implementation
 *
 * Copyright (C) Lauris Kaplinski 2000
 * Released under GNU GPL
 */

#include "sp-object.h"

#define SP_TYPE_DEFS            (sp_defs_get_type ())
#define SP_DEFS(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DEFS, SPDefs))
#define SP_DEFS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DEFS, SPDefsClass))
#define SP_IS_DEFS(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DEFS))
#define SP_IS_DEFS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DEFS))

struct _SPDefs {
	SPObject object;
	SPObject * children;
};

struct _SPDefsClass {
	SPObjectClass parent_class;
};

/* Standard Gtk function */

GtkType sp_defs_get_type (void);

#endif
