#define NODEPATH_C

#include <math.h>
#include "svg/svg.h"
#include "helper/sp-canvas-util.h"
#include "mdi-desktop.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "node-context.h"
#include "nodepath.h"

GMemChunk * nodechunk = NULL;

SPPathNodeSide endside = {NULL, {0.0, 0.0}, NULL, NULL};

static void sp_node_adjust_knot (SPPathNode * node, gint which_adjust);
static void sp_node_adjust_knots (SPPathNode * node);
static void sp_nodepath_ensure_ctrls (SPNodePath * nodepath);
static void sp_node_ensure_ctrls (SPPathNode * node);

static void sp_nodepath_update_object (SPNodePath * nodepath);
static void sp_nodepath_flush (SPNodePath * nodepath);

static SPNodePath *
sp_nodepath_current (void)
{
	SPEventContext * event_context;

	event_context = (SP_ACTIVE_DESKTOP)->event_context;

	if (!SP_IS_NODE_CONTEXT (event_context)) return NULL;

	return &SP_NODE_CONTEXT (event_context)->nodepath;
}

SPNodeSubPath *
sp_nodepath_subpath_new (void)
{
	SPNodeSubPath * s;

	s = g_new (SPNodeSubPath, 1);
	g_return_val_if_fail (s != NULL, NULL);

	/* fixme: use as parameter */
	s->nodepath = NULL;
	s->path_pos = 0;
	s->n_bpaths = 0;
	s->closed = FALSE;
	s->nodes = NULL;
	s->lines = NULL;

	return (s);
}

SPPathNode *
sp_nodepath_node_new (void)
{
	SPPathNode * n;

	if (nodechunk == NULL)
		nodechunk = g_mem_chunk_create (SPPathNode, 32, G_ALLOC_AND_FREE);

	n = g_mem_chunk_alloc (nodechunk);
	g_return_val_if_fail (n != NULL, NULL);

	n->subpath = NULL;
	n->type = SP_PATHNODE_CUSP;
	n->pos.x = n->pos.y = 0.0;
	n->subpath_pos = 0;
	n->subpath_end = 0;
	n->prev = endside;
	n->next = endside;
	n->ctrl = NULL;

	return (n);
}

SPPathLine *
sp_nodepath_line_new (void)
{
	SPPathLine * l;

	l = g_new (SPPathLine, 1);
	g_return_val_if_fail (l != NULL, NULL);

	l->subpath = NULL;
	l->code = ART_LINETO;
	l->start = NULL;
	l->end = NULL;

	return (l);
}

static SPPathNode *
sp_nodepath_line_midpoint (SPPathNode * node, SPPathLine * line, gdouble t)
{
	SPPathNode * start, * end;
	gdouble s;
	gdouble f000, f001, f011, f111, f00t, f0tt, fttt, f11t, f1tt, f01t;

	g_assert (line != NULL);
	g_assert (node != NULL);

	start = line->start;
	end = line->end;

	if (line->code == ART_LINETO) {
		node->type = SP_PATHNODE_CUSP;
		node->pos.x = (start->pos.x + end->pos.x) / 2;
		node->pos.y = (start->pos.y + end->pos.y) / 2;
	} else {
		node->type = SP_PATHNODE_SMOOTH;
		s = 1 - t;
		f000 = start->pos.x;
		f001 = start->next.cpoint.x;
		f011 = end->prev.cpoint.x;
		f111 = end->pos.x;
		f00t = s * f000 + t * f001;
		f01t = s * f001 + t * f011;
		f11t = s * f011 + t * f111;
		f0tt = s * f00t + t * f01t;
		f1tt = s * f01t + t * f11t;
		fttt = s * f0tt + t * f1tt;
		start->next.cpoint.x = f00t;
		node->prev.cpoint.x = f0tt;
		node->pos.x = fttt;
		node->next.cpoint.x = f1tt;
		end->prev.cpoint.x = f11t;
		f000 = start->pos.y;
		f001 = start->next.cpoint.y;
		f011 = end->prev.cpoint.y;
		f111 = end->pos.y;
		f00t = s * f000 + t * f001;
		f01t = s * f001 + t * f011;
		f11t = s * f011 + t * f111;
		f0tt = s * f00t + t * f01t;
		f1tt = s * f01t + t * f11t;
		fttt = s * f0tt + t * f1tt;
		start->next.cpoint.y = f00t;
		node->prev.cpoint.y = f0tt;
		node->pos.y = fttt;
		node->next.cpoint.y = f1tt;
		end->prev.cpoint.y = f11t;
	}
	return node;
}

static SPNodeSubPath *
sp_nodepath_subpath_reverse (SPNodeSubPath * subpath)
{
	SPPathNode * n;
	SPPathNodeSide s;
	SPPathLine * line;
	GList * l;

	g_assert (! subpath->closed);

	/* we can simply change prev <-> next & start <-> end, I hope */
	for (l = subpath->nodes; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		s = n->prev;
		n->prev = n->next;
		n->next = s;
		n->subpath_pos = subpath->n_bpaths - n->subpath_pos - 1;
	}
	for (l = subpath->lines; l != NULL; l = l->next) {
		line = (SPPathLine *) l->data;
		n = line->start;
		line->start = line->end;
		line->end = n;
	}
	return subpath;
}

static SPNodeSubPath *
sp_nodepath_subpath_slideto_last (SPNodeSubPath * subpath)
{
	SPNodePath * nodepath;
	SPNodeSubPath * sp;
	GList * l;
	gint new_pos;

	nodepath = subpath->nodepath;

	new_pos = nodepath->n_bpaths - subpath->n_bpaths - 1;

	if (new_pos == subpath->path_pos) return subpath;

	g_assert (new_pos > subpath->path_pos);

	for (l = nodepath->subpaths; l != NULL; l = l->next) {
		sp = (SPNodeSubPath *) l->data;
		if (sp->path_pos > subpath->path_pos) sp->path_pos -= subpath->n_bpaths;
	}

	subpath->path_pos = new_pos;

	return subpath;
}

static SPNodeSubPath *
sp_nodepath_subpath_rotateto (SPPathNode * node)
{
	SPNodeSubPath * subpath;
	SPPathNode * n;
	GList * l;
	gint len, pos, mov;

	g_assert (node != NULL);
	subpath = node->subpath;
	g_assert (subpath->closed);

	if (node->subpath_pos == 0) return subpath;

	len = subpath->n_bpaths - 1;
	pos = node->subpath_pos;
	mov = len - pos;

	for (l = subpath->nodes; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		n->subpath_pos = (n->subpath_pos + mov) % len;
		n->subpath_end = -1;
	}

	node->subpath_end = len;

	return subpath;
}

static SPPathNodeSide *
sp_node_side_opposite (SPPathNode * node, SPPathNodeSide * me)
{
	g_assert (node != NULL);
	if (me == &node->prev) return &node->next;
	if (me == &node->next) return &node->prev;
	g_assert_not_reached ();
	return NULL;
}

