#define __SP_SELECTION_CHEMISTRY_C__

/*
 * Miscellanous operations on selected items
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "svg/svg.h"
#include "xml/repr-private.h"
#include "verbs.h"
#include "document.h"
#include "sodipodi.h"
#include "desktop.h"
#include "selection.h"
#include "desktop-handles.h"
#include "sp-item-transform.h" 
#include "sp-item-group.h"
#include "sp-path.h"
#include "path-chemistry.h"

#include "selection-chemistry.h"

/* fixme: find a better place */
GSList *clipboard = NULL;

static SPGroup *sp_item_list_common_parent_group (const GSList *items);

/* Action based functionality */

static void sp_selection_perform_transform (SPDesktop *dt, SPRepr *config);

static void sp_selection_raise (SPDesktop *dt);
static void sp_selection_raise_to_top (SPDesktop *dt);
static void sp_selection_lower (SPDesktop *dt);
static void sp_selection_lower_to_bottom (SPDesktop *dt);

static void sp_selection_group (SPDesktop *dt);
static void sp_selection_ungroup (SPDesktop *dt);

void
sp_selection_action_perform (SPAction *action, void *config, void *data)
{
	SPDesktop *dt;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;

	switch ((int) data) {
	case SP_VERB_SELECTION_TO_FRONT:
		sp_selection_raise_to_top (dt);
		break;
	case SP_VERB_SELECTION_TO_BACK:
		sp_selection_lower_to_bottom (dt);
		break;
	case SP_VERB_SELECTION_RAISE:
		sp_selection_raise (dt);
		break;
	case SP_VERB_SELECTION_LOWER:
		sp_selection_lower (dt);
		break;
	case SP_VERB_SELECTION_GROUP:
		sp_selection_group (dt);
		break;
	case SP_VERB_SELECTION_UNGROUP:
		sp_selection_ungroup (dt);
		break;
	case SP_VERB_SELECTION_COMBINE:
		sp_selected_path_combine ();
		break;
	case SP_VERB_SELECTION_BREAK_APART:
		sp_selected_path_break_apart ();
		break;
	case SP_VERB_SELECTION_TRANSFORM:
		sp_selection_perform_transform (dt, (SPRepr *) config);
		break;
	default:
		break;
	}
}

static void
sp_selection_perform_transform (SPDesktop *dt, SPRepr *config)
{
	unsigned int duplicate;
	const unsigned char *tstr;
	NRMatrixF transform;
	const GSList *l;

	if (!config) return;

	/* Duplication (new objects get selected) */
	sp_repr_get_boolean (config, "duplicate", &duplicate);
	if (duplicate) sp_selection_duplicate (NULL, NULL);
	/* Transformation */
	tstr = sp_repr_attr (config, "transform");
	if (!tstr) return;
	nr_matrix_f_set_identity (&transform);
	sp_svg_transform_read (tstr, &transform);

	for (l = sp_selection_item_list (SP_DT_SELECTION (dt)); l != NULL; l = l->next) {
		NRMatrixF curaff, newaff;
		SPItem *item;
		item = (SPItem *) l->data;
		sp_item_i2d_affine (item, &curaff);
		nr_matrix_multiply_fff (&newaff, &curaff, &transform);
		/* fixme: This is far from elegant (Lauris) */
		sp_item_set_i2d_affine (item, &newaff);
		/* update repr -  needed for undo */
		sp_item_write_transform (item, SP_OBJECT_REPR (item), &item->transform);
	}
}

#if 0
#define PRINT_STR(s) g_print (s)
#define PRINT_OBJ(s, o) g_print ("%s: %s\n", s, (o) ? (gchar *) sp_repr_attr (SP_OBJECT_REPR (o), "id") : "NULL")
#else
#define PRINT_STR(s)
#define PRINT_OBJ(s, o)
#endif

