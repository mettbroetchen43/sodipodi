#ifndef __NR_OBJECT_H__
#define __NR_OBJECT_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib-object.h>

#define NR_CHECK_INSTANCE_CAST G_TYPE_CHECK_INSTANCE_CAST
#define NR_CHECK_INSTANCE_TYPE G_TYPE_CHECK_INSTANCE_TYPE

typedef struct _GObject NRObject;
typedef struct _GObjectClass NRObjectClass;

#endif
