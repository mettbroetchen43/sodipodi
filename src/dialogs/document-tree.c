#define __SP_DOCUMENT_TREE_C__

/*
 * Document tree dialog
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2004 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noTREE_DEBUG

#include <glib.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktreeselection.h>

#include "macros.h"
#include "helper/sp-intl.h"
#include "helper/window.h"
#include "widgets/tree-store.h"
#include "widgets/cell-renderer-image.h"
#include "sodipodi.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "sp-item-group.h"

static GtkWidget *sp_document_tree_new (SPDesktop *desktop);

static GtkWidget *dlg = NULL;

static void
sp_document_tree_dialog_doc_destroy (GtkObject *object, gpointer data)
{
	gtk_widget_destroy (dlg);
}

static void
sp_document_tree_dialog_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data ((GObject *) g_object_get_data ((GObject *) dlg, "document"), dlg);
	sp_signal_disconnect_by_data (SODIPODI, dlg);

	dlg = NULL;
}

void
sp_document_tree_dialog (void)
{
	if (!dlg) {
		SPDocument *doc;
		GtkWidget *w;
		dlg = sp_window_new (_("Document"), TRUE, FALSE);
		gtk_window_set_default_size ((GtkWindow *) dlg, 300, 400);
		doc = SP_ACTIVE_DOCUMENT;
		g_object_set_data ((GObject *) dlg, "document", doc);
		w = sp_document_tree_new (SP_ACTIVE_DESKTOP);
		g_signal_connect ((GObject *) doc, "destroy", (GCallback) sp_document_tree_dialog_doc_destroy, dlg);
		gtk_container_add ((GtkContainer *) dlg, w);
		g_signal_connect (G_OBJECT (dlg), "destroy", G_CALLBACK (sp_document_tree_dialog_destroy), NULL);
		gtk_widget_show_all (dlg);
	}

	gtk_window_present ((GtkWindow *) dlg);
}

#if 0
static void
sp_document_tree_selection_changed (GtkTreeSelection *sel, GtkTreeModel *model)
{
	SPObject *object;
	GtkTreeIter iter;
	gtk_tree_selection_get_selected (sel, NULL, &iter);
	object = iter.user_data;
	if (SP_IS_GROUP (object)) {
		SPDesktop *desktop;
		desktop = SP_ACTIVE_DESKTOP;
		if (SP_DT_DOCUMENT (desktop) == object->document) {
			sp_desktop_set_base (desktop, (SPGroup *) object);
		}
	}
}
#endif

static void
sp_document_tree_target_toggled (SPCellRendererImage *cri, const char *path, GtkTreeModel *model)
{
	if (cri->value) {
		SPObject *object;
		GtkTreeIter iter;
		gtk_tree_model_get_iter_from_string (model, &iter, path);
		object = iter.user_data;
		if (SP_IS_GROUP (object)) {
			SPDesktop *desktop;
			desktop = SP_ACTIVE_DESKTOP;
			if (SP_DT_DOCUMENT (desktop) == object->document) {
				sp_desktop_set_base (desktop, (SPGroup *) object);
			}
		}
	}
}

static void
sp_document_tree_visible_toggled (GtkCellRendererToggle *crt, const char *path, GtkTreeModel *model)
{
	SPObject *object;
	GtkTreeIter iter;
	unsigned int val;
	gtk_tree_model_get_iter_from_string (model, &iter, path);
	object = iter.user_data;
	val = FALSE;
	sp_repr_get_boolean (object->repr, "sodipodi:invisible", &val);
	sp_repr_set_boolean (object->repr, "sodipodi:invisible", !val);
}

static void
sp_document_tree_sensitive_toggled (GtkCellRendererToggle *crt, const char *path, GtkTreeModel *model)
{
	SPObject *object;
	GtkTreeIter iter;
	unsigned int val;
	gtk_tree_model_get_iter_from_string (model, &iter, path);
	object = iter.user_data;
	val = FALSE;
	sp_repr_get_boolean (object->repr, "sodipodi:insensitive", &val);
	sp_repr_set_boolean (object->repr, "sodipodi:insensitive", !val);
}

static void
sp_document_tree_printable_toggled (GtkCellRendererToggle *crt, const char *path, GtkTreeModel *model)
{
	SPObject *object;
	GtkTreeIter iter;
	unsigned int val;
	gtk_tree_model_get_iter_from_string (model, &iter, path);
	object = iter.user_data;
	val = FALSE;
	sp_repr_get_boolean (object->repr, "sodipodi:nonprintable", &val);
	sp_repr_set_boolean (object->repr, "sodipodi:nonprintable", !val);
}

static GtkWidget *
sp_document_tree_new (SPDesktop *desktop)
{
	SPDocument *doc;
	GtkWidget *sw, *tree;
	SPTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *rtext, *rimg, *rtoggle;
	GtkTreeSelection *sel;
	/* GtkTreeIter iter, piter; */
	/* GValue val = {0}; */

	doc = SP_VIEW_DOCUMENT (desktop);

	store = sp_tree_store_new (desktop, NULL, NULL);

	sw = gtk_scrolled_window_new (NULL, NULL);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref ((GObject *) store);
	gtk_widget_show (tree);
	gtk_container_add ((GtkContainer *) sw, tree);

	column = gtk_tree_view_column_new ();
	rtext = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, rtext, TRUE);
	gtk_tree_view_column_set_title (column, "ID");
	gtk_tree_view_column_add_attribute (column, rtext, "text", SP_TREE_STORE_COLUMN_ID);
	gtk_tree_view_append_column ((GtkTreeView *) tree, column);

	column = gtk_tree_view_column_new ();
	rimg = sp_cell_renderer_image_new (2, 16);
	sp_cell_renderer_image_set_image ((SPCellRendererImage *) rimg, 0, "drawing-nontarget");
	sp_cell_renderer_image_set_image ((SPCellRendererImage *) rimg, 1, "drawing-target");
	g_signal_connect ((GObject *) rimg, "toggled", (GCallback) sp_document_tree_target_toggled, store);
	gtk_tree_view_column_pack_start (column, rimg, TRUE);
	gtk_tree_view_column_set_title (column, "Tgt");
	gtk_tree_view_column_add_attribute (column, rimg, "activatable", SP_TREE_STORE_COLUMN_IS_GROUP);
	gtk_tree_view_column_add_attribute (column, rimg, "value", SP_TREE_STORE_COLUMN_TARGET);
	gtk_tree_view_append_column ((GtkTreeView *) tree, column);

	column = gtk_tree_view_column_new ();
	rtoggle = gtk_cell_renderer_toggle_new ();
	g_signal_connect ((GObject *) rtoggle, "toggled", (GCallback) sp_document_tree_visible_toggled, store);
	gtk_tree_view_column_pack_start (column, rtoggle, TRUE);
	gtk_tree_view_column_set_title (column, "Vis");
	gtk_tree_view_column_add_attribute (column, rtoggle, "activatable", SP_TREE_STORE_COLUMN_IS_VISUAL);
	gtk_tree_view_column_add_attribute (column, rtoggle, "active", SP_TREE_STORE_COLUMN_VISIBLE);
	gtk_tree_view_append_column ((GtkTreeView *) tree, column);

	column = gtk_tree_view_column_new ();
	rtoggle = gtk_cell_renderer_toggle_new ();
	g_signal_connect ((GObject *) rtoggle, "toggled", (GCallback) sp_document_tree_sensitive_toggled, store);
	gtk_tree_view_column_pack_start (column, rtoggle, TRUE);
	gtk_tree_view_column_set_title (column, "Sen");
	gtk_tree_view_column_add_attribute (column, rtoggle, "activatable", SP_TREE_STORE_COLUMN_IS_VISUAL);
	gtk_tree_view_column_add_attribute (column, rtoggle, "active", SP_TREE_STORE_COLUMN_SENSITIVE);
	gtk_tree_view_append_column ((GtkTreeView *) tree, column);

	column = gtk_tree_view_column_new ();
	rtoggle = gtk_cell_renderer_toggle_new ();
	g_signal_connect ((GObject *) rtoggle, "toggled", (GCallback) sp_document_tree_printable_toggled, store);
	gtk_tree_view_column_pack_start (column, rtoggle, TRUE);
	gtk_tree_view_column_set_title (column, "Pri");
	gtk_tree_view_column_add_attribute (column, rtoggle, "activatable", SP_TREE_STORE_COLUMN_IS_VISUAL);
	gtk_tree_view_column_add_attribute (column, rtoggle, "active", SP_TREE_STORE_COLUMN_PRINTABLE);
	gtk_tree_view_append_column ((GtkTreeView *) tree, column);

	sel = gtk_tree_view_get_selection ((GtkTreeView *) tree);
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
	
#if 0
	g_signal_connect ((GObject *) sel, "changed", (GCallback) sp_document_tree_selection_changed, store);
#endif

	return sw;
}

