#define __SODIPODI_C__

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>

#include <libarikkei/arikkei-token.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmessagedialog.h>

#include "monostd.h"

#include "helper/sp-intl.h"
#include "helper/sp-marshal.h"
#include "helper/action.h"
#include "xml/repr-private.h"

#include "api.h"
#include "system.h"
#include "verbs.h"
#include "shortcuts.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "event-context.h"
#include "sodipodi.h"
#include "sodipodi-private.h"

/* Backbones of configuration xml data */
#include "preferences-skeleton.h"
#include "extensions-skeleton.h"

enum {
	MODIFY_SELECTION,
	CHANGE_SELECTION,
	SET_SELECTION,
	SET_EVENTCONTEXT,
	NEW_DESKTOP,
	DESTROY_DESKTOP,
	ACTIVATE_DESKTOP,
	DESACTIVATE_DESKTOP,
	NEW_DOCUMENT,
	DESTROY_DOCUMENT,
	COLOR_SET,
	LAST_SIGNAL
};

#define SP_TYPE_SODIPODI (sodipodi_get_type ())
#define SP_SODIPODI(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_SODIPODI, Sodipodi))
#define SP_IS_SODIPODI(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_SODIPODI))

#define DESKTOP_IS_ACTIVE(d) ((d) && (sodipodi->desktops) && (d) == sodipodi->desktops->data)

static void sodipodi_class_init (SodipodiClass *klass);
static void sodipodi_init (SPObject *object);
static void sodipodi_finalize (GObject *object);
static void sodipodi_dispose (GObject *object);

static void sodipodi_activate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop);
static void sodipodi_desactivate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop);

static void sodipodi_init_config (SPReprDoc *doc, const gchar *config_name, const gchar *skeleton, int skel_size,
				  const unsigned char *e_mkdir,
				  const unsigned char *e_notdir,
				  const unsigned char *e_ccf,
				  const unsigned char *e_cwf);
static void sodipodi_init_preferences (Sodipodi *sodipodi);
static void sodipodi_init_extensions (Sodipodi *sodipodi);

void sodipodi_save_extensions (Sodipodi * sodipodi);

struct _Sodipodi {
	GObject object;
	SPReprDoc *preferences;
	SPReprDoc *extensions;
	GSList *documents;
	GSList *desktops;
	/* Last action */
	unsigned int action_verb;
	SPRepr *action_config;
	/* Current tool verb */
	unsigned int eventverb;
};

struct _SodipodiClass {
	GObjectClass object_class;

	/* Signals */
	void (* change_selection) (Sodipodi * sodipodi, SPSelection * selection);
	void (* modify_selection) (Sodipodi * sodipodi, SPSelection * selection, guint flags);
	void (* set_selection) (Sodipodi * sodipodi, SPSelection * selection);
	void (* set_eventcontext) (Sodipodi * sodipodi, SPEventContext * eventcontext);
	void (* new_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* destroy_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* activate_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* desactivate_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* new_document) (Sodipodi *sodipodi, SPDocument *doc);
	void (* destroy_document) (Sodipodi *sodipodi, SPDocument *doc);

	void (* color_set) (Sodipodi *sodipodi, SPColor *color, double opacity);
};

static GObjectClass * parent_class;
static guint sodipodi_signals[LAST_SIGNAL] = {0};

Sodipodi *sodipodi = NULL;

static GType
sodipodi_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SodipodiClass),
			NULL, NULL,
			(GClassInitFunc) sodipodi_class_init,
			NULL, NULL,
			sizeof (Sodipodi),
			4,
			(GInstanceInitFunc) sodipodi_init,
		};
		type = g_type_register_static (G_TYPE_OBJECT, "Sodipodi", &info, 0);
	}
	return type;
}