static void
sp_selection_raise (SPDesktop *dt)
{
	const GSList *items;
	SPGroup *group;
	SPRepr *grepr;
	SPObject *child, *newref;
	GSList *rev;

	items = sp_selection_item_list (SP_DT_SELECTION (dt));
	if (!items) return;
	group = sp_item_list_common_parent_group (items);
	if (!group) return;
	grepr = SP_OBJECT_REPR (group);

	/* construct reverse-ordered list of selected children */
	rev = NULL;
	for (child = ((SPObject *) group)->children; child; child = child->next) {
		if (g_slist_find ((GSList *) items, child)) {
			rev = g_slist_prepend (rev, child);
		}
	}

	while (rev) {
		child = SP_OBJECT (rev->data);
		for (newref = child->next; newref; newref = newref->next) {
			if (SP_IS_ITEM (newref)) {
				if (!g_slist_find ((GSList *) items, newref)) {
					/* Found available position */
					sp_repr_change_order (grepr, SP_OBJECT_REPR (child), SP_OBJECT_REPR (newref));
				}
				break;
			}
		}
		rev = g_slist_remove (rev, child);
	}

	sp_document_done (SP_DT_DOCUMENT (dt));
}

static void
sp_selection_raise_to_top (SPDesktop *dt)
{
	SPRepr *repr;
	GSList *rl, *l;

	if (sp_selection_is_empty (SP_DT_SELECTION (dt))) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (SP_DT_SELECTION (dt)));

	for (l = rl; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		sp_repr_set_position_absolute (repr, -1);
	}

	g_slist_free (rl);

	sp_document_done (SP_DT_DOCUMENT (dt));
}

static void
sp_selection_lower (SPDesktop *dt)
{
	const GSList *items;
	SPGroup *group;
	SPRepr *grepr;
	SPObject *child, *newref, *oldref;
	gboolean skip;

	items = sp_selection_item_list (SP_DT_SELECTION (dt));
	if (!items) return;
	group = sp_item_list_common_parent_group (items);
	if (!group) return;
	grepr = SP_OBJECT_REPR (group);

	PRINT_STR ("STARTING\n");
	/* Start from beginning */
	skip = TRUE;
	newref = NULL;
	oldref = NULL;
	child = ((SPObject *) group)->children;
	while (child != NULL) {
		if (SP_IS_ITEM (child)) {
			/* We are item */
			skip = FALSE;
			/* fixme: Remove from list (Lauris) */
			if (g_slist_find ((GSList *) items, child)) {
				/* Need lower */
				if (newref != oldref) {
					if (sp_repr_change_order (grepr, SP_OBJECT_REPR (child), (newref) ? SP_OBJECT_REPR (newref) : NULL)) {
						PRINT_STR ("Change order succeeded\n");
						PRINT_OBJ ("  child", child);
						PRINT_OBJ ("  oldref", oldref);
						PRINT_OBJ ("  newref", newref);
						/* Order change succeeded */
						/* Next available position */
						newref = child;
						/* Oldref is just what it was */
						/* Continue from oldref */
						child = oldref->next;
					} else {
						PRINT_STR ("Change order failed\n");
						PRINT_OBJ ("  child", child);
						PRINT_OBJ ("  oldref", oldref);
						PRINT_OBJ ("  newref", newref);
						/* Order change did not succeed */
						newref = oldref;
						oldref = child;
						child = child->next;
					}
				} else {
					/* Item position will not change */
					/* Other items will lower only following positions */
					newref = child;
					oldref = child;
					child = child->next;
				}
			} else {
				PRINT_STR ("Item not in list\n");
				PRINT_OBJ ("  child", child);
				PRINT_OBJ ("  oldref", oldref);
				PRINT_OBJ ("  newref", newref);
				/* We were item, but not in list */
				newref = oldref;
				oldref = child;
				child = child->next;
			}
		} else {
			PRINT_STR ("Not an item\n");
			PRINT_OBJ ("  child", child);
			PRINT_OBJ ("  oldref", oldref);
			PRINT_OBJ ("  newref", newref);
			/* We want to refind newref only to skip initial non-items */
			if (skip) newref = child;
			oldref = child;
			child = child->next;
		}
	}

	sp_document_done (SP_DT_DOCUMENT (dt));
}

