#ifndef __SP_MENU_BUTTON_H__
#define __SP_MENU_BUTTON_H__

/*
 * Button with menu
 *
 * Authors:
 *   The Gtk+ Team
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <gtk/gtktogglebutton.h>

#define SP_TYPE_MENU_BUTTON (sp_menu_button_get_type ())
#define SP_MENU_BUTTON(o) (GTK_CHECK_CAST ((o), SP_TYPE_MENU_BUTTON, SPMenuButton))
#define SP_MENU_BUTTON_CLASS(k) (GTK_CHECK_CLASS_CAST ((k), SP_TYPE_MENU_BUTTON, SPMenuButtonClass))
#define SP_IS_MENU_BUTTON(o) (GTK_CHECK_TYPE ((o), SP_TYPE_MENU_BUTTON))
#define SP_IS_MENU_BUTTON_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SP_TYPE_MENU_BUTTON))

typedef struct _SPMenuButton SPMenuButton;
typedef struct _SPMenuButtonClass SPMenuButtonClass;

struct _SPMenuButton
{
	GtkToggleButton button;
  
	GtkWidget *menu;
	GtkWidget *menuitem;
  
	guint cbid;

	guint16 width;
	guint16 height;
};

struct _SPMenuButtonClass
{
	GtkToggleButtonClass parent_class;

	void (* activate_item) (SPMenuButton *mb, gpointer data);
};


GtkType sp_menu_button_get_type (void);

GtkWidget* sp_menu_button_new (void);

void sp_menu_button_append_child (SPMenuButton *mb, GtkWidget *child, gpointer data);

void sp_menu_button_set_active (SPMenuButton *mb, gpointer data);
gpointer sp_menu_button_get_active_data (SPMenuButton *mb);

#if 0
GtkWidget* gtk_option_menu_get_menu    (GtkOptionMenu *option_menu);
void       gtk_option_menu_set_menu    (GtkOptionMenu *option_menu,
					GtkWidget     *menu);
void       gtk_option_menu_remove_menu (GtkOptionMenu *option_menu);
void       gtk_option_menu_set_history (GtkOptionMenu *option_menu,
					guint          index);
#endif

#endif
