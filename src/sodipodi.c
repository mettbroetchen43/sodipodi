#define SODIPODI_C

/*
 * Sodipodi
 *
 * This is interface to main application
 *
 * Copyright (C) Lauris Kaplinski <lauris@kaplinski.com> 1999-2000
 *
 */

#include <string.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include "desktop-handles.h"
#include "sodipodi.h"
#include "sodipodi-private.h"

enum {
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

static SPRepr * sp_decode_key (Sodipodi * sodipodi, const gchar ** key);

struct _Sodipodi {
	GtkObject object;
	SPReprDoc * preferences;
	GSList * documents;
	GSList * desktops;
};

struct _SodipodiClass {
	GtkObjectClass object_class;

	/* Signals */

	void (* change_selection) (Sodipodi * sodipodi, SPSelection * selection);
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
}

static void
sodipodi_init (SPObject * object)
{
	gchar * filename;

	if (!sodipodi) {
		sodipodi = (Sodipodi *) object;
	} else {
		g_assert_not_reached ();
	}
	/* fixme: */
	filename = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi");
	sodipodi->preferences = sp_repr_read_file (filename);
	g_free (filename);
	if (!sodipodi->preferences) {
		sodipodi->preferences = sp_repr_document_new ("sodipodi");
	}
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
		sodipodi->documents = g_slist_remove_link (sodipodi->documents, sodipodi->documents);
	}

	g_assert (!sodipodi->documents);
	g_assert (!sodipodi->desktops);

	if (sodipodi->preferences) {
		gchar * filename;
		filename = g_concat_dir_and_file (g_get_home_dir (), ".sodipodi");
		sp_repr_save_file (sodipodi->preferences, filename);
		g_free (filename);
		sp_repr_document_unref (sodipodi->preferences);
		sodipodi->preferences = NULL;
	}

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);

	gtk_main_quit ();
}

Sodipodi *
sodipodi_application_new (void)
{
	return gtk_type_new (SP_TYPE_SODIPODI);
	/* fixme: load application defaults */
}

/* Preference management */
/* We use '.' as separator */

void
sodipodi_set_key (Sodipodi * sodipodi, const gchar * key, const gchar * value)
{
	SPRepr * repr;
	const gchar *attr;

	attr = key;

	repr = sp_decode_key (sodipodi, &attr);
	g_assert (attr);
	g_assert (repr);

	sp_repr_set_attr (repr, attr, value);
}

const gchar *
sodipodi_get_key (Sodipodi * sodipodi, const gchar * key)
{
	SPRepr * repr;
	const gchar *attr;

	attr = key;

	repr = sp_decode_key (sodipodi, &attr);
	g_assert (attr);
	g_assert (repr);

	return sp_repr_attr (repr, attr);
}

void
sodipodi_set_key_as_number (Sodipodi * sodipodi, const gchar * key, gdouble value)
{
	gchar c[32];

	g_snprintf (c, 32, "%g", value);

	sodipodi_set_key (sodipodi, key, c);
}

gdouble
sodipodi_get_key_as_number (Sodipodi * sodipodi, const gchar * key)
{
	const gchar * value;

	value = sodipodi_get_key (sodipodi, key);

	return (value) ? atof (value) : 0.0;
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
		gtk_signal_emit (GTK_OBJECT (sodipodi), sodipodi_signals[DESTROY_DESKTOP], desktop);
	}

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

static SPRepr *
sp_decode_key (Sodipodi * sodipodi, const gchar ** key)
{
	SPRepr * repr;
	const gchar * attr, * str, * k;

	repr = sp_repr_document_root (sodipodi->preferences);
	attr = NULL;
	k = str = *key;

	while (*str) {
		k = str;
		while ((*k) && (*k != '.')) k++;
		if (*k) {
			gint len;
			const GList * children, * l;
			len = MIN (k - str, 255);
			children = sp_repr_children (repr);
			for (l = children; l != NULL; l = l->next) {
				if (!strncmp (sp_repr_name ((SPRepr *) l->data), str, len)) break;
			}
			if (l != NULL) {
				repr = (SPRepr *) l->data;
			} else {
				SPRepr * newrepr;
				gchar c[256];
				strncpy (c, str, len);
				c[len] = '\0';
				newrepr = sp_repr_new (c);
				sp_repr_append_child (repr, newrepr);
				repr = newrepr;
			}
			k++;
		}
		attr = str;
		str = k;
	}
	g_assert (attr);

	*key = attr;

	return repr;
}





