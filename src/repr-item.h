#ifndef SP_REPR_ITEM_H
#define SP_REPR_ITEM_H

/*
 * repr-item
 *
 * Holds a link between reprs and items
 *
 */

#include "sp-item.h"

/*
 * Creates new item tree from repr
 * Adds relevant signals & so on
 */

SPItem * sp_repr_item_create (SPRepr * repr);

#if 0
/*
 * Destroys item tree
 * Removes relevant signals & so on
 */

void sp_repr_item_destroy (SPItem * item);
#endif

#define sp_item_repr(item) (item)->repr

SPItem * sp_repr_item_last_item (void);

#endif
