#define SP_XML_TREE_C

#include <gnome.h>
#include <glade/glade.h>
#include "../xml/repr-private.h"
#include "../sodipodi.h"
#include "../document.h"
#include "../sp-object.h"
#include "xml-tree.h"
#include "../desktop.h"
#include "../desktop-handles.h"
#include "../selection-chemistry.h"

static GladeXML * xml;
static GtkWidget * dialog = NULL;
static GtkTree * tree = NULL;
static GtkCList * attributes = NULL;
static GtkText * content = NULL;
static SPRepr * selected_repr = NULL;
static SPRepr * clipboard_repr = NULL;
static gint selected_row = -1;
/* vieport adjustment counters */
static gint all_items, s_item;

static void sp_xml_tree_set_contents (SPRepr * repr);
SPRepr * sp_xml_tree_get_dt_select (void);
void sp_xml_tree_set_dt_select (SPRepr * repr);
void sp_xml_tree_set_vadjustment (void);
static void sp_xml_tree_repr_subtree (GtkWidget * widget, SPRepr * repr, const guchar *key);
void sp_xml_tree_close (GtkWidget * widget);
void sp_xml_tree_select (GtkItem * item, gpointer data);
void sp_xml_tree_deselect (GtkItem * item, gpointer data);
void sp_xml_tree_select_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data);
void sp_xml_tree_unselect_attribute (GtkCList * clist, gint row, gint column, GdkEvent * event, gpointer data);
void sp_xml_tree_delete_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_add_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_change_attribute (GtkWidget * widget, gpointer data);
void sp_xml_tree_change_attribute_long (SPRepr * repr, const gchar * attr, const gchar * value);
void sp_xml_tree_change_content (GtkWidget * widget, gpointer data);
void sp_xml_tree_remove_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_new_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_up_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_down_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_dup_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_copy_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_cut_element (GtkWidget * widget, gpointer data);
void sp_xml_tree_paste_element (GtkWidget * widget, gpointer data);

void
sp_xml_tree_dialog (void)
{
	SPDocument * document;

	document = SP_ACTIVE_DOCUMENT;
	if (document == NULL) return;
	g_assert (SP_IS_DOCUMENT (document));

	selected_repr = NULL;
	if (clipboard_repr) {
		sp_repr_unref (clipboard_repr);
		clipboard_repr = NULL;
	}
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

	sp_xml_tree_set_contents (sp_document_repr_root (document));

	gtk_widget_show (dialog);
}

static void
sp_xml_tree_set_contents (SPRepr * repr)
{
	GList * children;
	GtkWidget * root;
	const guchar *key;

	g_assert (repr != NULL);

	/* Clear all containers */

	children = gtk_container_children (GTK_CONTAINER (tree));
	gtk_tree_remove_items (tree, children);
	gtk_clist_clear (attributes);
	gtk_editable_delete_text (GTK_EDITABLE (content), 0, -1);
	
	/* What object is selected on desktop ? */
	if (!selected_repr)
		selected_repr = sp_xml_tree_get_dt_select();
	if (selected_repr) 
		key = sp_repr_attr(selected_repr, (guchar *)"id");
	else
		key = NULL;
	selected_row = -1;

	root = gtk_tree_item_new_with_label (sp_repr_name (repr));
	gtk_signal_connect (GTK_OBJECT (root), "select",
		GTK_SIGNAL_FUNC (sp_xml_tree_select), repr);
	gtk_signal_connect (GTK_OBJECT (root), "deselect",
		GTK_SIGNAL_FUNC (sp_xml_tree_deselect), repr);
	gtk_tree_append (tree, root);

	all_items = 1;
	s_item = -1;
	sp_xml_tree_repr_subtree (root, repr, key);

	sp_xml_tree_set_vadjustment();

	gtk_widget_show_all (GTK_WIDGET (tree));
}

SPRepr *
sp_xml_tree_get_dt_select (void)
{
	SPDesktop *desktop;
	SPSelection *selection;
	SPObject *object;

	desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP (desktop)) return NULL;
	selection = SP_DT_SELECTION (desktop);
	if sp_selection_is_empty (selection) return NULL;
	object = SP_OBJECT (selection->items->data);
	if (!object) return NULL;
	return SP_OBJECT_REPR (object);
}

void
sp_xml_tree_set_dt_select (SPRepr *repr)
{
	SPDesktop *desktop;
	SPSelection *selection;
	SPObject *object;
	const gchar *id;
	
	desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP (desktop)) return;
	selection = SP_DT_SELECTION (desktop);
	id = sp_repr_attr(repr, "id");
	object = (id) ? sp_document_lookup_id (SP_DT_DOCUMENT (desktop), id) : NULL;
	/* it's ok for objects from other namespaces */
	if (object == NULL) return;
	if (SP_IS_ITEM (object))
	sp_selection_set_item (selection, SP_ITEM (object));
}
	

