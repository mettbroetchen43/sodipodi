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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32
#error "This file is only usable for Windows"
#endif

#ifdef DATADIR
#undef DATADIR
#endif
#include <windows.h>

#include <module.h>

/* Initialization */

void sp_win32_init (int argc, char **argv, const char *name);
void sp_win32_finish (void);

/* Printing */

#define SP_TYPE_MODULE_PRINT_WIN32 (sp_module_print_win32_get_type())
#define SP_MODULE_PRINT_WIN32(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_WIN32, SPModulePrintWin32))
#define SP_IS_MODULE_PRINT_WIN32(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_WIN32))

typedef struct _SPModulePrintWin32 SPModulePrintWin32;
typedef struct _SPModulePrintWin32Class SPModulePrintWin32Class;

struct _SPModulePrintWin32 {
	SPModulePrint module;

	/* Document dimensions */
	float PageWidth;
	float PageHeight;

	HDC hDC;

	unsigned int landscape;
};

struct _SPModulePrintWin32Class {
	SPModulePrintClass module_print_class;
};

GType sp_module_print_win32_get_type (void);

/* File dialogs */

char *sp_win32_get_open_filename (unsigned char *dir, unsigned char *filter, unsigned char *title);
char *sp_win32_get_write_filename (unsigned char *dir, unsigned char *filter, unsigned char *title);

char *sp_win32_get_save_filename (unsigned char *dir, unsigned int *spns);

/* fixme: */

const char *sp_win32_get_data_dir (void);
const char *sp_win32_get_doc_dir (void);
const char *sp_win32_get_locale_dir (void);
const char *sp_win32_get_appdata_dir (void);

#include <libarikkei/arikkei-strlib.h>

#ifdef _UNICODE
#define sp_w32_utf8_strdup(T) arikkei_ucs2_utf8_strdup (T)
#define sp_utf8_w32_strdup(s) arikkei_utf8_ucs2_strdup (s)
#else
#define sp_w32_utf8_strdup(T) sp_multibyte_utf8_strdup (T)
#define sp_utf8_w32_strdup(s) sp_utf8_multibyte_strdup (s)
unsigned char *sp_multibyte_utf8_strdup (const char *mbs);
char *sp_utf8_multibyte_strdup (const unsigned char *utf8);
#endif

#endif