static void
sodipodi_class_init (SodipodiClass * klass)
{
	GObjectClass * object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);

	sodipodi_signals[MODIFY_SELECTION] = g_signal_new ("modify_selection",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, modify_selection),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER_UINT,
							   G_TYPE_NONE, 2,
							   G_TYPE_POINTER, G_TYPE_UINT);
	sodipodi_signals[CHANGE_SELECTION] = g_signal_new ("change_selection",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, change_selection),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[SET_SELECTION] =    g_signal_new ("set_selection",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, set_selection),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[SET_EVENTCONTEXT] = g_signal_new ("set_eventcontext",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, set_eventcontext),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[NEW_DESKTOP] =      g_signal_new ("new_desktop",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, new_desktop),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[DESTROY_DESKTOP] =  g_signal_new ("destroy_desktop",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, destroy_desktop),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[ACTIVATE_DESKTOP] = g_signal_new ("activate_desktop",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, activate_desktop),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[DESACTIVATE_DESKTOP] = g_signal_new ("desactivate_desktop",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, desactivate_desktop),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[NEW_DOCUMENT] =     g_signal_new ("new_document",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, new_document),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[DESTROY_DOCUMENT] = g_signal_new ("destroy_document",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, destroy_document),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER,
							   G_TYPE_NONE, 1,
							   G_TYPE_POINTER);
	sodipodi_signals[COLOR_SET] =        g_signal_new ("color_set",
							   G_TYPE_FROM_CLASS (klass),
							   G_SIGNAL_RUN_FIRST,
							   G_STRUCT_OFFSET (SodipodiClass, color_set),
							   NULL, NULL,
							   sp_marshal_NONE__POINTER_DOUBLE,
							   G_TYPE_NONE, 2,
							   G_TYPE_POINTER, G_TYPE_DOUBLE);

	object_class->finalize = sodipodi_finalize;
	object_class->dispose = sodipodi_dispose;

	klass->activate_desktop = sodipodi_activate_desktop_private;
	klass->desactivate_desktop = sodipodi_desactivate_desktop_private;
}

static void
sodipodi_init (SPObject * object)
{
	g_assert (!sodipodi);

	/* Set static object */
	sodipodi = (Sodipodi *) object;
	sodipodi->eventverb = SP_VERB_NONE;
	/* Initialize preferences to default tree */
	sodipodi->preferences = sp_repr_doc_new_from_mem (preferences_skeleton, PREFERENCES_SKELETON_SIZE, NULL);
	/* Initialize extensions to default tree */
	sodipodi->extensions = sp_repr_doc_new_from_mem (extensions_skeleton, EXTENSIONS_SKELETON_SIZE, NULL);

	/* Initialize shortcut table */
	sp_shortcut_table_load (NULL);
}

static void
sodipodi_finalize (GObject *object)
{
	g_assert (object == (GObject *) sodipodi);

	sodipodi = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);

	gtk_main_quit ();
}

/* fixme: We need to use ::dispose because document want to ref sodipodi */

static void
sodipodi_dispose (GObject *object)
{
	g_assert (object == (GObject *) sodipodi);
	g_assert (!sodipodi->desktops);

	/* fixme: This is uttermost crap (Lauris) */
	while (sodipodi->documents) {
		g_object_unref ((GObject *) sodipodi->documents->data);
		/* Only private documents can exist at this step */
		g_assert (sodipodi);
	}

	if (sodipodi->action_config) {
		sodipodi->action_config = sp_repr_unref (sodipodi->action_config);
	}

	if (sodipodi->extensions) {
		/* fixme: This is not the best place */
		sodipodi_save_extensions (sodipodi);
		sp_repr_doc_unref (sodipodi->extensions);
		sodipodi->extensions = NULL;
	}

	if (sodipodi->preferences) {
		/* fixme: This is not the best place */
		sodipodi_save_preferences (sodipodi);
		sp_repr_doc_unref (sodipodi->preferences);
		sodipodi->preferences = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
sodipodi_activate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop)
{
	sp_desktop_set_active (desktop, TRUE);
}

static void
sodipodi_desactivate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop)
{
	sp_desktop_set_active (desktop, FALSE);
}

Sodipodi *
sodipodi_application_new (void)
{
	Sodipodi *sp;

	sp = g_object_new (SP_TYPE_SODIPODI, NULL);
	/* fixme: load application defaults */

	return sp;
}

static void
sodipodi_document_destroy (SPDocument *doc, void *data)
{
	sodipodi_remove_document (doc);
	sodipodi_unref ();
}

