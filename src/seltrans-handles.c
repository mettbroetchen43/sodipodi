#define SP_SELTRANS_HANDLES_C

#include "seltrans-handles.h"

double transform_nw[6] = {-1.0, 0.0, 0.0, 1.0, 1.0, 0.0};
double transform_n[6]  = { 1.0, 0.0, 0.0, 1.0,-0.5, 0.0};
double transform_ne[6] = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
double transform_e[6]  = { 0.0, 1.0, 1.0, 0.0,-0.5, 0.0};
double transform_se[6] = { 1.0, 0.0, 0.0,-1.0, 0.0, 1.0};
double transform_s[6]  = { 1.0, 0.0, 0.0,-1.0,-0.5, 1.0};
double transform_sw[6] = {-1.0, 0.0, 0.0,-1.0, 1.0, 1.0};
double transform_w[6]  = { 0.0,-1.0, 1.0, 0.0,-0.5, 1.0};

SPSelTransHandle handles_scale[] = {
	{GTK_ANCHOR_SE, GDK_TOP_LEFT_CORNER,NULL,sp_handle_scale, transform_nw,0,1},
	{GTK_ANCHOR_S, GDK_TOP_SIDE,NULL,sp_handle_stretch, transform_n,0.5,1},
	{GTK_ANCHOR_SW, GDK_TOP_RIGHT_CORNER,NULL,sp_handle_scale, transform_ne,1,1},
	{GTK_ANCHOR_W, GDK_RIGHT_SIDE,NULL,sp_handle_stretch, transform_e,1,0.5},
	{GTK_ANCHOR_NW, GDK_BOTTOM_RIGHT_CORNER,NULL,sp_handle_scale, transform_se,1,0},
	{GTK_ANCHOR_N, GDK_BOTTOM_SIDE,NULL,sp_handle_stretch, transform_s,0.5,0},
	{GTK_ANCHOR_NE, GDK_BOTTOM_LEFT_CORNER,NULL,sp_handle_scale, transform_sw,0,0},
	{GTK_ANCHOR_E, GDK_LEFT_SIDE,NULL,sp_handle_stretch, transform_w,0,0.5}
};

SPSelTransHandle handles_rotate[] = {
	{GTK_ANCHOR_SE, GDK_EXCHANGE, NULL, sp_handle_rotate, NULL, 0,1},
	{GTK_ANCHOR_S, GDK_SB_H_DOUBLE_ARROW, NULL, sp_handle_skew, transform_n, 0.5,1},
	{GTK_ANCHOR_SW, GDK_EXCHANGE, NULL, sp_handle_rotate, NULL, 1,1},
	{GTK_ANCHOR_W, GDK_SB_V_DOUBLE_ARROW, NULL, sp_handle_skew, transform_e, 1,0.5},
	{GTK_ANCHOR_NW, GDK_EXCHANGE, NULL, sp_handle_rotate, NULL, 1,0},
	{GTK_ANCHOR_N, GDK_SB_H_DOUBLE_ARROW, NULL, sp_handle_skew, transform_s, 0.5,0},
	{GTK_ANCHOR_NE, GDK_EXCHANGE, NULL, sp_handle_rotate, NULL, 0,0},
	{GTK_ANCHOR_E, GDK_SB_V_DOUBLE_ARROW, NULL, sp_handle_skew, transform_w, 0,0.5}
};

SPSelTransHandle handle_center =
	{GTK_ANCHOR_CENTER, GDK_CROSSHAIR, NULL, sp_handle_center, NULL, 0.5, 0.5};

