#define SP_NODE_CONTEXT_C

#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "sp-path.h"
#include "rubberband.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "nodepath.h"
#include "node-context.h"

static void sp_node_context_class_init (SPNodeContextClass * klass);
static void sp_node_context_init (SPNodeContext * node_context);
static void sp_node_context_destroy (GtkObject * object);

static void sp_node_context_setup (SPEventContext * event_context, SPDesktop * desktop);
static gint sp_node_context_root_handler (SPEventContext * event_context, GdkEvent * event);
static gint sp_node_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event);

static SPEventContextClass * parent_class;

GtkType
sp_node_context_get_type (void)
{
	static GtkType node_context_type = 0;

	if (!node_context_type) {

		static const GtkTypeInfo node_context_info = {
			"SPNodeContext",
			sizeof (SPNodeContext),
			sizeof (SPNodeContextClass),
			(GtkClassInitFunc) sp_node_context_class_init,
			(GtkObjectInitFunc) sp_node_context_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		node_context_type = gtk_type_unique (sp_event_context_get_type (), &node_context_info);
	}

	return node_context_type;
}

static void
sp_node_context_class_init (SPNodeContextClass * klass)
{
	GtkObjectClass * object_class;
	SPEventContextClass * event_context_class;

	object_class = (GtkObjectClass *) klass;
	event_context_class = (SPEventContextClass *) klass;

	parent_class = gtk_type_class (sp_event_context_get_type ());

	object_class->destroy = sp_node_context_destroy;

	event_context_class->setup = sp_node_context_setup;
	event_context_class->root_handler = sp_node_context_root_handler;
	event_context_class->item_handler = sp_node_context_item_handler;
}

static void
sp_node_context_init (SPNodeContext * node_context)
{
}

static void
sp_node_context_destroy (GtkObject * object)
{
	SPNodeContext * node_context;

	node_context = SP_NODE_CONTEXT (object);

	sp_nodepath_shutdown (&node_context->nodepath);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_node_context_setup (SPEventContext * event_context, SPDesktop * desktop)
{
	SPNodeContext * node_context;

	node_context = SP_NODE_CONTEXT (event_context);

	if (SP_EVENT_CONTEXT_CLASS (parent_class)->setup)
		SP_EVENT_CONTEXT_CLASS (parent_class)->setup (event_context, desktop);

	sp_nodepath_init (&node_context->nodepath, event_context->desktop);
}

static gint
sp_node_context_item_handler (SPEventContext * event_context, SPItem * item, GdkEvent * event)
{
	SPDesktop * desktop;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;

	switch (event->type) {
		case GDK_BUTTON_RELEASE:
			if (SP_IS_PATH (item)) {
				sp_selection_set_item (SP_DT_SELECTION (desktop), item);
				ret = TRUE;
			}
		default:
			break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->item_handler (event_context, item, event);
	}

	return ret;
}

static gint
sp_node_context_root_handler (SPEventContext * event_context, GdkEvent * event)
{
	SPDesktop * desktop;
	SPNodeContext * node_context;
	SPNodePath * nodepath;
	ArtPoint p;
	ArtDRect b;
	gint ret;

	ret = FALSE;

	desktop = event_context->desktop;
	node_context = SP_NODE_CONTEXT (event_context);
	nodepath = &node_context->nodepath;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (desktop, &p, event->button.x, event->button.y);
			sp_rubberband_start (desktop, p.x, p.y);
#if 0
			gnome_canvas_item_grab (item, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, event->button.time);
#endif
			ret = TRUE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		if (event->motion.state & GDK_BUTTON1_MASK) {
			sp_desktop_w2d_xy_point (desktop, &p, event->motion.x, event->motion.y);
			sp_rubberband_move (p.x, p.y);
			ret = TRUE;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
#if 0
			gnome_canvas_item_ungrab (item, event->button.time);
#endif
			if (sp_rubberband_rect (&b)) {
				sp_rubberband_stop ();
				sp_nodepath_node_select_rect (nodepath, &b, event->button.state & GDK_SHIFT_MASK);
				/* fixme: do something useful */
				ret = TRUE;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!ret) {
		if (SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler)
			ret = SP_EVENT_CONTEXT_CLASS (parent_class)->root_handler (event_context, event);
	}

	return ret;
}

#if 0
static void
sp_node_path_finish (SPNodePath * nodepath)
{
	SPRepr * repr;
	ArtBpath * bpath;
	gchar * pathstr, * typestr;

	g_assert (nodepath != NULL);
	g_assert (nodepath->bpath != NULL);

	repr = nodepath->repr;
	bpath = nodepath->bpath;
	pathstr = sp_svg_write_path (nodepath->bpath);
	g_assert (pathstr != NULL);
	typestr = nodepath->typestr;
	g_assert (typestr != NULL);

	sp_nodepath_destroy (nodepath, FALSE, FALSE);

	sp_repr_set_attr (repr, "d", pathstr);
	sp_repr_set_attr (repr, "SODIPODI-PATH-NODE-TYPES", typestr);
	g_free (pathstr);

#if 1
	g_free (typestr);
#endif
#if 0
	art_free (bpath);
#endif
}
#endif
