#ifndef __SP_USE_H__
#define __SP_USE_H__

/*
 * SVG <svg> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-item.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_USE            (sp_use_get_type ())
#define SP_USE(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_USE, SPUse))
#define SP_USE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_USE, SPUseClass))
#define SP_IS_USE(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_USE))
#define SP_IS_USE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_USE))

typedef struct _SPUse SPUse;
typedef struct _SPUseClass SPUseClass;

struct _SPUse {
	SPItem item;
	SPObject *child;
	gdouble x, y, width, height;
	guchar *href;
};

struct _SPUseClass {
	SPItemClass parent_class;
};

GtkType sp_use_get_type (void);

END_GNOME_DECLS

#endif
