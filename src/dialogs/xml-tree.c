#define SP_XML_TREE_C

#include <gtk/gtk.h>
#include <glade/glade.h>
#include "../sp-object.h"
#include "../document.h"
#include "../mdi-document.h"
#include "xml-tree.h"

static GladeXML * xml;
static GtkWidget * dialog = NULL;

static void sp_xml_tree_set_contents (void);
static void sp_xml_tree_repr_subtree (GtkWidget * widget, SPRepr * repr);
void sp_xml_tree_close (GtkWidget * widget);
void sp_xml_tree_select (GtkItem * item, gpointer data);

void
sp_xml_tree_dialog (void)
{
	if (dialog == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "xmltree");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "xmltree");
	}

	gtk_widget_show (dialog);

	sp_xml_tree_set_contents ();
}

static void
sp_xml_tree_set_contents (void)
{
	GtkTree * tree;
	GList * children;
	GtkWidget * child;
	SPDocument * document;
	SPRepr * repr;

	document = SP_ACTIVE_DOCUMENT;
	if (document == NULL) return;
	g_assert (SP_IS_DOCUMENT (document));

	repr = SP_OBJECT (document->root)->repr;

	tree = GTK_TREE (glade_xml_get_widget (xml, "tree"));
	children = gtk_container_children (GTK_CONTAINER (tree));
	gtk_tree_remove_items (tree, children);

	child = gtk_tree_item_new_with_label (sp_repr_name (repr));
	gtk_signal_connect (GTK_OBJECT (child), "select",
		GTK_SIGNAL_FUNC (sp_xml_tree_select), repr);
	gtk_tree_append (tree, child);

	sp_xml_tree_repr_subtree (child, repr);

	gtk_widget_show_all (GTK_WIDGET (tree));
}

static void
sp_xml_tree_repr_subtree (GtkWidget * widget, SPRepr * repr)
{
	GtkWidget * subtree;
	GtkWidget * child;
	const GList * children, * l;

	children = sp_repr_children (repr);

	if (children != NULL) {
		subtree = gtk_tree_new ();
		gtk_tree_item_set_subtree (GTK_TREE_ITEM (widget), subtree);

		for (l = children; l != NULL; l = l->next) {
			child = gtk_tree_item_new_with_label (sp_repr_name ((SPRepr *) l->data));
			gtk_signal_connect (GTK_OBJECT (child), "select",
				GTK_SIGNAL_FUNC (sp_xml_tree_select), l->data);
			gtk_tree_item_collapse (GTK_TREE_ITEM (child));
			gtk_tree_append (GTK_TREE (subtree), child);
			sp_xml_tree_repr_subtree (child, (SPRepr *) l->data);
		}

		gtk_widget_show_all (GTK_WIDGET (subtree));
	}
}


void
sp_xml_tree_close (GtkWidget * widget)
{
	gtk_widget_hide (dialog);
}

void
sp_xml_tree_select (GtkItem * item, gpointer data)
{
	GtkCList * clist;
	GtkText * text;
	GList * attr;
	gchar * rowtext[2];
	const gchar * content;
	gint pos;

	clist = (GtkCList *) glade_xml_get_widget (xml, "attributes");
	g_return_if_fail (GTK_IS_CLIST (clist));

	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);

	attr = sp_repr_attributes ((SPRepr *) data);

	while (attr) {
		rowtext[0] = (gchar *) attr->data;
		rowtext[1] = (gchar *) sp_repr_attr ((SPRepr *) data, rowtext[0]);
		gtk_clist_append (clist, rowtext);
		attr = g_list_remove (attr, attr->data);
	}

	gtk_clist_thaw (clist);

	text = (GtkText *) glade_xml_get_widget (xml, "content");
	g_return_if_fail (GTK_IS_TEXT (text));

	gtk_editable_delete_text (GTK_EDITABLE (text), 0, -1);

	content = sp_repr_content ((SPRepr *) data);

	if (content != NULL) {
		pos = 0;
		gtk_editable_insert_text (GTK_EDITABLE (text), content, strlen (content), &pos);
	}
}


