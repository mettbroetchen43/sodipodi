#ifndef SP_DOCUMENT_PRIVATE_H
#define SP_DOCUMENT_PRIVATE_H

#include "sp-defs.h"
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

#define SP_DOCUMENT_DEFS(d) (SP_DOCUMENT (d)->private->root->defs)

struct _SPDocumentPrivate {
	SPReprDoc * rdoc;	/* Our SPReprDoc */
	SPRepr * rroot;		/* Root element of SPReprDoc */

	SPRoot * root;		/* Our SPRoot */

	GHashTable * iddef;	/* id dictionary */

	gchar * uri;		/* Document uri */
	gchar * base;		/* Document base URI */

	SPAspect aspect;	/* Our aspect ratio preferences */
	guint clip :1;		/* Whether we clip or meet outer viewport */

#if 0
	GSList * namedviews;	/* Our NamedViews */
	/* Base <defs> node */
	/* fixme: I do not know, what to do with it (Lauris) */
	SPDefs *defs;
#endif

	/* Resources */
	/* It is GHashTable of GSLists */
	GHashTable *resources;

	/* State */
	guint sensitive: 1; /* If we save actions to undo stack */
	GSList * undo; /* Undo stack of reprs */
	GSList * redo; /* Redo stack of reprs */

	const guchar *key; /* Last action key */
	SPAction *actions; /* List of current actions */

	/* Handler ID */

	guint modified_id;
};

void sp_action_free_list (SPAction *action);

#endif