static SPPathNode *
sp_node_adjacent_node (SPPathNode * node, SPPathNodeSide * side)
{
	g_assert (node != NULL);
	g_assert (side != NULL);
	g_assert (side->line != NULL);
	if (side->line->start == node)
		return side->line->end;
	if (side->line->end == node)
		return side->line->start;
	g_assert_not_reached ();
	return NULL;
}

static SPPathNodeSide *
sp_node_side_from_code (SPPathNode * node, gint which)
{
	g_assert (node != NULL);
	switch (which) {
	case -1:
		return &node->prev;
	case 1:
		return &node->next;
	default:
		break;
	}
	g_assert_not_reached ();
	return NULL;
}

static SPPathNode *
sp_nodepath_line_add_node (SPPathLine * line, gdouble t)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath;
	SPPathNode * start, * end, * newnode, * n;
	SPPathLine * newline;
	GList * l;

	g_assert (line != NULL);

	subpath = line->start->subpath;
	nodepath = subpath->nodepath;

	start = line->start;
	end = line->end;
	sp_nodepath_subpath_slideto_last (subpath);
	sp_curve_ensure_space (nodepath->curve, 1);

	nodepath->n_bpaths++;
	subpath->n_bpaths++;
	newnode = sp_nodepath_node_new ();
	newline = sp_nodepath_line_new ();
	sp_nodepath_line_midpoint (newnode, line, t);
	line->end = newnode;
	newnode->subpath = subpath;
	newnode->subpath_pos = start->subpath_pos + 1;
	newnode->subpath_end = -1;
	newnode->prev.line = line;
	newnode->prev.cknot = NULL;
	newnode->prev.cline = NULL;
	newnode->next.line = newline;
	newnode->next.cknot = NULL;
	newnode->next.cline = NULL;
	newnode->selected = FALSE;
	newnode->ctrl = NULL;
	newline->subpath = subpath;
	newline->code = line->code;
	newline->start = newnode;
	newline->end = end;
	end->prev.line = newline;
	for (l = subpath->nodes; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		if (n->subpath_pos > start->subpath_pos) n->subpath_pos++;
		if (n->subpath_end != -1) n->subpath_end++;
	}
	subpath->nodes = g_list_prepend (subpath->nodes, newnode);
	subpath->lines = g_list_prepend (subpath->lines, newline);

	return newnode;
}

static SPPathNode *
sp_nodepath_node_break (SPPathNode * node)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath, * newsubpath;
	SPPathNode * last, * newnode, * n;
	SPPathLine * l;

	g_assert (node != NULL);

	subpath = node->subpath;
	nodepath = subpath->nodepath;
	sp_nodepath_subpath_slideto_last (subpath);

	if (subpath->closed) {
		sp_nodepath_subpath_rotateto (node);
		last = sp_nodepath_node_new ();
		last->subpath = subpath;
		last->type = SP_PATHNODE_CUSP;
		last->pos = node->pos;
		last->subpath_pos = node->subpath_end;
		last->subpath_end = -1;
		last->prev = node->prev;
		last->prev.line->end = last;
		last->next = endside;
		last->selected = FALSE;
		last->ctrl = NULL;
		node->subpath_end = -1;
		node->prev = endside;
		subpath->closed = FALSE;
		subpath->nodes = g_list_prepend (subpath->nodes, last);
		sp_node_adjust_knots (last);
#if 0
		sp_nodepath_update_bpath (nodepath);
		sp_path_change_bpath (nodepath->path, nodepath->bpath);
#endif
		return last;
	}
	/* We are on an open path ;-( */
	if (node->next.line == NULL) return NULL;
	if (node->prev.line == NULL) return NULL;
	sp_curve_ensure_space (nodepath->curve, 1);
	nodepath->n_bpaths++;
	newsubpath = g_new (SPNodeSubPath, 1);
	newsubpath->nodepath = nodepath;
	newsubpath->path_pos = subpath->path_pos + node->subpath_pos + 1;
	newsubpath->n_bpaths = subpath->n_bpaths - node->subpath_pos;
	newsubpath->closed = FALSE;
	newsubpath->nodes = NULL;
	newsubpath->lines = NULL;
	nodepath->subpaths = g_list_prepend (nodepath->subpaths, newsubpath);
	newnode = sp_nodepath_node_new ();
	newnode->subpath = newsubpath;
	newnode->type = SP_PATHNODE_CUSP;
	newnode->pos = node->pos;
	newnode->subpath_pos = 0;
	newnode->subpath_end = -1;
	newnode->prev = endside;
	newnode->next = node->next;
	newnode->next.line->start = newnode;
	newnode->selected = FALSE;
	newnode->ctrl = NULL;
	newsubpath->nodes = g_list_prepend (newsubpath->nodes, newnode);
	node->next = endside;
	subpath->n_bpaths = node->subpath_pos + 1;
	for (l = newnode->next.line; l != NULL; l = l->end->next.line) {
		n = l->end;
		subpath->nodes = g_list_remove (subpath->nodes, n);
		newsubpath->nodes = g_list_prepend (newsubpath->nodes, n);
		n->subpath = newsubpath;
		n->subpath_pos -= node->subpath_pos;
		subpath->lines = g_list_remove (subpath->lines, l);
		newsubpath->lines = g_list_prepend (newsubpath->lines, l);
		l->subpath = newsubpath;
	}

	return newnode;
}

static SPPathLine *
sp_nodepath_set_line_type (SPPathLine * line, ArtPathcode code)
{
	double dx, dy;

	g_assert (line != NULL);

	if (line->code == code) return line;

	line->code = code;

	if (code == ART_LINETO) {
		if (line->start->prev.line != NULL)
			if (line->start->prev.line->code == ART_LINETO)
				line->start->type = SP_PATHNODE_CUSP;
		if (line->end->next.line != NULL)
			if (line->end->next.line->code == ART_LINETO)
				line->end->type = SP_PATHNODE_CUSP;
		sp_node_adjust_knot (line->start, -1);
		sp_node_adjust_knot (line->end, 1);
	} else {
		dx = line->end->pos.x - line->start->pos.x;
		dy = line->end->pos.y - line->start->pos.y;
		line->start->next.cpoint.x = line->start->pos.x + dx / 3;
		line->start->next.cpoint.y = line->start->pos.y + dy / 3;
		line->end->prev.cpoint.x = line->end->pos.x - dx / 3;
		line->end->prev.cpoint.y = line->end->pos.y - dy / 3;
		sp_node_adjust_knot (line->start, 1);
		sp_node_adjust_knot (line->end, -1);
	}

	sp_node_ensure_ctrls (line->start);
	sp_node_ensure_ctrls (line->end);

	return line;
}

static SPPathNode *
sp_nodepath_set_node_type (SPPathNode * node, SPPathNodeType type)
{
	g_assert (node != NULL);

	if (type == node->type) return node;
	if ((node->prev.line != NULL) && (node->next.line != NULL))
		if ((node->prev.line->code == ART_MOVETO) && (node->next.line->code == ART_MOVETO))
			type = SP_PATHNODE_CUSP;
	node->type = type;
	sp_node_adjust_knots (node);

	return node;
}


