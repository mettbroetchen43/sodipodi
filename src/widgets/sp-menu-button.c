#define __SP_MENU_BUTTON_C__

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

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>

#include "sp-menu-button.h"

enum {ACTIVATE_ITEM, LAST_SIGNAL};

static void sp_menu_button_class_init (SPMenuButtonClass *klass);
static void sp_menu_button_init (SPMenuButton *mb);
static void sp_menu_button_destroy (GtkObject *object);

#if 0
static void sp_menu_button_draw (GtkWidget *widget, GdkRectangle *area);
#endif
static void sp_menu_button_draw_arrow (GtkWidget * widget);

static void sp_menu_button_size_request (GtkWidget *widget, GtkRequisition *req);
static gint sp_menu_button_expose (GtkWidget *widget, GdkEventExpose *event);
static gint sp_menu_button_button_press (GtkWidget *widget, GdkEventButton *event);
static gint sp_menu_button_button_release (GtkWidget *widget, GdkEventButton *event);
static void sp_menu_button_released (GtkButton *button);

static gint sp_menu_button_button_timeout (gpointer data);
static void sp_menu_button_menu_deactivate (GtkMenuShell *shell, SPMenuButton *mb);

static void sp_menu_button_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data);

static GtkToggleButtonClass *parent_class = NULL;
static guint button_signals[LAST_SIGNAL] = {0};

GtkType
sp_menu_button_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GtkTypeInfo info = {
			"SPMenuButton",
			sizeof (SPMenuButton),
			sizeof (SPMenuButtonClass),
			(GtkClassInitFunc) sp_menu_button_class_init,
			(GtkObjectInitFunc) sp_menu_button_init,
			NULL, NULL, NULL
		};
		type = gtk_type_unique (GTK_TYPE_TOGGLE_BUTTON, &info);
	}

	return type;
}

static void
sp_menu_button_class_init (SPMenuButtonClass *klass)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	button_class = (GtkButtonClass*) klass;

	parent_class = gtk_type_class (GTK_TYPE_TOGGLE_BUTTON);

	object_class->destroy = sp_menu_button_destroy;

	button_signals[ACTIVATE_ITEM] = gtk_signal_new ("activate_item",
						   GTK_RUN_FIRST,
						   GTK_CLASS_TYPE(object_class),
						   GTK_SIGNAL_OFFSET (SPMenuButtonClass, activate_item),
						   gtk_marshal_NONE__POINTER,
						   GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);

#if 0
  	widget_class->draw = sp_menu_button_draw;
#endif
	widget_class->size_request = sp_menu_button_size_request;
	widget_class->expose_event = sp_menu_button_expose;
	widget_class->button_press_event = sp_menu_button_button_press;
	widget_class->button_release_event = sp_menu_button_button_release;

	button_class->released = sp_menu_button_released;
}

static void
sp_menu_button_init (SPMenuButton *mb)
{
	GTK_WIDGET_SET_FLAGS (mb, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (mb, GTK_CAN_DEFAULT | GTK_RECEIVES_DEFAULT);

	mb->menu = gtk_menu_new ();
	mb->menuitem = NULL;
	gtk_signal_connect (GTK_OBJECT (mb->menu), "deactivate", GTK_SIGNAL_FUNC (sp_menu_button_menu_deactivate), mb);

	mb->cbid = 0;

	mb->width = 0;
	mb->height = 0;
}

static void
sp_menu_button_destroy (GtkObject *object)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (object);

	if (mb->cbid) {
		gtk_timeout_remove (mb->cbid);
		mb->cbid = 0;
	}

	if (mb->menu) {
		gtk_widget_destroy (mb->menu);
		mb->menu = NULL;
	}

	mb->menuitem = NULL;

	GTK_OBJECT_CLASS (parent_class)->destroy (object);


}

#if 0
static void
sp_menu_button_draw (GtkWidget *widget, GdkRectangle *area)
{
	if (GTK_WIDGET_CLASS (parent_class)->draw)
		GTK_WIDGET_CLASS (parent_class)->draw (widget, area);

	sp_menu_button_draw_arrow(widget);
}
#endif

