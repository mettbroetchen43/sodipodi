#ifndef __SP_DRAW_CONTEXT_H__
#define __SP_DRAW_CONTEXT_H__

#include "helper/curve.h"
#include "event-context.h"

#define SP_TYPE_DRAW_CONTEXT (sp_draw_context_get_type ())
#define SP_DRAW_CONTEXT(o) (GTK_CHECK_CAST ((o), SP_TYPE_DRAW_CONTEXT, SPDrawContext))
#define SP_DRAW_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_DRAW_CONTEXT, SPDrawContextClass))
#define SP_IS_DRAW_CONTEXT(o) (GTK_CHECK_TYPE ((o), SP_TYPE_DRAW_CONTEXT))
#define SP_IS_DRAW_CONTEXT_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_DRAW_CONTEXT))

typedef struct _SPDrawContext SPDrawContext;
typedef struct _SPDrawContextClass SPDrawContextClass;
typedef struct _SPDrawAnchor SPDrawAnchor;

struct _SPDrawContext {
	SPEventContext event_context;

	guint addline : 1;

	/* Current item */
	SPItem *white_item;
	/* White curve list */
	GSList *white_cl;
	/* White anchor list */
	GSList *white_al;

	/* Red bpath */
	GnomeCanvasItem *red_bpath;
	/* Red curve */
	SPCurve *red_curve;

	/* Blue bpath */
	GnomeCanvasItem *bbp;
	/* Blue curve */
	SPCurve *bc;

	/* Green list */
	GSList *gl;
	/* Green curve */
	SPCurve *gc;
	/* Green anchor */
	SPDrawAnchor *ga;

	/* Start anchor */
	SPDrawAnchor *sa;
	/* End anchor */
	SPDrawAnchor *ea;

	ArtPoint p[16];
	gint npoints;

	SPRepr * repr;
#if 0
	guint destroyid;

	GnomeCanvasItem * citem;
	ArtPoint cpos;
	guint32 ccolor;
	gboolean cinside;
#endif
};

struct _SPDrawContextClass {
	SPEventContextClass parent_class;
};

GtkType sp_draw_context_get_type (void);

/* Drawing anchors */

struct _SPDrawAnchor {
	SPDrawContext *dc;
	SPCurve *curve;
	guint start : 1;
	guint active : 1;
	ArtPoint dp, wp;
	GnomeCanvasItem *ctrl;
};

SPDrawAnchor *sp_draw_anchor_new (SPDrawContext *dc, SPCurve *curve, gboolean start, gdouble x, gdouble y);
void sp_draw_anchor_destroy (SPDrawAnchor *anchor);

#endif
