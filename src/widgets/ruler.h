#ifndef __SP_RULER_H__
#define __SP_RULER_H__

/*
 * Customized ruler class for sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtk/gtkruler.h>
#include "sp-metrics.h"

void sp_ruler_set_metric (GtkRuler * ruler, SPMetric  metric);

/* HRuler */

#define SP_TYPE_RULER (sp_ruler_get_type ())
#define SP_RULER(obj) GTK_CHECK_CAST (obj, sp_ruler_get_type (), SPRuler)
#define SP_IS_RULER(obj) GTK_CHECK_TYPE (obj, sp_ruler_get_type ())

typedef struct _SPRuler SPRuler;
typedef struct _SPRulerClass SPRulerClass;

struct _SPRuler
{
	GtkRuler ruler;
	unsigned int vertical : 1;
};

struct _SPRulerClass
{
	GtkRulerClass parent_class;
};

GtkType sp_ruler_get_type (void);
GtkWidget* sp_ruler_new (unsigned int vertical);

#endif
