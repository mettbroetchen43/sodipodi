#define NODEPATH_C

#include <math.h>
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "helper/sp-ctrlline.h"
#include "knot.h"
#include "sodipodi.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-snap.h"
#include "node-context.h"
#include "nodepath.h"

/* fixme: Implement these via preferences */

#define NODE_FILL 0xbfbfbf7f
#define NODE_STROKE 0x3f3f3f7f
#define NODE_FILL_HI 0xff7f7f7f
#define NODE_STROKE_HI 0xff3f3fbf
#define NODE_FILL_SEL 0xbfbfbf7f
#define NODE_STROKE_SEL 0x0000ffbf
#define NODE_FILL_SEL_HI 0x7f7fffbf
#define NODE_STROKE_SEL_HI 0x3f3fffff
#define KNOT_FILL 0xbfbfbf7f
#define KNOT_STROKE 0x3f3f3f7f
#define KNOT_FILL_HI 0xff7f7f7f
#define KNOT_STROKE_HI 0xff3f3fbf

GMemChunk * nodechunk = NULL;

/* Creation from object */

static ArtBpath * subpath_from_bpath (SPNodePath * np, ArtBpath * b, const gchar * t);
static gchar * parse_nodetypes (const gchar * types, gint length);

/* Object updating */

static void update_object (SPNodePath * np);
static void update_repr (SPNodePath * np);
static SPCurve * create_curve (SPNodePath * np);
static gchar * create_typestr (SPNodePath * np);

static void sp_node_ensure_ctrls (SPPathNode * node);

void sp_nodepath_node_select (SPPathNode * node, gboolean incremental);
void sp_nodepath_select_rect (SPNodePath * nodepath, ArtDRect * b, gboolean incremental);

static void sp_node_set_selected (SPPathNode * node, gboolean selected);

/* Control knot placement, if node or other knot is moved */

static void sp_node_adjust_knot (SPPathNode * node, gint which_adjust);
static void sp_node_adjust_knots (SPPathNode * node);

/* Knot event handlers */

static void node_clicked (SPKnot * knot, guint state, gpointer data);
static void node_grabbed (SPKnot * knot, guint state, gpointer data);
static void node_ungrabbed (SPKnot * knot, guint state, gpointer data);
static gboolean node_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data);
static void node_ctrl_clicked (SPKnot * knot, guint state, gpointer data);
static void node_ctrl_grabbed (SPKnot * knot, guint state, gpointer data);
static void node_ctrl_ungrabbed (SPKnot * knot, guint state, gpointer data);
static gboolean node_ctrl_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data);
static void node_ctrl_moved (SPKnot * knot, ArtPoint * p, guint state, gpointer data);

/* Constructors and destrouctos */

static SPNodeSubPath * sp_nodepath_subpath_new (SPNodePath * nodepath);
static void sp_nodepath_subpath_destroy (SPNodeSubPath * subpath);
static void sp_nodepath_subpath_close (SPNodeSubPath * sp);
static void sp_nodepath_subpath_open (SPNodeSubPath * sp, SPPathNode * n);
static SPPathNode * sp_nodepath_node_new (SPNodeSubPath * sp, SPPathNode * next, SPPathNodeType type, ArtPathcode code,
					  ArtPoint * ppos, ArtPoint * pos, ArtPoint * npos);
static void sp_nodepath_node_destroy (SPPathNode * node);

/* Helpers */

static SPPathNodeSide * sp_node_get_side (SPPathNode * node, gint which);
static SPPathNodeSide * sp_node_opposite_side (SPPathNode * node, SPPathNodeSide * me);
static ArtPathcode sp_node_path_code_from_side (SPPathNode * node, SPPathNodeSide * me);

/* Creation from object */

SPNodePath *
sp_nodepath_new (SPDesktop * desktop, SPItem * item)
{
	SPNodePath * np;
	SPPath * path;
	SPCurve * curve;
	ArtBpath * bpath, * b;
	const gchar * nodetypes;
	gchar * typestr;
	gint length;

	if (!SP_IS_PATH (item)) return NULL;
	path = SP_PATH (item);
	if (!path->independent) return NULL;
	curve = sp_path_normalized_bpath (path);
	g_return_val_if_fail (curve != NULL, NULL);

	bpath = sp_curve_first_bpath (curve);
	length = curve->end;
	nodetypes = sp_repr_attr (SP_OBJECT (item)->repr, "sodipodi:nodetypes");
	typestr = parse_nodetypes (nodetypes, length);

	np = g_new (SPNodePath, 1);

	np->desktop = desktop;
	np->path = path;
	np->subpaths = NULL;
	np->selected = NULL;
	sp_item_i2d_affine (SP_ITEM (path), np->i2d);
	art_affine_invert (np->d2i, np->i2d);

	/* Now the bitchy part */

	b = bpath;

	while (b->code != ART_END) {
		b = subpath_from_bpath (np, b, typestr + (b - bpath));
	}

	g_free (typestr);
	sp_curve_unref (curve);

	return np;
}

static ArtBpath *
subpath_from_bpath (SPNodePath * np, ArtBpath * b, const gchar * t)
{
	SPNodeSubPath * sp;
	SPPathNode * n;
	ArtPoint ppos, pos, npos;
	gboolean closed;

	g_assert ((b->code == ART_MOVETO) || (b->code == ART_MOVETO_OPEN));

	sp = sp_nodepath_subpath_new (np);
	closed = (b->code == ART_MOVETO);

	pos.x = b->x3;
	pos.y = b->y3;
	art_affine_point (&pos, &pos, np->i2d);
	if ((b + 1)->code == ART_CURVETO) {
		npos.x = (b + 1)->x1;
		npos.y = (b + 1)->y1;
		art_affine_point (&npos, &npos, np->i2d);
	} else {
		npos = pos;
	}
	n = sp_nodepath_node_new (sp, NULL, *t, ART_MOVETO, &pos, &pos, &npos);
	g_assert (sp->first == n);
	g_assert (sp->last == n);

	b++;
	t++;

	while ((b->code == ART_CURVETO) || (b->code == ART_LINETO)) {
		pos.x = b->x3;
		pos.y = b->y3;
		art_affine_point (&pos, &pos, np->i2d);
		if (b->code == ART_CURVETO) {
			ppos.x = b->x2;
			ppos.y = b->y2;
			art_affine_point (&ppos, &ppos, np->i2d);
		} else {
			ppos = pos;
		}
		if ((b + 1)->code == ART_CURVETO) {
			npos.x = (b + 1)->x1;
			npos.y = (b + 1)->y1;
			art_affine_point (&npos, &npos, np->i2d);
		} else {
			npos = pos;
		}
		n = sp_nodepath_node_new (sp, NULL, *t, b->code, &ppos, &pos, &npos);
		b++;
		t++;
	}

	if (closed) sp_nodepath_subpath_close (sp);

	return b;
}

