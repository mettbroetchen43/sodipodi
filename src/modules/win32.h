#ifndef __SP_MODULE_WIN32_H__
#define __SP_MODULE_WIN32_H__

/*
 * Windows stuff
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <config.h>

#ifndef WIN32
#error "This file is only usable for Windows"
#endif

#ifdef DATADIR
#undef DATADIR
#endif
#include <windows.h>

#include <module.h>

#define SP_TYPE_MODULE_PRINT_WIN32 (sp_module_print_win32_get_type())
#define SP_MODULE_PRINT_WIN32(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_WIN32, SPModulePrintWin32))
#define SP_IS_MODULE_PRINT_WIN32(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_WIN32))

typedef struct _SPModulePrintWin32 SPModulePrintWin32;
typedef struct _SPModulePrintWin32Class SPModulePrintWin32Class;

struct _SPModulePrintWin32 {
	SPModulePrint module;

	/* Document dimensions */
	float width;
	float height;

	HDC hDC;
};

struct _SPModulePrintWin32Class {
	SPModulePrintClass module_print_class;
};

unsigned int sp_module_print_win32_get_type (void);

#endif
