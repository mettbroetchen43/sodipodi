#define SP_REPR_C

#include "repr.h"
#include "repr-private.h"

static void sp_repr_hash_del_value (gpointer key, gpointer value, gpointer user_data);
static void sp_repr_attr_to_list (gpointer key, gpointer value, gpointer user_data);
static void sp_repr_hash_copy (gpointer key, gpointer value, gpointer new_hash);

#define dontDEBUG_REPR

#ifdef DEBUG_REPR
	gint num_repr = 0;
#endif

SPRepr * sp_repr_new (void)
{
	SPRepr * repr;

	repr = g_new (SPRepr, 1);
	g_return_val_if_fail (repr != NULL, NULL);

#ifdef DEBUG_REPR
	num_repr++;
	g_print ("num_repr = %d\n", num_repr);
#endif

	repr->ref_count = 1;
	repr->parent = NULL;
	repr->name = g_quark_from_static_string ("unknown");
	repr->content = NULL;
	repr->attr = g_hash_table_new (NULL, NULL);
	repr->children = NULL;
	repr->data = NULL;
	repr->destroy = NULL;
	repr->child_added = NULL;
	repr->unparented = NULL;
	repr->attr_changed = NULL;
	repr->content_changed = NULL;

	return repr;
}

void sp_repr_ref (SPRepr * repr)
{
	g_return_if_fail (repr != NULL);

	repr->ref_count++;

	return;
}

void sp_repr_unref (SPRepr * repr)
{
	SPRepr * child;

	g_return_if_fail (repr != NULL);

	repr->ref_count--;

	if (repr->ref_count == 0) {

		/*
		 * parents have to do refcounting !!!
		 */

		g_assert (repr->parent == NULL);

		if (repr->destroy) {
			repr->destroy (repr, repr->destroy_data);
		}

		while (repr->children) {
			child = (SPRepr *) repr->children->data;
			sp_repr_remove_child (repr, child);
			sp_repr_unref (child);
			repr->children = g_list_remove (repr->children, child);
		}

		if (repr->attr) {
			g_hash_table_foreach (repr->attr, sp_repr_hash_del_value, repr);
			g_hash_table_destroy (repr->attr);
		}

#ifdef DEBUG_REPR
		num_repr--;
		g_print ("num_repr = %d\n", num_repr);
#endif

		g_free (repr);
	}

	return;
}

SPRepr * sp_repr_copy (SPRepr * repr)
{
	SPRepr * new, * child, * newchild;
	GList * list;
	gint n;

	g_return_val_if_fail (repr != NULL, NULL);

	new = sp_repr_new ();
	g_return_val_if_fail (new != NULL, NULL);

	new->name = repr->name;

	if (repr->content != NULL)
		new->content = g_strdup (repr->content);

	n = 0;
	for (list = repr->children; list != NULL; list = list->next) {
		child = (SPRepr *) list->data;
		newchild = sp_repr_copy (child);
		g_assert (newchild != NULL);
		sp_repr_add_child (new, newchild, n);
		n++;
	}

	g_hash_table_foreach (repr->attr, sp_repr_hash_copy, new->attr);

	return new;
}

void
sp_repr_set_name (SPRepr * repr, const gchar * name)
{
	g_assert (repr != NULL);
	g_assert (name != NULL);

	repr->name = g_quark_from_string (name);
}

const gchar *
sp_repr_name (SPRepr * repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return g_quark_to_string (repr->name);
}

void
sp_repr_set_content (SPRepr * repr, const gchar * content)
{
	g_assert (repr != NULL);

	if (repr->content)
		g_free (repr->content);

	repr->content = g_strdup (content);

	if (repr->content_changed)
		repr->content_changed (repr, repr->content_changed_data);

	return;
}

const gchar *
sp_repr_content (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->content;
}

