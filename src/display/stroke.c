#define SP_STROKE_C

#include "stroke.h"

SPStroke * default_stroke = NULL;

#define DEBUG_STROKE

#ifdef DEBUG_STROKE
	gint num_stroke = 1;
#endif

SPStroke * sp_stroke_new (void)
{
	SPStroke * stroke;

	stroke = g_new (SPStroke, 1);
	g_return_val_if_fail (stroke != NULL, NULL);

#ifdef DEBUG_STROKE
	num_stroke++;
	g_print ("num_stroke = %d\n", num_stroke);
#endif

	stroke->ref_count = 1;

	stroke->type = SP_STROKE_NONE;
	stroke->color = 0x000000ff;
	stroke->width = 1;
	stroke->scaled = FALSE;
	stroke->join = ART_PATH_STROKE_JOIN_MITER;
	stroke->cap = ART_PATH_STROKE_CAP_BUTT;

	return stroke;
}

SPStroke * sp_stroke_new_colored (guint32 stroke_color, double stroke_width, gint stroke_is_scaled)
{
	SPStroke * stroke;

	stroke = sp_stroke_new ();
	g_return_val_if_fail (stroke != NULL, NULL);

	stroke->type = SP_STROKE_COLOR;
	stroke->color = stroke_color;
	stroke->width = stroke_width;
	stroke->scaled = stroke_is_scaled;

	return stroke;
}

void sp_stroke_ref (SPStroke * stroke)
{
	g_return_if_fail (stroke != NULL);

	stroke->ref_count++;
}

void sp_stroke_unref (SPStroke * stroke)
{
	g_return_if_fail (stroke != NULL);

	stroke->ref_count--;

	if (stroke->ref_count < 1) {

#ifdef DEBUG_STROKE
		num_stroke--;
		g_print ("num_stroke = %d\n", num_stroke);
#endif

		g_free (stroke);
	}
}

SPStroke * sp_stroke_copy (SPStroke * stroke)
{
	SPStroke * new_stroke;

	new_stroke = sp_stroke_new ();
	g_return_val_if_fail (new_stroke != NULL, NULL);

	/* we should do more intelligent copying here */

	memcpy (new_stroke, stroke, sizeof (SPStroke));

	new_stroke->ref_count = 1;

	return new_stroke;
}

SPStroke * sp_stroke_default (void)
{
	if (default_stroke == NULL)
		default_stroke = sp_stroke_new ();

	return default_stroke;
}

