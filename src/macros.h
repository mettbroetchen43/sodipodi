#ifndef __MACROS_H__
#define __MACROS_H__

/*
 * Useful macros for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#define SP_PRINT_TRANSFORM(s,t) g_print ("%s (%g %g %g %g %g %g)\n", s, (t)[0], (t)[1], (t)[2], (t)[3], (t)[4], (t)[5])

#endif
