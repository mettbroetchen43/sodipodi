#define SP_NODE_CONTEXT_C

#include "xml/repr.h"
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "sp-path.h"
#include "rubberband.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "nodepath.h"
#include "sp-node-context.h"

SPNodePath * nodepath = NULL;

gint
sp_node_handler (SPDesktop * desktop, SPItem * item, GdkEvent * event)
{
	gint handled;

	handled = FALSE;

	if (strcmp (sp_repr_name (item->repr), "path") != 0)
		return FALSE;

	switch (event->type) {
		case GDK_BUTTON_PRESS:
			break;
		case GDK_BUTTON_RELEASE:
			sp_selection_set_item (SP_DT_SELECTION (desktop), item);
			handled = TRUE;
		default:
			break;
	}

	return handled;
}

gint
sp_node_root_handler (SPDesktop * desktop, GdkEvent * event)
{
	ArtPoint p;
	ArtDRect b;
	gint ret;

	ret = FALSE;

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

	return ret;
}

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

void
sp_node_context_selection_changed (SPDesktop * desktop)
{
	SPItem * item;
	if (nodepath != NULL) {
		sp_node_path_finish (nodepath);
		nodepath = NULL;
	}
	item = sp_selection_item (SP_DT_SELECTION (desktop));
	if (item != NULL) {
		nodepath = sp_nodepath_new (desktop, item);
	}
}

void sp_node_context_set (SPDesktop * desktop)
{
	SPItem * item;
	item = sp_selection_item (SP_DT_SELECTION (desktop));
	if (item != NULL)
		nodepath = sp_nodepath_new (desktop, item);
}

void sp_node_context_release (SPDesktop * desktop)
{
	if (nodepath != NULL) {
		sp_node_path_finish (nodepath);
		nodepath = NULL;
	}
}

