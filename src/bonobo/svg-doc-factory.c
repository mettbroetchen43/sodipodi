#define SP_SVG_DOC_FACTORY_C

#include "embeddable-document.h"
#include "svg-doc-factory.h"

void
sp_svg_doc_factory_init (void)
{
	static BonoboEmbeddableFactory * doc_factory = NULL;

	if (doc_factory != NULL) return;

	doc_factory = bonobo_embeddable_factory_new (
		"embeddable-factory:sodipodi-svg-doc",
		sp_embeddable_document_factory, NULL);

	if (doc_factory == NULL)
		g_error (_("Could not create sodipodi-svg-doc factory"));
}

