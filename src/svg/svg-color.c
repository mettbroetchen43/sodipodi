#define SP_SVG_COLOR_C

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "svg.h"
#include "svg-color.h"

static GHashTable * sp_svg_create_color_hash (void);

guint32
sp_svg_read_color (const guchar *str, guint32 def)
{
	static GHashTable *colors = NULL;
	gchar c[32];
	guint32 val = 0;

	/* 
	 * todo: handle the rgb (r, g, b) and rgb ( r%, g%, b%), syntax 
	 * defined in http://www.w3.org/TR/REC-CSS2/syndata.html#color-units 
	 */

	if (str == NULL) return def;
	while ((*str <= ' ') && *str) str++;
	if (!*str) return def;

	if (str[0] == '#') {
		gint i;
		for (i = 1; str[i]; i++) {
			int hexval;
			if (str[i] >= '0' && str[i] <= '9')
				hexval = str[i] - '0';
			else if (str[i] >= 'A' && str[i] <= 'F')
				hexval = str[i] - 'A' + 10;
			else if (str[i] >= 'a' && str[i] <= 'f')
				hexval = str[i] - 'a' + 10;
			else
				break;
			val = (val << 4) + hexval;
		}
		/* handle #rgb case */
		if (i == 4) {
			val = ((val & 0xf00) << 8) |
				((val & 0x0f0) << 4) |
				(val & 0x00f);
			val |= val << 4;
		}
	} else if (!strncmp (str, "rgb(", 4)) {
		gboolean hasp, hasd;
		gchar *s, *e;
		gdouble r, g, b;

		s = (gchar *) str + 4;
		hasp = FALSE;
		hasd = FALSE;

		r = strtod (s, &e);
		if (s == e) return def;
		s = e;
		if (*s == '%') {
			hasp = TRUE;
			s += 1;
		} else {
			hasd = TRUE;
		}
		while (*s && isspace (*s)) s += 1;
		if (*s != ',') return def;
		s += 1;
		while (*s && isspace (*s)) s += 1;
		g = strtod (s, &e);
		if (s == e) return def;
		s = e;
		if (*s == '%') {
			hasp = TRUE;
			s += 1;
		} else {
			hasd = TRUE;
		}
		while (*s && isspace (*s)) s += 1;
		if (*s != ',') return def;
		s += 1;
		while (*s && isspace (*s)) s += 1;
		b = strtod (s, &e);
		if (s == e) return def;
		s = e;
		if (*s == '%') {
			hasp = TRUE;
		} else {
			hasd = TRUE;
		}
		if (hasp && hasd) return def;
		if (hasp) {
			val = (guint) floor (CLAMP (r, 0.0, 100.0) * 2.559999) << 24;
			val |= ((guint) floor (CLAMP (g, 0.0, 100.0) * 2.559999) << 16);
			val |= ((guint) floor (CLAMP (b, 0.0, 100.0) * 2.559999) << 8);
		} else {
			val = (guint) CLAMP (r, 0, 255) << 24;
			val |= ((guint) CLAMP (g, 0, 255) << 16);
			val |= ((guint) CLAMP (b, 0, 255) << 8);
		}
		return val;
	} else {
		gint i;
		if (!colors) {
			colors = sp_svg_create_color_hash ();
		}
		for (i = 0; i < 31; i++) {
			if (str[i] == ';') {
				c[i] = '\0';
				break;
			}
			c[i] = tolower (str[i]);
			if (!str[i]) break;
		}
		c[31] = '\0';
		/* this will default to black on a failed lookup */
		val = SP_SVG_COLOR (g_hash_table_lookup (colors, c), def);
	}

	return (val << 8);
}

gint
sp_svg_write_color (gchar * buf, gint buflen, guint32 color)
{
	return g_snprintf (buf, buflen, "#%06x", color >> 8);
}

static GHashTable *
sp_svg_create_color_hash (void)
{
	GHashTable * colors;
	int i;
	guint32 * val;
	char * name;

	colors = g_hash_table_new (g_str_hash, g_str_equal);

	for (i = 0; i < SP_SVG_NUMCOLORS; i++)
	{
		name = sp_svg_color_named[i].name;
		val = &(sp_svg_color_named[i].rgb);
		g_hash_table_insert (colors,name,(gpointer) val);
	}

	return colors;
}
