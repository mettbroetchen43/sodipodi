#define __NR_MATRIX_C__

/*
 * Pixel buffer rendering library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "nr-matrix.h"

NRMatrixD *
nr_matrix_multiply_ddd (NRMatrixD *d, const NRMatrixD *m0, const NRMatrixD *m1)
{
	if (m0) {
		if (m1) {
			double d0, d1, d2, d3, d4, d5;

			d0 = m0->c[0] * m1->c[0] + m0->c[1] * m1->c[2];
			d1 = m0->c[0] * m1->c[1] + m0->c[1] * m1->c[3];
			d2 = m0->c[2] * m1->c[0] + m0->c[3] * m1->c[2];
			d3 = m0->c[2] * m1->c[1] + m0->c[3] * m1->c[3];
			d4 = m0->c[4] * m1->c[0] + m0->c[5] * m1->c[2] + m1->c[4];
			d5 = m0->c[4] * m1->c[1] + m0->c[5] * m1->c[3] + m1->c[5];
			d->c[0] = d0;
			d->c[1] = d1;
			d->c[2] = d2;
			d->c[3] = d3;
			d->c[4] = d4;
			d->c[5] = d5;
		} else {
			*d = *m0;
		}
	} else {
		if (m1) {
			*d = *m1;
		} else {
			nr_matrix_d_set_identity (d);
		}
	}

	return d;
}

NRMatrixF *
nr_matrix_multiply_fdd (NRMatrixF *d, const NRMatrixD *m0, const NRMatrixD *m1)
{
	if (m0) {
		if (m1) {
			d->c[0] = m0->c[0] * m1->c[0] + m0->c[1] * m1->c[2];
			d->c[1] = m0->c[0] * m1->c[1] + m0->c[1] * m1->c[3];
			d->c[2] = m0->c[2] * m1->c[0] + m0->c[3] * m1->c[2];
			d->c[3] = m0->c[2] * m1->c[1] + m0->c[3] * m1->c[3];
			d->c[4] = m0->c[4] * m1->c[0] + m0->c[5] * m1->c[2] + m1->c[4];
			d->c[5] = m0->c[4] * m1->c[1] + m0->c[5] * m1->c[3] + m1->c[5];
		} else {
			d->c[0] = m0->c[0];
			d->c[1] = m0->c[1];
			d->c[2] = m0->c[2];
			d->c[3] = m0->c[3];
			d->c[4] = m0->c[4];
			d->c[5] = m0->c[5];
		}
	} else {
		if (m1) {
			d->c[0] = m1->c[0];
			d->c[1] = m1->c[1];
			d->c[2] = m1->c[2];
			d->c[3] = m1->c[3];
			d->c[4] = m1->c[4];
			d->c[5] = m1->c[5];
		} else {
			nr_matrix_f_set_identity (d);
		}
	}

	return d;
}

NRMatrixF *
nr_matrix_multiply_fdf (NRMatrixF *d, const NRMatrixD *m0, const NRMatrixF *m1)
{
	if (m0) {
		if (m1) {
			float d0, d1, d2, d3, d4, d5;

			d0 = m0->c[0] * m1->c[0] + m0->c[1] * m1->c[2];
			d1 = m0->c[0] * m1->c[1] + m0->c[1] * m1->c[3];
			d2 = m0->c[2] * m1->c[0] + m0->c[3] * m1->c[2];
			d3 = m0->c[2] * m1->c[1] + m0->c[3] * m1->c[3];
			d4 = m0->c[4] * m1->c[0] + m0->c[5] * m1->c[2] + m1->c[4];
			d5 = m0->c[4] * m1->c[1] + m0->c[5] * m1->c[3] + m1->c[5];
			d->c[0] = d0;
			d->c[1] = d1;
			d->c[2] = d2;
			d->c[3] = d3;
			d->c[4] = d4;
			d->c[5] = d5;
		} else {
			d->c[0] = m0->c[0];
			d->c[1] = m0->c[1];
			d->c[2] = m0->c[2];
			d->c[3] = m0->c[3];
			d->c[4] = m0->c[4];
			d->c[5] = m0->c[5];
		}
	} else {
		if (m1) {
			*d = *m1;
		} else {
			nr_matrix_f_set_identity (d);
		}
	}

	return d;
}

NRMatrixF *
nr_matrix_multiply_ffd (NRMatrixF *d, const NRMatrixF *m0, const NRMatrixD *m1)
{
	if (m0) {
		if (m1) {
			float d0, d1, d2, d3, d4, d5;

			d0 = m0->c[0] * m1->c[0] + m0->c[1] * m1->c[2];
			d1 = m0->c[0] * m1->c[1] + m0->c[1] * m1->c[3];
			d2 = m0->c[2] * m1->c[0] + m0->c[3] * m1->c[2];
			d3 = m0->c[2] * m1->c[1] + m0->c[3] * m1->c[3];
			d4 = m0->c[4] * m1->c[0] + m0->c[5] * m1->c[2] + m1->c[4];
			d5 = m0->c[4] * m1->c[1] + m0->c[5] * m1->c[3] + m1->c[5];
			d->c[0] = d0;
			d->c[1] = d1;
			d->c[2] = d2;
			d->c[3] = d3;
			d->c[4] = d4;
			d->c[5] = d5;
		} else {
			*d = *m0;
		}
	} else {
		if (m1) {
			d->c[0] = m1->c[0];
			d->c[1] = m1->c[1];
			d->c[2] = m1->c[2];
			d->c[3] = m1->c[3];
			d->c[4] = m1->c[4];
			d->c[5] = m1->c[5];
		} else {
			nr_matrix_f_set_identity (d);
		}
	}

	return d;
}

NRMatrixF *
nr_matrix_multiply_fff (NRMatrixF *d, const NRMatrixF *m0, const NRMatrixF *m1)
{
	if (m0) {
		if (m1) {
			float d0, d1, d2, d3, d4, d5;

			d0 = m0->c[0] * m1->c[0] + m0->c[1] * m1->c[2];
			d1 = m0->c[0] * m1->c[1] + m0->c[1] * m1->c[3];
			d2 = m0->c[2] * m1->c[0] + m0->c[3] * m1->c[2];
			d3 = m0->c[2] * m1->c[1] + m0->c[3] * m1->c[3];
			d4 = m0->c[4] * m1->c[0] + m0->c[5] * m1->c[2] + m1->c[4];
			d5 = m0->c[4] * m1->c[1] + m0->c[5] * m1->c[3] + m1->c[5];
			d->c[0] = d0;
			d->c[1] = d1;
			d->c[2] = d2;
			d->c[3] = d3;
			d->c[4] = d4;
			d->c[5] = d5;
		} else {
			*d = *m0;
		}
	} else {
		if (m1) {
			*d = *m1;
		} else {
			nr_matrix_f_set_identity (d);
		}
	}

	return d;
}

NRMatrixD *
nr_matrix_d_invert (NRMatrixD *d, const NRMatrixD *m)
{
	if (m) {
		double det;
		det = m->c[0] * m->c[3] - m->c[1] * m->c[2];
		if (!NR_DF_TEST_CLOSE (det, 1.0, NR_EPSILON_D)) {
			double rdet, t;
			rdet = 1.0 / det;
			t = m->c[3] * rdet;
			d->c[3] = m->c[0] * rdet;
			d->c[0] = t;
			t = -m->c[1] * rdet;
			d->c[1] = -m->c[1] * rdet;
			d->c[2] = -m->c[2] * rdet;
			d->c[4] = -m->c[4] * d->c[0] - m->c[5] * d->c[2];
			d->c[5] = -m->c[4] * d->c[1] - m->c[5] * d->c[3];
		} else {
			nr_matrix_d_set_identity (d);
		}
	} else {
		nr_matrix_d_set_identity (d);
	}

	return d;
}

NRMatrixF *
nr_matrix_f_invert (NRMatrixF *d, const NRMatrixF *m)
{
	if (m) {
		float det;
		det = m->c[0] * m->c[3] - m->c[1] * m->c[2];
		if (!NR_DF_TEST_CLOSE (det, 1.0, NR_EPSILON_F)) {
			float rdet, t;
			rdet = 1.0 / det;
			t = m->c[3] * rdet;
			d->c[3] = m->c[0] * rdet;
			d->c[0] = t;
			t = -m->c[1] * rdet;
			d->c[1] = -m->c[1] * rdet;
			d->c[2] = -m->c[2] * rdet;
			d->c[4] = -m->c[4] * d->c[0] - m->c[5] * d->c[2];
			d->c[5] = -m->c[4] * d->c[1] - m->c[5] * d->c[3];
		} else {
			nr_matrix_f_set_identity (d);
		}
	} else {
		nr_matrix_f_set_identity (d);
	}

	return d;
}

NRMatrixD *
nr_matrix_d_set_translate (NRMatrixD *m, double x, double y)
{
	m->c[0] = 1.0;
	m->c[1] = 0.0;
	m->c[2] = 0.0;
	m->c[3] = 1.0;
	m->c[4] = x;
	m->c[5] = y;

	return m;
}

NRMatrixF *
nr_matrix_f_set_translate (NRMatrixF *m, float x, float y)
{
	m->c[0] = 1.0;
	m->c[1] = 0.0;
	m->c[2] = 0.0;
	m->c[3] = 1.0;
	m->c[4] = x;
	m->c[5] = y;

	return m;
}

NRMatrixD *
nr_matrix_d_set_scale (NRMatrixD *m, double sx, double sy)
{
	m->c[0] = sx;
	m->c[1] = 0.0;
	m->c[2] = 0.0;
	m->c[3] = sy;
	m->c[4] = 0.0;
	m->c[5] = 0.0;

	return m;
}

NRMatrixD *
nr_matrix_f_set_scale (NRMatrixD *m, float sx, float sy)
{
	m->c[0] = sx;
	m->c[1] = 0.0;
	m->c[2] = 0.0;
	m->c[3] = sy;
	m->c[4] = 0.0;
	m->c[5] = 0.0;

	return m;
}

