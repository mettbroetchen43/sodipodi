#ifndef __SP_VERBS_H__
#define __SP_VERBS_H__

/*
 * Frontend to actions
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

#include "forward.h"
#include "helper/action.h"

SPAction *sp_verb_get_action (unsigned int verb);

void sp_shortcut_set_verb (unsigned int shortcut, unsigned int verb, unsigned int primary);
void sp_shortcut_remove_verb (unsigned int shortcut);
unsigned int sp_shortcut_get_verb (unsigned int shortcut);

enum {
	/* Header */
	SP_VERB_INVALID,
	SP_VERB_NONE,
	/* Event contexts */
	SP_VERB_CONTEXT_SELECT,
	SP_VERB_CONTEXT_NODE,
	SP_VERB_CONTEXT_RECT,
	SP_VERB_CONTEXT_ARC,
	SP_VERB_CONTEXT_STAR,
	SP_VERB_CONTEXT_SPIRAL,
	SP_VERB_CONTEXT_PENCIL,
	SP_VERB_CONTEXT_PEN,
	SP_VERB_CONTEXT_CALLIGRAPHIC,
	SP_VERB_CONTEXT_TEXT,
	SP_VERB_CONTEXT_ZOOM,
	SP_VERB_CONTEXT_DROPPER,
	/* Zooming */
	SP_VERB_ZOOM_IN,
	SP_VERB_ZOOM_OUT,
	SP_VERB_ZOOM_1_1,
	SP_VERB_ZOOM_1_2,
	SP_VERB_ZOOM_2_1,
	SP_VERB_ZOOM_PAGE,
	SP_VERB_ZOOM_DRAWING,
	SP_VERB_ZOOM_SELECTION,
	/* Footer */
	SP_VERB_LAST
};

#endif
