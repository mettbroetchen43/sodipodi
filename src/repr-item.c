#define SP_REPR_ITEM_C

#include "repr-item.h"

#define noREPR_ITEM_VERBOSE

static void sp_repr_item_destroy (SPRepr * repr, SPItem * item);
static void sp_repr_item_attr_changed (SPRepr * repr, const gchar * attr, SPItem * item);
static void sp_repr_item_content_changed (SPRepr * repr, SPItem * item);

void sp_repr_item_create_tree (SPRepr * parent, SPRepr * repr, SPGroup * parent_item);

SPItem * last_item = NULL;

SPItem *
sp_repr_item_create (SPRepr * repr)
{
	SPItem * item;
	const GList * list;
	SPRepr * child;

	g_assert (repr != NULL);

	last_item = NULL;

	item = sp_item_new_root (repr);

	if (item == NULL)
		return NULL;

#if 0
	sp_repr_set_data (repr, item);
#endif
	sp_repr_set_signal (repr, "destroy", sp_repr_item_destroy, item);
	sp_repr_set_signal (repr, "child_added", sp_repr_item_create_tree, item);
	sp_repr_set_signal (repr, "attr_changed", sp_repr_item_attr_changed, item);
	sp_repr_set_signal (repr, "content_changed", sp_repr_item_content_changed, item);

	if (SP_IS_GROUP (item)) {
		for (list = sp_repr_children (repr); list != NULL; list = list->next) {
			child = (SPRepr *) list->data;
			sp_repr_item_create_tree (repr, child, SP_GROUP (item));
		}
	}

	last_item = item;

	return item;
}

void
sp_repr_item_create_tree (SPRepr * parent, SPRepr * repr, SPGroup * parent_item)
{
	SPItem * item;
	const GList * list;
	SPRepr * child;

	g_assert (repr != NULL);
	g_assert (parent == sp_repr_parent (repr));
	g_assert (parent_item != NULL);
	g_assert (sp_item_repr (SP_ITEM (parent_item)) == parent);

	last_item = NULL;

	item = sp_item_new (repr, parent_item);
	if (item == NULL) return;

#if 0
	sp_repr_set_data (repr, item);
#endif
	sp_repr_set_signal (repr, "destroy", sp_repr_item_destroy, item);
	sp_repr_set_signal (repr, "child_added", sp_repr_item_create_tree, item);
	sp_repr_set_signal (repr, "unparented", sp_repr_item_destroy, item);
	sp_repr_set_signal (repr, "attr_changed", sp_repr_item_attr_changed, item);
	sp_repr_set_signal (repr, "content_changed", sp_repr_item_content_changed, item);

	if (SP_IS_GROUP (item)) {
		for (list = sp_repr_children (repr); list != NULL; list = list->next) {
			child = (SPRepr *) list->data;
			sp_repr_item_create_tree (repr, child, SP_GROUP (item));
		}
	}

	last_item = item;
}

void
sp_repr_item_destroy (SPRepr * repr, SPItem * item)
{
#if 0
#if 0
	const GList * list;
	SPRepr * child;
	SPItem * child_item;
#else
	GSList * l;
	SPItem * child;
#endif

	g_assert (repr != NULL);
	g_assert (sp_item_repr (item) == repr);

	while (item->children) {
		child = (SPItem *) item->children->data;
		sp_repr_item_destroy (sp_item_repr (child), child);
	}

#if 0
	for (list = sp_repr_children (repr); list != NULL; list = list->next) {
		child = (SPRepr *) list->data;
		child_item = sp_repr_data (child);
		if (child_item != NULL) {
			sp_repr_item_destroy (child);
		}
	}
#endif
#endif
	/* we destroy object, it should remove signals itself */
	gtk_object_destroy (GTK_OBJECT (item));
#if 0
	sp_repr_set_data (repr, NULL);
#endif
#if 0
	sp_repr_remove_signals (repr);
#endif
}

static void
sp_repr_item_attr_changed (SPRepr * repr, const gchar * attr, SPItem * item)
{
	g_assert (repr != NULL);
	g_assert (attr != NULL);
	g_assert (item != NULL);
	g_assert (sp_item_repr (item) == repr);

	sp_item_read_attr (item, repr, attr);
}

static void
sp_repr_item_content_changed (SPRepr * repr, SPItem * item)
{
	g_assert (repr != NULL);
	g_assert (item != NULL);
	g_assert (sp_item_repr (item) == repr);

	sp_item_read_attr (item, repr, "SP-INTERNAL-CONTENT-ATTR");
}

#if 0
SPItem *
sp_repr_item (SPRepr * repr)
{
	gpointer data;

	g_assert (repr != NULL);

	data = sp_repr_data (repr);
	if (data == NULL)
		return NULL;
	g_assert (SP_IS_ITEM (data));

	return SP_ITEM (data);
}
#endif

SPItem *
sp_repr_item_last_item (void)
{
	return last_item;
}


