#define SP_USE_C

#include <string.h>
#include "display/nr-arena-group.h"
#include "document.h"
#include "sp-object-repr.h"
#include "sp-use.h"

/* fixme: */
#include "desktop-events.h"

enum {ARG_0, ARG_X, ARG_Y, ARG_WIDTH, ARG_HEIGHT, ARG_HREF};

static void sp_use_class_init (SPUseClass *class);
static void sp_use_init (SPUse *use);
static void sp_use_destroy (GtkObject *object);
static void sp_use_set_arg (GtkObject * object, GtkArg * arg, guint arg_id);

static void sp_use_build (SPObject * object, SPDocument * document, SPRepr * repr);
static void sp_use_read_attr (SPObject * object, const gchar * attr);
static SPRepr *sp_use_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_use_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform);
static void sp_use_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_use_description (SPItem * item);
static NRArenaItem *sp_use_show (SPItem *item, NRArena *arena);
static void sp_use_hide (SPItem *item, NRArena *arena);

static void sp_use_changed (SPUse * use);
static void sp_use_href_changed (SPUse * use);

static SPItemClass * parent_class;

GtkType
sp_use_get_type (void)
{
	static GtkType use_type = 0;
	if (!use_type) {
		GtkTypeInfo use_info = {
			"SPUse",
			sizeof (SPUse),
			sizeof (SPUseClass),
			(GtkClassInitFunc) sp_use_class_init,
			(GtkObjectInitFunc) sp_use_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		use_type = gtk_type_unique (sp_item_get_type (), &use_info);
	}
	return use_type;
}

static void
sp_use_class_init (SPUseClass *class)
{
	GtkObjectClass * gtk_object_class;
	SPObjectClass * sp_object_class;
	SPItemClass * item_class;

	gtk_object_class = (GtkObjectClass *) class;
	sp_object_class = (SPObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_item_get_type ());

	gtk_object_add_arg_type ("SPUse::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X);
	gtk_object_add_arg_type ("SPUse::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y);
	gtk_object_add_arg_type ("SPUse::width", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH);
	gtk_object_add_arg_type ("SPUse::height", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_HEIGHT);
	gtk_object_add_arg_type ("SPUse::href", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_HREF);

	gtk_object_class->destroy = sp_use_destroy;
	gtk_object_class->set_arg = sp_use_set_arg;

	sp_object_class->build = sp_use_build;
	sp_object_class->read_attr = sp_use_read_attr;
	sp_object_class->write = sp_use_write;

	item_class->bbox = sp_use_bbox;
	item_class->description = sp_use_description;
	item_class->print = sp_use_print;
	item_class->show = sp_use_show;
	item_class->hide = sp_use_hide;
}

static void
sp_use_init (SPUse * use)
{
	use->x = use->y = 0.0;
	use->width = use->height = 1.0;
	use->href = NULL;
}

static void
sp_use_destroy (GtkObject *object)
{
	SPUse *use;

	use = SP_USE (object);

	if (use->child) {
		SP_OBJECT (use->child)->parent = NULL;
		gtk_object_destroy (GTK_OBJECT (use->child));
	}

	if (use->href) g_free (use->href);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_use_set_arg (GtkObject * object, GtkArg * arg, guint arg_id)
{
	SPUse * use;
	gchar * newref;

	use = SP_USE (object);

	switch (arg_id) {
	case ARG_X:
		use->x = GTK_VALUE_DOUBLE (*arg);
		sp_use_changed (use);
		break;
	case ARG_Y:
		use->y = GTK_VALUE_DOUBLE (*arg);
		sp_use_changed (use);
		break;
	case ARG_WIDTH:
		use->width = GTK_VALUE_DOUBLE (*arg);
		sp_use_changed (use);
		break;
	case ARG_HEIGHT:
		use->height = GTK_VALUE_DOUBLE (*arg);
		sp_use_changed (use);
		break;
	case ARG_HREF:
		newref = GTK_VALUE_STRING (*arg);
		if (newref) {
			if (use->href) {
				if (strcmp (newref, use->href) == 0) return;
				g_free (use->href);
				use->href = g_strdup (newref + 1);
			} else {
				use->href = g_strdup (newref + 1);
			}
		} else {
			if (use->href) {
				g_free (use->href);
				use->href = NULL;
			}
		}
		sp_use_href_changed (use);
		break;
	}
}

static void
sp_use_build (SPObject * object, SPDocument * document, SPRepr * repr)
{
	SPUse * use;

	use = SP_USE (object);

	if (SP_OBJECT_CLASS(parent_class)->build)
		(* SP_OBJECT_CLASS(parent_class)->build) (object, document, repr);

	sp_use_read_attr (object, "x");
	sp_use_read_attr (object, "y");
	sp_use_read_attr (object, "width");
	sp_use_read_attr (object, "height");
	sp_use_read_attr (object, "xlink:href");

	if (use->href) {
		SPObject *refobj;
		refobj = sp_document_lookup_id (document, use->href);
		if (refobj) {
			SPRepr *childrepr;
			GtkType type;
			childrepr = SP_OBJECT_REPR (refobj);
			type = sp_repr_type_lookup (childrepr);
			g_return_if_fail (type > GTK_TYPE_NONE);
			if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
				SPObject *childobj;
				childobj = gtk_type_new (type);
				use->child = (SPItem *) sp_object_attach_reref (object, childobj, NULL);
				sp_object_invoke_build (childobj, document, childrepr, TRUE);
			}
		}
	}
}

static void
sp_use_read_attr (SPObject * object, const gchar * attr)
{
	SPUse * use;
	double n;

	use = SP_USE (object);

	/* fixme: we should really collect updates */

	if (strcmp (attr, "x") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, use->x);
		use->x = n;
		sp_use_changed (use);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, use->y);
		use->y = n;
		sp_use_changed (use);
		return;
	}
	if (strcmp (attr, "width") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, use->width);
		use->width = n;
		sp_use_changed (use);
		return;
	}
	if (strcmp (attr, "height") == 0) {
		n = sp_repr_get_double_attribute (object->repr, attr, use->height);
		use->height = n;
		sp_use_changed (use);
		return;
	}
	if (strcmp (attr, "xlink:href") == 0) {
		const gchar * newref;
		newref = sp_repr_attr (object->repr, attr);
		if (newref) {
			if (use->href) {
				if (strcmp (newref, use->href) == 0) return;
				g_free (use->href);
				use->href = g_strdup (newref + 1);
			} else {
				use->href = g_strdup (newref + 1);
			}
		} else {
			if (use->href) {
				g_free (use->href);
				use->href = NULL;
			}
		}
		sp_use_href_changed (use);
		return;
	}

	if (SP_OBJECT_CLASS (parent_class)->read_attr)
		SP_OBJECT_CLASS (parent_class)->read_attr (object, attr);

}

