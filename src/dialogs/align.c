#define SP_QUICK_ALIGN_C

#include <config.h>
#include <math.h>
#include <gnome.h>
#include <glade/glade.h>
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
 * - new align types (align to smaler object / chose objects to align to)
 * - aligns for nodes
 *
 */ 

typedef enum {
	SP_ALIGN_LAST,
	SP_ALIGN_FIRST,
	SP_ALIGN_BIGGEST,
	SP_ALIGN_SMALLEST,
	SP_ALIGN_PAGE,
	SP_ALIGN_DRAWING,
	SP_ALIGN_SELECTION,
} SPAlignBase;

static void sp_quick_align_arrange (gdouble mx0, gdouble mx1, gdouble my0, gdouble my1,
				    gdouble sx0, gdouble sx1, gdouble sy0, gdouble sy1);
static GtkWidget * create_base_menu (void);
static void set_base (GtkMenuItem * menuitem, gpointer data);
static SPItem * sp_quick_align_find_master (const GSList * slist, gboolean horizontal);

static GladeXML  * quick_align_xml = NULL;
static GtkWidget * quick_align_dialog = NULL;
static SPAlignBase base = SP_ALIGN_LAST;

void
sp_quick_align_dialog (void)
{
	if (quick_align_dialog == NULL) {
		GtkWidget * align_base;

		quick_align_xml = glade_xml_new (SODIPODI_GLADEDIR "/align.glade", "quick_align");
		if (quick_align_xml == NULL) return;
		glade_xml_signal_autoconnect (quick_align_xml);
		quick_align_dialog = glade_xml_get_widget (quick_align_xml, "quick_align");
		if (quick_align_dialog == NULL) return;
		align_base = glade_xml_get_widget (quick_align_xml, "align_base");
		if (align_base == NULL) return;
		gtk_option_menu_set_menu ((GtkOptionMenu *) align_base, create_base_menu ());
	} else {
		if (!GTK_WIDGET_VISIBLE (quick_align_dialog))
			gtk_widget_show (quick_align_dialog);
	}
}

void
sp_quick_align_dialog_close (void)
{
	g_assert (quick_align_dialog != NULL);

	if (GTK_WIDGET_VISIBLE (quick_align_dialog))
		gtk_widget_hide (quick_align_dialog);
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
set_base (GtkMenuItem * menuitem, gpointer data)
{
	base = (SPAlignBase) data;
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
sp_quick_align_find_master (const GSList * slist, gboolean horizontal)
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





