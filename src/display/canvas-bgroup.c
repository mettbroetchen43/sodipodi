#define SP_CANVAS_BGROUP_C

#include "canvas-bgroup.h"

/* fixme: This should go to common header */
#define SP_CANVAS_STICKY_FLAG (1 << 16)

static void sp_canvas_bgroup_class_init (SPCanvasBgroupClass * klass);
static void sp_canvas_bgroup_init (SPCanvasBgroup * group);
static void sp_canvas_bgroup_destroy (GtkObject * object);

static double sp_canvas_bgroup_point (GnomeCanvasItem * item, double x, double y,
			      int cx, int cy, GnomeCanvasItem **actual_item);

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

	parent_class = gtk_type_class (gnome_canvas_group_get_type ());

	object_class->destroy = sp_canvas_bgroup_destroy;

	item_class->point = sp_canvas_bgroup_point;
}

static void
sp_canvas_bgroup_init (SPCanvasBgroup * group)
{
	group->transparent = FALSE;
	group->sensitive = TRUE;
}

static void
sp_canvas_bgroup_destroy (GtkObject *object)
{

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static double sp_canvas_bgroup_point (GnomeCanvasItem *item, double x, double y,
			      int cx, int cy, GnomeCanvasItem **actual_item)
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
