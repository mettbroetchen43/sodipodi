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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "testing.h"

#include <string.h>
#include <stdlib.h>

#include <gtk/gtkimagemenuitem.h>

#include "helper/sp-intl.h"
#include "widgets/menu.h"
#include "xml/repr-private.h"
#include "modules/ps.h"
#include "dir-util.h"
#include "sodipodi.h"
#include "sp-object.h"
#include "document.h"
#include "module.h"

/* SPModule */

static void sp_module_class_init (SPModuleClass *klass);
static void sp_module_init (SPModule *module);
static void sp_module_finalize (GObject *object);

static void sp_module_private_setup (SPModule *module, SPRepr *repr);

static const unsigned char *sp_module_get_unique_id (unsigned char *c, int len, const unsigned char *val);
static void sp_module_register (SPModule *module);
static void sp_module_unregister (SPModule *module);

static GObjectClass *module_parent_class;

GType
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

	klass->setup = sp_module_private_setup;
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
		free (module->name);
	}

	G_OBJECT_CLASS (module_parent_class)->finalize (object);
}

static void
sp_module_private_setup (SPModule *module, SPRepr *repr)
{
	if (repr) {
		const unsigned char *val;
		val = sp_repr_attr (repr, "name");
		if (val) {
			module->name = g_strdup (val);
		}
		sp_repr_get_boolean (repr, "about", &module->about);
		val = sp_repr_attr (repr, "icon");
		if (val) {
			module->icon = g_strdup (val);
		}
		sp_repr_get_boolean (repr, "toolbox", &module->toolbox);
	}
}

SPModule *
sp_module_new_from_path (GType type, const unsigned char *path)
{
	SPModule *module;
	SPRepr *repr;

	module = NULL;
	repr = sodipodi_get_repr (SODIPODI, path);
	if (repr) {
		const unsigned char *id;
		id = sp_repr_attr (repr, "id");
		if (id) {
			module = g_object_new (type, NULL);
			if (module) sp_module_setup (module, repr, id);
		}
	}
	return module;
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

/* Registers module */
unsigned int
sp_module_setup (SPModule *module, SPRepr *repr, const unsigned char *key)
{
	unsigned char c[256];
	key = sp_module_get_unique_id (c, 256, key);
	module->id = strdup (key);
	if (repr) sp_repr_ref (repr);
	module->repr = repr;
	if (((SPModuleClass *) G_OBJECT_GET_CLASS (module))->setup)
		((SPModuleClass *) G_OBJECT_GET_CLASS (module))->setup (module, repr);
	sp_module_register (module);
	return TRUE;
}

/* Unregisters module */
unsigned int
sp_module_release (SPModule *module)
{
	return TRUE;
}

/* Creates config repr */

SPRepr *
sp_module_write (SPModule *module, SPRepr *repr)
{
	if (((SPModuleClass *) G_OBJECT_GET_CLASS (module))->write)
		return ((SPModuleClass *) G_OBJECT_GET_CLASS (module))->write (module, repr);

	return NULL;
}

/* Executes default action of module */

unsigned int
sp_module_invoke (SPModule *module, SPRepr *config)
{
	if (((SPModuleClass *) G_OBJECT_GET_CLASS (module))->execute)
		return ((SPModuleClass *) G_OBJECT_GET_CLASS (module))->execute (module, config);

	return FALSE;
}

/* Returns new reference to module */

SPModule *
sp_module_find (const unsigned char *key)
{
	SPModule *mod;
	if (!moduledict) moduledict = g_hash_table_new (g_str_hash, g_str_equal);
	mod = g_hash_table_lookup (moduledict, key);
	if (mod) sp_module_ref (mod);
	return mod;
}

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
	if (module->id) g_hash_table_insert (moduledict, module->id, module);
}

static void
sp_module_unregister (SPModule *module)
{
	if (module->id) g_hash_table_remove (moduledict, module->id);
}

/* ModuleInput */

static void sp_module_input_class_init (SPModuleInputClass *klass);
static void sp_module_input_init (SPModuleInput *object);
static void sp_module_input_finalize (GObject *object);

static void sp_module_input_setup (SPModule *module, SPRepr *repr);

static SPModuleClass *input_parent_class;

