#ifndef SP_SELTRANS_HANDLES_H
#define SP_SELTRANS_HANDLES_H

#include "helper/sodipodi-ctrl.h"
#include "seltrans.h"

typedef struct _SPSelTransHandle SPSelTransHandle;

// request handlers
gboolean sp_sel_trans_scale_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
gboolean sp_sel_trans_stretch_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
gboolean sp_sel_trans_skew_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
gboolean sp_sel_trans_rotate_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
gboolean sp_sel_trans_center_request (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);

// action handlers
void sp_sel_trans_scale (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
void sp_sel_trans_stretch (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
void sp_sel_trans_skew (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
void sp_sel_trans_rotate (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
void sp_sel_trans_center (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);



struct _SPSelTransHandle {
	GtkAnchorType anchor;
	GdkCursorType cursor;
	SPKnotShapeType control;
	void (* action) (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);
	gboolean (* request) (SPSelTrans * seltrans, SPSelTransHandle * handle, ArtPoint * p, guint state);        
	gdouble x, y;
};

#ifndef SP_SELTRANS_HANDLES_C
extern const SPSelTransHandle handles_scale[9];
extern const SPSelTransHandle handles_rotate[9];
extern const SPSelTransHandle handle_center;
#endif

#endif

