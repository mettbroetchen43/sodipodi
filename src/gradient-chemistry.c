#define __SP_GRADIENT_CHEMISTRY_C__

/*
 * Various utility methods for gradients
 *
 * Author:
 *   Lauris Kaplinski <lauris@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL
 */

#include <string.h>
#include "xml/repr-private.h"
#include "document-private.h"
#include "sp-root.h"
#include "gradient-chemistry.h"

/*
 * fixme: This sucks a lot - basically we keep moving gradients around
 * all the time, causing LOT of unnecessary parsing
 *
 * This should be fixed by SPHeader fine grouping, but I am
 * currently too tired to write it
 *
 * Lauris
 */

static void sp_gradient_repr_set_link (SPRepr *repr, SPGradient *gr);
static void sp_item_repr_set_fill_gradient (SPRepr *repr, SPGradient *gr);
static guchar *sp_style_change_property (const guchar *sstr, const guchar *key, const guchar *value);

/* fixme: One more step is needed - normalization vector to 0-1 (not sure 100% still) */

SPGradient *
sp_gradient_ensure_vector_normalized (SPGradient *gr)
{
	SPDocument *doc;
	SPDefs *defs;

	g_return_val_if_fail (gr != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT (gr), NULL);

	/* If we are already normalized vector, just return */
	if (gr->state == SP_GRADIENT_STATE_VECTOR) return gr;
	/* Fail, if we have wrong state set */
	if (gr->state != SP_GRADIENT_STATE_UNKNOWN) {
		g_warning ("file %s: line %d: Cannot normalize private gradient to vector (%s)", __FILE__, __LINE__, SP_OBJECT_ID (gr));
		return NULL;
	}

	g_print ("GVECTORNORM: Requested vector normalization of gradient %s\n", SP_OBJECT_ID (gr));

	doc = SP_OBJECT_DOCUMENT (gr);
	defs = SP_DOCUMENT_DEFS (doc);

	if (SP_OBJECT_PARENT (gr) != SP_OBJECT (defs)) {
		SPGradient *new;
		SPRepr *repr;
		/* Lonely gradient */
		/* Ensure vector, so we can know some our metadata */
		sp_gradient_ensure_vector (gr);
		g_assert (gr->vector);
		/* NOTICE */
		/* We are in some lonely place in tree, so clone EVERYTHING */
		/* And do not forget to flatten original */
		g_print ("GVECTORNORM: Gradient %s IS NOT in <defs>\n", SP_OBJECT_ID (gr));
		/* Step 1 - flatten original EXCEPT vector */
		sp_gradient_repr_flatten_attributes (gr, SP_OBJECT_REPR (gr), TRUE);
		g_print ("GVECTORNORM: Gradient %s attributes flattened\n", SP_OBJECT_ID (gr));
		/* Step 2 - create new empty gradient and prepend it to <defs> */
		repr = sp_repr_new ("linearGradient");
		sp_repr_add_child (SP_OBJECT_REPR (defs), repr, NULL);
		new = (SPGradient *) sp_document_lookup_id (doc, sp_repr_attr (repr, "id"));
		g_assert (gr != NULL);
		g_assert (SP_IS_GRADIENT (gr));
		g_print ("GVECTORNORM: Created new vector gradient %s\n", SP_OBJECT_ID (new));
		/* Step 3 - set vector of new gradient */
		sp_gradient_repr_set_vector (new, SP_OBJECT_REPR (new), gr->vector);
		g_print ("GVECTORNORM: Added stops to %s\n", SP_OBJECT_ID (new));
		/* Step 4 - set state flag */
		new->state = SP_GRADIENT_STATE_VECTOR;
		g_print ("GVECTORNORM: Set of %s to vector normalized\n", SP_OBJECT_ID (new));
		/* Step 5 - set href of old vector */
		sp_gradient_repr_set_link (SP_OBJECT_REPR (gr), new);
		g_print ("GVECTORNORM: Set href of %s to %s\n", SP_OBJECT_ID (gr), SP_OBJECT_ID (new));
		/* Step 6 - clear stops of old gradient */
		sp_gradient_repr_set_vector (gr, SP_OBJECT_REPR (gr), NULL);
		g_print ("GVECTORNORM: Cleared all stops of %s\n", SP_OBJECT_ID (gr));
		/* Now we have successfully created new normalized vector, and cleared old stops */
		return new;
	} else {
		SPObject *child;
		/* Gradient is in <defs> */
		g_print ("GVECTORNORM: Gradient %s IS in <defs>\n", SP_OBJECT_ID (gr));
		/* First make sure we have vector directly defined */
		if (!gr->has_stops) {
			/* We do not have stops ourselves, so flatten stops as well */
			sp_gradient_ensure_vector (gr);
			g_assert (gr->vector);
			sp_gradient_repr_set_vector (gr, SP_OBJECT_REPR (gr), gr->vector);
			g_print ("GVECTORNORM: Added stops to %s\n", SP_OBJECT_ID (gr));
		}
		/* Nof break free hrefing */
		if (gr->href) {
			/* We are hrefing someone, so require flattening */
			sp_gradient_repr_flatten_attributes (gr, SP_OBJECT_REPR (gr), TRUE);
			g_print ("GVECTORNORM: Gradient %s attributes flattened\n", SP_OBJECT_ID (gr));
			sp_gradient_repr_set_link (SP_OBJECT_REPR (gr), NULL);
		}
		/* Now we can be sure we are in good condition */
		/* Still have to ensure we are in vector position */
		for (child = defs->children; child != NULL; child = child->next) {
			if (SP_IS_GRADIENT (child)) {
				SPGradient *gchild;
				gchild = SP_GRADIENT (child);
				if (gchild == gr) {
					/* Everything is OK, set state flag */
					/* fixme: I am not sure, whether one should clone, if hrefcount > 1 */
					gr->state = SP_GRADIENT_STATE_VECTOR;
					g_print ("GVECTORNORM: Found gradient %s in right position\n", SP_OBJECT_ID (gr));
					return gr;
				}
				/* If there is private gradient, we have to rearrange ourselves */
				if (gchild->state == SP_GRADIENT_STATE_PRIVATE) break;
			}
		}
		/* If we didn't find ourselves, <defs> is probably messed up */
		g_assert (child != NULL);
		/* Now we have to move ourselves to the beggining of <defs> */
		g_print ("GVECTORNORM: Moving %s to the beginning of <defs>\n", SP_OBJECT_ID (gr));
		if (!sp_repr_change_order (SP_OBJECT_REPR (defs), SP_OBJECT_REPR (gr), NULL)) {
			g_warning ("file %s: line %d: Cannot move gradient %s to vector position", __FILE__, __LINE__, SP_OBJECT_ID (gr));
			return NULL;
		}
		/* Everything is OK, set state flag */
		gr->state = SP_GRADIENT_STATE_VECTOR;
		g_print ("GVECTORNORM: Moved gradient %s to the right position\n", SP_OBJECT_ID (gr));
		return gr;
	}
}

