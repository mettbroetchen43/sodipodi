#define __SP_PLAIN_PDF_C__

/*
 * PDF printing
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *
 * Basic printing code, EXCEPT image and
 * ascii85 filter is in public domain
 *
 * Image printing and Ascii85 filter:
 *
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997-98 Peter Kirchgessner
 * George White <aa056@chebucto.ns.ca>
 * Austin Donnelly <austin@gimp.org>
 *
 * Licensed under GNU GPL
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <libarikkei/arikkei-strlib.h>

#include <libnr/nr-macros.h>
#include <libnr/nr-matrix.h>

#include <glib.h>

#include "helper/sp-intl.h"
#include "display/nr-arena-item.h"
#include "enums.h"
#include "document.h"
#include "style.h"

#include "plain-pdf.h"

#define PDF_EOL "\r\n"
#define PDF_BUFFER_INITIAL_SIZE 1024

typedef struct _SPPrintPlainPDFDriver SPPrintPlainPDFDriver;
typedef struct _SPPDFObject SPPDFObject;
typedef struct _SPPDFBuffer SPPDFBuffer;

struct _SPPrintPlainPDFDriver {
	SPPrintPlainDriver driver;
	SPModulePrintPlain *module;
	unsigned int type;
	/* PDF internal state */
	SPPDFBuffer *stream;
	SPPDFObject *objects;
	int n_objects;
	int offset;
	float stroke_rgb[3];
	float fill_rgb[3];
	unsigned int color_set_stroke : 1;
	unsigned int color_set_fill : 1;
	/* Objects Ids */
	int page_id;				/* single page support */
	int contents_id;
	int resources_id;
};

enum {
	PDFO_DICT_NONE = 0,
	PDFO_DICT_START,
	PDFO_DICT_END
};

enum {
	PDFO_STREAM_NONE = 0,
	PDFO_STREAM_START,
	PDFO_STREAM_END
};

struct _SPPDFObject {
	int id;
	int offset;
	unsigned int state_dict : 2;
	unsigned int state_stream : 2;
	SPPDFObject *next;
};

struct _SPPDFBuffer {
	unsigned char *buffer;
	int length;
	int alloc_size;
};


static void sp_plain_pdf_print_bpath (SPPrintPlainPDFDriver *pdf, const ArtBpath *bp);
static unsigned int sp_pdf_print_image (FILE *ofp, unsigned char *px, unsigned int width, unsigned int height, unsigned int rs, const NRMatrixF *transform);

static int sp_plain_pdf_fprintf (SPPrintPlainPDFDriver *pdf, const char *format, ...);
static int sp_plain_pdf_fwrite (SPPrintPlainPDFDriver *pdf, const char *buffer, int len);
static int sp_plain_pdf_fprint_double (SPPrintPlainPDFDriver *pdf, double value);

static int sp_plain_pdf_stream_fprintf (SPPrintPlainPDFDriver *pdf, const char *format, ...);
static int sp_plain_pdf_stream_fprint_double (SPPrintPlainPDFDriver *pdf, double value);

