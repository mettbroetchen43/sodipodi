#define __SP_QUICK_ALIGN_C__

/*
 * Object align dialog
 *
 * Authors:
 *   Frank Felfe <innerspace@iname.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <math.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktable.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <libgnomeui/gnome-stock.h>
#include "../sodipodi.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../sp-item-transform.h"
#include "../selection.h"
#include "align.h"

/*
 * handler functions for quick align dialog
 *
 * todo: dialog with more aligns
 * - more aligns (30 % left from center ...)
 * - aligns for nodes
 *
 */ 

enum {
	SP_ALIGN_LAST,
	SP_ALIGN_FIRST,
	SP_ALIGN_BIGGEST,
	SP_ALIGN_SMALLEST,
	SP_ALIGN_PAGE,
	SP_ALIGN_DRAWING,
	SP_ALIGN_SELECTION,
};

void sp_quick_align_top_in (void);
void sp_quick_align_top_out (void);
void sp_quick_align_bottom_in (void);
void sp_quick_align_bottom_out (void);
void sp_quick_align_left_in (void);
void sp_quick_align_left_out (void);
void sp_quick_align_right_in (void);
void sp_quick_align_right_out (void);
void sp_quick_align_center_hor (void);
void sp_quick_align_center_ver (void);

void sp_quick_align_dialog_close (void);

static void sp_quick_align_arrange (gdouble mx0, gdouble mx1, gdouble my0, gdouble my1,
				    gdouble sx0, gdouble sx1, gdouble sy0, gdouble sy1);
static GtkWidget * create_base_menu (void);
static void set_base (GtkMenuItem * menuitem, gpointer data);
static SPItem * sp_quick_align_find_master (const GSList * slist, gboolean horizontal);

static GtkWidget *dlg = NULL;

static unsigned int base = SP_ALIGN_LAST;

static int
sp_quick_align_dialog_delete (void)
{
	if (GTK_WIDGET_VISIBLE (dlg)) gtk_widget_hide (dlg);

	return TRUE;
}

static void
sp_align_add_button (GtkWidget *t, int col, int row, GtkSignalFunc handler, const unsigned char *pxname)
{
	GtkWidget *pm, *b;
	unsigned char c[1024];

	g_snprintf (c, 1024, "%s/%s", SODIPODI_PIXMAPDIR, pxname);
	pm = gnome_stock_pixmap_widget (t, c);
	gtk_widget_show (pm);
	b = gtk_button_new ();
	gtk_widget_show (b);
	gtk_container_add (GTK_CONTAINER (b), pm);
	if (handler) gtk_signal_connect (GTK_OBJECT (b), "clicked", handler, NULL);
	gtk_table_attach (GTK_TABLE (t), b, col, col + 1, row, row + 1, 0, 0, 0, 0);
}

