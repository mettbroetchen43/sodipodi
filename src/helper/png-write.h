#ifndef SP_PNG_WRITE_H
#define SP_PNG_WRITE_H

#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_pixbuf.h>

gboolean sp_png_write_rgba (const gchar * filename, ArtPixBuf * pixbuf);

#endif
