
/* Generated data (by glib-mkenums) */

#include <glib-object.h>
#include "sp-dock-typebuiltins.h"



/* enumerations from "sp-dock-object.h" */
static const GFlagsValue _sp_dock_param_flags_values[] = {
  { SP_DOCK_PARAM_EXPORT, "SP_DOCK_PARAM_EXPORT", "export" },
  { SP_DOCK_PARAM_AFTER, "SP_DOCK_PARAM_AFTER", "after" },
  { 0, NULL, NULL }
};

GType
sp_dock_param_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("SPDockParamFlags", _sp_dock_param_flags_values);

  return type;
}


static const GFlagsValue _sp_dock_object_flags_values[] = {
  { SP_DOCK_AUTOMATIC, "SP_DOCK_AUTOMATIC", "dock-automatic" },
  { SP_DOCK_ATTACHED, "SP_DOCK_ATTACHED", "dock-attached" },
  { SP_DOCK_IN_REFLOW, "SP_DOCK_IN_REFLOW", "dock-in-reflow" },
  { SP_DOCK_IN_DETACH, "SP_DOCK_IN_DETACH", "dock-in-detach" },
  { 0, NULL, NULL }
};

GType
sp_dock_object_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("SPDockObjectFlags", _sp_dock_object_flags_values);

  return type;
}


static const GEnumValue _sp_dock_placement_values[] = {
  { SP_DOCK_NONE, "SP_DOCK_NONE", "dock-none" },
  { SP_DOCK_TOP, "SP_DOCK_TOP", "dock-top" },
  { SP_DOCK_BOTTOM, "SP_DOCK_BOTTOM", "dock-bottom" },
  { SP_DOCK_RIGHT, "SP_DOCK_RIGHT", "dock-right" },
  { SP_DOCK_LEFT, "SP_DOCK_LEFT", "dock-left" },
  { SP_DOCK_CENTER, "SP_DOCK_CENTER", "dock-center" },
  { SP_DOCK_FLOATING, "SP_DOCK_FLOATING", "dock-floating" },
  { 0, NULL, NULL }
};

GType
sp_dock_placement_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("SPDockPlacement", _sp_dock_placement_values);

  return type;
}



/* enumerations from "sp-dock-item.h" */
static const GFlagsValue _sp_dock_item_behavior_values[] = {
  { SP_DOCK_ITEM_BEH_NORMAL, "SP_DOCK_ITEM_BEH_NORMAL", "dock-item-beh-normal" },
  { SP_DOCK_ITEM_BEH_NEVER_FLOATING, "SP_DOCK_ITEM_BEH_NEVER_FLOATING", "dock-item-beh-never-floating" },
  { SP_DOCK_ITEM_BEH_NEVER_VERTICAL, "SP_DOCK_ITEM_BEH_NEVER_VERTICAL", "dock-item-beh-never-vertical" },
  { SP_DOCK_ITEM_BEH_NEVER_HORIZONTAL, "SP_DOCK_ITEM_BEH_NEVER_HORIZONTAL", "dock-item-beh-never-horizontal" },
  { SP_DOCK_ITEM_BEH_LOCKED, "SP_DOCK_ITEM_BEH_LOCKED", "dock-item-beh-locked" },
  { SP_DOCK_ITEM_BEH_CANT_DOCK_TOP, "SP_DOCK_ITEM_BEH_CANT_DOCK_TOP", "dock-item-beh-cant-dock-top" },
  { SP_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM, "SP_DOCK_ITEM_BEH_CANT_DOCK_BOTTOM", "dock-item-beh-cant-dock-bottom" },
  { SP_DOCK_ITEM_BEH_CANT_DOCK_LEFT, "SP_DOCK_ITEM_BEH_CANT_DOCK_LEFT", "dock-item-beh-cant-dock-left" },
  { SP_DOCK_ITEM_BEH_CANT_DOCK_RIGHT, "SP_DOCK_ITEM_BEH_CANT_DOCK_RIGHT", "dock-item-beh-cant-dock-right" },
  { SP_DOCK_ITEM_BEH_CANT_DOCK_CENTER, "SP_DOCK_ITEM_BEH_CANT_DOCK_CENTER", "dock-item-beh-cant-dock-center" },
  { 0, NULL, NULL }
};

GType
sp_dock_item_behavior_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("SPDockItemBehavior", _sp_dock_item_behavior_values);

  return type;
}


static const GFlagsValue _sp_dock_item_flags_values[] = {
  { SP_DOCK_IN_DRAG, "SP_DOCK_IN_DRAG", "dock-in-drag" },
  { SP_DOCK_IN_PREDRAG, "SP_DOCK_IN_PREDRAG", "dock-in-predrag" },
  { SP_DOCK_USER_ACTION, "SP_DOCK_USER_ACTION", "dock-user-action" },
  { 0, NULL, NULL }
};

GType
sp_dock_item_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("SPDockItemFlags", _sp_dock_item_flags_values);

  return type;
}



/* Generated data ends here */

