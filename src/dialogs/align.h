#ifndef SP_QUICK_ALIGN_H
#define SP_QUICK_ALIGN_H

#include "../sp-item.h"

void sp_quick_align_dialog (void);
void sp_quick_align_dialog_close (void);
void sp_move_item_rel (SPItem * item, double dx, double dy);
GSList * sp_get_selectionlist (void);
void sp_quick_align_top_in (void);
void sp_quick_align_top_out (void);
void sp_quick_align_bottom_in (void);
void sp_quick_align_bottom_out (void);
void sp_quick_align_left_in (void);
void sp_quick_align_left_out (void);
void sp_quick_align_right_in (void);
void sp_quick_align_right_out (void);
void sp_quick_align_center_hor (void);
void sp_quick_align_center_ver (void);

#endif
