#define __SP_VERBS_C__

/*
 * Actions for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include <assert.h>

#include "helper/sp-intl.h"

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
#include "dropper-context.h"

#include "sodipodi-private.h"
#include "document.h"
#include "desktop.h"
#include "selection-chemistry.h"
#include "shortcuts.h"

#include "verbs.h"

static void sp_verbs_init (void);

static SPAction *verb_actions = NULL;

SPAction *
sp_verb_get_action (unsigned int verb)
{
	if (!verb_actions) sp_verbs_init ();
	return &verb_actions[verb];
}

static void
sp_verb_action_set_shortcut (SPAction *action, unsigned int shortcut, void *data)
{
	unsigned int verb, ex;
	verb = (unsigned int) data;
	ex = sp_shortcut_get_verb (shortcut);
	if (verb != ex) sp_shortcut_set_verb (shortcut, verb, FALSE);
}

static void
sp_verb_action_ctx_perform (SPAction *action, void *data)
{
	SPDesktop *dt;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;

	switch ((int) data) {
	case SP_VERB_CONTEXT_SELECT:
		sp_desktop_set_event_context (dt, SP_TYPE_SELECT_CONTEXT, "tools.select");
		/* fixme: This is really ugly hack. We should bind and unbind class methods */
		sp_desktop_activate_guides (dt, TRUE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_NODE:
		sp_desktop_set_event_context (dt, SP_TYPE_NODE_CONTEXT, "tools.nodes");
		sp_desktop_activate_guides (dt, TRUE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_RECT:
		sp_desktop_set_event_context (dt, SP_TYPE_RECT_CONTEXT, "tools.shapes.rect");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_ARC:
		sp_desktop_set_event_context (dt, SP_TYPE_ARC_CONTEXT, "tools.shapes.arc");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_STAR:
		sp_desktop_set_event_context (dt, SP_TYPE_STAR_CONTEXT, "tools.shapes.star");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_SPIRAL:
		sp_desktop_set_event_context (dt, SP_TYPE_SPIRAL_CONTEXT, "tools.shapes.spiral");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_PENCIL:
		sp_desktop_set_event_context (dt, SP_TYPE_PENCIL_CONTEXT, "tools.freehand.pencil");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_PEN:
		sp_desktop_set_event_context (dt, SP_TYPE_PEN_CONTEXT, "tools.freehand.pen");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_CALLIGRAPHIC:
		sp_desktop_set_event_context (dt, SP_TYPE_DYNA_DRAW_CONTEXT, "tools.calligraphic");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_TEXT:
		sp_desktop_set_event_context (dt, SP_TYPE_TEXT_CONTEXT, "tools.text");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_ZOOM:
		sp_desktop_set_event_context (dt, SP_TYPE_ZOOM_CONTEXT, "tools.zoom");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	case SP_VERB_CONTEXT_DROPPER:
		sp_desktop_set_event_context (dt, SP_TYPE_DROPPER_CONTEXT, "tools.dropper");
		sp_desktop_activate_guides (dt, FALSE);
		sodipodi_eventcontext_set (SP_DT_EVENTCONTEXT (dt));
		break;
	default:
		break;
	}
}

static void
sp_verb_action_zoom_perform (SPAction *action, void *data)
{
	SPDesktop *dt;
	NRRectF d;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;

	switch ((int) data) {
	case SP_VERB_ZOOM_IN:
		sp_desktop_get_display_area (dt, &d);
		sp_desktop_zoom_relative (dt, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, SP_DESKTOP_ZOOM_INC);
		break;
	case SP_VERB_ZOOM_OUT:
		sp_desktop_get_display_area (dt, &d);
		sp_desktop_zoom_relative (dt, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 1 / SP_DESKTOP_ZOOM_INC);
		break;
	case SP_VERB_ZOOM_1_1:
		sp_desktop_get_display_area (dt, &d);
		sp_desktop_zoom_absolute (dt, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 1.0);
		break;
	case SP_VERB_ZOOM_1_2:
		sp_desktop_get_display_area (dt, &d);
		sp_desktop_zoom_absolute (dt, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 0.5);
		break;
	case SP_VERB_ZOOM_2_1:
		sp_desktop_get_display_area (dt, &d);
		sp_desktop_zoom_absolute (dt, (d.x0 + d.x1) / 2, (d.y0 + d.y1) / 2, 2.0);
		break;
	case SP_VERB_ZOOM_PAGE:
		sp_desktop_zoom_page (dt);
		break;
	case SP_VERB_ZOOM_DRAWING:
		sp_desktop_zoom_drawing (dt);
		break;
	case SP_VERB_ZOOM_SELECTION:
		sp_desktop_zoom_selection (dt);
		break;
	default:
		break;
	}
}

