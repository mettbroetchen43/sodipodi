#define __SODIPODI_C__

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-stock.h>
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
	NEW_DOCUMENT,
	DESTROY_DOCUMENT,
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
	void (* new_document) (Sodipodi *sodipodi, SPDocument *doc);
	void (* destroy_document) (Sodipodi *sodipodi, SPDocument *doc);
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
		GTK_TYPE_NONE, 2,
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
	sodipodi_signals[NEW_DOCUMENT] = gtk_signal_new ("new_document",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, new_document),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	sodipodi_signals[DESTROY_DOCUMENT] = gtk_signal_new ("destroy_document",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SodipodiClass, destroy_document),
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

#define SP_INDENT 8

static void
sodipodi_segv_handler (int signum)
{
	static gint recursion = FALSE;
	GSList *savednames, *failednames, *l;
	gchar *home, *istr, *sstr, *fstr, *b;
	gint count, nllen, len, pos;
	time_t sptime;
	struct tm *sptm;
	char sptstr[256];
	GtkWidget *msgbox;

	/* Kill loops */
	if (recursion) abort ();
	recursion = TRUE;

	g_warning ("Emergency save activated");

	home = g_get_home_dir ();
	sptime = time (NULL);
	sptm = localtime (&sptime);
	strftime (sptstr, 256, "%Y_%m_%d_%H_%M_%S", sptm);

	count = 0;
	savednames = NULL;
	failednames = NULL;
	for (l = sodipodi->documents; l != NULL; l = l->next) {
		SPDocument *doc;
		SPRepr *repr;
		doc = (SPDocument *) l->data;
		repr = sp_document_repr_root (doc);
		if (sp_repr_attr (repr, "sodipodi:modified")) {
			const guchar *docname, *d0, *d;
			gchar n[64], c[1024];
			FILE *file;
#if 0
			docname = sp_repr_attr (repr, "sodipodi:docname");
			if (docname) {
				docname = g_basename (docname);
			}
#else
			docname = doc->name;
			if (docname) {
				/* fixme: Quick hack to remove emergency file suffix */
				d0 = strrchr (docname, '.');
				if (d0 && (d0 > docname)) {
					d0 = strrchr (d0 - 1, '.');
					if (d0 && (d0 > docname)) {
						d = d0;
						while (isdigit (*d) || (*d == '.') || (*d == '_')) d += 1;
						if (*d) {
							memcpy (n, docname, MIN (d0 - docname - 1, 64));
							n[64] = '\0';
							docname = n;
						}
					}
				}
			}
#endif
			if (!docname || !*docname) docname = "emergency";
			g_snprintf (c, 1024, "%s/.sodipodi/%.256s.%s.%d", home, docname, sptstr, count);
			file = fopen (c, "w");
			if (!file) {
				g_snprintf (c, 1024, "%s/sodipodi-%.256s.%s.%d", home, docname, sptstr, count);
				file = fopen (c, "w");
			}
			if (!file) {
				g_snprintf (c, 1024, "/tmp/sodipodi-%.256s.%s.%d", docname, sptstr, count);
				file = fopen (c, "w");
			}
			if (file) {
				sp_repr_save_stream (sp_repr_document (repr), file);
				savednames = g_slist_prepend (savednames, g_strdup (c));
				fclose (file);
			} else {
				docname = sp_repr_attr (repr, "sodipodi:docname");
				failednames = g_slist_prepend (failednames, (docname) ? g_strdup (docname) : g_strdup (_("Untitled document")));
			}
			count++;
		}
	}

	savednames = g_slist_reverse (savednames);
	failednames = g_slist_reverse (failednames);
	if (savednames) {
		fprintf (stderr, "\nEmergency save locations:\n");
		for (l = savednames; l != NULL; l = l->next) {
			fprintf (stderr, "  %s\n", (gchar *) l->data);
		}
	}
	if (failednames) {
		fprintf (stderr, "Failed to do emergency save for:\n");
		for (l = failednames; l != NULL; l = l->next) {
			fprintf (stderr, "  %s\n", (gchar *) l->data);
		}
	}

	g_warning ("Emergency save completed, now crashing...");

	/* Show nice dialog box */

	istr = N_("Sodipodi encountered an internal error and will close now.\n");
	sstr = N_("Automatic backups of unsaved documents were done to following locations:\n");
	fstr = N_("Automatic backup of following documents failed:\n");
	nllen = strlen ("\n");
	len = strlen (istr) + strlen (sstr) + strlen (fstr);
	for (l = savednames; l != NULL; l = l->next) {
		len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
	}
	for (l = failednames; l != NULL; l = l->next) {
		len = len + SP_INDENT + strlen ((gchar *) l->data) + nllen;
	}
	len += 1;
	b = g_new (gchar, len);
	pos = 0;
	len = strlen (istr);
	memcpy (b + pos, istr, len);
	pos += len;
	if (savednames) {
		len = strlen (sstr);
		memcpy (b + pos, sstr, len);
		pos += len;
		for (l = savednames; l != NULL; l = l->next) {
			memset (b + pos, ' ', SP_INDENT);
			pos += SP_INDENT;
			len = strlen ((gchar *) l->data);
			memcpy (b + pos, l->data, len);
			pos += len;
			memcpy (b + pos, "\n", nllen);
			pos += nllen;
		}
	}
	if (failednames) {
		len = strlen (fstr);
		memcpy (b + pos, fstr, len);
		pos += len;
		for (l = failednames; l != NULL; l = l->next) {
			memset (b + pos, ' ', SP_INDENT);
			pos += SP_INDENT;
			len = strlen ((gchar *) l->data);
			memcpy (b + pos, l->data, len);
			pos += len;
			memcpy (b + pos, "\n", nllen);
			pos += nllen;
		}
	}
	*(b + pos) = '\0';
	msgbox = gnome_message_box_new (b, GNOME_MESSAGE_BOX_ERROR, GNOME_STOCK_BUTTON_CLOSE, NULL);
	gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	gtk_widget_destroy (msgbox);
	g_free (b);

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

	sp_repr_document_merge (sodipodi->preferences, doc, "id");
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
sodipodi_selection_modified (SPSelection *selection, guint flags)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	if (DESKTOP_IS_ACTIVE (selection->desktop)) {
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[MODIFY_SELECTION], selection, flags);
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

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[NEW_DOCUMENT], document);
}

void
sodipodi_remove_document (SPDocument *document)
{
	g_return_if_fail (sodipodi != NULL);
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	g_assert (g_slist_find (sodipodi->documents, document));

	gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[DESTROY_DOCUMENT], document);

	sodipodi->documents = g_slist_remove (sodipodi->documents, document);

	if (document->public && SP_DOCUMENT_URI (document)) {
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
				/* fixme: */
				sp_repr_set_attr (child, "id", SP_DOCUMENT_NAME (document));
				sp_repr_set_attr (child, "uri", SP_DOCUMENT_URI (document));
				sp_repr_add_child (recent, child, NULL);
			}
			sp_repr_set_attr (child, "name", SP_DOCUMENT_NAME (document));
		}
	}
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
		sp_view_request_redraw (SP_VIEW (l->data));
	}
}

/*
 * fixme: This also
 */

void
sodipodi_exit (Sodipodi *sodipodi)
{
	/* fixme: fixme: fixme: This has to allow saving data */

	gtk_main_quit ();
}

