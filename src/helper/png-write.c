#define __SP_PNG_WRITE_C__

/*
 * PNG file format utilities
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Whoever wrote this example in libpng documentation
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef WIN32
#include <unistd.h>
#endif

#include <png.h>

#ifdef WIN32
#include "modules/win32.h"
#endif

#include <glib.h>

#include "png-write.h"

/* This is an example of how to use libpng to read and write PNG files.
 * The file libpng.txt is much more verbose then this.  If you have not
 * read it, do so first.  This was designed to be a starting point of an
 * implementation.  This is not officially part of libpng, and therefore
 * does not require a copyright notice.
 *
 * This file does not currently compile, because it is missing certain
 * parts, like allocating memory to hold an image.  You will have to
 * supply these parts to get it to compile.  For an example of a minimal
 * working PNG reader/writer, see pngtest.c, included in this distribution.
 */

/* write a png file */

#if 1
struct SPPNGBD {
	const unsigned char *px;
	int rowstride;
};

static int
sp_png_get_block_stripe (const unsigned char **rows, int row, int num_rows, void *data)
{
	struct SPPNGBD *bd;
	int r;

	bd = (struct SPPNGBD *) data;

	for (r = 0; r < num_rows; r++) {
		rows[r] = bd->px + (row + r) * bd->rowstride;
	}

	return num_rows;
}

int
sp_png_write_rgba (const unsigned char *filename, const unsigned char *px, int width, int height, int rowstride)
{
	struct SPPNGBD bd;

	bd.px = px;
	bd.rowstride = rowstride;

	return sp_png_write_rgba_striped (filename, width, height, sp_png_get_block_stripe, &bd);
}

#else
int
sp_png_write_rgba (const unsigned char *filename, const unsigned char *px, int width, int height, int rowstride)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_color_8 sig_bit;
	png_text text_ptr[3];
	png_uint_32 k;
	png_bytep *row_pointers;

	g_return_val_if_fail (filename != NULL, FALSE);

	/* open the file */

	fp = fopen (filename, "wb");
	g_return_val_if_fail (fp != NULL, FALSE);

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also check that
	 * the library version is compatible with the one used at compile time,
	 * in case we are using dynamically linked libraries.  REQUIRED.
	 */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return FALSE;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, NULL);
		return FALSE;
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	 * error hadnling functions in the png_create_write_struct() call.
	 */
	if (setjmp(png_ptr->jmpbuf)) {
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return FALSE;
	}

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* Set the image information here.  Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	 * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	 * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	 * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	 * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	 */
	png_set_IHDR(png_ptr, info_ptr,
		     width,
		     height,
		     8, /* bit_depth */
		     PNG_COLOR_TYPE_RGB_ALPHA,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);

	/* otherwise, if we are dealing with a color image then */
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	/* if the image has an alpha channel then */
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

#if 0
	/* Optional gamma chunk is strongly suggested if you have any guess
	 * as to the correct gamma of the image.
	 */
	png_set_gAMA(png_ptr, info_ptr, gamma);
#endif

	/* Optionally write comments into the image */
	text_ptr[0].key = "Software";
	text_ptr[0].text = "Sodipodi";
	text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
#if 0
	text_ptr[1].key = "Author";
	text_ptr[1].text = "Unknown";
	text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[2].key = "Description";
	text_ptr[2].text = "a picture";
	text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
#endif
	png_set_text(png_ptr, info_ptr, text_ptr, 1);

	/* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
	/* note that if sRGB is present the cHRM chunk must be ignored
	 * on read and must be written in accordance with the sRGB profile */

	/* Write the file header information.  REQUIRED */
	png_write_info(png_ptr, info_ptr);

	/* Once we write out the header, the compression type on the text
	 * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
	 * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
	 * at the end.
	 */

	/* set up the transformations you want.  Note that these are
	 * all optional.  Only call them if you want them.
	 */

	/* --- CUT --- */

	/* The easiest way to write the image (you may have a different memory
	 * layout, however, so choose what fits your needs best).  You need to
	 * use the first method if you aren't handling interlacing yourself.
	 */

	row_pointers = g_new (png_bytep, height);

	for (k = 0; k < height; k += 64) {
		int ke, kk;
		ke = MIN (k + 64, height);
		for (kk = k; kk < ke; kk++) {
			row_pointers[kk] = (png_bytep) px + kk * rowstride;
		}
		png_write_rows (png_ptr, &row_pointers[k], ke - k);
	}

	g_free (row_pointers);

	/* You can write optional chunks like tEXt, zTXt, and tIME at the end
	 * as well.
	 */

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

	/* if you allocated any text comments, free them here */

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	/* close the file */
	fclose(fp);

	/* that's it */
	return TRUE;
}
#endif

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

