#ifndef SP_OBJECT_REPR_H
#define SP_OBJECT_REPR_H

#include <xml/repr.h>
#include "sp-object.h"

SPObject * sp_object_repr_build_tree (SPDocument * document, SPRepr * repr);

GType sp_repr_type_lookup (SPRepr *repr);
GType sp_object_type_lookup (const guchar *name);

#endif
