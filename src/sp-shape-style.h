#ifndef SP_SHAPE_STYLE_H
#define SP_SHAPE_STYLE_H

#include "xml/repr.h"
#include "display/fill.h"
#include "display/stroke.h"

SPFill * sp_fill_read (SPFill * fill, SPCSSAttr * css);
SPStroke * sp_stroke_read (SPStroke * stroke, SPCSSAttr * css);

#endif
