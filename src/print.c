#define __SP_PRINT_C__

/*
 * Frontend to printing
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "testing.h"

#include <string.h>
#include <ctype.h>

#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-pixblock.h>

#include "helper/sp-intl.h"
#include "enums.h"
#include "document.h"
#include "sp-item.h"
#include "style.h"
#include "sp-paint-server.h"
#include "module.h"
#include "print.h"

unsigned int
sp_print_bind (SPPrintContext *ctx, const NRMatrixF *transform, float opacity)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->bind)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->bind (SP_MODULE_PRINT (ctx), transform, opacity);

	return 0;
}

unsigned int
sp_print_release (SPPrintContext *ctx)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->release)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->release (SP_MODULE_PRINT (ctx));

	return 0;
}

unsigned int
sp_print_fill (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
	       const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->fill)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->fill (SP_MODULE_PRINT (ctx), bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

unsigned int
sp_print_stroke (SPPrintContext *ctx, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
		 const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->stroke)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->stroke (SP_MODULE_PRINT (ctx), bpath, ctm, style, pbox, dbox, bbox);

	return 0;
}

unsigned int
sp_print_image_R8G8B8A8_N (SPPrintContext *ctx,
			   unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			   const NRMatrixF *transform, const SPStyle *style)
{
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->image)
		return ((SPModulePrintClass *) G_OBJECT_GET_CLASS (ctx))->image (SP_MODULE_PRINT (ctx), px, w, h, rs, transform, style);

	return 0;
}

#ifndef WITHOUT_PRINT_UI

#ifdef WIN32
#include <modules/win32.h>
#endif

#ifdef WITH_GNOME_PRINT
#include <modules/gnome.h>
#endif

#ifdef WITH_KDE
#include <modules/kde.h>
#endif

#include "display/nr-arena.h"
#include "display/nr-arena-item.h"

/* UI */

void
sp_print_preview_document (SPDocument *doc)
{
	SPModulePrint *mod;
	unsigned int ret;

	sp_document_ensure_up_to_date (doc);

	mod = NULL;
#ifdef WIN32
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_WIN32);
#endif
#ifdef WITH_KDE
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_KDE);
#endif
#ifdef WITH_GNOME_PRINT
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_GNOME);
#endif

#ifdef USE_PRINT_DRIVERS
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN2);
#else
	/* Still nothing, fall back to plain */
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN);
#endif

	ret = FALSE;
	if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->set_preview)
		ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->set_preview (mod);

	if (ret) {
		/* fixme: This has to go into module constructor somehow */
		/* Create new arena */
		mod->base = SP_ITEM (sp_document_root (doc));
		mod->arena = (NRArena *) nr_object_new (NR_TYPE_ARENA);
		mod->dkey = sp_item_display_key_new (1);
		mod->root = sp_item_invoke_show (mod->base, mod->arena, mod->dkey, SP_ITEM_SHOW_PRINT);
		/* Print document */
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin (mod, doc);
		sp_item_invoke_print (mod->base, (SPPrintContext *) mod);
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish (mod);
		/* Release arena */
		sp_item_invoke_hide (mod->base, mod->dkey);
		mod->base = NULL;
		nr_arena_item_unref (mod->root);
		mod->root = NULL;
		nr_object_unref ((NRObject *) mod->arena);
		mod->arena = NULL;
	}

	g_object_unref (G_OBJECT (mod));
}

void
sp_print_document (SPDocument *doc, unsigned int direct, const unsigned char *filename)
{
	SPModulePrint *mod;
	unsigned int ret;

	sp_document_ensure_up_to_date (doc);

	mod = NULL;
#ifdef USE_PRINT_DRIVERS
	if (direct) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN2);
#else
	if (direct) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN);
#endif

#ifdef WIN32
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_WIN32);
#endif
#ifdef WITH_KDE
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_KDE);
#endif
#ifdef WITH_GNOME_PRINT
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_GNOME);
#endif
#ifdef USE_PRINT_DRIVERS
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN2);
#else
	if (!mod) mod = (SPModulePrint *) sp_module_find (SP_MODULE_KEY_PRINT_PLAIN);
#endif

	ret = FALSE;
        if (filename != NULL) {
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup_file)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup_file (mod, filename);
        } else {
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->setup (mod);
        }

	if (ret) {
		/* fixme: This has to go into module constructor somehow */
		/* Create new arena */
		mod->base = SP_ITEM (sp_document_root (doc));
		mod->arena = (NRArena *) nr_object_new (NR_TYPE_ARENA);
		mod->dkey = sp_item_display_key_new (1);
		mod->root = sp_item_invoke_show (mod->base, mod->arena, mod->dkey, SP_ITEM_SHOW_PRINT);
		/* Print document */
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->begin (mod, doc);
		sp_item_invoke_print (mod->base, (SPPrintContext *) mod);
		if (((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish)
			ret = ((SPModulePrintClass *) G_OBJECT_GET_CLASS (mod))->finish (mod);
		/* Release arena */
		sp_item_invoke_hide (mod->base, mod->dkey);
		mod->base = NULL;
		nr_arena_item_unref (mod->root);
		mod->root = NULL;
		nr_object_unref ((NRObject *) mod->arena);
		mod->arena = NULL;
	}

	g_object_unref (G_OBJECT (mod));
}


#endif