GType
sp_module_input_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleInputClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_input_class_init,
			NULL, NULL,
			sizeof (SPModuleInput),
			16,
			(GInstanceInitFunc) sp_module_input_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleInput", &info, 0);
	}
	return type;
}

static void
sp_module_input_class_init (SPModuleInputClass *klass)
{
	GObjectClass *g_object_class;
	SPModuleClass *module_class;

	g_object_class = (GObjectClass *) klass;
	module_class = (SPModuleClass *) klass;

	input_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_input_finalize;

	module_class->setup = sp_module_input_setup;
}

static void
sp_module_input_init (SPModuleInput *imod)
{
	/* Nothing here by now */
}

static void
sp_module_input_finalize (GObject *object)
{
	SPModuleInput *imod;

	imod = (SPModuleInput *) object;
	
	if (imod->mimetype) {
		g_free (imod->mimetype);
	}

	if (imod->extention) {
		g_free (imod->extention);
	}

	G_OBJECT_CLASS (input_parent_class)->finalize (object);
}

static void
sp_module_input_setup (SPModule *module, SPRepr *repr)
{
	SPModuleInput *imod;

	imod = (SPModuleInput *) module;

	if (((SPModuleClass *) input_parent_class)->setup)
		((SPModuleClass *) input_parent_class)->setup (module, repr);

	if (repr) {
		const unsigned char *val;
		val = sp_repr_attr (repr, "mimetype");
		if (val) {
			imod->mimetype = g_strdup (val);
		}
		val = sp_repr_attr (repr, "extension");
		if (val) {
			imod->extention = g_strdup (val);
		}
	}
}

SPDocument *
sp_module_input_document_open (SPModuleInput *mod, const unsigned char *uri, unsigned int advertize, unsigned int keepalive)
{
	return sodipodi_document_new (uri, advertize, keepalive);
}

/* ModuleOutput */

static void sp_module_output_class_init (SPModuleOutputClass *klass);
static void sp_module_output_init (SPModuleOutput *omod);
static void sp_module_output_finalize (GObject *object);

static void sp_module_output_setup (SPModule *module, SPRepr *repr);

static SPModuleClass *output_parent_class;

GType sp_module_output_get_type (void) {
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleOutputClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_output_class_init,
			NULL, NULL,
			sizeof (SPModuleOutput),
			16,
			(GInstanceInitFunc) sp_module_output_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleOutput", &info, 0);
	}
	return type;
}

static void
sp_module_output_class_init (SPModuleOutputClass *klass)
{
	GObjectClass *g_object_class;
	SPModuleClass *module_class;

	g_object_class = (GObjectClass *)klass;
	module_class = (SPModuleClass *) klass;

	output_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_output_finalize;

	module_class->setup = sp_module_output_setup;
}

static void
sp_module_output_init (SPModuleOutput *omod)
{
	/* Nothing here */
}

static void
sp_module_output_finalize (GObject *object)
{
	SPModuleOutput *omod;

	omod = (SPModuleOutput *) object;
	
	G_OBJECT_CLASS (output_parent_class)->finalize (object);
}

static void
sp_module_output_setup (SPModule *module, SPRepr *repr)
{
	SPModuleOutput *mo;

	mo = (SPModuleOutput *) module;

	if (((SPModuleClass *) output_parent_class)->setup)
		((SPModuleClass *) output_parent_class)->setup (module, repr);

	if (repr) {
		const unsigned char *val;
		val = sp_repr_attr (repr, "mimetype");
		if (val) {
			mo->mimetype = g_strdup (val);
		}
		val = sp_repr_attr (repr, "extension");
		if (val) {
			mo->extention = g_strdup (val);
		}
	}
}

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
		rdoc = sp_repr_doc_new ("svg");
		repr = sp_repr_doc_get_root (rdoc);
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

	sp_repr_doc_write_file (sp_repr_get_doc (repr), uri);
	sp_document_set_uri (doc, uri);

	if (!spns) sp_repr_doc_unref (rdoc);
}

/* ModuleFilter */

static void sp_module_filter_class_init (SPModuleFilterClass *klass);
static void sp_module_filter_init (SPModuleFilter *fmod);
static void sp_module_filter_finalize (GObject *object);

static SPModuleClass *filter_parent_class;

