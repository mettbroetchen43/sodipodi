#ifndef SP_STROKE_H
#define SP_STROKE_H

/*
 * Stroke settings for canvas items
 *
 */

#include <glib.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>

typedef struct _SPStroke SPStroke;

typedef enum {
	SP_STROKE_NONE,
	SP_STROKE_COLOR
} SPStrokeType;

struct _SPStroke {
	gint ref_count;
	SPStrokeType type;
	guint32 color;
	double width;
	gint scaled;
	ArtPathStrokeJoinType join;
	ArtPathStrokeCapType cap;
};

SPStroke * sp_stroke_new (void);
void sp_stroke_ref (SPStroke * stroke);
void sp_stroke_unref (SPStroke * stroke);

SPStroke * sp_stroke_copy (SPStroke * stroke);
SPStroke * sp_stroke_default (void);

/* Convenience functions */

SPStroke * sp_stroke_new_colored (guint32 stroke_color, double stroke_width, gint stroke_is_scaled);

#endif