void
sp_node_selected_add_node (void)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath;
	SPPathNode * new;
	SPPathLine * line;
	GList * lsp, * ll, * nll;

	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;

	nll = NULL;

	for (lsp = nodepath->subpaths; lsp != NULL; lsp = lsp->next) {
		subpath = (SPNodeSubPath *) lsp->data;
		for (ll = subpath->lines; ll != NULL; ll = ll->next) {
			line = (SPPathLine *) ll->data;
			if ((line->start->selected) && (line->end->selected)) {
				nll = g_list_prepend (nll, line);
			}
		}
	}

	while (nll) {
		line = (SPPathLine *) nll->data;
		new = sp_nodepath_line_add_node (line, 0.5);
		sp_nodepath_node_select (new, TRUE);
		nll = g_list_remove (nll, line);
	}

	/* fixme: adjust ? */
	sp_nodepath_ensure_ctrls (nodepath);
	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

void
sp_node_selected_break (void)
{
	SPNodePath * nodepath;
	SPPathNode * n, * nn;
	GList * l;

	nodepath = sp_nodepath_current ();

	if (nodepath == NULL) return;

	for (l = nodepath->sel; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		nn = sp_nodepath_node_break (n);
		/* seems that we can prepend here ;-) */
		nn->selected = TRUE;
		nodepath->sel = g_list_prepend (nodepath->sel, nn);
	}

	sp_nodepath_ensure_ctrls (nodepath);
	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

void
sp_node_selected_join (void)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath, * sa, * sb;
	SPPathNode * a, * b, * t;
	gdouble x, y;
	GList * l;

	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;
	if (g_list_length (nodepath->sel) != 2) return;

	a = (SPPathNode *) nodepath->sel->data;
	b = (SPPathNode *) nodepath->sel->next->data;

	g_assert (a != b);

	if ((a->subpath->closed) || (b->subpath->closed))
		return;
	if ((a->prev.line != NULL) && (a->next.line != NULL))
		return;
	if ((b->prev.line != NULL) && (b->next.line != NULL))
		return;
	/* a and b are endpoints */

	x = (a->pos.x + b->pos.x) / 2;
	y = (a->pos.y + b->pos.y) / 2;

	if (a->subpath == b->subpath) {
		if (a->subpath->n_bpaths < 3) return;
		/* we are at the same subpath & should produce closed path */
		if (b->next.line != NULL) {
			/* b is first */
			t = a; a = b; b = t;
		}
		/* here we are. a is first & b is last */
		subpath = a->subpath;
		subpath->closed = TRUE;
		a->subpath_end = b->subpath_pos;
		a->prev.line = b->prev.line;
		a->prev.line->end = a;
		a->prev.cpoint.x = b->prev.cpoint.x;
		a->prev.cpoint.y = b->prev.cpoint.y;
		a->pos.x = x;
		a->pos.y = y;
		sp_nodepath_node_destroy (b);
		/* we are closed now, so increase bpath count again */
		subpath->n_bpaths++;
		nodepath->n_bpaths++;
		sp_node_adjust_knots (a);
		sp_nodepath_update_bpath (nodepath);

		sp_nodepath_flush (nodepath);

		return;
	}
	/* a and b are separate subpaths */
	sa = a->subpath;
	sb = b->subpath;
	if (sa->path_pos > sb->path_pos) {
		t = a; a = b; b = t;
		sa = a->subpath; sb = b->subpath;
	}
	/* subpath a is before subpath b */
	if (a->next.line != NULL) {
		/* a is FIRST at subpath */
		sp_nodepath_subpath_reverse (sa);
	}
	if (b->prev.line != NULL) {
		/* b is LAST at subpath */
		sp_nodepath_subpath_reverse (sb);
	}

	/* now the chemistry */
	sp_nodepath_subpath_slideto_last (sa);
	sp_nodepath_subpath_slideto_last (sb);
	g_assert (sa->path_pos + sa->n_bpaths == sb->path_pos);
	g_assert (sb->path_pos + sb->n_bpaths + 1 == nodepath->n_bpaths);

	/* here we are: a is last, b is first, sb is next to sa at path end*/
	a->pos.x = x;
	a->pos.y = y;
	a->next.line = b->next.line;
	a->next.line->start = a;
	a->next.cpoint.x = b->next.cpoint.x;
	a->next.cpoint.y = b->next.cpoint.y;
	a->next.cknot = b->next.cknot;
	b->next.cknot = NULL;
	a->next.cline = b->next.cline;
	b->next.cline = NULL;

	/* destroy b */
	g_assert (b->subpath_pos == 0);
	sp_nodepath_node_destroy (b);

	for (l = sb->nodes; l != NULL; l = l->next) {
		t = (SPPathNode *) l->data;
		t->subpath = sa;
		t->subpath_pos += sa->n_bpaths - 1;
	}
	sa->nodes = g_list_concat (sa->nodes, sb->nodes);
	sa->n_bpaths += sb->n_bpaths;

	for (l = sb->lines; l != NULL; l = l->next) {
		((SPPathLine *) l->data)->subpath = sa;
	}
	sa->lines = g_list_concat (sa->lines, sb->lines);

	/* and now destroy sb */

	nodepath->subpaths = g_list_remove (nodepath->subpaths, sb);
	g_free (sb);

	sp_node_adjust_knots (a);
	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

void
sp_node_selected_delete (void)
{
	SPNodePath * nodepath;
	SPPathNode * node;

	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;
	if (nodepath->sel == NULL) return;

	/* fixme: do it the right way */
	while (nodepath->sel) {
		node = (SPPathNode *) nodepath->sel->data;
		sp_nodepath_node_remove (node);
	}
	/* fixme: update knots? */
	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

void
sp_node_selected_set_line_type (ArtPathcode code)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath;
	SPPathLine * line;
	GList * ls, * ll;

	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;

	for (ls = nodepath->subpaths; ls != NULL; ls = ls->next) {
		subpath = (SPNodeSubPath *) ls->data;
		for (ll = subpath->lines; ll != NULL; ll = ll->next) {
			line = (SPPathLine *) ll->data;
			if ((line->start->selected) && (line->end->selected))
				sp_nodepath_set_line_type (line, code);
		}
	}
	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

void
sp_node_selected_set_type (SPPathNodeType type)
{
	SPNodePath * nodepath;
	GList * l;

	/* fixme: do it the right way */
	nodepath = sp_nodepath_current ();
	if (nodepath == NULL) return;

	for (l = nodepath->sel; l != NULL; l = l->next) {
		sp_nodepath_set_node_type ((SPPathNode *) l->data, type);
	}

	sp_nodepath_update_bpath (nodepath);

	sp_nodepath_flush (nodepath);
}

static void
sp_node_set_selected (SPPathNode * node, gboolean selected)
{
	node->selected = selected;
	sp_node_ensure_ctrls (node);
	if (node->next.line != NULL)
		sp_node_ensure_ctrls (node->next.line->end);
	if (node->prev.line != NULL)
		sp_node_ensure_ctrls (node->prev.line->start);
}

void
sp_nodepath_node_select (SPPathNode * node, gboolean incremental)
{
	SPNodePath * nodepath;

	nodepath = node->subpath->nodepath;

	if (incremental) {
		if (node->selected) {
			nodepath->sel = g_list_remove (nodepath->sel, node);
		} else {
			nodepath->sel = g_list_prepend (nodepath->sel, node);
		}
		sp_node_set_selected (node, !node->selected);
	} else {
		while (nodepath->sel) {
			sp_node_set_selected ((SPPathNode *) nodepath->sel->data, FALSE);
			nodepath->sel = g_list_remove (nodepath->sel, nodepath->sel->data);
		}
		nodepath->sel = g_list_prepend (nodepath->sel, node);
		sp_node_set_selected (node, TRUE);
	}
}

void
sp_nodepath_node_select_rect (SPNodePath * nodepath, ArtDRect * b, gboolean incremental)
{
	SPNodeSubPath * subpath;
	SPPathNode * node;
	ArtPoint p;
	GList * spl, * nl;

	if (!incremental) {
		while (nodepath->sel) {
			sp_node_set_selected ((SPPathNode *) nodepath->sel->data, FALSE);
			nodepath->sel = g_list_remove (nodepath->sel, nodepath->sel->data);
		}
	}

	for (spl = nodepath->subpaths; spl != NULL; spl = spl->next) {
		subpath = (SPNodeSubPath *) spl->data;
		for (nl = subpath->nodes; nl != NULL; nl = nl->next) {
			node = (SPPathNode *) nl->data;
			art_affine_point (&p, &node->pos, nodepath->i2d);
			if ((p.x > b->x0) && (p.x < b->x1) && (p.y > b->y0) && (p.y < b->y1)) {
				sp_nodepath_node_select (node, TRUE);
			}
		}
	}
}

static void
sp_node_adjust_knot (SPPathNode * node, gint which_adjust)
{
	SPPathNode * othernode;
	SPPathNodeSide * me, * other;
	double len, otherlen, linelen, dx, dy;

	g_assert (node != NULL);

	me = sp_node_side_from_code (node, which_adjust);
	other = sp_node_side_opposite (node, me);

	if (me->line == NULL) return;
	/* I have line */
	if (me->line->code == ART_LINETO) return;
	/* I am curve */
	if (other->line == NULL) return;
	/* other has line */
	if (node->type == SP_PATHNODE_CUSP) return;
	if (other->line->code == ART_LINETO) {
		/* other is lineto, we are either smooth or symm */
		othernode = sp_node_adjacent_node (node, other);
		dx = me->cpoint.x - node->pos.x;
		dy = me->cpoint.y - node->pos.y;
		len = sqrt (dx * dx + dy * dy);
		dx = node->pos.x - othernode->pos.x;
		dy = node->pos.y - othernode->pos.y;
		linelen = sqrt (dx * dx + dy * dy);
		if (linelen < 1e-18) return;
		me->cpoint.x = node->pos.x + dx * len / linelen;
		me->cpoint.y = node->pos.y + dy * len / linelen;
		sp_node_ensure_ctrls (node);
		return;
	}
	if (node->type == SP_PATHNODE_SYMM) {
		me->cpoint.x = 2 * node->pos.x - other->cpoint.x;
		me->cpoint.y = 2 * node->pos.y - other->cpoint.y;
		sp_node_ensure_ctrls (node);
		return;
	}
	/* We are smooth */
		dx = me->cpoint.x - node->pos.x;
		dy = me->cpoint.y - node->pos.y;
		len = sqrt (dx * dx + dy * dy);
		dx = other->cpoint.x - node->pos.x;
		dy = other->cpoint.y - node->pos.y;
		otherlen = sqrt (dx * dx + dy * dy);
		if (otherlen < 1e-18) return;
		me->cpoint.x = node->pos.x - dx * len / otherlen;
		me->cpoint.y = node->pos.y - dy * len / otherlen;
		sp_node_ensure_ctrls (node);
}

static void
sp_node_adjust_knots (SPPathNode * node)
{
	SPNodePath * nodepath;
	double dx, dy, pdx, pdy, ndx, ndy, plen, nlen, scale;

	g_assert (node != NULL);

	nodepath = node->subpath->nodepath;

	if (node->type == SP_PATHNODE_CUSP) return;
	/* we are either smooth or symm */
	if (node->prev.line == NULL) return;
	if (node->next.line == NULL) return;
	if (node->prev.line->code == ART_LINETO) {
		if (node->next.line->code == ART_LINETO) return;
		sp_node_adjust_knot (node, 1);
		sp_node_ensure_ctrls (node);
#if 0
		sp_node_update_bpath (node);
		sp_path_change_bpath (nodepath->path, nodepath->bpath);
#endif
		return;
	}
	if (node->next.line->code == ART_LINETO) {
		if (node->prev.line->code == ART_LINETO) return;
		sp_node_adjust_knot (node, -1);
		sp_node_ensure_ctrls (node);
#if 0
		sp_node_update_bpath (node);
		sp_path_change_bpath (nodepath->path, nodepath->bpath);
#endif
		return;
	}
	/* both are curves */
	dx = node->next.cpoint.x - node->prev.cpoint.x;
	dy = node->next.cpoint.y - node->prev.cpoint.y;
	if (node->type == SP_PATHNODE_SYMM) {
		node->prev.cpoint.x = node->pos.x - dx / 2;
		node->prev.cpoint.y = node->pos.y - dy / 2;
		node->next.cpoint.x = node->pos.x + dx / 2;
		node->next.cpoint.y = node->pos.y + dy / 2;
		sp_node_ensure_ctrls (node);
#if 0
		sp_node_update_bpath (node);
		sp_path_change_bpath (nodepath->path, nodepath->bpath);
#endif
		return;
	}
	/* We are smooth */
	pdx = node->prev.cpoint.x - node->pos.x;
	pdy = node->prev.cpoint.y - node->pos.y;
	plen = sqrt (pdx * pdx + pdy * pdy);
	if (plen < 1e-18) return;
	ndx = node->next.cpoint.x - node->pos.x;
	ndy = node->next.cpoint.y - node->pos.y;
	nlen = sqrt (ndx * ndx + ndy * ndy);
	if (nlen < 1e-18) return;
	scale = plen / (plen + nlen);
	node->prev.cpoint.x = node->pos.x - dx * scale;
	node->prev.cpoint.y = node->pos.y - dy * scale;
	scale = nlen / (plen + nlen);
	node->next.cpoint.x = node->pos.x + dx * scale;
	node->next.cpoint.y = node->pos.y + dy * scale;
	sp_node_ensure_ctrls (node);
}

static void
sp_knot_moveto (SPPathNode * node, gint which, double x, double y)
{
	SPPathNodeSide * me, * other;
	SPPathNode * othernode;
	SPNodePath * nodepath;
	ArtPoint p;
	double len, linelen, scal, dx, dy, ndx, ndy;

	me = sp_node_side_from_code (node, which);
	other = sp_node_side_opposite (node, me);

	nodepath = node->subpath->nodepath;

	p.x = x;
	p.y = y;
	art_affine_point (&p, &p, nodepath->d2i);

	if (other->line != NULL) {
		if ((node->type != SP_PATHNODE_CUSP) && (other->line->code == ART_LINETO)) {
			/* We are smooth node adjacent with line */
			dx = p.x - node->pos.x;
			dy = p.y - node->pos.y;
			len = sqrt (dx * dx + dy * dy);
			othernode = sp_node_adjacent_node (node, other);
			ndx = node->pos.x - othernode->pos.x;
			ndy = node->pos.y - othernode->pos.y;
			linelen = sqrt (ndx * ndx + ndy * ndy);
			if ((len > 1e-18) && (linelen > 1e-18)) {
				scal = (dx * ndx + dy * ndy) / linelen;
				p.x = node->pos.x + ndx / linelen * scal;
				p.y = node->pos.y + ndy / linelen * scal;
			}
		}
	}

	me->cpoint.x = p.x;
	me->cpoint.y = p.y;
	sp_node_adjust_knot (node, -which);

	/* fixme: this will probably go to adjust_knots */
	sp_node_update_bpath (node);
	sp_node_ensure_ctrls (node);

	sp_nodepath_update_object (nodepath);
}

static gint
sp_knot_event (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	static gint dragging = FALSE;
	static gint dragged = FALSE;
	SPPathNode * node;
	gint handled;
	ArtPoint p;
	gint which;

	handled = FALSE;

	node = (SPPathNode *) data;

	which = 0;
	if ((SPCtrl *) item == node->prev.cknot) which = -1;
	if ((SPCtrl *) item == node->next.cknot) which = 1;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			dragging = TRUE;
			dragged = FALSE;
			break;
		default:
			break;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			if (dragged) sp_nodepath_flush (node->subpath->nodepath);
			if (! node->selected)
				sp_nodepath_node_select (node, TRUE);
			dragging = FALSE;
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		sp_desktop_w2d_xy_point (node->subpath->nodepath->desktop, &p, event->motion.x, event->motion.y);

		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			sp_knot_moveto (node, which, p.x, p.y);
			dragged = TRUE;
			handled = TRUE;
		}

		sp_desktop_set_position (node->subpath->nodepath->desktop, p.x, p.y);

		break;
	case GDK_ENTER_NOTIFY:
		gnome_canvas_item_set (item, "filled", TRUE, NULL);
		handled = TRUE;
		break;
	case GDK_LEAVE_NOTIFY:
		gnome_canvas_item_set (item, "filled", FALSE, NULL);
		handled = TRUE;
		break;
	default:
		break;
	}

	return handled;
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
#if 0
	art_affine_point (&p, &p, nodepath->d2i);
#endif
	dx = p.x - node->pos.x;
	dy = p.y - node->pos.y;
	node->pos.x = p.x;
	node->pos.y = p.y;
	if (node->prev.line != NULL) {
		if (node->prev.line->code == ART_CURVETO) {
			node->prev.cpoint.x += dx;
			node->prev.cpoint.y += dy;
		} else {
			sp_node_adjust_knot (node, 1);
			sp_node_adjust_knot (node->prev.line->start, -1);
		}
	}
	if (node->next.line != NULL) {
		if (node->next.line->code == ART_CURVETO) {
			node->next.cpoint.x += dx;
			node->next.cpoint.y += dy;
		} else {
			sp_node_adjust_knot (node, -1);
			sp_node_adjust_knot (node->next.line->end, 1);
		}
	}

	sp_node_ensure_ctrls (node);
}

