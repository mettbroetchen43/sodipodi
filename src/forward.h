#ifndef __FORWARD_H__
#define __FORWARD_H__

/*
 * Forward declarations of most used objects
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib-object.h>

#include "api.h"

/* Generic containers */

typedef struct _Sodipodi Sodipodi;
typedef struct _SodipodiClass SodipodiClass;

/* Editing Window */

typedef struct _SPEventContext SPEventContext;
typedef struct _SPEventContextClass SPEventContextClass;

#define SP_TYPE_EVENT_CONTEXT (sp_event_context_get_type ())
#define SP_EVENT_CONTEXT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_EVENT_CONTEXT, SPEventContext))
#define SP_IS_EVENT_CONTEXT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_EVENT_CONTEXT))

GType sp_event_context_get_type (void);

/* Objects */

typedef struct _SPGroup SPGroup;
typedef struct _SPGroupClass SPGroupClass;

typedef struct _SPDefs SPDefs;
typedef struct _SPDefsClass SPDefsClass;

typedef struct _SPRoot SPRoot;
typedef struct _SPRootClass SPRootClass;

typedef struct _SPHeader SPHeader;
typedef struct _SPHeaderClass SPHeaderClass;

typedef struct _SPNamedView SPNamedView;
typedef struct _SPNamedViewClass SPNamedViewClass;

typedef struct _SPGuide SPGuide;
typedef struct _SPGuideClass SPGuideClass;

typedef struct _SPObjectGroup SPObjectGroup;
typedef struct _SPObjectGroupClass SPObjectGroupClass;

typedef struct _SPPath SPPath;
typedef struct _SPPathClass SPPathClass;

typedef struct _SPShape SPShape;
typedef struct _SPShapeClass SPShapeClass;

typedef struct _SPEllipse SPEllipse;
typedef struct _SPEllipseClass SPEllipseClass;

typedef struct _SPCircle SPCircle;
typedef struct _SPCircleClass SPCircleClass;

typedef struct _SPArc SPArc;
typedef struct _SPArcClass SPArcClass;

typedef struct _SPChars SPChars;
typedef struct _SPCharsClass SPCharsClass;

typedef struct _SPText SPText;
typedef struct _SPTextClass SPTextClass;

typedef struct _SPTSpan SPTSpan;
typedef struct _SPTSpanClass SPTSpanClass;

typedef struct _SPString SPString;
typedef struct _SPStringClass SPStringClass;

typedef struct _SPPaintServer SPPaintServer;
typedef struct _SPPaintServerClass SPPaintServerClass;

typedef struct _SPStop SPStop;
typedef struct _SPStopClass SPStopClass;

typedef struct _SPGradient SPGradient;
typedef struct _SPGradientClass SPGradientClass;

typedef struct _SPLinearGradient SPLinearGradient;
typedef struct _SPLinearGradientClass SPLinearGradientClass;

typedef struct _SPRadialGradient SPRadialGradient;
typedef struct _SPRadialGradientClass SPRadialGradientClass;

typedef struct _SPPattern SPPattern;

typedef struct _SPClipPath SPClipPath;
typedef struct _SPClipPathClass SPClipPathClass;

typedef struct _SPAnchor SPAnchor;
typedef struct _SPAnchorClass SPAnchorClass;

/* Misc */

typedef struct _SPColorSpace SPColorSpace;
typedef struct _SPColor SPColor;

typedef struct _SPStyle SPStyle;

typedef struct _SPEvent SPEvent;

typedef struct _SPPrintContext SPPrintContext;

#endif
