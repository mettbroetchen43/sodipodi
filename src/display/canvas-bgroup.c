#define SP_CANVAS_BGROUP_C

#include "canvas-bgroup.h"

enum {ARG_0, ARG_OPACITY};

/* fixme: This should go to common header */
#define SP_CANVAS_STICKY_FLAG (1 << 16)

static void sp_canvas_bgroup_class_init (SPCanvasBgroupClass * klass);
static void sp_canvas_bgroup_init (SPCanvasBgroup * group);
static void sp_canvas_bgroup_destroy (GtkObject * object);
static void sp_canvas_bgroup_set_arg (GtkObject *object, GtkArg *arg, guint arg_id);

static void sp_canvas_bgroup_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static double sp_canvas_bgroup_point (GnomeCanvasItem * item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);

static GnomeCanvasGroupClass *parent_class;

GtkType
sp_canvas_bgroup_get_type (void)
{
	static GtkType group_type = 0;
	if (!group_type) {
		GtkTypeInfo group_info = {
			"SPCanvasBgroup",
			sizeof (SPCanvasBgroup),
			sizeof (SPCanvasBgroupClass),
			(GtkClassInitFunc) sp_canvas_bgroup_class_init,
			(GtkObjectInitFunc) sp_canvas_bgroup_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		group_type = gtk_type_unique (gnome_canvas_group_get_type (), &group_info);
	}
	return group_type;
}

static void
sp_canvas_bgroup_class_init (SPCanvasBgroupClass *klass)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) klass;
	item_class = (GnomeCanvasItemClass *) klass;

	gtk_object_add_arg_type ("SPCanvasBgroup::opacity", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_OPACITY);

	parent_class = gtk_type_class (gnome_canvas_group_get_type ());

	object_class->destroy = sp_canvas_bgroup_destroy;
	object_class->set_arg = sp_canvas_bgroup_set_arg;

	item_class->update = sp_canvas_bgroup_update;
	item_class->point = sp_canvas_bgroup_point;
}

static void
sp_canvas_bgroup_init (SPCanvasBgroup * group)
{
	group->opacity = 1.0;
	group->realopacity = 1.0;
	group->transparent = FALSE;
	group->sensitive = TRUE;
}

static void
sp_canvas_bgroup_destroy (GtkObject *object)
{

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_canvas_bgroup_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	SPCanvasBgroup *bgroup;

	bgroup = SP_CANVAS_BGROUP (object);

	switch (arg_id) {
	case ARG_OPACITY:
		bgroup->opacity = GTK_VALUE_DOUBLE (*arg);
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (object));
		break;
	}
}

static void
sp_canvas_bgroup_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	SPCanvasBgroup *bgroup;

	bgroup = SP_CANVAS_BGROUP (item);

	if (SP_IS_CANVAS_BGROUP (item->parent)) {
		bgroup->realopacity = bgroup->opacity * SP_CANVAS_BGROUP (item->parent)->realopacity;
	} else {
		bgroup->realopacity = bgroup->opacity;
	}

	if (((GnomeCanvasItemClass *) parent_class)->update)
		(* ((GnomeCanvasItemClass *) parent_class)->update) (item, affine, clip_path, flags);
}

static double
sp_canvas_bgroup_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	SPCanvasBgroup * group;
	double dist = 1e24;

	group = (SPCanvasBgroup *) item;

	if (!group->sensitive && !(GTK_OBJECT_FLAGS (item->canvas) & SP_CANVAS_STICKY_FLAG)) return 1e18;

	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->point)
		dist = (* GNOME_CANVAS_ITEM_CLASS (parent_class)->point) (item, x, y, cx, cy, actual_item);

	if ((!group->transparent) && (* actual_item != NULL))
		* actual_item = item;

	return dist;
}

void
sp_canvas_bgroup_set_sensitive (SPCanvasBgroup * group, gboolean sensitive)
{
	g_assert (SP_IS_CANVAS_BGROUP (group));

	group->sensitive = sensitive;
}
