#ifndef __SP_PLAIN_H__
#define __SP_PLAIN_H__

/*
 * Plain printing subsystem
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

#define SP_TYPE_MODULE_PRINT_PLAIN (sp_module_print_plain_get_type())
#define SP_MODULE_PRINT_PLAIN(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_PRINT_PLAIN, SPModulePrintPlain))
#define SP_IS_MODULE_PRINT_PLAIN(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_PRINT_PLAIN))

typedef struct _SPModulePrintPlain SPModulePrintPlain;
typedef struct _SPModulePrintPlainClass SPModulePrintPlainClass;

typedef struct _SPPrintPlainDriver SPPrintPlainDriver;
typedef enum   _SPPrintPlainDriverType SPPrintPlainDriverType;

#include <module.h>

struct _SPModulePrintPlain {
	SPModulePrint module;
	unsigned int dpi : 15;
	float width;
	float height;
	FILE *stream;
	unsigned int driver_type;
	SPPrintPlainDriver *driver;
};

struct _SPModulePrintPlainClass {
	SPModulePrintClass module_print_class;
};

GType sp_module_print_plain_get_type (void);


struct _SPPrintPlainDriver {
	void (*initialize) (SPPrintPlainDriver *driver);
	void (*finalize) (SPPrintPlainDriver *driver);

	unsigned int (*begin) (SPPrintPlainDriver *driver, SPDocument *doc);
	unsigned int (*finish) (SPPrintPlainDriver *driver);

	unsigned int (* bind) (SPPrintPlainDriver *driver, const NRMatrixF *transform, float opacity);
	unsigned int (* release) (SPPrintPlainDriver *driver);
	unsigned int (* fill) (SPPrintPlainDriver *driver, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			       const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
	unsigned int (* stroke) (SPPrintPlainDriver *driver, const NRBPath *bpath, const NRMatrixF *transform, const SPStyle *style,
				 const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox);
	unsigned int (* image) (SPPrintPlainDriver *driver, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
				const NRMatrixF *transform, const SPStyle *style);
};

enum _SPPrintPlainDriverType {
	SP_PRINT_PLAIN_DRIVER_PS_DEFAULT,
	SP_PRINT_PLAIN_DRIVER_PS_BITMAP,
	SP_PRINT_PLAIN_DRIVER_PDF_DEFAULT,
	SP_PRINT_PLAIN_DRIVER_PDF_BITMAP
};

SPPrintPlainDriver *sp_print_plain_driver_new (SPPrintPlainDriverType type, SPModulePrintPlain *module);
SPPrintPlainDriver *sp_print_plain_driver_delete (SPPrintPlainDriver *driver);

#endif
