#define SP_EMBEDDABLE_DESKTOP_C

#include "../desktop.h"
#include "embeddable-desktop.h"

static void sp_embeddable_desktop_class_init (GtkObjectClass * klass);
static void sp_embeddable_desktop_init (GtkObject * object);

static void sp_embeddable_desktop_activate (BonoboView * view, gboolean activate, gpointer data);

GtkType
sp_embeddable_desktop_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPEmbeddableDesktop",
			sizeof (SPEmbeddableDesktop),
			sizeof (SPEmbeddableDesktopClass),
			(GtkClassInitFunc) sp_embeddable_desktop_class_init,
			(GtkObjectInitFunc) sp_embeddable_desktop_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (BONOBO_VIEW_TYPE, &info);
	}
	return type;
}

static void
sp_embeddable_desktop_class_init (GtkObjectClass * klass)
{
}

static void
sp_embeddable_desktop_init (GtkObject * object)
{
	SPEmbeddableDesktop * desktop;

	desktop = (SPEmbeddableDesktop *) object;

	desktop->document = NULL;
	desktop->desktop = NULL;
}

BonoboView *
sp_embeddable_desktop_factory (BonoboEmbeddable * embeddable,
	const Bonobo_ViewFrame view_frame, gpointer data)
{
	SPEmbeddableDesktop * desktop;
	Bonobo_Embeddable corba_desktop;

	desktop = gtk_type_new (SP_EMBEDDABLE_DESKTOP_TYPE);

	corba_desktop = bonobo_view_corba_object_create (BONOBO_OBJECT (desktop));

	if (corba_desktop == CORBA_OBJECT_NIL) {
		gtk_object_unref (GTK_OBJECT (desktop));
		return CORBA_OBJECT_NIL;
	}

	desktop->document = SP_EMBEDDABLE_DOCUMENT (embeddable);
	desktop->desktop = sp_desktop_new (desktop->document->document);

	bonobo_view_construct (BONOBO_VIEW (desktop),
		corba_desktop,
		GTK_WIDGET (desktop->desktop));

	gtk_widget_show_all (GTK_WIDGET (desktop->desktop));

	gtk_signal_connect (GTK_OBJECT (desktop), "activate",
		GTK_SIGNAL_FUNC (sp_embeddable_desktop_activate), NULL);

	return BONOBO_VIEW (desktop);
}

static void
sp_embeddable_desktop_activate (BonoboView * view, gboolean activate, gpointer data)
{
	bonobo_view_activate_notify (view, activate);
}