static void
sp_menu_button_size_request (GtkWidget *widget, GtkRequisition *req)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	if ((mb->width > 0) || (mb->height > 0)) {
		req->width = mb->width;
		req->height = mb->height;
	} else {
		if (GTK_WIDGET_CLASS (parent_class)->size_request)
			GTK_WIDGET_CLASS (parent_class)->size_request (widget, req);
	}
}

static gint
sp_menu_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
	gint ret;

	if (GTK_WIDGET_CLASS (parent_class)->expose_event) {
		ret = GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
	} else {
		ret = FALSE;
	}

	sp_menu_button_draw_arrow(widget);

	return ret;
}

static gint
sp_menu_button_button_press (GtkWidget *widget, GdkEventButton *event)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	g_print ("Press\n");

	if ((event->button == 1) && !mb->cbid) {
		mb->cbid = gtk_timeout_add (100, sp_menu_button_button_timeout, mb);
	}

	if (GTK_WIDGET_CLASS (parent_class)->button_press_event)
		return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

	return FALSE;
}

static gint
sp_menu_button_button_release (GtkWidget *widget, GdkEventButton *event)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (widget);

	g_print ("Release\n");

	if ((event->button == 1) && mb->cbid) {
		gtk_timeout_remove (mb->cbid);
		mb->cbid = 0;
	}

	if (GTK_WIDGET_CLASS (parent_class)->button_release_event)
		return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

	return FALSE;
}

static void
sp_menu_button_released (GtkButton *button)
{
	SPMenuButton *mb;
	gpointer data;

	mb = SP_MENU_BUTTON (button);

	if (GTK_BUTTON_CLASS (parent_class)->released)
		GTK_BUTTON_CLASS (parent_class)->released (button);

	if (mb->menuitem) {
		data = gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
		gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
	}
}

GtkWidget*
sp_menu_button_new (void)
{
	SPMenuButton *mb;

	mb = gtk_type_new (SP_TYPE_MENU_BUTTON);

	return (GtkWidget *) mb;
}

void
sp_menu_button_append_child (SPMenuButton *mb, GtkWidget *child, gpointer data)
{
	GtkWidget *mi;

	g_return_if_fail (mb != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (mb));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	mi = gtk_menu_item_new ();
	gtk_widget_show (mi);
	gtk_object_set_data (GTK_OBJECT (mi), "user", data);

	if (mb->menuitem) {
		/* Put child into menu now */
		gtk_container_add (GTK_CONTAINER (mi), child);
	} else {
		/* Put child into main button */
		gtk_container_add (GTK_CONTAINER (mb), child);
		mb->menuitem = mi;
	}

	gtk_menu_append (GTK_MENU (mb->menu), mi);
}

void
sp_menu_button_set_active (SPMenuButton *mb, gpointer data)
{
	g_return_if_fail (mb != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (mb));

	if (mb->menu) {
		GList *children;
		children = GTK_MENU_SHELL (mb->menu)->children;
		while (children) {
			gpointer user;
			user = gtk_object_get_data (GTK_OBJECT (children->data), "user");
			if (user == data) {
				if (children->data != mb->menuitem) {
					gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb)), mb->menuitem);
					mb->menuitem = GTK_WIDGET (children->data);
					gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb->menuitem)), GTK_WIDGET (mb));
					gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
				}
				return;
			}
			children = children->next;
		}

		g_warning ("Data not associated with any child");
	}

	g_warning ("No menu");
}

gpointer
sp_menu_button_get_active_data (SPMenuButton *mb)
{
	g_return_val_if_fail (mb != NULL, NULL);
	g_return_val_if_fail (SP_IS_MENU_BUTTON (mb), NULL);

	if (mb->menuitem) {
		return gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
	}

	return NULL;
}

