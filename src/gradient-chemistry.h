#ifndef __SP_GRADIENT_CHEMISTRY_H__
#define __SP_GRADIENT_CHEMISTRY_H__

/*
 * Various utility methods for gradients
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include "forward.h"
#include "sp-gradient.h"

/*
 * Either normalizes given gradient to vector, or returns fresh normalized
 * vector - in latter case, original gradient is flattened and stops cleared
 */

SPGradient *sp_gradient_ensure_vector_normalized (SPGradient *gradient);

/*
 * Either normalizes given gradient to private, or returns fresh normalized
 * private - gradient is flattened in any case, and vector set
 * Vector has to be normalized beforehand
 */

SPGradient *sp_gradient_ensure_private_normalized (SPGradient *gradient, SPGradient *vector);

/*
 * Sets item fill to lineargradient with given vector, creating
 * new private gradient, if needed
 * gr has to be normalized vector
 */

void sp_item_force_fill_lineargradient_vector (SPItem *item, SPGradient *gradient);

/*
 * Get default normalized gradient vector of document, create if there is none
 */

SPGradient *sp_document_default_gradient_vector (SPDocument *document);

void sp_object_ensure_fill_gradient_normalized (SPObject *object);
void sp_object_ensure_stroke_gradient_normalized (SPObject *object);

#endif