static int sp_plain_pdf_write_contents (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_write_resources (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_write_page (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_write_pages (SPPrintPlainPDFDriver *pdf, int catalog_id);

static int sp_plain_pdf_object_new (SPPrintPlainPDFDriver *pdf);
static void sp_plain_pdf_object_start (SPPrintPlainPDFDriver *pdf, int object_id);
static void sp_plain_pdf_object_start_stream (SPPrintPlainPDFDriver *pdf, int object_id);
static void sp_plain_pdf_object_end (SPPrintPlainPDFDriver *pdf, int object_id);
SPPDFObject *sp_plain_pdf_objects_reverse (SPPDFObject *objects);

static SPPDFBuffer *sp_pdf_buffer_new (void);
static SPPDFBuffer *sp_pdf_buffer_delete (SPPDFBuffer *buffer);
static int sp_pdf_buffer_write (SPPDFBuffer *buffer, unsigned char *text, int length);

static void
sp_plain_pdf_initialize (SPPrintPlainDriver *driver)
{
	SPPrintPlainPDFDriver *pdf;

	printf ("pdf_initialize\n");
	pdf = (SPPrintPlainPDFDriver *)driver;

	pdf->stream = sp_pdf_buffer_new ();
	pdf->objects = NULL;
	pdf->n_objects = 0;

	pdf->offset = 0;
	pdf->color_set_stroke = FALSE;
	pdf->color_set_fill = FALSE;

	pdf->contents_id = 0;
	pdf->resources_id = 0;
}

static void
sp_plain_pdf_finalize (SPPrintPlainDriver *driver)
{
	SPPrintPlainPDFDriver *pdf;
	SPPDFObject *object;

	pdf = (SPPrintPlainPDFDriver *)driver;

	sp_pdf_buffer_delete (pdf->stream);

	for (object = pdf->objects; object; ) {
		SPPDFObject *next;
		next = object->next;
		free (object);
		object = next;
	}
}

static unsigned int
sp_plain_pdf_begin (SPPrintPlainDriver *driver, SPDocument *doc)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;
	int res;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	res = sp_plain_pdf_fprintf (pdf, "%%PDF-1.3" PDF_EOL);
	if (res >= 0) res = sp_plain_pdf_fprintf (pdf, "%%\xb5\xed\xae\xfe" PDF_EOL);
	/* flush this to test output stream as early as possible */
	if (fflush (pmod->stream)) {
	/* g_print("caught error in sp_plain_pdf_begin\n");*/
		if (ferror (pmod->stream)) {
			g_print("Error %d on output stream: %s\n", errno,
				g_strerror(errno));
		}
		g_warning ("Printing failed\n");
		/* fixme: should use pclose() for pipes */
		fclose (pmod->stream);
		pmod->stream = NULL;
		fflush (stdout);
		return 0;
	}
	pmod->width = sp_document_width (doc);
	pmod->height = sp_document_height (doc);

	sp_plain_pdf_stream_fprintf (pdf, "0.8 0 0 -0.8 0.0 ");
	sp_plain_pdf_stream_fprint_double (pdf, pmod->height);
	res = sp_plain_pdf_stream_fprintf (pdf, " cm" PDF_EOL);

	pdf->page_id = sp_plain_pdf_object_new (pdf);
	pdf->contents_id = sp_plain_pdf_object_new (pdf);
	pdf->resources_id = sp_plain_pdf_object_new (pdf);

	return res;
}

static unsigned int
sp_plain_pdf_finish (SPPrintPlainDriver *driver)
{
	SPModulePrint *mod;
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;
	SPPDFObject *object;
	int res;
	int xref_offset, n_objects;
	int catalog_id;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;
	mod = (SPModulePrint*)pmod;

	if (!pmod->stream) return 0;

	/* Write single page contents */
	sp_plain_pdf_write_page (pdf);

	/* Write pages */
	catalog_id = sp_plain_pdf_object_new (pdf);
	sp_plain_pdf_write_pages (pdf, catalog_id);

	/* Write info block */

	/* Write xref */
	xref_offset = pdf->offset;
	n_objects = pdf->n_objects + 1;
	sp_plain_pdf_fprintf (pdf, "xref" PDF_EOL
						  "0 %d" PDF_EOL
						  "%010d %05d f" PDF_EOL,
						  n_objects,
						  0, 65535);

	pdf->objects = sp_plain_pdf_objects_reverse (pdf->objects);

	for (object = pdf->objects; object; object = object->next) {
		if (object->offset < 1) g_warning ("Object with zero offset!");
		
		sp_plain_pdf_fprintf (pdf, "%010d %05d n" PDF_EOL, object->offset, 0);
	}

	/* Write trailer */
	sp_plain_pdf_fprintf (pdf, "trailer" PDF_EOL
						  "<<" PDF_EOL
						  "/Size %d" PDF_EOL
						  "/Root %d 0 R" PDF_EOL
						  ">>" PDF_EOL
						  "startxref" PDF_EOL
						  "%d" PDF_EOL
						  "%%%%EOF" PDF_EOL,
						  n_objects, catalog_id,
						  xref_offset);
						  
	/* Flush stream to be sure */
	(void) fflush (pmod->stream);

	/* fixme: should really use pclose for popen'd streams */
	fclose (pmod->stream);
	pmod->stream = 0;

	return res;
}

static unsigned int
sp_plain_pdf_bind (SPPrintPlainDriver *driver, const NRMatrixF *transform, float opacity)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;
	unsigned int i;
	unsigned int res;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	if (!pmod->stream) return -1;

	sp_plain_pdf_stream_fprintf (pdf, "q" PDF_EOL);

	for (i = 0; i < 6; i++) {
		sp_plain_pdf_stream_fprint_double (pdf, transform->c[i]);
		sp_plain_pdf_stream_fprintf (pdf, " ");
	}
	res = sp_plain_pdf_stream_fprintf (pdf, "cm" PDF_EOL);

	return res;
}

static unsigned int
sp_plain_pdf_release (SPPrintPlainDriver *driver)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;
	int res;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	printf ("pdf_release\n");

	if (!pmod->stream) return -1;

	pdf->color_set_fill = FALSE;
	pdf->color_set_stroke = FALSE;

	res = sp_plain_pdf_stream_fprintf (pdf, "Q" PDF_EOL);

	return res;
}

