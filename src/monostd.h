/*
 * Compatibility layer
 *
 * Authors:
 *
 * Copyright (C) 1999-2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef __MONOSTD_H__
#define __MONOSTD_H__

/* 
 * Compatibility defines for poor platforms which don't have
 * 'unistd.h'. Currently only WIN32 supported.
 */

#ifndef WIN32

/* For UNIX-likes include <unistd.h> directly */
#include <unistd.h>

#else

/* For windows inlcude <glib.h> and define some macros */
#include <glib.h> /* part of the compatibiloity defines are here */
#include <io.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#include <direct.h>
#define mkdir(n,f) _mkdir(n)

#endif

#endif