void
sp_xml_tree_set_vadjustment (void) 
{
	GtkAdjustment *vadj;
	gfloat newv;
	
	if (s_item < 0)
		return;
	vadj = gtk_viewport_get_vadjustment (
			GTK_VIEWPORT((GTK_WIDGET(tree))->parent));
	/* Why '15', you ask? If you know how, do it better. */
	if (s_item * 15 < vadj->value) {
		gtk_adjustment_set_value (vadj, s_item * 15);
	} else if ((s_item * 15) + 15 > (vadj->value + vadj->page_size)) {
		newv = s_item * 15 + 15 - vadj->page_size;
		vadj->lower = 0;
		vadj->value = newv;
		vadj->upper = newv + vadj->page_size;
		gtk_viewport_set_vadjustment (
				GTK_VIEWPORT((GTK_WIDGET(tree))->parent),
				vadj);
		gtk_adjustment_set_value (vadj, newv);
	}
	/* How to force tree to refresh ? */
}


static void
sp_xml_tree_repr_subtree (GtkWidget * item, SPRepr * repr, const guchar *key)
{
	GtkWidget * subtree;
	GtkWidget * child;
	int i = 0;
	const guchar *act_key;

	if (repr->children) {
		SPRepr * crepr;

		subtree = gtk_tree_new ();
		gtk_tree_item_set_subtree (GTK_TREE_ITEM (item), subtree);

		for (crepr = repr->children; crepr != NULL; crepr = crepr->next, i++) {
			child = gtk_tree_item_new_with_label (sp_repr_name (crepr));
			gtk_signal_connect (GTK_OBJECT (child), "select",
				GTK_SIGNAL_FUNC (sp_xml_tree_select), crepr);
			gtk_signal_connect (GTK_OBJECT (child), "deselect",
				GTK_SIGNAL_FUNC (sp_xml_tree_deselect), crepr);
			gtk_tree_item_collapse (GTK_TREE_ITEM (child));
			gtk_tree_append (GTK_TREE (subtree), child);
			/* set selection to object selected on desktop */
			if (key) {
				act_key = sp_repr_attr(crepr, (guchar *)"id");
				if (act_key &&(!strcmp(act_key, key))) {
					gtk_tree_select_item((GtkTree *)subtree, i);
					s_item = all_items;
				}
			}
			all_items ++;
			sp_xml_tree_repr_subtree (child, crepr, key);
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
	SPReprAttr * attr;
	gchar * rowtext[2];
	const gchar * text;
	gint row, pos;

	selected_repr = (SPRepr *) data;

	gtk_clist_freeze (attributes);

	gtk_clist_clear (attributes);

	for (attr = selected_repr->attributes; attr != NULL; attr = attr->next) {
		rowtext[0] = (gchar *) SP_REPR_ATTRIBUTE_KEY (attr);
		rowtext[1] = (gchar *) SP_REPR_ATTRIBUTE_VALUE (attr);
		row = gtk_clist_append (attributes, rowtext);
		gtk_clist_set_row_data (attributes, row, rowtext[0]);
	}

	gtk_clist_thaw (attributes);

	gtk_editable_delete_text (GTK_EDITABLE (content), 0, -1);

	text = sp_repr_content ((SPRepr *) data);

	if (text != NULL) {
		pos = 0;
		gtk_editable_insert_text (GTK_EDITABLE (content), text, strlen (text), &pos);
	}

	if (selected_repr) 
		sp_xml_tree_set_dt_select(selected_repr);
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

	if (attr && (!strcmp(attr, "id"))) {
		dialog = gnome_message_box_new(
				_("You can't delete 'id' attribute!"),
				GNOME_MESSAGE_BOX_INFO,
				GNOME_STOCK_BUTTON_OK,
				NULL);
		gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
		return;
	}
	message = g_strdup_printf (_("Do you want to delete attribute %s?"), 
			attr);

	dialog = gnome_message_box_new (message,
		GNOME_MESSAGE_BOX_QUESTION,
		_("Yes"),
		_("No"),
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
	static GladeXML * xml = NULL;
	static GtkWidget * dialog, * attr_entry, * val_entry;
	gchar * attr, * value;
	gint button, row;
	gchar * rowtext[2];

	if (selected_repr == NULL) return;

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "add_attribute");
		g_assert (xml != NULL);
		dialog = glade_xml_get_widget (xml, "add_attribute");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		attr_entry = glade_xml_get_widget (xml, "attribute");
		g_assert (attr_entry != NULL);
		g_assert (GTK_IS_ENTRY (attr_entry));
		val_entry = glade_xml_get_widget (xml, "value");
		g_assert (val_entry != NULL);
		g_assert (GTK_IS_ENTRY (val_entry));
	}

	button = gnome_dialog_run (GNOME_DIALOG (dialog));

	if (button == 0) {
		attr = gtk_entry_get_text (GTK_ENTRY (attr_entry));
		value = gtk_entry_get_text (GTK_ENTRY (val_entry));
		if (attr && value && (strlen (attr) > 0) && 
				(strlen (value) > 0))
			if (sp_repr_set_attr (selected_repr, attr, value)) {
				rowtext[0] = attr;
				rowtext[1] = value;
				row = gtk_clist_append (attributes, rowtext);
				gtk_clist_set_row_data (attributes, row, rowtext[0]);
		}
		sp_document_done (SP_ACTIVE_DOCUMENT);
	}

	gtk_widget_hide (GTK_WIDGET (dialog));
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

void
sp_xml_tree_change_content (GtkWidget * widget, gpointer data)
{
	static GladeXML * xml = NULL;
	static GtkWidget * dialog, * label, * text;
	gchar * newcontent;
	gint pos, button;

	if (!selected_repr) return;
	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "change_content");
		g_assert (xml != NULL);
		dialog = glade_xml_get_widget (xml, "change_content");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		label = glade_xml_get_widget (xml, "name");
		g_assert (label != NULL);
		g_assert (GTK_IS_LABEL (label));
		text = glade_xml_get_widget (xml, "content");
		g_assert (text != NULL);
		g_assert (GTK_IS_TEXT (text));
	}

	gtk_label_set_text (GTK_LABEL (label), SP_REPR_NAME(selected_repr));
	gtk_editable_delete_text (GTK_EDITABLE (text), 0, -1);
	pos = 0;
	if (SP_REPR_CONTENT (selected_repr))
		gtk_editable_insert_text (GTK_EDITABLE (text),
			SP_REPR_CONTENT (selected_repr),
			strlen (SP_REPR_CONTENT (selected_repr)),
			&pos);

	button = gnome_dialog_run (GNOME_DIALOG (dialog));

	if (button == 0) {
		newcontent = gtk_editable_get_chars (GTK_EDITABLE (text), 0, -1);
		if (sp_repr_set_content(selected_repr, newcontent)) {
			gtk_editable_insert_text (GTK_EDITABLE (content), 
					newcontent, 
					strlen (newcontent), 
					&pos);
		}
		sp_document_done (SP_ACTIVE_DOCUMENT);
		if (newcontent) g_free (newcontent);
	}

	gtk_widget_hide (GTK_WIDGET (dialog));
}

