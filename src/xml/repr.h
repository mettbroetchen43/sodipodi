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
#include <stdio.h>

/*
 * SPRepr is opaque
 */

typedef struct _SPRepr SPRepr;

/* Create new repr & similar */

SPRepr *sp_repr_new (const guchar *name);
SPRepr *sp_repr_ref (SPRepr *repr);
SPRepr *sp_repr_unref (SPRepr *repr);
SPRepr *sp_repr_duplicate (const SPRepr *repr);

/* Documents - 1st step in migrating to real XML */

typedef struct _SPReprDoc SPReprDoc;

SPReprDoc * sp_repr_document_new (const gchar * rootname);
void sp_repr_document_ref (SPReprDoc * doc);
void sp_repr_document_unref (SPReprDoc * doc);
SPRepr * sp_repr_document_root (SPReprDoc * doc);

SPReprDoc * sp_repr_document (SPRepr * repr);
/* Documents Utility */
void        sp_repr_document_overwrite (SPReprDoc       *doc,
					const SPReprDoc *source,
					const guchar    *key);

/* Contents */

const guchar *sp_repr_name (const SPRepr *repr);
const guchar *sp_repr_content (const SPRepr *repr);
const guchar *sp_repr_attr (const SPRepr *repr, const guchar *key);

/*
 * NB! signal handler may decide, that change is not allowed
 *
 * TRUE, if successful
 */

gboolean sp_repr_set_content (SPRepr *repr, const guchar *content);
gboolean sp_repr_set_attr (SPRepr *repr, const guchar *key, const guchar *value);

/*
 * Returns list of attribute strings
 * List should be freed by caller, but attributes not
 */

GList * sp_repr_attributes (SPRepr * repr);

void sp_repr_set_data (SPRepr * repr, gpointer data);
gpointer sp_repr_data (SPRepr * repr);

SPRepr * sp_repr_parent (SPRepr * repr);
const GList * sp_repr_children (SPRepr * repr);
gint sp_repr_n_children (SPRepr * repr);
gboolean sp_repr_add_child (SPRepr * repr, SPRepr * child, SPRepr * ref);
gboolean sp_repr_remove_child (SPRepr * repr, SPRepr * child);
#if 0
void sp_repr_set_signal (SPRepr * repr, const gchar * name, gpointer func, gpointer data);
#endif
/* IO */

SPReprDoc * sp_repr_read_file (const gchar * filename);
SPReprDoc * sp_repr_read_mem (const gchar * buffer, gint length);
void sp_repr_save_stream (SPReprDoc * doc, FILE * to_file);
void sp_repr_save_file (SPReprDoc * doc, const gchar * filename);

void sp_repr_print (SPRepr * repr);

/* CSS stuff */

typedef struct _SPCSSAttr SPCSSAttr;

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

void sp_repr_unparent (SPRepr * repr);

gint sp_repr_attr_is_set (SPRepr * repr, const gchar * key);

gdouble sp_repr_get_double_attribute (SPRepr * repr, const gchar * key, gdouble def);
gint sp_repr_get_int_attribute (SPRepr * repr, const gchar * key, gint def);
gboolean sp_repr_set_double_attribute (SPRepr * repr, const gchar * key, gdouble value);
gboolean sp_repr_set_int_attribute (SPRepr * repr, const gchar * key, gint value);

gint sp_repr_compare_position (SPRepr * first, SPRepr * second);

gint sp_repr_position (SPRepr * repr);
void sp_repr_set_position_absolute (SPRepr * repr, gint pos);
void sp_repr_set_position_relative (SPRepr * repr, gint pos);
gint sp_repr_n_children (SPRepr * repr);
void sp_repr_append_child (SPRepr * repr, SPRepr * child);

const gchar * sp_repr_doc_attr (SPRepr * repr, const gchar * key);

SPRepr * sp_repr_duplicate_and_parent (SPRepr * repr);

void sp_repr_remove_signals (SPRepr * repr);

const guchar *sp_repr_attr_inherited (SPRepr *repr, const guchar *key);
gboolean sp_repr_set_attr_recursive (SPRepr *repr, const guchar *key, const guchar *value);

SPRepr       *sp_repr_lookup_child  (SPRepr    	        *repr,
				     const guchar       *key,
				     const guchar       *value);
gboolean      sp_repr_overwrite     (SPRepr             *repr,
				     const SPRepr       *src,
				     const guchar       *key);


#endif
