#define SP_XML_TREE_C

#include <gnome.h>
#include <glade/glade.h>
#include "../sp-object.h"
#include "../document.h"
#include "../mdi-document.h"
#include "xml-tree.h"

static GladeXML * xml;
static GtkWidget * dialog = NULL;
static GtkTree * tree = NULL;
static GtkCList * attributes = NULL;
static GtkText * content = NULL;
static SPRepr * selected_repr = NULL;
static gint selected_row = -1;

static void sp_xml_tree_set_contents (SPRepr * repr);
static void sp_xml_tree_repr_subtree (GtkWidget * widget, SPRepr * repr);
void sp_xml_tree_close (GtkWidget * widget);
void sp_xml_tree_select (GtkItem * item, gpointer data);
void sp_xml_tree_deselect (GtkItem * item, gpointer data);
void sp_xml_tree_select_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data);
void sp_xml_tree_unselect_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data);
void sp_xml_tree_delete_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_add_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_change_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_change_attribute_long (SPRepr * repr, const gchar * attr, const gchar * value);

void
sp_xml_tree_dialog (void)
{
	SPDocument * document;

	document = SP_ACTIVE_DOCUMENT;
	if (document == NULL) return;
	g_assert (SP_IS_DOCUMENT (document));

	if (dialog == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "xmltree");
		glade_xml_signal_autoconnect (xml);
		dialog = glade_xml_get_widget (xml, "xmltree");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		tree = (GtkTree *) glade_xml_get_widget (xml, "tree");
		g_assert (tree != NULL);
		g_assert (GTK_IS_TREE (tree));
		attributes = (GtkCList *) glade_xml_get_widget (xml, "attributes");
		g_assert (attributes != NULL);
		g_assert (GTK_IS_CLIST (attributes));
		content = (GtkText *) glade_xml_get_widget (xml, "content");
		g_assert (content != NULL);
		g_assert (GTK_IS_TEXT (content));
	}

	sp_xml_tree_set_contents (SP_OBJECT (document->root)->repr);

	gtk_widget_show (dialog);
}

static void
sp_xml_tree_set_contents (SPRepr * repr)
{
	GList * children;
	GtkWidget * root;

	g_assert (repr != NULL);

	/* Clear all containers */

	children = gtk_container_children (GTK_CONTAINER (tree));
	gtk_tree_remove_items (tree, children);
	gtk_clist_clear (attributes);
	gtk_editable_delete_text (GTK_EDITABLE (content), 0, -1);

	selected_repr = NULL;
	selected_row = -1;

	root = gtk_tree_item_new_with_label (sp_repr_name (repr));
	gtk_signal_connect (GTK_OBJECT (root), "select",
		GTK_SIGNAL_FUNC (sp_xml_tree_select), repr);
	gtk_signal_connect (GTK_OBJECT (root), "deselect",
		GTK_SIGNAL_FUNC (sp_xml_tree_deselect), repr);
	gtk_tree_append (tree, root);

	sp_xml_tree_repr_subtree (root, repr);

	gtk_widget_show_all (GTK_WIDGET (tree));
}

static void
sp_xml_tree_repr_subtree (GtkWidget * item, SPRepr * repr)
{
	GtkWidget * subtree;
	GtkWidget * child;
	const GList * children, * l;

	children = sp_repr_children (repr);

	if (children != NULL) {
		subtree = gtk_tree_new ();
		gtk_tree_item_set_subtree (GTK_TREE_ITEM (item), subtree);

		for (l = children; l != NULL; l = l->next) {
			child = gtk_tree_item_new_with_label (sp_repr_name ((SPRepr *) l->data));
			gtk_signal_connect (GTK_OBJECT (child), "select",
				GTK_SIGNAL_FUNC (sp_xml_tree_select), l->data);
			gtk_signal_connect (GTK_OBJECT (child), "deselect",
				GTK_SIGNAL_FUNC (sp_xml_tree_deselect), l->data);
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
	GList * attr;
	gchar * rowtext[2];
	const gchar * text;
	gint row, pos;

	selected_repr = (SPRepr *) data;

	gtk_clist_freeze (attributes);

	gtk_clist_clear (attributes);

	attr = sp_repr_attributes ((SPRepr *) data);

	while (attr) {
		rowtext[0] = (gchar *) attr->data;
		rowtext[1] = (gchar *) sp_repr_attr ((SPRepr *) data, rowtext[0]);
		row = gtk_clist_append (attributes, rowtext);
		gtk_clist_set_row_data (attributes, row, rowtext[0]);
		attr = g_list_remove (attr, attr->data);
	}

	gtk_clist_thaw (attributes);

	gtk_editable_delete_text (GTK_EDITABLE (content), 0, -1);

	text = sp_repr_content ((SPRepr *) data);

	if (text != NULL) {
		pos = 0;
		gtk_editable_insert_text (GTK_EDITABLE (content), text, strlen (text), &pos);
	}
}

void
sp_xml_tree_deselect (GtkItem * item, gpointer data)
{
	if (selected_repr == (SPRepr *) data) selected_repr = NULL;
}

void
sp_xml_tree_select_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data)
{
	if (selected_repr == NULL) return;

	selected_row = row;
}

void
sp_xml_tree_unselect_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data)
{
	if (selected_row == row) selected_row = -1;
}

