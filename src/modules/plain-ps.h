#ifndef __SP_PLAIN_PS_H__
#define __SP_PLAIN_PS_H__

/*
 * PostScript printing
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

typedef enum _SPPlainPSType SPPlainPSType;

enum _SPPlainPSType {
	SP_PLAIN_PS_DEFAULT,
	SP_PLAIN_PS_BITMAP
};

SPPrintPlainDriver *sp_plain_ps_driver_new (SPPlainPSType type, SPModulePrintPlain *module);

#endif
