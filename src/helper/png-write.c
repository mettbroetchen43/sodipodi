#define SP_PNG_WRITE_C

/*
 * This is example taken from png documentation
 * hope, that it works...
 * (C) Whoever write this. All rights reserved.
 */

#include <png.h>
#include "png-write.h"

/* example.c - an example of using libpng */

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

gboolean sp_png_write_rgba (const gchar * filename, ArtPixBuf * pixbuf)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_color_8 sig_bit;
	png_text text_ptr[3];
	png_uint_32 k;
	png_bytep * row_pointers;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (pixbuf != NULL, FALSE);
	g_return_val_if_fail (pixbuf->n_channels == 4, FALSE);

	/* open the file */

	fp = fopen (filename, "wb");
	g_return_val_if_fail (fp != NULL, FALSE);

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
#if 0
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      png_voidp user_error_ptr, user_error_fn, user_warning_fn);
#else
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	   	NULL, NULL, NULL);
#endif

   if (png_ptr == NULL)
   {
      fclose(fp);
      return FALSE;
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return FALSE;
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error hadnling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_ptr->jmpbuf))
   {
      /* If we get here, we had a problem reading the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
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
	pixbuf->width,
	pixbuf->height,
	8, /* bit_depth */
	PNG_COLOR_TYPE_RGB_ALPHA,
	PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_BASE,
	PNG_FILTER_TYPE_BASE);

#if 0
   /* set the palette if there is one.  REQUIRED for indexed-color images */
   palette = (png_colorp)png_malloc(png_ptr, 256 * sizeof (png_color));
   /* ... set palette colors ... */
   png_set_PLTE(png_ptr, info_ptr, palette, 256);
#endif

#if 0
   /* optional significant bit chunk */
   /* if we are dealing with a grayscale image then */
   sig_bit.gray = true_bit_depth;
#endif
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
   text_ptr[0].key = "Title";
   text_ptr[0].text = "Made with Sodipodi";
   text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[1].key = "Author";
   text_ptr[1].text = "Unknown";
   text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
   text_ptr[2].key = "Description";
   text_ptr[2].text = "a picture";
   text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
   png_set_text(png_ptr, info_ptr, text_ptr, 3);

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

#if 0
   /* invert monocrome pixels */
   png_set_invert_mono(png_ptr);
#endif

#if 0
   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   png_set_shift(png_ptr, &sig_bit);

   /* pack pixels into bytes */
   png_set_packing(png_ptr);

   /* swap location of alpha bytes from ARGB to RGBA */
   png_set_swap_alpha(png_ptr);

   /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
    * RGB (4 channels -> 3 channels). The second parameter is not used.
    */
   png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

   /* flip BGR pixels to RGB */
   png_set_bgr(png_ptr);

   /* swap bytes of 16-bit files to most significant byte first */
   png_set_swap(png_ptr);

   /* swap bits of 1, 2, 4 bit packed pixel formats */
   png_set_packswap(png_ptr);
#endif
   /* turn on interlace handling if you are not using png_write_image() */
#if 0
   if (interlacing)
      number_passes = png_set_interlace_handling(png_ptr);
   else
      number_passes = 1;
#endif

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */

	row_pointers = g_new (png_bytep, pixbuf->height);

	for (k = 0; k < pixbuf->height; k++)
		row_pointers[k] = pixbuf->pixels + k * pixbuf->rowstride;

	/* One of the following output methods is REQUIRED */
	png_write_image(png_ptr, row_pointers);

	g_free (row_pointers);

   /* You can write optional chunks like tEXt, zTXt, and tIME at the end
    * as well.
    */

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

#if 0
   /* if you malloced the palette, free it here */
   free(info_ptr->palette);
#endif

   /* if you allocated any text comments, free them here */

   /* clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(fp);

   /* that's it */
   return TRUE;
}