static gchar *
parse_nodetypes (const gchar * types, gint length)
{
	gchar * typestr;
	gint pos, i;

	g_assert (length > 0);

	typestr = g_new (gchar, length + 1);

	pos = 0;

	if (types) {
		for (i = 0; types[i] && (i < length); i++) {
			while ((types[i] > '\0') && (types[i] <= ' ')) i++;
			if (types[i] != '\0') {
				switch (types[i]) {
				case 's':
					typestr[pos++] = SP_PATHNODE_SMOOTH;
					break;
				case 'z':
					typestr[pos++] = SP_PATHNODE_SYMM;
					break;
				case 'c':
				default:
					typestr[pos++] = SP_PATHNODE_CUSP;
					break;
				}
			}
		}
	}

	while (pos < length) typestr[pos++] = SP_PATHNODE_CUSP;

	return typestr;
}

/*
 * Object updating
 */

static void
update_object (SPNodePath * np)
{
	SPCurve * curve;

	g_assert (np);

	curve = create_curve (np);

	sp_path_clear (np->path);
	sp_path_add_bpath (np->path, curve, TRUE, NULL);

	sp_curve_unref (curve);
}

static void
update_repr (SPNodePath * np)
{
	SPCurve * curve;
	gchar * typestr;
	gchar * svgpath;

	g_assert (np);

	curve = create_curve (np);
	typestr = create_typestr (np);

	svgpath = sp_svg_write_path (curve->bpath);

	sp_repr_set_attr (SP_OBJECT (np->path)->repr, "d", svgpath);
	sp_repr_set_attr (SP_OBJECT (np->path)->repr, "sodipodi:nodetypes", typestr);

	sp_document_done (SP_DT_DOCUMENT (np->desktop));

	g_free (svgpath);
	g_free (typestr);
	sp_curve_unref (curve);
}

