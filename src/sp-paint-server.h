#ifndef __SP_PAINT_SERVER_H__
#define __SP_PAINT_SERVER_H__

/*
 * Base class for gradients and patterns
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <libart_lgpl/art_rect.h>
#include "forward.h"
#include "sp-object.h"

typedef struct _SPPainter SPPainter;

#define SP_TYPE_PAINT_SERVER (sp_paint_server_get_type ())
#define SP_PAINT_SERVER(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_PAINT_SERVER, SPPaintServer))
#define SP_PAINT_SERVER_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_PAINT_SERVER, SPPaintServerClass))
#define SP_IS_PAINT_SERVER(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_PAINT_SERVER))
#define SP_IS_PAINT_SERVER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_PAINT_SERVER))

typedef enum {
	SP_PAINTER_IND,
	SP_PAINTER_DEP
} SPPainterType;

typedef void (* SPPainterFillFunc) (SPPainter *painter, guchar *px, gint x0, gint y0, gint width, gint height, gint rowstride);

/* fixme: I do not like that class thingie (Lauris) */
struct _SPPainter {
	SPPainter *next;
	SPPaintServer *server;
	GtkType server_type;
	SPPainterType type;
	SPPainterFillFunc fill;
};

struct _SPPaintServer {
	SPObject object;
	/* List of paints */
	SPPainter *painters;
};

struct _SPPaintServerClass {
	SPObjectClass sp_object_class;
	/* Get SPPaint instance */
	SPPainter * (* painter_new) (SPPaintServer *ps, const gdouble *affine, const ArtDRect *bbox);
	/* Free SPPaint instance */
	void (* painter_free) (SPPaintServer *ps, SPPainter *painter);
};

GtkType sp_paint_server_get_type (void);

SPPainter *sp_paint_server_painter_new (SPPaintServer *ps, const gdouble *affine, const ArtDRect *bbox);

#if 0
void sp_paint_server_painter_free (SPPaintServer *ps, SPPainter *painter);
#endif

SPPainter *sp_painter_free (SPPainter *painter);

END_GNOME_DECLS

#endif