int
sp_png_write_rgba_striped (const unsigned char *filename, int width, int height,
			   int (* get_rows) (const unsigned char **rows, int row, int num_rows, void *data),
			   void *data)
{
#ifdef WIN32
	TCHAR *tfilename;
#else
	unsigned char *fsfn;
	gsize bread, bwritten;
#endif
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_color_8 sig_bit;
	png_text text_ptr[3];
	png_uint_32 r;

	g_return_val_if_fail (filename != NULL, FALSE);

	/* open the file */
#ifdef WIN32
	tfilename = sp_utf8_w32_strdup (filename);
	fp = _tfopen (tfilename, TEXT("wb"));
	free (tfilename);
#else
	fsfn = g_filename_from_utf8 (filename, strlen (filename), &bread, &bwritten, NULL);
	fp = fopen (fsfn, "wb");
	g_free (fsfn);
#endif
	g_return_val_if_fail (fp != NULL, FALSE);

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters.  We also check that
	 * the library version is compatible with the one used at compile time,
	 * in case we are using dynamically linked libraries.  REQUIRED.
	 */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose (fp);
		/* fixme: We have to use properly encoded name here (Lauris) */
		unlink (filename);
		return FALSE;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose (fp);
		unlink (filename);
		png_destroy_write_struct(&png_ptr, NULL);
		return FALSE;
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	 * error hadnling functions in the png_create_write_struct() call.
	 */
	if (setjmp (png_ptr->jmpbuf)) {
		/* If we get here, we had a problem reading the file */
		fclose (fp);
		unlink (filename);
		png_destroy_write_struct (&png_ptr, &info_ptr);
		return FALSE;
	}

	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* Set the image information here.  Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	 * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	 * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	 * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	 * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	 */
	png_set_IHDR(png_ptr, info_ptr,
		     width,
		     height,
		     8, /* bit_depth */
		     PNG_COLOR_TYPE_RGB_ALPHA,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);

	/* otherwise, if we are dealing with a color image then */
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	/* if the image has an alpha channel then */
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

#if 0
	/* Optional gamma chunk is strongly suggested if you have any guess
	 * as to the correct gamma of the image.
	 */
	png_set_gAMA(png_ptr, info_ptr, gamma);
#endif

	/* Optionally write comments into the image */
	text_ptr[0].key = "Software";
	text_ptr[0].text = "Sodipodi";
	text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
#if 0
	text_ptr[1].key = "Author";
	text_ptr[1].text = "Unknown";
	text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[2].key = "Description";
	text_ptr[2].text = "a picture";
	text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
#endif
	png_set_text(png_ptr, info_ptr, text_ptr, 1);

	/* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
	/* note that if sRGB is present the cHRM chunk must be ignored
	 * on read and must be written in accordance with the sRGB profile */

	/* Write the file header information.  REQUIRED */
	png_write_info(png_ptr, info_ptr);

	/* Once we write out the header, the compression type on the text
	 * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
	 * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
	 * at the end.
	 */

	/* set up the transformations you want.  Note that these are
	 * all optional.  Only call them if you want them.
	 */

	/* --- CUT --- */

	/* The easiest way to write the image (you may have a different memory
	 * layout, however, so choose what fits your needs best).  You need to
	 * use the first method if you aren't handling interlacing yourself.
	 */

	r = 0;
	while (r < height) {
		png_bytep row_pointers[64];
		int h, n;

		h = MIN (height - r, 64);
		n = get_rows ((const unsigned char **) row_pointers, r, h, data);
		if (!n) longjmp (png_ptr->jmpbuf, 1);
		png_write_rows (png_ptr, row_pointers, n);
		r += n;
	}

	/* You can write optional chunks like tEXt, zTXt, and tIME at the end
	 * as well.
	 */

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

	/* if you allocated any text comments, free them here */

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	/* close the file */
	fclose(fp);

	/* that's it */
	return TRUE;
}

