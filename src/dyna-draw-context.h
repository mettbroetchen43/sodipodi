#ifndef SP_DYNA_DRAW_CONTEXT_H
#define SP_DYNA_DRAW_CONTEXT_H

#include "helper/curve.h"
#include "display/canvas-shape.h"
#include "event-context.h"

#define SP_TYPE_DYNA_DRAW_CONTEXT            (sp_dyna_draw_context_get_type ())
#define SP_DYNA_DRAW_CONTEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContext))
#define SP_DYNA_DRAW_CONTEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DYNA_DRAW_CONTEXT, SPDynaDrawContextClass))
#define SP_IS_DYNA_DRAW_CONTEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_DYNA_DRAW_CONTEXT))
#define SP_IS_DYNA_DRAW_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DYNA_DRAW_CONTEXT))

typedef struct _SPDynaDrawContext SPDynaDrawContext;
typedef struct _SPDynaDrawContextClass SPDynaDrawContextClass;
typedef struct _SPDynaDrawCtrl SPDynaDrawCtrl;

#define SAMPLING_SIZE 16        /* fixme: ?? */

struct _SPDynaDrawContext
{
  SPEventContext event_context;

  SPCurve *accumulated;
  GSList *segments;
  /* current shape and curves */
  SPCanvasShape *currentshape;
  SPCurve *currentcurve;
  SPCurve *cal1;
  SPCurve *cal2;
  /* temporary work area */
  ArtPoint point1[SAMPLING_SIZE];
  ArtPoint point2[SAMPLING_SIZE];
  gint npoints;

  /* repr */
  SPRepr *repr;

  /* control */
  GnomeCanvasItem *citem;
  ArtPoint cpos;
  guint32 ccolor;
  gboolean cinside;

  /* time_id if use timeout */
  gint timer_id;

  /* DynaDraw */
  double curx, cury;
  double velx, vely, vel;
  double accx, accy, acc;
  double angx, angy;
  double lastx, lasty;
  double delx, dely;
  /* attributes */
  /* fixme: shuld be merge dragging and dynahand ?? */
  guint dragging : 1;           /* mouse state: mouse is dragging */
  guint dynahand : 1;           /* mouse state: mouse is in draw */
  guint use_timeout : 1;
  guint use_calligraphic : 1;
  guint fixed_angle : 1;
  double mass, drag;
  double angle;
  double width;
};

struct _SPDynaDrawContextClass
{
  SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_dyna_draw_context_get_type (void);

#endif
