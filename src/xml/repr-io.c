#define SP_REPR_IO_C

#include <stdio.h>
#include "repr.h"
#include "repr-private.h"
#include <gnome-xml/parser.h>
#include <gnome-xml/tree.h>

static SPRepr * sp_repr_svg_read_node (xmlNodePtr node);
static void repr_write (SPRepr * repr, FILE * file, gint level);

SPRepr * sp_repr_read_file (const gchar * filename)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	SPRepr * repr;

	g_return_val_if_fail (filename != NULL, NULL);

	doc = xmlParseFile (filename);
	if (doc == NULL) return NULL;

	repr = NULL;

	for (node = doc->root; node != NULL; node = node->next) {
		if (strcmp (node->name, "svg") == 0) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
	}

	xmlFreeDoc (doc);

	return repr;
}

static SPRepr * sp_repr_svg_read_node (xmlNodePtr node)
{
	SPRepr * repr, * crepr;
	xmlAttr * prop;
	xmlNodePtr child;

	g_return_val_if_fail (node != NULL, NULL);

	repr = sp_repr_new (node->name);

	for (prop = node->properties; prop != NULL; prop = prop->next) {
		if (prop->val) {
			sp_repr_set_attr (repr, prop->name, prop->val->content);
		}
	}

	if (node->content)
		sp_repr_set_content (repr, g_strdup (node->content));

	child = node->childs;
	if ((child != NULL) &&
		(strcmp (child->name, node->name) == 0) &&
		(child->properties == NULL) &&
		(child->content != NULL)) {

		sp_repr_set_content (repr, g_strdup (child->content));

	} else {

	for (child = node->childs; child != NULL; child = child->next) {
		crepr = sp_repr_svg_read_node (child);
		sp_repr_append_child (repr, crepr);
	}

	}

	return repr;
}

void
sp_repr_save_file (SPRepr * repr, const gchar * filename)
{
	FILE * file;

	file = fopen (filename, "w");
	g_return_if_fail (file != NULL);

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
		fputs ("\n", file);
	}

	for (i = 0; i < level; i++) fputs ("  ", file);
	fprintf (file, "</%s>", sp_repr_name (repr));
}

