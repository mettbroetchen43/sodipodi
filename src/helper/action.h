#ifndef __SP_ACTION_H__
#define __SP_ACTION_H__

/*
 * Sodipodi UI action implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Lauris Kaplinski
 *
 * This code is in public domain
 */

#define SP_TYPE_ACTION (sp_action_get_type ())
#define SP_ACTION(o) (NR_CHECK_INSTANCE_CAST ((o), SP_TYPE_ACTION, SPAction))
#define SP_IS_ACTION(o) (NR_CHECK_INSTANCE_TYPE ((o), SP_TYPE_ACTION))

typedef struct _SPAction SPAction;
typedef struct _SPActionClass SPActionClass;

#include <libnr/nr-object.h>

/* We define high-bit mask for packing into single int */

#define SP_ACTION_SHIFT_MASK (1 << 24)
#define SP_ACTION_CONTROL_MASK (1 << 25)
#define SP_ACTION_ALT_MASK (1 << 26)

typedef struct _SPActionEventVector SPActionEventVector;

struct _SPActionEventVector {
	NRObjectEventVector object_vector;
	void (* perform) (SPAction *action, void *data);
	void (* set_sensitive) (SPAction *action, unsigned int sensitive, void *data);
	void (* set_shortcut) (SPAction *action, unsigned int shortcut, void *data);
};

struct _SPAction {
	NRActiveObject object;
	unsigned int sensitive : 1;
	unsigned char *id;
	unsigned char *name;
	unsigned char *tip;
	unsigned char *image;
	unsigned int shortcut;
};

struct _SPActionClass {
	NRActiveObjectClass parent_class;
};

unsigned int sp_action_get_type (void);

SPAction *sp_action_setup (SPAction *action,
			   const unsigned char *id,
			   const unsigned char *name,
			   const unsigned char *tip,
			   const unsigned char *image);

void sp_action_perform (SPAction *action);
void sp_action_set_sensitive (SPAction *action, unsigned int sensitive);
void sp_action_set_shortcut (SPAction *action, unsigned int shortcut);

#endif
