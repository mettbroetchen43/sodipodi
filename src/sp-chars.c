#define SP_CHARS_C

#include "sp-chars.h"

static void sp_chars_class_init (SPCharsClass *class);
static void sp_chars_init (SPChars *chars);
static void sp_chars_destroy (GtkObject *object);

static void sp_chars_set_shape (SPChars * chars);

static SPShapeClass *parent_class;

GtkType
sp_chars_get_type (void)
{
	static GtkType chars_type = 0;

	if (!chars_type) {
		GtkTypeInfo chars_info = {
			"SPChars",
			sizeof (SPChars),
			sizeof (SPCharsClass),
			(GtkClassInitFunc) sp_chars_class_init,
			(GtkObjectInitFunc) sp_chars_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		chars_type = gtk_type_unique (sp_shape_get_type (), &chars_info);
	}
	return chars_type;
}

static void
sp_chars_class_init (SPCharsClass *class)
{
	GtkObjectClass *object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_shape_get_type ());

	object_class->destroy = sp_chars_destroy;
}

static void
sp_chars_init (SPChars *chars)
{
	chars->shape.path.independent = FALSE;
	chars->element = NULL;
}

static void
sp_chars_destroy (GtkObject *object)
{
	SPChars * chars;
	SPCharElement * el;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_CHARS (object));

	chars = SP_CHARS (object);

	while (chars->element) {
		el = (SPCharElement *) chars->element->data;
		sp_font_unref (el->font);
		g_free (el);
		chars->element = g_list_remove (chars->element, el);
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_chars_set_shape (SPChars * chars)
{
	SPShape * shape;
	GList * list;
	SPCharElement * el;
	SPCurve * curve;

	shape = (SPShape *) chars;

	sp_path_clear (SP_PATH (chars));

	for (list = chars->element; list != NULL; list = list->next) {
		el = (SPCharElement *) list->data;
		curve = sp_font_glyph_outline (el->font, el->glyph);
		if (curve != NULL)
			sp_path_add_bpath (SP_PATH (chars), curve, FALSE, el->affine);
	}
}

void
sp_chars_clear (SPChars * chars)
{
	SPCharElement * el;

	sp_path_clear (SP_PATH (chars));

	while (chars->element) {
		el = (SPCharElement *) chars->element->data;
		sp_font_unref (el->font);
		g_free (el);
		chars->element = g_list_remove (chars->element, el);
	}
}

void
sp_chars_add_element (SPChars * chars, guint glyph, SPFont * font, double affine[])
{
	SPCharElement * el;
	gint i;

	el = g_new (SPCharElement, 1);

	el->glyph = glyph;
	el->font = font;
	for (i = 0; i < 6; i++) el->affine[i] = affine[i];

	sp_font_ref (font);

	chars->element = g_list_prepend (chars->element, el);
	/* fixme: */
	sp_chars_set_shape (chars);
}