static unsigned int
sp_plain_pdf_fill (SPPrintPlainDriver *driver, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			    const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	if (!pmod->stream) return -1;

	if (style->fill.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->fill.value.color, rgb);

		/* Set fill color */
		if (! (pdf->color_set_fill &&
			   (pdf->fill_rgb[0] == rgb[0]) &&
			   (pdf->fill_rgb[1] == rgb[1]) &&
			   (pdf->fill_rgb[2] == rgb[2]))) {
			sp_plain_pdf_stream_fprintf (pdf, "%g %g %g rg" PDF_EOL, rgb[0], rgb[1], rgb[2]);
			pdf->fill_rgb[0] = rgb[0];
			pdf->fill_rgb[1] = rgb[1];
			pdf->fill_rgb[2] = rgb[2];
			pdf->color_set_fill = TRUE;
		}
		sp_plain_pdf_print_bpath (pdf, bpath->path);

		switch (style->fill_rule.value) {
		case SP_WIND_RULE_NONZERO:
			sp_plain_pdf_stream_fprintf (pdf, "f" PDF_EOL);
			break;
		case SP_WIND_RULE_EVENODD:
		default:
			sp_plain_pdf_stream_fprintf (pdf, "f*" PDF_EOL);
			break;
		}
	}

	return 0;
}

static unsigned int
sp_plain_pdf_stroke (SPPrintPlainDriver *driver, const NRBPath *bpath, const NRMatrixF *ctm, const SPStyle *style,
			      const NRRectF *pbox, const NRRectF *dbox, const NRRectF *bbox)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	if (!pmod->stream) return -1;

	if (style->stroke.type == SP_PAINT_TYPE_COLOR) {
		float rgb[3];

		sp_color_get_rgb_floatv (&style->stroke.value.color, rgb);

		/* Set stroke color */
		if (! (pdf->color_set_stroke && 
			   (pdf->stroke_rgb[0] != rgb[0]) &&
			   (pdf->stroke_rgb[1] != rgb[1]) &&
			   (pdf->stroke_rgb[2] != rgb[2]))) {
			sp_plain_pdf_stream_fprintf (pdf, "%.3g %.3g %.3g RG" PDF_EOL, rgb[0], rgb[1], rgb[2]);
			pdf->stroke_rgb[0] = rgb[0];
			pdf->stroke_rgb[1] = rgb[1];
			pdf->stroke_rgb[2] = rgb[2];
			pdf->color_set_stroke = TRUE;
		}
		
		/* Set dash */
		if (style->stroke_dash.n_dash > 0) {
			int i;
			sp_plain_pdf_stream_fprintf (pdf, "[");
			for (i = 0; i < style->stroke_dash.n_dash; i++) {
				sp_plain_pdf_stream_fprintf (pdf, (i) ? " " : "");
				sp_plain_pdf_stream_fprint_double (pdf, style->stroke_dash.dash[i]);
			}
			sp_plain_pdf_stream_fprintf (pdf, "] %g d" PDF_EOL, style->stroke_dash.offset);
		} else {
			sp_plain_pdf_stream_fprintf (pdf, "[] 0 d" PDF_EOL);
		}

		/* Set line */
		sp_plain_pdf_stream_fprint_double (pdf, style->stroke_width.computed);
		sp_plain_pdf_stream_fprintf (pdf, " w %d J %d j ",
							  style->stroke_linecap.computed,
							  style->stroke_linejoin.computed);
		sp_plain_pdf_stream_fprint_double (pdf, style->stroke_miterlimit.value);
		sp_plain_pdf_stream_fprintf (pdf, " M" PDF_EOL);

		/* Path */
		sp_plain_pdf_print_bpath (pdf, bpath->path);

		/* Stroke */
		sp_plain_pdf_stream_fprintf (pdf, "S" PDF_EOL);
	}

	return 0;
}

