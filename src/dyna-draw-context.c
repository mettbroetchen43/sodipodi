/*
 * dyna-draw-context handlers
 *
 * todo: use intelligent bpathing, i.e. without copying def all the time
 *       make dynahand generating curves instead of lines
 *
 * Copyright (C) Mitsuru Oka (oka326@parkcity.ne.jp), 2001
 * Copyright (C) Lauris Kaplinski (lauris@kaplinski.com), 1999-2000
 *
 * Contains pieces of code published in:
 * "Graphics Gems", Academic Press, 1990 by Philip J. Schneider
 *
 * Dynadraw/Gtk+ code came from:
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Authors: Federico Mena <federico@nuclecu.unam.mx> - Port to Gtk+
 *          Nat Friedman <ndf@mit.edu> - Original port to Xlib
 *          Paul Haeberli <paul@sgi.com> - Original Dynadraw code for GL
 */
#define SP_DYNA_DRAW_CONTEXT_C

/*
 * TODO: Tue Oct  2 22:57:15 2001
 *  - Decide control point behavior when use_calligraphic==1.
 *  - Option dialog box support if it is availabe.
 *  - Decide to use NORMALIZED_COORDINATE or not.
 *  - Remove hard coded sytle attributes and move to customizable style.
 *  - Bug fix.
 */

#define noDYNA_DRAW_DEBUG
#define noDYNA_DRAW_VERBOSE
#define NORMALIZED_COORDINATE


#include <math.h>
#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/curve.h"
#include "helper/sodipodi-ctrl.h"
#include "display/canvas-shape.h"
#include "helper/bezier-utils.h"

#include "sodipodi.h"
#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "dyna-draw-context.h"


#define SAMPLE_TIMEOUT            10
#define TOLERANCE_LINE            1.0
#define TOLERANCE_CALLIGRAPHIC     3.0
#define DYNA_EPSILON   1.0e-6
#ifdef NORMALIZED_COORDINATE
#define DYNA_MIN_WIDTH 1.0e-6
#else
#define DYNA_MIN_WIDTH 1.0
#endif


static void sp_dyna_draw_context_class_init (SPDynaDrawContextClass * klass);
static void sp_dyna_draw_context_init (SPDynaDrawContext * dyna_draw_context);
static void sp_dyna_draw_context_destroy (GtkObject * object);

static void sp_dyna_draw_context_setup (SPEventContext * event_context,
					SPDesktop * desktop);
static gint sp_dyna_draw_context_root_handler (SPEventContext * event_context,
					       GdkEvent * event);
static gint sp_dyna_draw_context_item_handler (SPEventContext * event_context,
					       SPItem * item,
					       GdkEvent * event);

static void clear_current (SPDynaDrawContext * dc);
static void set_to_accumulated (SPDynaDrawContext * dc);
static void concat_current_line (SPDynaDrawContext * dc);
static void accumulate_calligraphic (SPDynaDrawContext * dc);

static void test_inside (SPDynaDrawContext * dc, double x, double y);
static void move_ctrl (SPDynaDrawContext * dc, double x, double y);
static void remove_ctrl (SPDynaDrawContext * dc);

static void fit_and_split (SPDynaDrawContext * dc, gboolean release);
static void fit_and_split_line (SPDynaDrawContext * dc, gboolean release);
static void fit_and_split_calligraphics (SPDynaDrawContext * dc, gboolean release);

static void sp_dyna_draw_reset (SPDynaDrawContext * dc, double x, double y);
static void sp_dyna_draw_get_npoint (const SPDynaDrawContext *dc,
                                     gdouble            vx,
                                     gdouble            vy,
                                     gdouble           *nx,
                                     gdouble           *ny);
static void sp_dyna_draw_get_vpoint (const SPDynaDrawContext *dc,
                                     gdouble            nx,
                                     gdouble            ny,
                                     gdouble           *vx,
                                     gdouble           *vy);
static void sp_dyna_draw_get_curr_vpoint (const SPDynaDrawContext * dc,
                                          gdouble *vx,
                                          gdouble *vy);
static void draw_temporary_box (SPDynaDrawContext *dc);


static SPEventContextClass *parent_class;

GtkType
sp_dyna_draw_context_get_type (void)
{
  static GtkType dyna_draw_context_type = 0;

  if (!dyna_draw_context_type)
    {

      static const GtkTypeInfo dyna_draw_context_info = {
	"SPDynaDrawContext",
	sizeof (SPDynaDrawContext),
	sizeof (SPDynaDrawContextClass),
	(GtkClassInitFunc) sp_dyna_draw_context_class_init,
	(GtkObjectInitFunc) sp_dyna_draw_context_init,
	NULL,
	NULL,
	(GtkClassInitFunc) NULL
      };

      dyna_draw_context_type =
	gtk_type_unique (sp_event_context_get_type (),
			 &dyna_draw_context_info);
    }

  return dyna_draw_context_type;
}

