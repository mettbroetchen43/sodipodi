#define SODIPODI_C

/*
 * Sodipodi
 *
 * This is interface to main application
 *
 * Copyright (C) Lauris Kaplinski <lauris@kaplinski.com> 1999-2000
 *
 */

#include <config.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-messagebox.h>
#include <xml/repr-private.h>
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "event-context.h"
#include "sodipodi.h"
#include "sodipodi-private.h"
#include "preferences-skeleton.h"

enum {
	MODIFY_SELECTION,
	CHANGE_SELECTION,
	SET_SELECTION,
	SET_EVENTCONTEXT,
	NEW_DESKTOP,
	DESTROY_DESKTOP,
	ACTIVATE_DESKTOP,
	DESACTIVATE_DESKTOP,
	LAST_SIGNAL
};

#define DESKTOP_IS_ACTIVE(d) ((d) == sodipodi->desktops->data)

static void sodipodi_class_init (SodipodiClass * klass);
static void sodipodi_init (SPObject * object);
static void sodipodi_destroy (GtkObject * object);

static void sodipodi_activate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop);
static void sodipodi_desactivate_desktop_private (Sodipodi *sodipodi, SPDesktop *desktop);

static void sodipodi_init_preferences (Sodipodi * sodipodi);

struct _Sodipodi {
	GtkObject object;
	SPReprDoc *preferences;
	GSList *documents;
	GSList *desktops;
};

struct _SodipodiClass {
	GtkObjectClass object_class;

	/* Signals */

	void (* change_selection) (Sodipodi * sodipodi, SPSelection * selection);
	void (* modify_selection) (Sodipodi * sodipodi, SPSelection * selection, guint flags);
	void (* set_selection) (Sodipodi * sodipodi, SPSelection * selection);
	void (* set_eventcontext) (Sodipodi * sodipodi, SPEventContext * eventcontext);
	void (* new_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* destroy_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* activate_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
	void (* desactivate_desktop) (Sodipodi * sodipodi, SPDesktop * desktop);
};

static GtkObjectClass * parent_class;
static guint sodipodi_signals[LAST_SIGNAL] = {0};

Sodipodi * sodipodi = NULL;

static void (* segv_handler) (int) = NULL;

GtkType
sodipodi_get_type (void)
{
	static GtkType sodipodi_type = 0;
	if (!sodipodi_type) {
		GtkTypeInfo sodipodi_info = {
			"Sodipodi",
			sizeof (Sodipodi),
			sizeof (SodipodiClass),
			(GtkClassInitFunc) sodipodi_class_init,
			(GtkObjectInitFunc) sodipodi_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		sodipodi_type = gtk_type_unique (gtk_object_get_type (), &sodipodi_info);
	}
	return sodipodi_type;
}

static void
sodipodi_class_init (SodipodiClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	sodipodi_signals[MODIFY_SELECTION] = gtk_signal_new ("modify_selection",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, modify_selection),
		gtk_marshal_NONE__POINTER_UINT,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER, GTK_TYPE_UINT);
	sodipodi_signals[CHANGE_SELECTION] = gtk_signal_new ("change_selection",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, change_selection),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[SET_SELECTION] = gtk_signal_new ("set_selection",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, set_selection),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[SET_EVENTCONTEXT] = gtk_signal_new ("set_eventcontext",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, set_eventcontext),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[NEW_DESKTOP] = gtk_signal_new ("new_desktop",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, new_desktop),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[DESTROY_DESKTOP] = gtk_signal_new ("destroy_desktop",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, destroy_desktop),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[ACTIVATE_DESKTOP] = gtk_signal_new ("activate_desktop",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, activate_desktop),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[DESACTIVATE_DESKTOP] = gtk_signal_new ("desactivate_desktop",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, desactivate_desktop),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, sodipodi_signals, LAST_SIGNAL);

	object_class->destroy = sodipodi_destroy;

	klass->activate_desktop = sodipodi_activate_desktop_private;
	klass->desactivate_desktop = sodipodi_desactivate_desktop_private;
}

static void
sodipodi_init (SPObject * object)
{
	if (!sodipodi) {
		sodipodi = (Sodipodi *) object;
	} else {
		g_assert_not_reached ();
	}

	sodipodi->preferences = sp_repr_read_mem (preferences_skeleton, PREFERENCES_SKELETON_SIZE);

	sodipodi->documents = NULL;
	sodipodi->desktops = NULL;
}