static unsigned int
sp_plain_pdf_image (SPPrintPlainDriver *driver, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			     const NRMatrixF *transform, const SPStyle *style)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	if (!pmod->stream) return -1;

	return sp_pdf_print_image (pmod->stream, px, w, h, rs, transform);
}

/* PDF helpers */

static void
sp_plain_pdf_print_bpath (SPPrintPlainPDFDriver *pdf, const ArtBpath *bp)
{
	unsigned int started, closed;

	started = FALSE;
	closed = FALSE;
	while (bp->code != ART_END) {
		switch (bp->code) {
		case ART_MOVETO:
			if (started && closed) {
				sp_plain_pdf_stream_fprintf (pdf, "h" PDF_EOL);
			}
			started = TRUE;
			closed = TRUE;
			sp_plain_pdf_stream_fprintf (pdf, "%g %g m" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_MOVETO_OPEN:
			if (started && closed) {
				sp_plain_pdf_stream_fprintf (pdf, "h" PDF_EOL);
			}
			started = FALSE;
			closed = FALSE;
			sp_plain_pdf_stream_fprintf (pdf, "%g %g m" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_LINETO:
			sp_plain_pdf_stream_fprintf (pdf, "%g %g l" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_CURVETO:
			sp_plain_pdf_stream_fprintf (pdf, "%g %g %g %g %g %g c" PDF_EOL, bp->x1, bp->y1, bp->x2, bp->y2, bp->x3, bp->y3);
			break;
		default:
			break;
		}
		bp += 1;
	}
	if (started && closed) {
		sp_plain_pdf_stream_fprintf (pdf, "h" PDF_EOL);
	}
}

static int
sp_plain_pdf_fprintf (SPPrintPlainPDFDriver *pdf, const char *format, ...)
{
	va_list args;
	char *text;
	int len;

	va_start (args, format);
	text = g_strdup_vprintf (format, args);
	va_end (args);

	len = strlen (text);
	fputs (text, pdf->module->stream);
	pdf->offset += len;

	g_free (text);

	return len;
}

static int
sp_plain_pdf_fwrite (SPPrintPlainPDFDriver *pdf, const char *buffer, int len)
{
	fputs (buffer, pdf->module->stream);
	pdf->offset += len;

	return len;
}

static int
sp_plain_pdf_fprint_double (SPPrintPlainPDFDriver *pdf, double value)
{
	unsigned char c[32];
	int len;
	
	len = arikkei_dtoa_simple (c, 32, value, 6, 0, FALSE);
	fputs (c, pdf->module->stream);
	pdf->offset += len;

	return len;
}

static int
sp_plain_pdf_stream_fprintf (SPPrintPlainPDFDriver *pdf, const char *format, ...)
{
	va_list args;
	char *text;
	int len;

	va_start (args, format);
	text = g_strdup_vprintf (format, args);
	va_end (args);

	len = sp_pdf_buffer_write (pdf->stream, text, -1);

	g_free (text);

	return len;
}

static int
sp_plain_pdf_stream_fprint_double (SPPrintPlainPDFDriver *pdf, double value)
{
	unsigned char c[32];
	int len;
	
	len = arikkei_dtoa_simple (c, 32, value, 6, 0, FALSE);

	sp_pdf_buffer_write (pdf->stream, c, len);

	return len;
}

static int
sp_plain_pdf_write_contents (SPPrintPlainPDFDriver *pdf)
{
	int res = 0;

	printf ("pdf_write_contents\n");

	sp_plain_pdf_object_start (pdf, pdf->contents_id);
	sp_plain_pdf_fprintf (pdf, "/Length %d" PDF_EOL, pdf->stream->length);
	sp_plain_pdf_object_start_stream (pdf, pdf->contents_id);
	sp_plain_pdf_fwrite (pdf, pdf->stream->buffer, pdf->stream->length);
	sp_plain_pdf_object_end (pdf, pdf->contents_id);

	return res;
}

static int
sp_plain_pdf_write_resources (SPPrintPlainPDFDriver *pdf)
{
	int res = 0;

	printf ("pdf_write_resources\n");

	sp_plain_pdf_object_start (pdf, pdf->resources_id);
	sp_plain_pdf_fprintf (pdf, "/ProcSet [/PDF");
#if 0
	if (pdf->use_images) {
		sp_plain_pdf_fprintf (pdf, " /ImageC");
	}
#endif
	sp_plain_pdf_fprintf (pdf, "]" PDF_EOL);
	sp_plain_pdf_object_end (pdf, pdf->resources_id);
	
	return res;
}

static int
sp_plain_pdf_write_page (SPPrintPlainPDFDriver *pdf)
{
	int res = 0;
	
	res += sp_plain_pdf_write_contents (pdf);

	res += sp_plain_pdf_write_resources (pdf);

	return res;
}

static int
sp_plain_pdf_write_pages (SPPrintPlainPDFDriver *pdf, int catalog_id)
{
	SPModulePrintPlain *pmod;
	int res = 0;
	int pages_id;

	pmod = pdf->module;

	pages_id = sp_plain_pdf_object_new (pdf);

	/* We have only single page */
	sp_plain_pdf_object_start (pdf, pdf->page_id);
	sp_plain_pdf_fprintf (pdf,
						  "/Type /Page" PDF_EOL
						  "/Parent %d 0 R" PDF_EOL
						  "/Resources %d 0 R" PDF_EOL
						  "/Contents %d 0 R" PDF_EOL,
						  pages_id,
						  pdf->resources_id,
						  pdf->contents_id);
	sp_plain_pdf_object_end (pdf, pdf->page_id);

	/* Pages object */
	sp_plain_pdf_object_start (pdf, pages_id);
	sp_plain_pdf_fprintf (pdf,
						  "/Type /Pages" PDF_EOL
						  "/Kids [%d 0 R]" PDF_EOL /* Single page */
						  "/Count %d" PDF_EOL
						  "/MediaBox [0 0 %d %d]" PDF_EOL,
						  pdf->page_id,
						  1,
						  (int)pmod->width, (int)pmod->height);
	sp_plain_pdf_object_end (pdf, pages_id);
	
	/* Catalog */
	sp_plain_pdf_object_start (pdf, catalog_id);
	sp_plain_pdf_fprintf (pdf,
						  "/Type /Catalog" PDF_EOL
						  "/Pages %d 0 R" PDF_EOL,
						  pages_id);
	sp_plain_pdf_object_end (pdf, catalog_id);
	
	return res;
}

static int
sp_plain_pdf_object_new (SPPrintPlainPDFDriver *pdf)
{
	SPPDFObject *object;

	object = (SPPDFObject*)malloc (sizeof (SPPDFObject));

	object->id = ++pdf->n_objects;
	object->offset = 0;
	object->state_dict = PDFO_DICT_NONE;
	object->state_stream = PDFO_STREAM_NONE;

	if (pdf->objects) {
		object->next = pdf->objects;
	} else {
		object->next = NULL;
	}
	pdf->objects = object;

	return object->id;
}

static void
sp_plain_pdf_object_start (SPPrintPlainPDFDriver *pdf, int object_id)
{
	SPPDFObject *object;

	object = pdf->objects;
	while (object && object->id != object_id) object = object->next;
	assert (object);

	object->offset = pdf->offset;

	sp_plain_pdf_fprintf (pdf,
						  "%d 0 obj" PDF_EOL
						  "<<" PDF_EOL,
						  object_id);

	if (object->state_dict != PDFO_DICT_NONE) g_warning ("Dictionary state is invalid: object = %d, state = %d", object_id, object->state_dict);
	object->state_dict = PDFO_DICT_START;
}

static void
sp_plain_pdf_object_start_stream (SPPrintPlainPDFDriver *pdf, int object_id)
{
	SPPDFObject *object;

	object = pdf->objects;
	while (object && object->id != object_id) object = object->next;
	assert (object);

	if (object->state_dict == PDFO_DICT_START) {
		sp_plain_pdf_fprintf (pdf, ">>" PDF_EOL);
		object->state_dict = PDFO_DICT_END;
	}

	sp_plain_pdf_fprintf (pdf, "stream" PDF_EOL);
	object->state_stream = PDFO_STREAM_START;
}

static void
sp_plain_pdf_object_end (SPPrintPlainPDFDriver *pdf, int object_id)
{
	SPPDFObject *object;

	object = pdf->objects;
	while (object && object->id != object_id) object = object->next;
	assert (object);

	if (object->state_dict == PDFO_DICT_START) {
		sp_plain_pdf_fprintf (pdf, ">>" PDF_EOL);
		object->state_dict = PDFO_DICT_END;
	} else if (object->state_stream == PDFO_STREAM_START) {
		sp_plain_pdf_fprintf (pdf, "endstream" PDF_EOL);
		object->state_stream = PDFO_STREAM_END;
	}

	sp_plain_pdf_fprintf (pdf, "endobj" PDF_EOL);
}

SPPDFObject *
sp_plain_pdf_objects_reverse (SPPDFObject *objects)
{
	SPPDFObject *prev;

	prev = NULL;

	while (objects) {
		SPPDFObject *next;

		next = objects->next;
		objects->next = prev;

		prev = objects;
		objects = next;
	}

	return prev;
}

static SPPDFBuffer *
sp_pdf_buffer_new (void)
{
	SPPDFBuffer *buffer;

	buffer = (SPPDFBuffer *)malloc (sizeof(SPPDFBuffer));
	buffer->buffer = NULL;
	buffer->length = 0;
	buffer->alloc_size = 0;

	return buffer;
}

static SPPDFBuffer *
sp_pdf_buffer_delete (SPPDFBuffer *buffer)
{
	if (buffer->buffer) {
		free (buffer->buffer);
	}
	free (buffer);

	return NULL;
}

static int
sp_pdf_buffer_write (SPPDFBuffer *buffer, unsigned char *text, int length)
{
	if (!text || length == 0) return 0;

	if (length < 0) {
		unsigned char *p;
		p = text;
		for (length = 0; p && *p; p++) length += 1;
	}

	if (length + buffer->length + 1 > buffer->alloc_size) {
		int grow_size = PDF_BUFFER_INITIAL_SIZE;
		while (length + buffer->length + 1 > buffer->alloc_size) {
			buffer->alloc_size += grow_size;
			grow_size <<= 1;
		}
		buffer->buffer = (unsigned char *)realloc (buffer->buffer, buffer->alloc_size);
	}

	memcpy (buffer->buffer + buffer->length, text, length);
	buffer->length += length;
	buffer->buffer[buffer->length] = '\0';

	return length;
}


/* The following code is licensed under GNU GPL */

static void
compress_packbits (int nin,
                   unsigned char *src,
                   int *nout,
                   unsigned char *dst)

{register unsigned char c;
 int nrepeat, nliteral;
 unsigned char *run_start;
 unsigned char *start_dst = dst;
 unsigned char *last_literal = NULL;

 for (;;)
 {
   if (nin <= 0) break;

   run_start = src;
   c = *run_start;

   /* Search repeat bytes */
   if ((nin > 1) && (c == src[1]))
   {
     nrepeat = 1;
     nin -= 2;
     src += 2;
     while ((nin > 0) && (c == *src))
     {
       nrepeat++;
       src++;
       nin--;
       if (nrepeat == 127) break; /* Maximum repeat */
     }

     /* Add two-byte repeat to last literal run ? */
     if (   (nrepeat == 1)
         && (last_literal != NULL) && (((*last_literal)+1)+2 <= 128))
     {
       *last_literal += 2;
       *(dst++) = c;
       *(dst++) = c;
       continue;
     }

     /* Add repeat run */
     *(dst++) = (unsigned char)((-nrepeat) & 0xff);
     *(dst++) = c;
     last_literal = NULL;
     continue;
   }
   /* Search literal bytes */
   nliteral = 1;
   nin--;
   src++;

   for (;;)
   {
     if (nin <= 0) break;

     if ((nin >= 2) && (src[0] == src[1])) /* A two byte repeat ? */
       break;

     nliteral++;
     nin--;
     src++;
     if (nliteral == 128) break; /* Maximum literal run */
   }

   /* Could be added to last literal run ? */
   if ((last_literal != NULL) && (((*last_literal)+1)+nliteral <= 128))
   {
     *last_literal += nliteral;
   }
   else
   {
     last_literal = dst;
     *(dst++) = (unsigned char)(nliteral-1);
   }
   while (nliteral-- > 0) *(dst++) = *(run_start++);
 }
 *nout = dst - start_dst;
}

static guint32 ascii85_buf;
static int ascii85_len = 0;
static int ascii85_linewidth = 0;

static void
ascii85_init (void)
{
  ascii85_len = 0;
  ascii85_linewidth = 0;
}

static void
ascii85_flush (FILE *ofp)
{
  char c[5];
  int i;
  gboolean zero_case = (ascii85_buf == 0);
  static int max_linewidth = 75;

  for (i=4; i >= 0; i--)
    {
      c[i] = (ascii85_buf % 85) + '!';
      ascii85_buf /= 85;
    }
  /* check for special case: "!!!!!" becomes "z", but only if not
   * at end of data. */
  if (zero_case && (ascii85_len == 4))
    {
      if (ascii85_linewidth >= max_linewidth)
      {
        putc ('\n', ofp);
        ascii85_linewidth = 0;
      }
      putc ('z', ofp);
      ascii85_linewidth++;
    }
  else
    {
      for (i=0; i < ascii85_len+1; i++)
      {
        if ((ascii85_linewidth >= max_linewidth) && (c[i] != '%'))
        {
          putc ('\n', ofp);
          ascii85_linewidth = 0;
        }
	putc (c[i], ofp);
        ascii85_linewidth++;
      }
    }

  ascii85_len = 0;
  ascii85_buf = 0;
}

static inline void
ascii85_out (unsigned char byte, FILE *ofp)
{
  if (ascii85_len == 4)
    ascii85_flush (ofp);

  ascii85_buf <<= 8;
  ascii85_buf |= byte;
  ascii85_len++;
}

static void
ascii85_nout (int n, unsigned char *uptr, FILE *ofp)
{
 while (n-- > 0)
 {
   ascii85_out (*uptr, ofp);
   uptr++;
 }
}

static void
ascii85_done (FILE *ofp)
{
  if (ascii85_len)
    {
      /* zero any unfilled buffer portion, then flush */
      ascii85_buf <<= (8 * (4-ascii85_len));
      ascii85_flush (ofp);
    }

  putc ('~', ofp);
  putc ('>', ofp);
  putc ('\n', ofp);
}

static unsigned int
sp_pdf_print_image (FILE *ofp, unsigned char *px, unsigned int width, unsigned int height, unsigned int rs,
		   const NRMatrixF *transform)
{
#if 0 /* Not supported yet */
	int i, j;
	/* guchar *data, *src; */
	guchar *packb = NULL, *plane = NULL;

	fprintf (ofp, "q" PDF_EOL);
	fprintf (ofp, "[%g %g %g %g %g %g] concat" PDF_EOL,
		 transform->c[0],
		 transform->c[1],
		 transform->c[2],
		 transform->c[3],
		 transform->c[4],
		 transform->c[5]);
	fprintf (ofp, "%d %d 8 [%d 0 0 -%d 0 %d]" PDF_EOL, width, height, width, height, height);

	/* Write read image procedure */
	fprintf (ofp, "%% Strings to hold RGB-samples per scanline" PDF_EOL);
	fprintf (ofp, "/rstr %d string def" PDF_EOL, width);
	fprintf (ofp, "/gstr %d string def" PDF_EOL, width);
	fprintf (ofp, "/bstr %d string def" PDF_EOL, width);
	fprintf (ofp, "{currentfile /ASCII85Decode filter /RunLengthDecode filter rstr readstring pop}" PDF_EOL);
	fprintf (ofp, "{currentfile /ASCII85Decode filter /RunLengthDecode filter gstr readstring pop}" PDF_EOL);
	fprintf (ofp, "{currentfile /ASCII85Decode filter /RunLengthDecode filter bstr readstring pop}" PDF_EOL);
	fprintf (ofp, "true 3" PDF_EOL);

	/* Allocate buffer for packbits data. Worst case: Less than 1% increase */
	packb = (guchar *)g_malloc ((width * 105)/100+2);
	plane = (guchar *)g_malloc (width);

	/* pdf_begin_data (ofp); */
	fprintf (ofp, "colorimage" PDF_EOL);

#define GET_RGB_TILE(begin) \
  {int scan_lines; \
    scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i); \
    gimp_pixel_rgn_get_rect (&pixel_rgn, begin, 0, i, width, scan_lines); \
    src = begin; }

	for (i = 0; i < height; i++) {
		/* if ((i % tile_height) == 0) GET_RGB_TILE (data); */ /* Get more data */
		guchar *plane_ptr, *src_ptr;
		int rgb, nout;
		unsigned char *src;

		src = px + i * rs;

		/* Iterate over RGB */
		for (rgb = 0; rgb < 3; rgb++) {
			src_ptr = src + rgb;
			plane_ptr = plane;
			for (j = 0; j < width; j++) {
				*(plane_ptr++) = *src_ptr;
				src_ptr += 4;
			}
			compress_packbits (width, plane, &nout, packb);
			ascii85_init ();
			ascii85_nout (nout, packb, ofp);
			ascii85_out (128, ofp); /* Write EOD of RunLengthDecode filter */
			ascii85_done (ofp);
		}
	}
	/* pdf_end_data (ofp); */

#if 0
	fprintf (ofp, "showpage" PDF_EOL);
	g_free (data);
#endif

	if (packb != NULL) g_free (packb);
	if (plane != NULL) g_free (plane);

#if 0
	if (ferror (ofp))
	{
		g_message (_("write error occured"));
		return (FALSE);
	}
#endif

	fprintf (ofp, "Q" PDF_EOL);
#endif
	return 0;
#undef GET_RGB_TILE
}

/* End of GNU GPL code */


SPPrintPlainDriver *
sp_plain_pdf_driver_new (SPPlainPDFType type, SPModulePrintPlain *module)
{
	SPPrintPlainDriver *driver;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)malloc (sizeof(SPPrintPlainPDFDriver));
	driver = (SPPrintPlainDriver *)pdf;

	pdf->module = module;
	pdf->type = type;

	driver->initialize = sp_plain_pdf_initialize;
	driver->finalize = sp_plain_pdf_finalize;
	driver->begin = sp_plain_pdf_begin;
	driver->finish = sp_plain_pdf_finish;
	driver->bind = sp_plain_pdf_bind;
	driver->release = sp_plain_pdf_release;
	driver->fill = sp_plain_pdf_fill;
	driver->stroke = sp_plain_pdf_stroke;
	driver->image = sp_plain_pdf_image;

	return driver;
}