static SPCurve *
create_curve (SPNodePath * np)
{
	SPCurve * curve;
	GSList * spl;
	ArtPoint p1, p2, p3;

	curve = sp_curve_new ();

	for (spl = np->subpaths; spl != NULL; spl = spl->next) {
		SPNodeSubPath * sp;
		SPPathNode * n;
		sp = (SPNodeSubPath *) spl->data;
		p3.x = sp->first->pos.x;
		p3.y = sp->first->pos.y;
		art_affine_point (&p3, &p3, np->d2i);
		sp_curve_moveto (curve, p3.x, p3.y);
		n = sp->first->n.other;
		while (n) {
			p3.x = n->pos.x;
			p3.y = n->pos.y;
			art_affine_point (&p3, &p3, np->d2i);
			switch (n->code) {
			case ART_LINETO:
				sp_curve_lineto (curve, p3.x ,p3.y);
				break;
			case ART_CURVETO:
				p1.x = n->p.other->n.pos.x;
				p1.y = n->p.other->n.pos.y;
				art_affine_point (&p1, &p1, np->d2i);
				p2.x = n->p.pos.x;
				p2.y = n->p.pos.y;
				art_affine_point (&p2, &p2, np->d2i);
				sp_curve_curveto (curve, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
				break;
			default:
				g_assert_not_reached ();
				break;
			}
			if (n != sp->last) {
				n = n->n.other;
			} else {
				n = NULL;
			}
		}
		if (sp->closed) {
			sp_curve_closepath (curve);
		}
	}

	return curve;
}

static gchar *
create_typestr (SPNodePath * np)
{
	gchar * typestr;
	gint len, pos;
	GSList * spl;

	typestr = g_new (gchar, 32);
	len = 32;
	pos = 0;

	for (spl = np->subpaths; spl != NULL; spl = spl->next) {
		SPNodeSubPath * sp;
		SPPathNode * n;
		sp = (SPNodeSubPath *) spl->data;

		if (pos >= len) {
			typestr = g_renew (gchar, typestr, len + 32);
			len += 32;
		}

		typestr[pos++] = 'c';

		n = sp->first->n.other;
		while (n) {
			gchar code;

			switch (n->type) {
			case SP_PATHNODE_CUSP:
				code = 'c';
				break;
			case SP_PATHNODE_SMOOTH:
				code = 's';
				break;
			case SP_PATHNODE_SYMM:
				code = 'z';
				break;
			default:
				g_assert_not_reached ();
				code = '\0';
				break;
			}

			if (pos >= len) {
				typestr = g_renew (gchar, typestr, len + 32);
				len += 32;
			}

			typestr[pos++] = code;

			if (n != sp->last) {
				n = n->n.other;
			} else {
				n = NULL;
			}
		}
	}

	if (pos >= len) {
		typestr = g_renew (gchar, typestr, len + 1);
		len += 1;
	}

	typestr[pos++] = '\0';

	return typestr;
}

void
sp_nodepath_destroy (SPNodePath * np)
{
	g_assert (np);

	while (np->subpaths) {
		sp_nodepath_subpath_destroy ((SPNodeSubPath *) np->subpaths->data);
	}

	g_assert (!np->selected);

	g_free (np);
}

static SPNodePath *
sp_nodepath_current (void)
{
	SPEventContext * event_context;

	if (!SP_ACTIVE_DESKTOP) return NULL;

	event_context = (SP_ACTIVE_DESKTOP)->event_context;

	if (!SP_IS_NODE_CONTEXT (event_context)) return NULL;

	return SP_NODE_CONTEXT (event_context)->nodepath;
}

/*
 * Fills node and control positions for three nodes, splitting line
 * marked by end at distance t
 */

static void
sp_nodepath_line_midpoint (SPPathNode * new, SPPathNode * end, gdouble t)
{
	SPPathNode * start;
	gdouble s;
	gdouble f000, f001, f011, f111, f00t, f0tt, fttt, f11t, f1tt, f01t;

	g_assert (new != NULL);
	g_assert (end != NULL);

	g_assert (end->p.other == new);
	start = new->p.other;
	g_assert (start);

	if (end->code == ART_LINETO) {
		new->type = SP_PATHNODE_CUSP;
		new->code = ART_LINETO;
		new->pos.x = (t * start->pos.x + (1 - t) * end->pos.x);
		new->pos.y = (t * start->pos.y + (1 - t) * end->pos.y);
	} else {
		new->type = SP_PATHNODE_SMOOTH;
		new->code = ART_CURVETO;
		s = 1 - t;
		f000 = start->pos.x;
		f001 = start->n.pos.x;
		f011 = end->p.pos.x;
		f111 = end->pos.x;
		f00t = s * f000 + t * f001;
		f01t = s * f001 + t * f011;
		f11t = s * f011 + t * f111;
		f0tt = s * f00t + t * f01t;
		f1tt = s * f01t + t * f11t;
		fttt = s * f0tt + t * f1tt;
		start->n.pos.x = f00t;
		new->p.pos.x = f0tt;
		new->pos.x = fttt;
		new->n.pos.x = f1tt;
		end->p.pos.x = f11t;
		f000 = start->pos.y;
		f001 = start->n.pos.y;
		f011 = end->p.pos.y;
		f111 = end->pos.y;
		f00t = s * f000 + t * f001;
		f01t = s * f001 + t * f011;
		f11t = s * f011 + t * f111;
		f0tt = s * f00t + t * f01t;
		f1tt = s * f01t + t * f11t;
		fttt = s * f0tt + t * f1tt;
		start->n.pos.y = f00t;
		new->p.pos.y = f0tt;
		new->pos.y = fttt;
		new->n.pos.y = f1tt;
		end->p.pos.y = f11t;
	}
}

static SPPathNode *
sp_nodepath_line_add_node (SPPathNode * end, gdouble t)
{
	SPNodePath * np;
	SPNodeSubPath * sp;
	SPPathNode * start;
	SPPathNode * newnode;

	g_assert (end);
	g_assert (end->subpath);
	g_assert (g_slist_find (end->subpath->nodes, end));

	sp = end->subpath;
	np = sp->nodepath;

	start = end->p.other;

	g_assert (start->n.other == end);

	newnode = sp_nodepath_node_new (sp, end, SP_PATHNODE_SMOOTH, end->code, &start->pos, &start->pos, &start->n.pos);
	sp_nodepath_line_midpoint (newnode, end, t);

	sp_node_ensure_ctrls (start);
	sp_node_ensure_ctrls (newnode);
	sp_node_ensure_ctrls (end);

	return newnode;
}

static SPPathNode *
sp_nodepath_node_break (SPPathNode * node)
{
	SPNodePath * np;
	SPNodeSubPath * sp;

	g_assert (node);
	g_assert (node->subpath);
	g_assert (g_slist_find (node->subpath->nodes, node));

	sp = node->subpath;
	np = sp->nodepath;

	if (sp->closed) {
		sp_nodepath_subpath_open (sp, node);

		return sp->first;
	} else {
		SPNodeSubPath * newsubpath;
		SPPathNode * newnode;
		SPPathNode * n;

		if (node == sp->first) return NULL;
		if (node == sp->last) return NULL;

		newsubpath = sp_nodepath_subpath_new (np);

		newnode = sp_nodepath_node_new (newsubpath, NULL, node->type, ART_MOVETO, &node->pos, &node->pos, &node->n.pos);

		while (node->n.other) {
			n = node->n.other;
			sp_nodepath_node_new (newsubpath, NULL, n->type, n->code, &n->p.pos, &n->pos, &n->n.pos);
			sp_nodepath_node_destroy (n);
		}

		return newnode;
	}
}

static void
sp_nodepath_set_line_type (SPPathNode * end, ArtPathcode code)
{
	SPPathNode * start;
	double dx, dy;

	g_assert (end);
	g_assert (end->subpath);
	g_assert (end->p.other);

	if (end->code == code) return;

	start = end->p.other;

	end->code = code;

	if (code == ART_LINETO) {
		if (start->code == ART_LINETO) start->type = SP_PATHNODE_CUSP;
		if (end->n.other) {
			if (end->n.other->code == ART_LINETO) end->type = SP_PATHNODE_CUSP;
		}
		sp_node_adjust_knot (start, -1);
		sp_node_adjust_knot (end, 1);
	} else {
		dx = end->pos.x - start->pos.x;
		dy = end->pos.y - start->pos.y;
		start->n.pos.x = start->pos.x + dx / 3;
		start->n.pos.y = start->pos.y + dy / 3;
		end->p.pos.x = end->pos.x - dx / 3;
		end->p.pos.y = end->pos.y - dy / 3;
		sp_node_adjust_knot (start, 1);
		sp_node_adjust_knot (end, -1);
	}

	sp_node_ensure_ctrls (start);
	sp_node_ensure_ctrls (end);
}

static SPPathNode *
sp_nodepath_set_node_type (SPPathNode * node, SPPathNodeType type)
{
	g_assert (node);
	g_assert (node->subpath);

	if (type == node->type) return node;

	if ((node->p.other != NULL) && (node->n.other != NULL)) {
		if ((node->code == ART_LINETO) && (node->n.other->code == ART_LINETO)) {
			type = SP_PATHNODE_CUSP;
		}
	}

	node->type = type;
	sp_node_adjust_knots (node);

	return node;
}

static void
sp_node_moveto (SPPathNode * node, double x, double y)
{
	SPNodePath * nodepath;
	ArtPoint p;
	double dx, dy;

	nodepath = node->subpath->nodepath;

	p.x = x;
	p.y = y;

	dx = p.x - node->pos.x;
	dy = p.y - node->pos.y;
	node->pos.x = p.x;
	node->pos.y = p.y;

	node->p.pos.x += dx;
	node->p.pos.y += dy;
	node->n.pos.x += dx;
	node->n.pos.y += dy;

	if (node->p.other) {
		if (node->code == ART_LINETO) {
			sp_node_adjust_knot (node, 1);
			sp_node_adjust_knot (node->p.other, -1);
		}
	}
	if (node->n.other) {
		if (node->n.other->code == ART_LINETO) {
			sp_node_adjust_knot (node, -1);
			sp_node_adjust_knot (node->n.other, 1);
		}
	}

	sp_node_ensure_ctrls (node);
}

static void
sp_nodepath_selected_nodes_move (SPNodePath * nodepath, gdouble dx, gdouble dy)
{
	gdouble dist, besth, bestv, bx, by;
	GSList * l;

	besth = bestv = 1e18;
	bx = dx;
	by = dy;

	for (l = nodepath->selected; l != NULL; l = l->next) {
		SPPathNode * n;
		ArtPoint p;
		n = (SPPathNode *) l->data;
		p.x = n->pos.x + dx;
		p.y = n->pos.y + dy;
		dist = sp_desktop_horizontal_snap (nodepath->desktop, &p);
		if (dist < besth) {
			besth = dist;
			bx = p.x - n->pos.x;
		}
		dist = sp_desktop_vertical_snap (nodepath->desktop, &p);
		if (dist < bestv) {
			bestv = dist;
			by = p.y - n->pos.y;
		}
	}

	for (l = nodepath->selected; l != NULL; l = l->next) {
		SPPathNode * n;
		n = (SPPathNode *) l->data;
		sp_node_moveto (n, n->pos.x + bx, n->pos.y + by);
	}

	update_object (nodepath);
}

static void
sp_node_ensure_knot (SPPathNode * node, gint which, gboolean show_knot)
{
	SPNodePath * nodepath;
	SPPathNodeSide * side;
	ArtPathcode code;
	ArtPoint p;

	g_assert (node != NULL);

	nodepath = node->subpath->nodepath;

	side = sp_node_get_side (node, which);
	code = sp_node_path_code_from_side (node, side);

	show_knot = show_knot && (code == ART_CURVETO);

	if (show_knot) {
		if (!SP_KNOT_IS_VISIBLE (side->knot)) {
			sp_knot_show (side->knot);
		}

		p.x = side->pos.x;
		p.y = side->pos.y;

		sp_knot_set_position (side->knot, &p, 0);
		gnome_canvas_item_show (side->line);

	} else {
		if (SP_KNOT_IS_VISIBLE (side->knot)) {
			sp_knot_hide (side->knot);
		}
		gnome_canvas_item_hide (side->line);
	}
}

void
sp_node_ensure_ctrls (SPPathNode * node)
{
	SPNodePath * nodepath;
	ArtPoint p;
	gboolean show_knots;

	g_assert (node != NULL);

	nodepath = node->subpath->nodepath;

	if (!SP_KNOT_IS_VISIBLE (node->knot)) {
		sp_knot_show (node->knot);
	}

	p.x = node->pos.x;
	p.y = node->pos.y;

	sp_knot_set_position (node->knot, &p, 0);

	show_knots = node->selected;
	if (node->p.other != NULL) {
		if (node->p.other->selected) show_knots = TRUE;
	}
	if (node->n.other != NULL) {
		if (node->n.other->selected) show_knots = TRUE;
	}

	sp_node_ensure_knot (node, -1, show_knots);
	sp_node_ensure_knot (node, 1, show_knots);
}

static void
sp_nodepath_subpath_ensure_ctrls (SPNodeSubPath * subpath)
{
	GSList * l;

	g_assert (subpath != NULL);

	for (l = subpath->nodes; l != NULL; l = l->next) {
		sp_node_ensure_ctrls ((SPPathNode *) l->data);
	}
}

static void
sp_nodepath_ensure_ctrls (SPNodePath * nodepath)
{
	GSList * l;

	g_assert (nodepath != NULL);

	for (l = nodepath->subpaths; l != NULL; l = l->next)
		sp_nodepath_subpath_ensure_ctrls ((SPNodeSubPath *) l->data);
}

void
sp_node_selected_add_node (void)
{
	SPNodePath * nodepath;
	GSList * l, * nl;

	nodepath = sp_nodepath_current ();
	if (!nodepath) return;

	nl = NULL;

	for (l = nodepath->selected; l != NULL; l = l->next) {
		SPPathNode * t;
		t = (SPPathNode *) l->data;
		g_assert (t->selected);
		if (t->p.other && t->p.other->selected) nl = g_slist_prepend (nl, t);
	}

	while (nl) {
		SPPathNode * t, * n;
		t = (SPPathNode *) nl->data;
		n = sp_nodepath_line_add_node (t, 0.5);
		sp_nodepath_node_select (n, TRUE);
		nl = g_slist_remove (nl, t);
	}

	/* fixme: adjust ? */
	sp_nodepath_ensure_ctrls (nodepath);

	update_repr (nodepath);
}

void
sp_node_selected_break (void)
{
	SPNodePath * nodepath;
	SPPathNode * n, * nn;
	GSList * l;

	nodepath = sp_nodepath_current ();
	if (!nodepath) return;

	for (l = nodepath->selected; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		nn = sp_nodepath_node_break (n);
		/* seems that we can prepend here ;-) */
		nn->selected = TRUE;
		nodepath->selected = g_slist_prepend (nodepath->selected, nn);
	}

	sp_nodepath_ensure_ctrls (nodepath);

	update_repr (nodepath);
}

void
sp_node_selected_join (void)
{
	SPNodePath * nodepath;
	SPNodeSubPath * sa, * sb;
	SPPathNode * a, * b, * n;
	ArtPoint p, c;
	ArtPathcode code;

	nodepath = sp_nodepath_current ();
	if (!nodepath) return;
	if (g_slist_length (nodepath->selected) != 2) return;

	a = (SPPathNode *) nodepath->selected->data;
	b = (SPPathNode *) nodepath->selected->next->data;

	g_assert (a != b);

	if ((a->subpath->closed) || (b->subpath->closed)) return;
	if (a->p.other && a->n.other) return;
	g_assert (a->p.other || a->n.other);
	if (b->p.other && b->n.other) return;
	g_assert (b->p.other || b->n.other);
	/* a and b are endpoints */

	c.x = (a->pos.x + b->pos.x) / 2;
	c.y = (a->pos.y + b->pos.y) / 2;

	if (a->subpath == b->subpath) {
		SPNodeSubPath * sp;

		sp = a->subpath;
		sp_nodepath_subpath_close (sp);

		sp_nodepath_ensure_ctrls (sp->nodepath);

		update_repr (nodepath);

		return;
	}

	/* a and b are separate subpaths */
	sa = a->subpath;
	sb = b->subpath;

	if (a == sa->first) {
		SPNodeSubPath * t;
		p = sa->first->n.pos;
		code = sa->first->n.other->code;
		t = sp_nodepath_subpath_new (sa->nodepath);
		n = sa->last;
		sp_nodepath_node_new (t, NULL, SP_PATHNODE_CUSP, ART_MOVETO, &n->n.pos, &n->pos, &n->p.pos);
		n = n->p.other;
		while (n) {
			sp_nodepath_node_new (t, NULL, n->type, n->n.other->code, &n->n.pos, &n->pos, &n->p.pos);
			n = n->p.other;
			if (n == sa->first) n = NULL;
		}
		sp_nodepath_subpath_destroy (sa);
		sa = t;
	} else if (a == sa->last) {
		p = sa->last->p.pos;
		code = sa->last->code;
		sp_nodepath_node_destroy (sa->last);
	} else {
		code = ART_END;
		g_assert_not_reached ();
	}

	if (b == sb->first) {
		sp_nodepath_node_new (sa, NULL, SP_PATHNODE_CUSP, code, &p, &c, &sb->first->n.pos);
		for (n = sb->first->n.other; n != NULL; n = n->n.other) {
			sp_nodepath_node_new (sa, NULL, n->type, n->code, &n->p.pos, &n->pos, &n->n.pos);
		}
	} else if (b == sb->last) {
		sp_nodepath_node_new (sa, NULL, SP_PATHNODE_CUSP, code, &p, &c, &sb->last->p.pos);
		for (n = sb->last->p.other; n != NULL; n = n->p.other) {
			sp_nodepath_node_new (sa, NULL, n->type, n->n.other->code, &n->n.pos, &n->pos, &n->p.pos);
		}
	} else {
		g_assert_not_reached ();
	}
	/* and now destroy sb */

	sp_nodepath_subpath_destroy (sb);

	sp_nodepath_ensure_ctrls (sa->nodepath);

	update_repr (nodepath);
}

void
sp_node_selected_delete (void)
{
	SPNodePath * nodepath;
	SPPathNode * node;

	nodepath = sp_nodepath_current ();
	if (!nodepath) return;
	if (!nodepath->selected) return;

	/* fixme: do it the right way */
	while (nodepath->selected) {
		node = (SPPathNode *) nodepath->selected->data;
		sp_nodepath_node_destroy (node);
	}

	sp_nodepath_ensure_ctrls (nodepath);

	update_repr (nodepath);
}

void
sp_node_selected_set_line_type (ArtPathcode code)
{
	SPNodePath * nodepath;
	GSList * l;

	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;

	for (l = nodepath->selected; l != NULL; l = l->next) {
		SPPathNode * n;
		n = (SPPathNode *) l->data;
		g_assert (n->selected);
		if (n->p.other && n->p.other->selected) {
			sp_nodepath_set_line_type (n, code);
		}
	}

	update_repr (nodepath);
}

void
sp_node_selected_set_type (SPPathNodeType type)
{
	SPNodePath * nodepath;
	GSList * l;

	/* fixme: do it the right way */
	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;

	for (l = nodepath->selected; l != NULL; l = l->next) {
		sp_nodepath_set_node_type ((SPPathNode *) l->data, type);
	}

	update_repr (nodepath);
}

static void
sp_node_set_selected (SPPathNode * node, gboolean selected)
{
	node->selected = selected;

	if (selected) {
		gtk_object_set (GTK_OBJECT (node->knot),
			"fill", NODE_FILL_SEL,
			"fill_mouseover", NODE_FILL_SEL_HI,
			"stroke", NODE_STROKE_SEL,
			"stroke_mouseover", NODE_STROKE_SEL_HI,
			NULL);
	} else {
		gtk_object_set (GTK_OBJECT (node->knot),
			"fill", NODE_FILL,
			"fill_mouseover", NODE_FILL_HI,
			"stroke", NODE_STROKE,
			"stroke_mouseover", NODE_STROKE_HI,
			NULL);
	}

	sp_node_ensure_ctrls (node);
	if (node->n.other) sp_node_ensure_ctrls (node->n.other);
	if (node->p.other) sp_node_ensure_ctrls (node->p.other);
}

void
sp_nodepath_node_select (SPPathNode * node, gboolean incremental)
{
	SPNodePath * nodepath;

	nodepath = node->subpath->nodepath;

	if (incremental) {
		if (node->selected) {
			g_assert (g_slist_find (nodepath->selected, node));
			nodepath->selected = g_slist_remove (nodepath->selected, node);
		} else {
			g_assert (!g_slist_find (nodepath->selected, node));
			nodepath->selected = g_slist_prepend (nodepath->selected, node);
		}
		sp_node_set_selected (node, !node->selected);
	} else {
		while (nodepath->selected) {
			sp_node_set_selected ((SPPathNode *) nodepath->selected->data, FALSE);
			nodepath->selected = g_slist_remove (nodepath->selected, nodepath->selected->data);
		}
		nodepath->selected = g_slist_prepend (nodepath->selected, node);
		sp_node_set_selected (node, TRUE);
	}
}

void
sp_nodepath_select_rect (SPNodePath * nodepath, ArtDRect * b, gboolean incremental)
{
	SPNodeSubPath * subpath;
	SPPathNode * node;
	ArtPoint p;
	GSList * spl, * nl;

	if (!incremental) {
		while (nodepath->selected) {
			sp_node_set_selected ((SPPathNode *) nodepath->selected->data, FALSE);
			nodepath->selected = g_slist_remove (nodepath->selected, nodepath->selected->data);
		}
	}

	for (spl = nodepath->subpaths; spl != NULL; spl = spl->next) {
		subpath = (SPNodeSubPath *) spl->data;
		for (nl = subpath->nodes; nl != NULL; nl = nl->next) {
			node = (SPPathNode *) nl->data;
#if 0
			art_affine_point (&p, &node->pos, nodepath->i2d);
#else
			p.x = node->pos.x;
			p.y = node->pos.y;
#endif
			if ((p.x > b->x0) && (p.x < b->x1) && (p.y > b->y0) && (p.y < b->y1)) {
				sp_nodepath_node_select (node, TRUE);
			}
		}
	}
}

/*
 * Adjusts control point according to node type and line code
 */

static void
sp_node_adjust_knot (SPPathNode * node, gint which_adjust)
{
	SPPathNode * othernode;
	SPPathNodeSide * me, * other;
	ArtPathcode mecode, ocode;
	double len, otherlen, linelen, dx, dy;

	g_assert (node);

	me = sp_node_get_side (node, which_adjust);
	other = sp_node_opposite_side (node, me);

	if (me->other == NULL) return;

	/* I have line */

	if (which_adjust == 1) {
		mecode = me->other->code;
		ocode = node->code;
	} else {
		mecode = node->code;
		ocode = other->other->code;
	}

	if (mecode == ART_LINETO) return;

	/* I am curve */

	if (other->other == NULL) return;

	/* Other has line */

	if (node->type == SP_PATHNODE_CUSP) return;

	if (ocode == ART_LINETO) {
		/* other is lineto, we are either smooth or symm */
		othernode = other->other;
		dx = me->pos.x - node->pos.x;
		dy = me->pos.y - node->pos.y;
		len = hypot (dx, dy);
		dx = node->pos.x - othernode->pos.x;
		dy = node->pos.y - othernode->pos.y;
		linelen = hypot (dx, dy);
		if (linelen < 1e-18) return;

		me->pos.x = node->pos.x + dx * len / linelen;
		me->pos.y = node->pos.y + dy * len / linelen;
		sp_knot_set_position (me->knot, &me->pos, 0);

		sp_node_ensure_ctrls (node);
		return;
	}

	if (node->type == SP_PATHNODE_SYMM) {

		me->pos.x = 2 * node->pos.x - other->pos.x;
		me->pos.y = 2 * node->pos.y - other->pos.y;
		sp_knot_set_position (me->knot, &me->pos, 0);

		sp_node_ensure_ctrls (node);
		return;
	}

	/* We are smooth */

	dx = me->pos.x - node->pos.x;
	dy = me->pos.y - node->pos.y;
	len = hypot (dx, dy);
	dx = other->pos.x - node->pos.x;
	dy = other->pos.y - node->pos.y;
	otherlen = hypot (dx, dy);
	if (otherlen < 1e-18) return;

	me->pos.x = node->pos.x - dx * len / otherlen;
	me->pos.y = node->pos.y - dy * len / otherlen;
	sp_knot_set_position (me->knot, &me->pos, 0);

	sp_node_ensure_ctrls (node);
}

/*
 * Adjusts control point according to node type and line code
 */

static void
sp_node_adjust_knots (SPPathNode * node)
{
	SPNodePath * nodepath;
	double dx, dy, pdx, pdy, ndx, ndy, plen, nlen, scale;

	g_assert (node);

	nodepath = node->subpath->nodepath;

	if (node->type == SP_PATHNODE_CUSP) return;

	/* we are either smooth or symm */

	if (node->p.other == NULL) return;

	if (node->n.other == NULL) return;

	if (node->code == ART_LINETO) {
		if (node->n.other->code == ART_LINETO) return;
		sp_node_adjust_knot (node, 1);
		sp_node_ensure_ctrls (node);
		return;
	}

	if (node->n.other->code == ART_LINETO) {
		if (node->code == ART_LINETO) return;
		sp_node_adjust_knot (node, -1);
		sp_node_ensure_ctrls (node);
		return;
	}

	/* both are curves */

	dx = node->n.pos.x - node->p.pos.x;
	dy = node->n.pos.y - node->p.pos.y;

	if (node->type == SP_PATHNODE_SYMM) {
		node->p.pos.x = node->pos.x - dx / 2;
		node->p.pos.y = node->pos.y - dy / 2;
		node->n.pos.x = node->pos.x + dx / 2;
		node->n.pos.y = node->pos.y + dy / 2;
		sp_node_ensure_ctrls (node);
		return;
	}

	/* We are smooth */

	pdx = node->p.pos.x - node->pos.x;
	pdy = node->p.pos.y - node->pos.y;
	plen = hypot (pdx, pdy);
	if (plen < 1e-18) return;
	ndx = node->n.pos.x - node->pos.x;
	ndy = node->n.pos.y - node->pos.y;
	nlen = hypot (ndx, ndy);
	if (nlen < 1e-18) return;
	scale = plen / (plen + nlen);
	node->p.pos.x = node->pos.x - dx * scale;
	node->p.pos.y = node->pos.y - dy * scale;
	scale = nlen / (plen + nlen);
	node->n.pos.x = node->pos.x + dx * scale;
	node->n.pos.y = node->pos.y + dy * scale;
	sp_node_ensure_ctrls (node);
}

/*
 * Knot events
 */

static void
node_clicked (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	sp_nodepath_node_select (n, (state & GDK_SHIFT_MASK));
}

static void
node_grabbed (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	if (!n->selected) {
		sp_nodepath_node_select (n, (state & GDK_SHIFT_MASK));
	}
}

static void
node_ungrabbed (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	update_repr (n->subpath->nodepath);
}

static gboolean
node_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	/* fixme: This goes to "moved" event? */
	sp_nodepath_selected_nodes_move (n->subpath->nodepath,
					 p->x - knot->x, p->y - knot->y);

	return TRUE;
}

