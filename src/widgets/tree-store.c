#define __SP_TREE_STORE_C__

/*
 * GtkTreeModel implementation for SPDocument
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <gtk/gtktreemodel.h>

#include "macros.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "sp-item-group.h"

#include "tree-store.h"

struct _SPTreeStore {
	GObject parent;

	int stamp;

	SPDocument *doc;
	SPDesktop *dt;
	SPObject *root;
};

struct _SPTreeStoreClass {
	GObjectClass parent_class;
};

static void sp_tree_store_init (SPTreeStore *store);
static void sp_tree_store_class_init (SPTreeStoreClass *Klass);
static void sp_tree_store_tree_model_init (GtkTreeModelIface *iface);
static void sp_tree_store_finalize (GObject *object);

static void sp_tree_store_attach_document (SPTreeStore *store, SPDocument *doc, SPObject *root);
static void sp_tree_store_detach_document (SPTreeStore *store);

/* GtkTreeModel Interface */

static GtkTreeModelFlags sp_tree_store_get_flags (GtkTreeModel *model);
static gint sp_tree_store_get_n_columns (GtkTreeModel *model);
static GType sp_tree_store_get_column_type (GtkTreeModel *model, gint index);
static gboolean sp_tree_store_get_iter (GtkTreeModel *model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *sp_tree_store_get_path (GtkTreeModel *model, GtkTreeIter *iter);
static void sp_tree_store_get_value (GtkTreeModel *model, GtkTreeIter *iter, gint column, GValue *value);
static gboolean sp_tree_store_iter_next (GtkTreeModel *model, GtkTreeIter *iter);
static gboolean sp_tree_store_iter_children (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent);
static gboolean sp_tree_store_iter_has_child (GtkTreeModel *model, GtkTreeIter *iter);
static gint sp_tree_store_iter_n_children (GtkTreeModel *model, GtkTreeIter *iter);
static gboolean sp_tree_store_iter_nth_child (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent, gint n);
static gboolean sp_tree_store_iter_parent (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *child);

static GObjectClass *store_parent_class = NULL;

GType
sp_tree_store_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (SPTreeStoreClass),
			NULL, NULL,
			(GClassInitFunc) sp_tree_store_class_init,
			NULL, NULL,
			sizeof (SPTreeStore),
			0,
			(GInstanceInitFunc) sp_tree_store_init
		};
		static const GInterfaceInfo model_info = {
			(GInterfaceInitFunc) sp_tree_store_tree_model_init,
			NULL, NULL
		};

		type = g_type_register_static (G_TYPE_OBJECT, "SPTreeStore", &info, 0);
		g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL, &model_info);
	}

	return type;
}

static void
sp_tree_store_class_init (SPTreeStoreClass *class)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) class;

	store_parent_class = g_type_class_peek_parent (class);

	object_class->finalize = sp_tree_store_finalize;
}

static void
sp_tree_store_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags = sp_tree_store_get_flags;
	iface->get_n_columns = sp_tree_store_get_n_columns;
	iface->get_column_type = sp_tree_store_get_column_type;
	iface->get_iter = sp_tree_store_get_iter;
	iface->get_path = sp_tree_store_get_path;
	iface->get_value = sp_tree_store_get_value;
	iface->iter_next = sp_tree_store_iter_next;
	iface->iter_children = sp_tree_store_iter_children;
	iface->iter_has_child = sp_tree_store_iter_has_child;
	iface->iter_n_children = sp_tree_store_iter_n_children;
	iface->iter_nth_child = sp_tree_store_iter_nth_child;
	iface->iter_parent = sp_tree_store_iter_parent;
}

static void
sp_tree_store_init (SPTreeStore *tree_store)
{
	static int stamp = 0;

	tree_store->stamp = ++stamp;
}

static void
sp_tree_store_finalize (GObject *object)
{
	SPTreeStore *store;

	store = (SPTreeStore *) object;

	/* Detach from document */
	if (store->doc) {
		sp_tree_store_detach_document (store);
	}

	store_parent_class->finalize (object);
}