static void
sp_dyna_draw_context_class_init (SPDynaDrawContextClass * klass)
{
  GtkObjectClass *object_class;
  SPEventContextClass *event_context_class;

  object_class = (GtkObjectClass *) klass;
  event_context_class = (SPEventContextClass *) klass;

  parent_class = gtk_type_class (sp_event_context_get_type ());

  object_class->destroy = sp_dyna_draw_context_destroy;

  event_context_class->setup = sp_dyna_draw_context_setup;
  event_context_class->root_handler = sp_dyna_draw_context_root_handler;
  event_context_class->item_handler = sp_dyna_draw_context_item_handler;
}

static void
sp_dyna_draw_context_init (SPDynaDrawContext * dc)
{
  dc->accumulated = NULL;
  dc->segments = NULL;
  dc->currentcurve = NULL;
  dc->currentshape = NULL;
  dc->npoints = 0;
  dc->cal1 = NULL;
  dc->cal2 = NULL;
  dc->repr = NULL;
  dc->citem = NULL;
  dc->ccolor = 0xff0000ff;
  dc->cinside = -1;

  /* DynaDraw values */
  dc->curx = 0.0;
  dc->cury = 0.0;
  dc->lastx = 0.0;
  dc->lasty = 0.0;
  dc->velx = 0.0;
  dc->vely = 0.0;
  dc->accx = 0.0;
  dc->accy = 0.0;
  dc->angx = 0.0;
  dc->angy = 0.0;
  dc->delx = 0.0;
  dc->dely = 0.0;

  /* attributes */
  dc->fixed_angle = FALSE;
  dc->use_timeout = FALSE;
  dc->use_calligraphic = TRUE;
  dc->timer_id = 0;
  dc->dragging = FALSE;
  dc->dynahand = FALSE;
#ifdef NORMALIZED_COORDINATE
  dc->mass = 0.3;
  dc->drag = 0.5;
  dc->angle = 30.0;
  dc->width = 0.2;
#else
  dc->mass = 0.2;
  dc->drag = 0.5;
  dc->angle = 30.0;
  dc->width = 0.1;
#endif
}

static void
sp_dyna_draw_context_destroy (GtkObject * object)
{
  SPDynaDrawContext *dc;

  dc = SP_DYNA_DRAW_CONTEXT (object);

  if (dc->accumulated)
    sp_curve_unref (dc->accumulated);

  while (dc->segments)
    {
      gtk_object_destroy (GTK_OBJECT (dc->segments->data));
      dc->segments = g_slist_remove (dc->segments, dc->segments->data);
    }

  if (dc->currentcurve)
    sp_curve_unref (dc->currentcurve);

  if (dc->cal1)
    sp_curve_unref (dc->cal1);
  if (dc->cal2)
    sp_curve_unref (dc->cal2);

  if (dc->currentshape)
    gtk_object_destroy (GTK_OBJECT (dc->currentshape));

  if (dc->citem)
    gtk_object_destroy (GTK_OBJECT (dc->citem));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_dyna_draw_context_setup (SPEventContext * event_context,
			    SPDesktop * desktop)
{
  SPDynaDrawContext *dc;
  SPStyle *style;

  if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
    SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

  dc = SP_DYNA_DRAW_CONTEXT (event_context);

  dc->accumulated = sp_curve_new_sized (32);
  dc->currentcurve = sp_curve_new_sized (4);

  dc->cal1 = sp_curve_new_sized (32);
  dc->cal2 = sp_curve_new_sized (32);

  /* fixme: */
  style = sp_style_new (NULL);
  /* style should be changed when dc->use_calligraphc is touched */  
#ifdef DYNA_DRAW_DEBUG
  style->fill.type = SP_PAINT_TYPE_NONE;
  style->stroke.type = SP_PAINT_TYPE_COLOR;
#else
  style->fill.type = SP_PAINT_TYPE_COLOR;
  style->stroke.type = SP_PAINT_TYPE_NONE;
#endif
  style->stroke_width.unit = SP_UNIT_ABSOLUTE;
  style->stroke_width.distance = 1.0;
  style->absolute_stroke_width = 1.0;
  style->user_stroke_width = 1.0;
  dc->currentshape =
    (SPCanvasShape *)
    gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
			   SP_TYPE_CANVAS_SHAPE, NULL);
  sp_canvas_shape_set_style (dc->currentshape, style);
  sp_style_unref (style);
  gtk_signal_connect (GTK_OBJECT (dc->currentshape), "event",
		      GTK_SIGNAL_FUNC (sp_desktop_root_handler),
		      SP_EVENT_CONTEXT (dc)->desktop);
}

static gint
sp_dyna_draw_context_item_handler (SPEventContext * event_context,
				   SPItem * item, GdkEvent * event)
{
  gint ret;

  ret = FALSE;

  if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
    ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);

  return ret;
}

static double
flerp (double f0, double f1, double p)
{
  return f0 + (f1 - f0) * p;
}