static void
sp_nodepath_selected_nodes_move (SPNodePath * nodepath, gdouble dx, gdouble dy)
{
	SPPathNode * n;
	GList * l;

	for (l = nodepath->sel; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		sp_node_moveto (n, n->pos.x + dx, n->pos.y + dy);
	}

	sp_nodepath_update_bpath (nodepath);
	sp_nodepath_update_object (nodepath);
}

static gint
sp_node_event (GnomeCanvasItem * item, GdkEvent * event, gpointer data)
{
	static gboolean dragging = FALSE;
	static gboolean dragged = FALSE;
	static ArtPoint s;
	SPNodePath * nodepath;
	SPPathNode * node;
	gint handled;
	ArtPoint p;

	handled = FALSE;

	node = (SPPathNode *) data;
	nodepath = node->subpath->nodepath;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		switch (event->button.button) {
		case 1:
			sp_desktop_w2d_xy_point (nodepath->desktop, &s, event->button.x, event->button.y);
			art_affine_point (&s, &s, nodepath->d2i);
			dragging = TRUE;
			dragged = FALSE;
			break;
		default:
			break;
		}
		break;
	case GDK_BUTTON_RELEASE:
		switch (event->button.button) {
		case 1:
			dragging = FALSE;
			if (!dragged) {
				if (event->button.state & GDK_SHIFT_MASK) {
					sp_nodepath_node_select (node, TRUE);
				} else {
					sp_nodepath_node_select (node, FALSE);
				}
			} else {
				sp_nodepath_flush (nodepath);
			}
			break;
		default:
			break;
		}
		break;
	case GDK_MOTION_NOTIFY:
		sp_desktop_w2d_xy_point (nodepath->desktop, &p, event->motion.x, event->motion.y);
		sp_desktop_set_position (nodepath->desktop, p.x, p.y);

		if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
			art_affine_point (&p, &p, nodepath->d2i);
			if (!node->selected)
				sp_nodepath_node_select (node, event->motion.state & GDK_SHIFT_MASK);
			sp_nodepath_selected_nodes_move (node->subpath->nodepath, p.x - s.x, p.y - s.y);
			s.x = p.x;
			s.y = p.y;
			dragged = TRUE;
			handled = TRUE;
		}


		break;
	case GDK_ENTER_NOTIFY:
		gnome_canvas_item_set (item, "filled", TRUE, NULL);
		handled = TRUE;
		break;
	case GDK_LEAVE_NOTIFY:
		gnome_canvas_item_set (item, "filled", FALSE, NULL);
		handled = TRUE;
		break;
	default:
		break;
	}

	return handled;
}

