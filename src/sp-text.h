#ifndef SP_TEXT_H
#define SP_TEXT_H

#include "sp-chars.h"

#define SP_TYPE_TEXT            (sp_text_get_type ())
#define SP_TEXT(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_TEXT, SPText))
#define SP_TEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_TEXT, SPTextClass))
#define SP_IS_TEXT(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_TEXT))
#define SP_IS_TEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_TEXT))

typedef struct _SPText SPText;
typedef struct _SPTextClass SPTextClass;

struct _SPText {
	SPChars chars;

	gdouble x, y;

	gchar * text;
	gchar * fontname;
	GnomeFontWeight weight;
	gboolean italic;
	SPFont * font;
	double size;
};

struct _SPTextClass {
	SPCharsClass parent_class;
};


/* Standard Gtk function */
GtkType sp_text_get_type (void);

#endif
