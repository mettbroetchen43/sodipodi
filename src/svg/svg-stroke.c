#define SP_SVG_STROKE_C

#include <string.h>
#include <stdio.h>
#include "svg.h"

SPStrokeType
sp_svg_read_stroke_type (const gchar * str)
{
	g_return_val_if_fail (str != NULL, SP_STROKE_NONE);

	if (strcmp (str, "none") == 0)
		return SP_STROKE_NONE;
	
	return SP_STROKE_COLOR;
}

ArtPathStrokeJoinType sp_svg_read_stroke_join (const gchar * str)
{
	g_return_val_if_fail (str != NULL, ART_PATH_STROKE_JOIN_MITER);

	if (strcmp (str, "round") == 0)
		return ART_PATH_STROKE_JOIN_ROUND;

	if (strcmp (str, "bevel") == 0)
		return ART_PATH_STROKE_JOIN_BEVEL;

	return ART_PATH_STROKE_JOIN_MITER;
}

ArtPathStrokeCapType sp_svg_read_stroke_cap (const gchar * str)
{
	g_return_val_if_fail (str != NULL, ART_PATH_STROKE_CAP_BUTT);

	if (strcmp (str, "round") == 0)
		return ART_PATH_STROKE_CAP_ROUND;

	if (strcmp (str, "square") == 0)
		return ART_PATH_STROKE_CAP_SQUARE;

	return ART_PATH_STROKE_CAP_BUTT;
}

