#define SP_TEXT_C

#include "svg/svg.h"
#include "sp-text.h"

static void sp_text_class_init (SPTextClass *class);
static void sp_text_init (SPText *text);
static void sp_text_destroy (GtkObject *object);

static void sp_text_read (SPItem * item, SPRepr * repr);
static void sp_text_read_attr (SPItem * item, SPRepr * repr, const gchar * attr);

static void sp_text_set_shape (SPText * text);

static SPCharsClass *parent_class;

GtkType
sp_text_get_type (void)
{
	static GtkType text_type = 0;

	if (!text_type) {
		GtkTypeInfo text_info = {
			"SPText",
			sizeof (SPText),
			sizeof (SPTextClass),
			(GtkClassInitFunc) sp_text_class_init,
			(GtkObjectInitFunc) sp_text_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};
		text_type = gtk_type_unique (sp_chars_get_type (), &text_info);
	}
	return text_type;
}

static void
sp_text_class_init (SPTextClass *class)
{
	GtkObjectClass *object_class;
	SPItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (SPItemClass *) class;

	parent_class = gtk_type_class (sp_chars_get_type ());

	object_class->destroy = sp_text_destroy;

	item_class->read = sp_text_read;
	item_class->read_attr = sp_text_read_attr;
}

static void
sp_text_init (SPText *text)
{
	text->x = text->y = 0.0;
	text->text = NULL;
	text->fontname = g_strdup ("Helvetica");
	text->weight = GNOME_FONT_BOOK;
	text->italic = FALSE;
	text->font = NULL;
	text->size = 12.0;
}

static void
sp_text_destroy (GtkObject *object)
{
	SPText *text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_TEXT (object));

	text = SP_TEXT (object);

	if (text->text)
		g_free (text->text);
	if (text->fontname)
		g_free (text->fontname);
	if (text->font)
		sp_font_unref (text->font);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
sp_text_set_shape (SPText * text)
{
	SPChars * chars;
	SPFont * font;
	guchar * c;
	guint glyph;
	gdouble x, y;
	double a[6], trans[6], scale[6];
	double w;

	chars = SP_CHARS (text);

	sp_chars_clear (chars);

	font = sp_font_get (text->fontname, text->weight, text->italic);
	if (text->font)
		sp_font_unref (text->font);
	text->font = font;

	x = text->x;
	y = text->y;

	art_affine_scale (scale, text->size * 0.001, text->size * -0.001);

	if (text->text) {
		for (c = text->text; *c; c++) {
		glyph = * c;
		switch (glyph) {
		case '\n':
			x = text->x;
			y += text->size;
			break;
		default:
			w = sp_font_glyph_width (font, glyph);
			w = w * text->size / 1000.0;
			art_affine_translate (trans, x, y);
			art_affine_multiply (a, scale, trans);
			sp_chars_add_element (chars, glyph, text->font, a);
			x += w;
		}
		}
	}
}

static void
sp_text_read (SPItem * item, SPRepr * repr)
{
	if (SP_ITEM_CLASS (parent_class)->read)
		(SP_ITEM_CLASS (parent_class)->read) (item, repr);

	sp_text_read_attr (item, repr, "SP-INTERNAL-CONTENT-ATTR");
	sp_text_read_attr (item, repr, "x");
	sp_text_read_attr (item, repr, "y");
	sp_text_read_attr (item, repr, "style");
}

static void
sp_text_read_attr (SPItem * item, SPRepr * repr, const gchar * attr)
{
	SPText * text;
	const gchar * string;
	SPCSSAttr * css;
	const gchar * fontname;
	gdouble size;
	GnomeFontWeight weight;
	gboolean italic;
	const gchar * str;
	SPSVGUnit unit;

	text = SP_TEXT (item);

	if (strcmp (attr, "SP-INTERNAL-CONTENT-ATTR") == 0) {
		if (text->text) g_free (text->text);
		string = sp_repr_content (repr);
		text->text = g_strdup (string);
		sp_text_set_shape (text);
		return;
	}
	if (strcmp (attr, "x") == 0) {
		text->x = sp_repr_get_double_attribute (repr, attr, text->y);
		sp_text_set_shape (text);
		return;
	}
	if (strcmp (attr, "y") == 0) {
		text->y = sp_repr_get_double_attribute (repr, attr, text->y);
		sp_text_set_shape (text);
		return;
	}
	if (strcmp (attr, "style") == 0) {
		css = sp_repr_css_attr_inherited (repr, attr);
		fontname = sp_repr_css_property (css, "font-family", "Helvetica");
		str = sp_repr_css_property (css, "font-size", "12pt");
		size = sp_svg_read_length (&unit, str);
		str = sp_repr_css_property (css, "font-weight", "normal");
		weight = sp_svg_read_font_weight (str);
		str = sp_repr_css_property (css, "font-style", "normal");
		italic = sp_svg_read_font_italic (str);
		if (text->fontname)
			g_free (text->fontname);
		text->fontname = g_strdup (fontname);
		if (text->font)
			sp_font_unref (text->font);
		text->font = NULL;
		text->size = size;
		text->weight = weight;
		text->italic = italic;
		sp_repr_css_attr_unref (css);
		sp_text_set_shape (text);

		if (SP_ITEM_CLASS (parent_class)->read_attr)
			(SP_ITEM_CLASS (parent_class)->read_attr) (item, repr, attr);

		return;
	}

	if (SP_ITEM_CLASS (parent_class)->read_attr)
		(SP_ITEM_CLASS (parent_class)->read_attr) (item, repr, attr);
}