/* Get normalized point */
static void
sp_dyna_draw_get_npoint (const SPDynaDrawContext *dc,
                         gdouble            vx,
                         gdouble            vy,
                         gdouble           *nx,
                         gdouble           *ny)
{
#ifdef NORMALIZED_COORDINATE
  ArtDRect drect;

  sp_desktop_get_visible_area (SP_EVENT_CONTEXT(dc)->desktop, &drect);
  *nx = (vx - drect.x0)/(drect.x1 - drect.x0);
  *ny = (vy - drect.y0)/(drect.y1 - drect.y0);
#else
  *nx = vx;
  *ny = vy;
#endif

/*    g_print ("NP:: vx:%g vy:%g => nx:%g ny:%g\n", vx, vy, *nx, *ny); */
/*    g_print ("NP:: x0:%g y0:%g x1:%g y1:%g\n", drect.x0, drect.y0, drect.y1, drect.y1); */
}

/* Get view point */
static void
sp_dyna_draw_get_vpoint (const SPDynaDrawContext *dc,
                         gdouble            nx,
                         gdouble            ny,
                         gdouble           *vx,
                         gdouble           *vy)
{
#ifdef NORMALIZED_COORDINATE
  ArtDRect drect;

  sp_desktop_get_visible_area (SP_EVENT_CONTEXT(dc)->desktop, &drect);
  *vx = nx * (drect.x1 - drect.x0) + drect.x0;
  *vy = ny * (drect.y1 - drect.y0) + drect.y0;
#else
  *vx = nx;
  *vy = ny;
#endif

/*    g_print ("VP:: nx:%g ny:%g => vx:%g vy:%g\n", nx, ny, *vx, *vy); */
}

/* Get current view point */
static void
sp_dyna_draw_get_curr_vpoint (const SPDynaDrawContext * dc,
                              gdouble *vx,
                              gdouble *vy)
{
#ifdef NORMALIZED_COORDINATE
  ArtDRect drect;

  sp_desktop_get_visible_area (SP_EVENT_CONTEXT(dc)->desktop, &drect);
  *vx = dc->curx * (drect.x1 - drect.x0) + drect.x0;
  *vy = dc->cury * (drect.y1 - drect.y0) + drect.y0;
#else
  *vx = dc->curx;
  *vy = dc->cury;
#endif
}

static void
sp_dyna_draw_reset (SPDynaDrawContext * dc, double x, double y)
{
  gdouble nx, ny;

  sp_dyna_draw_get_npoint (dc, x, y, &nx, &ny);

  dc->curx = nx;
  dc->cury = ny;
  dc->lastx = nx;
  dc->lasty = ny;
  dc->velx = 0.0;
  dc->vely = 0.0;
  dc->accx = 0.0;
  dc->accy = 0.0;
  dc->angx = 0.0;
  dc->angy = 0.0;
  dc->delx = 0.0;
  dc->dely = 0.0;
}

static gboolean
sp_dyna_draw_apply (SPDynaDrawContext * dc, double x, double y)
{
  double mass, drag;
  double fx, fy;
  double nx, ny;                /* normalized coordinates */

  sp_dyna_draw_get_npoint (dc, x, y, &nx, &ny);

  /* Calculate mass and drag */
  mass = flerp (1.0, 160.0, dc->mass);
  drag = flerp (0.0, 0.5, dc->drag * dc->drag);

  /* Calculate force and acceleration */
  fx = nx - dc->curx;
  fy = ny - dc->cury;
  dc->acc = hypot(fx, fy);
  if (dc->acc < DYNA_EPSILON)
    return FALSE;

  dc->accx = fx / mass;
  dc->accy = fy / mass;

  /* Calculate new velocity */
  dc->velx += dc->accx;
  dc->vely += dc->accy;
  dc->vel = hypot (dc->velx, dc->vely);

  /* Calculate angle of drawing tool */
  if (dc->fixed_angle)
    {
      dc->angx = cos (dc->angle * M_PI / 180.0);
      dc->angy = -sin (dc->angle * M_PI / 180.0);
    }
  else
    {
      if (dc->vel < DYNA_EPSILON)
        return FALSE;
      dc->angx = -dc->vely / dc->vel;
      dc->angy =  dc->velx / dc->vel;
    }

  /* Apply drag */
  dc->velx *= 1.0 - drag;
  dc->vely *= 1.0 - drag;

  /* Update position */
  dc->lastx = dc->curx;
  dc->lasty = dc->cury;
  dc->curx += dc->velx;
  dc->cury += dc->vely;

  return TRUE;
}

