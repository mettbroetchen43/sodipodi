#define SP_DOCUMENT_C

/*
 * SPDocument
 *
 * This is toplevel class, implementing a gateway from repr to most
 * editing properties, like canvases (desktops), undo stacks etc.
 *
 * Copyright (C) Lauris Kaplinski <lauris@ariman.ee> 1999-2000
 *
 */

#include <xml/repr.h>
#include "sp-object-repr.h"
#include "sp-root.h"
#include "document.h"

#define A4_WIDTH  (21.0 * 72.0 / 2.54)
#define A4_HEIGHT (29.7 * 72.0 / 2.54)

static void sp_document_class_init (SPDocumentClass * klass);
static void sp_document_init (SPDocument * item);
static void sp_document_destroy (GtkObject * object);

static GtkObjectClass * parent_class;

GtkType
sp_document_get_type (void)
{
	static GtkType document_type = 0;
	if (!document_type) {
		GtkTypeInfo document_info = {
			"SPDocument",
			sizeof (SPDocument),
			sizeof (SPDocumentClass),
			(GtkClassInitFunc) sp_document_class_init,
			(GtkObjectInitFunc) sp_document_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		document_type = gtk_type_unique (gtk_object_get_type (), &document_info);
	}
	return document_type;
}

static void
sp_document_class_init (SPDocumentClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	object_class->destroy = sp_document_destroy;
}

static void
sp_document_init (SPDocument * document)
{
	document->root = NULL;
	document->width = 0.0;
	document->height = 0.0;
	document->iddef = g_hash_table_new (g_str_hash, g_str_equal);
	document->uri = NULL;
	document->base = NULL;
}

static void
sp_document_destroy (GtkObject * object)
{
	SPDocument * document;

	document = (SPDocument *) object;

	if (document->base)
		g_free (document->base);

	if (document->uri)
		g_free (document->uri);

	if (document->iddef)
		g_hash_table_destroy (document->iddef);

	if (document->root)
		gtk_object_unref (GTK_OBJECT (document->root));

	if (((GtkObjectClass *) (parent_class))->destroy)
		(* ((GtkObjectClass *) (parent_class))->destroy) (object);
}

SPDocument *
sp_document_new (const gchar * uri)
{
	SPDocument * document;
	SPRepr * repr;
	SPObject * object;
	gchar * b;
	gint len;

	if (uri != NULL) {
		/* Try to fetch repr from file */
		repr = sp_repr_read_file (uri);
		/* If file cannot be loaded, return NULL without warning */
		if (repr == NULL) return NULL;
		/* If xml file is not svg, return NULL without warning */
		if (strcmp (sp_repr_name (repr), "svg") != 0) return NULL;
	} else {
		repr = sp_repr_new_with_name ("svg");
		g_return_val_if_fail (repr != NULL, NULL);
	}

	document = gtk_type_new (SP_TYPE_DOCUMENT);
	g_return_val_if_fail (document != NULL, NULL);

	object = sp_object_repr_build_tree (document, repr);
	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (SP_IS_ROOT (object), NULL);

	document->root = SP_ROOT (object);

	/* We assume, that SPRoot requires width and height, and sets these
	 * if not specified in SVG */

	document->width = document->root->width;
	document->height = document->root->height;

	if (uri != NULL) {
		b = g_strdup (uri);
		g_return_val_if_fail (b != NULL, NULL);
		document->uri = b;
		b = g_dirname (uri);
		g_return_val_if_fail (b != NULL, NULL);
		len = strlen (b) + 2;
		document->base = g_malloc (len);
		g_return_val_if_fail (document->base != NULL, NULL);
		g_snprintf (document->base, len, "%s/", b);
		g_free (b);
		sp_repr_set_attr (repr, "SP-DOCNAME", uri);
		sp_repr_set_attr (repr, "SP-DOCBASE", document->base);
	}

	return document;
}

SPRoot *
sp_document_root (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	return document->root;
}

gdouble
sp_document_width (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);

	return document->width;
}

gdouble
sp_document_height (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, 0.0);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), 0.0);

	return document->height;
}

const gchar *
sp_document_uri (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	return document->uri;
}

const gchar *
sp_document_base (SPDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	return document->base;
}

void
sp_document_def_id (SPDocument * document, const gchar * id, SPObject * object)
{
}

void
sp_document_undef_id (SPDocument * document, const gchar * id)
{
}

SPObject *
sp_document_lookup_id (SPDocument * document, const gchar * id)
{
	return NULL;
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

	group = SP_GROUP (document->root);

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

SPItem *
sp_document_add_repr (SPDocument * document, SPRepr * repr)
{
	sp_repr_append_child (SP_OBJECT (document->root)->repr, repr);
#if 0
	/* fixme: */
	return sp_repr_item_last_item ();
#endif
	return NULL;
}


#if 0
/* Old SPDocument */
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

#endif


