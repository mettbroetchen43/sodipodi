#ifndef __SP_MARKER_H__
#define __SP_MARKER_H__

/*
 * SVG <marker> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * This is quite similar in logic to <svg>
 * Maybe we should merge them somehow (Lauris)
 */

#define SP_TYPE_MARKER (sp_marker_get_type ())
#define SP_MARKER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MARKER, SPMarker))
#define SP_IS_MARKER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MARKER))

typedef struct _SPMarker SPMarker;
typedef struct _SPMarkerClass SPMarkerClass;

#include <libnr/nr-types.h>
#include "svg/svg-types.h"
#include "enums.h"
#include "sp-item-group.h"

struct _SPMarker {
	SPGroup group;

	/* units */
	unsigned int markerUnits_set : 1;
	unsigned int markerUnits : 1;

	/* reference point */
	SPSVGLength refX;
	SPSVGLength refY;

	/* dimensions */
	SPSVGLength markerWidth;
	SPSVGLength markerHeight;

	/* orient */
	unsigned int orient_set : 1;
	unsigned int orient_auto : 1;
	float orient;

	/* viewBox; */
	unsigned int viewBox_set : 1;
	NRRectD viewBox;

	/* preserveAspectRatio */
	unsigned int aspect_set : 1;
	unsigned int aspect_align : 4;
	unsigned int aspect_clip : 1;

	/* Child to parent additional transform */
	NRMatrixD c2p;
};

struct _SPMarkerClass {
	SPGroupClass parent_class;
};

GType sp_marker_get_type (void);

NRArenaItem *sp_marker_show (SPMarker *marker, NRArena *arena, unsigned int key);
void sp_marker_hide (SPMarker *marker, unsigned int key);

#endif