static SPRepr *
sp_use_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPUse *use;

	use = SP_USE (object);

	if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
		repr = sp_repr_new ("use");
	}

	if (repr != SP_OBJECT_REPR (object)) {
		sp_repr_merge (repr, SP_OBJECT_REPR (object), "id");
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}

static void
sp_use_bbox (SPItem *item, ArtDRect *bbox, const gdouble *transform)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) {
		sp_item_invoke_bbox (use->child, bbox, transform);
	} else {
		bbox->x0 = bbox->y0 = 0.0;
		bbox->x1 = bbox->y1 = 0.0;
	}
}

static void
sp_use_print (SPItem * item, GnomePrintContext * gpc)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) sp_item_print (use->child, gpc);
}

static gchar *
sp_use_description (SPItem * item)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) return sp_item_description (use->child);

	return g_strdup ("Empty reference [SHOULDN'T HAPPEN]");
}

static NRArenaItem *
sp_use_show (SPItem *item, NRArena *arena)
{
	SPUse *use;

	use = SP_USE (item);

	if (use->child) {
		NRArenaItem *ai, *ac;
		ai = nr_arena_item_new (arena, NR_TYPE_ARENA_GROUP);
		nr_arena_group_set_transparent (NR_ARENA_GROUP (ai), FALSE);
		ac = sp_item_show (use->child, arena);
		if (ac) {
			nr_arena_item_add_child (ai, ac, NULL);
			gtk_object_unref (GTK_OBJECT(ac));
		}
		return ai;
	}
		
	return NULL;
}

static void
sp_use_hide (SPItem * item, NRArena *arena)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) sp_item_hide (use->child, arena);

	if (SP_ITEM_CLASS (parent_class)->hide)
		(* SP_ITEM_CLASS (parent_class)->hide) (item, arena);
}

static void
sp_use_changed (SPUse * use)
{
}

static void
sp_use_href_changed (SPUse * use)
{
	SPItem * item;

	item = SP_ITEM (use);

	if (use->child) {
		SP_OBJECT (use->child)->parent = NULL;
		gtk_object_destroy (GTK_OBJECT (use->child));
	}

	use->child = NULL;

	if (use->href) {
		SPObject * refobj;
		refobj = sp_document_lookup_id (SP_OBJECT (use)->document, use->href);
		if (refobj) {
			SPRepr * repr;
			GtkType type;
			repr = refobj->repr;
			type = sp_repr_type_lookup (repr);
			g_return_if_fail (type > GTK_TYPE_NONE);
			if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
				SPObject * childobj;
				SPItemView * v;
				childobj = gtk_type_new (type);
				childobj->parent = SP_OBJECT (use);
				use->child = SP_ITEM (childobj);
				sp_object_invoke_build (childobj, SP_OBJECT (use)->document, repr, TRUE);
				for (v = item->display; v != NULL; v = v->next) {
					NRArenaItem *ai;
					ai = sp_item_show (SP_ITEM (childobj), v->arena);
					if (ai) {
						nr_arena_item_add_child (v->arenaitem, ai, NULL);
						gtk_object_unref (GTK_OBJECT(ai));
					}
				}
			}
		}
	}
}