void
sp_quick_align_dialog (void)
{
	if (!dlg) {
		GtkWidget *vb, *om, *t;

		dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (dlg), _("Align objects"));
		gtk_signal_connect (GTK_OBJECT (dlg), "delete_event", GTK_SIGNAL_FUNC (sp_quick_align_dialog_delete), NULL);

		vb = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vb);
		gtk_container_add (GTK_CONTAINER (dlg), vb);

		om = gtk_option_menu_new ();
		gtk_widget_show (om);
		gtk_box_pack_start (GTK_BOX (vb), om, FALSE, FALSE, 0);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (om), create_base_menu ());

		t = gtk_table_new (2, 5, TRUE);
		gtk_widget_show (t);
		gtk_box_pack_start (GTK_BOX (vb), t, FALSE, FALSE, 0);

		sp_align_add_button (t, 0, 0, GTK_SIGNAL_FUNC (sp_quick_align_left_out), "al_left_out.xpm");
		sp_align_add_button (t, 1, 0, GTK_SIGNAL_FUNC (sp_quick_align_left_in), "al_left_in.xpm");
		sp_align_add_button (t, 2, 0, GTK_SIGNAL_FUNC (sp_quick_align_center_hor), "al_center_hor.xpm");
		sp_align_add_button (t, 3, 0, GTK_SIGNAL_FUNC (sp_quick_align_right_in), "al_right_in.xpm");
		sp_align_add_button (t, 4, 0, GTK_SIGNAL_FUNC (sp_quick_align_right_out), "al_right_out.xpm");

		sp_align_add_button (t, 0, 1, GTK_SIGNAL_FUNC (sp_quick_align_top_out), "al_top_out.xpm");
		sp_align_add_button (t, 1, 1, GTK_SIGNAL_FUNC (sp_quick_align_top_in), "al_top_in.xpm");
		sp_align_add_button (t, 2, 1, GTK_SIGNAL_FUNC (sp_quick_align_center_ver), "al_center_ver.xpm");
		sp_align_add_button (t, 3, 1, GTK_SIGNAL_FUNC (sp_quick_align_bottom_in), "al_bottom_in.xpm");
		sp_align_add_button (t, 4, 1, GTK_SIGNAL_FUNC (sp_quick_align_bottom_out), "al_bottom_out.xpm");
	}

	if (!GTK_WIDGET_VISIBLE (dlg)) gtk_widget_show (dlg);
}

void
sp_quick_align_dialog_close (void)
{
	g_assert (dlg != NULL);

	if (GTK_WIDGET_VISIBLE (dlg)) gtk_widget_hide (dlg);
}

static GtkWidget *
create_base_menu (void)
{
	GtkWidget * menu;
	GtkWidget * menuitem;

	menu = gtk_menu_new ();

	menuitem = gtk_menu_item_new_with_label (_("Last selected"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_LAST));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("First selected"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_FIRST));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("Biggest item"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_BIGGEST));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("Smallest item"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_SMALLEST));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("Page"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_PAGE));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("Drawing"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_DRAWING));
	gtk_menu_append ((GtkMenu *) menu, menuitem);
	menuitem = gtk_menu_item_new_with_label (_("Selection"));
	gtk_widget_show (menuitem);
	gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			    GTK_SIGNAL_FUNC (set_base), GINT_TO_POINTER (SP_ALIGN_SELECTION));
	gtk_menu_append ((GtkMenu *) menu, menuitem);

	gtk_widget_show (menu);

	return menu;
}

static void
set_base (GtkMenuItem *menuitem, gpointer data)
{
	base = GPOINTER_TO_UINT (data);
}


