#define SP_SELTRANS_HANDLES_C

#include "seltrans-handles.h"

const SPSelTransHandle handles_scale[] = {
// anchor         cursor                  control               action                request                       x    y
  {GTK_ANCHOR_SE, GDK_TOP_LEFT_CORNER,    SP_KNOT_SHAPE_SQUARE, sp_sel_trans_scale,   sp_sel_trans_scale_request,   0,   1},
  {GTK_ANCHOR_S,  GDK_TOP_SIDE,           SP_KNOT_SHAPE_SQUARE, sp_sel_trans_stretch, sp_sel_trans_stretch_request, 0.5, 1},
  {GTK_ANCHOR_SW, GDK_TOP_RIGHT_CORNER,   SP_KNOT_SHAPE_SQUARE, sp_sel_trans_scale,   sp_sel_trans_scale_request,   1,   1},
  {GTK_ANCHOR_W,  GDK_RIGHT_SIDE,         SP_KNOT_SHAPE_SQUARE, sp_sel_trans_stretch, sp_sel_trans_stretch_request, 1,   0.5},
  {GTK_ANCHOR_NW, GDK_BOTTOM_RIGHT_CORNER,SP_KNOT_SHAPE_SQUARE, sp_sel_trans_scale,   sp_sel_trans_scale_request,   1,   0},
  {GTK_ANCHOR_N,  GDK_BOTTOM_SIDE,        SP_KNOT_SHAPE_SQUARE, sp_sel_trans_stretch, sp_sel_trans_stretch_request, 0.5, 0},
  {GTK_ANCHOR_NE, GDK_BOTTOM_LEFT_CORNER, SP_KNOT_SHAPE_SQUARE, sp_sel_trans_scale,   sp_sel_trans_scale_request,   0,   0},
  {GTK_ANCHOR_E,  GDK_LEFT_SIDE,          SP_KNOT_SHAPE_SQUARE, sp_sel_trans_stretch, sp_sel_trans_stretch_request, 0,   0.5}
};

const SPSelTransHandle handles_rotate[] = {
  {GTK_ANCHOR_SE, GDK_EXCHANGE,           SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_rotate,  sp_sel_trans_rotate_request,  0,   1},
  {GTK_ANCHOR_S,  GDK_SB_H_DOUBLE_ARROW,  SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_skew,    sp_sel_trans_skew_request,    0.5, 1},
  {GTK_ANCHOR_SW, GDK_EXCHANGE,           SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_rotate,  sp_sel_trans_rotate_request,  1,   1},
  {GTK_ANCHOR_W,  GDK_SB_V_DOUBLE_ARROW,  SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_skew,    sp_sel_trans_skew_request,    1,   0.5},
  {GTK_ANCHOR_NW, GDK_EXCHANGE,           SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_rotate,  sp_sel_trans_rotate_request,  1,   0},
  {GTK_ANCHOR_N,  GDK_SB_H_DOUBLE_ARROW,  SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_skew,    sp_sel_trans_skew_request,    0.5, 0},
  {GTK_ANCHOR_NE, GDK_EXCHANGE,           SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_rotate,  sp_sel_trans_rotate_request,  0,   0},
  {GTK_ANCHOR_E,  GDK_SB_V_DOUBLE_ARROW,  SP_KNOT_SHAPE_DIAMOND, sp_sel_trans_skew,    sp_sel_trans_skew_request,    0,   0.5}
};

const SPSelTransHandle handle_center =
  {GTK_ANCHOR_CENTER, GDK_CROSSHAIR,      SP_KNOT_SHAPE_SQUARE, sp_sel_trans_center,  sp_sel_trans_center_request,  0.5, 0.5};