static void
sp_dyna_draw_brush (SPDynaDrawContext *dc)
{

  g_assert (dc->npoints >= 0 && dc->npoints < SAMPLING_SIZE);

  if (dc->use_calligraphic)
    {
      /* calligraphics */
      double width;
      double delx, dely;
      ArtPoint *vp;

      /* fixme: */
#ifdef NORMALIZED_COORDINATE
      width = (0.05 - dc->vel) * dc->width;
#else
      width = (100.0 - dc->vel) * dc->width;
#endif
      if (width < DYNA_MIN_WIDTH)
        width = DYNA_MIN_WIDTH;
      delx = dc->angx * width;
      dely = dc->angy * width;

#if 0
      g_print ("brush:: [w:%g] [dc->w:%g] [dc->vel:%g]\n", width, dc->width, dc->vel);
      g_print("brush:: [del: %g, %g]\n", delx, dely);
#endif

      vp = &dc->point1[dc->npoints];
      sp_dyna_draw_get_vpoint (dc, dc->curx + delx, dc->cury + dely,
                               &vp->x, &vp->y);
      vp = &dc->point2[dc->npoints];
      sp_dyna_draw_get_vpoint (dc, dc->curx - delx, dc->cury - dely,
                               &vp->x, &vp->y);

      dc->delx = delx;
      dc->dely = dely;
    }
  else
    {
      sp_dyna_draw_get_curr_vpoint (dc, &dc->point1[dc->npoints].x,
                                    &dc->point1[dc->npoints].y);
    }

  dc->npoints ++;
}

static gint
sp_dyna_draw_timeout_handler (gpointer data)
{
  SPDynaDrawContext *dc;
  SPDesktop *desktop;
  GnomeCanvas *canvas;
  gint x, y;
  ArtPoint p;

  dc = SP_DYNA_DRAW_CONTEXT (data);
  desktop = SP_EVENT_CONTEXT(dc)->desktop;
  canvas = GNOME_CANVAS (SP_DT_CANVAS (desktop));

  dc->dragging = TRUE;
  dc->dynahand = TRUE;
  
  gtk_widget_get_pointer (GTK_WIDGET(canvas), &x, &y);
  gnome_canvas_window_to_world (canvas, (double)x, (double)y, &p.x, &p.y);
  sp_desktop_w2d_xy_point (desktop, &p, p.x, p.y);
/*    g_print ("(%d %d)=>(%g %g)\n", x, y, p.x, p.y); */
  if (! sp_dyna_draw_apply (dc, p.x, p.y))
    return TRUE;
  sp_dyna_draw_get_curr_vpoint (dc, &p.x, &p.y);
  test_inside (dc, p.x, p.y);
  
  if (!dc->cinside)
    {
      sp_desktop_free_snap (desktop, &p);
    }
  
  if ((dc->curx != dc->lastx) || (dc->cury != dc->lasty))
    {
      sp_dyna_draw_brush (dc);
      g_assert (dc->npoints > 0);
      fit_and_split (dc, FALSE);
    }
  return TRUE;
}

gint
sp_dyna_draw_context_root_handler (SPEventContext * event_context,
				   GdkEvent * event)
{
  SPDynaDrawContext *dc;
  SPDesktop *desktop;
  ArtPoint p;
  gint ret;

  dc = SP_DYNA_DRAW_CONTEXT (event_context);
  desktop = event_context->desktop;

  ret = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
	{
	  sp_desktop_w2d_xy_point (desktop, &p, event->button.x,
				   event->button.y);
	  sp_dyna_draw_reset (dc, p.x, p.y);
          sp_dyna_draw_apply (dc, p.x, p.y);
          sp_dyna_draw_get_curr_vpoint (dc, &p.x, &p.y);

	  test_inside (dc, p.x, p.y);
	  if (!dc->cinside)
	    {
	      sp_desktop_free_snap (desktop, &p);
	      sp_curve_reset (dc->accumulated);
	      if (dc->repr)
		{
		  dc->repr = NULL;
		}
	      move_ctrl (dc, p.x, p.y);
	    }
	  else if (dc->accumulated->end > 1)
	    {
	      ArtBpath *bp;
	      bp = sp_curve_last_bpath (dc->accumulated);
	      p.x = bp->x3;
	      p.y = bp->y3;
	      move_ctrl (dc, dc->accumulated->bpath->x3,
			 dc->accumulated->bpath->y3);
	    }
	  else
	    {
	      ret = TRUE;
	      break;
	    }

	  /* initialize first point */
          dc->npoints = 0;
         
          if (dc->use_timeout)
            gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
                                    GDK_BUTTON_RELEASE_MASK |
                                    GDK_BUTTON_PRESS_MASK, NULL,
                                    event->button.time);
          else
            gnome_canvas_item_grab (GNOME_CANVAS_ITEM (desktop->acetate),
                                    GDK_BUTTON_RELEASE_MASK |
                                    GDK_POINTER_MOTION_MASK |
                                    GDK_BUTTON_PRESS_MASK, NULL,
                                    event->button.time);

          if (dc->use_timeout && !dc->timer_id)
            dc->timer_id = gtk_timeout_add (SAMPLE_TIMEOUT, sp_dyna_draw_timeout_handler, dc);
	  ret = TRUE;
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (!dc->use_timeout && (event->motion.state & GDK_BUTTON1_MASK))
	{
	  dc->dragging = TRUE;
	  dc->dynahand = TRUE;

          sp_desktop_w2d_xy_point (desktop, &p, event->motion.x,
                                   event->motion.y);
#if 0
          g_print ("(%g %g)=>(%g %g)\n", event->motion.x, event->motion.y, p.x, p.y);
#endif
	  if (! sp_dyna_draw_apply (dc, p.x, p.y))
            {
              ret = TRUE;
              break;
            }
          sp_dyna_draw_get_curr_vpoint (dc, &p.x, &p.y);

          test_inside (dc, p.x, p.y);

	  if (!dc->cinside)
	    {
	      sp_desktop_free_snap (desktop, &p);
	    }

	  if ((dc->curx != dc->lastx) || (dc->cury != dc->lasty))
	    {
              sp_dyna_draw_brush (dc);
              g_assert (dc->npoints > 0);
              fit_and_split (dc, FALSE);
	    }
	  ret = TRUE;
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1 &&
          dc->use_timeout && dc->timer_id != 0)
        {
          gtk_timeout_remove (dc->timer_id);
          dc->timer_id = 0;
        }
      if (dc->dragging && event->button.button == 1)
	{
	  dc->dragging = FALSE;

/*            g_print ("[release]\n"); */
	  if (dc->dynahand)
	    {
	      dc->dynahand = FALSE;
	      /* Remove all temporary line segments */
	      while (dc->segments)
		{
		  gtk_object_destroy (GTK_OBJECT (dc->segments->data));
		  dc->segments = g_slist_remove (dc->segments, dc->segments->data);
		}
	      /* Create object */
              fit_and_split (dc, TRUE);
              if (dc->use_calligraphic)
                {
                  accumulate_calligraphic (dc);
                }
              else
                {
                  concat_current_line (dc);
                  if (dc->cinside && dc->accumulated->end > 3)
                    {
                      sp_curve_closepath_current (dc->accumulated);
                    }
                }
              set_to_accumulated (dc); /* temporal implementation */
              if (dc->use_calligraphic || dc->cinside)
                {
                  /* reset accumulated curve */
                  sp_curve_reset (dc->accumulated);
                  clear_current (dc);
                  if (dc->repr)
		    {
		      dc->repr = NULL;
		    }
                  remove_ctrl (dc);
                }
              
	    }

	  gnome_canvas_item_ungrab (GNOME_CANVAS_ITEM (desktop->acetate),
				    event->button.time);
	  ret = TRUE;
	}
      break;
    default:
      break;
    }

  if (!ret)
    {
      if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
	ret = SP_EVENT_CONTEXT_CLASS (parent_class)-> root_handler (event_context, event);
    }

  return ret;
}