static gint
sp_menu_button_button_timeout (gpointer data)
{
	SPMenuButton *mb;

	mb = SP_MENU_BUTTON (data);

	g_print ("Menu button timeout\n");

	/* fixme: Think (lauris) */
	/* We keep original values to prevent resizing */
	mb->width = GTK_WIDGET (mb)->allocation.width;
	mb->height = GTK_WIDGET (mb)->allocation.height;
	gtk_widget_reparent (gtk_bin_get_child(GTK_BIN (mb)), mb->menuitem);
	gtk_menu_popup (GTK_MENU (mb->menu), NULL, NULL, sp_menu_button_menu_position, mb, 1, GDK_CURRENT_TIME);

	mb->cbid = 0;

	return FALSE;
}

static void
sp_menu_button_menu_deactivate (GtkMenuShell *shell, SPMenuButton *mb)
{
	/* fixme: Think (Lauris) */
	mb->menuitem = gtk_menu_get_active (GTK_MENU (shell));

	if (mb->menuitem) {
		GtkWidget *child;
		gpointer data;

		child = GTK_BIN (mb->menuitem)->child;

		if (child) {
			gtk_widget_reparent (child, GTK_WIDGET (mb));
		}

		mb->width = 0;
		mb->height = 0;

		gtk_grab_remove (GTK_WIDGET (mb));
		gtk_button_released (GTK_BUTTON (mb));

		data = gtk_object_get_data (GTK_OBJECT (mb->menuitem), "user");
		gtk_signal_emit (GTK_OBJECT (mb), button_signals[ACTIVATE_ITEM], data);
	}
}

static void
sp_menu_button_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
	SPMenuButton *menu_button;
	GtkWidget *active;
#if 0
	GtkWidget *child;
#endif
	GtkRequisition requisition;
	GList *children;
	gint shift_menu;
	gint screen_width;
	gint screen_height;
	gint menu_xpos;
	gint menu_ypos;
	gint width;
	gint height;
	
	g_return_if_fail (data != NULL);
	g_return_if_fail (SP_IS_MENU_BUTTON (data));

	menu_button = SP_MENU_BUTTON (data);

	gtk_widget_get_child_requisition (GTK_WIDGET (menu), &requisition);
	width = requisition.width;
	height = requisition.height;
	
	active = gtk_menu_get_active (GTK_MENU (menu_button->menu));
	children = GTK_MENU_SHELL (menu_button->menu)->children;
	gdk_window_get_origin (GTK_WIDGET (menu_button)->window, &menu_xpos, &menu_ypos);
	menu_xpos += GTK_WIDGET (menu_button)->allocation.x;
	menu_ypos += GTK_WIDGET (menu_button)->allocation.y;

#if 1	/* if 0 to compatible with sodipodi with gtk+1.2 */
	menu_ypos += GTK_WIDGET (menu_button)->allocation.height / 2 - 2;
	
	if (active != NULL) {
		gtk_widget_get_child_requisition (active, &requisition);
		menu_ypos -= requisition.height / 2;
	}

	children = children->next;

screen_width = gdk_screen_width ();
screen_height = gdk_screen_height ();

  shift_menu = FALSE;
  if (menu_ypos < 0)
    {
      menu_ypos = 0;
      shift_menu = TRUE;
    }
  else if ((menu_ypos + height) > screen_height)
    {
      menu_ypos -= ((menu_ypos + height) - screen_height);
      shift_menu = TRUE;
    }

  if (shift_menu)
    {
      if ((menu_xpos + GTK_WIDGET (menu_button)->allocation.width + width) <= screen_width)
	menu_xpos += GTK_WIDGET (menu_button)->allocation.width;
      else
	menu_xpos -= width;
    }

  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + width) > screen_width)
    menu_xpos -= ((menu_xpos + width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;
#endif
  gdk_window_get_origin (GTK_WIDGET (menu_button)->window, x, y);
}

#if 0
GtkWidget*
sp_menu_button_get_menu (SPMenuButton *menu_button)
{
  g_return_val_if_fail (menu_button != NULL, NULL);
  g_return_val_if_fail (SP_IS_MENU_BUTTON (menu_button), NULL);

  return menu_button->menu;
}

static void
sp_menu_button_detacher (GtkWidget     *widget,
			  GtkMenu	*menu)
{
  SPMenuButton *menu_button;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));

  menu_button = SP_MENU_BUTTON (widget);
  g_return_if_fail (menu_button->menu == (GtkWidget*) menu);

  sp_menu_button_remove_contents (menu_button);
  gtk_signal_disconnect_by_data (GTK_OBJECT (menu_button->menu),
				 menu_button);
  
  menu_button->menu = NULL;
}

