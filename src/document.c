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
	document->aspect = SPXMidYMid;
	document->clip = FALSE;

	document->sensitive = TRUE;
	document->undo = NULL;
	document->redo = NULL;
	document->actions = NULL;
}

static void
sp_document_destroy (GtkObject * object)
{
	SPDocument * document;

	document = (SPDocument *) object;

	while (document->actions) {
		sp_repr_unref ((SPRepr *) document->actions->data);
		document->actions = g_list_remove (document->actions, document->actions->data);
	}

	sp_document_clear_redo (document);
	sp_document_clear_undo (document);

	if (document->base)
		g_free (document->base);

	if (document->uri)
		g_free (document->uri);

	if (document->root)
		gtk_object_unref (GTK_OBJECT (document->root));

	if (document->iddef)
		g_hash_table_destroy (document->iddef);

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
		repr = sp_repr_new ("svg");
		g_return_val_if_fail (repr != NULL, NULL);
		sp_repr_set_attr (repr, "style",
			"fill:#000000;fill-opacity:10%;stroke:none");
	}

	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (repr, "xmlns", "http://www.w3.org/Graphics/SVG/SVG-19991203.dtd");
	sp_repr_set_attr (repr, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	/* End of quick hack */

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
		sp_repr_set_attr (repr, "sodipodi:docname", uri);
		sp_repr_set_attr (repr, "sodipodi:docbase", document->base);
	}

	return document;
}

SPDocument *
sp_document_new_from_mem (const gchar * buffer, gint length)
{
	SPDocument * document;
	SPRepr * repr;
	SPObject * object;

	repr = sp_repr_read_mem (buffer, length);

	/* If it cannot be loaded, return NULL without warning */
	if (repr == NULL) return NULL;

	/* If xml file is not svg, return NULL without warning */
	if (strcmp (sp_repr_name (repr), "svg") != 0) return NULL;

	/* A quick hack to get namespaces into doc */
	sp_repr_set_attr (repr, "xmlns", "http://www.w3.org/Graphics/SVG/SVG-19991203.dtd");
	sp_repr_set_attr (repr, "xmlns:sodipodi", "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd");
	/* End of quick hack */

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

	/* fixme: docbase */

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
	g_assert (g_hash_table_lookup (document->iddef, id) == NULL);

	g_hash_table_insert (document->iddef, (gchar *) id, object);
}

void
sp_document_undef_id (SPDocument * document, const gchar * id)
{
	g_assert (g_hash_table_lookup (document->iddef, id) != NULL);

	g_hash_table_remove (document->iddef, id);
}

SPObject *
sp_document_lookup_id (SPDocument * document, const gchar * id)
{
	return g_hash_table_lookup (document->iddef, id);
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

