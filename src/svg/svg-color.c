#define SP_SVG_COLOR_C

#include <string.h>
#include <stdio.h>
#include "svg.h"

static GHashTable * sp_svg_create_color_hash (void);

guint32
sp_svg_read_color (const gchar * str)
{
  static GHashTable *colors = NULL;
  gchar c[32];
  gint val = 0;

  /* 
   * todo: handle the rgb (r, g, b) and rgb ( r%, g%, b%), syntax 
   * defined in http://www.w3.org/TR/REC-CSS2/syndata.html#color-units 
   */

#ifdef VERBOSE
  g_print ("color = %s\n", str);
#endif

	if (str == NULL) return 0x00000000;

  if (str[0] == '#')
    {
      int i;
      for (i = 1; str[i]; i++)
	{
	  int hexval;
	  if (str[i] >= '0' && str[i] <= '9')
	    hexval = str[i] - '0';
	  else if (str[i] >= 'A' && str[i] <= 'A')
	    hexval = str[i] - 'A' + 10;
	  else if (str[i] >= 'a' && str[i] <= 'f')
	    hexval = str[i] - 'a' + 10;
	  else
	    break;
	  val = (val << 4) + hexval;
	}
      /* handle #rgb case */
      if (i == 4)
	{
	  val = ((val & 0xf00) << 8) |
	    ((val & 0x0f0) << 4) |
	    (val & 0x00f);
	  val |= val << 4;
	}
#ifdef VERBOSE
      printf ("val = %x\n", val);
#endif
    } 
  else
    {
	if (!colors)
		colors = sp_svg_create_color_hash ();

		strncpy (c, str, 32);
		c[31] = '\0';
		g_strdown (c);
		/* this will default to black on a failed lookup */
		val = GPOINTER_TO_INT (g_hash_table_lookup (colors, c)); 
    }

  return (val << 8);
}

gint
sp_svg_write_color (gchar * buf, gint buflen, guint32 color)
{
	return snprintf (buf, buflen, "#%06x", color >> 8);
}

static GHashTable *
sp_svg_create_color_hash (void)
{
	GHashTable * colors;

	colors = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (colors, "black",    GINT_TO_POINTER (0x000000));
	g_hash_table_insert (colors, "silver",   GINT_TO_POINTER (0xc0c0c0));
	g_hash_table_insert (colors, "gray",     GINT_TO_POINTER (0x808080));
	g_hash_table_insert (colors, "white",    GINT_TO_POINTER (0xFFFFFF));
	g_hash_table_insert (colors, "maroon",   GINT_TO_POINTER (0x800000));
	g_hash_table_insert (colors, "red",      GINT_TO_POINTER (0xFF0000));
	g_hash_table_insert (colors, "purple",   GINT_TO_POINTER (0x800080));
	g_hash_table_insert (colors, "fuchsia",  GINT_TO_POINTER (0xFF00FF));
	g_hash_table_insert (colors, "green",    GINT_TO_POINTER (0x008000));
	g_hash_table_insert (colors, "lime",     GINT_TO_POINTER (0x00FF00));
	g_hash_table_insert (colors, "olive",    GINT_TO_POINTER (0x808000));
	g_hash_table_insert (colors, "yellow",   GINT_TO_POINTER (0xFFFF00));
	g_hash_table_insert (colors, "navy",     GINT_TO_POINTER (0x000080));
	g_hash_table_insert (colors, "blue",     GINT_TO_POINTER (0x0000FF));
	g_hash_table_insert (colors, "teal",     GINT_TO_POINTER (0x008080));
	g_hash_table_insert (colors, "aqua",     GINT_TO_POINTER (0x00FFFF));

	return colors;
}
