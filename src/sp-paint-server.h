#ifndef __SP_PAINT_SERVER_H__
#define __SP_PAINT_SERVER_H__

/*
 * SPPaintServer
 *
 * Abstract base class for different paint types
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <libart_lgpl/art_rect.h>
#include "sp-object.h"

typedef struct _SPPaintServer SPPaintServer;
typedef struct _SPPaintServerClass SPPaintServerClass;
typedef struct _SPPainter SPPainter;

#define SP_TYPE_PAINT_SERVER (sp_paint_server_get_type ())
#define SP_PAINT_SERVER(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_PAINT_SERVER, SPPaintServer))
#define SP_PAINT_SERVER_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_PAINT_SERVER, SPPaintServerClass))
#define SP_IS_PAINT_SERVER(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_PAINT_SERVER))
#define SP_IS_PAINT_SERVER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_PAINT_SERVER))

typedef enum {
	SP_PAINT_IND,
	SP_PAINT_DEP
} SPPainterType;

struct _SPPainter {
	SPPainter *next;
	SPPainterType type;
};

struct _SPPaintServer {
	SPObject object;
	/* List of paints */
	SPPainter *painters;
};

struct _SPPaintServerClass {
	SPObjectClass sp_object_class;
	/* Get SPPaint instance */
	SPPainter * (* painter_new) (SPPaintServer *ps, gdouble *affine, ArtDRect *bbox);
	/* Free SPPaint instance */
	void (* painter_free) (SPPaintServer *ps, SPPainter *painter);
};

GtkType sp_paint_server_get_type (void);

SPPainter *sp_paint_server_painter_new (SPPaintServer *ps, gdouble *affine, ArtDRect *bbox);
void sp_paint_server_painter_free (SPPaintServer *ps, SPPainter *painter);

END_GNOME_DECLS

#endif
