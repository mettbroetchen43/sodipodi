#ifndef __SP_MARSHAL_H__
#define __SP_MARSHAL_H__

/*
 * Gtk+ signal marshallers
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <gtk/gtkobject.h>

void sp_marshal_NONE__DOUBLE_DOUBLE (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args);

#endif