static void
node_ctrl_clicked (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	sp_nodepath_node_select (n, (state & GDK_SHIFT_MASK));
}

static void
node_ctrl_grabbed (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	if (!n->selected) {
		sp_nodepath_node_select (n, (state & GDK_SHIFT_MASK));
	}
}

static void
node_ctrl_ungrabbed (SPKnot * knot, guint state, gpointer data)
{
	SPPathNode * n;

	n = (SPPathNode *) data;

	update_repr (n->subpath->nodepath);
}

static gboolean
node_ctrl_request (SPKnot * knot, ArtPoint * p, guint state, gpointer data)
{
	SPPathNode * n;
	SPPathNodeSide * me, * opposite;
	ArtPathcode othercode;
	gint which;

	n = (SPPathNode *) data;

	if (n->p.knot == knot) {
		me = &n->p;
		opposite = &n->n;
		which = -1;
	} else if (n->n.knot == knot) {
		me = &n->n;
		opposite = &n->p;
		which = 1;
	} else {
		me = opposite = NULL;
		which = 0;
		g_assert_not_reached ();
	}

	othercode = sp_node_path_code_from_side (n, opposite);

	if (opposite->other && (n->type != SP_PATHNODE_CUSP) && (othercode == ART_LINETO)) {
		SPPathNode * othernode;
		gdouble dx, dy, ndx, ndy, len, linelen, scal;
		/* We are smooth node adjacent with line */
		dx = p->x - n->pos.x;
		dy = p->y - n->pos.y;
		len = hypot (dx, dy);
		othernode = opposite->other;
		ndx = n->pos.x - othernode->pos.x;
		ndy = n->pos.y - othernode->pos.y;
		linelen = hypot (ndx, ndy);
		if ((len > 1e-18) && (linelen > 1e-18)) {
			scal = (dx * ndx + dy * ndy) / linelen;
			p->x = n->pos.x + ndx / linelen * scal;
			p->y = n->pos.y + ndy / linelen * scal;
		}
		sp_desktop_vector_snap (n->subpath->nodepath->desktop, p, ndx, ndy);
	} else {
		sp_desktop_free_snap (n->subpath->nodepath->desktop, p);
	}

	sp_node_adjust_knot (n, -which);

	return FALSE;
}

