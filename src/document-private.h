#ifndef __SP_DOCUMENT_PRIVATE_H__
#define __SP_DOCUMENT_PRIVATE_H__

/*
 * Seldom needed document data
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-defs.h"
#include "sp-root.h"
#include "document.h"

typedef struct _SPAction SPAction;
typedef struct _SPActionAdd SPActionAdd;
typedef struct _SPActionDel SPActionDel;
typedef struct _SPActionChgAttr SPActionChgAttr;
typedef struct _SPActionChgContent SPActionChgContent;
typedef struct _SPActionChgOrder SPActionChgOrder;

/* Actions - i.e. undo/redo stuff */

typedef enum {
	SP_ACTION_INVALID,
	SP_ACTION_ADD,
	SP_ACTION_DEL,
	SP_ACTION_CHGATTR,
	SP_ACTION_CHGCONTENT,
	SP_ACTION_CHGORDER
} SPActionType;

struct _SPActionAdd {
	SPRepr *child; /* Copy of child */
	guchar *ref; /* ID of ref */
};

struct _SPActionDel {
	SPRepr *child; /* Copy of child */
	guchar *ref; /* ID of ref */
};

struct _SPActionChgAttr {
	gint key; /* Quark of key name */
	guchar *oldval; /* Old value */
	guchar *newval; /* New value */
};

struct _SPActionChgContent {
	guchar *oldval; /* Old value */
	guchar *newval; /* New value */
};

struct _SPActionChgOrder {
	guchar *child; /* Child ID */
	guchar *oldref; /* ID of old ref */
	guchar *newref; /* ID of new ref */
};

struct _SPAction {
	SPAction *next;
	SPActionType type;
	guchar *id; /* ID of repr that emitted signal */
	union {
		SPActionAdd add;
		SPActionDel del;
		SPActionChgAttr chgattr;
		SPActionChgContent chgcontent;
		SPActionChgOrder chgorder;
	} act;
};

#define SP_DOCUMENT_DEFS(d) ((SPObject *) SP_ROOT (SP_DOCUMENT_ROOT (d))->defs)

struct _SPDocumentPrivate {
	GHashTable * iddef;	/* id dictionary */

	SPAspect aspect;	/* Our aspect ratio preferences */
	guint clip :1;		/* Whether we clip or meet outer viewport */

	/* Resources */
	/* It is GHashTable of GSLists */
	GHashTable *resources;

	/* State */
	guint sensitive: 1; /* If we save actions to undo stack */
	GSList * undo; /* Undo stack of reprs */
	GSList * redo; /* Redo stack of reprs */

	SPAction *actions; /* List of current actions */
};

void sp_action_free_list (SPAction *action);

#endif