static void
sp_tree_store_doc_destroy (SPDocument *doc, SPTreeStore *store)
{
	/* fixme: Should not happen now if we ref document (Lauris) */
	/* fixme: How to handle that? (Lauris) */
	store->doc = NULL;
	store->root = NULL;
}

static void
sp_tree_store_doc_object_added (SPDocument *doc, SPObject *parent, SPObject *ref, SPTreeStore *store)
{
	GtkTreeIter iter = {0};
	GtkTreePath *path;

	iter.stamp = store->stamp;
	if (ref) {
		iter.user_data = ref->next;
	} else {
		iter.user_data = parent->children;
	}
	path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);

#ifdef TREE_DEBUG
	if (1) {
		gint *indices;
		gint depth, i;
		indices = gtk_tree_path_get_indices (path);
		depth = gtk_tree_path_get_depth (path);
		g_print ("ADDED [");
		for (i = 0; i < depth; i++) g_print (" %d", indices[i]);
		g_print ("] (%s) (%s)\n", SP_OBJECT_ID (parent), SP_OBJECT_ID (iter.user_data));
	}
#endif

	if (!ref && !parent->children->next) {
		GtkTreeIter piter = {0};
		/* We are new child */
		gtk_tree_path_up (path);
		piter.stamp = store->stamp;
		piter.user_data = parent;
#ifdef TREE_DEBUG
		g_print ("Toggle\n");
#endif
		gtk_tree_model_row_has_child_toggled ((GtkTreeModel *) store, path, &piter);
		gtk_tree_path_free (path);
		path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);
	}
	gtk_tree_model_row_inserted ((GtkTreeModel *) store, path, &iter);
	gtk_tree_path_free (path);
}

static void
sp_tree_store_doc_object_removed (SPDocument *doc, SPObject *parent, SPObject *ref, SPTreeStore *store)
{
	GtkTreeIter iter = {0};
	GtkTreePath *path;

	/* Find the path to removed item */
	iter.stamp = store->stamp;
	if (ref) {
		iter.user_data = ref;
		path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);
		gtk_tree_path_next (path);
	} else {
		iter.user_data = parent;
		path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);
		gtk_tree_path_down (path);
	}

#ifdef TREE_DEBUG
	if (1) {
		gint *indices;
		gint depth, i;
		indices = gtk_tree_path_get_indices (path);
		depth = gtk_tree_path_get_depth (path);
		g_print ("REMOVED [");
		for (i = 0; i < depth; i++) g_print (" %d", indices[i]);
		g_print ("] (%s)\n", SP_OBJECT_ID (parent));
	}
#endif

	gtk_tree_model_row_deleted ((GtkTreeModel *) store, path);
	/* Notify if no more children */
	if (!parent || !parent->children) {
		gtk_tree_path_up (path);
		iter.user_data = parent;
#ifdef TREE_DEBUG
		g_print ("Toggle\n");
#endif
		gtk_tree_model_row_has_child_toggled ((GtkTreeModel *) store, path, &iter);
	}
	gtk_tree_path_free (path);
}

static void
sp_tree_store_doc_order_changed (SPDocument *doc, SPObject *parent, SPObject *oldref, SPObject *ref, SPTreeStore *store)
{
	SPObject *child;
	int *order;
	int num, oldpos, pos, i;
	GtkTreeIter iter = {0};
	GtkTreePath *path;

#ifdef TREE_DEBUG
	g_print ("Order changed\n");
#endif

	num = 0;
	oldpos = 0;
	pos = 0;
	for (child = parent->children; child; child = child->next) {
		if (child == oldref) oldpos = num + 1;
		if (child == ref) pos = num + 1;
		num += 1;
	}
	order = (int *) alloca (num * sizeof (int));
	if (oldpos < pos) {
		for (i = 0; i < num; i++) {
			if (i < oldpos) {
				order[i] = i;
			} else if (i == oldpos) {
				order[i] = pos;
			} else if (i <= pos) {
				order[i] = i - 1;
			} else {
				order[i] = i;
			}
		}
	} else {
		oldpos -= 1;
		for (i = 0; i < num; i++) {
			if (i < pos) {
				order[i] = i;
			} else if (i < oldpos) {
				order[i] = i + 1;
			} else if (i == oldpos) {
				order[i] = pos;
			} else {
				order[i] = i;
			}
		}
	}
#ifdef TREE_DEBUG
	g_print ("New World order [");
	for (i = 0; i < num; i++) g_print (" %d", order[i]);
	g_print ("]\n");
#endif

	iter.stamp = store->stamp;
	iter.user_data = parent;
	path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);
	gtk_tree_model_rows_reordered ((GtkTreeModel *) store, path, &iter, order);
	gtk_tree_path_free (path);
}