/*
 * Either normalizes given gradient to private, or returns fresh normalized
 * private - gradient is flattened in any case, and vector set
 * Vector has to be normalized beforehand
 */

SPGradient *
sp_gradient_ensure_private_normalized (SPGradient *gr, SPGradient *vector)
{
	SPDocument *doc;
	SPDefs *defs;

	g_return_val_if_fail (gr != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT (gr), NULL);
	g_return_val_if_fail (vector != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT (vector), NULL);
	g_return_val_if_fail (vector->state == SP_GRADIENT_STATE_VECTOR, NULL);

	/* If we are already normalized private, change href and return */
	if (gr->state == SP_GRADIENT_STATE_PRIVATE) {
		/* This assertion is dangerous - we have to be very-very strict about it */
		g_assert (SP_OBJECT_HREFCOUNT (gr) == 1);
		if (gr->href != vector) {
			/* href is not vector */
			sp_gradient_repr_set_link (SP_OBJECT_REPR (gr), vector);
		}
		return gr;
	}
#if 0
	/* fixme: Think - this is not correct, as due to svg madness, gr may point to vector gradient */
	/* Fail, if we have wrong state set */
	g_return_val_if_fail (gr->state == SP_GRADIENT_STATE_UNKNOWN, NULL);
#endif

	g_print ("Private normalization of gradient %s requested\n", SP_OBJECT_ID (gr));

	doc = SP_OBJECT_DOCUMENT (gr);
	defs = SP_DOCUMENT_DEFS (doc);

	/* Determine, whether we have to clone fresh new gradient */
	if ((gr->has_stops) ||
	    (gr->state != SP_GRADIENT_STATE_UNKNOWN) ||
	    (SP_OBJECT_PARENT (gr) != SP_OBJECT (defs)) ||
	    (SP_OBJECT_HREFCOUNT (gr) > 1)) {
		SPRepr *repr;
		/* We are either in some lonely place, or have multiple refs, or vector or whatever, so clone */
		/* fixme: no 'linear' please */
		repr = sp_lineargradient_build_repr (SP_LINEARGRADIENT (gr), FALSE);
		sp_gradient_repr_set_link (repr, vector);
		/* Append cloned private gradient to defs */
		sp_repr_append_child (SP_OBJECT_REPR (defs), repr);
		sp_repr_unref (repr);
		/* fixme: This does not look like nice */
		gr = (SPGradient *) sp_document_lookup_id (doc, sp_repr_attr (repr, "id"));
		g_assert (gr != NULL);
		g_assert (SP_IS_GRADIENT (gr));
		/* fixme: Maybe add extra sanity check here */
		gr->state = SP_GRADIENT_STATE_PRIVATE;
		return gr;
	} else {
		SPObject *sibling;
		/* We still have to determine, whether we have to change our position */
		for (sibling = SP_OBJECT_NEXT (gr); sibling != NULL; sibling = SP_OBJECT_NEXT (sibling)) {
			if (SP_IS_GRADIENT (sibling) && SP_GRADIENT (sibling)->state == SP_GRADIENT_STATE_VECTOR) {
				/* Found vector after us, so move */
				while (SP_OBJECT_NEXT (sibling)) sibling = SP_OBJECT_NEXT (sibling);
				if (!sp_repr_change_order (SP_OBJECT_REPR (defs), SP_OBJECT_REPR (gr), SP_OBJECT_REPR (sibling))) {
					g_warning ("Cannot move gradient %s to private position", SP_OBJECT_ID (gr));
					return NULL;
				}
				break;
			}
		}
		/* Set state */
		gr->state = SP_GRADIENT_STATE_PRIVATE;
		return gr;
	}
}

