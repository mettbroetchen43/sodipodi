#define __SODIPODI_CTRL_C__

/*
 * Visual handle for various things
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <math.h>

#include <libnr/nr-matrix.h>

#include "sp-canvas.h"
#include "sp-canvas-util.h"

#include "sodipodi-ctrl.h"

enum {
	ARG_0,
	ARG_SHAPE,
	ARG_MODE,
	ARG_ANCHOR,
	ARG_SIZE,
	ARG_FILLED,
	ARG_FILL_COLOR,
	ARG_STROKED,
	ARG_STROKE_COLOR,
	ARG_PIXBUF
};


static void sp_ctrl_class_init (SPCtrlClass *klass);
static void sp_ctrl_init (SPCtrl *ctrl);
static void sp_ctrl_destroy (GtkObject *object);
static void sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_ctrl_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags);
static void sp_ctrl_render (SPCanvasItem *item, SPCanvasBuf *buf);

static double sp_ctrl_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item);


static SPCanvasItemClass *parent_class;

GtkType
sp_ctrl_get_type (void)
{
	static GtkType ctrl_type = 0;
	if (!ctrl_type) {
		static const GTypeInfo ctrl_info = {
			sizeof (SPCtrlClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_ctrl_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPCtrl),
			0,	/* n_preallocs */
			(GInstanceInitFunc) sp_ctrl_init,
		};
		ctrl_type = g_type_register_static (SP_TYPE_CANVAS_ITEM, "SPCtrl", &ctrl_info, 0);
	}
	return ctrl_type;
}