void
sp_menu_button_set_menu (SPMenuButton *menu_button,
			  GtkWidget     *menu)
{
  g_return_if_fail (menu_button != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (menu_button));
  g_return_if_fail (menu != NULL);
  g_return_if_fail (GTK_IS_MENU (menu));

  if (menu_button->menu != menu)
    {
      sp_menu_button_remove_menu (menu_button);

      menu_button->menu = menu;
      gtk_menu_attach_to_widget (GTK_MENU (menu),
				 GTK_WIDGET (menu_button),
				 sp_menu_button_detacher);

      sp_menu_button_calc_size (menu_button);

      gtk_signal_connect (GTK_OBJECT (menu_button->menu), "deactivate",
			  (GtkSignalFunc) sp_menu_button_deactivate,
			  menu_button);

      if (GTK_WIDGET (menu_button)->parent)
	gtk_widget_queue_resize (GTK_WIDGET (menu_button));

      sp_menu_button_update_contents (menu_button);
    }
}

void
sp_menu_button_remove_menu (SPMenuButton *menu_button)
{
  g_return_if_fail (menu_button != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (menu_button));

  if (menu_button->menu)
    gtk_menu_detach (GTK_MENU (menu_button->menu));
}

void
sp_menu_button_set_history (SPMenuButton *menu_button,
			     guint          index)
{
  GtkWidget *menu_item;

  g_return_if_fail (menu_button != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (menu_button));

  if (menu_button->menu)
    {
      gtk_menu_set_active (GTK_MENU (menu_button->menu), index);
      menu_item = gtk_menu_get_active (GTK_MENU (menu_button->menu));

      if (menu_item != menu_button->menu_item)
	sp_menu_button_update_contents (menu_button);
    }
}

static void
sp_menu_button_get_props (SPMenuButton       *menu_button,
			   SPMenuButtonProps  *props)
{
  GtkWidget *widget =  GTK_WIDGET (menu_button);

  props->indicator_width = gtk_style_get_prop_experimental (widget->style,
							    "SPMenuButton::indicator_width",
							    default_props.indicator_width);

  props->indicator_height = gtk_style_get_prop_experimental (widget->style,
							     "SPMenuButton::indicator_height",
							     default_props.indicator_height);

  props->indicator_top_spacing = gtk_style_get_prop_experimental (widget->style,
								  "SPMenuButton::indicator_top_spacing",
								  default_props.indicator_top_spacing);
  props->indicator_bottom_spacing = gtk_style_get_prop_experimental (widget->style,
								     "SPMenuButton::indicator_bottom_spacing",
								     default_props.indicator_bottom_spacing);
  props->indicator_left_spacing = gtk_style_get_prop_experimental (widget->style,
							       "SPMenuButton::indicator_left_spacing",
							       default_props.indicator_left_spacing);
  props->indicator_right_spacing = gtk_style_get_prop_experimental (widget->style,
								    "SPMenuButton::indicator_right_spacing",
								    default_props.indicator_right_spacing);
}