GType
sp_module_filter_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleFilterClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_filter_class_init,
			NULL, NULL,
			sizeof (SPModuleFilter),
			16,
			(GInstanceInitFunc) sp_module_filter_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleFilter", &info, 0);
	}
	return type;
}

static void
sp_module_filter_class_init (SPModuleFilterClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = (GObjectClass *)klass;

	filter_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_filter_finalize;
}

static void
sp_module_filter_init (SPModuleFilter *fmod)
{
	/* Nothing here */
}

static void
sp_module_filter_finalize (GObject *object)
{
	SPModuleFilter *fmod;

	fmod = (SPModuleFilter *) object;
	
	G_OBJECT_CLASS (filter_parent_class)->finalize (object);
}

/* ModulePrint */

struct _SPPrintContext {
	SPModulePrint module;
};

static void sp_module_print_class_init (SPModulePrintClass *klass);
static void sp_module_print_init (SPModulePrint *fmod);
static void sp_module_print_finalize (GObject *object);

static SPModuleClass *print_parent_class;

GType
sp_module_print_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModulePrintClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_print_class_init,
			NULL, NULL,
			sizeof (SPModulePrint),
			16,
			(GInstanceInitFunc) sp_module_print_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModulePrint", &info, 0);
	}
	return type;
}

static void
sp_module_print_class_init (SPModulePrintClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = (GObjectClass *)klass;

	print_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_finalize;
}

static void
sp_module_print_init (SPModulePrint *fmod)
{
	/* Nothing here */
}

static void
sp_module_print_finalize (GObject *object)
{
	SPModulePrint *fmod;

	fmod = (SPModulePrint *) object;
	
	G_OBJECT_CLASS (print_parent_class)->finalize (object);
}


/* Global methods */

#include <gmodule.h>

#ifdef WITH_GNOME_PRINT
#include "modules/gnome.h"
#endif
#ifdef WITH_KDE
#include "modules/kde.h"
#endif
#ifdef WIN32
#include "modules/win32.h"
#endif

#include "dialogs/xml-tree.h"

/* static SPModule *module_xml_editor = NULL; */
#ifdef USE_PRINT_DRIVERS
static SPModule *module_printing_plain = NULL;
#else
static SPModule *module_printing_ps = NULL;
#endif
#ifdef WIN32
static SPModule *module_printing_win32 = NULL;
#endif
#ifdef WITH_KDE
static SPModule *module_printing_kde = NULL;
#endif
#ifdef WITH_GNOME_PRINT
static SPModule *module_printing_gnome = NULL;
#endif