static void
sp_ctrl_class_init (SPCtrlClass *klass)
{
	GtkObjectClass *object_class;
	SPCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (SPCanvasItemClass *) klass;

	parent_class = gtk_type_class (sp_canvas_item_get_type ());

	gtk_object_add_arg_type ("SPCtrl::shape", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_SHAPE);
	gtk_object_add_arg_type ("SPCtrl::mode", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_MODE);
	gtk_object_add_arg_type ("SPCtrl::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
	gtk_object_add_arg_type ("SPCtrl::size", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_SIZE);
	gtk_object_add_arg_type ("SPCtrl::pixbuf", GTK_TYPE_POINTER, GTK_ARG_READWRITE, ARG_PIXBUF);
	gtk_object_add_arg_type ("SPCtrl::filled", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_FILLED);
	gtk_object_add_arg_type ("SPCtrl::fill_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("SPCtrl::stroked", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_STROKED);
	gtk_object_add_arg_type ("SPCtrl::stroke_color", GTK_TYPE_INT, GTK_ARG_READWRITE, ARG_STROKE_COLOR);

	object_class->destroy = sp_ctrl_destroy;
	object_class->set_arg = sp_ctrl_set_arg;

	item_class->update = sp_ctrl_update;
	item_class->render = sp_ctrl_render;
	item_class->point = sp_ctrl_point;
}

static void
sp_ctrl_init (SPCtrl *ctrl)
{
	ctrl->shape = SP_CTRL_SHAPE_SQUARE;
	ctrl->mode = SP_CTRL_MODE_COLOR;
	ctrl->anchor = GTK_ANCHOR_CENTER;
	ctrl->span = 3;
	ctrl->defined = TRUE;
	ctrl->shown = FALSE;
	ctrl->build = FALSE;
	ctrl->filled = 1;
	ctrl->stroked = 0;
	ctrl->fill_color = 0x000000ff;
	ctrl->stroke_color = 0x000000ff;

	ctrl->box.x0 = ctrl->box.y0 = ctrl->box.x1 = ctrl->box.y1 = 0;
	ctrl->cache = NULL;
	ctrl->pixbuf = NULL;
}

static void
sp_ctrl_destroy (GtkObject *object)
{
	SPCtrl *ctrl;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CTRL (object));

	ctrl = SP_CTRL (object);

        if (ctrl->cache) {
          g_free (ctrl->cache);
          ctrl->cache = NULL;
        }

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_ctrl_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPCanvasItem *item;
	SPCtrl *ctrl;
	GdkPixbuf * pixbuf = NULL;

	item = SP_CANVAS_ITEM (object);
	ctrl = SP_CTRL (object);

	switch (arg_id) {
	case ARG_SHAPE:
		ctrl->shape = GTK_VALUE_INT (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_MODE:
		ctrl->mode = GTK_VALUE_INT (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_ANCHOR:
		ctrl->anchor = GTK_VALUE_INT (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_SIZE:
		ctrl->span = (gint) ((GTK_VALUE_DOUBLE (*arg) - 1.0) / 2.0 + 0.5);
		ctrl->defined = (ctrl->span > 0);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_FILLED:
		ctrl->filled = GTK_VALUE_BOOL (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_FILL_COLOR:
		ctrl->fill_color = GTK_VALUE_INT (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_STROKED:
		ctrl->stroked = GTK_VALUE_BOOL (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_STROKE_COLOR:
		ctrl->stroke_color = GTK_VALUE_INT (*arg);
		ctrl->build = FALSE;
		sp_canvas_item_request_update (item);
		break;
	case ARG_PIXBUF:
	        pixbuf  = GTK_VALUE_POINTER (*arg);
		if (gdk_pixbuf_get_has_alpha (pixbuf)) {
			ctrl->pixbuf = pixbuf;
		} else {
			ctrl->pixbuf = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);
			gdk_pixbuf_unref (pixbuf);
		}
		ctrl->build = FALSE;
		break;
	default:
		break;
	}
}

static void
sp_ctrl_update (SPCanvasItem *item, const NRMatrixD *ctm, unsigned int flags)
{
	SPCtrl *ctrl;
	gint x, y;

	ctrl = SP_CTRL (item);

	if (((SPCanvasItemClass *) parent_class)->update)
		(* ((SPCanvasItemClass *) parent_class)->update) (item, ctm, flags);

	sp_canvas_item_reset_bounds (item);

	if (ctrl->shown) {
		sp_canvas_request_redraw (item->canvas, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);
	}

	if (!ctrl->defined) return;

	x = (gint) (ctm->c[4] + 0.5) - ctrl->span;
	y = (gint) (ctm->c[5] + 0.5) - ctrl->span;

	switch (ctrl->anchor) {
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		break;
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		x += ctrl->span;
		break;
	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= (ctrl->span + 1);
		break;
	}

	switch (ctrl->anchor) {
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		break;
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		y += ctrl->span;
		break;
	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= (ctrl->span + 1);
		break;
	}

	ctrl->box.x0 = x;
	ctrl->box.y0 = y;
	ctrl->box.x1 = ctrl->box.x0 + 2 * ctrl->span;
	ctrl->box.y1 = ctrl->box.y0 + 2 * ctrl->span;

	sp_canvas_update_bbox (item, ctrl->box.x0, ctrl->box.y0, ctrl->box.x1 + 1, ctrl->box.y1 + 1);

}

static double
sp_ctrl_point (SPCanvasItem *item, double x, double y, SPCanvasItem **actual_item)
{
	SPCtrl *ctrl;

	ctrl = SP_CTRL (item);

	*actual_item = item;

	if ((x >= ctrl->box.x0) && (x <= ctrl->box.x1) && (y >= ctrl->box.y0) && (y <= ctrl->box.y1)) return 0.0;

	return 1e18;
}

static void
sp_ctrl_build_cache (SPCtrl *ctrl)
{
        guchar * p, *q;
	gint size, x, y, z, s, a, side, c;
	guint8 fr, fg, fb, fa, sr, sg, sb, sa;
	
	if (ctrl->filled) {
	       fr = (ctrl->fill_color >> 24) & 0xff;
	       fg = (ctrl->fill_color >> 16) & 0xff;
	       fb = (ctrl->fill_color >> 8) & 0xff;
	       fa = (ctrl->fill_color) & 0xff;
	} else { fr = 0x00; fg = 0x00; fb = 0x00; fa = 0x00; }
	if (ctrl->stroked) {
		sr = (ctrl->stroke_color >> 24) & 0xff;
		sg = (ctrl->stroke_color >> 16) & 0xff;
		sb = (ctrl->stroke_color >> 8) & 0xff;
		sa = (ctrl->stroke_color) & 0xff;
	} else { sr = fr; sg = fg; sb = fb; sa = fa; }


 	side = (ctrl->span * 2 +1);
	c = ctrl->span ;
	if (ctrl->cache) g_free (ctrl->cache);
	size = (side) * (side) * 4;
	ctrl->cache = g_malloc (size);
	if (side < 2) return;
	
	switch (ctrl->shape) {
	case SP_CTRL_SHAPE_SQUARE: 
		p = ctrl->cache;
		for (x=0; x < side; x++) { *p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; }
		for (y = 2; y < side; y++) {
			*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa;
			for (x=2; x < side; x++) { *p++ = fr; *p++ = fg; *p++ = fb; *p++ = fa; }
			*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa;
		}
		for (x=0; x < side; x++) { *p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; }
		ctrl->build = TRUE;
		break;
	case SP_CTRL_SHAPE_DIAMOND:
		p = ctrl->cache;
		for (y = 0; y < side; y++) {
			z = abs (c - y);
			for (x = 0; x < z; x++) {*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;}
			*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; x++; 
			for (; x < side - z -1; x++) { *p++ = fr; *p++ = fg; *p++ = fb; *p++ = fa; }
			if (z != c) {*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; x++;} 
			for (; x < side; x++) {*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;}
		}
		break;
	case SP_CTRL_SHAPE_CIRCLE:
		p = ctrl->cache;
		q = p + size -1;
		s = -1;
		for (y = 0; y <= c ; y++) {
			a = abs (c - y);
			z = (gint)(0.0 + sqrt ((c+0.5)*(c+0.5) - a*a));
			x = 0;
			while (x < c-z) {
				*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
				*q-- = 0x00; *q-- = 0x00; *q-- = 0x00; *q-- = 0x00; 
				x++;
			}
			do {
				*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa;   
				*q-- = sa; *q-- = sb; *q-- = sg; *q-- = sr;
				x++;
			} while (x < c-s);
			while (x < MIN(c+s+1, c+z)) {
				*p++ = fr;   *p++ = fg;   *p++ = fb;   *p++ = fa;   
				*q-- = fa;   *q-- = fb;   *q-- = fg;   *q-- = fr;   
				x++;
			}
			do {
				*p++ = sr;   *p++ = sg;   *p++ = sb;   *p++ = sa;   
				*q-- = sa; *q-- = sb; *q-- = sg; *q-- = sr;
				x++;
			} while (x <= c+z);
			while (x < side) {
				*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00; 
				*q-- = 0x00; *q-- = 0x00; *q-- = 0x00; *q-- = 0x00; 
				x++;
			}
			s = z;
		}
		ctrl->build = TRUE;
		break;
	case SP_CTRL_SHAPE_CROSS:
		p = ctrl->cache;
		for (y = 0; y < side; y++) {
			z = abs (c - y);
			for (x = 0; x < c-z; x++) {*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;}
			*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; x++; 
			for (; x < c + z; x++) {*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;}
			if (z != 0) {*p++ = sr; *p++ = sg; *p++ = sb; *p++ = sa; x++;} 
			for (; x < side; x++) {*p++ = 0x00; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;}
		}
		ctrl->build = TRUE;
		break;
	case SP_CTRL_SHAPE_BITMAP:
		if (ctrl->pixbuf) {
			unsigned char *px;
			unsigned int rs;
			px = gdk_pixbuf_get_pixels (ctrl->pixbuf);
			rs = gdk_pixbuf_get_rowstride (ctrl->pixbuf);
			for (y = 0; y < side; y++){
				unsigned char *s, *d;
				s = px + y * rs;
				d = ctrl->cache + 4 * side * y;
				for (x = 0; x < side; x++) {
					if (s[3] < 0x80) {
						d[0] = 0x00;
						d[1] = 0x00;
						d[2] = 0x00;
						d[3] = 0x00;
					} else if (s[0] < 0x80) {
						d[0] = sr;
						d[1] = sg;
						d[2] = sb;
						d[3] = sa;
					} else {
						d[0] = fr;
						d[1] = fg;
						d[2] = fb;
						d[3] = fa;
					}
					s += 4;
					d += 4;
				}
			}
		} else {
			g_print ("control has no pixmap\n");
		}
		ctrl->build = TRUE;
		break;
	case SP_CTRL_SHAPE_IMAGE:
		if (ctrl->pixbuf) {
			guint r = gdk_pixbuf_get_rowstride (ctrl->pixbuf);
			guchar * pix;
			q = gdk_pixbuf_get_pixels (ctrl->pixbuf);
			p = ctrl->cache;
			for (y = 0; y < side; y++){
				pix = q + (y * r);
				for (x = 0; x < side; x++) {
					*p++ = *pix++;
					*p++ = *pix++;
					*p++ = *pix++;
					*p++ = *pix++;
				}
			}
		} else { g_print ("control has no pixmap\n"); } 
		ctrl->build = TRUE;
		break;
	default:
		break;
	}
	
}

#define COMPOSEP11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * 255 + 127) / 255)
#define COMPOSEN11(fc,fa,bc) (((255 - (fa)) * (bc) + (fc) * (fa) + 127) / 255)

static void
sp_ctrl_render (SPCanvasItem *item, SPCanvasBuf *buf)
{
	SPCtrl *ctrl;
	NRPixBlock *pb;
	int y0, y1, y, x0,x1,x;
	unsigned char *p, *q, a;
	
	ctrl = SP_CTRL (item);
	
	if (!ctrl->defined) return;
	if ((!ctrl->filled) && (!ctrl->stroked)) return;
	
	sp_canvas_buf_ensure_buf (buf);
	
	/* the control-image is rendered into ctrl->cache */
	if (!ctrl->build) sp_ctrl_build_cache (ctrl);
	
	pb = &buf->pixblock;
	/* then we render from ctrl->cache */
	y0 = MAX (ctrl->box.y0, pb->area.y0);
	y1 = MIN (ctrl->box.y1, pb->area.y1 - 1);
	x0 = MAX (ctrl->box.x0, pb->area.x0);
	x1 = MIN (ctrl->box.x1, pb->area.x1 - 1);
	for (y = y0; y <= y1; y++) {
		p = NR_PIXBLOCK_PX (pb) + (y - pb->area.y0) * pb->rs + (x0 - pb->area.x0) * 3;
		q = ctrl->cache + ((y - ctrl->box.y0) * (ctrl->span * 2 + 1) + (x0 - ctrl->box.x0)) * 4;
		for (x = x0; x <= x1; x++) {
			a = *(q + 3);
			switch (ctrl->mode) {
			case SP_CTRL_MODE_COLOR:
				p[0] = COMPOSEN11 (q[0], a, p[0]);
				p[1] = COMPOSEN11 (q[1], a, p[1]);
				p[2] = COMPOSEN11 (q[2], a, p[2]);
				q += 4;
				p += 3;
				break;
			case SP_CTRL_MODE_XOR:
				p[0] = COMPOSEN11 (q[0], a, p[0] ^ q[0]);
				p[1] = COMPOSEN11 (q[1], a, p[1] ^ q[1]);
				p[2] = COMPOSEN11 (q[2], a, p[2] ^ q[2]);
				q += 4;
				p += 3;
				break;
			}
		}
	}
	ctrl->shown = TRUE;
}

void
sp_ctrl_moveto (SPCtrl * ctrl, double x, double y)
{
	NRMatrixD transform;

	nr_matrix_d_set_translate (&transform, x, y);

	sp_canvas_item_affine_absolute (SP_CANVAS_ITEM (ctrl), &transform);
}
