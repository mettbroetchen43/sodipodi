#ifndef SP_TRANSFORMATION_H
#define SP_TRANSFORMATION_H

#include "../sp-metrics.h"

void sp_transformation_dialog (void);

// dialog invocation and close can be called from menues etc
void sp_transformation_dialog_move (void);
void sp_transformation_dialog_scale (void);
void sp_transformation_dialog_rotate (void);
void sp_transformation_dialog_skew (void);
void sp_transformation_dialog_close (void);

typedef enum {
  ABSOLUTE,
  RELATIVE
} SPTransformationType;

typedef enum {
  SELECTION,
  DESKTOP
} SPReferType;

#endif