static void
sp_verb_action_edit_perform (SPAction *action, void *data)
{
	SPDesktop *dt;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;

	switch ((int) data) {
	case SP_VERB_EDIT_UNDO:
		sp_document_undo (SP_DT_DOCUMENT (dt));
		break;
	case SP_VERB_EDIT_REDO:
		sp_document_redo (SP_DT_DOCUMENT (dt));
		break;
	case SP_VERB_EDIT_CUT:
		sp_selection_cut (NULL);
		break;
	case SP_VERB_EDIT_COPY:
		sp_selection_copy (NULL);
		break;
	case SP_VERB_EDIT_PASTE:
		sp_selection_paste (NULL);
		break;
	case SP_VERB_EDIT_DELETE:
		sp_selection_delete (NULL, NULL);
		break;
	case SP_VERB_EDIT_DUPLICATE:
		sp_selection_duplicate (NULL, NULL);
		break;
	default:
		break;
	}
}

static SPActionEventVector action_ctx_vector = {{NULL}, sp_verb_action_ctx_perform, NULL, sp_verb_action_set_shortcut};
static SPActionEventVector action_zoom_vector = {{NULL}, sp_verb_action_zoom_perform, NULL, sp_verb_action_set_shortcut};
static SPActionEventVector action_edit_vector = {{NULL}, sp_verb_action_edit_perform, NULL, sp_verb_action_set_shortcut};

#define SP_VERB_IS_CONTEXT(v) ((v >= SP_VERB_CONTEXT_SELECT) && (v <= SP_VERB_CONTEXT_DROPPER))
#define SP_VERB_IS_ZOOM(v) ((v >= SP_VERB_ZOOM_IN) && (v <= SP_VERB_ZOOM_SELECTION))
#define SP_VERB_IS_EDIT(v) ((v >= SP_VERB_EDIT_UNDO) && (v <= SP_VERB_EDIT_DUPLICATE))

typedef struct {
	unsigned int code;
	const unsigned char *id;
	const unsigned char *name;
	const unsigned char *tip;
	const unsigned char *image;
} SPVerbActionDef;

