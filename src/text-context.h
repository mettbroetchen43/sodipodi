#ifndef __SP_TEXT_CONTEXT_H__
#define __SP_TEXT_CONTEXT_H__

/*
 * SPTextContext
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "event-context.h"

#define SP_TC_XIM

#define SP_TYPE_TEXT_CONTEXT (sp_text_context_get_type ())
#define SP_TEXT_CONTEXT(obj) (GTK_CHECK_CAST ((obj), SP_TYPE_TEXT_CONTEXT, SPTextContext))
#define SP_TEXT_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TEXT_CONTEXT, SPTextContextClass))
#define SP_IS_TEXT_CONTEXT(obj) (GTK_CHECK_TYPE ((obj), SP_TYPE_TEXT_CONTEXT))
#define SP_IS_TEXT_CONTEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TEXT_CONTEXT))

typedef struct _SPTextContext SPTextContext;
typedef struct _SPTextContextClass SPTextContextClass;

struct _SPTextContext {
	SPEventContext event_context;

	SPItem *text;

	/* Text item position in root coordinates */
	NRPointF pdoc;
	/* Insertion point position */
	gint ipos;

	guchar uni[5];
	guchar unimode;
	guchar unipos;

	SPCanvasItem *cursor;
	gint timeout;
	guint show : 1;
	guint phase : 1;

#ifdef SP_TC_XIM
	GdkIC *ic;
	GdkICAttr *ic_attr;
	GdkEventMask savedmask;
#endif
};

struct _SPTextContextClass {
	SPEventContextClass parent_class;
};

/* Standard Gtk function */

GtkType sp_text_context_get_type (void);

#endif