static void
sp_selection_lower_to_bottom (SPDesktop *dt)
{
	SPRepr *repr;
	GSList *rl, *l;

	if (sp_selection_is_empty (SP_DT_SELECTION (dt))) return;

	rl = g_slist_copy ((GSList *) sp_selection_repr_list (SP_DT_SELECTION (dt)));

	rl = g_slist_reverse (rl);

	for (l = rl; l != NULL; l = l->next) {
		gint minpos;
		SPObject *pp, *pc;
		repr = (SPRepr *) l->data;
		pp = sp_document_lookup_id (SP_DT_DOCUMENT (dt), sp_repr_attr (sp_repr_parent (repr), "id"));
		minpos = 0;
		g_assert (SP_IS_GROUP (pp));
		pc = pp->children;
		while (!SP_IS_ITEM (pc)) {
			minpos += 1;
			pc = pc->next;
		}
		sp_repr_set_position_absolute (repr, minpos);
	}

	g_slist_free (rl);

	sp_document_done (SP_DT_DOCUMENT (dt));
}

/* fixme: sequencing */

static void
sp_selection_group (SPDesktop *dt)
{
	SPSelection * selection;
	SPRepr * current;
	SPRepr * group;
	SPItem * new;
	const GSList * l;
	GSList * p;

	selection = SP_DT_SELECTION (dt);
	if (sp_selection_is_empty (selection)) return;
	l = sp_selection_repr_list (selection);
	if (l->next == NULL) return;

	p = g_slist_copy ((GSList *) l);

	sp_selection_empty (SP_DT_SELECTION (dt));
	p = g_slist_sort (p, (GCompareFunc) sp_repr_compare_position);

	group = sp_repr_new ("g");

	while (p) {
		SPRepr *new;
		current = (SPRepr *) p->data;
		new = sp_repr_duplicate (current);
		sp_repr_unparent (current);
		sp_repr_append_child (group, new);
		sp_repr_unref (new);
		p = g_slist_remove (p, current);
	}

	new = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (dt), group);
	sp_document_done (SP_DT_DOCUMENT (dt));

	sp_selection_set_repr (selection, group);
	sp_repr_unref (group);
}

static void
sp_selection_ungroup (SPDesktop *dt)
{
	SPItem *group;
	GSList *children;

	group = sp_selection_item (SP_DT_SELECTION (dt));
	if (!group) return;
	/* We do not allow ungrouping <svg> etc. */
	if (strcmp (sp_repr_name (SP_OBJECT_REPR (group)), "g")) return;

	children = NULL;
	/* This is not strictly required, but is nicer to rely on group ::destroy */
	sp_selection_empty (SP_DT_SELECTION (dt));
	sp_item_group_ungroup (SP_GROUP (group), &children);
	sp_selection_set_item_list (SP_DT_SELECTION (dt), children);
	g_slist_free (children);
}

/* Simple stuff */

static void sp_matrix_d_set_rotate (NRMatrixD *m, double theta);

void
sp_selection_delete (gpointer object, gpointer data)
{
	SPDesktop *desktop;
	SPSelection *selection;
	GSList *selected;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	selected = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sp_selection_empty (selection);

	while (selected) {
		sp_repr_unparent ((SPRepr *) selected->data);
		selected = g_slist_remove (selected, selected->data);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

/* fixme: sequencing */
void sp_selection_duplicate (gpointer object, gpointer data)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * selected, * newsel;
	SPRepr * copy;
	SPItem * item;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	selection = SP_DT_SELECTION (desktop);

	if (sp_selection_is_empty (selection)) return;

	selected = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sp_selection_empty (selection);

	selected = g_slist_sort (selected, (GCompareFunc) sp_repr_compare_position);

	newsel = NULL;

	while (selected) {
		copy = sp_repr_duplicate ((SPRepr *) selected->data);
		item = (SPItem *) sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		g_assert (item != NULL);
		newsel = g_slist_prepend (newsel, copy);
		selected = g_slist_remove (selected, selected->data);
		sp_repr_unref (copy);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));

	sp_selection_set_repr_list (SP_DT_SELECTION (desktop), newsel);

	g_slist_free (newsel);
}

