#ifndef __SP_FILE_H__
#define __SP_FILE_H__

/*
 * File/Print operations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Chema Celorio <chema@celorio.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"

void sp_file_new (void);

void sp_file_open (void);

void sp_file_save (GtkWidget * widget);
void sp_file_save_document (SPDocument *document);

void sp_file_save_as (GtkWidget * widget);

void sp_file_import (GtkWidget * widget);
void sp_file_export (GtkWidget * widget);

void sp_file_print (GtkWidget * widget);
void sp_file_print_preview (GtkWidget * widget);

void sp_do_file_print (SPDocument * doc);

void sp_do_file_print_to_file (SPDocument * doc, gchar *filename);
void sp_file_print_preview (GtkWidget * widget);

void sp_file_exit (void);

#endif
