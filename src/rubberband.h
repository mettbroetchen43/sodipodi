#ifndef __RUBBERBAND_H__
#define __RUBBERBAND_H__

/*
 * Rubberbanding selector
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <libart_lgpl/art_rect.h>
#include "desktop-handles.h"

/* fixme: do multidocument safe */

void sp_rubberband_start (SPDesktop * desktop, double x, double y);
void sp_rubberband_move (double x, double y);
void sp_rubberband_stop (void);

gboolean sp_rubberband_rect (ArtDRect * rect);

#endif