SPDocument *
sodipodi_document_new (const unsigned char *uri, unsigned int advertize, unsigned int keepalive)
{
	SPDocument *doc;
	doc = sp_document_new (uri);
	if (doc) {
		sp_document_set_advertize (doc, TRUE);
		if (keepalive) {
			g_signal_connect ((GObject *) doc, "destroy", (GCallback) sodipodi_document_destroy, NULL);
			sodipodi_ref ();
		}
		sp_document_set_undo_sensitive (doc, TRUE);
		sodipodi_add_document (doc);
	}
	return doc;
}

SPDocument *
sodipodi_document_new_from_mem (const unsigned char *cdata, unsigned int length,
				unsigned int advertize, unsigned int keepalive)
{
	SPDocument *doc;
	doc = sp_document_new_from_mem (cdata, length);
	if (doc) {
		sp_document_set_advertize (doc, TRUE);
		if (keepalive) {
			g_signal_connect ((GObject *) doc, "destroy", (GCallback) sodipodi_document_destroy, NULL);
			sodipodi_ref ();
		}
		sp_document_set_undo_sensitive (doc, TRUE);
		sodipodi_add_document (doc);
	}
	return doc;
}


/* Preference management */
/* We use '.' as separator */

static void
sodipodi_load_config (const unsigned char *filename, SPReprDoc *config, const unsigned char *skeleton, unsigned int skel_size,
		      const unsigned char *e_notreg, const unsigned char *e_notxml, const unsigned char *e_notsp)
{
	unsigned char *fnutf8, *fnos;
	struct stat s;
	GtkWidget * w;
	SPReprDoc * doc;
	SPRepr * root;
	gsize bytesin, bytesout;

#ifdef WIN32
	fnutf8 = g_build_filename (SODIPODI_APPDATADIR, filename, NULL);
	fnos = g_filename_from_utf8 (fnutf8, strlen (fnutf8), &bytesin, &bytesout, NULL);
#else
	fnutf8 = g_build_filename (g_get_home_dir (), ".sodipodi", filename, NULL);
	fnos = g_filename_from_utf8 (fnutf8, strlen (fnutf8), &bytesin, &bytesout, NULL);
#endif
	if (stat (fnos, &s)) {
		/* No such file */
		/* fixme: Think out something (Lauris) */
		if (!strcmp (filename, "extensions")) {
			sodipodi_init_extensions (SODIPODI);
		} else {
			sodipodi_init_preferences (SODIPODI);
		}
		g_free (fnos);
		g_free (fnutf8);
		return;
	}

	if (!S_ISREG (s.st_mode)) {
		/* Not a regular file */
		w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					    e_notreg, fnutf8);
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
		g_free (fnos);
		g_free (fnutf8);
		return;
	}

	doc = sp_repr_doc_new_from_file (fnutf8, NULL);
	if (doc == NULL) {
		/* Not an valid xml file */
		w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					    e_notxml, fnutf8);
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
		g_free (fnos);
		g_free (fnutf8);
		return;
	}

	root = sp_repr_doc_get_root (doc);
	if (strcmp (sp_repr_name (root), "sodipodi")) {
		w = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					    e_notsp, fnutf8);
		gtk_dialog_run (GTK_DIALOG (w));
		gtk_widget_destroy (w);
		sp_repr_doc_unref (doc);
		g_free (fnos);
		g_free (fnutf8);
		return;
	}

	g_free (fnos);
	g_free (fnutf8);

	sp_repr_doc_merge (config, doc, "id");
	sp_repr_doc_unref (doc);
}

/* Preferences management */

