#ifndef _SP_TOOLBOX_H_
#define _SP_TOOLBOX_H_

/*
 * Toolbox widget
 *
 * Authors:
 *   Frank Felfe  <innerspace@iname.com>
 *   Lauris Kaplinski  <lauris@helixcode.com>
 *
 * Copyright (C) 2000-2001 Helix Code, Inc. and authors
 */

typedef struct _SPToolBox SPToolBox;
typedef struct _SPToolBoxClass SPToolBoxClass;

#define SP_TYPE_TOOLBOX            (sp_toolbox_get_type ())
#define SP_TOOLBOX(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_TOOLBOX, SPToolBox))
#define SP_TOOLBOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TOOLBOX, SPToolBoxClass))
#define SP_IS_TOOLBOX(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_TOOLBOX))
#define SP_IS_TOOLBOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TOOLBOX))

#include "gtk/gtk.h"

enum {
	SP_TOOLBOX_VISIBLE = (1 << 0),
	SP_TOOLBOX_STANDALONE = (1 << 1)
};

struct _SPToolBox {
	GtkVBox vbox;
	guint state;
	GtkWidget * contents;
	GtkWidget * window;
	GtkWidget * windowvbox;
	gint width;
	gchar * name;
	gchar * internalname;
};

struct _SPToolBoxClass {
	GtkVBoxClass parent_class;

	gboolean (* set_state) (SPToolBox * toolbox, guint state);
};

GtkType sp_toolbox_get_type (void);

GtkWidget * sp_toolbox_new (GtkWidget * contents, const gchar * name, const gchar * internalname, const gchar * pixmapname);
void sp_toolbox_set_state (SPToolBox * toolbox, guint state);

#endif
