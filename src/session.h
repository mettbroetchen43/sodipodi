#ifndef SESSION_H
#define SESSION_H

gint
sp_sm_save_yourself (GnomeClient * client,
	gint phase,
	GnomeSaveStyle save_style,
	gint is_shutdown,
	GnomeInteractStyle interact_style,
	gint is_fast,
	gpointer clinet_data);

gint
sp_sm_die (GnomeClient * client, gpointer client_data);

gint
sp_sm_restore_children (void);

#endif
