#ifndef __NR_MATRIX_H__
#define __NR_MATRIX_H__

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

#include <libnr/nr-macros.h>
#include <libnr/nr-values.h>

#define nr_matrix_d_set_identity(m) (*(m) = NR_MATRIX_D_IDENTITY)
#define nr_matrix_f_set_identity(m) (*(m) = NR_MATRIX_F_IDENTITY)

#define nr_matrix_d_test_identity(m,e) (!(m) || NR_MATRIX_DF_TEST_CLOSE (m, &NR_MATRIX_D_IDENTITY, e))
#define nr_matrix_f_test_identity(m,e) (!(m) || NR_MATRIX_DF_TEST_CLOSE (m, &NR_MATRIX_F_IDENTITY, e))

#define nr_matrix_d_test_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_CLOSE (m0, m1, e)))
#define nr_matrix_f_test_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_CLOSE (m0, m1, e)))
#define nr_matrix_d_test_transform_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSFORM_CLOSE (m0, m1, e)))
#define nr_matrix_f_test_transform_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSFORM_CLOSE (m0, m1, e)))
#define nr_matrix_d_test_translate_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSLATE_CLOSE (m0, m1, e)))
#define nr_matrix_f_test_translate_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSLATE_CLOSE (m0, m1, e)))

NRMatrixD *nr_matrix_d_invert (NRMatrixD *d, NRMatrixD *m);
NRMatrixF *nr_matrix_f_invert (NRMatrixF *d, NRMatrixF *m);

NRMatrixD *nr_matrix_multiply_ddd (NRMatrixD *d, const NRMatrixD *m0, const NRMatrixD *m1);
NRMatrixF *nr_matrix_multiply_fdd (NRMatrixF *d, const NRMatrixD *m0, const NRMatrixD *m1);
NRMatrixF *nr_matrix_multiply_fdf (NRMatrixF *d, const NRMatrixD *m0, const NRMatrixF *m1);
NRMatrixF *nr_matrix_multiply_ffd (NRMatrixF *d, const NRMatrixF *m0, const NRMatrixD *m1);

NRMatrixD *nr_matrix_d_set_scale (NRMatrixD *m, double sx, double sy);
NRMatrixD *nr_matrix_f_set_scale (NRMatrixD *m, float sx, float sy);

#endif