void
sodipodi_load_preferences (Sodipodi *sodipodi)
{
	sodipodi_load_config ("preferences", sodipodi->preferences, preferences_skeleton, PREFERENCES_SKELETON_SIZE,
			      _("%s is not regular file.\n"
				"Although sodipodi will run, you can\n"
				"neither load nor save preferences\n"),
			      _("%s either is not valid xml file or\n"
				"you do not have read premissions on it.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"preferences."),
			      _("%s is not valid sodipodi preferences file.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"preferences."));
}

/* Extensions management */

void
sodipodi_load_extensions (Sodipodi *sodipodi)
{
	sodipodi_load_config ("extensions", sodipodi->extensions, extensions_skeleton, EXTENSIONS_SKELETON_SIZE,
			      _("%s is not regular file.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"),
			      _("%s either is not valid xml file or\n"
				"you do not have read premissions on it.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"),
			      _("%s is not valid sodipodi extensions file.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"));
}

void
sodipodi_save_preferences (Sodipodi * sodipodi)
{
	gchar * fn;

#ifdef WIN32
	fn = g_build_filename (SODIPODI_APPDATADIR, "preferences", NULL);
#else
	fn = g_build_filename (g_get_home_dir (), ".sodipodi/preferences", NULL);
#endif

	sp_repr_doc_write_file (sodipodi->preferences, fn);

	g_free (fn);
}

void
sodipodi_save_extensions (Sodipodi * sodipodi)
{
	gchar * fn;

#ifdef WIN32
	fn = g_build_filename (SODIPODI_APPDATADIR, "extensions", NULL);
#else
	fn = g_build_filename (g_get_home_dir (), ".sodipodi/extensions", NULL);
#endif

	sp_repr_doc_write_file (sodipodi->extensions, fn);

	g_free (fn);
}

void
sodipodi_selection_modified (SPSelection *selection, guint flags)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[MODIFY_SELECTION], 0, selection, flags);
	}
}

void
sodipodi_selection_changed (SPSelection * selection)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, selection);
	}
}

void
sodipodi_selection_set (SPSelection * selection)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], 0, selection);
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, selection);
	}
}

void
sodipodi_eventcontext_set (SPEventContext *eventcontext)
{
	SPAction *action;

	g_return_if_fail (sodipodi != NULL);

	action = sp_verb_get_action (sodipodi->eventverb);
	if (action) sp_action_set_active (action, FALSE);
	sodipodi->eventverb = (eventcontext) ? eventcontext->verb : SP_VERB_NONE;
	action = sp_verb_get_action (sodipodi->eventverb);
	if (action) sp_action_set_active (action, TRUE);

	if (eventcontext && DESKTOP_IS_ACTIVE (eventcontext->desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], 0, eventcontext);
	}
}

void
sodipodi_add_desktop (SPDesktop * desktop)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	g_assert (!g_slist_find (sodipodi->desktops, desktop));

	sodipodi->desktops = g_slist_append (sodipodi->desktops, desktop);

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[NEW_DESKTOP], 0, desktop);

	if (DESKTOP_IS_ACTIVE (desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], 0, desktop);
		/* Update actions and emit signal */
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (desktop));
		/* g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], 0, SP_DT_EVENTCONTEXT (desktop)); */
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], 0, SP_DT_SELECTION (desktop));
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, SP_DT_SELECTION (desktop));
	}
}

void
sodipodi_remove_desktop (SPDesktop * desktop)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	g_assert (g_slist_find (sodipodi->desktops, desktop));

	if (DESKTOP_IS_ACTIVE (desktop)) {
		g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[DESACTIVATE_DESKTOP], 0, desktop);
		if (sodipodi->desktops->next != NULL) {
			SPDesktop * new;
			new = (SPDesktop *) sodipodi->desktops->next->data;
			sodipodi->desktops = g_slist_remove (sodipodi->desktops, new);
			sodipodi->desktops = g_slist_prepend (sodipodi->desktops, new);
			g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], 0, new);
			/* Update actions and emit signal */
			sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (new));
			/* g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], 0, SP_DT_EVENTCONTEXT (new)); */
			g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], 0, SP_DT_SELECTION (new));
			g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, SP_DT_SELECTION (new));
		} else {
			/* Update actions and emit signal */
			sodipodi_eventcontext_set (NULL);
			/* g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], 0, NULL); */
			g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], 0, NULL);
			g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, NULL);
		}
	}

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[DESTROY_DESKTOP], 0, desktop);

	sodipodi->desktops = g_slist_remove (sodipodi->desktops, desktop);
}

void
sodipodi_activate_desktop (SPDesktop * desktop)
{
	SPDesktop * current;

	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (desktop != NULL);
	g_return_if_fail (SP_IS_DESKTOP (desktop));

	if (DESKTOP_IS_ACTIVE (desktop)) return;

	g_assert (g_slist_find (sodipodi->desktops, desktop));

	current = (SPDesktop *) sodipodi->desktops->data;

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[DESACTIVATE_DESKTOP], 0, current);

	sodipodi->desktops = g_slist_remove (sodipodi->desktops, desktop);
	sodipodi->desktops = g_slist_prepend (sodipodi->desktops, desktop);

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], 0, desktop);
	/* Update actions and emit signal */
	sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (desktop));
	/* g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], 0, SP_DT_EVENTCONTEXT (desktop)); */
	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], 0, SP_DT_SELECTION (desktop));
	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], 0, SP_DT_SELECTION (desktop));
}

