#ifndef __SP_BUTTON_H__
#define __SP_BUTTON_H__

/*
 * Generic button widget
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

typedef struct _SPButton SPButton;
typedef struct _SPButtonClass SPButtonClass;

typedef struct _SPBImageData SPBImageData;

#define SP_TYPE_BUTTON (sp_button_get_type ())
#define SP_BUTTON(o) (GTK_CHECK_CAST ((o), SP_TYPE_BUTTON, SPButton))
#define SP_IS_BUTTON(o) (GTK_CHECK_TYPE ((o), SP_TYPE_BUTTON))

#include <gtk/gtkwidget.h>
#include <gtk/gtktooltips.h>

enum {
	SP_BUTTON_TYPE_NORMAL,
	SP_BUTTON_TYPE_TOGGLE
};

struct _SPBImageData {
	unsigned char *px;
	unsigned char *tip;
};

struct _SPButton {
	GtkWidget widget;

	unsigned int noptions : 4;
	unsigned int option : 4;
	unsigned int type : 2;
	unsigned int inside : 1;
	unsigned int pressed : 1;
	unsigned int down : 1;
	unsigned int initial : 1;
	unsigned int grabbed : 1;
	unsigned int size : 8;

	SPBImageData *options;

	GtkWidget *menu;
	GtkTooltips *tooltips;

	int timeout;

	GdkWindow *event_window;
};

struct _SPButtonClass {
	GtkWidgetClass parent_class;

	void (* pressed) (SPButton *button);
	void (* released) (SPButton *button);
	void (* clicked) (SPButton *button);
	void (* toggled) (SPButton *button);
};

#define SP_BUTTON_IS_DOWN(b) (SP_BUTTON (b)->down)

GType sp_button_get_type (void);

GtkWidget *sp_button_new (unsigned int size, const unsigned char *name, const unsigned char *tip);
GtkWidget *sp_button_toggle_new (unsigned int size, const unsigned char *name, const unsigned char *tip);
GtkWidget *sp_button_menu_new (unsigned int size, unsigned int noptions);
GtkWidget *sp_button_toggle_menu_new (unsigned int size, unsigned int noptions);

void sp_button_set_tooltips (SPButton *button, GtkTooltips *tooltips);

void sp_button_toggle_set_down (SPButton *button, unsigned int down, unsigned int signal);

void sp_button_add_option (SPButton *button, unsigned int option, const unsigned char *name, const unsigned char *tip);
unsigned int sp_button_get_option (SPButton *button);
void sp_button_set_option (SPButton *button, unsigned int option);

#endif
