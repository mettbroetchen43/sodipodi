#ifndef SP_DOCUMENT_PRIVATE_H
#define SP_DOCUMENT_PRIVATE_H

#include "document.h"

struct _SPDocumentPrivate {
	SPReprDoc * rdoc;	/* Our SPReprDoc */
	SPRepr * rroot;		/* Root element of SPReprDoc */

	SPRoot * root;		/* Our SPRoot */

	GHashTable * iddef;	/* id dictionary */

	gchar * uri;		/* Document uri */
	gchar * base;		/* Document base URI */

	SPAspect aspect;	/* Our aspect ratio preferences */
	guint clip :1;		/* Whether we clip or meet outer viewport */

	GSList * namedviews;	/* Our NamedViews */

	/* State */

	guint sensitive: 1;	/* If we save actions to undo stack */
	GSList * undo;		/* Undo stack of reprs */
	GSList * redo;		/* Redo stack of reprs */
	GList * actions;	/* List of current actions */

	/* Handler ID */

	guint modified_id;
};

#endif
