#ifndef __SP_XML_TREE_H__
#define __SP_XML_TREE_H__

/*
 * XML tree editing dialog for Sodipodi
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2000-2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 *
 */

void sp_xml_tree_dialog (void);

/* Experimental */

#include <module.h>

/* Get new reference */
SPModule *sp_xml_module_load (void);
/* Unref module */
void sp_xml_module_unload (SPModule *module);

#endif