static void
sp_node_ensure_knot (SPPathNode * node, gint which, gboolean show_knot)
{
	SPNodePath * nodepath;
	SPPathNodeSide * side;
	ArtPoint p, n;

	g_assert (node != NULL);

	nodepath = node->subpath->nodepath;

	side = NULL;

	switch (which) {
	case -1:
		side = &node->prev;
		break;
	case 1:
		side = &node->next;
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	if (side->line == NULL) {
		show_knot = FALSE;
	} else {
		if (side->line->code != ART_CURVETO)
			show_knot = FALSE;
	}

	if (show_knot) {
		if (side->cknot == NULL) {
			side->cknot = (SPCtrl *) gnome_canvas_item_new (SP_DT_CONTROLS (nodepath->desktop),
				SP_TYPE_CTRL, "anchor", 0, "size", 5.0,
				"filled", FALSE, "fill_color", 0xff0000ff,
				"stroked", TRUE, "stroke_color", 0x000000ff, NULL);
			gtk_signal_connect (GTK_OBJECT (side->cknot), "event",
				GTK_SIGNAL_FUNC (sp_knot_event), node);
		}
		art_affine_point (&p, &side->cpoint, nodepath->i2d);
		sp_ctrl_moveto (side->cknot, p.x, p.y);
		if (side->cline == NULL) {
			side->cline = (SPCtrlLine *) gnome_canvas_item_new (SP_DT_CONTROLS (nodepath->desktop),
				SP_TYPE_CTRLLINE, NULL);
		}
		art_affine_point (&n, &node->pos, nodepath->i2d);
		sp_ctrlline_set_coords (side->cline, n.x, n.y, p.x, p.y);
	} else {
		if (side->cknot != NULL) {
			gtk_object_destroy ((GtkObject *) side->cknot);
			side->cknot = NULL;
		}
		if (side->cline != NULL) {
			gtk_object_destroy ((GtkObject *) side->cline);
			side->cline = NULL;
		}
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

	if (! node->ctrl) {
		node->ctrl = (SPCtrl *) gnome_canvas_item_new (SP_DT_CONTROLS (nodepath->desktop),
			SP_TYPE_CTRL, "anchor", GTK_ANCHOR_CENTER,
			"filled", FALSE, "fill_color", 0xff0000ff,
			"stroked", TRUE,
			NULL);
			gtk_signal_connect (GTK_OBJECT (node->ctrl), "event",
				GTK_SIGNAL_FUNC (sp_node_event), node);
	}
	if (node->selected) {
		gnome_canvas_item_set ((GnomeCanvasItem *) node->ctrl,
			"stroke_color", 0x0000bfff, NULL);
	} else {
		gnome_canvas_item_set ((GnomeCanvasItem *) node->ctrl,
			"stroke_color", 0x0000007f, NULL);
	}

	art_affine_point (&p, &node->pos, nodepath->i2d);
	sp_ctrl_moveto (node->ctrl, p.x, p.y);

	show_knots = node->selected;
	if (node->prev.line != NULL) {
		if (node->prev.line->start->selected) show_knots = TRUE;
	}
	if (node->next.line != NULL) {
		if (node->next.line->end->selected) show_knots = TRUE;
	}

	sp_node_ensure_knot (node, -1, show_knots);
	sp_node_ensure_knot (node, 1, show_knots);
}

static void
sp_nodepath_subpath_ensure_ctrls (SPNodeSubPath * subpath)
{
	GList * l;

	g_assert (subpath != NULL);

	for (l = subpath->nodes; l != NULL; l = l->next) {
		sp_node_ensure_ctrls ((SPPathNode *) l->data);
	}
}

static void
sp_nodepath_ensure_ctrls (SPNodePath * nodepath)
{
	GList * l;

	g_assert (nodepath != NULL);

	for (l = nodepath->subpaths; l != NULL; l = l->next)
		sp_nodepath_subpath_ensure_ctrls ((SPNodeSubPath *) l->data);
}

static void
sp_nodepath_subpath_append_node (SPNodeSubPath * subpath, gint subpath_pos, gint pos, const gchar typestr[])
{
	ArtBpath * bpath;
	SPPathNode * node;
	SPPathLine * line;
	GList * fl, * ll;
	SPPathNode * fn, * ln;

	g_assert (subpath != NULL);
	g_assert (typestr != NULL);

	bpath = subpath->nodepath->curve->bpath + subpath->path_pos + subpath_pos;

	fn = NULL;
	ln = NULL;

	ll = g_list_last (subpath->nodes);
	if (ll != NULL)
		ln = (SPPathNode *) ll->data;
	fl = g_list_first (subpath->nodes);
	if (fl != NULL)
		fn = (SPPathNode *) fl->data;

	if ((pos == 1) && (subpath->closed)) {
		/* we do not render node */
#if 0
		g_assert ((bpath->x3 == fn->pos.x) && (bpath->y3 == fn->pos.y));
#endif
		line = sp_nodepath_line_new ();
		line->subpath = subpath;
		line->code = bpath->code;
		line->start = ln;
		line->end = fn;
		subpath->lines = g_list_prepend (subpath->lines, line);
		fn->subpath_end = subpath_pos;
		ln->next.line = line;
		if (bpath->code == ART_CURVETO) {
			ln->next.cpoint.x = bpath->x1;
			ln->next.cpoint.y = bpath->y1;
		}
		fn->prev.line = line;
		if (bpath->code == ART_CURVETO) {
			fn->prev.cpoint.x = bpath->x2;
			fn->prev.cpoint.y = bpath->y2;
		}
		return;
	}

	node = sp_nodepath_node_new ();
	node->subpath = subpath;
	switch (typestr[subpath->path_pos + subpath_pos]) {
		case 'n':
			node->type = SP_PATHNODE_NONE;
			break;
		case 'c':
			node->type = SP_PATHNODE_CUSP;
			break;
		case 's':
			node->type = SP_PATHNODE_SMOOTH;
			break;
		case 'y':
			node->type = SP_PATHNODE_SYMM;
			break;
		default:
			g_assert_not_reached ();
			break;
	}
#if 0
g_print ("pathcode %c\n", typestr[subpath->path_pos + subpath_pos]);
#endif
	node->pos.x = bpath->x3;
	node->pos.y = bpath->y3;
	node->subpath_pos = subpath_pos;
	node->subpath_end = -1;
	node->selected = FALSE;
	node->ctrl = NULL;

	if (pos == -1) {
		node->prev.line = NULL;
	} else {
		line = sp_nodepath_line_new ();
		line->subpath = subpath;
		line->code = bpath->code;
		line->start = ln;
		line->end = node;
		subpath->lines = g_list_prepend (subpath->lines, line);
		node->prev.line = line;
		ln->next.line = line;
		if (bpath->code == ART_CURVETO) {
			node->prev.cpoint.x = bpath->x2;
			node->prev.cpoint.y = bpath->y2;
			ln->next.cpoint.x = bpath->x1;
			ln->next.cpoint.y = bpath->y1;
		}
	}
	node->prev.cknot = NULL;
	node->prev.cline = NULL;

	node->next.line = NULL;
	node->next.cknot = NULL;
	node->next.cline = NULL;

	subpath->nodes = g_list_append (subpath->nodes, node);
}

static SPNodeSubPath *
sp_nodepath_append_subpath (SPNodePath * nodepath, gint start, gint end, const gchar typestr[])
{
	ArtBpath * bpath;
	SPNodeSubPath * subpath;
	gint n, pos;

	g_assert (nodepath != NULL);
	g_assert (typestr != NULL);

	bpath = nodepath->curve->bpath;

	subpath = sp_nodepath_subpath_new ();
	subpath->nodepath = nodepath;
	subpath->path_pos = start;
	subpath->n_bpaths = end - start;
	switch (bpath[start].code) {
		case ART_MOVETO_OPEN:
			subpath->closed = FALSE;
			break;
		case ART_MOVETO:
			subpath->closed = TRUE;
			break;
		default:
			g_assert_not_reached ();
			break;
	}

	for (n = start; n < end; n++) {
		pos = 0;
		if (n == start) pos = -1;
		if (n == (end - 1)) pos = 1;
		sp_nodepath_subpath_append_node (subpath, n - start, pos, typestr);
	}

	return subpath;
}


/*
 * Destroy methods
 *
 * They DO NOT update bpath, nor do line<->node chemistry
 *
 * Their main reason is freeing corresponding Canvas Items
 *
 */

void
sp_nodepath_subpath_destroy (SPNodeSubPath * subpath)
{
	SPNodePath * nodepath;

	nodepath = subpath->nodepath;

	while (subpath->nodes) {
		sp_nodepath_node_destroy ((SPPathNode *) subpath->nodes->data);
	}
	while (subpath->lines) {
		g_free (subpath->lines->data);
		subpath->lines = g_list_remove (subpath->lines, subpath->lines->data);
	}
	if (subpath->closed) {
		subpath->n_bpaths--;
		nodepath->n_bpaths--;
	}
	g_assert (subpath->n_bpaths == 0);

	nodepath->subpaths = g_list_remove (nodepath->subpaths, subpath);
#if 1
	g_free (subpath);
#endif
}

void
sp_nodepath_node_destroy (SPPathNode * node)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath;

	g_assert (node != NULL);

	subpath = node->subpath;
	nodepath = subpath->nodepath;

	if (node->ctrl)
		gtk_object_destroy (GTK_OBJECT (node->ctrl));
	if (node->prev.cknot)
		gtk_object_destroy (GTK_OBJECT (node->prev.cknot));
	if (node->prev.cline)
		gtk_object_destroy (GTK_OBJECT (node->prev.cline));
	if (node->next.cknot)
		gtk_object_destroy (GTK_OBJECT (node->next.cknot));
	if (node->next.cline)
		gtk_object_destroy (GTK_OBJECT (node->next.cline));

	if (node->selected)
		nodepath->sel = g_list_remove (nodepath->sel, node);

	subpath->nodes = g_list_remove (subpath->nodes, node);

	g_mem_chunk_free (nodechunk, node);

	subpath->n_bpaths--;
	nodepath->n_bpaths--;
}

void
sp_nodepath_subpath_remove (SPNodeSubPath * subpath)
{
	SPNodePath * nodepath;
	SPNodeSubPath * s;
	GList * l;
	gint sub_pos, sub_len;

	nodepath = subpath->nodepath;

	sub_pos = subpath->path_pos;
	sub_len = subpath->n_bpaths;

	while (subpath->nodes)
		sp_nodepath_node_destroy ((SPPathNode *) subpath->nodes->data);

	sp_nodepath_subpath_destroy (subpath);

	for (l = nodepath->subpaths; l != NULL; l = l->next) {
		s = (SPNodeSubPath *) l->data;
		if (s->path_pos > sub_pos) s->path_pos -= sub_len;
	}
}

void
sp_nodepath_node_remove (SPPathNode * node)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath, * sp;
	SPPathNode * prevnode, * nextnode, * n;
	SPPathLine * prevline, * nextline;
	GList * l;

	g_assert (node != NULL);

	subpath = node->subpath;
	nodepath = subpath->nodepath;

	/* lines */

	prevline = node->prev.line;
	nextline = node->next.line;

	if ((prevline != NULL) && (nextline != NULL)) {
		/* we are middle point in open or closed curve*/
		nextnode = nextline->end;
		prevnode = prevline->start;
		if ((prevline->code == ART_CURVETO) || (nextline->code == ART_CURVETO))
			prevline->code = ART_CURVETO;
		prevline->end = nextline->end;
		nextnode->prev.line = prevline;
		/* if we are starting point of closed curve */
		if (node->subpath_end != -1) {
			nextnode->subpath_end = node->subpath_end;
		}
		subpath->lines = g_list_remove (subpath->lines, nextline);
		g_free (nextline);
	} else {
		if (nextline != NULL) {
			/* we are FIRST node on open curve */
			nextnode = nextline->end;
			nextnode->prev.line = NULL;
			subpath->lines = g_list_remove (subpath->lines, nextline);
			g_free (nextline);
		} else {
			/* we are LAST node on open curve */
			prevnode = prevline->start;
			prevnode->next.line = NULL;
			subpath->lines = g_list_remove (subpath->lines, prevline);
			g_free (prevline);
		}
	}

	/* path_pos */

	for (l = subpath->nodes; l != NULL; l = l->next) {
		n = (SPPathNode *) l->data;
		if (n->subpath_pos > node->subpath_pos) n->subpath_pos--;
		if (n->subpath_end != -1) n->subpath_end--;
	}

	sp_nodepath_node_destroy (node);

	for (l = nodepath->subpaths; l != NULL; l = l->next) {
		sp = (SPNodeSubPath *) l->data;
		if (sp->path_pos > subpath->path_pos) sp->path_pos --;
	}

	if (((subpath->closed) && (subpath->n_bpaths < 3)) ||
		((!subpath->closed) && (subpath->n_bpaths < 2))) {
		sp_nodepath_subpath_remove (subpath);
		return;
	}
}