static void
sp_menu_button_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
  SPMenuButton *menu_button;
  SPMenuButtonProps props;
  gint tmp;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));
  g_return_if_fail (requisition != NULL);

  menu_button = SP_MENU_BUTTON (widget);

  sp_menu_button_get_props (menu_button, &props);

  requisition->width = ((GTK_CONTAINER (widget)->border_width +
			 GTK_WIDGET (widget)->style->klass->xthickness) * 2 +
			menu_button->width +
			props.indicator_width +
			props.indicator_left_spacing + props.indicator_right_spacing +
			CHILD_LEFT_SPACING + CHILD_RIGHT_SPACING + 2);
  requisition->height = ((GTK_CONTAINER (widget)->border_width +
			  GTK_WIDGET (widget)->style->klass->ythickness) * 2 +
			 menu_button->height +
			 CHILD_TOP_SPACING + CHILD_BOTTOM_SPACING + 2);

  tmp = (requisition->height - menu_button->height +
	 props.indicator_height + props.indicator_top_spacing + props.indicator_bottom_spacing);
  requisition->height = MAX (requisition->height, tmp);
}

static void
sp_menu_button_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GtkWidget *child;
  GtkAllocation child_allocation;
  SPMenuButtonProps props;
    
  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));
  g_return_if_fail (allocation != NULL);

  sp_menu_button_get_props (SP_MENU_BUTTON (widget), &props);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  child = GTK_BIN (widget)->child;
  if (child && GTK_WIDGET_VISIBLE (child))
    {
      child_allocation.x = (GTK_CONTAINER (widget)->border_width +
			    GTK_WIDGET (widget)->style->klass->xthickness) + 1;
      child_allocation.y = (GTK_CONTAINER (widget)->border_width +
			    GTK_WIDGET (widget)->style->klass->ythickness) + 1;
      child_allocation.width = MAX (1, (gint)allocation->width - child_allocation.x * 2 -
				    props.indicator_width - props.indicator_left_spacing - props.indicator_right_spacing -
				    CHILD_LEFT_SPACING - CHILD_RIGHT_SPACING - 2);
      child_allocation.height = MAX (1, (gint)allocation->height - child_allocation.y * 2 -
				     CHILD_TOP_SPACING - CHILD_BOTTOM_SPACING - 2);
      child_allocation.x += CHILD_LEFT_SPACING;
      child_allocation.y += CHILD_TOP_SPACING;

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static void
sp_menu_button_paint (GtkWidget    *widget,
		       GdkRectangle *area)
{
  GdkRectangle button_area;
  SPMenuButtonProps props;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      sp_menu_button_get_props (SP_MENU_BUTTON (widget), &props);

      button_area.x = GTK_CONTAINER (widget)->border_width + 1;
      button_area.y = GTK_CONTAINER (widget)->border_width + 1;
      button_area.width = widget->allocation.width - button_area.x * 2;
      button_area.height = widget->allocation.height - button_area.y * 2;

      /* This is evil, and should be elimated here and in the button
       * code. The point is to clear the focus, and make it
       * sort of transparent if it isn't there.
       */
      gdk_window_set_back_pixmap (widget->window, NULL, TRUE);
      gdk_window_clear_area (widget->window, area->x, area->y, area->width, area->height);

      gtk_paint_box(widget->style, widget->window,
		    GTK_WIDGET_STATE (widget), GTK_SHADOW_OUT,
		    area, widget, "optionmenu",
		    button_area.x, button_area.y,
		    button_area.width, button_area.height);
      
      gtk_paint_tab (widget->style, widget->window,
		     GTK_WIDGET_STATE (widget), GTK_SHADOW_OUT,
		     area, widget, "optionmenutab",
		     button_area.x + button_area.width - 
		     props.indicator_width - props.indicator_right_spacing -
		     widget->style->klass->xthickness,
		     button_area.y + (button_area.height - props.indicator_height) / 2,
		     props.indicator_width, props.indicator_height);
      
      if (GTK_WIDGET_HAS_FOCUS (widget))
       gtk_paint_focus (widget->style, widget->window,
			area, widget, "button",
			button_area.x - 1, 
			button_area.y - 1, 
			button_area.width + 1,
			button_area.height + 1);
    }
}

