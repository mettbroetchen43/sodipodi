#ifndef __SP_ITEM_TRANSFORM_H__
#define __SP_ITEM_TRANSFORM_H__

/*
 * Item transformations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"

void sp_desktop_set_i2d_transform_f (SPDesktop *dt, SPItem *item, const NRMatrixF *transform);

void sp_item_rotate_relative (SPDesktop *dt, SPItem *item, float theta_deg, unsigned int commit);

void sp_item_scale_rel (SPDesktop *dt, SPItem *item, double dx, double dy);
void sp_item_skew_rel (SPDesktop *dt, SPItem *item, double dx, double dy); 
void sp_item_move_rel (SPDesktop *dt, SPItem *item, double dx, double dy);

#endif
