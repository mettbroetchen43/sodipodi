#define __SP_ITEM_TRANSFORM_C__

/*
 * Transformations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-matrix.h>
#include "svg/svg.h"
#include "sp-item.h"
#include "document.h"
#include "desktop.h"

#include "sp-item-transform.h"

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

#ifdef SP_MACROS_SILENT
#undef SP_MACROS_SILENT
#endif

#include <libart_lgpl/art_affine.h>

#include "macros.h"

void art_affine_skew (double dst[6], double dx, double dy);

void
sp_desktop_set_i2d_transform_f (SPDesktop *dt, SPItem *item, const NRMatrixF *transform)
{
	SPItem *pitem;
	NRMatrixF p2d, d2p, i2p;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (transform != NULL);

	pitem = (SPItem *) SP_OBJECT_PARENT (item);

	if (pitem && SP_IS_ITEM (pitem)) {
		/* This finds parent:master -> desktop */
		sp_desktop_get_i2d_transform_f (dt, (SPItem *) SP_OBJECT_PARENT (item), &p2d);
		/* But we need parent:child -> desktop */
		if (pitem->has_extra_transform) {
			NRMatrixD c2p;
			nr_matrix_d_set_identity (&c2p);
			sp_item_get_extra_transform (pitem, &c2p);
			nr_matrix_multiply_fdf (&p2d, &c2p, &p2d);
		}
	} else {
		nr_matrix_f_set_scale (&p2d, 0.8, -0.8);
		p2d.c[5] = sp_document_height (SP_OBJECT_DOCUMENT (item));
	}

	SP_PRINT_MATRIX ("p2d", &p2d);

	nr_matrix_f_invert (&d2p, &p2d);

	SP_PRINT_MATRIX ("d2p", &d2p);

	nr_matrix_multiply_fff (&i2p, transform, &d2p);

	SP_PRINT_MATRIX ("i2p", &i2p);

	sp_item_set_item_transform (item, &i2p);
}

void
sp_item_rotate_relative (SPDesktop *dt, SPItem *item, float theta_deg, unsigned int commit)
{
	NRRectF b;
	NRMatrixF curt, newt;
	NRMatrixD rotation, s, d2n, u, v;
	double x,y;
	char tstr[80];

	sp_desktop_get_item_bbox (dt, item, &b);

	x = b.x0 + (b.x1 - b.x0) / 2.0F;
	y = b.y0 + (b.y1 - b.y0) / 2.0F;

	nr_matrix_d_set_rotate (&rotation, theta_deg);
	nr_matrix_d_set_translate (&s, x, y);
	nr_matrix_d_set_translate (&d2n, -x, -y);


	tstr[79] = '\0';

	/* rotate item */
	sp_desktop_get_i2d_transform_f (dt, item, &curt);
	nr_matrix_multiply_dfd (&u, &curt, &d2n);
	nr_matrix_multiply_ddd (&v, &u, &rotation);
	nr_matrix_multiply_fdd (&newt, &v, &s);
	sp_desktop_set_i2d_transform_f (dt, item, &newt);

	/* update repr */
	if (sp_svg_transform_write (tstr, 80, &item->transform)) {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
	} else {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
	}
}

void
sp_item_scale_rel (SPDesktop *dt, SPItem *item, double dx, double dy)
{
	NRMatrixF curaff, new;
	NRMatrixD scale, s, t, u, v;
	char tstr[80];
	double x, y;
	NRRectF b;

	sp_desktop_get_item_bbox (dt, item, &b);

	x = b.x0 + (b.x1 - b.x0) / 2;
	y = b.y0 + (b.y1 - b.y0) / 2;

	nr_matrix_d_set_scale (&scale, dx, dy);
	nr_matrix_d_set_translate (&s, x, y);
	nr_matrix_d_set_translate (&t, -x, -y);

	tstr[79] = '\0';

	// scale item
	sp_desktop_get_i2d_transform_f (dt, item, &curaff);
	nr_matrix_multiply_dfd (&u, &curaff, &t);
	nr_matrix_multiply_ddd (&v, &u, &scale);
	nr_matrix_multiply_ddd (&u, &v, &s);
	nr_matrix_f_from_d (&new, &u);

	sp_desktop_set_i2d_transform_f (dt, item, &new);

	//update repr
	if (sp_svg_transform_write (tstr, 79, &item->transform)) {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
	} else {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
	}
} 

void
art_affine_skew (double dst[6], double dx, double dy)
{
  dst[0] = 1;
  dst[1] = dy;
  dst[2] = dx;
  dst[3] = 1;
  dst[4] = 0;
  dst[5] = 0;
}

void
sp_item_skew_rel (SPDesktop *dt, SPItem *item, double dx, double dy)
{
	NRMatrixF cur, new;
	double skew[6], s[6], t[6] ,u[6] ,v[6] ,newaff[6];
	char tstr[80];
	double x,y;
	NRRectF b;

	sp_desktop_get_item_bbox (dt, item, &b);
	x = b.x0 + (b.x1 - b.x0) / 2;
	y = b.y0 + (b.y1 - b.y0) / 2;

	art_affine_skew (skew, dx, dy);
	art_affine_translate (s, x, y);
	art_affine_translate (t, -x, -y);

	tstr[79] = '\0';

	// skew item
	sp_desktop_get_i2d_transform_f (dt, item, &cur);
	nr_matrix_multiply_dfd (NR_MATRIX_D_FROM_DOUBLE (u), &cur, NR_MATRIX_D_FROM_DOUBLE (t));
	art_affine_multiply (v, u, skew);
	art_affine_multiply (newaff, v, s);
	nr_matrix_f_from_d (&new, NR_MATRIX_D_FROM_DOUBLE (newaff));
	sp_desktop_set_i2d_transform_f (dt, item, &new);
	//update repr
	if (sp_svg_transform_write (tstr, 79, &item->transform)) {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
	} else {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
	}
}

void
sp_item_move_rel (SPDesktop *dt, SPItem *item, double dx, double dy)
{
	NRMatrixF cur, new;
	double move[6];
	char tstr[80];

	tstr[79] = '\0';

	// move item
	art_affine_translate (move, dx, dy);
	sp_desktop_get_i2d_transform_f (dt, item, &cur);
	nr_matrix_multiply_ffd (&new, &cur, NR_MATRIX_D_FROM_DOUBLE (move));
	sp_desktop_set_i2d_transform_f (dt, item, &new);

	//update repr
	if (sp_svg_transform_write (tstr, 79, &item->transform)) {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
	} else {
		sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
	}
}

