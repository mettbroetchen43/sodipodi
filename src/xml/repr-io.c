#define SP_REPR_IO_C

#include <stdio.h>
#include "repr.h"
#include "repr-private.h"
#include <gnome-xml/parser.h>
#include <gnome-xml/tree.h>

static SPRepr * sp_repr_svg_read_node (xmlNodePtr node);
static void repr_write (SPRepr * repr, FILE * file, gint level);

SPReprDoc * sp_repr_read_file (const gchar * filename)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	SPReprDoc * rdoc;
	SPRepr * repr;

	g_return_val_if_fail (filename != NULL, NULL);

	doc = xmlParseFile (filename);
	if (doc == NULL) return NULL;

	rdoc = sp_repr_document_new ();

	repr = NULL;

	for (node = doc->root; node != NULL; node = node->next) {
		if (strcmp (node->name, "svg") == 0) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
	}

	if (repr != NULL) sp_repr_document_set_root (rdoc, repr);

	xmlFreeDoc (doc);

	return rdoc;
}

SPReprDoc * sp_repr_read_mem (const gchar * buffer, gint length)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	SPReprDoc * rdoc;
	SPRepr * repr;

	g_return_val_if_fail (buffer != NULL, NULL);

	doc = xmlParseMemory ((gchar *) buffer, length);
	if (doc == NULL) return NULL;

	rdoc = sp_repr_document_new ();

	repr = NULL;

	for (node = doc->root; node != NULL; node = node->next) {
		if (strcmp (node->name, "svg") == 0) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
	}

	if (repr != NULL) sp_repr_document_set_root (rdoc, repr);

	xmlFreeDoc (doc);

	return rdoc;
}

static SPRepr * sp_repr_svg_read_node (xmlNodePtr node)
{
	SPRepr * repr, * crepr;
	xmlAttr * prop;
	xmlNodePtr child;
	gchar c[256];

	g_return_val_if_fail (node != NULL, NULL);

	if (node->type == XML_TEXT_NODE) return NULL;
	if (node->type == XML_COMMENT_NODE) return NULL;

	g_snprintf (c, 256, node->name);
	if (node->ns != NULL) {
	if (node->ns->prefix != NULL) {
		if (strcmp (node->ns->prefix, "sodipodi") == 0) {
			g_snprintf (c, 256, "sodipodi:%s", node->name);
		}
	}
	}

	repr = sp_repr_new (c);

	for (prop = node->properties; prop != NULL; prop = prop->next) {
		if (prop->val) {
			g_snprintf (c, 256, prop->name);
			if (prop->ns != NULL) {
			if (prop->ns->prefix != NULL) {
			if (strcmp (prop->ns->prefix, "sodipodi") == 0) {
				g_snprintf (c, 256, "sodipodi:%s", prop->name);
			}
			}
			}
			sp_repr_set_attr (repr, c, prop->val->content);
		}
	}

	if (node->content)
		sp_repr_set_content (repr, node->content);

	child = node->childs;
#if 1
	if ((child != NULL) &&
		(strcmp (child->name, node->name) == 0) &&
		(child->properties == NULL) &&
		(child->content != NULL)) {

		sp_repr_set_content (repr, child->content);

	} else {
#endif
	for (child = node->childs; child != NULL; child = child->next) {
		crepr = sp_repr_svg_read_node (child);
		if (crepr) sp_repr_append_child (repr, crepr);
	}
#if 1
	}
#endif
	return repr;
}

void
sp_repr_save_file (SPReprDoc * doc, const gchar * filename)
{
	SPRepr * repr;
	FILE * file;

	file = fopen (filename, "w");
	g_return_if_fail (file != NULL);

	/* fixme: do this The Right Way */

	fputs ("<?xml version=\"1.0\" standalone=\"no\"?>\n"
		"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG August 1999//EN\"\n"
		"\"http://www.w3.org/Graphics/SVG/SVG-19990812.dtd\">\n",
		file);

	repr = sp_repr_document_root (doc);

	repr_write (repr, file, 0);

	fclose (file);

	return;
}

void
sp_repr_print (SPRepr * repr)
{
	repr_write (repr, stdout, 0);

	return;
}

static void
repr_write (SPRepr * repr, FILE * file, gint level)
{
	GList * attrlist;
	const GList * childrenlist;
	SPRepr * child;
	const gchar * key, * val;
	gint i;

	g_return_if_fail (repr != NULL);
	g_return_if_fail (file != NULL);

	if (level > 16) level = 16;
	for (i = 0; i < level; i++) fputs ("  ", file);
	fprintf (file, "<%s", sp_repr_name (repr));

	attrlist = sp_repr_attributes (repr);

	while (attrlist) {
		key = (const gchar *) attrlist->data;
		val = sp_repr_attr (repr, key);
		fputs ("\n", file);
		for (i = 0; i < level + 1; i++) fputs ("  ", file);
		fprintf (file, " %s=\"%s\"", key, val);
		attrlist = g_list_remove (attrlist, (gpointer) key);
	}

	fprintf (file, ">");

	childrenlist = sp_repr_children (repr);

	while (childrenlist) {
		child = (SPRepr *) childrenlist->data;
		repr_write (child, file, level + 1);
		childrenlist = childrenlist->next;
	}

	if (sp_repr_content (repr)) {
		fprintf (file, "%s", sp_repr_content (repr));
	} else {
		fputs ("\n", file);
		for (i = 0; i < level; i++) fputs ("  ", file);
	}

	fprintf (file, "</%s>\n", sp_repr_name (repr));
}

