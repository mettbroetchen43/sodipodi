#ifndef __GNOME_CANVAS_ACETATE_H__
#define __GNOME_CANVAS_ACETATE_H__

/*
 * Infinite invisible canvas item
 *
 * Author:
 *   Federico Mena <federico@nuclecu.unam.mx>
 *   Raph Levien <raph@acm.org>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1998-1999 The Free Software Foundation
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_CANVAS_ACETATE (gnome_canvas_acetate_get_type ())
#define GNOME_CANVAS_ACETATE(obj) (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_ACETATE, GnomeCanvasAcetate))
#define GNOME_CANVAS_ACETATE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_ACETATE, GnomeCanvasAcetateClass))
#define GNOME_IS_CANVAS_ACETATE(obj) (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ACETATE))
#define GNOME_IS_CANVAS_ACETATE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_ACETATE))

typedef struct _GnomeCanvasAcetate GnomeCanvasAcetate;
typedef struct _GnomeCanvasAcetateClass GnomeCanvasAcetateClass;

#include "sp-canvas.h"

struct _GnomeCanvasAcetate {
	GnomeCanvasItem item;
};

struct _GnomeCanvasAcetateClass {
	GnomeCanvasItemClass parent_class;
};

GtkType gnome_canvas_acetate_get_type (void);

END_GNOME_DECLS

#endif
