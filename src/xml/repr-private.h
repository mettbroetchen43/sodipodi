#ifndef __SP_REPR_PRIVATE_H__
#define __SP_REPR_PRIVATE_H__

/*
 * Fuzzy DOM-like tree implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "repr.h"
#include "repr-action.h"

typedef struct _SPRepr SPXMLNode;
typedef struct _SPRepr SPXMLText;
typedef struct _SPRepr SPXMLElement;
typedef struct _SPReprAttr SPXMLAttribute;
typedef struct _SPReprDoc SPXMLDocument;
typedef struct _SPXMLNs SPXMLNs;

typedef struct _SPReprClass SPReprClass;
typedef struct _SPReprListener SPReprListener;

struct _SPReprClass {
	size_t size;
	SPRepr *pool;
	void (*init)(SPRepr *repr);
	void (*copy)(SPRepr *to, const SPRepr *from);
	void (*finalize)(SPRepr *repr);
};

struct _SPReprAttr {
	SPReprAttr *next;
	int key;
	unsigned char *value;
};

struct _SPReprListener {
	SPReprListener *next;
	const SPReprEventVector *vector;
	void * data;
};

struct _SPRepr {
	SPReprClass *type;
	int refcount;

	int name;

	SPReprDoc *doc;
	SPRepr *parent;
	SPRepr *next;
	SPRepr *children;

	SPReprAttr *attributes;
	SPReprListener *listeners;
	unsigned char *content;
};

struct _SPReprDoc {
	SPRepr repr;
	SPRepr *root;
	unsigned int is_logging;
	SPReprAction *log;
};

struct _SPXMLNs {
	SPXMLNs *next;
	unsigned int uri, prefix;
};

#define SP_REPR_NAME(r) g_quark_to_string ((r)->name)
#define SP_REPR_TYPE(r) ((r)->type)
#define SP_REPR_CONTENT(r) ((r)->content)
#define SP_REPR_ATTRIBUTE_KEY(a) g_quark_to_string ((a)->key)
#define SP_REPR_ATTRIBUTE_VALUE(a) ((a)->value)

extern SPReprClass _sp_repr_xml_document_class;
extern SPReprClass _sp_repr_xml_element_class;
extern SPReprClass _sp_repr_xml_text_class;
extern SPReprClass _sp_repr_xml_cdata_class;
extern SPReprClass _sp_repr_xml_comment_class;

#define SP_XML_DOCUMENT_NODE &_sp_repr_xml_document_class
#define SP_XML_ELEMENT_NODE &_sp_repr_xml_element_class
#define SP_XML_TEXT_NODE &_sp_repr_xml_text_class
#define SP_XML_CDATA_NODE &_sp_repr_xml_cdata_class
#define SP_XML_COMMENT_NODE &_sp_repr_xml_comment_class

SPRepr *sp_repr_nth_child (const SPRepr *repr, int n);

void sp_repr_document_set_root (SPReprDoc *doc, SPRepr *repr);

/* Stuff from repr.h */

/* SPXMLNs */

const unsigned char *sp_xml_ns_uri_prefix (const unsigned char *uri, const unsigned char *suggested);
const unsigned char *sp_xml_ns_prefix_uri (const unsigned char *prefix);

/* SPXMLDocument */

SPXMLText *sp_xml_document_createTextNode (SPXMLDocument *doc, const unsigned char *content);
SPXMLElement *sp_xml_document_createElement (SPXMLDocument *doc, const unsigned char *name);
SPXMLElement *sp_xml_document_createElementNS (SPXMLDocument *doc, const unsigned char *ns, const unsigned char *qname);

/* SPXMLNode */

SPXMLDocument *sp_xml_node_get_Document (SPXMLNode *node);

/* SPXMLElement */

void sp_xml_element_setAttributeNS (SPXMLElement *element, const unsigned char *ns, const unsigned char *qname, const unsigned char *val);

/* IO */


void sp_repr_print (SPRepr * repr);

/* CSS stuff */

typedef struct _SPCSSAttr SPCSSAttr;

SPCSSAttr * sp_repr_css_attr_new (void);
void sp_repr_css_attr_unref (SPCSSAttr * css);
SPCSSAttr * sp_repr_css_attr (SPRepr * repr, const char * attr);
SPCSSAttr * sp_repr_css_attr_inherited (SPRepr * repr, const char * attr);

const char * sp_repr_css_property (SPCSSAttr * css, const char * name, const char * defval);
void sp_repr_css_set_property (SPCSSAttr * css, const char * name, const char * value);
double sp_repr_css_double_property (SPCSSAttr * css, const char * name, double defval);

void sp_repr_css_set (SPRepr * repr, SPCSSAttr * css, const char * key);
void sp_repr_css_change (SPRepr * repr, SPCSSAttr * css, const char * key);
void sp_repr_css_change_recursive (SPRepr * repr, SPCSSAttr * css, const char * key);

/* Utility finctions */

int sp_repr_attr_is_set (SPRepr * repr, const char * key);

/* Defaults */
unsigned int sp_repr_set_double_default (SPRepr *repr, const unsigned char *key, double val, double def, double e);

int sp_repr_compare_position (SPRepr * first, SPRepr * second);

int sp_repr_position (SPRepr * repr);
void sp_repr_set_position_absolute (SPRepr * repr, int pos);
void sp_repr_set_position_relative (SPRepr * repr, int pos);
int sp_repr_n_children (SPRepr * repr);

const char *sp_repr_doc_attr (SPRepr * repr, const char * key);

SPRepr *sp_repr_duplicate_and_parent (SPRepr * repr);

void sp_repr_remove_signals (SPRepr * repr);

const unsigned char *sp_repr_attr_inherited (SPRepr *repr, const unsigned char *key);
unsigned int sp_repr_set_attr_recursive (SPRepr *repr, const unsigned char *key, const unsigned char *value);

unsigned int sp_repr_overwrite (SPRepr *repr, const SPRepr *src, const unsigned char *key);

#define sp_repr_attr sp_repr_get_attr
#define sp_repr_parent sp_repr_get_parent
#define sp_repr_name sp_repr_get_name
#define sp_repr_content sp_repr_get_content

#endif
