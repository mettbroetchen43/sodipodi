#define SP_USE_C

#include <string.h>
#include "display/canvas-bgroup.h"
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

static void sp_use_bbox (SPItem * item, ArtDRect * bbox);
static void sp_use_print (SPItem * item, GnomePrintContext * gpc);
static gchar * sp_use_description (SPItem * item);
static GnomeCanvasItem * sp_use_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group);
static void sp_use_hide (SPItem * item, SPDesktop * desktop);
static gboolean sp_use_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[]);

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

	item_class->bbox = sp_use_bbox;
	item_class->description = sp_use_description;
	item_class->print = sp_use_print;
	item_class->show = sp_use_show;
	item_class->hide = sp_use_hide;
	item_class->paint = sp_use_paint;
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
			const gchar *name;
			GtkType type;
			childrepr = SP_OBJECT_REPR (refobj);
			name = sp_repr_name (childrepr);
			g_assert (name != NULL);
			type = sp_object_type_lookup (name);
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

static void
sp_use_bbox (SPItem * item, ArtDRect * bbox)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) {
		sp_item_bbox (use->child, bbox);
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

static GnomeCanvasItem *
sp_use_show (SPItem * item, SPDesktop * desktop, GnomeCanvasGroup * canvas_group)
{
	SPUse * use;
	GnomeCanvasItem * ci;

	use = SP_USE (item);

	if (use->child) {
		ci = gnome_canvas_item_new (canvas_group, SP_TYPE_CANVAS_BGROUP, NULL);
		SP_CANVAS_BGROUP (ci)->transparent = FALSE;
		sp_item_show (use->child, desktop, GNOME_CANVAS_GROUP (ci));
		return ci;
	}
		
	return NULL;
}

static void
sp_use_hide (SPItem * item, SPDesktop * desktop)
{
	SPUse * use;

	use = SP_USE (item);

	if (use->child) sp_item_hide (use->child, desktop);

	if (SP_ITEM_CLASS (parent_class)->hide)
		(* SP_ITEM_CLASS (parent_class)->hide) (item, desktop);
}

static gboolean
sp_use_paint (SPItem * item, ArtPixBuf * buf, gdouble affine[])
{
	SPUse * use;

	use = SP_USE (item);

	if (SP_ITEM_CLASS (parent_class)->paint)
		(* SP_ITEM_CLASS (parent_class)->paint) (item, buf, affine);

	if (use->child) return sp_item_paint (use->child, buf, affine);

	return FALSE;
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
			const gchar * name;
			GtkType type;
			repr = refobj->repr;
			name = sp_repr_name (repr);
			g_assert (name != NULL);
			type = sp_object_type_lookup (name);
			g_return_if_fail (type > GTK_TYPE_NONE);
			if (gtk_type_is_a (type, SP_TYPE_ITEM)) {
				SPObject * childobj;
				SPItemView * v;
				childobj = gtk_type_new (type);
				childobj->parent = SP_OBJECT (use);
				use->child = SP_ITEM (childobj);
				sp_object_invoke_build (childobj, SP_OBJECT (use)->document, repr, TRUE);
				for (v = item->display; v != NULL; v = v->next) {
					sp_item_show (SP_ITEM (childobj), v->desktop, GNOME_CANVAS_GROUP (v->canvasitem));
				}
			}
		}
	}
}
