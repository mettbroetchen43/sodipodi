#define __NR_OBJECT_C__

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

#include <string.h>
#include <stdio.h>

#include <libnr/nr-macros.h>

#include "nr-object.h"

unsigned int
nr_emit_fail_warning (const unsigned char *file, unsigned int line, const unsigned char *method, const unsigned char *expr)
{
	fprintf (stderr, "File %s line %d (%s): Assertion %s failed\n", file, line, method, expr);
	return 1;
}

/* NRObject */

static NRObjectClass **classes = NULL;
static unsigned int classes_len = 0;
static unsigned int classes_size = 0;

NRObject *
nr_object_ref (NRObject *object)
{
	object->refcount += 1;
	return object;
}

NRObject *
nr_object_unref (NRObject *object)
{
	object->refcount -= 1;
	if (object->refcount < 1) {
		object->klass->finalize (object);
	}
	return NULL;
}

static void
nr_class_tree_object_invoke_init (NRObjectClass *klass, NRObject *object)
{
	if (klass->parent) {
		nr_class_tree_object_invoke_init (klass->parent, object);
	}
	klass->iinit (object);
}

NRObject *
nr_object_new (unsigned int type)
{
	NRObjectClass *klass;
	NRObject *object;

	nr_return_val_if_fail (type < classes_len, NULL);

	klass = classes[type];

	object = malloc (klass->isize);
	memset (object, 0, klass->isize);
	object->klass = klass;
	object->refcount = 1;

	nr_class_tree_object_invoke_init (klass, object);

	return object;
}

unsigned int
nr_type_is_a (unsigned int type, unsigned int test)
{
	NRObjectClass *klass;

	nr_return_val_if_fail (type < classes_len, FALSE);
	nr_return_val_if_fail (test < classes_len, FALSE);

	klass = classes[type];

	while (klass) {
		if (klass->type == test) return TRUE;
		klass = klass->parent;
	}

	return FALSE;
}

void *
nr_object_check_instance_cast (void *ip, unsigned int tc)
{
	nr_return_val_if_fail (ip != NULL, NULL);
	nr_return_val_if_fail (nr_type_is_a (((NRObject *) ip)->klass->type, tc), ip);
	return ip;
}

unsigned int
nr_object_check_instance_type (void *ip, unsigned int tc)
{
	if (ip == NULL) return FALSE;
	return nr_type_is_a (((NRObject *) ip)->klass->type, tc);
}

unsigned int
nr_object_register_type (unsigned int parent,
			 unsigned char *name,
			 unsigned int csize,
			 unsigned int isize,
			 void (* cinit) (NRObjectClass *),
			 void (* iinit) (NRObject *))
{
	unsigned int type;
	NRObjectClass *klass;

	if (classes_len >= classes_size) {
		classes_size += 32;
		classes = nr_renew (classes, NRObjectClass *, classes_size);
		if (classes_len == 0) {
			classes[0] = NULL;
			classes_len = 1;
		}
	}

	type = classes_len;
	classes_len += 1;

	classes[type] = malloc (csize);
	klass = classes[type];
	memset (klass, 0, csize);

	if (classes[parent]) {
		memcpy (klass, classes[parent], classes[parent]->csize);
	}

	klass->type = type;
	klass->parent = classes[parent];
	klass->name = strdup (name);
	klass->csize = csize;
	klass->isize = isize;
	klass->cinit = cinit;
	klass->iinit = iinit;

	klass->cinit (klass);

	return type;
}

static void nr_object_class_init (NRObjectClass *klass);
static void nr_object_init (NRObject *object);
static void nr_object_finalize (NRObject *object);

unsigned int
nr_object_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		type = nr_object_register_type (0,
						"NRObject",
						sizeof (NRObjectClass),
						sizeof (NRObject),
						(void (*) (NRObjectClass *)) nr_object_class_init,
						(void (*) (NRObject *)) nr_object_init);
	}
	return type;
}

static void
nr_object_class_init (NRObjectClass *klass)
{
	klass->finalize = nr_object_finalize;
}

static void nr_object_init (NRObject *object)
{
}

static void nr_object_finalize (NRObject *object)
{
	free (object);
}

