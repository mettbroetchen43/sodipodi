#define SP_SVG_AFFINE_C

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <libart_lgpl/art_affine.h>
#include "svg.h"

gdouble *
sp_svg_read_affine (gdouble dst[], const gchar * src)
{
  int idx;
  char keyword[32];
  double args[6];
  int n_args;
  int key_len;
  double tmp_affine[6];

	g_assert (dst != NULL);

	art_affine_identity (dst);

	if (src == NULL)
		return NULL;

  art_affine_identity (dst);

  idx = 0;
  while (src[idx])
    {
      /* skip initial whitespace */
      while (isspace (src[idx]))
	idx++;

      /* parse keyword */
      for (key_len = 0; key_len < sizeof (keyword); key_len++)
	{
	  char c;

	  c = src[idx];
	  if (isalpha (c) || c == '-')
	    keyword[key_len] = src[idx++];
	  else
	    break;
	}
      if (key_len >= sizeof (keyword))
	return NULL;
      keyword[key_len] = '\0';

      /* skip whitespace */
      while (isspace (src[idx]))
	idx++;

      if (src[idx] != '(')
	return NULL;
      idx++;

      for (n_args = 0; ; n_args++)
	{
	  char c;
	  char *end_ptr;

	  /* skip whitespace */
	  while (isspace (src[idx]))
	    idx++;
	  c = src[idx];
	  if (isdigit (c) || c == '+' || c == '-' || c == '.')
	    {
	      if (n_args == sizeof(args) / sizeof(args[0]))
		return NULL; /* too many args */
	      args[n_args] = strtod (src + idx, &end_ptr);
	      idx = end_ptr - src;

	      while (isspace (src[idx]))
		idx++;

	      /* skip optional comma */
	      if (src[idx] == ',')
		idx++;
	    }
	  else if (c == ')')
	    break;
	  else
	    return NULL;
	}
      idx++;

      /* ok, have parsed keyword and args, now modify the transform */
      if (!strcmp (keyword, "matrix"))
	{
	  if (n_args != 6)
	    return NULL;
	  art_affine_multiply (dst, args, dst);
	}
      else if (!strcmp (keyword, "translate"))
	{
	  if (n_args == 1)
	    args[1] = 0;
	  else if (n_args != 2)
	    return NULL;
	  art_affine_translate (tmp_affine, args[0], args[1]);
	  art_affine_multiply (dst, tmp_affine, dst);
	}
      else if (!strcmp (keyword, "scale"))
	{
	  if (n_args == 1)
	    args[1] = args[0];
	  else if (n_args != 2)
	    return NULL;
	  art_affine_scale (tmp_affine, args[0], args[1]);
	  art_affine_multiply (dst, tmp_affine, dst);
	}
      else if (!strcmp (keyword, "rotate"))
	{
	  if (n_args != 1)
	    return NULL;
	  art_affine_rotate (tmp_affine, args[0]);
	  art_affine_multiply (dst, tmp_affine, dst);
	}
      else if (!strcmp (keyword, "skewX"))
	{
	  if (n_args != 1)
	    return NULL;
	  art_affine_shear (tmp_affine, args[0]);
	  art_affine_multiply (dst, tmp_affine, dst);
	}
      else if (!strcmp (keyword, "skewY"))
	{
	  if (n_args != 1)
	    return NULL;
	  art_affine_shear (tmp_affine, args[0]);
	  /* transpose the affine, given that we know [1] is zero */
	  tmp_affine[1] = tmp_affine[2];
	  tmp_affine[2] = 0;
	  art_affine_multiply (dst, tmp_affine, dst);
	}
      else
	return NULL; /* unknown keyword */
    }
  return dst;
}

#define EQ(a,b) (fabs ((a) - (b)) < 1e-9)

gint
sp_svg_write_affine (gchar * buf, gint buflen, gdouble affine[])
{
	if (affine == NULL) return 0;

	/* Test, whether we are translate */
	if (EQ (affine[0], 1.0) && EQ (affine[1], 0.0) && EQ (affine[2], 0.0) && EQ (affine[3], 1.0)) {
		if (EQ (affine[4], 0.0) && EQ (affine[5], 0.0)) {
			*buf = '\0';
			return 0;
		}
		return g_snprintf (buf, buflen, "translate(%g,%g)", affine[4], affine[5]);
	} else if (EQ (affine[1], 0.0) && EQ (affine[2], 0.0) && EQ (affine[4], 0.0) && EQ (affine[5], 0.0)) {
		return g_snprintf (buf, buflen, "scale(%g,%g)", affine[0], affine[3]);
	} else {
		return g_snprintf (buf, buflen, "matrix(%g,%g,%g,%g,%g,%g)", affine[0], affine[1], affine[2], affine[3], affine[4], affine[5]);
	}
}

