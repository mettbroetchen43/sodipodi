#ifndef __SODIPODI_CTRLLINE_H__
#define __SODIPODI_CTRLLINE_H__

/*
 * Simple straight line
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include "sp-canvas.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_CTRLLINE (sp_ctrlline_get_type ())
#define SP_CTRLLINE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CTRLLINE, SPCtrlLine))
#define SP_IS_CTRLLINE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRLLINE))

typedef struct _SPCtrlLine SPCtrlLine;
typedef struct _SPCtrlLineClass SPCtrlLineClass;

GtkType sp_ctrlline_get_type (void);

void sp_ctrlline_set_rgba32 (SPCtrlLine *cl, guint32 rgba);
void sp_ctrlline_set_coords (SPCtrlLine *cl, gdouble x0, gdouble y0, gdouble x2, gdouble y1);

END_GNOME_DECLS

#endif
