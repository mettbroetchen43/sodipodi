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

#ifndef SODIPODI_APPDATADIR
#ifdef WIN32
#define SODIPODI_APPDATADIR sp_win32_get_appdata_dir ()
#else
#define SODIPODI_APPDATADIR g_get_home_dir ()
#endif
#endif

#ifndef SODIPODI_DOCDIR
#ifdef WIN32
#define SODIPODI_DOCDIR sp_win32_get_doc_dir ()
#else
#define SODIPODI_DOCDIR g_get_home_dir ()
#endif
#endif

#ifndef PACKAGE_LOCALE_DIR
#ifdef WIN32
#define PACKAGE_LOCALE_DIR sp_win32_get_locale_dir ()
#else
#define PACKAGE_LOCALE_DIR (exit (1),(const unsigned char *) "")
#endif
#endif

#ifndef SODIPODI_EXTENSIONDIR
#define SODIPODI_EXTENSIONDIR (exit (1),(const unsigned char *) "")
#endif

#ifndef SODIPODI_EXTENSIONDATADIR
#define SODIPODI_EXTENSIONDATADIR (exit (1),(const unsigned char *) "")
#endif

#endif