/*
 * Finds and normalizes first free gradient
 */

static SPGradient *
sp_gradient_get_private_normalized (SPDocument *document, SPGradient *vector)
{
	SPDefs *defs;
	SPObject *child;
	SPRepr *repr;
	SPGradient *gr;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (vector != NULL, NULL);
	g_return_val_if_fail (SP_IS_GRADIENT (vector), NULL);
	g_return_val_if_fail (vector->state == SP_GRADIENT_STATE_VECTOR, NULL);

	defs = SP_DOCUMENT_DEFS (document);

	for (child = defs->children; child != NULL; child = child->next) {
		if (SP_IS_GRADIENT (child)) {
			SPGradient *gr;
			gr = SP_GRADIENT (child);
			if (SP_OBJECT_HREFCOUNT (gr) == 0) {
				if (gr->state == SP_GRADIENT_STATE_PRIVATE) return gr;
				if (gr->state == SP_GRADIENT_STATE_UNKNOWN) {
					/* fixme: This is plain wrong - what if out gradient is not at private position? */
					sp_gradient_ensure_vector (gr);
					if (!gr->has_stops) return sp_gradient_ensure_private_normalized (gr, vector);
				}
			}
		}
	}

	/* Have to create our own */
	repr = sp_repr_new ("linearGradient");
	sp_gradient_repr_set_link (repr, vector);
	/* Append cloned private gradient to defs */
	sp_repr_append_child (SP_OBJECT_REPR (defs), repr);
	sp_repr_unref (repr);
	/* fixme: This does not look like nice */
	gr = (SPGradient *) sp_document_lookup_id (document, sp_repr_attr (repr, "id"));
	g_assert (gr != NULL);
	g_assert (SP_IS_GRADIENT (gr));
	/* fixme: Maybe add extra sanity check here */
	gr->state = SP_GRADIENT_STATE_PRIVATE;

	return gr;
}

/*
 * Sets item fill to lineargradient with given vector, creating
 * new private gradient, if needed
 * gr has to be normalized vector
 */

