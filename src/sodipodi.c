#define SODIPODI_C

/*
 * Sodipodi
 *
 * This is interface to main application
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include "desktop-handles.h"
#include "sodipodi.h"
#include "sodipodi-private.h"

#include "dialogs/object-fill.h"
#include "dialogs/object-stroke.h"
#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"
#include "dialogs/align.h"
#include "dialogs/transformation.h"

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

static void fake_dialogs (void);

struct _Sodipodi {
	GtkObject object;
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
static Sodipodi * sodipodi = NULL;

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
	if (!sodipodi) {
		sodipodi = (Sodipodi *) object;
	} else {
		g_assert_not_reached ();
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

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);

	gtk_main_quit ();

	fake_dialogs ();
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

static void
fake_dialogs (void)
{
	sp_object_fill_dialog ();
	sp_object_stroke_dialog ();
	sp_text_edit_dialog ();
	sp_export_dialog ();
	sp_xml_tree_dialog ();
	sp_quick_align_dialog ();
	sp_transformation_dialog ();
}






