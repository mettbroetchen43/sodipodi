#define __SP_ANCHOR_C__

/*
 * SPAnchor - implementation of SVG <a> element
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <string.h>
#include <glib.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-url.h>
#include "dialogs/object-attributes.h"
#include "sp-anchor.h"

/* fixme: This is insane, and should be removed */
#include "svg-view.h"

static void sp_anchor_class_init (SPAnchorClass *klass);
static void sp_anchor_init (SPAnchor *anchor);
static void sp_anchor_destroy (GtkObject *object);

static void sp_anchor_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_anchor_read_attr (SPObject * object, const gchar * attr);
static gchar *sp_anchor_description (SPItem *item);
static gint sp_anchor_event (SPItem *item, SPEvent *event);
static void sp_anchor_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu);

static void sp_anchor_link_properties (GtkMenuItem *menuitem, SPAnchor *anchor);
static void sp_anchor_link_follow (GtkMenuItem *menuitem, SPAnchor *anchor);
static void sp_anchor_link_remove (GtkMenuItem *menuitem, SPAnchor *anchor);

static SPGroupClass *parent_class;

GtkType
sp_anchor_get_type (void)
{
	static GtkType type = 0;
	if (!type) {
		GtkTypeInfo info = {
			"SPAnchor",
			sizeof (SPAnchor),
			sizeof (SPAnchorClass),
			(GtkClassInitFunc) sp_anchor_class_init,
			(GtkObjectInitFunc) sp_anchor_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (SP_TYPE_GROUP, &info);
	}
	return type;
}

static void
sp_anchor_class_init (SPAnchorClass *klass)
{
	GtkObjectClass *gtk_object_class;
	SPObjectClass *sp_object_class;
	SPItemClass *item_class;

	gtk_object_class = GTK_OBJECT_CLASS (klass);
	sp_object_class = SP_OBJECT_CLASS (klass);
	item_class = SP_ITEM_CLASS (klass);

	parent_class = gtk_type_class (SP_TYPE_GROUP);

	gtk_object_class->destroy = sp_anchor_destroy;

	sp_object_class->build = sp_anchor_build;
	sp_object_class->read_attr = sp_anchor_read_attr;

	item_class->description = sp_anchor_description;
	item_class->event = sp_anchor_event;
	item_class->menu = sp_anchor_menu;
}

static void
sp_anchor_init (SPAnchor *anchor)
{
	anchor->href = NULL;
}

static void
sp_anchor_destroy (GtkObject *object)
{
	SPAnchor *anchor;

	anchor = SP_ANCHOR (object);

	if (anchor->href) {
		g_free (anchor->href);
		anchor->href = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void sp_anchor_build (SPObject *object, SPDocument * document, SPRepr * repr)
{
	SPAnchor *anchor;

	anchor = SP_ANCHOR (object);

	if (((SPObjectClass *) (parent_class))->build)
		((SPObjectClass *) (parent_class))->build (object, document, repr);

	sp_anchor_read_attr (object, "xlink:type");
	sp_anchor_read_attr (object, "xlink:role");
	sp_anchor_read_attr (object, "xlink:arcrole");
	sp_anchor_read_attr (object, "xlink:title");
	sp_anchor_read_attr (object, "xlink:show");
	sp_anchor_read_attr (object, "xlink:actuate");
	sp_anchor_read_attr (object, "xlink:href");
	sp_anchor_read_attr (object, "target");
}

static void
sp_anchor_read_attr (SPObject *object, const gchar *key)
{
	SPAnchor * anchor;

	anchor = SP_ANCHOR (object);

	if (!strcmp (key, "xlink:href")) {
		if (anchor->href) g_free (anchor->href);
		anchor->href = g_strdup (sp_repr_attr (SP_OBJECT_REPR (object), key));
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else if (!strcmp (key, "xlink:type") ||
		   !strcmp (key, "xlink:role") ||
		   !strcmp (key, "xlink:arcrole") ||
		   !strcmp (key, "xlink:title") ||
		   !strcmp (key, "xlink:show") ||
		   !strcmp (key, "xlink:actuate") ||
		   !strcmp (key, "target")) {
		sp_object_request_modified (object, SP_OBJECT_MODIFIED_FLAG);
	} else {
		if (((SPObjectClass *) (parent_class))->read_attr)
			((SPObjectClass *) (parent_class))->read_attr (object, key);
	}
}

static gchar *
sp_anchor_description (SPItem *item)
{
	SPAnchor * anchor;
	static char c[128];

	anchor = SP_ANCHOR (item);

	snprintf (c, 128, _("Link to %s"), anchor->href);

	return g_strdup (c);
}

/* fixme: We should forward event to appropriate container/view */

static gint
sp_anchor_event (SPItem *item, SPEvent *event)
{
	SPAnchor *anchor;
	/* fixme: */
	SPSVGView *svgview;
	GdkCursor *cursor;

	anchor = SP_ANCHOR (item);

	g_print ("Anchor event\n");

	switch (event->type) {
	case SP_EVENT_ACTIVATE:
		if (anchor->href) {
			g_print ("Activated xlink:href=\"%s\"\n", anchor->href);
			gnome_url_show (anchor->href);
			return TRUE;
		}
		break;
	case SP_EVENT_MOUSEOVER:
		/* fixme: */
		if (SP_IS_SVG_VIEW (event->data)) {
			svgview = event->data;
			cursor = gdk_cursor_new (GDK_HAND2);
			gdk_window_set_cursor (GTK_WIDGET (GNOME_CANVAS_ITEM (svgview->drawing)->canvas)->window, cursor);
			gdk_cursor_destroy (cursor);
		}
		break;
	case SP_EVENT_MOUSEOUT:
		/* fixme: */
		if (SP_IS_SVG_VIEW (event->data)) {
			svgview = event->data;
			gdk_window_set_cursor (GTK_WIDGET (GNOME_CANVAS_ITEM (svgview->drawing)->canvas)->window, NULL);
		}
		break;
	default:
		break;
	}

	return FALSE;
}

/* Generate context menu item section */

static void
sp_anchor_menu (SPItem *item, SPDesktop *desktop, GtkMenu *menu)
{
	GtkWidget *i, *m, *w;

	if (SP_ITEM_CLASS (parent_class)->menu)
		(* SP_ITEM_CLASS (parent_class)->menu) (item, desktop, menu);

	/* Create toplevel menuitem */
	i = gtk_menu_item_new_with_label (_("Link"));
	m = gtk_menu_new ();
	/* Link dialog */
	w = gtk_menu_item_new_with_label (_("Link Properties"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_anchor_link_properties), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Separator */
	w = gtk_menu_item_new ();
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Select item */
	w = gtk_menu_item_new_with_label (_("Follow link"));
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_anchor_link_follow), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Reset transformations */
	w = gtk_menu_item_new_with_label (_("Remove link"));
	gtk_object_set_data (GTK_OBJECT (w), "desktop", desktop);
	gtk_signal_connect (GTK_OBJECT (w), "activate", GTK_SIGNAL_FUNC (sp_anchor_link_remove), item);
	gtk_widget_show (w);
	gtk_menu_append (GTK_MENU (m), w);
	/* Show menu */
	gtk_widget_show (m);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i), m);

	gtk_menu_append (menu, i);
	gtk_widget_show (i);
}

static void
sp_anchor_link_properties (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	sp_object_attributes_dialog (SP_OBJECT (anchor), "SPAnchor");
}

static void
sp_anchor_link_follow (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	g_return_if_fail (anchor != NULL);
	g_return_if_fail (SP_IS_ANCHOR (anchor));

	if (anchor->href) {
		gnome_url_show (anchor->href);
	}
}

static void
sp_anchor_link_remove (GtkMenuItem *menuitem, SPAnchor *anchor)
{
	GSList *children;

	g_return_if_fail (anchor != NULL);
	g_return_if_fail (SP_IS_ANCHOR (anchor));

	children = NULL;
	sp_item_group_ungroup (SP_GROUP (anchor), &children);

#if 0
	sp_selection_set_item_list (SP_DT_SELECTION (desktop), children);
#endif
	g_slist_free (children);
}

