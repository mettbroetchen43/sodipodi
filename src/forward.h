#ifndef _FORWARD_H_
#define _FORWARD_H_

/*
 * Forward declarations of most used objects
 *
 * Author: Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 */

typedef struct _Sodipodi Sodipodi;
typedef struct _SodipodiClass SodipodiClass;

typedef struct _SPDocument SPDocument;
typedef struct _SPDocumentClass SPDocumentClass;

typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;

typedef struct _SPSelection SPSelection;
typedef struct _SPSelectionClass SPSelectionClass;

typedef struct _SPEventContext SPEventContext;
typedef struct _SPEventContextClass SPEventContextClass;

typedef struct _SPObject SPObject;
typedef struct _SPObjectClass SPObjectClass;

typedef struct _SPItem SPItem;
typedef struct _SPItemClass SPItemClass;

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

typedef struct _SPChars SPChars;
typedef struct _SPCharsClass SPCharsClass;

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

typedef struct _SPClipPath SPClipPath;
typedef struct _SPClipPathClass SPClipPathClass;

typedef struct _SPAnchor SPAnchor;
typedef struct _SPAnchorClass SPAnchorClass;

typedef struct _SPColorSpace SPColorSpace;
typedef struct _SPColor SPColor;

typedef struct _SPStyle SPStyle;

typedef struct _SPEvent SPEvent;

#endif
