#ifndef __SP_TREE_STORE_H__
#define __SP_TREE_STORE_H__

/*
 * GtkTreeModel implementation for SPDocument
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#define SP_TYPE_TREE_STORE (sp_tree_store_get_type ())
#define SP_TREE_STORE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_TREE_STORE, SPTreeStore))
#define SP_IS_TREE_STORE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_TREE_STORE))

typedef struct _SPTreeStore SPTreeStore;
typedef struct _SPTreeStoreClass SPTreeStoreClass;

#include "forward.h"

enum {
	SP_TREE_STORE_COLUMN_ID,
	SP_TREE_STORE_COLUMN_IS_ITEM,
	SP_TREE_STORE_COLUMN_IS_GROUP,
	SP_TREE_STORE_COLUMN_IS_VISUAL,
	SP_TREE_STORE_COLUMN_TARGET,
	SP_TREE_STORE_COLUMN_VISIBLE,
	SP_TREE_STORE_COLUMN_SENSITIVE,
	SP_TREE_STORE_COLUMN_PRINTABLE,
	SP_TREE_STORE_NUM_COLUMNS
};

GType sp_tree_store_get_type (void);

SPTreeStore *sp_tree_store_new (SPDesktop *dt, SPDocument *doc, SPObject *root);

/* iter->user_data points to SPObject */

#endif