void
sp_item_force_fill_lineargradient_vector (SPItem *item, SPGradient *gr)
{
	SPStyle *style;

	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));
	g_return_if_fail (gr->state == SP_GRADIENT_STATE_VECTOR);

#if 0
	g_print ("Changing item %s gradient vector to %s requested\n", SP_OBJECT_ID (item), SP_OBJECT_ID (gr));
#endif

	style = SP_OBJECT_STYLE (item);
	g_assert (style != NULL);

	if ((style->fill.type != SP_PAINT_TYPE_PAINTSERVER) || !SP_IS_LINEARGRADIENT (style->fill.server)) {
		SPGradient *pg;
		/* Current fill style is not lineargradient, so construct everything */
		pg = sp_gradient_get_private_normalized (SP_OBJECT_DOCUMENT (item), gr);
		sp_item_repr_set_fill_gradient (SP_OBJECT_REPR (item), pg);
	} else {
		SPGradient *ig;
		/* Current fill style is lineargradient */
		ig = SP_GRADIENT (style->fill.server);
		if (ig->state != SP_GRADIENT_STATE_PRIVATE) {
			SPGradient *pg;
			/* Check, whether we have to normalize private gradient */
			pg = sp_gradient_ensure_private_normalized (ig, gr);
			g_return_if_fail (pg != NULL);
			g_return_if_fail (SP_IS_LINEARGRADIENT (pg));
			if (pg != ig) {
				/* We have to change object style here */
				g_print ("Changing object %s fill to gradient %s requested\n", SP_OBJECT_ID (item), SP_OBJECT_ID (pg));
				sp_item_repr_set_fill_gradient (SP_OBJECT_REPR (item), pg);
			}
			return;
		}
		/* ig is private gradient, so change href to vector */
		g_assert (ig->state == SP_GRADIENT_STATE_PRIVATE);
		if (ig->href != gr) {
			/* href is not vector */
			sp_gradient_repr_set_link (SP_OBJECT_REPR (ig), gr);
		}
	}
}

static void
sp_gradient_repr_set_link (SPRepr *repr, SPGradient *link)
{
	const guchar *id;
	gchar *ref;
	gint len;

	g_return_if_fail (repr != NULL);
	g_return_if_fail (link != NULL);
	g_return_if_fail (SP_IS_GRADIENT (link));

	if (link) {
		id = SP_OBJECT_ID (link);
		len = strlen (id);
		ref = alloca (len + 2);
		*ref = '#';
		memcpy (ref + 1, id, len + 1);
	} else {
		ref = NULL;
	}

	sp_repr_set_attr (repr, "xlink:href", ref);
}

static void
sp_item_repr_set_fill_gradient (SPRepr *repr, SPGradient *gr)
{
	const guchar *sstr;
	guchar *val, *newval;

	g_return_if_fail (repr != NULL);
	g_return_if_fail (gr != NULL);
	g_return_if_fail (SP_IS_GRADIENT (gr));

	val = g_strdup_printf ("url(#%s)", SP_OBJECT_ID (gr));
	sstr = sp_repr_attr (repr, "style");
	newval = sp_style_change_property (sstr, "fill", val);
	g_free (val);
	sp_repr_set_attr (repr, "style", newval);
	g_free (newval);
}

/*
 * Get default normalized gradient vector of document, create if there is none
 */

SPGradient *
sp_document_default_gradient_vector (SPDocument *document)
{
	SPDefs *defs;
	SPObject *child;
	SPRepr *repr, *stop;
	SPGradient *gr;

	defs = SP_DOCUMENT_DEFS (document);

	for (child = defs->children; child != NULL; child = child->next) {
		if (SP_IS_GRADIENT (child)) {
			SPGradient *gr;
			gr = SP_GRADIENT (child);
			if (gr->state == SP_GRADIENT_STATE_VECTOR) return gr;
			if (gr->state == SP_GRADIENT_STATE_PRIVATE) break;
			sp_gradient_ensure_vector (gr);
			if (gr->has_stops) {
				/* We have everything, but push it through normalization testing to be sure */
				return sp_gradient_ensure_vector_normalized (gr);
			}
		}
	}

	/* There were no suitable vector gradients - create one */
	repr = sp_repr_new ("linearGradient");
	stop = sp_repr_new ("stop");
	sp_repr_set_attr (stop, "style", "stop-color:#000;stop-opacity:1;");
	sp_repr_set_attr (stop, "offset", "0");
	sp_repr_append_child (repr, stop);
	sp_repr_unref (stop);
	stop = sp_repr_new ("stop");
	sp_repr_set_attr (stop, "style", "stop-color:#fff;stop-opacity:1;");
	sp_repr_set_attr (stop, "offset", "1");
	sp_repr_append_child (repr, stop);
	sp_repr_unref (stop);

	sp_repr_add_child (SP_OBJECT_REPR (defs), repr, NULL);
	sp_repr_unref (repr);

	/* fixme: This does not look like nice */
	gr = (SPGradient *) sp_document_lookup_id (document, sp_repr_attr (repr, "id"));
	g_assert (gr != NULL);
	g_assert (SP_IS_GRADIENT (gr));
	/* fixme: Maybe add extra sanity check here */
	gr->state = SP_GRADIENT_STATE_VECTOR;

	return gr;
}

