#define __SP_MODULE_WIN32_C__

/*
 * Windows stuff
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <config.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>

#include "display/nr-arena-item.h"
#include "display/nr-arena.h"
#include "document.h"

#include "win32.h"

static void sp_module_print_win32_class_init (SPModulePrintWin32Class *klass);
static void sp_module_print_win32_init (SPModulePrintWin32 *pmod);
static void sp_module_print_win32_finalize (GObject *object);

static unsigned int sp_module_print_win32_setup (SPModulePrint *mod);
static unsigned int sp_module_print_win32_begin (SPModulePrint *mod, SPDocument *doc);
static unsigned int sp_module_print_win32_finish (SPModulePrint *mod);

static SPModulePrintClass *print_win32_parent_class;

unsigned int
sp_module_print_win32_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModulePrintWin32Class),
			NULL, NULL,
			(GClassInitFunc) sp_module_print_win32_class_init,
			NULL, NULL,
			sizeof (SPModulePrintWin32),
			16,
			(GInstanceInitFunc) sp_module_print_win32_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE_PRINT, "SPModulePrintWin32", &info, 0);
	}
	return type;
}

static void
sp_module_print_win32_class_init (SPModulePrintWin32Class *klass)
{
	GObjectClass *g_object_class;
	SPModulePrintClass *module_print_class;

	g_object_class = (GObjectClass *)klass;
	module_print_class = (SPModulePrintClass *) klass;

	print_win32_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_print_win32_finalize;

	module_print_class->setup = sp_module_print_win32_setup;
	module_print_class->begin = sp_module_print_win32_begin;
	module_print_class->finish = sp_module_print_win32_finish;

}

static void
sp_module_print_win32_init (SPModulePrintWin32 *pmod)
{
	/* Nothing here */
}

static void
sp_module_print_win32_finalize (GObject *object)
{
	SPModulePrintWin32 *w32mod;

	w32mod = (SPModulePrintWin32 *) object;
	
	DeleteDC (w32mod->hDC);

	G_OBJECT_CLASS (print_win32_parent_class)->finalize (object);
}

static unsigned int
sp_module_print_win32_setup (SPModulePrint *mod)
{
	SPModulePrintWin32 *w32mod;
	HRESULT res;
	PRINTDLG pd = {
		sizeof (PRINTDLG),
		NULL, /* hwndOwner */
		NULL, /* hDevMode */
		NULL, /* hDevNames */
		NULL, /* hDC */
		PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC, /* Flags */
		1, 1, 1, 1, /* nFromPage, nToPage, nMinPage, nMaxPage */
		1, /* nCoies */
		NULL, /* hInstance */
		0, /* lCustData */
		NULL, NULL, NULL, NULL, NULL, NULL
	};

	w32mod = (SPModulePrintWin32 *) mod;

	res = PrintDlg (&pd);
	if (!res) return FALSE;

	w32mod->hDC = pd.hDC;

	return TRUE;
}

static unsigned int
sp_module_print_win32_begin (SPModulePrint *mod, SPDocument *doc)
{
	SPModulePrintWin32 *w32mod;
	DOCINFO di = {
		sizeof (DOCINFO),
		NULL, /* lpszDocName */
		NULL, /* lpszOutput */
		NULL, /* lpszDatatype */
		DI_APPBANDING /* fwType */
	};
	int res;

	w32mod = (SPModulePrintWin32 *) mod;

	w32mod->width = sp_document_width (doc);
	w32mod->height = sp_document_height (doc);

	di.lpszDocName = SP_DOCUMENT_NAME (doc);

	res = StartDoc (w32mod->hDC, &di);

	res = StartPage (w32mod->hDC);

	return 0;
}