/* fixme: These need probably signals too */

void
sodipodi_add_document (SPDocument *document)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	g_assert (!g_slist_find (sodipodi->documents, document));

	sodipodi->documents = g_slist_append (sodipodi->documents, document);

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[NEW_DOCUMENT], 0, document);
}

void
sodipodi_remove_document (SPDocument *document)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	g_assert (g_slist_find (sodipodi->documents, document));

	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[DESTROY_DOCUMENT], 0, document);

	sodipodi->documents = g_slist_remove (sodipodi->documents, document);

	if (document->advertize && SP_DOCUMENT_URI (document)) {
		SPRepr *recent;
		recent = sodipodi_get_repr (SODIPODI, "documents.recent");
		if (recent) {
			SPRepr *child;
			child = sp_repr_lookup_child (recent, "uri", SP_DOCUMENT_URI (document));
			if (child) {
				sp_repr_change_order (recent, child, NULL);
			} else {
				if (sp_repr_n_children (recent) >= 4) {
					child = recent->children->next->next;
					while (child->next) sp_repr_unparent (child->next);
				}
				child = sp_repr_new ("document");
				sp_repr_set_attr (child, "uri", SP_DOCUMENT_URI (document));
				sp_repr_add_child (recent, child, NULL);
			}
			sp_repr_set_attr (child, "name", SP_DOCUMENT_NAME (document));
		}
	}
}

void
sodipodi_set_color (SPColor *color, float opacity)
{
	g_signal_emit (G_OBJECT (sodipodi), sodipodi_signals[COLOR_SET], 0, color, (double) opacity);
}

SPDocument *
sodipodi_active_document (void)
{
	if (SP_ACTIVE_DESKTOP) return SP_DT_DOCUMENT (SP_ACTIVE_DESKTOP);

	return NULL;
}

SPEventContext *
sodipodi_active_event_context (void)
{
	if (SP_ACTIVE_DESKTOP) return SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP);

	return NULL;
}

/* Helpers */

static void
sodipodi_init_config (SPReprDoc *doc, const gchar *config_name, const gchar *skeleton, int skel_size,
		      const unsigned char *e_mkdir, const unsigned char *e_notdir, const unsigned char *e_ccf, const unsigned char *e_cwf)
{
	unsigned char *dnutf8, *dnos;
	unsigned char *fnutf8, *fnos;
	struct stat s;
	GtkWidget *mdlg;
	int fh;
	gsize bytesin, bytesout;
	unsigned int valid;

	valid = TRUE;
	mdlg = NULL;

#ifdef WIN32
	dnutf8 = g_strdup (SODIPODI_APPDATADIR);
#else
	dnutf8 = g_build_filename (g_get_home_dir (), ".sodipodi", NULL);
#endif
	dnos = g_filename_from_utf8 (dnutf8, strlen (dnutf8), &bytesin, &bytesout, NULL);
	if (stat (dnos, &s)) {
		if (mkdir (dnos, S_IRWXU | S_IRGRP | S_IXGRP)) {
			/* Cannot create directory */
			mdlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
						       e_mkdir, dnutf8);
			valid = FALSE;
		}
	} else if (!S_ISDIR (s.st_mode)) {
		/* Not a directory */
		mdlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					       e_notdir, dnutf8);
		valid = FALSE;
	}
	g_free (dnos);
	g_free (dnutf8);
	/* Return id error */
	if (mdlg) {
		gtk_dialog_run (GTK_DIALOG (mdlg));
		gtk_widget_destroy (mdlg);
		mdlg = NULL;
	}
	if (!valid) return;

#ifdef WIN32
	fnutf8 = g_build_filename (SODIPODI_APPDATADIR, config_name, NULL);
	fnos = g_filename_from_utf8 (fnutf8, strlen (fnutf8), &bytesin, &bytesout, NULL);
	fh = creat (fnos, S_IREAD | S_IWRITE);
