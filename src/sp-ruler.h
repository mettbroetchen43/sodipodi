/*
 */

#ifndef __SP_RULER_H__
#define __SP_RULER_H__

#include "sp-metrics.h"


GtkWidget* sp_hruler_new (void);
GtkWidget* sp_vruler_new (void);

void sp_ruler_set_metric (GtkRuler * ruler, SPMetric  metric);


#endif /* __SP_RULER_H__ */
