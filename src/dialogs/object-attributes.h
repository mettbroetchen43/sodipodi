#ifndef __SP_OBJECT_ATTRIBUTES_H__
#define __SP_OBJECT_ATTRIBUTES_H__

/*
 * Generic object attribute editor
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Licensed under GNU GPL
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <gtk/gtkwidget.h>
#include "../forward.h"

void sp_object_attributes_dialog (SPObject *object, const guchar *tag);

END_GNOME_DECLS

#endif
