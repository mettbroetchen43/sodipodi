#ifndef SP_KNOTHOLDER_H
#define SP_KNOTHOLDER_H

/*
 * SPKnotHolder - Hold SPKnot list and manage signals
 *
 * Author:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *
 * Copyright (C) 1999-2001 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2001 Mitsuru Oka
 *
 * Released under GNU GPL
 *
 */

#include <glib.h>
#include "knot.h"
#include "forward.h"
#include "desktop.h"


typedef void (* SPKnotHolderSetFunc) (SPItem *item, const ArtPoint *p, guint state);
typedef void (* SPKnotHolderGetFunc) (SPItem *item, ArtPoint *p);

typedef struct _SPKnotHolderEntity SPKnotHolderEntity;
typedef struct _SPKnotHolder       SPKnotHolder;


struct _SPKnotHolderEntity {
	SPKnot *knot;
	guint   handler_id;
	void (* knot_set) (SPItem *item, const ArtPoint *p, guint state);
	void (* knot_get) (SPItem *item, ArtPoint *p);
};

struct _SPKnotHolder {
	SPDesktop *desktop;
	SPItem *item;
	GSList *entity;
};


SPKnotHolder   *sp_knot_holder_new	(SPDesktop     *desktop,
					 SPItem	       *item);
void		sp_knot_holder_destroy	(SPKnotHolder       *knots);


void		sp_knot_holder_add	(SPKnotHolder       *knots,
					 SPKnotHolderSetFunc knot_set,
					 SPKnotHolderGetFunc knot_get);

/* possibly private functions */



#endif