void
sp_edit_clear_all (gpointer object, gpointer data)
{
	SPDesktop *dt;
	SPDocument *doc;
	GSList *items;

	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return;
	doc = SP_DT_DOCUMENT (dt);
	sp_selection_set_empty (SP_DT_SELECTION (dt));

	items = sp_item_group_item_list (SP_GROUP (sp_document_root (doc)));

	while (items) {
		sp_repr_unparent (SP_OBJECT_REPR (items->data));
		items = g_slist_remove (items, items->data);
	}

	sp_document_done (doc);
}

void
sp_edit_select_all (gpointer object, gpointer data)
{
	SPDesktop *dt;
	SPDocument *doc;
	GSList *items;
	SPSelection * selection;
	SPRepr * repr;
	
	dt = SP_ACTIVE_DESKTOP;
	if (!dt) return ;
	doc 	  = SP_DT_DOCUMENT (dt);
	selection = SP_DT_SELECTION (dt);
	items = sp_item_group_item_list (SP_GROUP (sp_document_root (doc)));
	
	while (items) {
		repr  = SP_OBJECT_REPR (items->data);
		if (!sp_selection_repr_selected (selection, repr))
			sp_selection_add_repr (selection, repr);
		items = g_slist_remove (items, items->data);
	}
	sp_document_done (doc);
}

static void
sp_group_cleanup (SPGroup *group)
{
	SPObject *child;
	GSList *l;

	l = NULL;
	for (child = ((SPObject *) group)->children; child != NULL; child = child->next) {
		sp_object_ref (child, NULL);
		l = g_slist_prepend (l, child);
	}

	while (l) {
		if (SP_IS_GROUP (l->data)) {
			sp_group_cleanup (SP_GROUP (l->data));
		} else if (SP_IS_PATH (l->data)) {
			sp_path_cleanup (SP_PATH (l->data));
		}
		sp_object_unref (SP_OBJECT (l->data), NULL);
		l = g_slist_remove (l, l->data);
	}


	if (!strcmp (sp_repr_name (SP_OBJECT_REPR (group)), "g")) {
		gint numitems;
		numitems = 0;
		for (child = ((SPObject *) group)->children; child != NULL; child = child->next) {
			if (SP_IS_ITEM (child)) numitems += 1;
		}
		if (numitems <= 1) {
			sp_item_group_ungroup (group, NULL);
		}
	}
}

void
sp_edit_cleanup (gpointer object, gpointer data)
{
	SPDocument *doc;
	SPGroup *root;

	doc = SP_ACTIVE_DOCUMENT;
	if (!doc) return;
	if (SP_ACTIVE_DESKTOP) {
		sp_selection_empty (SP_DT_SELECTION (SP_ACTIVE_DESKTOP));
	}

	root = SP_GROUP (SP_DOCUMENT_ROOT (doc));

	sp_group_cleanup (root);

	sp_document_done (doc);
}

static SPGroup *
sp_item_list_common_parent_group (const GSList *items)
{
	SPObject *parent;

	if (!items) return NULL;
	parent = SP_OBJECT_PARENT (items->data);
	/* Strictly speaking this CAN happen, if user selects <svg> from XML editor */
	if (!SP_IS_GROUP (parent)) return NULL;
	for (items = items->next; items; items = items->next) {
		if (SP_OBJECT_PARENT (items->data) != parent) return NULL;
	}

	return SP_GROUP (parent);
}

#if 0
void
sp_undo (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (SP_IS_DESKTOP(desktop)) {
		sp_document_undo (SP_DT_DOCUMENT (desktop));
	}
}

void
sp_redo (GtkWidget * widget)
{
	SPDesktop * desktop;

	desktop = SP_ACTIVE_DESKTOP;
	if (SP_IS_DESKTOP(desktop)) {
		sp_document_redo (SP_DT_DOCUMENT (desktop));
	}
}
#endif

void
sp_selection_cut (GtkWidget * widget)
{
	sp_selection_copy (widget);
	sp_selection_delete (NULL, NULL);
}

