#ifndef __NR_OBJECT_H__
#define __NR_OBJECT_H__

/*
 * RGBA display list system for sodipodi
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define nr_return_if_fail(expr) if (!(expr) && nr_emit_fail_warning (__FILE__, __LINE__, "?", #expr)) return
#define nr_return_val_if_fail(expr,val) if (!(expr) && nr_emit_fail_warning (__FILE__, __LINE__, "?", #expr)) return (val)

unsigned int nr_emit_fail_warning (const unsigned char *file, unsigned int line, const unsigned char *method, const unsigned char *expr);

#ifndef NR_DISABLE_CAST_CHECKS
#define NR_CHECK_INSTANCE_CAST(ip, tc, ct) ((ct *) nr_object_check_instance_cast (ip, tc))
#else
#define NR_CHECK_INSTANCE_CAST(ip, tc, ct) ((ct *) ip)
#endif

#define NR_CHECK_INSTANCE_TYPE(ip, tc) nr_object_check_instance_type (ip, tc)
#define NR_OBJECT_GET_CLASS(ip) ((NRObjectClass *) (((NRObject *) ip)->klass))

#define NR_TYPE_OBJECT (nr_object_get_type ())

typedef struct _NRObject NRObject;
typedef struct _NRObjectClass NRObjectClass;

NRObject *nr_object_ref (NRObject *object);
NRObject *nr_object_unref (NRObject *object);

NRObject *nr_object_new (unsigned int type);

unsigned int nr_type_is_a (unsigned int type, unsigned int test);

void *nr_object_check_instance_cast (void *ip, unsigned int tc);
unsigned int nr_object_check_instance_type (void *ip, unsigned int tc);

unsigned int nr_object_register_type (unsigned int parent,
				      unsigned char *name,
				      unsigned int csize,
				      unsigned int isize,
				      void (* cinit) (NRObjectClass *),
				      void (* iinit) (NRObject *));

unsigned int nr_object_get_type (void);

struct _NRObject {
	NRObjectClass *klass;
	unsigned int refcount;
};

struct _NRObjectClass {
	unsigned int type;
	NRObjectClass *parent;

	unsigned char *name;
	unsigned int csize;
	unsigned int isize;
	void (* cinit) (NRObjectClass *);
	void (* iinit) (NRObject *);

	void (* finalize) (NRObject *object);
};

#endif