static void
clear_current (SPDynaDrawContext * dc)
{
  /* reset canvas_shape */
  sp_canvas_shape_clear (dc->currentshape);
  /* reset curve */
  sp_curve_reset (dc->currentcurve);
  sp_curve_reset (dc->cal1);
  sp_curve_reset (dc->cal2);
  /* reset points */
  dc->npoints = 0;
}

static void
set_to_accumulated (SPDynaDrawContext * dc)
{
  SPDesktop *desktop;

  desktop = SP_EVENT_CONTEXT (dc)->desktop;

  if (!sp_curve_empty (dc->accumulated))
    {
      ArtBpath *abp;
      gdouble d2doc[6];
      gchar *str;

      if (!dc->repr)
	{
	  SPRepr *repr, *style;
	  SPCSSAttr *css;
	  /* Create object */
	  repr = sp_repr_new ("path");
	  /* Set style */
#if 1
          if (dc->use_calligraphic)
            style = sodipodi_get_repr (SODIPODI, "paint.calligraphic");
          else
            style = sodipodi_get_repr (SODIPODI, "paint.freehand");
#else
/*  	  style = sodipodi_get_repr (SODIPODI, "paint.dynahand"); */
	  style = sodipodi_get_repr (SODIPODI, "paint.freehand");
#endif
	  if (style)
	    {
	      css = sp_repr_css_attr_inherited (style, "style");
/*	      sp_repr_css_set_property (css, "stroke", "none"); */
/*	      sp_repr_css_set_property (css, "fill-rule", "nonzero"); */
	      sp_repr_css_set (repr, css, "style");
	      sp_repr_css_attr_unref (css);
	    }
	  dc->repr = repr;

	  sp_document_add_repr (SP_DT_DOCUMENT (desktop), dc->repr);
	  sp_repr_unref (dc->repr);
	  sp_selection_set_repr (SP_DT_SELECTION (desktop), dc->repr);
	}
      sp_desktop_d2doc_affine (desktop, d2doc);
      abp = art_bpath_affine_transform (sp_curve_first_bpath (dc->accumulated), d2doc);
      str = sp_svg_write_path (abp);
      g_assert (str != NULL);
      art_free (abp);
      sp_repr_set_attr (dc->repr, "d", str);
      g_free (str);
    }
  else
    {
      if (dc->repr)
	sp_repr_unparent (dc->repr);
      dc->repr = NULL;
    }

  sp_document_done (SP_DT_DOCUMENT (desktop));
}

