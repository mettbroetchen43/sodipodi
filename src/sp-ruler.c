#include "sp-ruler.h"
#include "gtk/gtkhruler.h"
#include "gtk/gtkvruler.h"

GtkWidget*
sp_hruler_new (void)
{
  return gtk_hruler_new ();
}


GtkWidget*
sp_vruler_new (void)
{
  return gtk_vruler_new ();
}


void
sp_ruler_set_metric (GtkRuler *ruler,
		     SPMetric  metric)
{
  g_return_if_fail (ruler != NULL);
  g_return_if_fail (GTK_IS_RULER (ruler));

  ruler->metric = (GtkRulerMetric *) &sp_ruler_metrics[metric];

  if (GTK_WIDGET_DRAWABLE (ruler))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