static void
sp_tree_store_doc_object_modified (SPDocument *doc, SPObject *object, unsigned int flags, SPTreeStore *store)
{
	/* fixme: Depending on what info we keep in tree PARENT_MODIFIED may be needed too */
	if (flags & SP_OBJECT_MODIFIED_FLAG) {
		GtkTreeIter iter = {0};
		GtkTreePath *path;

		iter.stamp = store->stamp;
		iter.user_data = object;

		path = gtk_tree_model_get_path ((GtkTreeModel *) store, &iter);
		gtk_tree_model_row_changed ((GtkTreeModel *) store, path, &iter);
		gtk_tree_path_free (path);
	}
}

static void
sp_tree_store_root_release (SPObject *object, SPTreeStore *store)
{
	/* fixme: Do we need this? It should be handled by ::object_removed (Lauris) */
	store->root = NULL;
}

static void
sp_tree_store_attach_document (SPTreeStore *store, SPDocument *doc, SPObject *root)
{
	if (doc && !root) root = (SPObject *) SP_DOCUMENT_ROOT (doc);
	sp_document_ref (doc);
	store->doc = doc;
	store->root = root;
	if (doc) {
		/* Document signals */
		sp_document_set_object_signals (store->doc, TRUE);
		g_signal_connect ((GObject *) store->doc, "destroy",
				  (GCallback) sp_tree_store_doc_destroy, store);
		g_signal_connect ((GObject *) store->doc, "object_added",
				  (GCallback) sp_tree_store_doc_object_added, store);
		g_signal_connect ((GObject *) store->doc, "object_removed",
				  (GCallback) sp_tree_store_doc_object_removed, store);
		g_signal_connect ((GObject *) store->doc, "order_changed",
				  (GCallback) sp_tree_store_doc_order_changed, store);
		g_signal_connect ((GObject *) store->doc, "object_modified",
				  (GCallback) sp_tree_store_doc_object_modified, store);
	}
	if (root) {
		/* Root signals */
		g_signal_connect ((GObject *) store->doc, "release",
				  (GCallback) sp_tree_store_root_release, store);
	}
}

static void
sp_tree_store_detach_document (SPTreeStore *store)
{
	if (store->dt) {
		/* Detach desktop */
		sp_signal_disconnect_by_data (store->dt, store);
		store->dt = NULL;
	}
	/* Detach root */
	sp_signal_disconnect_by_data (store->root, store);
	/* Detach document */
	sp_signal_disconnect_by_data (store->doc, store);
	sp_document_set_object_signals (store->doc, FALSE);
	sp_document_unref (store->doc);
}

static void
sp_tree_store_desktop_shutdown (SPDesktop *dt, SPTreeStore *store)
{
	/* Document remains attached (Lauris) */
	store->dt = NULL;
}

static void
sp_tree_store_desktop_root_set (SPDesktop *dt, SPItem *root, SPTreeStore *store)
{
	if ((SPObject *) root != store->root) {
		GtkTreeIter iter = {0};
		GtkTreePath *path;
		/* Detach document */
		sp_tree_store_detach_document (store);
		/* Attach new document */
		sp_tree_store_attach_document (store, SP_OBJECT_DOCUMENT (root), (SPObject *) root);
		/* fixme: (Lauris) */
		/* Emulate full refresh */
		iter.stamp = store->stamp;
		iter.user_data = NULL;
		path = sp_tree_store_get_path ((GtkTreeModel *) store, &iter);
		gtk_tree_path_down (path);
		gtk_tree_model_row_deleted ((GtkTreeModel *) store, path);
		gtk_tree_model_row_inserted ((GtkTreeModel *) store, path, &iter);
		gtk_tree_path_free (path);
	}
}