unsigned int
sp_modules_init (int *argc, const char **argv, unsigned int gui)
{
	SPRepr *repr, *parent;
	SPModule *mod;

#ifdef WITH_KDE
	sp_kde_init (*argc, argv, "Sodipodi");
#endif
#ifdef WIN32
	sp_win32_init (0, NULL, "Sodipodi");
#endif

#ifdef USE_PRINT_DRIVERS
	module_printing_plain = (SPModule *) g_object_new (SP_TYPE_MODULE_PRINT_PLAIN, NULL);
	repr = sodipodi_get_repr (SODIPODI, "extensions.printing.plain");
	if (!repr) {
		parent = sodipodi_get_repr (SODIPODI, "extensions");
		repr = sp_module_write (module_printing_plain, parent);
	}
	sp_module_setup (module_printing_plain, repr, "printing.plain");
#else
	/* Register plain printing module */
	module_printing_ps = (SPModule *) g_object_new (SP_TYPE_MODULE_PRINT_PLAIN, NULL);
	repr = sodipodi_get_repr (SODIPODI, "extensions.printing.ps");
	if (!repr) {
		parent = sodipodi_get_repr (SODIPODI, "extensions");
		repr = sp_module_write (module_printing_ps, parent);
	}
	sp_module_setup (module_printing_ps, repr, "printing.ps");
#endif

#ifdef WIN32
	/* Windows printing */
	module_printing_win32 = (SPModule *) g_object_new (SP_TYPE_MODULE_PRINT_WIN32, NULL);
	repr = sodipodi_get_repr (SODIPODI, "extensions.printing.win32");
	if (!repr) {
		parent = sodipodi_get_repr (SODIPODI, "extensions");
		repr = sp_module_write (module_printing_win32, parent);
	}
	sp_module_setup (module_printing_win32, repr, "printing.win32");
#endif

#ifdef WITH_KDE
	/* KDE printing */
	module_printing_kde = (SPModule *) g_object_new (SP_TYPE_MODULE_PRINT_KDE, NULL);
	repr = sodipodi_get_repr (SODIPODI, "extensions.printing.kde");
	if (!repr) {
		parent = sodipodi_get_repr (SODIPODI, "extensions");
		repr = sp_module_write (module_printing_kde, parent);
	}
	sp_module_setup (module_printing_kde, repr, "printing.kde");
#endif

#ifdef WITH_GNOME_PRINT
	/* KDE printing */
	module_printing_gnome = (SPModule *) g_object_new (SP_TYPE_MODULE_PRINT_GNOME, NULL);
	repr = sodipodi_get_repr (SODIPODI, "extensions.printing.gnome");
	if (!repr) {
		parent = sodipodi_get_repr (SODIPODI, "extensions");
		repr = sp_module_write (module_printing_gnome, parent);
	}
	sp_module_setup (module_printing_gnome, repr, "printing.gnome");
#endif

	/* XML editor module */
	/* module_xml_editor = sp_xml_module_load (); */
	/* sp_module_setup (module_xml_editor, repr, "xmleditor"); */

	/* Default input */
	mod = g_object_new (SP_TYPE_MODULE_INPUT, NULL);
	sp_module_setup (mod, NULL, SP_MODULE_KEY_INPUT_SVG);

	/* Default output */
	mod = g_object_new (SP_TYPE_MODULE_OUTPUT, NULL);
	sp_module_setup (mod, NULL, SP_MODULE_KEY_OUTPUT_SVG);
	mod = g_object_new (SP_TYPE_MODULE_OUTPUT, NULL);
	sp_module_setup (mod, NULL, SP_MODULE_KEY_OUTPUT_SVG_SODIPODI);

	return TRUE;
}

unsigned int
sp_modules_finish (void)
{
#ifdef WITH_KDE
	sp_kde_finish ();
#endif
#ifdef WIN32
	sp_win32_finish ();
#endif

#ifdef USE_PRINT_DRIVERS
	module_printing_plain = sp_module_unref (module_printing_plain);
#else
	/* fixme: Release these */
	module_printing_ps = sp_module_unref (module_printing_ps);
#endif

	return TRUE;
}

static void
sp_modules_menu_action_activate (GtkWidget *widget, SPRepr *repr)
{
	const unsigned char *type;
	SPRepr *actrepr;
	actrepr = sp_repr_lookup_child_by_name (repr, "action");
	if (!actrepr) return;
	type = sp_repr_attr (actrepr, "type");
	if (!type) return;
	if (!strcmp (type, "internal")) {
		const unsigned char *path;
		SPModule *module;
		path = sp_repr_attr (actrepr, "path");
		if (!path) return;
		module = sp_module_find (path);
		if (!module) return;
		sp_module_invoke (module, repr);
		sp_module_unref (module);
	} else if (!strcmp (type, "dll")) {
		const unsigned char *path;
		SPModule *module;
		GModule *gmod;
		path = sp_repr_attr (actrepr, "path");
		if (!path) return;
		module = sp_module_find (path);
		if (module) {
			sp_module_invoke (module, repr);
			sp_module_unref (module);
		} else {
			const unsigned char *dllpath;
			dllpath = sp_repr_attr (actrepr, "dllpath");
			/* fixme: Set as invalid instead */
			if (!dllpath) return;
			gmod = g_module_open (dllpath, G_MODULE_BIND_LAZY);
			if (gmod) {
				SPModule * (*sp_module_load) (void);
				if (g_module_symbol (gmod, "sp_module_load", (gpointer *) &sp_module_load)) {
					module = sp_module_load ();
					if (module) {
						sp_module_setup (module, repr, path);
					} else {
						g_module_close (gmod);
					}
				}
				if (!module) {
					sp_repr_set_attr (repr, "invalid", "true");
				}
			}
			if (module) {
				sp_module_invoke (module, repr);
			}
		}
	}
}

