#ifndef SP_DESKTOP_HANDLES_H
#define SP_DESKTOP_HANDLES_H

#include <libgnomeui/gnome-canvas.h>

#ifndef SP_DOCUMENT_DEFINED
#define SP_DOCUMENT_DEFINED
typedef struct _SPDocument SPDocument;
#endif

#ifndef SP_SELECTION_DEFINED
#define SP_SELECTION_DEFINED
typedef struct _SPSelection SPSelection;
#endif

#ifndef SP_DESKTOP_DEFINED
#define SP_DESKTOP_DEFINED
typedef struct _SPDesktop SPDesktop;
typedef struct _SPDesktopClass SPDesktopClass;
#endif

#define SP_DT_IS_EDITABLE(d) (TRUE)

#define SP_DT_SELECTION(d) sp_desktop_selection (d)
#define SP_DT_DOCUMENT(d) sp_desktop_document (d)
#define SP_DT_CANVAS(d) sp_desktop_canvas (d)
#define SP_DT_ACETATE(d) sp_desktop_acetate (d)
#define SP_DT_MAIN(d) sp_desktop_main (d)
#define SP_DT_GRID(d) sp_desktop_grid (d)
#define SP_DT_GUIDES(d) sp_desktop_guides (d)
#define SP_DT_DRAWING(d) sp_desktop_drawing (d)
#define SP_DT_SKETCH(d) sp_desktop_sketch (d)
#define SP_DT_CONTROLS(d) sp_desktop_controls (d)

SPSelection * sp_desktop_selection (SPDesktop * desktop);
SPDocument * sp_desktop_document (SPDesktop * desktop);
GnomeCanvas * sp_desktop_canvas (SPDesktop * desktop);
GnomeCanvasItem * sp_desktop_acetate (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_main (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_grid (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_guides (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_drawing (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_sketch (SPDesktop * desktop);
GnomeCanvasGroup * sp_desktop_controls (SPDesktop * desktop);

#endif