#else
	fnutf8 = g_build_filename (g_get_home_dir (), ".sodipodi", config_name, NULL);
	fnos = g_filename_from_utf8 (fnutf8, strlen (fnutf8), &bytesin, &bytesout, NULL);
	fh = creat (fnos, S_IRUSR | S_IWUSR | S_IRGRP);
#endif
	if (fh < 0) {
		/* Cannot create file */
		mdlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					       e_ccf, fnutf8);
		valid = FALSE;
	}
	if (write (fh, skeleton, skel_size) != skel_size) {
		/* Cannot create file */
		mdlg = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					       e_cwf, fnutf8);
		valid = FALSE;
	}
	if (fh >= 0) close (fh);
	g_free (fnos);
	g_free (fnutf8);
	if (mdlg) {
		gtk_dialog_run (GTK_DIALOG (mdlg));
		gtk_widget_destroy (mdlg);
	}
}

/* This routine should be obsoleted in favor of the generic version */

static void
sodipodi_init_preferences (Sodipodi *sodipodi)
{
	sodipodi_init_config (sodipodi->preferences, "preferences", preferences_skeleton, PREFERENCES_SKELETON_SIZE,
			      _("Cannot create directory %s.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"%s."),
			      _("%s is not a valid directory.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"preferences."),
			      _("Cannot create file %s.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"preferences."),
			      _("Cannot write file %s.\n"
				"Although sodipodi will run, you\n"
				"are neither able to load nor save\n"
				"preferences."));
}

static void
sodipodi_init_extensions (Sodipodi *sodipodi)
{
	sodipodi_init_config (sodipodi->extensions, "extensions", extensions_skeleton, EXTENSIONS_SKELETON_SIZE,
			      _("Cannot create directory %s.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"),
			      _("%s is not a valid directory.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"),
			      _("Cannot create file %s.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"),
			      _("Cannot write file %s.\n"
				"Although sodipodi will run, you are\n"
				"not able to use extensions (plugins)\n"));
}

void
sodipodi_verb_perform (unsigned int verb, void *config)
{
	SPAction *action;
	action = sp_verb_get_action (verb);
	if (action) {
		sodipodi->action_verb = verb;
		if (config) sp_repr_ref (config);
		if (sodipodi->action_config) {
			sodipodi->action_config = sp_repr_unref (sodipodi->action_config);
		}
		sodipodi->action_config = config;
		sp_action_perform (action, config);
	}
}

void
sodipodi_verb_repeat (void)
{
	sodipodi_verb_perform (sodipodi->action_verb, sodipodi->action_config);
}

void
sodipodi_refresh_display (Sodipodi *sodipodi)
{
	GSList *l;

	for (l = sodipodi->desktops; l != NULL; l = l->next) {
		sp_view_request_redraw (SP_VIEW (l->data));
	}
}

unsigned int
sodipodi_shutdown_all_views (void)
{
	GSList *views;
	views = g_slist_copy (sodipodi->desktops);
	while (views) {
		if (sp_view_shutdown ((SPView *) views->data)) {
			g_slist_free (views);
			return FALSE;
		}
		views = g_slist_remove (views, views->data);
	}
	return TRUE;
}

void
sodipodi_exit (void)
{
	while (sodipodi && sodipodi->documents) g_object_unref ((GObject *) sodipodi->documents->data);
	while (sodipodi) sodipodi_unref ();
}

const GSList *
sodipodi_get_document_list (void)
{
	return sodipodi->documents;
}

SPDesktop *
sodipodi_get_active_desktop (void)
{
	if (sodipodi->desktops == NULL) return NULL;

	return (SPDesktop *) sodipodi->desktops->data;
}

/* Configuration management */

#if 0
/* We use '.' as separator */
SPRepr *
sodipodi_get_repr (Sodipodi *sodipodi, const unsigned char *key)
{
	SPRepr * repr;
	const gchar * id, * s, * e;
	gint len;

	if (key == NULL) return NULL;

	if (!strncmp (key, "extensions", 10) && (!key[10] || (key[10] == '.'))) {
		repr = sp_repr_doc_get_root (sodipodi->extensions);
	} else {
		repr = sp_repr_doc_get_root (sodipodi->preferences);
	}
	g_assert (!(strcmp (sp_repr_get_name (repr), "sodipodi")));

	s = key;
	while ((s) && (*s)) {
		SPRepr * child;
		/* Find next name */
		if ((e = strchr (s, '.'))) {
			len = e++ - s;
		} else {
			len = strlen (s);
		}
		for (child = repr->children; child != NULL; child = child->next) {
			id = sp_repr_attr (child, "id");
			if ((id) && (strlen (id) == len) && (!strncmp (id, s, len))) break;
		}
		if (child == NULL) return NULL;

		repr = child;
		s = e;
	}
	return repr;
}
#endif

#define SP_MAX_CONFIG_DEPTH 64
#define SP_MAX_CONFIG_ID_LENGTH 256

SPRepr *
sp_config_node_get (const unsigned char *path, unsigned int create)
{
	ArikkeiToken tokens[SP_MAX_CONFIG_DEPTH];
	ArikkeiToken ptoken;
	unsigned int ntokens;
	SPRepr *repr;
	unsigned int pos;

	if (!path) return NULL;
	if (!*path) return NULL;

	/* Tokenize path */
	arikkei_token_set_from_string (&ptoken, path);
	ntokens = arikkei_token_tokenize_ws (&ptoken, tokens, SP_MAX_CONFIG_DEPTH, ".", 0);
	assert (ntokens > 0);
	if (!arikkei_token_strcmp (&tokens[0], "extensions")) {
		repr = sp_repr_doc_get_root (sodipodi->extensions);
	} else {
		repr = sp_repr_doc_get_root (sodipodi->preferences);
	}
	assert (!(strcmp (sp_repr_get_name (repr), "sodipodi")));

	/* Parse all tokens */
	pos = 0;
	while (pos < ntokens) {
		unsigned char c[SP_MAX_CONFIG_ID_LENGTH];
		SPRepr *child;

		arikkei_token_strncpy (&tokens[pos], c, SP_MAX_CONFIG_ID_LENGTH);
		if (!c[0]) return NULL;
		child = sp_repr_lookup_child (repr, "id", c);
		if (!child && create) {
			child = sp_repr_new ("node");
			sp_repr_set_attr (child, "id", c);
			sp_repr_append_child (repr, child);
		}
		if (!child) return NULL;
		repr = child;
		pos += 1;
	}
	return repr;
}

const unsigned char *
sp_config_value_get (const unsigned char *path, const unsigned char *key, const unsigned char *defval)
{
	SPRepr *repr;
	const unsigned char *val;
	repr = sp_config_node_get (path, FALSE);
	if (!repr) return defval;
	val = sp_repr_get_attr (repr, key);
	return (val) ? val : defval;
}

unsigned int
sp_config_value_get_boolean (const unsigned char *path, const unsigned char *key, unsigned int defval)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, FALSE);
	if (!repr) return defval;
	sp_repr_get_boolean (repr, key, &defval);
	return defval;
}

int
sp_config_value_get_int (const unsigned char *path, const unsigned char *key, int defval)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, FALSE);
	if (!repr) return defval;
	sp_repr_get_int (repr, key, &defval);
	return defval;
}

double
sp_config_value_get_double (const unsigned char *path, const unsigned char *key, double defval)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, FALSE);
	if (!repr) return defval;
	sp_repr_get_double (repr, key, &defval);
	return defval;
}

unsigned int
sp_config_value_set (const unsigned char *path, const unsigned char *key, const unsigned char *val, unsigned int create)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, create);
	if (!repr) return FALSE;
	return sp_repr_set_attr (repr, key, val);
}

unsigned int
sp_config_value_set_boolean (const unsigned char *path, const unsigned char *key, unsigned int val, unsigned int create)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, create);
	if (!repr) return FALSE;
	return sp_repr_set_boolean (repr, key, val);
}

unsigned int
sp_config_value_set_int (const unsigned char *path, const unsigned char *key, int val, unsigned int create)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, create);
	if (!repr) return FALSE;
	return sp_repr_set_int (repr, key, val);
}

unsigned int
sp_config_value_set_double (const unsigned char *path, const unsigned char *key, double val, unsigned int create)
{
	SPRepr *repr;
	repr = sp_config_node_get (path, create);
	if (!repr) return FALSE;
	return sp_repr_set_double (repr, key, val);
}

