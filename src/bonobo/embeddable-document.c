#define SP_EMBEDDABLE_DOCUMENT_C

#include <config.h>
#include "../sp-object.h"
#include "../sp-item.h"
#include "../sodipodi.h"
#include "../document.h"
#include "sodipodi-bonobo.h"
#include "embeddable-desktop.h"
#include "embeddable-drawing.h"
#include "embeddable-document.h"

#define STREAM_CHUNK_SIZE 4096

static void sp_embeddable_document_class_init (GtkObjectClass * klass);
static void sp_embeddable_document_init (GtkObject * object);

#if 0
static void sp_embeddable_document_destroyed (SPEmbeddableDocument * document);
#endif

static int sp_embeddable_document_pf_load (BonoboPersistFile * pfile, const CORBA_char * filename, gpointer closure);
static int sp_embeddable_document_pf_save (BonoboPersistFile * pfile, const CORBA_char * filename, gpointer closure);
static int sp_embeddable_document_ps_load (BonoboPersistStream * ps, Bonobo_Stream stream, gpointer closure);
static int sp_embeddable_document_ps_save (BonoboPersistStream * ps, Bonobo_Stream stream, gpointer closure);
static void sp_embeddable_document_print (GnomePrintContext * ctx, gdouble width, gdouble height, const Bonobo_PrintScissor * scissor, gpointer data);

static gint sp_bonobo_stream_read (Bonobo_Stream stream, gchar ** buffer);

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
	BonoboPersistFile * pfile;
	BonoboPersistStream * pstream;
#if 0
	BonoboPrint * print;
#endif

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
		sp_embeddable_drawing_factory, NULL);

	/* Add interfaces */

	pfile = bonobo_persist_file_new (sp_embeddable_document_pf_load,
		sp_embeddable_document_pf_save, document);

	if (pfile == NULL) {
		gtk_object_unref (GTK_OBJECT (document));
		return CORBA_OBJECT_NIL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (document), BONOBO_OBJECT (pfile));

	pstream = bonobo_persist_stream_new (sp_embeddable_document_ps_load,
		sp_embeddable_document_ps_save, document);

	if (pstream == NULL) {
		gtk_object_unref (GTK_OBJECT (document));
		return CORBA_OBJECT_NIL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (document), BONOBO_OBJECT (pstream));
#if 0
	print = bonobo_print_new (sp_embeddable_document_print, document);

	if (print == NULL) {
		gtk_object_unref (GTK_OBJECT (document));
		return CORBA_OBJECT_NIL;
	}

	bonobo_object_add_interface (BONOBO_OBJECT (document), BONOBO_OBJECT (print));
#endif

#if 0
#if 0
	sp_bonobo_objects++;
#else
	gnome_mdi_register (SODIPODI, GTK_OBJECT (document));
#endif
	gtk_signal_connect (GTK_OBJECT (document), "destroy",
		GTK_SIGNAL_FUNC (sp_embeddable_document_destroyed), NULL);
#endif

	return BONOBO_OBJECT (document);
}

#if 0
static void
sp_embeddable_document_destroyed (SPEmbeddableDocument * document)
{
#if 0
	if (--sp_bonobo_objects > 0) return;

	/* Should we destroy factory */

	gtk_main_quit ();
#else
	gnome_mdi_unregister (SODIPODI, GTK_OBJECT (document));
#endif
}
#endif

static int
sp_embeddable_document_pf_load (BonoboPersistFile * pfile, const CORBA_char * filename, gpointer closure)
{
	SPEmbeddableDocument * document;
	SPDocument * newdocument;

	document = SP_EMBEDDABLE_DOCUMENT (closure);

	newdocument = sp_document_new (filename);

	if (newdocument == NULL) return -1;

	gtk_object_unref (GTK_OBJECT (document->document));
	document->document = newdocument;

	bonobo_embeddable_foreach_view (BONOBO_EMBEDDABLE (document),
		sp_embeddable_desktop_new_doc, NULL);

	bonobo_embeddable_foreach_item (BONOBO_EMBEDDABLE (document),
		sp_embeddable_drawing_new_doc, NULL);

	return 0;
}

static int
sp_embeddable_document_pf_save (BonoboPersistFile * pfile, const CORBA_char * filename, gpointer closure)
{
	SPEmbeddableDocument * document;

	document = SP_EMBEDDABLE_DOCUMENT (closure);

	/* fixme */

	sp_repr_save_file (SP_OBJECT (document->document->root)->repr, filename);

	return 0;
}

static int
sp_embeddable_document_ps_load (BonoboPersistStream * ps, Bonobo_Stream stream, gpointer closure)
{
	SPEmbeddableDocument * document;
	SPDocument * newdocument;
	gchar * buffer;
	gint len;

	document = SP_EMBEDDABLE_DOCUMENT (closure);

	len = sp_bonobo_stream_read (stream, &buffer);
	if (len < 0) return -1;

	newdocument = sp_document_new_from_mem (buffer, len);

	g_free (buffer);

	if (newdocument == NULL) return -1;

	gtk_object_unref (GTK_OBJECT (document->document));
	document->document = newdocument;

	bonobo_embeddable_foreach_view (BONOBO_EMBEDDABLE (document),
		sp_embeddable_desktop_new_doc, NULL);

	bonobo_embeddable_foreach_item (BONOBO_EMBEDDABLE (document),
		sp_embeddable_drawing_new_doc, NULL);

	return 0;
}

static int
sp_embeddable_document_ps_save (BonoboPersistStream * ps, Bonobo_Stream stream, gpointer closure)
{
	g_warning ("embeddable_document_ps_save unimplemented");
	return 0;
}

static void
sp_embeddable_document_print (GnomePrintContext * ctx,
	gdouble width, gdouble height,
	const Bonobo_PrintScissor * scissor,
	gpointer data)
{
	SPEmbeddableDocument * document;

	document = SP_EMBEDDABLE_DOCUMENT (data);

	sp_item_print (SP_ITEM (document->document), ctx);
}

static gint
sp_bonobo_stream_read (Bonobo_Stream stream, gchar ** buffer)
{
	Bonobo_Stream_iobuf * iobuf;
	CORBA_long bytes_read;
	CORBA_Environment ev;
	gint len;

	CORBA_exception_init (&ev);

	* buffer = NULL;
	len = 0;

	do {
		bytes_read = Bonobo_Stream_read (stream, STREAM_CHUNK_SIZE, &iobuf, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			if (* buffer != NULL) g_free (* buffer);
			len = -1;
			break;
		}

		* buffer = g_realloc (* buffer, len + iobuf->_length);
		memcpy (* buffer + len, iobuf->_buffer, iobuf->_length);

		len += iobuf->_length;

	} while (bytes_read > 0);

	CORBA_exception_free (&ev);

	return len;
}