#if 0
static void
accumulate_line (SPDynaDrawContext *dc)
{
}
#endif
static void
concat_current_line (SPDynaDrawContext * dc)
{
  if (!sp_curve_empty (dc->currentcurve))
    {
      ArtBpath *bpath;
      if (sp_curve_empty (dc->accumulated))
	{
	  bpath = sp_curve_first_bpath (dc->currentcurve);
	  g_assert (bpath->code == ART_MOVETO_OPEN);
	  sp_curve_moveto (dc->accumulated, bpath->x3, bpath->y3);
	}
      bpath = sp_curve_last_bpath (dc->currentcurve);
      if (bpath->code == ART_CURVETO)
	{
	  sp_curve_curveto (dc->accumulated, bpath->x1, bpath->y1, bpath->x2,
			    bpath->y2, bpath->x3, bpath->y3);
	}
      else if (bpath->code == ART_LINETO)
	{
	  sp_curve_lineto (dc->accumulated, bpath->x3, bpath->y3);
	}
      else
	{
	  g_assert_not_reached ();
	}
    }
}

static void
accumulate_calligraphic (SPDynaDrawContext * dc)
{
  SPCurve *rev_cal2;

  if (!sp_curve_empty (dc->cal1) && !sp_curve_empty(dc->cal2))
    {
      sp_curve_reset (dc->accumulated); /*  Is this required ?? */
      rev_cal2 = sp_curve_reverse (dc->cal2);
      sp_curve_append (dc->accumulated, dc->cal1, FALSE);
      sp_curve_append (dc->accumulated, rev_cal2, TRUE);
      sp_curve_closepath(dc->accumulated);

      sp_curve_unref (rev_cal2);

      sp_curve_reset (dc->cal1);
      sp_curve_reset (dc->cal2);
    }
}

static void
test_inside (SPDynaDrawContext * dc, double x, double y)
{
  ArtPoint p;

  if (dc->citem == NULL)
    {
      dc->cinside = FALSE;
      return;
    }

  sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &p, x, y);

#define MAX_INSIDE_DISTANCE 4.0

  if ((fabs (p.x - dc->cpos.x) > MAX_INSIDE_DISTANCE) ||
      (fabs (p.y - dc->cpos.y) > MAX_INSIDE_DISTANCE))
    {
      if (dc->cinside != 0)
	{
	  gnome_canvas_item_set (dc->citem, "filled", FALSE, NULL);
	  dc->cinside = 0;
	}
    }
  else
    {
      if (dc->cinside != 1)
	{
	  gnome_canvas_item_set (dc->citem, "filled", TRUE, NULL);
	  dc->cinside = 1;
	}
    }
}


static void
move_ctrl (SPDynaDrawContext * dc, double x, double y)
{
  if (dc->citem == NULL)
    {
      dc->citem =
	gnome_canvas_item_new (SP_DT_CONTROLS
			       (SP_EVENT_CONTEXT (dc)->desktop), SP_TYPE_CTRL,
			       "size", 4.0, "filled", 0, "fill_color",
			       dc->ccolor, "stroked", 1, "stroke_color",
			       0x000000ff, NULL);
      gtk_signal_connect (GTK_OBJECT (dc->citem), "event",
			  GTK_SIGNAL_FUNC (sp_desktop_root_handler),
			  SP_EVENT_CONTEXT (dc)->desktop);
    }
  sp_ctrl_moveto (SP_CTRL (dc->citem), x, y);
  sp_desktop_d2w_xy_point (SP_EVENT_CONTEXT (dc)->desktop, &dc->cpos, x, y);
  dc->cinside = -1;
}

static void
remove_ctrl (SPDynaDrawContext * dc)
{
  if (dc->citem)
    {
      gtk_object_destroy (GTK_OBJECT (dc->citem));
      dc->citem = NULL;
    }
}

#define BEZIER_ASSERT(b) { \
      g_assert ((b[0].x > -8000.0) && (b[0].x < 8000.0)); \
      g_assert ((b[0].y > -8000.0) && (b[0].y < 8000.0)); \
      g_assert ((b[1].x > -8000.0) && (b[1].x < 8000.0)); \
      g_assert ((b[1].y > -8000.0) && (b[1].y < 8000.0)); \
      g_assert ((b[2].x > -8000.0) && (b[2].x < 8000.0)); \
      g_assert ((b[2].y > -8000.0) && (b[2].y < 8000.0)); \
      g_assert ((b[3].x > -8000.0) && (b[3].x < 8000.0)); \
      g_assert ((b[3].y > -8000.0) && (b[3].y < 8000.0)); \
      }

static void
fit_and_split (SPDynaDrawContext *dc,
               gboolean           release)
{
  if (dc->use_calligraphic)
    {
      fit_and_split_calligraphics (dc, release);
    }
  else
    {
      fit_and_split_line (dc, release);
    }
}

