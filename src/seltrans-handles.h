#ifndef SP_SELTRANS_HANDLES_H
#define SP_SELTRANS_HANDLES_H

#include "helper/sodipodi-ctrl.h"
#include "desktop.h"

typedef struct _SPSelTransHandle SPSelTransHandle;

void sp_handle_stretch (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_scale (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_skew (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_rotate (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);
void sp_handle_center (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);

struct _SPSelTransHandle {
	GtkAnchorType anchor;
	GdkCursorType cursor;
	SPCtrl * control;
	void (* action) (SPDesktop * desktop, SPSelTransHandle * handle, double x, double y, guint state);
	gdouble * affine;
	gdouble x, y;
};

#ifndef SP_SELTRANS_HANDLES_C
extern SPSelTransHandle handles_scale[8];
extern SPSelTransHandle handles_rotate[8];
extern SPSelTransHandle handle_center;
#endif

#endif

