#define SP_PATH_ARCHETYPE_C

#include <config.h>
#include <math.h>
#include <glib.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_rect_svp.h>
#ifdef NEW_RENDER
#include "nr-svp.h"
#include "nr-svp-uncross.h"
#endif
#include "path-archetype.h"

#define noDEBUG_PATH_AT

#ifdef DEBUG_PATH_AT
	gint num_at = 0;
#endif

static guint sp_pat_hash (gconstpointer key);
static gint sp_pat_equal (gconstpointer a, gconstpointer b);

static void sp_pat_free_state (SPPathAT * at);

static SPPathAT * sp_path_at_new (SPCurve * curve,
				  gboolean private,
				  double affine[],
				  gint rule,
				  double stroke_width,
				  ArtPathStrokeJoinType join,
				  ArtPathStrokeCapType cap);

GHashTable * archetypes = NULL;

SPPathAT *
sp_path_at (SPCurve * curve,
	    gboolean private,
	    double affine[],
	    gint rule,
	    double stroke_width,
	    ArtPathStrokeJoinType join,
	    ArtPathStrokeCapType cap)
{
	SPPathAT cat, * at;
	gint i;

	g_return_val_if_fail (curve != NULL, NULL);

	if (archetypes == NULL)
		archetypes = g_hash_table_new (sp_pat_hash, sp_pat_equal);

	if (!private) {
		cat.curve = curve;
		for (i = 0; i < 4; i++) cat.affine[i] = affine[i];
		cat.rule = rule;
		cat.stroke_width = stroke_width;
		cat.join = join;
		cat.cap = cap;

		at = g_hash_table_lookup (archetypes, &cat);

		if (at != NULL) {
			at->refcount++;
			return at;
		}
	}

	at = sp_path_at_new (curve, private, affine, rule, stroke_width, join, cap);

	if (!private) {
		g_hash_table_insert (archetypes, at, at);
	}

	return at;
}

void
sp_path_at_ref (SPPathAT * at)
{
	g_assert (at != NULL);
	at->refcount++;
}

void
sp_path_at_unref (SPPathAT * at)
{
	g_assert (at != NULL);
	at->refcount--;

	if (at->refcount < 1) {
		if (!at->private)
			g_hash_table_remove (archetypes, at);
		sp_curve_unref (at->curve);
		sp_pat_free_state (at);
		g_free (at);
#ifdef DEBUG_PATH_AT
		num_at--;
		g_print ("num_at = %d\n", num_at);
#endif
	}
}

static SPPathAT *
sp_path_at_new (SPCurve * curve,
		gboolean private,
		double affine[],
		gint rule,
		double stroke_width,
		ArtPathStrokeJoinType join,
		ArtPathStrokeCapType cap)
{
	SPPathAT * at;
	gint i;
	ArtBpath * affine_bpath;
	ArtVpath * vpath;
	ArtSVP * svpa;
	gdouble fa[6];
#ifdef NEW_RENDER
	NRSVP * nrsvp0, * nrsvp1;
	ArtSVP * svpb;
#else
	ArtSVP * svpb;
#endif
	g_return_val_if_fail (curve != NULL, NULL);

	at = g_new (SPPathAT, 1);
	at->refcount = 1;
	at->curve = curve;
	sp_curve_ref (curve);
	at->private = private;
	for (i = 0; i < 4; i++) {
		at->affine[i] = affine[i];
		fa[i] = affine[i];
	}
	fa[4] = fa[5] = 0.0;
	at->rule = rule;
	at->stroke_width = stroke_width;
	at->join = join;
	at->cap = cap;

	affine_bpath = art_bpath_affine_transform (at->curve->bpath, fa);
	vpath = art_bez_path_to_vec (affine_bpath, 0.25);
	art_free (affine_bpath);

#ifdef NEW_RENDER
	at->vpath = vpath;
	art_vpath_bbox_drect (at->vpath, &at->bbox);
	nrsvp0 = nr_svp_from_art_vpath (at->vpath);
#if 0
	nrsvp1 = nr_svp_rewind (nrsvp0);
	nr_svp_free (nrsvp0);
	svpa = nr_art_svp_from_svp (nrsvp1);
#else
	svpa = nr_art_svp_from_svp (nrsvp0);
	svpb = art_svp_uncross (svpa);
	art_svp_free (svpa);
	svpa = art_svp_rewind_uncrossed (svpb, at->rule);
	art_svp_free (svpb);
#endif
	at->nrsvp = nrsvp0;
#else
	at->vpath = art_vpath_perturb (vpath);
	art_free (vpath);
	art_vpath_bbox_drect (at->vpath, &at->bbox);
	svpa = art_svp_from_vpath (at->vpath);
	svpb = art_svp_uncross (svpa);
	art_svp_free (svpa);
	svpa = art_svp_rewind_uncrossed (svpb, at->rule);
	art_svp_free (svpb);
#endif

	at->svp = svpa;

	if (at->stroke_width > 0.0) {
		at->stroke = art_svp_vpath_stroke (at->vpath, at->join, at->cap, at->stroke_width, 4, 0.25);
		/* it seems silly - isn't stroke always bigger than shape? */
		art_drect_svp_union (&at->bbox, at->stroke);
	} else {
		at->stroke = NULL;
	}

#ifdef DEBUG_PATH_AT
	num_at++;
	g_print ("num_at = %d\n", num_at);
#endif
	return at;
}

static void
sp_pat_free_state (SPPathAT * at)
{
	g_assert (at != NULL);
#ifdef NEW_RENDER
	if (at->nrsvp) {
		nr_svp_free_list (at->nrsvp);
		at->nrsvp = NULL;
	}
#endif
	if (at->svp != NULL) {
		art_svp_free (at->svp);
		at->svp = NULL;
	}
	if (at->stroke != NULL) {
		art_svp_free (at->stroke);
		at->stroke = NULL;
	}
	if (at->vpath != NULL) {
		art_free (at->vpath);
		at->vpath = NULL;
	}
};

static guint
sp_pat_hash (gconstpointer key)
{
	SPPathAT * at;
	at = (SPPathAT *) key;
	/* fixme: */
	return GPOINTER_TO_INT (at->curve);
}

static gint
sp_pat_equal (gconstpointer a, gconstpointer b)
{
	SPPathAT * ata, * atb;
	gint i;
	gdouble d;

	ata = (SPPathAT *) a;
	atb = (SPPathAT *) b;
	/* fixme: */
	if (ata->curve != atb->curve)
		return FALSE;
	for (i = 0; i < 4; i++) {
		d = fabs (ata->affine[i] - atb->affine[i]);
		if (d > 1e-6) return FALSE;
	}
	if (ata->rule != atb->rule) return FALSE;
	if (ata->stroke_width != atb->stroke_width)
		return FALSE;
	if (ata->join != atb->join)
		return FALSE;
	if (ata->cap != atb->cap)
		return FALSE;
	return TRUE;
}

