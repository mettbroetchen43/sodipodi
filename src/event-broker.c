#define SP_EVENT_BROKER_C

#include "select-context.h"
#include "node-context.h"
#include "rect-context.h"
#include "ellipse-context.h"
#include "draw-context.h"
#include "text-context.h"
#include "zoom-context.h"
#include "mdi-desktop.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "event-broker.h"

void
sp_event_context_set_select (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_SELECT_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_node_edit (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_NODE_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_rect (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_RECT_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_ellipse (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_ELLIPSE_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_freehand (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_DRAW_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_text (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_TEXT_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

void
sp_event_context_set_zoom (GtkWidget * widget)
{
  if (SP_ACTIVE_DESKTOP) {
    sp_desktop_set_event_context (SP_ACTIVE_DESKTOP, SP_TYPE_ZOOM_CONTEXT);
    sp_update_draw_toolbox (SP_ACTIVE_DESKTOP);
  }
}

