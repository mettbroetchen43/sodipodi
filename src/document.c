#define SP_DOCUMENT_C

#include "repr-item.h"
#include "document.h"

#define DEFAULT_PAGE_WIDTH (21.0 * 72.0 / 2.54)
#define DEFAULT_PAGE_HEIGHT (29.7 * 72.0 / 2.54)

void
sp_document_destroy (SPDocument * document)
{
	g_return_if_fail (document != NULL);
	g_return_if_fail (SP_IS_DOCUMENT (document));

	sp_repr_unref (SP_ITEM (document)->repr);
}

SPDocument *
sp_document_new (void)
{
	SPRepr * repr;
	SPItem * item;

	repr = sp_repr_new_with_name ("svg");
	g_return_val_if_fail (repr != NULL, NULL);
	sp_repr_set_double_attribute (repr, "width", DEFAULT_PAGE_WIDTH);
	sp_repr_set_double_attribute (repr, "height", DEFAULT_PAGE_HEIGHT);
#if 0
	/* fixme: */
	sp_repr_set_attr (repr, "transform", "matrix(1.0 0.0 0.0 -1.0 0.0 DEFAULT_PAGE_HEIGHT)");
#endif
	item = sp_repr_item_create (repr);
	g_return_val_if_fail (item != NULL, NULL);
	
	return SP_DOCUMENT (item);
}

SPDocument *
sp_document_new_from_file (const gchar * filename)
{
	SPRepr * repr;
	SPItem * item;
	gchar * base;
	/* fixme: */

	g_return_val_if_fail (filename != NULL, NULL);

	repr = sp_repr_read_file (filename);
	if (repr == NULL) return NULL;

	/* fixme: memory leak */
	base = g_strconcat (g_dirname (filename), "/", NULL);
	sp_repr_set_attr (repr, "SP-DOCNAME", filename);
	sp_repr_set_attr (repr, "SP-DOCBASE", base);

	item = sp_repr_item_create (repr);
#if 0
	sp_repr_unref (repr);
#endif
	g_print ("sp_document_new_from_file: partially implemented\n");
	return SP_DOCUMENT (item);
}

SPItem *
sp_document_add_repr (SPDocument * document, SPRepr * repr)
{
	sp_repr_append_child (SP_ITEM (document)->repr, repr);
	/* fixme: */
	return sp_repr_item_last_item ();
}

gdouble
sp_document_page_width (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, DEFAULT_PAGE_WIDTH);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), DEFAULT_PAGE_WIDTH);

	return SP_ROOT (document)->width;
}

gdouble
sp_document_page_height (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, DEFAULT_PAGE_HEIGHT);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), DEFAULT_PAGE_HEIGHT);

	return SP_ROOT (document)->height;
}

/*
 * Return list of items, contained in box
 *
 * Assumes box is normalized (and g_asserts it!)
 *
 */

GSList *
sp_document_items_in_box (SPDocument * document, ArtDRect * box)
{
	SPGroup * group;
	SPItem * child;
	ArtDRect b;
	GSList * s, * l;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);
	g_return_val_if_fail (box != NULL, NULL);

	group = SP_GROUP (document);

	s = NULL;

	for (l = group->children; l != NULL; l = l->next) {
		child = SP_ITEM (l->data);
		sp_item_bbox (child, &b);
		if ((b.x0 > box->x0) && (b.x1 < box->x1) &&
		    (b.y0 > box->y0) && (b.y1 < box->y1)) {
			s = g_slist_append (s, child);
		}
	}

	return s;
}



