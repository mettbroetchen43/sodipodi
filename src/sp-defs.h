#ifndef __SP_DEFS_H__
#define __SP_DEFS_H__

/*
 * SVG <defs> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define SP_TYPE_DEFS (sp_defs_get_type ())
#define SP_DEFS(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_DEFS, SPDefs))
#define SP_DEFS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DEFS, SPDefsClass))
#define SP_IS_DEFS(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_DEFS))
#define SP_IS_DEFS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DEFS))

#include "sp-object.h"

struct _SPDefs {
	SPObject object;
	SPObject *children;
};

struct _SPDefsClass {
	SPObjectClass parent_class;
};

GtkType sp_defs_get_type (void);

#endif
