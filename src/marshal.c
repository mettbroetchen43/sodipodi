#define __SP_MARSHAL_C__

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

#include "marshal.h"

typedef gint (* SPSignal_NONE__DOUBLE_DOUBLE) (GtkObject *object, gdouble arg1, gdouble arg2, gpointer user_data);

void
sp_marshal_NONE__DOUBLE_DOUBLE (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	SPSignal_NONE__DOUBLE_DOUBLE rfunc;

	rfunc = (SPSignal_NONE__DOUBLE_DOUBLE) func;

	(* rfunc) (object, GTK_VALUE_DOUBLE (args[0]), GTK_VALUE_DOUBLE (args[1]), func_data);
}


