#define __SP_EVENT_BROKER_C__

/*
 * Event context stuff
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "select-context.h"
#include "node-context.h"
#include "rect-context.h"
#include "arc-context.h"
#include "star-context.h"
#include "spiral-context.h"
#include "draw-context.h"
#include "dyna-draw-context.h"
#include "text-context.h"
#include "zoom-context.h"
#include "sodipodi-private.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "event-broker.h"

void
sp_event_context_set_select (gpointer data)
{
	if (SP_ACTIVE_DESKTOP) {
		sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_SELECT_CONTEXT);
		/* fixme: This is really ugly hack. We should bind bind and unbind class methods */
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, TRUE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
	}
}

void
sp_event_context_set_node_edit (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_NODE_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, TRUE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_rect (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_RECT_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_arc (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_ARC_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_star (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_STAR_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_spiral (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_SPIRAL_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_freehand (gpointer data)
{
	if (SP_ACTIVE_DESKTOP) {
		sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_PENCIL_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
	}
}

void
sp_event_context_set_pen (gpointer data)
{
	if (SP_ACTIVE_DESKTOP) {
		sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_PEN_CONTEXT);
		/* fixme: */
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
	}
}

void
sp_event_context_set_dynahand (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_DYNA_DRAW_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_text (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_TEXT_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

void
sp_event_context_set_zoom (gpointer data)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_ZOOM_CONTEXT);
		sp_desktop_activate_guides (SP_ACTIVE_DESKTOP, FALSE);
    sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (SP_ACTIVE_DESKTOP));
  }
}

