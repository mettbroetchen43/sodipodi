#define SP_FILL_C

#include <string.h>

#include "fill.h"

SPFill * default_fill = NULL;

#define noDEBUG_FILL

#ifdef DEBUG_FILL
	gint num_fill = 0;
#endif

SPFill * sp_fill_new (void)
{
	SPFill * fill;

	fill = g_new (SPFill, 1);
	g_return_val_if_fail (fill != NULL, NULL);

#ifdef DEBUG_FILL
	num_fill++;
	g_print ("num_fill = %d\n", num_fill);
#endif

	fill->ref_count = 1;

	fill->type = SP_FILL_COLOR;
	fill->color = 0xff0000ff;

	fill->handler.pre = NULL;
	fill->handler.post = NULL;
	fill->handler.pixel.ind = NULL;
	fill->handler.data = NULL;
	fill->handler.name = NULL;

	return fill;
}

SPFill * sp_fill_new_colored (guint32 fill_color)
{
	SPFill * fill;

	fill = sp_fill_new ();
	g_return_val_if_fail (fill != NULL, NULL);

	fill->type = SP_FILL_COLOR;
	fill->color = fill_color;

	return fill;
}

void sp_fill_ref (SPFill * fill)
{
	g_return_if_fail (fill != NULL);

	fill->ref_count++;
}

void sp_fill_unref (SPFill * fill)
{
	g_return_if_fail (fill != NULL);

	fill->ref_count--;

	if (fill->ref_count < 1) {

#ifdef DEBUG_FILL
		num_fill--;
		g_print ("num_fill = %d\n", num_fill);
#endif

		g_free (fill);
	}
}

SPFill * sp_fill_copy (SPFill * fill)
{
	SPFill * new_fill;

	new_fill = sp_fill_new ();
	g_return_val_if_fail (new_fill != NULL, NULL);

	/* we should do more intelligent copying here */

	memcpy (new_fill, fill, sizeof (SPFill));

	new_fill->ref_count = 1;

	return new_fill;
}

SPFill * sp_fill_default (void)
{
	if (default_fill == NULL)
		default_fill = sp_fill_new ();

	return default_fill;
}

