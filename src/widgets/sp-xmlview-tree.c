#define __SP_XMLVIEW_TREE_C__

/*
 * Specialization of GtkCTree for the XML tree view
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2002 MenTaLguY
 *
 * Released under the GNU GPL; see COPYING for details
 */

#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkctree.h>
#include <gtk/gtkcontainer.h>
/*#include "xml/repr-private.h" */

#include "sp-xmlview-tree.h"

typedef struct _NodeData {
	SPXMLViewTree * tree;
	GtkCTreeNode * node;
	SPRepr * repr;
} NodeData;

#define NODE_DATA(node) ((NodeData *)(GTK_CTREE_ROW ((node))->row.data))

static void sp_xmlview_tree_class_init (SPXMLViewTreeClass * klass);
static void sp_xmlview_tree_init (SPXMLViewTree * tree);
static void sp_xmlview_tree_destroy (GtkObject * object);

static NodeData * node_data_new (SPXMLViewTree * tree, GtkCTreeNode * node, SPRepr * repr);
static void node_data_free (gpointer data);

static GtkCTreeNode * add_node (SPXMLViewTree * tree, GtkCTreeNode * parent, GtkCTreeNode * before, SPRepr * repr);

static void element_child_added (SPRepr * repr, SPRepr * child, SPRepr * ref, gpointer data);
static void element_attr_changed (SPRepr * repr, const guchar * key, const guchar * old_value, const guchar * new_value, gpointer data);
static void element_child_removed (SPRepr * repr, SPRepr * child, SPRepr * ref, gpointer data);
static void element_order_changed (SPRepr * repr, SPRepr * child, SPRepr * oldref, SPRepr * newref, gpointer data);

static void text_content_changed (SPRepr * repr, const guchar * old_content, const guchar * new_content, gpointer data);

static void tree_move (GtkCTree * tree, GtkCTreeNode * node, GtkCTreeNode * new_parent, GtkCTreeNode * new_sibling);

static gboolean check_drag (GtkCTree * tree, GtkCTreeNode * node, GtkCTreeNode * new_parent, GtkCTreeNode * new_sibling);

static GtkCTreeNode * ref_to_sibling (GtkCTreeNode * parent, SPRepr * ref);
static GtkCTreeNode * repr_to_child (GtkCTreeNode * parent, SPRepr * repr);
static SPRepr * sibling_to_ref (GtkCTreeNode * parent, GtkCTreeNode * sibling);

static gint match_node_data_by_repr(gconstpointer data_p, gconstpointer repr);

static const SPReprEventVector element_repr_events = {
        NULL, /* destroy */
        NULL, /* add_child */
        element_child_added,
        NULL, /* remove_child */
        element_child_removed,
        NULL, /* change_attr */
        element_attr_changed,
        NULL, /* change_content */
        NULL, /* content_changed */
        NULL, /* change_order */
        element_order_changed
};

static const SPReprEventVector text_repr_events = {
        NULL, /* destroy */
        NULL, /* add_child */
        NULL, /* child_added */
        NULL, /* remove_child */
        NULL, /* child_removed */
        NULL, /* change_attr */
        NULL, /* attr_changed */
        NULL, /* change_content */
        text_content_changed,
        NULL, /* change_order */
        NULL  /* order_changed */
};

static GtkCTreeClass * parent_class = NULL;

GtkWidget *
sp_xmlview_tree_new (SPRepr * repr, void * factory, void * data)
{
	SPXMLViewTree * tree;

	tree = g_object_new (SP_TYPE_XMLVIEW_TREE, "n_columns", 1, "tree_column", 0, NULL);

	gtk_clist_column_titles_hide (GTK_CLIST (tree));
	gtk_ctree_set_line_style (GTK_CTREE (tree), GTK_CTREE_LINES_NONE);
	gtk_ctree_set_expander_style (GTK_CTREE (tree), GTK_CTREE_EXPANDER_TRIANGLE);
	gtk_clist_set_column_auto_resize (GTK_CLIST (tree), 0, TRUE);
	gtk_clist_set_reorderable (GTK_CLIST (tree), TRUE);
	gtk_ctree_set_drag_compare_func (GTK_CTREE (tree), check_drag);

	sp_xmlview_tree_set_repr (tree, repr);

	return (GtkWidget *) tree;
}

void
sp_xmlview_tree_set_repr (SPXMLViewTree * tree, SPRepr * repr)
{
	if ( tree->repr == repr ) return;
	gtk_clist_freeze (GTK_CLIST (tree));
	if (tree->repr) {
		gtk_clist_clear (GTK_CLIST (tree));
		sp_repr_unref (tree->repr);
	}
	tree->repr = repr;
	if (repr) {
		GtkCTreeNode * node;
		sp_repr_ref (repr);
		node = add_node (tree, NULL, NULL, repr);
		gtk_ctree_expand (GTK_CTREE (tree), node);
	}
	gtk_clist_thaw (GTK_CLIST (tree));
}

