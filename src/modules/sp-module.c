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

static void sp_module_input_class_init  (SPModuleInputClass  * klass);
static void sp_module_input_init        (SPModuleInput       * object);
static void sp_module_input_finalize     (GObject           * object);

static void sp_module_input_build (SPModule *module, SPRepr *repr);

static SPModuleClass * input_parent_class;

unsigned int
sp_module_input_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleInputClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_input_class_init,
			NULL, NULL,
			sizeof (SPModuleInput),
			16,
			(GInstanceInitFunc) sp_module_input_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleInput", &info, 0);
	}
	return type;
}

static void
sp_module_input_class_init (SPModuleInputClass * klass)
{
	GObjectClass *g_object_class;
	SPModuleClass *module_class;

	g_object_class = (GObjectClass *) klass;
	module_class = (SPModuleClass *) klass;

	input_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_input_finalize;

	module_class->build = sp_module_input_build;
}

static void sp_module_input_init (SPModuleInput * object) {
	g_return_if_fail(SP_IS_MODULE_INPUT(object));

	object->mimetype = NULL;
	object->extention = NULL;

	return;
}

static void sp_module_input_finalize (GObject * object) {
	SPModuleInput * module;

	module = SP_MODULE_INPUT(object);
	
	if (module->mimetype) {
		g_free(module->mimetype);
	}

	if (module->extention) {
		g_free(module->extention);
	}

	G_OBJECT_CLASS (input_parent_class)->finalize (object);
}

static void
sp_module_input_build (SPModule *module, SPRepr *repr)
{
	SPModuleInput *mi;

	mi = (SPModuleInput *) module;

	if (((SPModuleClass *) input_parent_class)->build)
		((SPModuleClass *) input_parent_class)->build (module, repr);

	if (repr) {
		const unsigned char *val;
		val = sp_repr_attr (repr, "mimetype");
		if (val) {
			mi->mimetype = g_strdup (val);
		}
		val = sp_repr_attr (repr, "extension");
		if (val) {
			mi->extention = g_strdup (val);
		}
	}
}

/* Module Output */

static void sp_module_output_class_init (SPModuleOutputClass * klass);
static void sp_module_output_init       (SPModuleOutput      * object);
static void sp_module_output_finalize    (GObject           * object);

static void sp_module_output_build (SPModule *module, SPRepr *repr);

static SPModuleClass * output_parent_class;

unsigned int sp_module_output_get_type (void) {
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleOutputClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_output_class_init,
			NULL, NULL,
			sizeof (SPModuleOutput),
			16,
			(GInstanceInitFunc) sp_module_output_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleOutput", &info, 0);
	}
	return type;
}

static void sp_module_output_class_init (SPModuleOutputClass * klass) {
	GObjectClass *g_object_class;
	SPModuleClass *module_class;

	g_object_class = (GObjectClass *)klass;
	module_class = (SPModuleClass *) klass;

	output_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_output_finalize;

	module_class->build = sp_module_output_build;
}

static void sp_module_output_init (SPModuleOutput * object) {
	g_return_if_fail(SP_IS_MODULE_OUTPUT(object));
	return;
}

static void sp_module_output_finalize (GObject * object) {
	SPModuleOutput * module;

	module = SP_MODULE_OUTPUT(object);
	
	G_OBJECT_CLASS (output_parent_class)->finalize (object);
}

static void
sp_module_output_build (SPModule *module, SPRepr *repr)
{
	SPModuleOutput *mo;

	mo = (SPModuleOutput *) module;

	if (((SPModuleClass *) output_parent_class)->build)
		((SPModuleClass *) output_parent_class)->build (module, repr);

	if (repr) {
		const unsigned char *val;
		val = sp_repr_attr (repr, "mimetype");
		if (val) {
			mo->mimetype = g_strdup (val);
		}
		val = sp_repr_attr (repr, "extension");
		if (val) {
			mo->extention = g_strdup (val);
		}
	}
}

/* Module Filter */

static void sp_module_filter_class_init (SPModuleFilterClass * klass);
static void sp_module_filter_init       (SPModuleFilter      * object);
static void sp_module_filter_finalize   (GObject           * object);

static SPModuleClass * filter_parent_class;

unsigned int
sp_module_filter_get_type (void)
{
	static GType type = 0;
	if (!type) {
		GTypeInfo info = {
			sizeof (SPModuleFilterClass),
			NULL, NULL,
			(GClassInitFunc) sp_module_filter_class_init,
			NULL, NULL,
			sizeof (SPModuleFilter),
			16,
			(GInstanceInitFunc) sp_module_filter_init,
		};
		type = g_type_register_static (SP_TYPE_MODULE, "SPModuleFilter", &info, 0);
	}
	return type;
}

static void sp_module_filter_class_init (SPModuleFilterClass * klass) {
	GObjectClass *g_object_class;

	g_object_class = (GObjectClass *)klass;

	filter_parent_class = g_type_class_peek_parent (klass);

	g_object_class->finalize = sp_module_filter_finalize;
}

static void sp_module_filter_init (SPModuleFilter * object) {
	return;
}

static void sp_module_filter_finalize (GObject * object) {
	SPModuleOutput * module;

	module = SP_MODULE_OUTPUT(object);
	
	G_OBJECT_CLASS (filter_parent_class)->finalize (object);
}
