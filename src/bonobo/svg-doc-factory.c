#define SP_SVG_DOC_FACTORY_C

#include "config.h"
#include "embeddable-document.h"
#include "svg-doc-factory.h"

void
sp_svg_doc_factory_init (void)
{
	static BonoboEmbeddableFactory * doc_factory = NULL;

	if (doc_factory != NULL) return;

#if USING_OAF
	doc_factory = bonobo_embeddable_factory_new (
		"OAFIID:embeddable-factory:sodipodi-svg-doc:4fda0c32-8996-4196-ac88-a79b5888e269",
		sp_embeddable_document_factory, NULL);
#else
	doc_factory = bonobo_embeddable_factory_new (
		"embeddable-factory:sodipodi-svg-doc",
		sp_embeddable_document_factory, NULL);
#endif

	if (doc_factory == NULL)
		g_error (_("Could not create sodipodi-svg-doc factory"));
}

