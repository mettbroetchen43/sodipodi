#ifndef __SP_SELTRANS_H__
#define __SP_SELTRANS_H__

#include "knot.h"
#include "desktop-handles.h"
#include "helper/sodipodi-ctrl.h"

typedef struct _SPSelTrans SPSelTrans;

typedef enum {
	SP_SELTRANS_OUTLINE,
	SP_SELTRANS_CONTENT
} SPSelTransShowType;

#ifndef __SP_SELTRANS_C__
extern SPSelTransShowType SelTransViewMode;
#else
SPSelTransShowType SelTransViewMode = SP_SELTRANS_CONTENT;
#endif

typedef enum {
	SP_SEL_TRANS_SCALE = 0,
	SP_SEL_TRANS_ROTATE = 1
} SPSelTransStateType;

struct _SPSelTrans {
	SPDesktop * desktop;
        GSList * snappoints;
	gboolean grabbed;
	gboolean show_handles;
	SPSelTransStateType state;
	gboolean empty;
	gboolean changed;
	gboolean sel_changed;
	ArtDRect box;
        double current[6];
        ArtPoint opposit;
        ArtPoint origin;
	ArtPoint point;
	ArtPoint center;
	SPKnot * shandle[8];
	SPKnot * rhandle[8];
	SPKnot * chandle;
        GnomeCanvasItem * norm;
        GnomeCanvasItem * grip;;
        GnomeCanvasItem * l1, * l2, * l3, * l4;
	guint sel_changed_id;
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
