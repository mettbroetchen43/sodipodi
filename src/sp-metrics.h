#ifndef SP_METRICS_H
#define SP_METRICS_H

#include "svg/svg.h"

// known metrics so far
// should be extended with meter, feet, yard etc
typedef enum {
        NONE,
	SP_MM,
	SP_CM,
	SP_IN,
	SP_PT
} SPMetric;

// this should be configurable in central place
#define SP_DEFAULT_METRIC SP_MM

gdouble sp_absolute_metric_to_metric (gdouble length_src, const SPMetric metric_src, const SPMetric metric_dst);
GString * sp_metric_to_metric_string (gdouble length,  const SPMetric metric_src, const SPMetric metric_dst);

// convenience since we mostly deal with points
#define SP_METRIC_TO_PT(l,m) sp_absolute_metric_to_metric(l,m,SP_PT);
#define SP_PT_TO_METRIC(l,m) sp_absolute_metric_to_metric(l,SP_PT,m);

#define SP_PT_TO_METRIC_STRING(l,m) sp_metric_to_metric_string(l,SP_PT,m)



#endif
