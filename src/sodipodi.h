#ifndef SODIPODI_H
#define SODIPODI_H

/*
 * sodipodi.h
 *
 * Signals:
 * "selection_changed"
 * "selection_set"
 * "eventcontext_set"
 * "new_desktop"
 * "destroy_desktop"
 * "desktop_activate"
 * "desktop_desactivate"
 * "new_document"
 * "destroy_document"
 * "document_activate"
 * "document_desactivate"
 *
 */

#define SP_TYPE_SODIPODI            (sodipodi_get_type ())
#define SP_SODIPODI(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_SODIPODI, SPObject))
#define SP_SODIPODI_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_SODIPODI, SPObjectClass))
#define SP_IS_SODIPODI(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_SODIPODI))
#define SP_IS_SODIPODI_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_SODIPODI))

#include <gtk/gtktypeutils.h>
#include <gtk/gtkobject.h>

#include "document.h"
#include "desktop.h"

typedef struct _Sodipodi Sodipodi;
typedef struct _SodipodiClass SodipodiClass;

GtkType sodipodi_get_type (void);

#define SODIPODI sodipodi

#ifndef SODIPODI_C
	extern Sodipodi * sodipodi;
#endif

#define SP_ACTIVE_EVENTCONTEXT sodipodi_active_event_context ()
SPEventContext * sodipodi_active_event_context (void);

#define SP_ACTIVE_DOCUMENT sodipodi_active_document ()
SPDocument * sodipodi_active_document (void);

#define SP_ACTIVE_DESKTOP sodipodi_active_desktop ()
SPDesktop * sodipodi_active_desktop (void);

#endif
