#define SP_QUICK_ALIGN_C

#include <glade/glade.h>
#include "align.h"
#include "../selection.h"
#include "../desktop.h"
#include "../desktop-handles.h"
#include "../mdi-desktop.h"
#include "../svg/svg.h"

/*
 * handler functions for quick align dialog
 *
 * todo: dialog with more aligns
 * - more aligns (30 % left from center ...)
 * - new align types (align to smaler object / chose objects to align to)
 * - aligns for nodes
 *
 */ 

GladeXML  * quick_align_xml = NULL;
GtkWidget * quick_align_dialog = NULL;

void
sp_quick_align_dialog (void)
{
	if (quick_align_dialog == NULL) {
		quick_align_xml = glade_xml_new (SODIPODI_GLADEDIR "/align.glade", "quick_align");
		if (quick_align_xml == NULL) return;
		glade_xml_signal_autoconnect (quick_align_xml);
		quick_align_dialog = glade_xml_get_widget (quick_align_xml, "quick_align");
		if (quick_align_dialog == NULL) return;
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


void
sp_move_item_rel (SPItem * item, double dx, double dy) {
  double move[6], newaff[6], curaff[6];
  char tstr[80];

  tstr[79] = '\0';

  // move item
  art_affine_translate (move, dx, dy);
  sp_item_i2d_affine (item, curaff);
  art_affine_multiply (newaff, curaff, move);
  sp_item_set_i2d_affine (item, newaff);    

  //update repr
  sp_svg_write_affine (tstr, 79, item->affine);
  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
} 


void
sp_quick_align_top_in (void) {
  double h,dy;
  double maxh = 0;
  double maxh_y = -1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, * maxitem = NULL;

  // get list of items in selection
  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  // compute biggest item in list
  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    h = (b.y1 - b.y0); 
    if (h > maxh) {
      maxh = h;
      maxh_y = b.y1;
      maxitem = item;
    }
  }
  
  // align al items in list
  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dy = maxh_y - b.y1;
      sp_move_item_rel(item,0,dy);
    }
  }  
  
  // update handels and undo
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_top_out (void) {
  double h,dy;
  double maxh = 0;
  double maxh_y = -1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, * maxitem = NULL;
  char tstr[80];
  

  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    h = (b.y1 - b.y0); 
    if (h > maxh) {
      maxh = h;
      maxh_y = b.y1;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dy = maxh_y - b.y0;
      sp_move_item_rel (item, 0, dy);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_right_in (void) {
  double w,dx;
  double maxw = 0;
  double maxw_x = -1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, * maxitem = NULL;
  char tstr[80];

  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    w = (b.x1 - b.x0); 
    if (w > maxw) {
      maxw = w;
      maxw_x = b.x1;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dx = maxw_x - b.x1;
      sp_move_item_rel (item, dx, 0);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_right_out (void) {
  double w,dx;
  double maxw = 0;
  double maxw_x = -1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, * maxitem = NULL;
  char tstr[80];

  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    w = (b.x1 - b.x0); 
    if (w > maxw) {
      maxw = w;
      maxw_x = b.x1;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dx = maxw_x - b.x0;
      sp_move_item_rel (item, dx, 0);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_bottom_in (void) {
  double h,dy;
  double maxh = 0;
  double maxh_y = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    h = (b.y1 - b.y0); 
    if (h > maxh) {
      maxh = h;
      maxh_y = b.y0;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dy = maxh_y - b.y0;
      sp_move_item_rel (item, 0, dy);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_bottom_out (void) {
  double h,dy;
  double maxh = 0;
  double maxh_y = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    h = (b.y1 - b.y0); 
    if (h > maxh) {
      maxh = h;
      maxh_y = b.y0;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dy = maxh_y - b.y1;
      sp_move_item_rel (item, 0, dy);
     }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_left_in (void) {
  double w,dx;
  double maxw = 0;
  double maxw_x = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    w = (b.x1 - b.x0); 
    if (w > maxw) {
      maxw = w;
      maxw_x = b.x0;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dx = maxw_x - b.x0;
      sp_move_item_rel (item, dx, 0);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}

void
sp_quick_align_left_out (void) {
  double w,dx;
  double maxw = 0;
  double maxw_x = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    w = (b.x1 - b.x0); 
    if (w > maxw) {
      maxw = w;
      maxw_x = b.x0;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dx = maxw_x - b.x1;
      sp_move_item_rel (item, dx, 0);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}


void
sp_quick_align_center_hor (void) {
  double w,dx;
  double maxw = 0;
  double mid_x = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    w = (b.x1 - b.x0); 
    if (w > maxw) {
      maxw = w;
      mid_x = b.x0 + (b.x1 - b.x0)/2;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dx = mid_x - (b.x0 + (b.x1 -b.x0)/2);
      sp_move_item_rel (item, dx, 0);
   }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}

void
sp_quick_align_center_ver (void) {
  double h,dy;
  double maxh = 0;
  double mid_y = 1e18;
  SPSelection * al_sel;
  SPDesktop * al_desk;
  GSList * al_list;
  GSList * l2;
  ArtDRect b;
  SPItem * item, *maxitem = NULL;
  gchar tstr[80];


  al_desk = SP_ACTIVE_DESKTOP;
  g_return_if_fail (al_desk != NULL);
  al_sel = SP_DT_SELECTION (al_desk);
  if (sp_selection_is_empty (al_sel)) return;
  al_list = al_sel->items;
  if (al_list->next == NULL) return;

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    sp_item_bbox (item, &b);
    h = (b.y1 - b.y0); 
    if (h > maxh) {
      maxh = h;
      mid_y = b.y0 + (b.y1 - b.y0)/2;
      maxitem = item;
    }
  }
  
  tstr[79] = '\0';

  for (l2 = al_list; l2 != NULL; l2 = l2-> next) {
    item = SP_ITEM (l2->data);
    if (item != maxitem) {
      sp_item_bbox (item, &b);
      dy = mid_y - (b.y0 + (b.y1 -b.y0)/2);
      sp_move_item_rel (item, 0, dy);
    }
  }  
  
  sp_selection_changed (al_sel);
  sp_document_done (SP_DT_DOCUMENT (al_desk));
}