static void
fit_and_split_line (SPDynaDrawContext *dc,
                    gboolean           release)
{
  ArtPoint b[4];
  gdouble tolerance;

  tolerance = SP_EVENT_CONTEXT (dc)->desktop->w2d[0] * TOLERANCE_LINE;
  tolerance = tolerance * tolerance;

  if (sp_bezier_fit_cubic (b, dc->point1, dc->npoints, tolerance) > 0
      && dc->npoints < SAMPLING_SIZE)
    {
      /* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
      g_print ("%d", dc->npoints);
#endif
/*        BEZIER_ASSERT (b); */
      sp_curve_reset (dc->currentcurve);
      sp_curve_moveto (dc->currentcurve, b[0].x, b[0].y);
      sp_curve_curveto (dc->currentcurve, b[1].x, b[1].y, b[2].x, b[2].y,
			b[3].x, b[3].y);
      sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
    }
  else
    {
      SPCurve *curve;
      SPStyle *style;
      SPCanvasShape *cshape;
      /* Fit and draw and copy last point */
#ifdef DYNA_DRAW_VERBOSE
      g_print ("[%d]Yup\n", dc->npoints);
#endif
      g_assert (!sp_curve_empty (dc->currentcurve));
      concat_current_line (dc);
      curve = sp_curve_copy (dc->currentcurve);
      /* fixme: */
      style = sp_style_new (NULL);
      style->fill.type = SP_PAINT_TYPE_NONE;
      style->stroke.type = SP_PAINT_TYPE_COLOR;
      style->stroke_width.unit = SP_UNIT_ABSOLUTE;
      style->stroke_width.distance = 1.0;
      style->absolute_stroke_width = 1.0;
      style->user_stroke_width = 1.0;
      cshape =
	(SPCanvasShape *)
	gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
			       SP_TYPE_CANVAS_SHAPE, NULL);
      sp_canvas_shape_set_style (cshape, style);
      sp_style_unref (style);
      gtk_signal_connect (GTK_OBJECT (cshape), "event",
			  GTK_SIGNAL_FUNC (sp_desktop_root_handler),
			  SP_EVENT_CONTEXT (dc)->desktop);

      sp_canvas_shape_add_component (cshape, curve, TRUE, NULL);
      sp_curve_unref (curve);

      dc->segments = g_slist_prepend (dc->segments, cshape);

#if 0
      dc->point1[0] = dc->point1[dc->npoints - 2];
      dc->point1[1] = dc->point1[dc->npoints - 1];
#else
      dc->point1[0] = dc->point1[dc->npoints - 2];
#endif
      dc->npoints = 1;
    }
}

