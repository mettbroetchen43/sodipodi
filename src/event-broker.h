#ifndef __SP_EVENT_BROKER_H__
#define __SP_EVENT_BROKER_H__

/*
 * Event context stuff
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2000 Lauris Kaplinski
 * Copyright (C) 2002 Lauris Kaplinski
 *
 * Released under GNU GPL
 */

#include <glib.h>

void sp_event_context_set_select (gpointer data);
void sp_event_context_set_node_edit (gpointer data);
void sp_event_context_set_rect (gpointer data);
void sp_event_context_set_arc (gpointer data);
void sp_event_context_set_star (gpointer data);
void sp_event_context_set_spiral (gpointer data);
void sp_event_context_set_freehand (gpointer data);
void sp_event_context_set_pen (gpointer data);
void sp_event_context_set_dynahand (gpointer data);
void sp_event_context_set_text (gpointer data);
void sp_event_context_set_zoom (gpointer data);

#endif
