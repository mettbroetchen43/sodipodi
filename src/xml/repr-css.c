#define SP_REPR_CSS_C

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "repr.h"
#include "repr-private.h"

struct _SPCSSAttr {
	SPRepr repr;
};

static void sp_repr_css_add_components (SPCSSAttr * css, SPRepr * repr, const gchar * attr);

SPCSSAttr *
sp_repr_css_attr_new (void)
{
	SPRepr * css;
	css = sp_repr_new_with_name ("css");
	g_return_val_if_fail (css != NULL, NULL);
	return (SPCSSAttr *) css;
}

void
sp_repr_css_attr_unref (SPCSSAttr * css)
{
	g_assert (css != NULL);
	sp_repr_unref ((SPRepr *) css);
}

SPCSSAttr * sp_repr_css_attr (SPRepr * repr, const gchar * attr)
{
	SPCSSAttr * css;

	g_assert (repr != NULL);
	g_assert (attr != NULL);

	css = sp_repr_css_attr_new ();

	sp_repr_css_add_components (css, repr, attr);

	return css;
}

SPCSSAttr * sp_repr_css_attr_inherited (SPRepr * repr, const gchar * attr)
{
	SPCSSAttr * css;
	SPRepr * current;

	g_assert (repr != NULL);
	g_assert (attr != NULL);

	css = sp_repr_css_attr_new ();

	sp_repr_css_add_components (css, repr, attr);
	current = sp_repr_parent (repr);

	while (current) {
		sp_repr_css_add_components (css, current, attr);
		current = sp_repr_parent (current);
	}

	return css;
}

static void
sp_repr_css_add_components (SPCSSAttr * css, SPRepr * repr, const gchar * attr)
{
	const gchar * data;
	gchar * new_str;
	gchar * current;
	gchar ** token, ** ctoken;
	gchar * key, * val;

	g_assert (css != NULL);
	g_assert (repr != NULL);
	g_assert (attr != NULL);

	data = sp_repr_attr (repr, attr);

	if (data != NULL) {
		new_str = g_strdup (data);
		token = g_strsplit (new_str, ";", 32);
		for (ctoken = token; *ctoken != NULL; ctoken++) {
			current = g_strstrip (* ctoken);
			key = current;
			for (val = key; *val != '\0'; val++)
				if (*val == ':') break;
			if (*val == '\0') break;
			*val++ = '\0';
			key = g_strstrip (key);
			val = g_strstrip (val);
			if (*val == '\0') break;

			if (!sp_repr_attr_is_set ((SPRepr *) css, key))
				sp_repr_set_attr ((SPRepr *) css, key, val);
		}
		g_strfreev (token);
		g_free (new_str);
	}

	return;
}

const gchar *
sp_repr_css_property (SPCSSAttr * css, const gchar * name, const gchar * defval)
{
	const gchar * attr;

	g_assert (css != NULL);
	g_assert (name != NULL);

	attr = sp_repr_attr ((SPRepr *) css, name);

	if (attr == NULL) return defval;

	return attr;
}

void
sp_repr_css_set_property (SPCSSAttr * css, const gchar * name, const gchar * value)
{
	g_assert (css != NULL);
	g_assert (name != NULL);

	sp_repr_set_attr ((SPRepr *) css, name, value);
}

gdouble
sp_repr_css_double_property (SPCSSAttr * css, const gchar * name, gdouble defval)
{
	g_assert (css != NULL);
	g_assert (name != NULL);

	return sp_repr_get_double_attribute ((SPRepr *) css, name, defval);
}

void
sp_repr_css_set (SPRepr * repr, SPCSSAttr * css, const gchar * attr)
{
	GList * list;
	const gchar * key;
	gchar * val;
	gchar * result;
	char c[128];

	g_assert (repr != NULL);
	g_assert (css != NULL);
	g_assert (attr != NULL);

	list = sp_repr_attributes ((SPRepr *) css);
	result = "";

	while (list) {
		key = (const gchar *) list->data;
		val = (gchar *) sp_repr_attr ((SPRepr *) css, key);
		snprintf (c, 128, "%s:%s; ", key, val);
		result = g_strconcat (result, c, NULL);
		list = g_list_remove (list, (gpointer) key);
	}
g_print ("style: %s\n", result);
	sp_repr_set_attr (repr, attr, result);
}

static void
sp_repr_css_merge (SPCSSAttr * dst, SPCSSAttr * src)
{
	GList * list;
	const gchar * key;
	gchar * val;

	g_assert (dst != NULL);
	g_assert (src != NULL);

	list = sp_repr_attributes ((SPRepr *) src);

	while (list) {
		key = (const gchar *) list->data;
		val = (gchar *) sp_repr_attr ((SPRepr *) src, key);
		sp_repr_set_attr ((SPRepr *) dst, key, val);
		list = g_list_remove (list, (gpointer) key);
	}

}

void
sp_repr_css_change (SPRepr * repr, SPCSSAttr * css, const gchar * attr)
{
	SPCSSAttr * current;

	g_assert (repr != NULL);
	g_assert (css != NULL);
	g_assert (attr != NULL);

	current = sp_repr_css_attr (repr, attr);
	sp_repr_css_merge (current, css);
	sp_repr_css_set (repr, current, attr);

	sp_repr_css_attr_unref (current);
}

void
sp_repr_css_change_recursive (SPRepr * repr, SPCSSAttr * css, const gchar * attr)
{
	const GList * list;
	SPRepr * child;

	g_assert (repr != NULL);
	g_assert (css != NULL);
	g_assert (attr != NULL);

	sp_repr_css_change (repr, css, attr);

	list = sp_repr_children (repr);

	while (list) {
		child = (SPRepr *) list->data;
		sp_repr_css_change_recursive (child, css, attr);
		list = list->next;
	}
}