void sp_repr_set_attr (SPRepr * repr, const gchar * key, const gchar * value)
{
	GQuark q;
	gchar * old_value;

	g_return_if_fail (repr != NULL);
	g_return_if_fail (key != NULL);

	q = g_quark_from_string (key);
	old_value = g_hash_table_lookup (repr->attr, GINT_TO_POINTER (q));

	if (value == NULL) {
		g_hash_table_remove (repr->attr, GINT_TO_POINTER (q));
	} else {
		g_hash_table_insert (repr->attr, GINT_TO_POINTER (q), g_strdup (value));
	}

	if (old_value)
		g_free (old_value);

	if (repr->attr_changed)
		repr->attr_changed (repr, key, repr->attr_changed_data);

	return;
}

const gchar * sp_repr_attr (SPRepr * repr, const gchar * key)
{
	GQuark q;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	q = g_quark_from_string (key);
	return g_hash_table_lookup (repr->attr, GINT_TO_POINTER (q));
}

void sp_repr_set_data (SPRepr * repr, gpointer data)
{
	g_assert (repr != NULL);

	repr->data = data;
}

gpointer sp_repr_data (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->data;
}

SPRepr *
sp_repr_parent (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->parent;
}

const GList *
sp_repr_children (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->children;
}

void sp_repr_add_child (SPRepr * repr, SPRepr * child, gint position)
{
	g_return_if_fail (repr != NULL);
	g_return_if_fail (child != NULL);
	g_return_if_fail (child->parent == NULL);
	g_return_if_fail (position >= 0);
	g_return_if_fail (position <= sp_repr_n_children (repr));

	repr->children = g_list_insert (repr->children, child, position);
	child->parent = repr;
	sp_repr_ref (child);

	if (repr->child_added)
		repr->child_added (repr, child, repr->child_added_data);
}

void sp_repr_remove_child (SPRepr * repr, SPRepr * child)
{
	g_return_if_fail (repr != NULL);
	g_return_if_fail (child != NULL);
	g_return_if_fail (child->parent == repr);

	if (child->unparented)
		child->unparented (child, child->unparented_data);

	repr->children = g_list_remove (repr->children, child);
	child->parent = NULL;
	sp_repr_unref (child);
}

void
sp_repr_set_signal (SPRepr * repr, const gchar * name, gpointer func, gpointer data)
{
	g_assert (repr != NULL);
	g_assert (name != NULL);

	if (strcmp (name, "destroy") == 0) {
		repr->destroy = (void (*)(SPRepr *, gpointer)) func;
		repr->destroy_data = data;
		return;
	}
	if (strcmp (name, "child_added") == 0) {
		repr->child_added = (void (*)(SPRepr *, SPRepr *, gpointer)) func;
		repr->child_added_data = data;
		return;
	}
	if (strcmp (name, "unparented") == 0) {
		repr->unparented = (void (*)(SPRepr *, gpointer)) func;
		repr->unparented_data = data;
		return;
	}
	if (strcmp (name, "attr_changed") == 0) {
		repr->attr_changed = (void (*)(SPRepr *, const gchar *, gpointer)) func;
		repr->attr_changed_data = data;
		return;
	}
	if (strcmp (name, "content_changed") == 0) {
		repr->content_changed = (void (*)(SPRepr *, gpointer)) func;
		repr->content_changed_data = data;
		return;
	}
	g_assert_not_reached ();
}

GList *
sp_repr_attributes (SPRepr * repr)
{
	GList * listptr;

	g_assert (repr != NULL);

	listptr = NULL;

	g_hash_table_foreach (repr->attr, sp_repr_attr_to_list, &listptr);

	return listptr;
}

/* Helpers */

static void
sp_repr_hash_del_value (gpointer key, gpointer value, gpointer user_data)
{
	g_free (value);
}

static void
sp_repr_attr_to_list (gpointer key, gpointer value, gpointer user_data)
{
	GList ** ptr;

	ptr = (GList **) user_data;

	* ptr = g_list_prepend (* ptr, g_quark_to_string (GPOINTER_TO_INT (key)));
}

static void
sp_repr_hash_copy (gpointer key, gpointer value, gpointer new_hash)
{
	g_hash_table_insert (new_hash, key, g_strdup (value));
}
