#ifndef __SP_IMAGE_H__
#define __SP_IMAGE_H__

/*
 * SPRect
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "sp-item.h"

#define SP_TYPE_IMAGE (sp_image_get_type ())
#define SP_IMAGE(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_IMAGE, SPImage))
#define SP_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_IMAGE, SPImageClass))
#define SP_IS_IMAGE(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_IMAGE))
#define SP_IS_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_IMAGE))


typedef struct _SPImage SPImage;
typedef struct _SPImageClass SPImageClass;

struct _SPImage {
	SPItem item;

	/* fixme: This violates spec */
	guint width_set : 1;
	guint height_set : 1;

	guchar *href;
	gdouble x, y;
	gdouble width, height;

	GdkPixbuf *pixbuf;
};

struct _SPImageClass {
	SPItemClass parent_class;
};

GtkType sp_image_get_type (void);

END_GNOME_DECLS

#endif
