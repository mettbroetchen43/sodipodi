
#ifndef __SP_BEZIER_UTILS_H__
#define __SP_BEZIER_UTILS_H__

#include <glib.h>
#include <libart_lgpl/art_point.h>


gboolean sp_bezier_fit_cubic            (ArtPoint       *bezier,
                                         const ArtPoint *data,
                                         gint            first,
                                         gint            last,
                                         gdouble         error);

gint     sp_bezier_fit_cubic_r          (ArtPoint       *bezier,
                                         const ArtPoint *data,
                                         gint            first,
                                         gint            last,
                                         gdouble         error,
                                         gint            max_depth);

gint     sp_bezier_fit_cubic_full       (ArtPoint       *bezier,
                                         const ArtPoint *data,
                                         gint            first,
                                         gint            last,
                                         ArtPoint       *tHat1,
                                         ArtPoint       *tHat2,
                                         gdouble         error,
                                         gint            max_depth);



#endif /* __SP_BEZIER_UTILS_H__ */