void
sp_node_update_bpath (SPPathNode * node)
{
	SPNodePath * nodepath;
	SPNodeSubPath * subpath;
	SPPathLine * line;
	ArtBpath * bpath;
	gint pos, nextpos;

	subpath = node->subpath;
	nodepath = subpath->nodepath;
	bpath = nodepath->curve->bpath;
	pos = node->subpath_pos + subpath->path_pos;
	nextpos = pos + 1;

	if (node->subpath_end != -1) {
		bpath[pos].code = ART_MOVETO;
		bpath[pos].x3 = node->pos.x;
		bpath[pos].y3 = node->pos.y;
		pos = node->subpath_end + subpath->path_pos;
	}

	bpath[pos].x3 = node->pos.x;
	bpath[pos].y3 = node->pos.y;

	line = node->prev.line;
	if (line == NULL) {
		bpath[pos].code = ART_MOVETO_OPEN;
	} else {
		bpath[pos].code = line->code;
		if (line->code == ART_CURVETO) {
			bpath[pos].x1 = line->start->next.cpoint.x;
			bpath[pos].y1 = line->start->next.cpoint.y;
			bpath[pos].x2 = line->end->prev.cpoint.x;
			bpath[pos].y2 = line->end->prev.cpoint.y;
		}
	}

	line = node->next.line;
	if (line != NULL) {
		if (line->code == ART_CURVETO) {
			bpath[nextpos].x1 = line->start->next.cpoint.x;
			bpath[nextpos].y1 = line->start->next.cpoint.y;
			bpath[nextpos].x2 = line->end->prev.cpoint.x;
			bpath[nextpos].y2 = line->end->prev.cpoint.y;
		}
	}
}