static void
sp_menu_button_draw (GtkWidget    *widget,
		      GdkRectangle *area)
{
  GtkWidget *child;
  GdkRectangle child_area;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      sp_menu_button_paint (widget, area);

      child = GTK_BIN (widget)->child;
      if (child && gtk_widget_intersect (child, area, &child_area))
	gtk_widget_draw (child, &child_area);
    }
}

static gint
sp_menu_button_expose (GtkWidget      *widget,
			GdkEventExpose *event)
{
  GtkWidget *child;
  GdkEventExpose child_event;
  gint remove_child;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (SP_IS_MENU_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      sp_menu_button_paint (widget, &event->area);


      /* The following code tries to draw the child in two places at
       * once. It fails miserably for several reasons
       *
       * - If the child is not no-window, removing generates
       *   more expose events. Bad, bad, bad.
       * 
       * - Even if the child is no-window, removing it now (properly)
       *   clears the space where it was, so it does no good
       */
      
#if 0
      remove_child = FALSE;
      child = GTK_BUTTON (widget)->child;

      if (!child)
	{
		child = children->data;
		
		if (active == child)
			break;
		
		if (GTK_WIDGET_VISIBLE (child))
		{
			gtk_widget_get_child_requisition (child, &requisition);
			menu_ypos -= requisition.height;
		}
		
		children = children->next;
	}
#endif
	
	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();
	
	shift_menu = FALSE;
	if (menu_ypos < 0)
	{
		menu_ypos = 0;
		shift_menu = TRUE;
	}
	else if ((menu_ypos + height) > screen_height)
	{
		menu_ypos -= ((menu_ypos + height) - screen_height);
		shift_menu = TRUE;
	}
	
	if (shift_menu)
	{
		if ((menu_xpos + GTK_WIDGET (menu_button)->allocation.width + width) <= screen_width)
			menu_xpos += GTK_WIDGET (menu_button)->allocation.width;
		else
			menu_xpos -= width;
	}
	
	if (menu_xpos < 0)
		menu_xpos = 0;
	else if ((menu_xpos + width) > screen_width)
		menu_xpos -= ((menu_xpos + width) - screen_width);
	
	*x = menu_xpos;
	*y = menu_ypos;
}


static void
sp_menu_button_hide_all (GtkWidget *widget)
{
  GtkContainer *container;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (SP_IS_MENU_BUTTON (widget));
  container = GTK_CONTAINER (widget);

  gtk_widget_hide (widget);
  gtk_container_foreach (container, (GtkCallback) gtk_widget_hide_all, NULL);
}
#endif


#define ASIZE 8
static void
sp_menu_button_draw_arrow (GtkWidget * widget)
{

	GtkWidget * drawing_target = NULL;
	
	/* FIXME: If you draw arrow on only child, drawing result 
	   is clipped out by parent window. The easiest solution is 
	   drawing twice, once parent and once child. */

	if (GTK_IS_BIN(widget) 
	    && GTK_BIN(widget)->child 
	    && GTK_WIDGET_DRAWABLE(GTK_BIN(widget)->child)) {
		drawing_target = GTK_BIN(widget)->child;
	} else if (GTK_WIDGET_DRAWABLE (widget)) {
		drawing_target = widget;
	}
	
	if (drawing_target) {
		gint w, h, bw, tx, ty;
		w = drawing_target->allocation.width;
		h = drawing_target->allocation.height;
		bw = GTK_CONTAINER (drawing_target)->border_width;
		tx = drawing_target->style->xthickness;
		ty = drawing_target->style->ythickness;
		gtk_draw_arrow (drawing_target->style, drawing_target->window,
				drawing_target->state, GTK_SHADOW_IN,
				GTK_ARROW_DOWN, TRUE,
				tx 
				- ((drawing_target == widget)?0 : (drawing_target->allocation.x)),
				h - 2 * bw - ty - ASIZE 
				+ ((drawing_target == widget)?0 : (drawing_target->allocation.y)),
				ASIZE, ASIZE);
	}
}