static void
sp_tree_store_desktop_base_set (SPDesktop *dt, SPGroup *base, SPTreeStore *store)
{
	GtkTreeIter iter = {0};
	GtkTreePath *path;

	iter.stamp = store->stamp;
	iter.user_data = base;

	path = gtk_tree_model_get_path ((GtkTreeModel *) store, &iter);
	gtk_tree_model_row_changed ((GtkTreeModel *) store, path, &iter);
	gtk_tree_path_free (path);
}

SPTreeStore *
sp_tree_store_new (SPDesktop *dt, SPDocument *doc, SPObject *root)
{
	SPTreeStore *store;

	store = g_object_new (SP_TYPE_TREE_STORE, NULL);

	if (dt && !doc) doc = SP_DT_DOCUMENT (dt);
	if (dt && !root) root = (SPObject *) SP_VIEW_ROOT (dt);

	sp_tree_store_attach_document (store, doc, root);
	store->dt = dt;

	/* Connect signals */
	if (dt) {
		/* Desktop signals */
		g_signal_connect ((GObject *) store->dt, "shutdown",
				  (GCallback) sp_tree_store_desktop_shutdown, store);
		g_signal_connect ((GObject *) store->dt, "root_set",
				  (GCallback) sp_tree_store_desktop_root_set, store);
		g_signal_connect ((GObject *) store->dt, "base_set",
				  (GCallback) sp_tree_store_desktop_base_set, store);
	}
	return store;
}

/* GtkTreeModel Interface */

static GtkTreeModelFlags
sp_tree_store_get_flags (GtkTreeModel *model)
{
	return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
sp_tree_store_get_n_columns (GtkTreeModel *model)
{
	return SP_TREE_STORE_NUM_COLUMNS;
}

static GType
sp_tree_store_get_column_type (GtkTreeModel *model, int idx)
{
	static GType types[] = {
		G_TYPE_STRING,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN,
		G_TYPE_BOOLEAN
	};

	return types[idx];
}

static gboolean
sp_tree_store_get_iter (GtkTreeModel *model, GtkTreeIter *iter, GtkTreePath *path)
{
	SPTreeStore *store;
	GtkTreeIter parent;
	gint *indices;
	gint depth, i;

	store = (SPTreeStore *) model;

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);
	if (depth < 1) return FALSE;

	/* path[0] is always root */
	iter->stamp = store->stamp;
	iter->user_data = store->root;

	/* parent.stamp = store->stamp; */
	/* parent.user_data = store->root; */
	/* if (!gtk_tree_model_iter_nth_child (model, iter, &parent, indices[0])) return FALSE; */

	for (i = 1; i < depth; i++) {
		parent = *iter;
		if (!gtk_tree_model_iter_nth_child (model, iter, &parent, indices[i])) return FALSE;
	}

	return TRUE;
}

static GtkTreePath *
sp_tree_store_get_path (GtkTreeModel *model, GtkTreeIter *iter)
{
	SPTreeStore *store;
	SPObject *object;
	GtkTreePath *path;

	store = (SPTreeStore *) model;

	object = (SPObject *) iter->user_data;

	path = gtk_tree_path_new ();

	while (object->parent && (object != store->root)) {
		SPObject *child;
		int pos;
		/* Find our position and prepend to path */
		pos = 0;
		for (child = object->parent->children; child != object; child = child->next) pos += 1;
		gtk_tree_path_prepend_index (path, pos);
		object = object->parent;
	}

	/* Finally prepend 0 */
	/* fixme: Is this correct (Lauris) */
	gtk_tree_path_prepend_index (path, 0);

	return path;
}


