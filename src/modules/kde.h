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

char *sp_kde_get_open_filename (unsigned char *dir, unsigned char *filter, unsigned char *title);
char *sp_kde_get_write_filename (unsigned char *dir, unsigned char *filter, unsigned char *title);

char *sp_kde_get_save_filename (unsigned char *dir, unsigned int *spns);

#define SP_TYPE_MODULE_PRINT_KDE (sp_module_print_kde_get_type())
#define SP_MODULE_PRINT_KDE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_KDE, SPModulePrintKDE))
#define SP_IS_MODULE_PRINT_KDE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_KDE))

typedef struct _SPModulePrintKDE SPModulePrintKDE;
typedef struct _SPModulePrintKDEClass SPModulePrintKDEClass;

GType sp_module_print_kde_get_type (void);

#if 0
SPModulePrint *sp_kde_get_module_print (void);
#endif

G_END_DECLS

#endif