static void
sodipodi_destroy (GtkObject * object)
{
	Sodipodi * sodipodi;

	sodipodi = (Sodipodi *) object;

	while (sodipodi->documents) {
		SPDocument * document;
		document = (SPDocument *) sodipodi->documents->data;
		gtk_object_destroy (GTK_OBJECT (document));
		sodipodi->documents = g_slist_remove (sodipodi->documents, sodipodi->documents->data);
	}

	g_assert (!sodipodi->documents);
	g_assert (!sodipodi->desktops);

	if (sodipodi->preferences) {
		sp_repr_document_unref (sodipodi->preferences);
		sodipodi->preferences = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);

	gtk_main_quit ();
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

/* fixme: This is EVIL, and belongs to main after all */

static void
sodipodi_segv_handler (int signum)
{
	static gint recursion = FALSE;
	GSList *l;
	gchar *home;
	gint count, date;

	if (recursion) abort ();
	recursion = TRUE;

	g_warning ("Emergency save activated");

	home = g_get_home_dir ();
	date = time (NULL);

	count = 0;
	for (l = sodipodi->documents; l != NULL; l = l->next) {
		SPDocument *doc;
		SPRepr *repr;
		doc = (SPDocument *) l->data;
		repr = sp_document_repr_root (doc);
		if (sp_repr_attr (repr, "sodipodi:modified")) {
			gchar *path;
			FILE *file;
			path = g_strdup_printf ("%s/.sodipodi/emergency-%d-%d.svg", home, date, count);
			file = fopen (path, "w");
			g_free (path);
			if (!file) {
				path = g_strdup_printf ("%s/sodipodi-emergency-%d-%d.svg", home, date, count);
				file = fopen (path, "w");
				g_free (path);
			}
			if (!file) {
				path = g_strdup_printf ("/tmp/sodipodi-emergency-%d-%d.svg", date, count);
				file = fopen (path, "w");
				g_free (path);
			}
			if (file) {
				sp_repr_save_stream (sp_repr_document (repr), file);
				fclose (file);
			}
			count++;
		}
	}

	g_warning ("Emergency save completed, now crashing...");

	(* segv_handler) (signum);
}

Sodipodi *
sodipodi_application_new (void)
{
	Sodipodi *sp;

	sp = gtk_type_new (SP_TYPE_SODIPODI);
	/* fixme: load application defaults */

	segv_handler = signal (SIGSEGV, sodipodi_segv_handler);
	signal (SIGFPE, sodipodi_segv_handler);
	signal (SIGILL, sodipodi_segv_handler);

	return sp;
}

/* Preference management */
/* We use '.' as separator */

void
sodipodi_load_preferences (Sodipodi * sodipodi)
{
	gchar * fn, * m;
	struct stat s;
	GtkWidget * w;
	SPReprDoc * doc;
	SPRepr * root;

	fn = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi/preferences");
	if (stat (fn, &s)) {
		/* No such file */
		sodipodi_init_preferences (sodipodi);
		g_free (fn);
		return;
	}

	if (!S_ISREG (s.st_mode)) {
		/* Not a regular file */
		m = g_strdup_printf (_("%s is not regular file.\n"
				       "Although sodipodi will run, you can\n"
				       "neither load nor save preferences\n"), fn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		g_free (fn);
		return;
	}

	doc = sp_repr_read_file (fn);
	if (doc == NULL) {
		/* Not an valid xml file */
		m = g_strdup_printf (_("%s either is not valid xml file or\n"
				       "you do not have read premissions on it.\n"
				       "Although sodipodi will run, you\n"
				       "are neither able to load nor save\n"
				       "preferences."), fn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		g_free (fn);
		return;
	}

	root = sp_repr_document_root (doc);
	if (strcmp (sp_repr_name (root), "sodipodi")) {
		m = g_strdup_printf (_("%s is not valid sodipodi preferences file.\n"
				       "Although sodipodi will run, you\n"
				       "are neither able to load nor save\n"
				       "preferences."), fn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		sp_repr_document_unref (doc);
		g_free (fn);
		return;
	}

	sp_repr_document_overwrite (sodipodi->preferences, doc, "id");
	sp_repr_document_unref (doc);
	g_free (fn);
}

void
sodipodi_save_preferences (Sodipodi * sodipodi)
{
	gchar * fn;

	fn = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi/preferences");

	sp_repr_save_file (sodipodi->preferences, fn);

	g_free (fn);
}

/* We use '.' as separator */
SPRepr *
sodipodi_get_repr (Sodipodi * sodipodi, const gchar * key)
{
	SPRepr * repr;
	const gchar * id, * s, * e;
	gint len;

	if (key == NULL) return NULL;

	repr = sp_repr_document_root (sodipodi->preferences);
	g_assert (!(strcmp (sp_repr_name (repr), "sodipodi")));

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

void
sodipodi_selection_modified (SPSelection *selection)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[MODIFY_SELECTION], selection,
				 SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG);
	}
}

void
sodipodi_selection_changed (SPSelection * selection)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], selection);
	}
}

