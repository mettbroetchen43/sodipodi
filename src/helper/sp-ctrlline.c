#define SODIPODI_CTRLLINE_C

/*
 * SPCtrlLine
 *
 * Very spartan straight line.
 *
 * TODO:
 * Draw it by hand - we really do not need aa stuff for it
 *
 */

#include <math.h>
#include <gtk/gtk.h>
#include <libgnomeui/gnome-canvas.h>
#include <libgnomeui/gnome-canvas-util.h>
#include "sp-ctrlline.h"

#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>

static void sp_ctrlline_class_init (SPCtrlLineClass *klass);
static void sp_ctrlline_init (SPCtrlLine *ctrlline);
static void sp_ctrlline_destroy (GtkObject *object);

static void sp_ctrlline_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void sp_ctrlline_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static double sp_ctrlline_point (GnomeCanvasItem *item, double x, double y,
			     int cx, int cy, GnomeCanvasItem **actual_item);


static GnomeCanvasItemClass *parent_class;

GtkType
sp_ctrlline_get_type (void)
{
	static GtkType ctrlline_type = 0;

	if (!ctrlline_type) {
		GtkTypeInfo ctrlline_info = {
			"SPCtrlLine",
			sizeof (SPCtrlLine),
			sizeof (SPCtrlLineClass),
			(GtkClassInitFunc) sp_ctrlline_class_init,
			(GtkObjectInitFunc) sp_ctrlline_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		ctrlline_type = gtk_type_unique (gnome_canvas_item_get_type (), &ctrlline_info);
	}
	return ctrlline_type;
}

static void
sp_ctrlline_class_init (SPCtrlLineClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	object_class->destroy = sp_ctrlline_destroy;

	item_class->update = sp_ctrlline_update;
	item_class->render = sp_ctrlline_render;
	item_class->point = sp_ctrlline_point;
}

static void
sp_ctrlline_init (SPCtrlLine *ctrlline)
{
	ctrlline->s.x = ctrlline->s.y = ctrlline->e.x = ctrlline->e.y = 0.0;
	ctrlline->svp = NULL;
}

static void
sp_ctrlline_destroy (GtkObject *object)
{
	SPCtrlLine *ctrlline;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRLLINE (object));

	ctrlline = SP_CTRLLINE (object);

	if (ctrlline->svp)
		art_svp_free (ctrlline->svp);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrlline_render (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	SPCtrlLine *ctrlline;

	ctrlline = SP_CTRLLINE (item);

	if (ctrlline->svp)
		gnome_canvas_render_svp (buf, ctrlline->svp, 0x0000ffff);
}

static void
sp_ctrlline_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCtrlLine *cl;
	ArtPoint p;
	ArtVpath vpath[3];
	ArtSVP *svp;

	cl = SP_CTRLLINE (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	gnome_canvas_item_reset_bounds (item);

	p.x = cl->s.x;
	p.y = cl->s.y;
	art_affine_point (&p, &p, affine);

	vpath[0].code = ART_MOVETO_OPEN;
	vpath[0].x = p.x;
	vpath[0].y = p.y;

	p.x = cl->e.x;
	p.y = cl->e.y;
	art_affine_point (&p, &p, affine);

	vpath[1].code = ART_LINETO;
	vpath[1].x = p.x;
	vpath[1].y = p.y;

	vpath[2].code = ART_END;

	svp = art_svp_vpath_stroke (vpath,
		ART_PATH_STROKE_CAP_BUTT,
		ART_PATH_STROKE_JOIN_MITER,
		1, 4, 0.25);
	gnome_canvas_item_update_svp_clip (item, &cl->svp, svp, clip_path);
	gnome_canvas_item_request_redraw_svp (item, cl->svp);
}

static double
sp_ctrlline_point (GnomeCanvasItem *item, double x, double y,
	       int cx, int cy, GnomeCanvasItem **actual_item)
{
	return 1000.0;
}

void
sp_ctrlline_set_coords (SPCtrlLine * cl, double x0, double y0, double x1, double y1)
{
	cl->s.x = x0;
	cl->s.y = y0;
	cl->e.x = x1;
	cl->e.y = y1;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (cl));
}
