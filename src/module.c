#define __SP_MODULE_C__

/*
 * Frontend to certain, possibly pluggable, actions
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "helper/sp-intl.h"
#include "dir-util.h"
#include "sp-object.h"
#include "document.h"
#include "module.h"

/* SPModule */

static void sp_module_class_init (SPModuleClass *klass);
static void sp_module_init (SPModule *module);
static void sp_module_finalize (GObject *object);

static void sp_module_private_build (SPModule *module, SPRepr *repr);

static const unsigned char *sp_module_get_unique_id (unsigned char *c, int len, const unsigned char *val);
static void sp_module_register (SPModule *module);
static void sp_module_unregister (SPModule *module);

static GObjectClass *module_parent_class;

unsigned int
sp_module_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_class_init,
			NULL, NULL,
			sizeof (SPModule),
			16,
			(GInstanceInitFunc) sp_module_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "SPModule", &info, 0);
	}
	return type;
}

static void
sp_module_class_init (SPModuleClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = (GObjectClass *)klass;

	module_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_finalize;

	klass->build = sp_module_private_build;
}

static void
sp_module_init (SPModule *module)
{
	module->about = TRUE;
}

static void
sp_module_finalize (GObject *object)
{
	SPModule *module;

	module = SP_MODULE (object);

	sp_module_unregister (module);

	if (module->repr) sp_repr_unref (module->repr);

	/* fixme: Free everything */
	if (module->name) {
		g_free (module->name);
	}

	G_OBJECT_CLASS (module_parent_class)->finalize (object);
}

static void
sp_module_private_build (SPModule *module, SPRepr *repr)
{
	if (repr) {
		const unsigned char *val;
		unsigned char c[256];
		val = sp_repr_attr (repr, "id");
		val = sp_module_get_unique_id (c, 256, val);
		module->id = g_strdup (val);
		val = sp_repr_attr (repr, "name");
		if (val) {
			module->name = g_strdup (val);
		}
		val = sp_repr_attr (repr, "about");
		module->about = (val && (!strcasecmp (val, "true") || !strcasecmp (val, "yes") || !strcasecmp (val, "y") || atoi (val)));
		val = sp_repr_attr (repr, "icon");
		if (val) {
			module->icon = g_strdup (val);
		}
		val = sp_repr_attr (repr, "toolbox");
		module->toolbox = (val && (!strcasecmp (val, "true") || !strcasecmp (val, "yes") || !strcasecmp (val, "y") || atoi (val)));
		sp_module_register (module);
	}
}

SPModule *
sp_module_ref (SPModule *mod)
{
	g_object_ref (G_OBJECT (mod));
	return mod;
}

SPModule *
sp_module_unref (SPModule *mod)
{
	g_object_unref (G_OBJECT (mod));
	return NULL;
}

static GHashTable *moduledict = NULL;

static const unsigned char *
sp_module_get_unique_id (unsigned char *c, int len, const unsigned char *val)
{
	static int mnumber = 0;
	if (!moduledict) moduledict = g_hash_table_new (g_str_hash, g_str_equal);
	while (!val || g_hash_table_lookup (moduledict, val)) {
		g_snprintf (c, len, "Module_%d", ++mnumber);
		val = c;
	}
	return val;
}

static void
sp_module_register (SPModule *module)
{
	g_hash_table_insert (moduledict, module->id, module);
}

static void
sp_module_unregister (SPModule *module)
{
	g_hash_table_remove (moduledict, module->id);
}

/* ModuleInput */

SPDocument *
sp_module_input_document_open (SPModuleInput *mod, const unsigned char *uri, unsigned int advertize, unsigned int keepalive)
{
	return sp_document_new (uri, advertize, keepalive);
}

/* ModuleOutput */

void
sp_module_output_document_save (SPModuleOutput *mod, SPDocument *doc, const unsigned char *uri)
{
	SPRepr *repr;
	gboolean spns;
	const GSList *images, *l;
	SPReprDoc *rdoc;
	const gchar *save_path;

	if (!doc) return;
	if (!uri) return;

	save_path = g_dirname (uri);

	spns = (!SP_MODULE_ID (mod) || !strcmp (SP_MODULE_ID (mod), SP_MODULE_KEY_OUTPUT_SVG_SODIPODI));
	if (spns) {
		rdoc = NULL;
		repr = sp_document_repr_root (doc);
		sp_repr_set_attr (repr, "sodipodi:docbase", save_path);
		sp_repr_set_attr (repr, "sodipodi:docname", uri);
	} else {
		rdoc = sp_repr_document_new ("svg");
		repr = sp_repr_document_root (rdoc);
		repr = sp_object_invoke_write (sp_document_root (doc), repr, SP_OBJECT_WRITE_BUILD);
	}

	images = sp_document_get_resource_list (doc, "image");
	for (l = images; l != NULL; l = l->next) {
		SPRepr *ir;
		const guchar *href, *relname;
		ir = SP_OBJECT_REPR (l->data);
		href = sp_repr_attr (ir, "xlink:href");
		if (spns && !g_path_is_absolute (href)) {
			href = sp_repr_attr (ir, "sodipodi:absref");
		}
		if (href && g_path_is_absolute (href)) {
			relname = sp_relative_path_from_path (href, save_path);
			sp_repr_set_attr (ir, "xlink:href", relname);
		}
	}

	/* fixme: */
	sp_document_set_undo_sensitive (doc, FALSE);
	sp_repr_set_attr (repr, "sodipodi:modified", NULL);
	sp_document_set_undo_sensitive (doc, TRUE);

	sp_repr_save_file (sp_repr_document (repr), uri);
	sp_document_set_uri (doc, uri);

	if (!spns) sp_repr_document_unref (rdoc);
}

/* Global methods */

#include "modules/sp-module-sys.h"

SPModule *
sp_module_system_get (const unsigned char *key)
{
	SPModule *mod;
	if (!moduledict) moduledict = g_hash_table_new (g_str_hash, g_str_equal);
	mod = g_hash_table_lookup (moduledict, key);
	if (mod) sp_module_ref (mod);
	return mod;
}

void
sp_module_system_menu_open (SPMenu *menu)
{
	sp_menu_append (menu, _("Scalable Vector Graphic (SVG)"), _("Sodipodi native file format and W3C standard"),
			SP_MODULE_KEY_INPUT_SVG);
}

void
sp_module_system_menu_save (SPMenu *menu)
{
	sp_menu_append (menu,
			_("SVG with \"xmlns:sodipodi\" namespace"),
			_("Scalable Vector Graphics format with sodipodi extensions"),
			SP_MODULE_KEY_OUTPUT_SVG_SODIPODI);
	sp_menu_append (menu,
			_("Plain SVG"),
			_("Scalable Vector Graphics format"),
			SP_MODULE_KEY_OUTPUT_SVG);
}


