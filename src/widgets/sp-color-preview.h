#ifndef __SP_COLOR_PREVIEW_H__
#define __SP_COLOR_PREVIEW_H__

/*
 * A simple color preview widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _SPColorPreview SPColorPreview;
typedef struct _SPColorPreviewClass SPColorPreviewClass;

#define SP_TYPE_COLOR_PREVIEW (sp_color_preview_get_type ())
#define SP_COLOR_PREVIEW(o) (GTK_CHECK_CAST ((o), SP_TYPE_COLOR_PREVIEW, SPColorPreview))
#define SP_IS_COLOR_PREVIEW(o) (GTK_CHECK_TYPE ((o), SP_TYPE_COLOR_PREVIEW))

#include <gtk/gtkwidget.h>

/* SPColorPreview */

struct _SPColorPreview {
	GtkWidget widget;
	guint32 rgba;
	guint transparent : 1;
	guint opaque : 1;
};

struct _SPColorPreviewClass {
	GtkWidgetClass parent_class;
};

GtkType sp_color_preview_get_type (void);

GtkWidget *sp_color_preview_new (guint32 rgba, gboolean transparent, gboolean opaque);

void sp_color_preview_set_rgba32 (SPColorPreview *cp, guint32 color);

/* SPColorPicker */

/* Like everything else, it has signals grabbed, dragged, released and changed */

typedef struct _SPColorPicker SPColorPicker;
typedef struct _SPColorPickerClass SPColorPickerClass;

#define SP_TYPE_COLOR_PICKER (sp_color_picker_get_type ())
#define SP_COLOR_PICKER(o) (GTK_CHECK_CAST ((o), SP_TYPE_COLOR_PICKER, SPColorPicker))
#define SP_IS_COLOR_PICKER(o) (GTK_CHECK_TYPE ((o), SP_TYPE_COLOR_PICKER))

GType sp_color_picker_get_type (void);

GtkWidget *sp_color_picker_new (guint32 rgba, const guchar *title);
guint32 sp_color_picker_get_rgba32 (SPColorPicker *picker);

#endif
