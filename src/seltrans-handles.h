#ifndef SP_SELTRANS_HANDLES_H
#define SP_SELTRANS_HANDLES_H

#include "helper/sodipodi-ctrl.h"
#include "seltrans.h"

typedef struct _SPSelTransHandle SPSelTransHandle;

void sp_handle_stretch (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_scale (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_skew (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_rotate (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_center (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);

struct _SPSelTransHandle {
	GtkAnchorType anchor;
	GdkCursorType cursor;
	SPCtrl * control;
	void (* action) (SPSelTrans * seltrans, SPSelTransHandle * handle, double x, double y, guint state);
	gdouble * affine;
	gdouble x, y;
};

#ifndef SP_SELTRANS_HANDLES_C
extern SPSelTransHandle handles_scale[8];
extern SPSelTransHandle handles_rotate[8];
extern SPSelTransHandle handle_center;
#endif

#endif

