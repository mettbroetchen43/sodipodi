#define SP_SVG_FONT_C

#include <string.h>
#include <stdio.h>
#include "svg.h"

GnomeFontWeight
sp_svg_read_font_weight (const gchar * str)
{
	g_return_val_if_fail (str != NULL, GNOME_FONT_BOOK);

	if (strcmp (str, "bold") == 0)
		return GNOME_FONT_BOLD;

	return GNOME_FONT_BOOK;
}

gboolean
sp_svg_read_font_italic (const gchar * str)
{
	g_return_val_if_fail (str != NULL, FALSE);

	if (strcmp (str, "italic") == 0)
		return TRUE;

	return FALSE;
}

