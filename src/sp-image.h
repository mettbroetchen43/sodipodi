#ifndef __SP_IMAGE_H__
#define __SP_IMAGE_H__

/*
 * SVG <image> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "svg/svg-types.h"
#include "sp-item.h"

#define SP_TYPE_IMAGE (sp_image_get_type ())
#define SP_IMAGE(o) (GTK_CHECK_CAST ((o), SP_TYPE_IMAGE, SPImage))
#define SP_IMAGE_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_IMAGE, SPImageClass))
#define SP_IS_IMAGE(o) (GTK_CHECK_TYPE ((o), SP_TYPE_IMAGE))
#define SP_IS_IMAGE_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_IMAGE))

typedef struct _SPImage SPImage;
typedef struct _SPImageClass SPImageClass;

struct _SPImage {
	SPItem item;

	SPSVGLength x;
	SPSVGLength y;
	SPSVGLength width;
	SPSVGLength height;

	guchar *href;

	GdkPixbuf *pixbuf;
};

struct _SPImageClass {
	SPItemClass parent_class;
};

GtkType sp_image_get_type (void);

END_GNOME_DECLS

#endif
