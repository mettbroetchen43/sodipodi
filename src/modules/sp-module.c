#include "sp-module-exec.h"

SPModule *
sp_module_new (unsigned int type, SPRepr *repr)
{
	SPModule *module;

	module = g_object_new (type, NULL);

	if (module) {
		if (repr) sp_repr_ref (repr);
		module->repr = repr;
		if (((SPModuleClass *) G_OBJECT_GET_CLASS (module))->build)
			((SPModuleClass *) G_OBJECT_GET_CLASS (module))->build (module, repr);
	}

	return module;
}

SPModuleHandler *
sp_module_set_exec (SPModule * object, 
		                           SPModuleHandler * exec) {
	g_return_val_if_fail(SP_IS_MODULE(object), NULL);
	g_return_val_if_fail(SP_IS_MODULE_EXEC(exec), NULL);

	if (object->exec != NULL) {
		/* TODO: Destroy here */

	}
	object->exec = exec;

	return exec;
}

/* Module Input */

/* Module Output */

/* Module Filter */