static void
sp_tree_store_get_value (GtkTreeModel *model, GtkTreeIter *iter, int column, GValue *value)
{
	SPTreeStore *store;
	SPObject *object;

	store = (SPTreeStore *) model;

	object = (SPObject *) iter->user_data;

	switch (column) {
	case SP_TREE_STORE_COLUMN_ID:
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, SP_OBJECT_ID (object));
		break;
	case SP_TREE_STORE_COLUMN_IS_GROUP:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, SP_IS_GROUP (object));
		break;
	case SP_TREE_STORE_COLUMN_IS_ITEM:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, SP_IS_ITEM (object));
		break;
	case SP_TREE_STORE_COLUMN_IS_VISUAL:
		g_value_init (value, G_TYPE_BOOLEAN);
		while (object) {
			if (!SP_IS_ITEM (object)) break;
			object = object->parent;
		}
		g_value_set_boolean (value, !object);
		break;
	case SP_TREE_STORE_COLUMN_TARGET:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, store->dt && (object == (SPObject *) store->dt->base));
		break;
	case SP_TREE_STORE_COLUMN_VISIBLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, SP_IS_ITEM (object) && (((SPItem *) object)->visible));
		break;
	case SP_TREE_STORE_COLUMN_SENSITIVE:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, SP_IS_ITEM (object) && (((SPItem *) object)->sensitive));
		break;
	case SP_TREE_STORE_COLUMN_PRINTABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, SP_IS_ITEM (object) && (((SPItem *) object)->printable));
		break;
	default:
		g_value_init (value, G_TYPE_INVALID);
		break;
	}
}

static gboolean
sp_tree_store_iter_next (GtkTreeModel *model, GtkTreeIter *iter)
{
	SPTreeStore *store;
	SPObject *object;

	store = (SPTreeStore *) model;

	if (iter) {
		object = (SPObject *) iter->user_data;
		if (object) {
			if (object->next) {
				iter->user_data = object->next;
			}
			return object->next != NULL;
		}
	}
	/* NULL or [0] iter is tree anchor */
	return FALSE;
}

static gboolean
sp_tree_store_iter_children (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	SPTreeStore *store;
	SPObject *object;

	store = (SPTreeStore *) model;

	if (parent) {
		object = (SPObject *) parent->user_data;
		if (object) {
			if (object->children) {
				iter->stamp = store->stamp;
				iter->user_data = object->children;
			}
			return object->children != NULL;
		}
	}
	/* NULL or [0] iter is tree anchor */
	iter->stamp = store->stamp;
	iter->user_data = store->root;
	return TRUE;
}

static gboolean
sp_tree_store_iter_has_child (GtkTreeModel *model, GtkTreeIter *iter)
{
	SPTreeStore *store;
	SPObject *object;

	store = (SPTreeStore *) model;

	if (iter) {
		object = (SPObject *) iter->user_data;
		if (object) {
			return object->children != NULL;
		}
	}
	/* NULL or [0] iter is tree anchor */
	return TRUE;
}

static gint
sp_tree_store_iter_n_children (GtkTreeModel *model, GtkTreeIter *iter)
{
	SPTreeStore *store;
	SPObject *object, *child;
	int num;

	store = (SPTreeStore *) model;

	if (iter) {
		object = (SPObject *) iter->user_data;
		if (object) {
			num = 0;
			for (child = object->children; child; child = child->next) num += 1;
			return num;
		}
	}
	/* NULL or [0] iter is tree anchor */
	return 1;
}

static gboolean
sp_tree_store_iter_nth_child (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *parent, gint idx)
{
	SPTreeStore *store;
	SPObject *object, *child;
	int pos;

	store = (SPTreeStore *) model;

	if (parent) {
		object = (SPObject *) parent->user_data;
		if (object) {
			pos = 0;
			for (child = object->children; child; child = child->next) {
				if (pos == idx) {
					iter->stamp = store->stamp;
					iter->user_data = child;
					break;
				}
				pos += 1;
			}
			return child != NULL;
		}
	}
	/* NULL or [0] iter is tree anchor */
	if (idx == 0) {
		iter->stamp = store->stamp;
		iter->user_data = store->root;
	}
	return idx == 0;
}

static gboolean
sp_tree_store_iter_parent (GtkTreeModel *model, GtkTreeIter *iter, GtkTreeIter *child)
{
	SPTreeStore *store;
	SPObject *object;

	store = (SPTreeStore *) model;

	if (child) {
		object = (SPObject *) child->user_data;
		if (object) {
			iter->stamp = store->stamp;
			iter->user_data = object->parent;
		}
		return object != NULL;
	}
	/* NULL or [0] iter is tree anchor */
	return FALSE;
}