static unsigned int
sp_module_print_win32_finish (SPModulePrint *mod)
{
	SPModulePrintWin32 *w32mod;
	float dpiX, dpiY;
	float scaleh, scalew;
	int pWidth, pHeight;
	int width, height;
	float x0, y0, x1, y1;
	NRMatrixF affine;
	unsigned char *px;
	int sheight, row;
	BITMAPINFO bmInfo = {
		{
			sizeof (BITMAPINFOHEADER), // bV4Size 
			64, /* biWidth */
			-64, /* biHeight */
			1, /* biPlanes */
			32, /* biBitCount */
			BI_RGB, /* biCompression */
			0, /* biSizeImage */
			2835, /* biXPelsPerMeter */
			2835, /* biYPelsPerMeter */
			0, /* biClrUsed */
			0, /* biClrImportant */
		},
		{0, 0, 0, 0} /* bmiColors */
	};
	int res;

	w32mod = (SPModulePrintWin32 *) mod;

	x0 = 0.0F;
	y0 = 0.0F;
	x1 = w32mod->width;
	y1 = w32mod->height;

	dpiX = (float) GetDeviceCaps (w32mod->hDC, LOGPIXELSX);
	dpiY = (float) GetDeviceCaps (w32mod->hDC, LOGPIXELSY);
	pWidth = GetDeviceCaps (w32mod->hDC, HORZRES); 
	pHeight = GetDeviceCaps (w32mod->hDC, VERTRES); 

	scaleh = dpiX / 72.0;
	scalew = dpiY / 72.0;

	width = w32mod->width * scaleh;
	height = w32mod->height * scalew;
	width = MIN (width, pWidth);
	height = MIN (height, pHeight);

	/*
	 * 1) a[0] * x0 + a[2] * y1 + a[4] = 0.0
	 * 2) a[1] * x0 + a[3] * y1 + a[5] = 0.0
	 * 3) a[0] * x1 + a[2] * y1 + a[4] = width
	 * 4) a[1] * x0 + a[3] * y0 + a[5] = height
	 * 5) a[1] = 0.0;
	 * 6) a[2] = 0.0;
	 *
	 * (1,3) a[0] * x1 - a[0] * x0 = width
	 * a[0] = width / (x1 - x0)
	 * (2,4) a[3] * y0 - a[3] * y1 = height
	 * a[3] = height / (y0 - y1)
	 * (1) a[4] = -a[0] * x0
	 * (2) a[5] = -a[3] * y1
	 */

	affine.c[0] = width / ((x1 - x0) * 1.25);
	affine.c[1] = 0.0;
	affine.c[2] = 0.0;
	affine.c[3] = height / ((y1 - y0) * 1.25);
	affine.c[4] = -affine.c[0] * x0 * 1.25;
	affine.c[5] = -affine.c[3] * y0 * 1.25;

	nr_arena_item_set_transform (mod->root, &affine);

	if ((width < 256) || ((width * height) < 32768)) {
		px = nr_pixelstore_64K_new (FALSE, 0);
		sheight = 65536 / (4 * width);
	} else {
		px = nr_new (unsigned char, 4 * 64 * width);
		sheight = 64;
	}

	/* Printing goes here */
	for (row = 0; row < height; row += sheight) {
		NRPixBlock pb;
		NRRectL bbox;
		NRGC gc;
		int num_rows;

		num_rows = sheight;
		if ((row + num_rows) > height) num_rows = height - row;

		/* Set area of interest */
		bbox.x0 = 0;
		bbox.y0 = row;
		bbox.x1 = width;
		bbox.y1 = row + num_rows;
		/* Update to renderable state */
		nr_matrix_d_set_identity (&gc.transform);
		nr_arena_item_invoke_update (mod->root, &bbox, &gc, NR_ARENA_ITEM_STATE_ALL, NR_ARENA_ITEM_STATE_NONE);

		nr_pixblock_setup_extern (&pb, NR_PIXBLOCK_MODE_R8G8B8A8N, bbox.x0, bbox.y0, bbox.x1, bbox.y1, px, 4 * width, FALSE, FALSE);
		memset (px, 0xff, 4 * num_rows * width);

		/* Render */
		nr_arena_item_invoke_render (mod->root, &bbox, &pb, 0);

		/* Blitter goes here */
		bmInfo.bmiHeader.biWidth = width;
		bmInfo.bmiHeader.biHeight = -num_rows;

		res = SetDIBitsToDevice (w32mod->hDC,
						   0, 0, width, num_rows,
						   0, 0, 0, num_rows, px, &bmInfo, DIB_RGB_COLORS);

		/* Blitter ends here */

		nr_pixblock_release (&pb);
	}

	if ((width < 256) || ((width * height) < 32768)) {
		nr_pixelstore_64K_free (px);
	} else {
		nr_free (px);
	}

	res = EndPage (w32mod->hDC);
	res = EndDoc (w32mod->hDC);

	return 0;
}