GtkType
sp_xmlview_tree_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPXMLViewTree",
			sizeof (SPXMLViewTree),
			sizeof (SPXMLViewTreeClass),
			(GtkClassInitFunc) sp_xmlview_tree_class_init,
			(GtkObjectInitFunc) sp_xmlview_tree_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_CTREE, &info);
	}

	return type;
}

void
sp_xmlview_tree_class_init (SPXMLViewTreeClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;
	parent_class = (GtkCTreeClass *) gtk_type_class (GTK_TYPE_CTREE);

	GTK_CTREE_CLASS (object_class)->tree_move = tree_move;

	object_class->destroy = sp_xmlview_tree_destroy;
}

void
sp_xmlview_tree_init (SPXMLViewTree * tree)
{
	tree->repr = NULL;
	tree->blocked = 0;
}

void
sp_xmlview_tree_destroy (GtkObject * object)
{
	SPXMLViewTree * tree;

	tree = SP_XMLVIEW_TREE (object);

	sp_xmlview_tree_set_repr (tree, NULL);

	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkCTreeNode *
add_node (SPXMLViewTree * tree, GtkCTreeNode * parent, GtkCTreeNode * before, SPRepr * repr)
{
	NodeData * data;
	GtkCTreeNode * node;
	const SPReprEventVector * vec;
	static const gchar *default_text[] = { "???" };

	g_assert (tree != NULL);
	g_assert (repr != NULL);

	node = gtk_ctree_insert_node (GTK_CTREE (tree), parent, before, (gchar **)default_text, 2,
						NULL, NULL, NULL, NULL, !sp_repr_is_element (repr), FALSE);
	g_assert (node != NULL);

	data = node_data_new (tree, node, repr);
	g_assert (data != NULL);

	gtk_ctree_node_set_row_data_full (GTK_CTREE (tree), data->node, data, node_data_free);

	if (sp_repr_is_text (repr) || sp_repr_is_cdata (repr)) {
		gtk_clist_freeze (GTK_CLIST (tree));
		sp_repr_add_listener (repr, &text_repr_events, data);
		/* Emulate content_changed event */
		text_content_changed (repr, NULL, sp_repr_get_content (repr), data);
		gtk_clist_thaw (GTK_CLIST (tree));
	} else if (sp_repr_is_element (repr)) {
		SPRepr *child, *ref;
		SPReprAttr *attr;
		gtk_clist_freeze (GTK_CLIST (tree));
		element_attr_changed (repr, "id", NULL, NULL, data);
		sp_repr_add_listener (repr, &element_repr_events, data);
		/* Emulate events */
		attr = sp_repr_attr_get_first (repr);
		while (attr) {
			element_attr_changed (repr, sp_repr_attr_get_key (repr, attr), NULL, sp_repr_attr_get_value (repr, attr), data);
			attr = sp_repr_attr_get_next (repr, attr);
		}
		ref = NULL;
		child = sp_repr_get_children (repr);
		while (child) {
			element_child_added (repr, child, ref, data);
			ref = child;
			child = sp_repr_get_next (child);
		}
		gtk_clist_thaw (GTK_CLIST (tree));
	} else {
		vec = NULL;
	}

	return node;
}

NodeData *
node_data_new (SPXMLViewTree * tree, GtkCTreeNode * node, SPRepr * repr)
{
	NodeData * data;
	data = g_new (NodeData, 1);
	data->tree = tree;
	data->node = node;
	data->repr = repr;
	sp_repr_ref (repr);
	return data;
}

void
node_data_free (gpointer ptr) {
	NodeData * data;
	data = (NodeData *) ptr;
	sp_repr_remove_listener_by_data (data->repr, data);
	g_assert (data->repr != NULL);
	sp_repr_unref (data->repr);
	g_free (data);
}

void
element_child_added (SPRepr * repr, SPRepr * child, SPRepr * ref, gpointer ptr)
{
	NodeData * data;
	GtkCTreeNode * before;

	data = (NodeData *) ptr;

	if (data->tree->blocked) return;

	before = ref_to_sibling (data->node, ref);

	add_node (data->tree, data->node, before, child);
}

void
element_attr_changed (SPRepr * repr, const guchar * key, const guchar * old_value, const guchar *new_value, gpointer ptr)
{
	NodeData *data;
	gchar *label;

	data = (NodeData *) ptr;

	if (data->tree->blocked) return;

	if (strcmp (key, "id")) return;
	
	if (new_value) {
		label = g_strdup_printf ("<%s id=\"%s\">", sp_repr_get_name (repr), new_value);
	} else {
		label = g_strdup_printf ("<%s>", sp_repr_get_name (repr));
	}
	gtk_ctree_node_set_text (GTK_CTREE (data->tree), data->node, 0, label);
	g_free (label);
}

void
element_child_removed (SPRepr * repr, SPRepr * child, SPRepr * ref, gpointer ptr)
{
	NodeData * data;

	data = (NodeData *) ptr;

	if (data->tree->blocked) return;

	gtk_ctree_remove_node (GTK_CTREE (data->tree), repr_to_child (data->node, child));
}

void
element_order_changed (SPRepr * repr, SPRepr * child, SPRepr * oldref, SPRepr * newref, gpointer ptr)
{
	NodeData * data;
	GtkCTreeNode * before, * node;

	data = (NodeData *) ptr;

	if (data->tree->blocked) return;

	before = ref_to_sibling (data->node, newref);
	node = repr_to_child (data->node, child);

	if ( before == node ) before = GTK_CTREE_ROW (before)->sibling;

	parent_class->tree_move (GTK_CTREE (data->tree), node, data->node, before);
}

void
text_content_changed (SPRepr *repr, const guchar * old_content, const guchar * new_content, gpointer ptr)
{
	NodeData *data;
	unsigned char c[64];
	unsigned char *label;
	int i;

	data = (NodeData *) ptr;

	if (data->tree->blocked) return;

	if (!new_content) new_content = "";
	if (strlen (new_content) < 64) {
		strcpy (c, new_content);
	} else {
		strncpy (c, new_content, 32);
		strncpy (c + 32, " ... ", 5);
		strncpy (c + 37, new_content + strlen (new_content) - 26, 26);
		c[63] = 0;
	}
	for (i = 0; c[i]; i++) if (c[i] < ' ') c[i] = ' ';

	if (sp_repr_is_text (repr)) {
		label = g_strdup_printf ("\"%s\"", c);
	} else {
		label = g_strdup_printf ("<![CDATA[%s]]>", c);
	}
	gtk_ctree_node_set_text (GTK_CTREE (data->tree), data->node, 0, label);
	g_free (label);
}

void
tree_move (GtkCTree * tree, GtkCTreeNode * node, GtkCTreeNode * new_parent, GtkCTreeNode * new_sibling)
{
	GtkCTreeNode * old_parent;
	SPRepr * ref;
	int ok;

	old_parent = GTK_CTREE_ROW (node)->parent;
	if ( !old_parent || !new_parent ) return;

	ref = sibling_to_ref (new_parent, new_sibling);

	gtk_clist_freeze (GTK_CLIST (tree));

	SP_XMLVIEW_TREE (tree)->blocked++;
	if (new_parent == old_parent) {
		ok = sp_repr_change_order (NODE_DATA (old_parent)->repr, NODE_DATA (node)->repr, ref);
	} else {
		ok = sp_repr_remove_child (NODE_DATA (old_parent)->repr, NODE_DATA (node)->repr) &&
		     sp_repr_add_child (NODE_DATA (new_parent)->repr, NODE_DATA (node)->repr, ref);
	}
	SP_XMLVIEW_TREE (tree)->blocked--;

	if (ok) {
		parent_class->tree_move (tree, node, new_parent, new_sibling);
	}

	gtk_clist_thaw (GTK_CLIST (tree));
}

GtkCTreeNode *
ref_to_sibling (GtkCTreeNode * parent, SPRepr * ref)
{
	if (ref) {
		GtkCTreeNode * before;
		before = repr_to_child (parent, ref); 
		g_assert (before != NULL);
		before = GTK_CTREE_ROW (before)->sibling;
		return before;
	} else {
		return GTK_CTREE_ROW (parent)->children;
	}
}

GtkCTreeNode *
repr_to_child (GtkCTreeNode * parent, SPRepr * repr)
{
	GtkCTreeNode * child;
	child = GTK_CTREE_ROW (parent)->children;
	while ( child && NODE_DATA (child)->repr != repr ) {
		child = GTK_CTREE_ROW (child)->sibling;
	}
	return child;
}

SPRepr *
sibling_to_ref (GtkCTreeNode * parent, GtkCTreeNode * sibling)
{
	GtkCTreeNode * child;
	child = GTK_CTREE_ROW (parent)->children;
	if ( child == sibling ) return NULL;
	while ( child && GTK_CTREE_ROW (child)->sibling != sibling ) {
		child = GTK_CTREE_ROW (child)->sibling;
	}
	return NODE_DATA (child)->repr;
}

gboolean
check_drag (GtkCTree * tree, GtkCTreeNode * node, GtkCTreeNode * new_parent, GtkCTreeNode * new_sibling)
{
	GtkCTreeNode * old_parent;

	old_parent = GTK_CTREE_ROW (node)->parent;

	if (!old_parent || !new_parent) return FALSE;
	if (!sp_repr_is_element (NODE_DATA (new_parent)->repr)) return FALSE;

	/* fixme: we need add_child/remove_child/etc repr events without side-effects, so we can check here and give better visual feedback */

	return TRUE;
}

SPRepr *
sp_xmlview_tree_node_get_repr (SPXMLViewTree * tree, GtkCTreeNode * node)
{
	return NODE_DATA (node)->repr;
}

GtkCTreeNode *
sp_xmlview_tree_get_repr_node (SPXMLViewTree * tree, SPRepr * repr)
{
	return gtk_ctree_find_by_row_data_custom (GTK_CTREE (tree), NULL, repr, match_node_data_by_repr);
}

gint
match_node_data_by_repr(gconstpointer data_p, gconstpointer repr)
{
	return ((const NodeData *)data_p)->repr != (const SPRepr *)repr;
}

