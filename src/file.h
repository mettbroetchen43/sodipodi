#ifndef __SP_FILE_H__
#define __SP_FILE_H__

/*
 * File/Print operations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Chema Celorio <chema@celorio.com>
 *
 * Copyright (C) 1999-2002 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"

void sp_file_new (void);

void sp_file_open (const guchar *uri);

void sp_file_open_dialog (gpointer object, gpointer data);

void sp_file_save (GtkWidget *widget);
void sp_file_save_document (SPDocument *document);

void sp_file_save_as (GtkWidget * widget);

void sp_file_import (GtkWidget * widget);

void sp_file_print (GtkWidget * widget);
void sp_file_print_preview (GtkWidget * widget);

void sp_do_file_print (SPDocument * doc);

void sp_do_file_print_to_file (SPDocument * doc, gchar *filename);
void sp_file_print_preview (GtkWidget * widget);

void sp_file_exit (void);

void sp_file_export_dialog (void *widget);
void sp_export_png_file (SPDocument *doc, unsigned char *filename,
			 double x0, double y0, double x1, double y1,
			 unsigned int width, unsigned int height,
			 unsigned long bgcolor);

#endif