void
sp_quick_align_top_in (void)
{
	sp_quick_align_arrange (0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
}


void
sp_quick_align_top_out (void) {
	sp_quick_align_arrange (0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0);
}


void
sp_quick_align_right_in (void) {
	sp_quick_align_arrange (0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}


void
sp_quick_align_right_out (void) {
	sp_quick_align_arrange (0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
}


void
sp_quick_align_bottom_in (void) {
	sp_quick_align_arrange (0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}


void
sp_quick_align_bottom_out (void) {
	sp_quick_align_arrange (0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
}


void
sp_quick_align_left_in (void) {
	sp_quick_align_arrange (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
}

void
sp_quick_align_left_out (void) {
	sp_quick_align_arrange (1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}


void
sp_quick_align_center_hor (void) {
	sp_quick_align_arrange (0.5, 0.5, 0.0, 0.0, 0.5, 0.5, 0.0, 0.0);
}

void
sp_quick_align_center_ver (void) {
	sp_quick_align_arrange (0.0, 0.0, 0.5, 0.5, 0.0, 0.0, 0.5, 0.5);
}

static void
sp_quick_align_arrange (gdouble mx0, gdouble mx1, gdouble my0, gdouble my1,
			gdouble sx0, gdouble sx1, gdouble sy0, gdouble sy1)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * slist;
	SPItem * master, * item;
	ArtDRect b;
	ArtPoint mp, sp;
	GSList * l;
	gboolean changed;

	desktop = SP_ACTIVE_DESKTOP;
	if (!desktop) return;

	selection = SP_DT_SELECTION (desktop);
	slist = (GSList *) sp_selection_item_list (selection);
	if (!slist) return;

	switch (base) {
	case SP_ALIGN_LAST:
	case SP_ALIGN_FIRST:
	case SP_ALIGN_BIGGEST:
	case SP_ALIGN_SMALLEST:
		if (!slist->next) return;
		slist = g_slist_copy (slist);
		master = sp_quick_align_find_master (slist, (mx0 != 0.0) || (mx1 != 0.0));
		slist = g_slist_remove (slist, master);
		sp_item_bbox_desktop (master, &b);
		mp.x = mx0 * b.x0 + mx1 * b.x1;
		mp.y = my0 * b.y0 + my1 * b.y1;
		break;
	case SP_ALIGN_PAGE:
		slist = g_slist_copy (slist);
		mp.x = mx1 * sp_document_width (SP_DT_DOCUMENT (desktop));
		mp.y = my1 * sp_document_height (SP_DT_DOCUMENT (desktop));
		break;
	case SP_ALIGN_DRAWING:
		slist = g_slist_copy (slist);
		sp_item_bbox_desktop ((SPItem *) sp_document_root (SP_DT_DOCUMENT (desktop)), &b);
		mp.x = mx0 * b.x0 + mx1 * b.x1;
		mp.y = my0 * b.y0 + my1 * b.y1;
		break;
	case SP_ALIGN_SELECTION:
		slist = g_slist_copy (slist);
		sp_selection_bbox (selection, &b);
		mp.x = mx0 * b.x0 + mx1 * b.x1;
		mp.y = my0 * b.y0 + my1 * b.y1;
		break;
	default:
		g_assert_not_reached ();
		break;
	};

	changed = FALSE;

	for (l = slist; l != NULL; l = l->next) {
		item = (SPItem *) l->data;
		sp_item_bbox_desktop (item, &b);
		sp.x = sx0 * b.x0 + sx1 * b.x1;
		sp.y = sy0 * b.y0 + sy1 * b.y1;

		if ((fabs (mp.x - sp.x) > 1e-9) || (fabs (mp.y - sp.y) > 1e-9)) {
			sp_item_move_rel (item, mp.x - sp.x, mp.y - sp.y);
			changed = TRUE;
		}
	}

	g_slist_free (slist);

	if (changed) {
		sp_selection_changed (selection);
		sp_document_done (SP_DT_DOCUMENT (desktop));
	}
}

static SPItem *
sp_quick_align_find_master (const GSList *slist, gboolean horizontal)
{
	ArtDRect b;
	const GSList * l;
	SPItem * master, * item;
	gdouble dim, max;

	master = NULL;

	switch (base) {
	case SP_ALIGN_LAST:
		return (SPItem *) slist->data;
		break;
	case SP_ALIGN_FIRST: 
		return (SPItem *) g_slist_last ((GSList *) slist)->data;
		break;
	case SP_ALIGN_BIGGEST:
		max = -1e18;
		for (l = slist; l != NULL; l = l->next) {
			item = (SPItem *) l->data;
			sp_item_bbox_desktop (item, &b);
			if (horizontal) {
				dim = b.x1 - b.x0; 
			} else {
				dim = b.y1 - b.y0;
			}
			if (dim > max) {
				max = dim;
				master = item;
			}
		}
		return master;
		break;
	case SP_ALIGN_SMALLEST:
		max = 1e18;
		for (l = slist; l != NULL; l = l->next) {
			item = (SPItem *) l->data;
			sp_item_bbox_desktop (item, &b);
			if (horizontal) {
				dim = b.x1 - b.x0;
			} else {
				dim = b.y1 - b.y0;
			}
			if (dim < max) {
				max = dim;
				master = item;
			}
		}
		return master;
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	return NULL;
}