static void
fit_and_split_calligraphics (SPDynaDrawContext *dc,
                            gboolean           release)
{
  gdouble tolerance;

  tolerance = SP_EVENT_CONTEXT (dc)->desktop->w2d[0] * TOLERANCE_CALLIGRAPHIC;
  tolerance = tolerance * tolerance;

#ifdef DYNA_DRAW_VERBOSE
  g_print ("[F&S:R=%c]", release?'T':'F');
#endif

  g_assert (dc->npoints > 0 && dc->npoints < SAMPLING_SIZE);

  if (dc->npoints == SAMPLING_SIZE-1 || release)
    {
#define BEZIER_SIZE       4
#define BEZIER_MAX_DEPTH  4
#define BEZIER_MAX_LENGTH (BEZIER_SIZE * (2 << (BEZIER_MAX_DEPTH-1)))
      SPCurve *curve;
      SPStyle *style;
      SPCanvasShape *cshape;
      ArtPoint b1[BEZIER_MAX_LENGTH], b2[BEZIER_MAX_LENGTH];
      gint nb1, nb2;            /* number of blocks */

#ifdef DYNA_DRAW_VERBOSE
      g_print ("[F&S:#] dc->npoints:%d, release:%s\n",
               dc->npoints, release ? "TRUE" : "FALSE");
#endif

      /* Current calligraphic */
      if (dc->cal1->end==0 || dc->cal2->end==0)
        {
          /* dc->npoints > 0 */
/*            g_print ("calligraphics(1|2) reset\n"); */
          sp_curve_reset (dc->cal1);
          sp_curve_reset (dc->cal2);
          
          sp_curve_moveto (dc->cal1, dc->point1[0].x, dc->point1[0].y);
          sp_curve_moveto (dc->cal2, dc->point2[0].x, dc->point2[0].y);
        }

      nb1 = sp_bezier_fit_cubic_r (b1, dc->point1, dc->npoints,
                                       tolerance, BEZIER_MAX_DEPTH);
      nb2 = sp_bezier_fit_cubic_r (b2, dc->point2, dc->npoints,
                                       tolerance, BEZIER_MAX_DEPTH);
      if (nb1 != -1 && nb2 != -1)
        {
          ArtPoint *bp1, *bp2;
          /* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
          g_print ("nb1:%d nb2:%d\n", nb1, nb2);
#endif
          /* CanvasShape */
          if (! release)
            {
              sp_curve_reset (dc->currentcurve);
              sp_curve_moveto (dc->currentcurve, b1[0].x, b1[0].y);
              for (bp1 = b1; bp1 < b1 + BEZIER_SIZE*nb1; bp1 += BEZIER_SIZE)
                {
                  BEZIER_ASSERT (bp1);
                  
                  sp_curve_curveto (dc->currentcurve, bp1[1].x, bp1[1].y,
                                    bp1[2].x, bp1[2].y, bp1[3].x, bp1[3].y);
                  
                }
              sp_curve_lineto (dc->currentcurve,
                               b2[BEZIER_SIZE*(nb2-1) + 3].x,
                               b2[BEZIER_SIZE*(nb2-1) + 3].y);
              for (bp2 = b2 + BEZIER_SIZE*(nb2-1); bp2 >= b2; bp2 -= BEZIER_SIZE)
                {
                  BEZIER_ASSERT (bp2);
                  
                  sp_curve_curveto (dc->currentcurve, bp2[2].x, bp2[2].y,
                                    bp2[1].x, bp2[1].y, bp2[0].x, bp2[0].y);
                }
              sp_curve_closepath (dc->currentcurve);
              sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
            }
          
          /* Current calligraphic */
          for (bp1 = b1; bp1 < b1 + BEZIER_SIZE*nb1; bp1 += BEZIER_SIZE)
            {
              sp_curve_curveto (dc->cal1, bp1[1].x, bp1[1].y,
                                bp1[2].x, bp1[2].y, bp1[3].x, bp1[3].y);
            }
          for (bp2 = b2; bp2 < b2 + BEZIER_SIZE*nb2; bp2 += BEZIER_SIZE)
            {
              sp_curve_curveto (dc->cal2, bp2[1].x, bp2[1].y,
                                bp2[2].x, bp2[2].y, bp2[3].x, bp2[3].y);
            }
        }
      else
        {
          gint  i;
          /* fixme: ??? */
#ifdef DYNA_DRAW_VERBOSE
          g_print ("[fit_and_split_calligraphics] failed to fit-cubic.\n");
#endif
          draw_temporary_box (dc);

          for (i = 1; i < dc->npoints; i++)
            sp_curve_lineto (dc->cal1, dc->point1[i].x, dc->point1[i].y);
          for (i = 1; i < dc->npoints; i++)
            sp_curve_lineto (dc->cal2, dc->point2[i].x, dc->point2[i].y);
        }

      /* Fit and draw and copy last point */
#ifdef DYNA_DRAW_VERBOSE
      g_print ("[%d]Yup\n", dc->npoints);
#endif
      if (! release)
        {
          g_assert (!sp_curve_empty (dc->currentcurve));
          curve = sp_curve_copy (dc->currentcurve);
          /* fixme: */
          style = sp_style_new (NULL);
          /* style should be changed when dc->use_calligraphc is touched */
#ifdef DYNA_DRAW_DEBUG
          style->fill.type = SP_PAINT_TYPE_NONE;
          style->stroke.type = SP_PAINT_TYPE_COLOR;
#else
          style->fill.type = SP_PAINT_TYPE_COLOR;
          style->stroke.type = SP_PAINT_TYPE_NONE;
#endif
          style->stroke_width.unit = SP_UNIT_ABSOLUTE;
          style->stroke_width.distance = 1.0;
          style->absolute_stroke_width = 1.0;
          style->user_stroke_width = 1.0;
          cshape = (SPCanvasShape *)
            gnome_canvas_item_new (SP_DT_SKETCH (SP_EVENT_CONTEXT (dc)->desktop),
                                   SP_TYPE_CANVAS_SHAPE, NULL);
          sp_canvas_shape_set_style (cshape, style);
          sp_style_unref (style);
          gtk_signal_connect (GTK_OBJECT (cshape), "event",
                              GTK_SIGNAL_FUNC (sp_desktop_root_handler),
                              SP_EVENT_CONTEXT (dc)->desktop);
          
          sp_canvas_shape_add_component (cshape, curve, TRUE, NULL);
          sp_curve_unref (curve);
          dc->segments = g_slist_prepend (dc->segments, cshape);
        }

      dc->point1[0] = dc->point1[dc->npoints - 1];
      dc->point2[0] = dc->point2[dc->npoints - 1];
      dc->npoints = 1;
    }
  else
  {
	  draw_temporary_box (dc);
  }
}

static void
draw_temporary_box (SPDynaDrawContext *dc)
{
  gint  i;
  
  sp_curve_reset (dc->currentcurve);
  sp_curve_moveto (dc->currentcurve, dc->point1[0].x, dc->point1[0].y);
  for (i = 1; i < dc->npoints; i++)
    sp_curve_lineto (dc->currentcurve, dc->point1[i].x, dc->point1[i].y);
  for (i = dc->npoints-1; i >= 0; i--)
    sp_curve_lineto (dc->currentcurve, dc->point2[i].x, dc->point2[i].y);
  sp_curve_closepath (dc->currentcurve);
  sp_canvas_shape_change_bpath (dc->currentshape, dc->currentcurve);
}