void
sp_xml_tree_delete_attribute (GtkWidget * widget, gpointer data)
{
	static GtkWidget * dialog = NULL;
	const gchar * attr;
	gchar * message;
	gint b;

	if (selected_repr == NULL) return;
	if (selected_row < 0) return;

	attr = gtk_clist_get_row_data (attributes, selected_row);

	message = g_strdup_printf ("Do you want to delete attribute %s?", attr);

	dialog = gnome_message_box_new (message,
		GNOME_MESSAGE_BOX_QUESTION,
		"Yes",
		"No",
		NULL);

	b = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

	g_free (message);

	if (b == 0) {
		if (sp_repr_set_attr (selected_repr, attr, NULL)) {
			gtk_clist_remove (attributes, selected_row);
			selected_row = -1;
		}
		sp_document_done (SP_ACTIVE_DOCUMENT);
	}
}

void
sp_xml_tree_add_attribute (GtkWidget * widget, gpointer data)
{
	g_warning ("xml_tree_add_attribute is unimplemented");
}

/*
 * GtkEntry does not like very long texts, so we have separate
 * version for loooong attributes (paths & similar)
 */

void
sp_xml_tree_change_attribute (GtkWidget * widget, gpointer data)
{
	static GladeXML * xml = NULL;
	static GtkWidget * dialog, * label, * entry;
	const gchar * attr, * value;
	gchar * newval;
	gint button;

	if (selected_repr == NULL) return;
	if (selected_row < 0) return;

	attr = gtk_clist_get_row_data (attributes, selected_row);
	value = sp_repr_attr (selected_repr, attr);

	if (strlen (value) > 32) {
		sp_xml_tree_change_attribute_long (selected_repr, attr, value);
		return;
	}

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "change_attribute");
		g_assert (xml != NULL);
		dialog = glade_xml_get_widget (xml, "change_attribute");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		label = glade_xml_get_widget (xml, "change_attr_attribute");
		g_assert (label != NULL);
		g_assert (GTK_IS_LABEL (label));
		entry = glade_xml_get_widget (xml, "change_attr_value");
		g_assert (entry != NULL);
		g_assert (GTK_IS_ENTRY (entry));
	}

	gtk_label_set_text (GTK_LABEL (label), attr);
	gtk_entry_set_text (GTK_ENTRY (entry), value);

	button = gnome_dialog_run (GNOME_DIALOG (dialog));

	if (button == 0) {
		newval = gtk_entry_get_text (GTK_ENTRY (entry));
		if (sp_repr_set_attr (selected_repr, attr, newval)) {
			gtk_clist_set_text (attributes, selected_row, 1, newval);
		}
		sp_document_done (SP_ACTIVE_DOCUMENT);
	}

	gtk_widget_hide (GTK_WIDGET (dialog));
}

void
sp_xml_tree_change_attribute_long (SPRepr * repr, const gchar * attr, const gchar * value)
{
	static GladeXML * xml = NULL;
	static GtkWidget * dialog, * label, * text;
	gchar * newval;
	gint pos, button;

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "change_attribute_long");
		g_assert (xml != NULL);
		dialog = glade_xml_get_widget (xml, "change_attribute_long");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		label = glade_xml_get_widget (xml, "change_attr_attribute");
		g_assert (label != NULL);
		g_assert (GTK_IS_LABEL (label));
		text = glade_xml_get_widget (xml, "change_attr_value");
		g_assert (text != NULL);
		g_assert (GTK_IS_TEXT (text));
	}

	gtk_label_set_text (GTK_LABEL (label), attr);
	gtk_editable_delete_text (GTK_EDITABLE (text), 0, -1);
	pos = 0;
	gtk_editable_insert_text (GTK_EDITABLE (text),
		value,
		strlen (value),
		&pos);

	button = gnome_dialog_run (GNOME_DIALOG (dialog));

	if (button == 0) {
		newval = gtk_editable_get_chars (GTK_EDITABLE (text), 0, -1);
		if (sp_repr_set_attr (repr, attr, newval)) {
			gtk_clist_set_text (attributes, selected_row, 1, newval);
		}
		sp_document_done (SP_ACTIVE_DOCUMENT);
		g_free (newval);
	}

	gtk_widget_hide (GTK_WIDGET (dialog));
}

