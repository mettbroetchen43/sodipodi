#ifndef SP_FILE_H
#define SP__FILE_H

void sp_file_new (void);

void sp_file_open (void);

void sp_file_save (GtkWidget * widget);

void sp_file_save_as (GtkWidget * widget);

void sp_file_import (GtkWidget * widget);
void sp_file_export (GtkWidget * widget);

void sp_file_print (GtkWidget * widget);

void sp_do_file_print (SPDocument * doc);

void sp_do_file_print_to_file (SPDocument * doc, gchar *filename);

void sp_file_exit (void);

#endif
