#ifndef __SP_KDE_H__
#define __SP_KDE_H__

/*
 * KDE utilities for Sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>

G_BEGIN_DECLS

#include "module.h"

void sp_kde_init (int argc, char **argv, const char *name);
void sp_kde_finish (void);

char *sp_kde_get_open_filename (void);

SPModulePrint *sp_kde_get_module_print (void);

G_END_DECLS

#endif
