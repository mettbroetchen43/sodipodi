#ifndef __SODIPODI_PRIVATE_H__
#define __SODIPODI_PRIVATE_H__

/*
 * Some forward declarations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sodipodi.h"

Sodipodi * sodipodi_new ();

#define sodipodi_ref() gtk_object_ref (GTK_OBJECT (SODIPODI))
#define sodipodi_unref() gtk_object_unref (GTK_OBJECT (SODIPODI))

/*
 * These are meant solely for desktop, document etc. implementations
 */

void sodipodi_selection_modified (SPSelection *selection, guint flags);
void sodipodi_selection_changed (SPSelection * selection);
void sodipodi_selection_set (SPSelection * selection);
void sodipodi_eventcontext_set (SPEventContext * eventcontext);
void sodipodi_add_desktop (SPDesktop * desktop);
void sodipodi_remove_desktop (SPDesktop * desktop);
void sodipodi_activate_desktop (SPDesktop * desktop);
void sodipodi_add_document (SPDocument *document);
void sodipodi_remove_document (SPDocument *document);

#endif


