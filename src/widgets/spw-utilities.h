#ifndef __SPW_UTILITIES_H__
#define __SPW_UTILITIES_H__

/*
 * Sodipodi Widget Utilities
 *
 * Authors:
 *   Bryce W. Harrington <brycehar@bryceharrington.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 * 
 * Copyright (C) 2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */



/* The following are helper routines for making Sodipodi dialog widgets.
   All are prefixed with spw_, short for sodipodi_widget.  This is not to
   be confused with SPWidget, an existing datatype associated with SPRepr/
   SPObject, that reacts to modification.
*/

void
spw_checkbutton (GtkWidget *dialog, GtkWidget *table,
		 const unsigned char *label,
		 const unsigned char *key,
		 int col, int row,
		 int sensitive, GCallback cb);

void
spw_dropdown(GtkWidget * dialog, GtkWidget * t,
	     const guchar * label, guchar * key, int row,
	     GtkWidget * selector
	     );

void
spw_unit_selector(GtkWidget * dialog, GtkWidget * t,
		  const guchar * label, guchar * key, int row,
		  GtkWidget * us, GCallback cb);

/* Config widgets */

GtkWidget *sp_config_check_button_new (const unsigned char *text,
				       const unsigned char *path, const unsigned char *key,
				       const unsigned char *trueval, const unsigned char *falseval);

#endif
