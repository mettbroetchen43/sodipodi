#ifndef SP_CHARS_H
#define SP_CHARS_H

#include "sp-shape.h"
#include "font-wrapper.h"

#define SP_TYPE_CHARS            (sp_chars_get_type ())
#define SP_CHARS(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_CHARS, SPChars))
#define SP_CHARS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_CHARS, SPCharsClass))
#define SP_IS_CHARS(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_CHARS))
#define SP_IS_CHARS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_CHARS))

typedef struct _SPChars SPChars;
typedef struct _SPCharsClass SPCharsClass;

typedef struct {
	guint glyph;
	SPFont * font;
	double affine[6];
} SPCharElement;

struct _SPChars {
	SPShape shape;
	GList * element;
};

struct _SPCharsClass {
	SPShapeClass parent_class;
};


/* Standard Gtk function */
GtkType sp_chars_get_type (void);

void sp_chars_clear (SPChars * chars);

void sp_chars_add_element (SPChars * chars, guint glyph, SPFont * font, double affine[]);

#endif