void
sp_nodepath_update_bpath (SPNodePath * nodepath)
{
	GList * lsp, * ln;
	SPNodeSubPath * subpath;
	SPPathNode * node;

	g_assert (nodepath != NULL);

	for (lsp = nodepath->subpaths; lsp != NULL; lsp = lsp->next) {
		subpath = (SPNodeSubPath *) lsp->data;
		for (ln = subpath->nodes; ln != NULL; ln = ln->next) {
			node = (SPPathNode *) ln->data;
			sp_node_update_bpath (node);
		}
	}
	nodepath->curve->bpath[nodepath->n_bpaths - 1].code = ART_END;
}

static void
sp_nodepath_update_object (SPNodePath * nodepath)
{
#if 1
	sp_path_bpath_modified (nodepath->path, nodepath->curve);
#else
	gchar * str;

	str = sp_svg_write_path (nodepath->bpath);
	sp_repr_set_attr (nodepath->repr, "d", str);
	g_free (str);
	sp_repr_set_attr (nodepath->repr, "SODIPODI-PATH-NODE-TYPES", nodepath->typestr);
#endif
}

static void
sp_nodepath_flush (SPNodePath * nodepath)
{
	SPNodeSubPath * subpath;
	SPPathNode * node;
	GList * l, * nl;
	gchar * str;

	str = sp_svg_write_path (nodepath->curve->bpath);
	sp_repr_set_attr (nodepath->repr, "d", str);
	g_free (str);

	str = g_new (gchar, nodepath->n_bpaths + 1);

	for (l = nodepath->subpaths; l != NULL; l = l->next) {
		subpath = (SPNodeSubPath *) l->data;
		for (nl = subpath->nodes; nl != NULL; nl = nl->next) {
			node = (SPPathNode *) nl->data;
			switch (node->type) {
			case SP_PATHNODE_NONE:
				str[subpath->path_pos + node->subpath_pos] = 'n';
				break;
			case SP_PATHNODE_CUSP:
				str[subpath->path_pos + node->subpath_pos] = 'c';
				break;
			case SP_PATHNODE_SMOOTH:
				str[subpath->path_pos + node->subpath_pos] = 's';
				break;
			case SP_PATHNODE_SYMM:
				str[subpath->path_pos + node->subpath_pos] = 'y';
				break;
			default:
				g_assert_not_reached ();
				break;
			}
			if (node->subpath_end >= 0)
				str[subpath->path_pos + node->subpath_end] = 'n';
		}
	}

	str[nodepath->n_bpaths] = '\0';

	sp_repr_set_attr (nodepath->repr, "SODIPODI-PATH-NODE-TYPES", str);
	g_free (str);
}

