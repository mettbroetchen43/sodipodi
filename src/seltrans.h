#ifndef SP_SELTRANS_H
#define SP_SELTRANS_H

#include "helper/sodipodi-ctrl.h"
#include "desktop.h"

typedef struct _SPSelTrans SPSelTrans;

typedef enum {
	SP_SEL_TRANS_SCALE = 0,
	SP_SEL_TRANS_ROTATE = 1
} SPSelTransStateType;

struct _SPSelTrans {
	SPDesktop * desktop;
	gboolean active;
	gboolean empty;
	gboolean grabbed;
	gboolean show_handles;
	gboolean changed;
	gboolean sel_changed;
	SPSelTransStateType state;
	ArtDRect box;
	double d2n[6];
	double n2current[6];
	ArtPoint point;
	ArtPoint center;
	SPCtrl * shandle[8];
	SPCtrl * rhandle[8];
	SPCtrl * chandle;
};

/*
 * Logic
 *
 * set_active - toggles handles + selection changed activity
 * grab - removes handles, makes unsensitive
 * ungrab - if changed, flushes, otherwise increases state, shows handles,
 *          makes sensitive
 * if changed or sel changed during grabbing, sets state to scale
 */ 

void sp_sel_trans_set_active (SPDesktop * desktop, gboolean active);
void sp_sel_trans_increase_state (SPDesktop * desktop);
void sp_sel_trans_set_center (SPDesktop * desktop, gdouble x, gdouble y);

void sp_sel_trans_grab (SPDesktop * desktop, gdouble affine[], gdouble x, gdouble y, gboolean show_handles);
void sp_sel_trans_transform (SPDesktop * desktop, gdouble affine[]);
void sp_sel_trans_ungrab (SPDesktop * desktop);

ArtPoint * sp_sel_trans_d2n_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y);
ArtPoint * sp_sel_trans_n2d_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y);
ArtPoint * sp_sel_trans_point_desktop (SPDesktop * desktop, ArtPoint * p);
ArtPoint * sp_sel_trans_point_normal (SPDesktop * desktop, ArtPoint * p);

#endif
