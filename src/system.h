#ifndef __SP_SYSTEM_H__
#define __SP_SYSTEM_H__

/*
 * System paths and such
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef WIN32
#include "modules/win32.h"
#endif

#ifndef SODIPODI_PIXMAPDIR
#ifdef WIN32
#define SODIPODI_PIXMAPDIR sp_win32_get_data_dir ()
#else
#define SODIPODI_PIXMAPDIR (exit (1),(const unsigned char *) "")
#endif
#endif

#ifndef SODIPODI_EXTENSIONDIR
#define SODIPODI_EXTENSIONDIR (exit (1),(const unsigned char *) "")
#endif

#ifndef SODIPODI_EXTENSIONDATADIR
#define SODIPODI_EXTENSIONDATADIR (exit (1),(const unsigned char *) "")
#endif

#endif