void
sodipodi_selection_set (SPSelection * selection)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], selection);
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], selection);
	}
}

void
sodipodi_eventcontext_set (SPEventContext * eventcontext)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (eventcontext != NULL);
	g_return_if_fail (SP_IS_EVENT_CONTEXT (eventcontext));

	if (DESKTOP_IS_ACTIVE (eventcontext->desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], eventcontext);
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

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[NEW_DESKTOP], desktop);

	if (DESKTOP_IS_ACTIVE (desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], desktop);
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], SP_DT_EVENTCONTEXT (desktop));
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], SP_DT_SELECTION (desktop));
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], SP_DT_SELECTION (desktop));
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
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[DESACTIVATE_DESKTOP], desktop);
		if (sodipodi->desktops->next != NULL) {
			SPDesktop * new;
			new = (SPDesktop *) sodipodi->desktops->next->data;
			sodipodi->desktops = g_slist_remove (sodipodi->desktops, new);
			sodipodi->desktops = g_slist_prepend (sodipodi->desktops, new);
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], new);
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], SP_DT_EVENTCONTEXT (new));
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], SP_DT_SELECTION (new));
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], SP_DT_SELECTION (new));
		} else {
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], NULL);
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], NULL);
			gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], NULL);
		}
	}

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[DESTROY_DESKTOP], desktop);

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

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[DESACTIVATE_DESKTOP], current);

	sodipodi->desktops = g_slist_remove (sodipodi->desktops, desktop);
	sodipodi->desktops = g_slist_prepend (sodipodi->desktops, desktop);

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[ACTIVATE_DESKTOP], desktop);
	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_EVENTCONTEXT], SP_DT_EVENTCONTEXT (desktop));
	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[SET_SELECTION], SP_DT_SELECTION (desktop));
	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[CHANGE_SELECTION], SP_DT_SELECTION (desktop));
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
}

void
sodipodi_remove_document (SPDocument *document)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	g_assert (g_slist_find (sodipodi->documents, document));

	sodipodi->documents = g_slist_remove (sodipodi->documents, document);
}

SPDesktop *
sodipodi_active_desktop (void)
{
	if (sodipodi->desktops == NULL) return NULL;

	return (SPDesktop *) sodipodi->desktops->data;
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
sodipodi_init_preferences (Sodipodi * sodipodi)
{
	gchar * dn, *fn;
	struct stat s;
	int fh;
	gchar * m;
	GtkWidget * w;

	dn = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi");
	if (stat (dn, &s)) {
		if (mkdir (dn, S_IRWXU | S_IRGRP | S_IXGRP)) {
			/* Cannot create directory */
			m = g_strdup_printf (_("Cannot create directory %s.\n"
					       "Although sodipodi will run, you\n"
					       "are neither able to load nor save\n"
					       "preferences."), dn);
			w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
			g_free (m);
			gnome_dialog_run_and_close (GNOME_DIALOG (w));
			g_free (dn);
			return;
		}
	} else if (!S_ISDIR (s.st_mode)) {
		/* Not a directory */
		m = g_strdup_printf (_("%s is not a valid directory.\n"
				       "Although sodipodi will run, you\n"
				       "are neither able to load nor save\n"
				       "preferences."), dn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		g_free (dn);
		return;
	}
	g_free (dn);

	fn = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi/preferences");
	fh = creat (fn, S_IRUSR | S_IWUSR | S_IRGRP);
	if (fh < 0) {
		/* Cannot create file */
		m = g_strdup_printf (_("Cannot create file %s.\n"
				       "Although sodipodi will run, you\n"
				       "are neither able to load nor save\n"
				       "preferences."), dn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		g_free (fn);
		return;
	}
	if (write (fh, preferences_skeleton, PREFERENCES_SKELETON_SIZE) != PREFERENCES_SKELETON_SIZE) {
		/* Cannot create file */
		m = g_strdup_printf (_("Cannot write file %s.\n"
				       "Although sodipodi will run, you\n"
				       "are neither able to load nor save\n"
				       "preferences."), dn);
		w = gnome_message_box_new (m, GNOME_MESSAGE_BOX_WARNING, "OK", NULL);
		g_free (m);
		gnome_dialog_run_and_close (GNOME_DIALOG (w));
		g_free (fn);
		close (fh);
		return;
	}
	close (fh);
}

void
sodipodi_refresh_display (Sodipodi *sodipodi)
{
	GSList *l;

	for (l = sodipodi->desktops; l != NULL; l = l->next) {
		SPDesktop *desktop;
		desktop = SP_DESKTOP (l->data);
		gtk_widget_queue_draw (GTK_WIDGET (desktop->owner));
	}
}





