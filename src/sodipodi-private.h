#ifndef SODIPODI_PRIVATE_H
#define SODIPODI_PRIVATE_H

#include "xml/repr.h"
#include "sodipodi.h"

Sodipodi * sodipodi_new ();

#define sodipodi_ref() gtk_object_ref (GTK_OBJECT (SODIPODI))
#define sodipodi_unref() gtk_object_unref (GTK_OBJECT (SODIPODI))

/*
 * These are meant solely for desktop, document etc. implementations
 */

void sodipodi_selection_modified (SPSelection *selection);
void sodipodi_selection_changed (SPSelection * selection);
void sodipodi_selection_set (SPSelection * selection);
void sodipodi_eventcontext_set (SPEventContext * eventcontext);
void sodipodi_add_desktop (SPDesktop * desktop);
void sodipodi_remove_desktop (SPDesktop * desktop);
void sodipodi_activate_desktop (SPDesktop * desktop);
void sodipodi_add_document (SPDocument *document);
void sodipodi_remove_document (SPDocument *document);

#endif


