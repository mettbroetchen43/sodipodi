#ifndef __SP_SELTRANS_H__
#define __SP_SELTRANS_H__

/*
 * Helper object for transforming selected items
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "knot.h"
#include "desktop-handles.h"
#if 0
#include "helper/sodipodi-ctrl.h"
#endif

typedef struct _SPSelTrans SPSelTrans;

enum {
	SP_SELTRANS_SHOW_CONTENT,
	SP_SELTRANS_SHOW_OUTLINE
};

enum {
	SP_SELTRANS_TRANSFORM_OPTIMIZE,
	SP_SELTRANS_TRANSFORM_KEEP
};

enum {
	SP_SELTRANS_STATE_SCALE,
	SP_SELTRANS_STATE_ROTATE
};

#if 0
#ifndef __SP_SELTRANS_C__
extern SPSelTransShowType SelTransViewMode;
#else
SPSelTransShowType SelTransViewMode = SP_SELTRANS_CONTENT;
#endif
#endif

struct _SPSelTrans {
	SPDesktop *desktop;
	SPSelection *selection;

	guint state : 1;
	guint show : 1;
	guint transform : 1;

	GSList *snappoints;
	gboolean grabbed;
	gboolean show_handles;
	gboolean empty;
	gboolean changed;
	gboolean sel_changed;
	ArtDRect box;
        double current[6];
        ArtPoint opposit;
        ArtPoint origin;
	ArtPoint point;
	ArtPoint center;
	SPKnot *shandle[8];
	SPKnot *rhandle[8];
	SPKnot *chandle;
        GnomeCanvasItem *norm;
        GnomeCanvasItem *grip;;
        GnomeCanvasItem *l1, *l2, *l3, *l4;
	guint sel_changed_id;
	guint sel_modified_id;
};

/*
 * Logic
 *
 * grab - removes handles, makes unsensitive
 * ungrab - if changed, flushes, otherwise increases state, shows handles,
 *          makes sensitive
 * if changed or sel changed during grabbing, sets state to scale
 *
 */ 

void sp_sel_trans_init (SPSelTrans * seltrans, SPDesktop * desktop);
void sp_sel_trans_shutdown (SPSelTrans * seltrans);

void sp_sel_trans_reset_state (SPSelTrans * seltrans);
void sp_sel_trans_increase_state (SPSelTrans * seltrans);
void sp_sel_trans_set_center (SPSelTrans * seltrans, gdouble x, gdouble y);

void sp_sel_trans_grab (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y, gboolean show_handles);
void sp_sel_trans_transform (SPSelTrans * seltrans, gdouble affine[], ArtPoint * norm);
void sp_sel_trans_ungrab (SPSelTrans * seltrans);

ArtPoint * sp_sel_trans_point_desktop (SPSelTrans * seltrans, ArtPoint * p);
ArtPoint * sp_sel_trans_origin_desktop (SPSelTrans * seltrans, ArtPoint * p);


#endif