/*
 * Private methods
 *
 */

static void
sp_nodepath_set_selection (SPNodePath * nodepath)
{
	SPItem * item;
	SPPath * path;
	SPCurve * curve;
	ArtBpath * bpath;
	SPNodeSubPath * subpath;
	gint n_bpaths;
	const gchar * rtstr;
	gchar * typestr;
	gint i, len, substart, subend;

	item = sp_selection_item (SP_DT_SELECTION (nodepath->desktop));

	if (item == NULL) return;
	if (!SP_IS_PATH (item)) return;

	path = SP_PATH (item);

	if (!sp_path_independent (path)) return;

	/* fixme: */
	curve = sp_path_normalize (path);
	if (curve == NULL) return;

	bpath = curve->bpath;
	for (n_bpaths = 0; bpath[n_bpaths].code != ART_END; n_bpaths++);
	n_bpaths++;

	typestr = g_new (gchar, n_bpaths);
	for (i = 0; i < n_bpaths; i++) typestr[i] = 'c';
	typestr[n_bpaths] = '\0';
	rtstr = sp_repr_attr (SP_OBJECT (item)->repr, "SODIPODI-PATH-NODE-TYPES");
	if (rtstr != NULL) {
		len = strlen (rtstr);
		for (i = 0; (i < len) && (i < n_bpaths); i++) typestr[i] = rtstr[i];
	}

	nodepath->repr = SP_OBJECT (item)->repr;
	nodepath->path = path;
	/* fixme: */
	sp_item_i2d_affine (SP_ITEM (path), nodepath->i2d);
	art_affine_invert (nodepath->d2i, nodepath->i2d);
	nodepath->n_bpaths = n_bpaths;
	nodepath->max_bpaths = n_bpaths;
	nodepath->subpaths = NULL;
	nodepath->sel = NULL;
	nodepath->curve = curve;
	sp_curve_ref (curve);

	substart = 0;
	while (substart < n_bpaths - 1) {
		for (i = substart + 1; (bpath[i].code != ART_MOVETO) && (bpath[i].code != ART_MOVETO_OPEN) && (bpath[i].code != ART_END); i++);
		subend = i;
		subpath = sp_nodepath_append_subpath (nodepath, substart, subend, typestr);
		nodepath->subpaths = g_list_append (nodepath->subpaths, subpath);
		substart = subend;
	}

	sp_nodepath_ensure_ctrls (nodepath);

	g_free (typestr);
}

static void
sp_nodepath_clean (SPNodePath * nodepath)
{
	g_return_if_fail (nodepath != NULL);

	while (nodepath->subpaths) {
		sp_nodepath_subpath_destroy ((SPNodeSubPath *) nodepath->subpaths->data);
	}

	g_assert (nodepath->sel == NULL);
	g_assert (nodepath->n_bpaths == 1);

	sp_curve_unref (nodepath->curve);

	nodepath->repr = NULL;
	nodepath->path = NULL;
	nodepath->n_bpaths = 0;
	nodepath->max_bpaths = 0;
	nodepath->subpaths = NULL;
	nodepath->sel = NULL;
	nodepath->curve = NULL;
}

/*
 * Public methods
 *
 */

static void
sp_nodepath_sel_changed (SPSelection * selection, gpointer data)
{
	SPNodePath * nodepath;
	SPItem * item;

	nodepath = (SPNodePath *) data;

	item = sp_selection_item (selection);

	if (nodepath->path)
		sp_nodepath_clean (nodepath);

	if (item)
		sp_nodepath_set_selection (nodepath);
}

void
sp_nodepath_init (SPNodePath * nodepath, SPDesktop * desktop)
{
	nodepath->desktop = desktop;
	nodepath->repr = NULL;
	nodepath->path = NULL;
	nodepath->n_bpaths = 0;
	nodepath->max_bpaths = 0;
	nodepath->subpaths = NULL;
	nodepath->sel = NULL;
	nodepath->curve = NULL;

	nodepath->sel_changed_id = gtk_signal_connect (GTK_OBJECT (SP_DT_SELECTION (desktop)), "changed",
		GTK_SIGNAL_FUNC (sp_nodepath_sel_changed), nodepath);

	sp_nodepath_set_selection (nodepath);
}

void
sp_nodepath_shutdown (SPNodePath * nodepath)
{
	g_return_if_fail (nodepath != NULL);

#if 0
	/* nodepath is always flushed, except when dragging */
	sp_nodepath_flush (nodepath);
#endif

	if (nodepath->sel_changed_id)
		gtk_signal_disconnect (GTK_OBJECT (SP_DT_SELECTION (nodepath->desktop)), nodepath->sel_changed_id);

	if (nodepath->path) {
		while (nodepath->subpaths) {
			sp_nodepath_subpath_destroy ((SPNodeSubPath *) nodepath->subpaths->data);
		}

		g_assert (nodepath->sel == NULL);
		g_assert (nodepath->n_bpaths == 1);
#if 1
		sp_curve_unref (nodepath->curve);
#endif
	}
}

