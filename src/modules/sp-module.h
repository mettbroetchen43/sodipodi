#ifndef __SODIPODI_MODULE_H__
#define __SODIPODI_MODULE_H__

#include "module.h"

SPModule *sp_module_new (unsigned int type, SPRepr *repr);

SPModuleHandler *   sp_module_set_exec      (SPModule *     object,
		                                  SPModuleHandler * exec);
#define sp_module_input_new(r) sp_module_new (SP_TYPE_MODULE_INPUT, r)

#define sp_module_output_new(r) sp_module_new (SP_TYPE_MODULE_OUTPUT, r)

/* Module Filter */

#define SP_TYPE_MODULE_FILTER (sp_module_filter_get_type())
#define SP_MODULE_FILTER(o)  (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_MODULE_FILTER, SPModuleFilter))
#define SP_IS_MODULE_FILTER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_MODULE_FILTER))

typedef struct _SPModuleFilter      SPModuleFilter;
typedef struct _SPModuleFilterClass SPModuleFilterClass;

struct _SPModuleFilter {
    SPModule object;
};

struct _SPModuleFilterClass {
    SPModuleClass parent_class;
};

unsigned int         sp_module_filter_get_type  (void);

#define sp_module_filter_new(r) sp_module_new (SP_TYPE_MODULE_FILTER, r)

#endif  /* __SODIPODI_MODULE_H__ */
