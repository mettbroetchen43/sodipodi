#ifndef SP_REPR_H
#define SP_REPR_H

/*
 * repr.h
 *
 * Currently fuzzy wrapper thing, which will be (I hope) merged into
 * gdome as soon, as it stabilizes
 *
 */

#include <glib.h>

/*
 * SPRepr is opaque
 */

typedef struct _SPRepr SPRepr;
typedef struct _SPRepr SPReprDoc;
typedef struct _SPCSSAttr SPCSSAttr;

/* Create new repr & similar */

SPRepr * sp_repr_new (void);
void sp_repr_ref (SPRepr * repr);
void sp_repr_unref (SPRepr * repr);
SPRepr * sp_repr_copy (SPRepr * repr);

SPReprDoc * sp_repr_document (SPRepr * repr);
void sp_repr_set_name (SPRepr * repr, const gchar * name);
const gchar * sp_repr_name (SPRepr * repr);
void sp_repr_set_content (SPRepr * repr, const gchar * content);
const gchar * sp_repr_content (SPRepr * repr);
void sp_repr_set_attr (SPRepr * repr, const gchar * key, const gchar * value);
const gchar * sp_repr_attr (SPRepr * repr, const gchar * key);

void sp_repr_set_data (SPRepr * repr, gpointer data);
gpointer sp_repr_data (SPRepr * repr);

SPRepr * sp_repr_parent (SPRepr * repr);
const GList * sp_repr_children (SPRepr * repr);
gint sp_repr_n_children (SPRepr * repr);
void sp_repr_add_child (SPRepr * repr, SPRepr * child, gint position);
void sp_repr_remove_child (SPRepr * repr, SPRepr * child);

void sp_repr_set_signal (SPRepr * repr, const gchar * name, gpointer func, gpointer data);

/* IO */

SPRepr * sp_repr_read_file (const gchar * filename);

void sp_repr_save_file (SPRepr * repr, const gchar * filename);

void sp_repr_print (SPRepr * repr);

/* CSS stuff */

SPCSSAttr * sp_repr_css_attr_new (void);
void sp_repr_css_attr_unref (SPCSSAttr * css);
SPCSSAttr * sp_repr_css_attr (SPRepr * repr, const gchar * attr);
SPCSSAttr * sp_repr_css_attr_inherited (SPRepr * repr, const gchar * attr);

const gchar * sp_repr_css_property (SPCSSAttr * css, const gchar * name, const gchar * defval);
void sp_repr_css_set_property (SPCSSAttr * css, const gchar * name, const gchar * value);
gdouble sp_repr_css_double_property (SPCSSAttr * css, const gchar * name, gdouble defval);

void sp_repr_css_set (SPRepr * repr, SPCSSAttr * css, const gchar * key);
void sp_repr_css_change (SPRepr * repr, SPCSSAttr * css, const gchar * key);
void sp_repr_css_change_recursive (SPRepr * repr, SPCSSAttr * css, const gchar * key);

/* Utility finctions */

SPRepr * sp_repr_new_with_name (const gchar * name);

gint sp_repr_attr_is_set (SPRepr * repr, const gchar * key);
double sp_repr_get_double_attribute (SPRepr * repr, const gchar * key, double def);
void sp_repr_set_double_attribute (SPRepr * repr, const gchar * key, double value);

gint sp_repr_compare_position (SPRepr * first, SPRepr * second);

gint sp_repr_position (SPRepr * repr);
void sp_repr_set_position_absolute (SPRepr * repr, gint pos);
void sp_repr_set_position_relative (SPRepr * repr, gint pos);
gint sp_repr_n_children (SPRepr * repr);
void sp_repr_append_child (SPRepr * repr, SPRepr * child);

const gchar * sp_repr_doc_attr (SPRepr * repr, const gchar * key);

void sp_repr_unparent (SPRepr * repr);
SPRepr * sp_repr_duplicate_and_parent (SPRepr * repr);

void sp_repr_remove_signals (SPRepr * repr);

const gchar * sp_repr_attr_inherited (SPRepr * repr, const gchar * key);

#endif