static void
node_ctrl_moved (SPKnot * knot, ArtPoint * p, guint state, gpointer data)
{
	SPPathNode * n;
	SPPathNodeSide * me;

	n = (SPPathNode *) data;

	if (n->p.knot == knot) {
		me = &n->p;
	} else if (n->n.knot == knot) {
		me = &n->n;
	} else {
		me = NULL;
		g_assert_not_reached ();
	}

	sp_ctrlline_set_coords (SP_CTRLLINE (me->line), n->pos.x, n->pos.y, me->pos.x, me->pos.y);

	me->pos.x = p->x;
	me->pos.y = p->y;

	update_object (n->subpath->nodepath);
}

/*
 * Constructors and destructors
 */

static SPNodeSubPath *
sp_nodepath_subpath_new (SPNodePath * nodepath)
{
	SPNodeSubPath * s;

	g_assert (nodepath);
	g_assert (nodepath->desktop);

	s = g_new (SPNodeSubPath, 1);

	s->nodepath = nodepath;
	s->closed = FALSE;
	s->nodes = NULL;
	s->first = NULL;
	s->last = NULL;

	nodepath->subpaths = g_slist_prepend (nodepath->subpaths, s);

	return s;
}

static void
sp_nodepath_subpath_destroy (SPNodeSubPath * subpath)
{
	g_assert (subpath);
	g_assert (subpath->nodepath);
	g_assert (g_slist_find (subpath->nodepath->subpaths, subpath));

	while (subpath->nodes) {
		sp_nodepath_node_destroy ((SPPathNode *) subpath->nodes->data);
	}

	subpath->nodepath->subpaths = g_slist_remove (subpath->nodepath->subpaths, subpath);

	g_free (subpath);
}

