#ifndef SP_SELTRANS_H
#define SP_SELTRANS_H

#include "knot.h"
#include "desktop-handles.h"

typedef struct _SPSelTrans SPSelTrans;

typedef enum {
	SP_SEL_TRANS_SCALE = 0,
	SP_SEL_TRANS_ROTATE = 1
} SPSelTransStateType;

struct _SPSelTrans {
	SPDesktop * desktop;
	gboolean grabbed;
	gboolean show_handles;
	SPSelTransStateType state;
	gboolean empty;
	gboolean changed;
	gboolean sel_changed;
	ArtDRect box;
	double d2n[6];
	double n2current[6];
	ArtPoint point;
	ArtPoint center;
	SPKnot * shandle[8];
	SPKnot * rhandle[8];
	SPKnot * chandle;
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

void sp_sel_trans_grab (SPSelTrans * seltrans, gdouble affine[], gdouble x, gdouble y, gboolean show_handles);
void sp_sel_trans_transform (SPSelTrans * seltrans, gdouble affine[]);
void sp_sel_trans_ungrab (SPSelTrans * seltrans);

ArtPoint * sp_sel_trans_d2n_xy_point (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y);
ArtPoint * sp_sel_trans_n2d_xy_point (SPSelTrans * seltrans, ArtPoint * p, gdouble x, gdouble y);
ArtPoint * sp_sel_trans_point_desktop (SPSelTrans * seltrans, ArtPoint * p);
ArtPoint * sp_sel_trans_point_normal (SPSelTrans * seltrans, ArtPoint * p);

#endif
