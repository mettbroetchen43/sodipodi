#ifndef __SP_REPR_H__
#define __SP_REPR_H__

/*
 * Fuzzy DOM-like tree implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include <stdio.h>

/*
 * NB! Unless explicitly stated all methods are noref/nostrcpy
 */

typedef struct _SPRepr SPXMLNode;
typedef struct _SPRepr SPXMLText;
typedef struct _SPRepr SPXMLElement;
typedef struct _SPReprAttr SPXMLAttribute;
typedef struct _SPReprDoc SPXMLDocument;

/* SPXMLDocument */

SPXMLText *sp_xml_document_createTextNode (SPXMLDocument *doc, const guchar *data);
SPXMLElement *sp_xml_document_createElement (SPXMLDocument *doc, const guchar *name);
SPXMLElement *sp_xml_document_createElementNS (SPXMLDocument *doc, const guchar *ns, const guchar *qname);

/* SPXMLNode */

SPXMLDocument *sp_xml_node_get_Document (SPXMLNode *node);

/* SPXMLElement */

void sp_xml_element_setAttributeNS (SPXMLElement *element, const guchar *nr, const guchar *qname, const guchar *val);

/*
 * SPRepr is opaque
 */

typedef struct _SPRepr SPRepr;

/* Create new repr & similar */

SPRepr *sp_repr_new (const guchar *name);
SPRepr *sp_repr_new_text (const guchar *content);
SPRepr *sp_repr_ref (SPRepr *repr);
SPRepr *sp_repr_unref (SPRepr *repr);
SPRepr *sp_repr_duplicate (const SPRepr *repr);

/* Documents - 1st step in migrating to real XML */

typedef struct _SPReprDoc SPReprDoc;

SPReprDoc * sp_repr_document_new (const gchar * rootname);
void sp_repr_document_ref (SPReprDoc * doc);
void sp_repr_document_unref (SPReprDoc * doc);

SPRepr *sp_repr_document_root (const SPReprDoc *doc);
SPReprDoc *sp_repr_document (const SPRepr *repr);

/* Documents Utility */
gboolean sp_repr_document_merge (SPReprDoc *doc, const SPReprDoc *src, const guchar *key);
gboolean sp_repr_merge (SPRepr *repr, const SPRepr *src, const guchar *key);

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

/* Tree */
SPRepr *sp_repr_parent (SPRepr *repr);
SPRepr *sp_repr_children (SPRepr *repr);
SPRepr *sp_repr_next (SPRepr *repr);

gboolean sp_repr_add_child (SPRepr * repr, SPRepr * child, SPRepr * ref);
gboolean sp_repr_remove_child (SPRepr * repr, SPRepr * child);

#if 0
const GList *sp_repr_get_children_list (SPRepr * repr);
#endif
gint sp_repr_n_children (SPRepr * repr);

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

/* Convenience */
gboolean sp_repr_get_boolean (SPRepr *repr, const guchar *key, gboolean *val);
gboolean sp_repr_get_int (SPRepr *repr, const guchar *key, gint *val);
gboolean sp_repr_get_double (SPRepr *repr, const guchar *key, gdouble *val);
gboolean sp_repr_set_boolean (SPRepr *repr, const guchar *key, gboolean val);
gboolean sp_repr_set_int (SPRepr *repr, const guchar *key, gint val);
gboolean sp_repr_set_double (SPRepr *repr, const guchar *key, gdouble val);
/* Defaults */
gboolean sp_repr_set_double_default (SPRepr *repr, const guchar *key, gdouble val, gdouble def, gdouble e);

/* Deprecated */
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