static void
sp_nodepath_subpath_close (SPNodeSubPath * sp)
{
	g_assert (!sp->closed);
	g_assert (sp->last != sp->first);
	g_assert (sp->first->code == ART_MOVETO);

	sp->closed = TRUE;

	sp->first->p.other = sp->last;
	sp->last->n.other = sp->first;
	sp->last->n.pos = sp->first->n.pos;

	sp->first = sp->last;

	sp_nodepath_node_destroy (sp->last->n.other);
}

static void
sp_nodepath_subpath_open (SPNodeSubPath * sp, SPPathNode * n)
{
	SPPathNode * new;

	g_assert (sp->closed);
	g_assert (n->subpath == sp);
	g_assert (sp->first == sp->last);

	/* We create new startpoint, current node will become last one */

	new = sp_nodepath_node_new (sp, n->n.other, SP_PATHNODE_CUSP, ART_MOVETO, &n->pos, &n->pos, &n->n.pos);

	sp->closed = FALSE;

	sp->first = new;
	sp->last = n;
	n->n.other = NULL;
	new->p.other = NULL;
}

SPPathNode *
sp_nodepath_node_new (SPNodeSubPath * sp, SPPathNode * next, SPPathNodeType type, ArtPathcode code,
		      ArtPoint * ppos, ArtPoint * pos, ArtPoint * npos)
{
	SPPathNode * n, * prev;

	g_assert (sp);
	g_assert (sp->nodepath);
	g_assert (sp->nodepath->desktop);

	if (nodechunk == NULL) {
		nodechunk = g_mem_chunk_create (SPPathNode, 32, G_ALLOC_AND_FREE);
	}

	n = g_mem_chunk_alloc (nodechunk);

	n->subpath = sp;
	n->type = type;
	n->code = code;
	n->selected = FALSE;
	n->pos = *pos;
	n->p.pos = *ppos;
	n->n.pos = *npos;

	if (next) {
		g_assert (g_slist_find (sp->nodes, next));
		prev = next->p.other;
	} else {
		prev = sp->last;
	}

	if (prev) {
		prev->n.other = n;
	} else {
		sp->first = n;
	}

	if (next) {
		next->p.other = n;
	} else {
		sp->last = n;
	}

	n->p.other = prev;
	n->n.other = next;

	n->knot = sp_knot_new (sp->nodepath->desktop);
	sp_knot_set_position (n->knot, pos, 0);
	gtk_object_set (GTK_OBJECT (n->knot),
			"size", 8,
			"anchor", GTK_ANCHOR_CENTER,
			"fill", NODE_FILL,
			"fill_mouseover", NODE_FILL_HI,
			"stroke", NODE_STROKE,
			"stroke_mouseover", NODE_STROKE_HI,
			NULL);
	gtk_signal_connect (GTK_OBJECT (n->knot), "clicked",
			    GTK_SIGNAL_FUNC (node_clicked), n);
	gtk_signal_connect (GTK_OBJECT (n->knot), "grabbed",
			    GTK_SIGNAL_FUNC (node_grabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->knot), "ungrabbed",
			    GTK_SIGNAL_FUNC (node_ungrabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->knot), "request",
			    GTK_SIGNAL_FUNC (node_request), n);
	sp_knot_show (n->knot);

	n->p.knot = sp_knot_new (sp->nodepath->desktop);
	sp_knot_set_position (n->p.knot, ppos, 0);
	gtk_object_set (GTK_OBJECT (n->p.knot),
			"size", 5,
			"anchor", GTK_ANCHOR_CENTER,
			"fill", KNOT_FILL,
			"fill_mouseover", KNOT_FILL_HI,
			"stroke", KNOT_STROKE,
			"stroke_mouseover", KNOT_STROKE_HI,
			NULL);
	gtk_signal_connect (GTK_OBJECT (n->p.knot), "clicked",
			    GTK_SIGNAL_FUNC (node_ctrl_clicked), n);
	gtk_signal_connect (GTK_OBJECT (n->p.knot), "grabbed",
			    GTK_SIGNAL_FUNC (node_ctrl_grabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->p.knot), "ungrabbed",
			    GTK_SIGNAL_FUNC (node_ctrl_ungrabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->p.knot), "request",
			    GTK_SIGNAL_FUNC (node_ctrl_request), n);
	gtk_signal_connect (GTK_OBJECT (n->p.knot), "moved",
			    GTK_SIGNAL_FUNC (node_ctrl_moved), n);
	sp_knot_hide (n->p.knot);
	n->p.line = gnome_canvas_item_new (SP_DT_CONTROLS (n->subpath->nodepath->desktop),
					       SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (n->p.line);

	n->n.knot = sp_knot_new (sp->nodepath->desktop);
	sp_knot_set_position (n->n.knot, npos, 0);
	gtk_object_set (GTK_OBJECT (n->n.knot),
			"size", 5,
			"anchor", GTK_ANCHOR_CENTER,
			"fill", KNOT_FILL,
			"fill_mouseover", KNOT_FILL_HI,
			"stroke", KNOT_STROKE,
			"stroke_mouseover", KNOT_STROKE_HI,
			NULL);
	gtk_signal_connect (GTK_OBJECT (n->n.knot), "clicked",
			    GTK_SIGNAL_FUNC (node_ctrl_clicked), n);
	gtk_signal_connect (GTK_OBJECT (n->n.knot), "grabbed",
			    GTK_SIGNAL_FUNC (node_ctrl_grabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->n.knot), "ungrabbed",
			    GTK_SIGNAL_FUNC (node_ctrl_ungrabbed), n);
	gtk_signal_connect (GTK_OBJECT (n->n.knot), "request",
			    GTK_SIGNAL_FUNC (node_ctrl_request), n);
	gtk_signal_connect (GTK_OBJECT (n->n.knot), "moved",
			    GTK_SIGNAL_FUNC (node_ctrl_moved), n);
	sp_knot_hide (n->n.knot);
	n->n.line = gnome_canvas_item_new (SP_DT_CONTROLS (n->subpath->nodepath->desktop),
					       SP_TYPE_CTRLLINE, NULL);
	gnome_canvas_item_hide (n->n.line);

	sp->nodes = g_slist_prepend (sp->nodes, n);

	return n;
}

static void
sp_nodepath_node_destroy (SPPathNode * node)
{
	SPNodeSubPath * sp;

	g_assert (node);
	g_assert (node->subpath);
	g_assert (SP_IS_KNOT (node->knot));
	g_assert (SP_IS_KNOT (node->p.knot));
	g_assert (SP_IS_KNOT (node->n.knot));
	g_assert (g_slist_find (node->subpath->nodes, node));

	sp = node->subpath;

	if (node->selected) {
		g_assert (g_slist_find (node->subpath->nodepath->selected, node));
		node->subpath->nodepath->selected = g_slist_remove (node->subpath->nodepath->selected, node);
	}

	node->subpath->nodes = g_slist_remove (node->subpath->nodes, node);
	gtk_object_destroy (GTK_OBJECT (node->knot));
	gtk_object_destroy (GTK_OBJECT (node->p.knot));
	gtk_object_destroy (GTK_OBJECT (node->p.line));
	gtk_object_destroy (GTK_OBJECT (node->n.knot));
	gtk_object_destroy (GTK_OBJECT (node->n.line));

	if (sp->nodes) {
		if (sp->closed) {
			if (sp->first == node) {
				g_assert (sp->last == node);
				sp->first = node->n.other;
				sp->last = sp->first;
			}
			node->p.other->n.other = node->n.other;
			node->n.other->p.other = node->p.other;
		} else {
			if (sp->first == node) sp->first = node->n.other;
			if (sp->last == node) sp->last = node->p.other;
			if (node->p.other) node->p.other->n.other = node->n.other;
			if (node->n.other) node->n.other->p.other = node->p.other;
		}
	}

	g_mem_chunk_free (nodechunk, node);
}

/*
 * Helpers
 */

static SPPathNodeSide *
sp_node_get_side (SPPathNode * node, gint which)
{
	g_assert (node);

	switch (which) {
	case -1:
		return &node->p;
	case 1:
		return &node->n;
	default:
		break;
	}

	g_assert_not_reached ();

	return NULL;
}

static SPPathNodeSide *
sp_node_opposite_side (SPPathNode * node, SPPathNodeSide * me)
{
	g_assert (node);

	if (me == &node->p) return &node->n;
	if (me == &node->n) return &node->p;

	g_assert_not_reached ();

	return NULL;
}

static ArtPathcode
sp_node_path_code_from_side (SPPathNode * node, SPPathNodeSide * me)
{
	g_assert (node);

	if (me == &node->p) {
		if (node->p.other) return node->code;
		return ART_MOVETO;
	}

	if (me == &node->n) {
		if (node->n.other) return node->n.other->code;
		return ART_MOVETO;
	}

	g_assert_not_reached ();

	return ART_END;
}



