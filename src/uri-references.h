#ifndef __SP_URI_REFERENCES_H__
#define __SP_URI_REFERENCES_H__

/*
 * Helper methods for resolving URI References
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Lauris Kaplinski and Ximian, Inc.
 *
 * Released under GNU GPL
 *
 */

#include <glib.h>
#include "forward.h"

SPObject *sp_uri_reference_resolve (SPDocument *document, const guchar *uri);

#endif