void
sp_xml_tree_remove_element (GtkWidget * widget, gpointer data)
{
	gchar * message, * name;
	static GtkWidget * dialog = NULL;
	gint b;

	if (!selected_repr) return;
	if (!selected_repr->parent) return;
	
	g_return_if_fail (tree->selection != NULL);

	name = SP_REPR_NAME (selected_repr);
	if (!strcmp(name, "sodipodi:namedview") ||
			!strcmp(name, "svg") ||
			!strcmp(name, "defs")) {
		message = g_strdup_printf (_("You can't delete element %s!"), 
				name);
		dialog = gnome_message_box_new(
				message,
				GNOME_MESSAGE_BOX_INFO,
				GNOME_STOCK_BUTTON_OK,
				NULL);
		gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
		g_free (message);
		return;
	}
	message = g_strdup_printf (_("Do you want to delete element %s?"), 
			name);

	dialog = gnome_message_box_new (message,
		GNOME_MESSAGE_BOX_QUESTION,
		_("Yes"),
		_("No"),
		NULL);
	b = gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
	g_free (message);
	if (b) return;
	if (sp_repr_remove_child (selected_repr->parent, selected_repr)) {
		gtk_tree_remove_items (tree, tree->selection);
		selected_repr = NULL;
		sp_document_done (SP_ACTIVE_DOCUMENT);
	}
}

