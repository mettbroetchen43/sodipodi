#ifndef SP_STAR_H
#define SP_STAR_H

#include "sp-polygon.h"

BEGIN_GNOME_DECLS

#define SP_TYPE_STAR            (sp_star_get_type ())
#define SP_STAR(obj)            (GTK_CHECK_CAST ((obj), SP_TYPE_STAR, SPStar))
#define SP_STAR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_STAR, SPStarClass))
#define SP_IS_STAR(obj)         (GTK_CHECK_TYPE ((obj), SP_TYPE_STAR))
#define SP_IS_STAR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_STAR))

typedef struct _SPStar SPStar;
typedef struct _SPStarClass SPStarClass;

struct _SPStar {
	SPPolygon polygon;

	gint   sides;
	ArtPoint center;
	double r1;
	double r2;
	double arg1;
	double arg2;
};

struct _SPStarClass {
	SPPolygonClass parent_class;
};


/* Standard Gtk function */
GtkType sp_star_get_type (void);

void    sp_star_set             (SPStar         *star,
                                 gint            sides,
                                 gdouble         cx,
                                 gdouble         cy,
                                 gdouble         r1,
                                 gdouble         r2,
                                 gdouble         arg1,
                                 gdouble         arg2);


END_GNOME_DECLS

#endif