/*
 * We take dumb approach currently
 *
 * Just ensure gradient href == 1, and flatten it
 *
 */

void
sp_object_ensure_fill_gradient_normalized (SPObject *object)
{
	SPLinearGradient *lg;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_OBJECT (object));
	/* Need style */
	g_return_if_fail (object->style != NULL);
	/* Fill has to be set */
	g_return_if_fail (object->style->fill_set);
	/* Fill has to be paintserver */
	g_return_if_fail (object->style->fill.type = SP_PAINT_TYPE_PAINTSERVER);
	/* Has to be linear gradient */
	g_return_if_fail (SP_IS_LINEARGRADIENT (object->style->fill.server));

	lg = SP_LINEARGRADIENT (object->style->fill.server);

	if (SP_OBJECT_HREFCOUNT (lg) > 1) {
		const guchar *sstr;
		SPRepr *repr;
		guchar *val, *newval;
		/* We have to clone gradient */
		sp_gradient_ensure_vector (SP_GRADIENT (lg));
		repr = sp_lineargradient_build_repr (lg, TRUE);
		lg = (SPLinearGradient *) sp_document_add_repr (SP_OBJECT_DOCUMENT (object), repr);
		sp_repr_unref (repr);
		val = g_strdup_printf ("url(#%s)", SP_OBJECT_ID (lg));
		sstr = sp_object_getAttribute (object, "style", NULL);
		newval = sp_style_change_property (sstr, "fill", val);
		g_free (newval);
		sp_object_setAttribute (object, "style", newval, NULL);
		g_free (newval);
	}
}

void
sp_object_ensure_stroke_gradient_normalized (SPObject *object)
{
	g_warning ("file %s: line %d: Normalization of stroke gradient not implemented", __FILE__, __LINE__);
}

/* Fixme: This belongs to SVG/style eventually */

static guchar *
sp_style_change_property (const guchar *sstr, const guchar *key, const guchar *value)
{
	guchar *str, *buf;
	gint len, klen, vlen;

	if (!sstr) {
		if (!value) return NULL;
		return g_strdup_printf ("%s:%s;", key, value);
	}

	len = strlen (sstr);
	klen = strlen (key);
	vlen = (value) ? strlen (value) : 0;

	str = buf = alloca (len + vlen + 2);

	while (sstr && *sstr) {
		const guchar *a;
		/* Rewing empty space */
		while (*sstr && *sstr <= ' ') sstr++;
		a = strchr (sstr, ':');
		if (a) {
			const guchar *b;
			b = strchr (a + 1, ';');
			if (!strncmp (sstr, key, klen)) {
				/* Found it */
				sstr = (b) ? b + 1 : NULL;
			} else {
				if (b) {
					memcpy (str, sstr, b + 1 - sstr);
					str += (b - sstr) + 1;
					sstr = b + 1;
				} else {
					gint l;
					l = strlen (sstr);
					memcpy (str, sstr, l);
					sstr += l;
					str += l;
					*str++ = ';';
				}
			}
		}
	}

	if (value) {
		memcpy (str, key, klen);
		str += klen;
		*str++ = ':';
		memcpy (str, value, vlen);
		str += vlen;
		*str++ = ';';
	}

	*str = '\0';

	return (*buf) ? g_strdup (buf) : NULL;
}
