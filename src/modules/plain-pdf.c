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

/* Plain Print */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

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

typedef struct _SPPrintPlainPDFDriver SPPrintPlainPDFDriver;
typedef struct _SPPDFObject SPPDFObject;

struct _SPPrintPlainPDFDriver {
	SPPrintPlainDriver driver;
	SPModulePrintPlain *module;
	unsigned int type;
	/* PDF internal state */
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

struct _SPPDFObject {
	int id;
	int offset;
	SPPDFObject *next;
};

static void sp_plain_pdf_print_bpath (SPPrintPlainPDFDriver *pdf, const ArtBpath *bp);
static int sp_plain_pdf_fprintf (SPPrintPlainPDFDriver *pdf, const char *format, ...);
static int sp_plain_pdf_fprint_double (SPPrintPlainPDFDriver *pdf, double value);
static unsigned int sp_pdf_print_image (FILE *ofp, unsigned char *px, unsigned int width, unsigned int height, unsigned int rs, const NRMatrixF *transform);

static int sp_plain_pdf_flush_contents (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_flush_resources (SPPrintPlainPDFDriver *pdf);

static int sp_plain_pdf_object_new (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_object_start (SPPrintPlainPDFDriver *pdf);
static int sp_plain_pdf_object_end (SPPrintPlainPDFDriver *pdf);


static void
sp_plain_pdf_initialize (SPPrintPlainDriver *driver)
{
	SPPrintPlainPDFDriver *pdf;

	printf ("initialize\n");
	pdf = (SPPrintPlainPDFDriver *)driver;

	pdf->offset = 0;
	pdf->color_set_stroke = FALSE;
	pdf->color_set_fill = FALSE;
}

static void
sp_plain_pdf_finalize (SPPrintPlainDriver *driver)
{
	printf ("finalize\n");
}

static unsigned int
sp_plain_pdf_begin (SPPrintPlainDriver *driver, SPDocument *doc)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;
/* 	unsigned char c[32]; */
	int res;

	printf ("begin\n");
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

/* fixme:
	if (res >= 0) res = fprintf (pmod->stream, "%g %g translate\n", 0.0, sp_document_height (doc));
	if (res >= 0) res = fprintf (pmod->stream, "0.8 -0.8 scale\n");
*/
	return res;
}

static unsigned int
sp_plain_pdf_finish (SPPrintPlainDriver *driver)
{
	int res;
	SPModulePrint *mod;
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;

	printf ("finish\n");
	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;
	mod = (SPModulePrint*)pmod;

	if (!pmod->stream) return 0;

/* 	res = sp_plain_pdf_fprintf (pdf, "showpage" PDF_EOL); */
/* 	sp_plain_pdf_flush_contents (driver); */
/* 	sp_plain_pdf_flush_resources (driver); */

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

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	printf ("bind\n");
	if (!pmod->stream) return -1;

	return fprintf (pmod->stream, "q [%g %g %g %g %g %g] concat\n",
			transform->c[0], transform->c[1],
			transform->c[2], transform->c[3],
			transform->c[4], transform->c[5]);
}

static unsigned int
sp_plain_pdf_release (SPPrintPlainDriver *driver)
{
	SPModulePrintPlain *pmod;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)driver;
	pmod = pdf->module;

	printf ("release\n");
	if (!pmod->stream) return -1;

	return fprintf (pmod->stream, "Q" PDF_EOL);
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
		if (pdf->color_set_fill && (
				(pdf->fill_rgb[0] != rgb[0]) ||
				(pdf->fill_rgb[1] != rgb[1]) ||
				(pdf->fill_rgb[2] != rgb[2]))) {
			sp_plain_pdf_fprintf (pdf, "%g %g %g rg" PDF_EOL, rgb[0], rgb[1], rgb[2]);
		}
		sp_plain_pdf_print_bpath (pdf, bpath->path);

		switch (style->fill_rule.value) {
		case SP_WIND_RULE_NONZERO:
			sp_plain_pdf_fprintf (pdf, "f" PDF_EOL);
			break;
		case SP_WIND_RULE_EVENODD:
		default:
			sp_plain_pdf_fprintf (pdf, "f*" PDF_EOL);
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
		if (pdf->color_set_stroke && (
				(pdf->stroke_rgb[0] != rgb[0]) ||
				(pdf->stroke_rgb[1] != rgb[1]) ||
				(pdf->stroke_rgb[2] != rgb[2]))) {
			sp_plain_pdf_fprintf (pdf, "%.3g %.3g %.3g RG" PDF_EOL, rgb[0], rgb[1], rgb[2]);
			pdf->stroke_rgb[0] = rgb[0];
			pdf->stroke_rgb[1] = rgb[1];
			pdf->stroke_rgb[2] = rgb[2];
			pdf->color_set_stroke = TRUE;
		}
		
		/* Set dash */
		if (style->stroke_dash.n_dash > 0) {
			int i;
			sp_plain_pdf_fprintf (pdf, "[");
			for (i = 0; i < style->stroke_dash.n_dash; i++) {
				sp_plain_pdf_fprintf (pdf, (i) ? " %g" : "%g", style->stroke_dash.dash[i]);
			}
			sp_plain_pdf_fprintf (pdf, "] %g setdash" PDF_EOL, style->stroke_dash.offset);
		} else {
			sp_plain_pdf_fprintf (pdf, "[] 0 setdash" PDF_EOL);
		}

		/* Set line */
		sp_plain_pdf_fprint_double (pdf, style->stroke_width.computed);
		sp_plain_pdf_fprintf (pdf, " w %d J %d j ",
							  style->stroke_linecap.computed,
							  style->stroke_linejoin.computed);
		sp_plain_pdf_fprint_double (pdf, style->stroke_miterlimit.value);
		sp_plain_pdf_fprintf (pdf, " M" PDF_EOL);

		/* Path */
		sp_plain_pdf_print_bpath (pdf, bpath->path);

		/* Stroke */
		sp_plain_pdf_fprintf (pdf, "S" PDF_EOL);
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
				sp_plain_pdf_fprintf (pdf, "h" PDF_EOL);
			}
			started = TRUE;
			closed = TRUE;
			sp_plain_pdf_fprintf (pdf, "%g %g m" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_MOVETO_OPEN:
			if (started && closed) {
				sp_plain_pdf_fprintf (pdf, "h" PDF_EOL);
			}
			started = FALSE;
			closed = FALSE;
			sp_plain_pdf_fprintf (pdf, "%g %g m" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_LINETO:
			sp_plain_pdf_fprintf (pdf, "%g %g l" PDF_EOL, bp->x3, bp->y3);
			break;
		case ART_CURVETO:
			sp_plain_pdf_fprintf (pdf, "%g %g %g %g %g %g c" PDF_EOL, bp->x1, bp->y1, bp->x2, bp->y2, bp->x3, bp->y3);
			break;
		default:
			break;
		}
		bp += 1;
	}
	if (started && closed) {
		sp_plain_pdf_fprintf (pdf, "h" PDF_EOL);
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
	fprintf (pdf->module->stream, text);
	pdf->offset += len;

	g_free (text);

	return len;
}

static int
sp_plain_pdf_fprint_double (SPPrintPlainPDFDriver *pdf, double value)
{
	unsigned char c[32];
	int len;
	
	len = arikkei_dtoa_simple (c, 32, value, 6, 0, FALSE);

	fprintf (pdf->module->stream, c);

	pdf->offset += len;

	return len;
}

static int
sp_plain_pdf_flush_contents (SPPrintPlainPDFDriver *pdf)
{
	int len = 0;

	sp_plain_pdf_fprintf (pdf, "%d 0 obj" PDF_EOL, num_obj);
	sp_plain_pdf_fprintf (pdf, "<<" PDF_EOL);
	sp_plain_pdf_fprintf (pdf, "/Length %d" PDF_EOL, );
	sp_plain_pdf_fprintf (pdf, ">> %d" PDF_EOL, );
}

static int
sp_plain_pdf_flush_resources (SPPrintPlainPDFDriver *pdf)
{
}

static int
sp_plain_pdf_object_new (SPPrintPlainPDFDriver *pdf)
{
	SPPDFObject *object;

	object = (SPPDFObject*)malloc (sizeof (SPPDFObject));

	object->id = pdf->n_objects++;
	object->offset = 0;

	if (pdf->objects) {
		object->next = pdf->objects;
		pdf->objects = object;
	} else {
		pdf->objects = object;
	}

	return object->id;
}

static void
sp_plain_pdf_object_start (SPPrintPlainPDFDriver *pdf, int object_id)
{
	SPPDFObject *object;

	object = pdf->object;
	while (object && object->id != object_id) object = object->next;
	assert (object);

	sp_plain_pdf_fprintf (pdf,
						  "%d 0 obj" PDF_EOL
						  "<<" PDF_EOL,
						  object_id);
}

static void
sp_plain_pdf_object_end (SPPrintPlainPDFDriver *pdf, int object_id)
{
	sp_plain_pdf_fprintf (pdf,
						  ">>" PDF_EOL
						  "endobj" PDF_EOL,
						  object_id);
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
sp_plain_pdf_driver_new (SPPlainPSType type, SPModulePrintPlain *module)
{
	SPPrintPlainDriver *driver;
	SPPrintPlainPDFDriver *pdf;

	pdf = (SPPrintPlainPDFDriver *)malloc (sizeof(SPPrintPlainPDFDriver));
	driver = (SPPrintPlainDriver *)pdf;

	pdf->module = module;
	pdf->type = type;
/*	switch (type) {
	case SP_PLAIN_PDF_DEFAULT:
		break;
	default:
		assert (0);
		break;
	}
*/
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
