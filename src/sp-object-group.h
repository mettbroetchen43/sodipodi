#ifndef __SP_OBJECTGROUP_H__
#define __SP_OBJECTGROUP_H__

/*
 * Abstract base class for non-item groups
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object.h"

#define SP_TYPE_OBJECTGROUP (sp_objectgroup_get_type ())
#define SP_OBJECTGROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_OBJECTGROUP, SPObjectGroup))
#define SP_IS_OBJECTGROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_OBJECTGROUP))

struct _SPObjectGroup {
	SPObject object;
};

struct _SPObjectGroupClass {
	SPObjectClass parent_class;
};

GType sp_objectgroup_get_type (void);

#endif
