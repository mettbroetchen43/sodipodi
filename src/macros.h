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

#ifdef SP_MACROS_SILENT
#define SP_PRINT_TRANSFORM(s,t)
#define SP_PRINT_DRECT(s,r)
#else
#define SP_PRINT_TRANSFORM(s,t) g_print ("%s (%g %g %g %g %g %g)\n", (s), (t)[0], (t)[1], (t)[2], (t)[3], (t)[4], (t)[5])
#define SP_PRINT_DRECT(s,r) g_print ("%s (%g %g %g %g)\n", (s), (r)->x0, (r)->y0, (r)->x1, (r)->y1)
#endif

#endif
