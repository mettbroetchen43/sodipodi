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
#define SP_PRINT_MATRIX(s,m)
#define SP_PRINT_TRANSFORM(s,t)
#define SP_PRINT_DRECT(s,r)
#define SP_PRINT_DRECT_WH(s,r)
#define SP_PRINT_IRECT(s,r)
#define SP_PRINT_IRECT_WH(s,r)
#else
#define SP_PRINT_MATRIX(s,m) g_print ("%s (%g %g %g %g %g %g)\n", (s), (m)->c[0], (m)->c[1], (m)->c[2], (m)->c[3], (m)->c[4], (m)->c[5])
#define SP_PRINT_TRANSFORM(s,t) g_print ("%s (%g %g %g %g %g %g)\n", (s), (t)[0], (t)[1], (t)[2], (t)[3], (t)[4], (t)[5])
#define SP_PRINT_DRECT(s,r) g_print ("%s (%g %g %g %g)\n", (s), (r)->x0, (r)->y0, (r)->x1, (r)->y1)
#define SP_PRINT_DRECT_WH(s,r) g_print ("%s (%g %g %g %g)\n", (s), (r)->x0, (r)->y0, (r)->x1 - (r)->x0, (r)->y1 - (r)->y0)
#define SP_PRINT_IRECT(s,r) g_print ("%s (%d %d %d %d)\n", (s), (r)->x0, (r)->y0, (r)->x1, (r)->y1)
#define SP_PRINT_IRECT_WH(s,r) g_print ("%s (%d %d %d %d)\n", (s), (r)->x0, (r)->y0, (r)->x1 - (r)->x0, (r)->y1 - (r)->y0)
#endif

#define sp_signal_disconnect_by_data(o,d) g_signal_handlers_disconnect_matched ((o), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, (d))
#define sp_signal_disconnect_by_id(o,i) g_signal_handler_disconnect ((o), (i))

#endif
