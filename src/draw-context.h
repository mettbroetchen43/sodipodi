#ifndef __SP_DRAW_CONTEXT_H__
#define __SP_DRAW_CONTEXT_H__

/*
 * Generic drawing context
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

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

#define SP_DRAW_POINTS_MAX 16

enum {
	SP_DRAW_CONTEXT_IDLE,
	SP_DRAW_CONTEXT_ADDLINE,
	SP_DRAW_CONTEXT_FREEHAND
};

struct _SPDrawContext {
	SPEventContext event_context;

	guint state : 2;

#if 0
	guint addline : 1;
#endif

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
	GnomeCanvasItem *blue_bpath;
	/* Blue curve */
	SPCurve *blue_curve;

	/* Green list */
	GSList *green_bpaths;
	/* Green curve */
	SPCurve *green_curve;
	/* Green anchor */
	SPDrawAnchor *green_anchor;

	/* Start anchor */
	SPDrawAnchor *sa;
	/* End anchor */
	SPDrawAnchor *ea;

	ArtPoint p[SP_DRAW_POINTS_MAX];
	gint npoints;
};

struct _SPDrawContextClass {
	SPEventContextClass parent_class;
};

GtkType sp_draw_context_get_type (void);

#endif
