#define SP_EMBEDDABLE_DOCUMENT_C

#include "../document.h"
#include "sodipodi-bonobo.h"
#include "embeddable-desktop.h"
#include "embeddable-document.h"

static void sp_embeddable_document_class_init (GtkObjectClass * klass);
static void sp_embeddable_document_init (GtkObject * object);

static void sp_embeddable_document_destroyed (SPEmbeddableDocument * document);

GtkType
sp_embeddable_document_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPEmbeddableDocument",
			sizeof (SPEmbeddableDocument),
			sizeof (SPEmbeddableDocumentClass),
			(GtkClassInitFunc) sp_embeddable_document_class_init,
			(GtkObjectInitFunc) sp_embeddable_document_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (BONOBO_EMBEDDABLE_TYPE, &info);
	}
	return type;
}

static void
sp_embeddable_document_class_init (GtkObjectClass * klass)
{
}

static void
sp_embeddable_document_init (GtkObject * object)
{
	SPEmbeddableDocument * document;

	document = (SPEmbeddableDocument *) object;

	document->document = NULL;
}

BonoboObject *
sp_embeddable_document_factory (BonoboEmbeddableFactory * this, gpointer data)
{
	SPEmbeddableDocument * document;
	Bonobo_Embeddable corba_document;

	document = gtk_type_new (SP_EMBEDDABLE_DOCUMENT_TYPE);

	corba_document = bonobo_embeddable_corba_object_create (
		BONOBO_OBJECT (document));
	if (corba_document == CORBA_OBJECT_NIL) {
		gtk_object_unref (GTK_OBJECT (document));
		return CORBA_OBJECT_NIL;
	}

	document->document = sp_document_new (NULL);

	bonobo_embeddable_construct_full (BONOBO_EMBEDDABLE (document),
		corba_document,
		sp_embeddable_desktop_factory, NULL,
		NULL /* Item factory */, NULL);

	sp_bonobo_objects++;

	gtk_signal_connect (GTK_OBJECT (document), "destroy",
		GTK_SIGNAL_FUNC (sp_embeddable_document_destroyed), NULL);

	return BONOBO_OBJECT (document);
}

static void
sp_embeddable_document_destroyed (SPEmbeddableDocument * document)
{
	if (--sp_bonobo_objects > 0) return;

	/* Should we destroy factory */

	gtk_main_quit ();
}
