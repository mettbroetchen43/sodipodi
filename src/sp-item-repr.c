#define SP_ITEM_REPR_C

/*
 * item-repr.c
 *
 * constructor: sp_item_new_from_repr
 *
 */

#include "sp-item.h"
#include "sp-root.h"
#include "sp-image.h"
#include "sp-rect.h"
#include "sp-text.h"
#include "sp-ellipse.h"
/* This is for silly event handlers ;-( */
#include "event-broker.h"

#define noITEM_REPR_VERBOSE

static void sp_item_release_signals (SPItem * item);

static GHashTable * hash_table_setup (void);

GHashTable * htable = NULL;

/*
 * Item main constructor
 *
 */

SPItem *
sp_item_new_root (SPRepr * repr)
{
	SPItem * item;
	GtkType (* get_type) (void);
	gchar * name;

	g_return_val_if_fail (repr != NULL, NULL);

	if (htable == NULL) htable = hash_table_setup ();

	name = (gchar *) sp_repr_name (repr);
	g_return_val_if_fail (name != NULL, NULL);

	get_type = g_hash_table_lookup (htable, name);
	if (get_type == NULL) {
		g_print ("Cannot handle SVG object <%s>\n", name);
		return NULL;
	}

	item = (SPItem *) gtk_type_new (get_type ());
	g_return_val_if_fail (item != NULL, NULL);

	/* item initialization */
	item->repr = repr;
	item->parent = NULL;

	gtk_signal_connect (GTK_OBJECT (item), "destroy",
		GTK_SIGNAL_FUNC (sp_item_release_signals), NULL);

	sp_item_read (item, repr);

	return item;
}

SPItem * sp_item_new (SPRepr * repr, SPGroup * parent)
{
	SPItem * item;
	SPGroup * pg;
	GnomeCanvasGroup * cg;
	GSList * l;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (SP_IS_GROUP (parent), NULL);

	item = sp_item_new_root (repr);

	if (item == NULL) return NULL;

	pg = SP_GROUP (parent);

	item->parent = parent;
	pg->children = g_slist_append (pg->children, item);

	/* Silly, silly */
	for (l = SP_ITEM (parent)->display; l != NULL; l = l->next) {
		cg = GNOME_CANVAS_GROUP (l->data);
		sp_item_show (item, cg, sp_event_handler);
	}

	return item;

#if 0
/* fixme: do this right */
	/* We try to calculate z-order - correctly or not */
	siblings = (GList *) sp_repr_children (sp_item_repr (parent_item));
	pos = 0;
	for (l = siblings; l != NULL; l = l->next) {
		sibling = (SPRepr *) l->data;
		if (repr == sibling)
			break;
		if (sp_repr_item (sibling) != NULL)
			pos++;
	}
	/* We HAVE to be somewhere in the list */
	g_assert (l != NULL);
	sp_item_move_to_z (item, pos);
#endif
}

static GHashTable * hash_table_setup (void)
{
	GHashTable * table;

	table = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (table, "g", sp_group_get_type);
	g_hash_table_insert (table, "svg", sp_root_get_type);
	g_hash_table_insert (table, "path", sp_shape_get_type);
	g_hash_table_insert (table, "rect", sp_rect_get_type);
	g_hash_table_insert (table, "ellipse", sp_ellipse_get_type);
	g_hash_table_insert (table, "text", sp_text_get_type);
	g_hash_table_insert (table, "image", sp_image_get_type);

	return table;
}

void sp_item_print_info (SPItem * item, gint level)
{
#if 0
	GnomeCanvasItem * item;
	GnomeCanvasGroup * group;
	double affine[6];
	char c[128];

	item = GNOME_CANVAS_ITEM (item);
	group = GNOME_CANVAS_GROUP (item);

	gnome_canvas_item_i2p_affine (GNOME_CANVAS_ITEM (item), affine);
	art_affine_to_string (c, affine);

	g_print ("item: %p\n", item);
	g_print ("group: n_childs %d\n", g_list_length (group->item_list));
	g_print ("item: parent: %p affine: %s\n", item->parent, c);
#endif
}

static void
sp_item_release_signals (SPItem * item)
{
	sp_repr_remove_signals (item->repr);
}


