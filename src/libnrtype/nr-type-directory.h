#ifndef __NR_TYPE_DIRECTORY_H__
#define __NR_TYPE_DIRECTORY_H__

/*
 * Typeface and script library
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnrtype/nr-typeface.h>

NRTypeFace *nr_type_directory_lookup_fuzzy (const unsigned char *family, const unsigned char *style);

#endif