static SPMenu *
sp_modules_menu_action_append (SPMenu *menu, SPRepr *repr, const unsigned char *name)
{
	unsigned int bval;
	GtkWidget *item;
	if (!sp_repr_get_boolean (repr, "action", &bval) || !bval) return menu;
	if (!menu) menu = (SPMenu *) sp_menu_new ();
	item = gtk_image_menu_item_new_with_label (name);
	g_signal_connect ((GObject *) item, "activate", (GCallback) sp_modules_menu_action_activate, repr);
	gtk_widget_show (item);
	gtk_menu_append ((GtkMenu *) menu, item);
	return menu;
}

#include <help.h>

static void
sp_modules_menu_about_activate (GtkWidget *widget, SPRepr *repr)
{
	SPRepr *arepr, *lrepr, *trepr;
	const unsigned char *text;
	arepr = sp_repr_lookup_child_by_name (repr, "about");
	if (!arepr) return;
	lrepr = sp_repr_lookup_child (arepr, "language", "C");
	if (!lrepr) return;
	trepr = sp_repr_get_children (lrepr);
	if (!trepr || !(sp_repr_is_text (trepr) || sp_repr_is_cdata (trepr))) return;
	text = sp_repr_get_content (trepr);
	if (!text || !*text) return;
	sp_help_about_module (text);
}

static SPMenu *
sp_modules_menu_about_append (SPMenu *menu, SPRepr *repr, const unsigned char *name)
{
	GtkWidget *item;
	if (!menu) menu = (SPMenu *) sp_menu_new ();
	item = gtk_image_menu_item_new_with_label (name);
	g_signal_connect ((GObject *) item, "activate", (GCallback) sp_modules_menu_about_activate, repr);
	gtk_widget_show (item);
	gtk_menu_append ((GtkMenu *) menu, item);
	return menu;
}

static GtkWidget *
sp_modules_menu_append_node (SPMenu *menu, SPRepr *repr, SPMenu * (*callback) (SPMenu *, SPRepr *, const unsigned char *))
{
	const unsigned char *name;
	name = sp_repr_attr (repr, "name");
	if (name) {
		if (!strcmp (sp_repr_name (repr), "group")) {
			SPRepr *child;
			GtkWidget *chmenu;
			chmenu = NULL;
			child = sp_repr_get_children (repr);
			while (child) {
				chmenu = sp_modules_menu_append_node ((SPMenu *) chmenu, child, callback);
				child = child->next;
			}
			if (chmenu) {
				GtkWidget *item;
				if (!menu) menu = (SPMenu *) sp_menu_new ();
				item = gtk_image_menu_item_new_with_label (name);
				gtk_widget_show (item);
				gtk_menu_item_set_submenu ((GtkMenuItem *) item, chmenu);
				gtk_menu_append ((GtkMenu *) menu, item);
			}
		} else if (!strcmp (sp_repr_name (repr), "module")) {
			unsigned int bval;
			if (!sp_repr_get_boolean (repr, "invalid", &bval) || !bval) {
				menu = callback (menu, repr, name);
			}
		}
	}
	return (GtkWidget *) menu;
}

GtkWidget *
sp_modules_menu_new (void)
{
	SPRepr *repr;
	GtkWidget *menu;
	menu = NULL;
	repr = sodipodi_get_repr (SODIPODI, "extensions");
	if (repr) {
		SPRepr *child;
		child = sp_repr_get_children (repr);
		while (child) {
			menu = sp_modules_menu_append_node ((SPMenu *) menu, child, sp_modules_menu_action_append);
			child = child->next;
		}
	}
	return (GtkWidget *) menu;
}

GtkWidget *
sp_modules_menu_about_new (void)
{
	SPRepr *repr;
	GtkWidget *menu;
	menu = NULL;
	repr = sodipodi_get_repr (SODIPODI, "extensions");
	if (repr) {
		SPRepr *child;
		child = sp_repr_get_children (repr);
		while (child) {
			menu = sp_modules_menu_append_node ((SPMenu *) menu, child, sp_modules_menu_about_append);
			child = child->next;
		}
	}
	return (GtkWidget *) menu;
}

void
sp_module_system_menu_open (SPMenu *menu)
{
	sp_menu_append (menu, _("Scalable Vector Graphic (SVG)"), _("Sodipodi native file format and W3C standard"), SP_MODULE_KEY_INPUT_SVG);
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


