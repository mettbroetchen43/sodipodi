#ifndef __SODIPODI_CTRLRECT_H__
#define __SODIPODI_CTRLRECT_H__

/*
 * Simple non-transformed rectangle, usable for rubberband
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <libart_lgpl/art_rect.h>
#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS

#define SP_TYPE_CTRLRECT (sp_ctrlrect_get_type ())
#define SP_CTRLRECT(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_CTRLRECT, SPCtrlRect))
#define SP_CTRLRECT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CTRLRECT, SPCtrlRectClass))
#define SP_IS_CTRLRECT(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_CTRLRECT))
#define SP_IS_CTRLRECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CTRLRECT))

typedef struct _SPCtrlRect SPCtrlRect;
typedef struct _SPCtrlRectClass SPCtrlRectClass;

struct _SPCtrlRect {
	GnomeCanvasItem item;

	ArtDRect rect;		/* Dimensions */
	
	double width;		/* Line width */

	ArtIRect irect;
};

struct _SPCtrlRectClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType sp_ctrlrect_get_type (void);

void
sp_ctrlrect_set_rect (SPCtrlRect * rect, ArtDRect * box);

END_GNOME_DECLS

#endif