void
sp_selection_copy (GtkWidget * widget)
{
	SPDesktop *desktop;
	SPSelection *selection;
	SPRepr *repr, *copy;
	SPCSSAttr *css;
	GSList *sl;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;

	/* Return if selection is empty */
	selection = SP_DT_SELECTION (desktop);
	if (sp_selection_is_empty (selection)) return;

	sl = g_slist_copy ((GSList *) sp_selection_repr_list (selection));
	sl = g_slist_sort (sl, (GCompareFunc) sp_repr_compare_position);

	/* Clear old clipboard */
	while (clipboard) {
		sp_repr_unref ((SPRepr *) clipboard->data);
		clipboard = g_slist_remove (clipboard, clipboard->data);
	}

	while (sl != NULL) {
		repr = (SPRepr *) sl->data;
		sl = g_slist_remove (sl, repr);
		css = sp_repr_css_attr_inherited (repr, "style");
		copy = sp_repr_duplicate (repr);
		sp_repr_css_set (copy, css, "style");
		sp_repr_css_attr_unref (css);

		clipboard = g_slist_prepend (clipboard, copy);
	}

	clipboard = g_slist_reverse (clipboard);
}

void
sp_selection_paste (GtkWidget * widget)
{
	SPDesktop * desktop;
	SPSelection * selection;
	GSList * l;
	SPRepr * repr, * copy;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	g_assert (SP_IS_DESKTOP (desktop));

	selection = SP_DT_SELECTION (desktop);
	g_assert (selection != NULL);
	g_assert (SP_IS_SELECTION (selection));

	sp_selection_empty (selection);

	for (l = clipboard; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		copy = sp_repr_duplicate (repr);
		sp_document_add_repr (SP_DT_DOCUMENT (desktop), copy);
		sp_selection_add_repr (selection, copy);
		sp_repr_unref (copy);
	}

	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_apply_affine (SPSelection *selection, NRMatrixF *transform, unsigned int duplicate, unsigned int repeatable)
{
	unsigned char tstr[128];
	SPRepr *config;
	config = sp_repr_new ("config");
	sp_repr_set_boolean (config, "duplicate", duplicate);
	sp_svg_transform_write (tstr, 128, transform);
	sp_repr_set_attr (config, "transform", tstr);

	if (repeatable) {
		sodipodi_verb_perform (SP_VERB_SELECTION_TRANSFORM, config);
	} else {
		SPAction *action;
		action = sp_verb_get_action (SP_VERB_SELECTION_TRANSFORM);
		if (action) sp_action_perform (action, config);
	}

	sp_repr_unref (config);
}


void
sp_selection_remove_transform (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	const GSList * l;

	desktop = SP_ACTIVE_DESKTOP;
	if (desktop == NULL) return;
	selection = SP_DT_SELECTION (desktop);
	if (!SP_IS_SELECTION (selection)) return;

	l = sp_selection_repr_list (selection);

	while (l != NULL) {
		sp_repr_set_attr (l->data,"transform", NULL);
		l = l->next;
	}

	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}


void
sp_selection_scale_absolute (SPSelection *selection, double x0, double x1, double y0, double y1)
{
	NRRectF bbox;
	NRMatrixD p2o, o2n, scale, s;
	NRMatrixF final;
	double dx, dy, nx, ny;
  
	g_assert (SP_IS_SELECTION (selection));

	sp_selection_bbox (selection, &bbox);

	nr_matrix_d_set_translate (&p2o, -bbox.x0, -bbox.y0);

	dx = (x1-x0) / (bbox.x1 - bbox.x0);
	dy = (y1-y0) / (bbox.y1 - bbox.y0);
	nr_matrix_d_set_scale (&scale, dx, dy);

	nx = x0;
	ny = y0;
	nr_matrix_d_set_translate (&o2n, nx, ny);

	nr_matrix_multiply_ddd (&s, &p2o, &scale);
	nr_matrix_multiply_fdd (&final, &s, &o2n);

	sp_selection_apply_affine (selection, &final, FALSE, FALSE);
}


void
sp_selection_scale_relative (SPSelection *selection, NRPointF *align, double dx, double dy, unsigned int duplicate)
{
	NRMatrixD scale, n2d, d2n, s;
	NRMatrixF final;

	nr_matrix_d_set_translate (&n2d, -align->x, -align->y);
	nr_matrix_d_set_translate (&d2n, align->x, align->y);
	nr_matrix_d_set_scale (&scale, dx, dy);

	nr_matrix_multiply_ddd (&s, &n2d, &scale);
	nr_matrix_multiply_fdd (&final, &s, &d2n);

	sp_selection_apply_affine (selection, &final, duplicate, TRUE);
}


void
sp_selection_rotate_relative (SPSelection *selection, NRPointF *center, double angle, unsigned int duplicate)
{
	NRMatrixD rotate, n2d, d2n, s;
	NRMatrixF final;
  
	nr_matrix_d_set_translate (&n2d, -center->x, -center->y);
	nr_matrix_d_invert (&d2n, &n2d);
	sp_matrix_d_set_rotate (&rotate, angle);

	nr_matrix_multiply_ddd (&s, &n2d, &rotate);
	nr_matrix_multiply_fdd (&final, &s, &d2n);

	sp_selection_apply_affine (selection, &final, duplicate, TRUE);
}


void
sp_selection_skew_relative (SPSelection *selection, NRPointF *align, double dx, double dy)
{
	NRMatrixD skew, n2d, d2n, s;
	NRMatrixF final;
  
	nr_matrix_d_set_translate (&n2d, -align->x, -align->y);
	nr_matrix_d_invert (&d2n, &n2d);

	skew.c[0] = 1;
	skew.c[1] = dy;
	skew.c[2] = dx;
	skew.c[3] = 1;
	skew.c[4] = 0;
	skew.c[5] = 0;

	nr_matrix_multiply_ddd (&s, &n2d, &skew);
	nr_matrix_multiply_fdd (&final, &s, &d2n);

	sp_selection_apply_affine (selection, &final, FALSE, TRUE);
}


void
sp_selection_move_relative (SPSelection * selection, double dx, double dy, unsigned int duplicate)
{
	NRMatrixF move;
  
	nr_matrix_f_set_translate (&move, dx, dy);

	sp_selection_apply_affine (selection, &move, duplicate, TRUE);
}

void
sp_selection_rotate_90 (void)
{
	SPDesktop * desktop;
	SPSelection * selection;
	SPItem * item;
	GSList * l, * l2;

	desktop = SP_ACTIVE_DESKTOP;
	if (!SP_IS_DESKTOP(desktop)) return;
	selection = SP_DT_SELECTION(desktop);
	if sp_selection_is_empty(selection) return;
	l = selection->items;  
	for (l2 = l; l2 != NULL; l2 = l2-> next) {
		item = SP_ITEM (l2->data);
		sp_item_rotate_rel (item,-90);
	}

	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}

void
sp_selection_move_screen (gdouble sx, gdouble sy)
{
	SPDesktop * desktop;
	SPSelection * selection;
	gdouble dx,dy,zf;

	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(SP_IS_DESKTOP (desktop));
	selection = SP_DT_SELECTION (desktop);
	if (!SP_IS_SELECTION (selection)) return;
	if sp_selection_is_empty(selection) return;

	zf = SP_DESKTOP_ZOOM (desktop);
	dx = sx / zf;
	dy = sy / zf;
	sp_selection_move_relative (selection, dx, dy, FALSE);

	//
	sp_selection_changed (selection);
	sp_document_done (SP_DT_DOCUMENT (desktop));
}

static unsigned int
sp_cycle_is_global (void)
{
	SPRepr *repr;
	const unsigned char *val;

	repr = sodipodi_get_repr (SODIPODI, "tools.select.cycling");
	if (!repr) return TRUE;
	val = sp_repr_attr (repr, "scope");
	if (!val) return TRUE;
	return strcmp (val, "visible");
}

static unsigned int
sp_cycle_keep_focus (void)
{
	SPRepr *repr;
	const unsigned char *val;

	repr = sodipodi_get_repr (SODIPODI, "tools.select.cycling");
	if (!repr) return TRUE;
	val = sp_repr_attr (repr, "focus");
	if (!val) return TRUE;
	return strcmp (val, "follow");
}

void
sp_selection_item_next (void)
{
	SPDocument *document;
	SPDesktop *desktop;
	SPSelection *selection;
	SPItem *item = NULL;
	GSList *children = NULL, *l = NULL;
	NRRectF dbox;

	document = SP_ACTIVE_DOCUMENT;
	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(document != NULL);
	g_return_if_fail(desktop != NULL);
	if (!SP_IS_DESKTOP (desktop)) return;
	selection = SP_DT_SELECTION(desktop);
	g_return_if_fail(selection!=NULL);
  
	/* Cycling type */
	if (sp_cycle_is_global ()) {
		children = sp_item_group_item_list (SP_GROUP(sp_document_root(document)));
	} else {
		NRRectD d;
		sp_desktop_get_display_area (desktop, &dbox);
		d.x0 = dbox.x0;
		d.y0 = dbox.y0;
		d.x1 = dbox.x1;
		d.y1 = dbox.y1;
		children = sp_document_partial_items_in_box (document, &d);
	}

	/* compute next item */
	if (children == NULL) return;
	if sp_selection_is_empty(selection) {
		item = children->data;
	} else {
		l = g_slist_find(children,selection->items->data);
		if ((l == NULL) || (l->next == NULL)) {
			item = SP_ITEM (children->data);
		} else {
			item = SP_ITEM (l->next->data);
		}
	}

	/* set selection to item */
	if (item != NULL) {
		sp_selection_set_item (selection, item);
	} else {
		return;
	}

	g_slist_free (children);

	if (!sp_cycle_keep_focus ()) {
		NRRectF bbox;
		/* adjust visible area to see whole new selection */
		sp_item_bbox_desktop (item, &bbox);
		if (!nr_rect_f_test_empty (&bbox)) {
			sp_desktop_scroll_absolute_center_desktop (desktop, 0.5 * (bbox.x0 + bbox.x1), 0.5 * (bbox.y0 + bbox.y1));
		}
	}
}

void
sp_selection_item_prev (void)
{
	SPDocument * document;
	SPDesktop * desktop;
	SPSelection * selection;
	SPItem * item = NULL;
	GSList * children = NULL, * l = NULL;
	NRRectF dbox;

	document = SP_ACTIVE_DOCUMENT;
	desktop = SP_ACTIVE_DESKTOP;
	g_return_if_fail(document != NULL);
	g_return_if_fail(desktop != NULL);
	if (!SP_IS_DESKTOP (desktop)) return;
	selection = SP_DT_SELECTION(desktop);
	g_return_if_fail(selection!=NULL);
  
	/* Cycling type */
	if (sp_cycle_is_global ()) {
		children = sp_item_group_item_list (SP_GROUP(sp_document_root(document)));
	} else {
		NRRectD d;
		sp_desktop_get_display_area (desktop, &dbox);
		d.x0 = dbox.x0;
		d.y0 = dbox.y0;
		d.x1 = dbox.x1;
		d.y1 = dbox.y1;
		children = sp_document_partial_items_in_box (document, &d);
	}
  
	/* compute prev item */
	if (children == NULL) return;
	if sp_selection_is_empty(selection) {
		item = SP_ITEM (g_slist_last (children)->data);
	} else {
		l = children;
		if (l->next == NULL) item = SP_ITEM (l->data);
		while ((l->next != NULL) && (l->next->data != selection->items->data)) {
			l = l->next;
		}
		item = SP_ITEM (l->data);
	}

	/* set selection to item */
	if (item == NULL) return;

	sp_selection_set_item (selection, item);
  
	g_slist_free (children);

	if (!sp_cycle_keep_focus ()) {
		NRRectF bbox;
		/* adjust visible area to see whole new selection */
		sp_item_bbox_desktop (item, &bbox);
		if (!nr_rect_f_test_empty (&bbox)) {
			sp_desktop_scroll_absolute_center_desktop (desktop, 0.5 * (bbox.x0 + bbox.x1), 0.5 * (bbox.y0 + bbox.y1));
		}
	}
}

static void
sp_matrix_d_set_rotate (NRMatrixD *m, double theta)
{
	double s, c;
	s = sin (theta * M_PI / 180.0);
	c = cos (theta * M_PI / 180.0);
	m->c[0] = c;
	m->c[1] = s;
	m->c[2] = -s;
	m->c[3] = c;
	m->c[4] = 0.0;
	m->c[5] = 0.0;
}