void
sp_xml_tree_new_element (GtkWidget * widget, gpointer data)
{
	static GladeXML * xml = NULL;
	static GtkWidget * dialog, * entry;
	const gchar * name;
	SPRepr * element;
	gint button;

	if (selected_repr == NULL) return;

	if (xml == NULL) {
		xml = glade_xml_new (SODIPODI_GLADEDIR "/xml-tree.glade", "new_element");
		g_assert (xml != NULL);
		dialog = glade_xml_get_widget (xml, "new_element");
		g_assert (dialog != NULL);
		g_assert (GNOME_IS_DIALOG (dialog));
		entry = glade_xml_get_widget (xml, "name");
		g_assert (entry != NULL);
		g_assert (GTK_IS_ENTRY (entry));
	}

	button = gnome_dialog_run (GNOME_DIALOG (dialog));

	if (button == 0) {
		name = gtk_entry_get_text (GTK_ENTRY (entry));
		if (name && (strlen (name) > 0)) { 
			if (!strncmp (name, "svg:", 4)) name += 4;
			if (strlen (name) > 0) {
				element = sp_repr_new (name);
				if (sp_repr_add_child (selected_repr,
							element,
							NULL)) {
					sp_xml_tree_set_contents (
							sp_document_repr_root 
							(SP_ACTIVE_DOCUMENT));
					sp_document_done (SP_ACTIVE_DOCUMENT);
				}
			}
		}
	}

	gtk_widget_hide (GTK_WIDGET (dialog));
}

void 
sp_xml_tree_up_element (GtkWidget * widget, gpointer data)
{
	gint cpos;
	
	if (!selected_repr) return;
	if (!tree->selection) return;
	
	cpos = sp_repr_position (selected_repr);
	if (cpos <= 0) return;
	sp_repr_set_position_absolute (selected_repr, cpos - 1);

	sp_xml_tree_set_contents (sp_document_repr_root (SP_ACTIVE_DOCUMENT));
	
	sp_document_done (SP_ACTIVE_DOCUMENT);
}

void 
sp_xml_tree_down_element (GtkWidget * widget, gpointer data)
{
	if (!selected_repr) return;
	if (!tree->selection) return;
	
	sp_repr_set_position_relative (selected_repr, 1);
	
	sp_xml_tree_set_contents (sp_document_repr_root (SP_ACTIVE_DOCUMENT));
	
	sp_document_done (SP_ACTIVE_DOCUMENT);
}

void 
sp_xml_tree_dup_element (GtkWidget * widget, gpointer data)
{
	SPRepr * new_r, *parent;
	
	if (!selected_repr) return;
	
	parent = sp_repr_parent (selected_repr);
	g_assert (parent != NULL);
		     
	new_r = sp_repr_duplicate (selected_repr);
	sp_repr_add_child (parent, new_r, selected_repr);
	sp_repr_unref (new_r);
	
	sp_xml_tree_set_contents (sp_document_repr_root (SP_ACTIVE_DOCUMENT));
	
	sp_document_done (SP_ACTIVE_DOCUMENT);
}

void 
sp_xml_tree_copy_element (GtkWidget * widget, gpointer data)
{
	if (!selected_repr) return;

	if (!selected_repr) return;
	
	g_return_if_fail (tree->selection != NULL);
	if (clipboard_repr) sp_repr_unref (clipboard_repr);
	clipboard_repr = sp_repr_duplicate (selected_repr); 
}

void 
sp_xml_tree_cut_element (GtkWidget * widget, gpointer data)
{
	gchar * message, * name;
	static GtkWidget * dialog = NULL;
	SPRepr * new_clip;

	if (!selected_repr) return;
	if (!selected_repr->parent) return;
	
	g_return_if_fail (tree->selection != NULL);

	name = SP_REPR_NAME (selected_repr);
	if (!strcmp(name, "sodipodi:namedview") ||
			!strcmp(name, "svg") ||
			!strcmp(name, "defs")) {
		message = g_strdup_printf (_("You can't cut element %s!"), 
				name);
		dialog = gnome_message_box_new(
				message,
				GNOME_MESSAGE_BOX_INFO,
				GNOME_STOCK_BUTTON_OK,
				NULL);
		gnome_dialog_run_and_close (GNOME_DIALOG (dialog));
		g_free (message);
		return;
	}
	
	new_clip = sp_repr_duplicate (selected_repr); 
	
	if (sp_repr_remove_child (selected_repr->parent, selected_repr)) {
		if (clipboard_repr)
			sp_repr_unref (clipboard_repr);
		clipboard_repr = new_clip;
		selected_repr = NULL;
		gtk_tree_remove_items (tree, tree->selection);
		sp_document_done (SP_ACTIVE_DOCUMENT);
	}
}

void 
sp_xml_tree_paste_element (GtkWidget * widget, gpointer data)
{
	SPRepr * new_r;
	
	if (!selected_repr) return;
	if (!clipboard_repr) return;
	
	g_return_if_fail (tree->selection != NULL);
	
	new_r = sp_repr_duplicate (clipboard_repr);
	sp_repr_append_child (selected_repr, new_r);
	selected_repr = new_r;
	sp_repr_unref (new_r);
	
	sp_xml_tree_set_contents (sp_document_repr_root (SP_ACTIVE_DOCUMENT));
	
	sp_document_done (SP_ACTIVE_DOCUMENT);
}