static const SPVerbActionDef props[] = {
	/* Header */
	{SP_VERB_INVALID, NULL, NULL, NULL, NULL},
	{SP_VERB_NONE, "None", N_("None"), N_("Does nothing"), NULL},
	/* Event contexts */
	{SP_VERB_CONTEXT_SELECT, "DrawSelect", N_("Select"), N_("Select and transform objects"), "draw_select"},
	{SP_VERB_CONTEXT_NODE, "DrawNode", N_("Node edit"), N_("Modify existing objects by control nodes"), "draw_node"},
	{SP_VERB_CONTEXT_RECT, "DrawRect", N_("Rectangle"), N_("Create rectangles and squares with optional rounded corners"), "draw_rect"},
	{SP_VERB_CONTEXT_ARC, "DrawArc", N_("Ellipse"), N_("Create circles, ellipses and arcs"), "draw_arc"},
	{SP_VERB_CONTEXT_STAR, "DrawStar", N_("Star"), N_("Create stars and polygons"), "draw_star"},
	{SP_VERB_CONTEXT_SPIRAL, "DrawSpiral", N_("Spiral"), N_("Create spirals"), "draw_spiral"},
	{SP_VERB_CONTEXT_PENCIL, "DrawPencil", N_("Pencil"), N_("Draw freehand curves and straight lines"), "draw_freehand"},
	{SP_VERB_CONTEXT_PEN, "DrawPen", N_("Pen"), N_("Draw precisely positioned curved and straight lines"), "draw_pen"},
	{SP_VERB_CONTEXT_CALLIGRAPHIC, "DrawCalligrphic", N_("Calligraphy"), N_("Draw calligraphic lines"), "draw_dynahand"},
	{SP_VERB_CONTEXT_TEXT, "DrawText", N_("Text"), N_("Create and edit text objects"), "draw_text"},
	{SP_VERB_CONTEXT_ZOOM, "DrawZoom", N_("Zoom"), N_("Zoom into preciely selected area"), "draw_zoom"},
	{SP_VERB_CONTEXT_DROPPER, "DrawDropper", N_("Dropper"), N_("Pick averaged colors from image"), "draw_dropper"},
	/* Zooming */
	{SP_VERB_ZOOM_IN, "ZoomIn", N_("In"), N_("Zoom in drawing"), "zoom_in"},
	{SP_VERB_ZOOM_OUT, "ZoomOut", N_("Out"), N_("Zoom out drawing"), "zoom_out"},
	{SP_VERB_ZOOM_1_1, "Zoom1:0", N_("1:1"), N_("Set zoom factor to 1:1"), "zoom_1_to_1"},
	{SP_VERB_ZOOM_1_2, "Zoom1:2", N_("1:2"), N_("Set zoom factor to 1:2"), "zoom_1_to_2"},
	{SP_VERB_ZOOM_2_1, "Zoom2:1", N_("2:1"), N_("Set zoom factor to 2:1"), "zoom_2_to_1"},
	{SP_VERB_ZOOM_PAGE, "ZoomPage", N_("Page"), N_("Fit the whole page into window"), "zoom_page"},
	{SP_VERB_ZOOM_DRAWING, "ZoomDrawing", N_("Drawing"), N_("Fit the whole drawing into window"), "zoom_draw"},
	{SP_VERB_ZOOM_SELECTION, "ZoomSelection", N_("Selection"), N_("Fit the whole selection into window"), "zoom_select"},
	/* Edit */
	{SP_VERB_EDIT_UNDO, "EditUndo", N_("Undo"), N_("Revert last action"), "edit_undo"},
	{SP_VERB_EDIT_REDO, "EditRedo", N_("Redo"), N_("Do again undone action"), "edit_redo"},
	{SP_VERB_EDIT_CUT, "EditCut", N_("Cut"), N_("Cut selected objects to clipboard"), "edit_cut"},
	{SP_VERB_EDIT_COPY, "EditCopy", N_("Copy"), N_("Copy selected objects to clipboard"), "edit_copy"},
	{SP_VERB_EDIT_PASTE, "EditPaste", N_("Paste"), N_("Paste objects from clipboard"), "edit_paste"},
	{SP_VERB_EDIT_DELETE, "EditDelete", N_("Delete"), N_("Delete selected objects"), "edit_delete"},
	{SP_VERB_EDIT_DUPLICATE, "EditDuplicate", N_("Duplicate"), N_("Duplicate selected objects"), "edit_duplicate"},
	/* Footer */
	{SP_VERB_LAST, NULL, NULL, NULL, NULL}
};

static void
sp_verbs_init (void)
{
	int v;
	verb_actions = nr_new (SPAction, SP_VERB_LAST);
	for (v = 0; v < SP_VERB_LAST; v++) {
		assert (props[v].code == v);
		sp_action_setup (&verb_actions[v], props[v].id, props[v].name, props[v].tip, props[v].image);
		/* fixme: Make more elegant (Lauris) */
		if (SP_VERB_IS_CONTEXT (v)) {
			nr_active_object_add_listener ((NRActiveObject *) &verb_actions[v],
						       (NRObjectEventVector *) &action_ctx_vector,
						       sizeof (SPActionEventVector),
						       (void *) v);
		} else if (SP_VERB_IS_ZOOM (v)) {
			nr_active_object_add_listener ((NRActiveObject *) &verb_actions[v],
						       (NRObjectEventVector *) &action_zoom_vector,
						       sizeof (SPActionEventVector),
						       (void *) v);
		} else if (SP_VERB_IS_EDIT (v)) {
			nr_active_object_add_listener ((NRActiveObject *) &verb_actions[v],
						       (NRObjectEventVector *) &action_edit_vector,
						       sizeof (SPActionEventVector),
						       (void *) v);
		}
	}
}
