#ifndef __SODIPODI_CTRLLINE_H__
#define __SODIPODI_CTRLLINE_H__

/*
 * Simple visual line for various things
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "sp-canvas.h"

G_BEGIN_DECLS

#define SP_TYPE_CTRLLINE (sp_ctrlline_get_type ())
#define SP_CTRLLINE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CTRLLINE, SPCtrlLine))
#define SP_IS_CTRLLINE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRLLINE))

typedef struct _SPCtrlLine SPCtrlLine;
typedef struct _SPCtrlLineClass SPCtrlLineClass;

GtkType sp_ctrlline_get_type (void);

void sp_ctrlline_set_rgba32 (SPCtrlLine *cl, guint32 rgba);
void sp_ctrlline_set_coords (SPCtrlLine *cl, float x0, float y0, float x1, float y1);

G_END_DECLS

#endif
