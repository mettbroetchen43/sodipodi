#ifndef __SP_PLAIN_PDF_H__
#define __SP_PLAIN_PDF_H__

/*
 * PDF printing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *
 * This code is in public domain
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plain.h"

typedef enum _SPPlainPDFType SPPlainPDFType;

enum _SPPlainPDFType {
	SP_PLAIN_PDF_DEFAULT,
	SP_PLAIN_PDF_BITMAP
};

SPPrintPlainDriver *sp_plain_pdf_driver_new (SPPlainPDFType type, SPModulePrintPlain *module);

#endif
