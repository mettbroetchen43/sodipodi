#ifndef __SP_OBJECTGROUP_H__
#define __SP_OBJECTGROUP_H__

/*
 * Abstract base class for non-item groups
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define SP_TYPE_OBJECTGROUP (sp_objectgroup_get_type ())
#define SP_OBJECTGROUP(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_OBJECTGROUP, SPObjectGroup))
#define SP_OBJECTGROUP_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_OBJECTGROUP, SPObjectGroupClass))
#define SP_IS_OBJECTGROUP(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_OBJECTGROUP))
#define SP_IS_OBJECTGROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_OBJECTGROUP))

#include "sp-object.h"

struct _SPObjectGroup {
	SPObject object;
	SPObject *children;
};

struct _SPObjectGroupClass {
	SPObjectClass parent_class;
};

GtkType sp_objectgroup_get_type (void);

#endif
