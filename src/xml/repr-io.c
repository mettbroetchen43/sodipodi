#define SP_REPR_IO_C

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "repr.h"
#include "repr-private.h"
#include <xmlmemory.h>
#include <parser.h>
#include <tree.h>

static SPRepr * sp_repr_svg_read_node (xmlNodePtr node);
static void repr_write (SPRepr * repr, FILE * file, gint level);

#ifdef HAVE_LIBWMF
static xmlDocPtr sp_wmf_convert (const char * file_name);
static char * sp_wmf_image_name (void * context);
#endif /* HAVE_LIBWMF */

SPReprDoc * sp_repr_read_file (const gchar * filename)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	SPReprDoc * rdoc;
	SPRepr * repr;

	g_return_val_if_fail (filename != NULL, NULL);

#ifdef HAVE_LIBWMF
	if (strlen (filename) > 4) {
		if ( (strcmp (filename + strlen (filename) - 4,".wmf") == 0)
		  || (strcmp (filename + strlen (filename) - 4,".WMF") == 0))
			doc = sp_wmf_convert (filename);
		else
			doc = xmlParseFile (filename);
	}
	else {
		doc = xmlParseFile (filename);
	}
#else /* HAVE_LIBWMF */
	doc = xmlParseFile (filename);
#endif /* HAVE_LIBWMF */
	if (doc == NULL) return NULL;

	rdoc = sp_repr_document_new ("void");

	repr = NULL;

	for (node = xmlDocGetRootElement(doc); node != NULL; node = node->next) {
#if 0
		if (node->name && (strcmp (node->name, "svg") == 0)) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
#else
		if (node->type == XML_ELEMENT_NODE) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
#endif
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

	rdoc = sp_repr_document_new ("void");

	repr = NULL;

	for (node = xmlDocGetRootElement(doc); node != NULL; node = node->next) {
#if 0
		if (node->name && (strcmp (node->name, "svg") == 0)) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
#else
		if (node->type == XML_ELEMENT_NODE) {
			repr = sp_repr_svg_read_node (node);
			break;
		}
#endif
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

	if (node->ns && node->ns->prefix && strcmp (node->ns->prefix, "svg")) {
		g_snprintf (c, 256, "%s:%s", node->ns->prefix, node->name);
	} else {
		g_snprintf (c, 256, node->name);
	}

	repr = sp_repr_new (c);

	for (prop = node->properties; prop != NULL; prop = prop->next) {
        xmlChar *value = xmlGetProp(node,prop->name);
		if (value != NULL) {
			if (prop->ns && prop->ns->prefix && strcmp (prop->ns->prefix, "svg")) {
				g_snprintf (c, 256, "%s:%s", prop->ns->prefix, prop->name);
			} else {
				g_snprintf (c, 256, prop->name);
			}
			sp_repr_set_attr (repr, c, value);
            xmlFree (value);
		}
	}

	if (node->content)
		sp_repr_set_content (repr, node->content);

	child = node->xmlChildrenNode;
#if 1
	if ((child != NULL) &&
        (child->name != NULL) &&
		(strcmp (child->name, node->name) == 0) &&
		(child->properties == NULL) &&
		(child->content != NULL)) {

		sp_repr_set_content (repr, child->content);

	} else {
#endif
	for (child = node->xmlChildrenNode; child != NULL; child = child->next) {
		crepr = sp_repr_svg_read_node (child);
		if (crepr) sp_repr_append_child (repr, crepr);
	}
#if 1
	}
#endif
	return repr;
}

void
sp_repr_save_stream (SPReprDoc * doc, FILE * to_file)
{
	SPRepr * repr;
	/* fixme: do this The Right Way */

	fputs ("<?xml version=\"1.0\" standalone=\"no\"?>\n"
	       "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\"\n"
	       "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n",
	       to_file);

	repr = sp_repr_document_root (doc);

	repr_write (repr, to_file, 0);
}

void
sp_repr_save_file (SPReprDoc * doc, const gchar * filename)
{
	FILE * file;

	file = fopen (filename, "w");
	g_return_if_fail (file != NULL);

	sp_repr_save_stream (doc, file);

	fclose (file);

	return;
}

void
sp_repr_print (SPRepr * repr)
{
	repr_write (repr, stdout, 0);

	return;
}

/* (No doubt this function already exists elsewhere.) */
static void
repr_quote_write (FILE * file, const gchar * val)
{
	/* TODO: Presumably VAL is actually encoded in UTF8, no?
	 * [Actually, preliminary tests suggest that this is not the
	 * case; can sodipodi encode big charsets at all?]  If VAL is
	 * UTF8, then this code will be buggy if e.g. `&' is the
	 * second byte of a single utf8 "character".
	 */

	for (; *val != '\0'; val++) {
		switch (*val) {
		case '"': fputs ("&quot;", file); break;
		case '&': fputs ("&amp;", file); break;
		case '<': fputs ("&lt;", file); break;
		case '>': fputs ("&gt;", file); break;
		default: putc (*val, file); break;
		}
	}
}

static void
repr_write (SPRepr * repr, FILE * file, gint level)
{
	SPReprAttr * attr;
	SPRepr * child;
	const gchar * key, * val;
	gint i;

	g_return_if_fail (repr != NULL);
	g_return_if_fail (file != NULL);

	if (level > 16) level = 16;
	for (i = 0; i < level; i++) fputs ("  ", file);
	fprintf (file, "<%s", sp_repr_name (repr));

	for (attr = repr->attributes; attr != NULL; attr = attr->next) {
		key = SP_REPR_ATTRIBUTE_KEY (attr);
		val = SP_REPR_ATTRIBUTE_VALUE (attr);
		fputs ("\n", file);
		for (i = 0; i < level + 1; i++) fputs ("  ", file);
		fprintf (file, " %s=\"", key);
		repr_quote_write (file, val);
		putc ('"', file);
	}

	fprintf (file, ">");

	for (child = repr->children; child != NULL; child = child->next) {
		repr_write (child, file, level + 1);
	}

	if (sp_repr_content (repr)) {
		repr_quote_write (file, sp_repr_content (repr));
	} else {
		fputs ("\n", file);
		for (i = 0; i < level; i++) fputs ("  ", file);
	}

	fprintf (file, "</%s>\n", sp_repr_name (repr));
}

#ifdef HAVE_LIBWMF

#include <math.h>

#include <libwmf/api.h>
#include <libwmf/svg.h>

#define SP_WMF_MAXWIDTH  (596)
#define SP_WMF_MAXHEIGHT (812)

typedef struct _SPImageContext SPImageContext;

struct _SPImageContext
{	wmfAPI* API;

	int number;

	char* prefix;
};

static xmlDocPtr
sp_wmf_convert (const char * file_name)
{
	float wmf_width;
	float wmf_height;

	float ratio_wmf;
	float ratio_bounds;

	unsigned long flags;
	unsigned long max_flags;
	unsigned long length;

	char * buffer = 0;

	static char * Default_Description = "sodipodi";

	SPImageContext IC;

	wmf_error_t err;

	wmf_svg_t * ddata = 0;

	wmfAPI * API = 0;

	wmfAPI_Options api_options;

	wmfD_Rect bbox;

	xmlDocPtr doc = 0;

	flags = WMF_OPT_IGNORE_NONFATAL;

	flags |= WMF_OPT_FUNCTION;
	api_options.function = wmf_svg_function;

	err = wmf_api_create (&API, flags, &api_options);

	if (err != wmf_E_None) {
		if (API) wmf_api_destroy (API);
		return (0);
	}

	err = wmf_file_open (API, (char *) file_name);

	if (err != wmf_E_None) {
		wmf_api_destroy (API);
		return (0);
	}

	err = wmf_scan (API, 0, &bbox);

	if (err != wmf_E_None) {
		wmf_api_destroy (API);
		return (0);
	}

/* Okay, got this far, everything seems cool.
 */
	ddata = WMF_SVG_GetData (API);

	ddata->out = wmf_stream_create (API, 0);

	ddata->Description = Default_Description;

	ddata->bbox = bbox;

	wmf_width  = ddata->bbox.BR.x - ddata->bbox.TL.x;
	wmf_height = ddata->bbox.BR.y - ddata->bbox.TL.y;

	if ((wmf_width <= 0) || (wmf_height <= 0)) { /* Bad image size - but this error shouldn't occur... */
		wmf_api_destroy (API);
		return (0);
	}

	max_flags = 0;

	if ((wmf_width > (float) SP_WMF_MAXWIDTH) || (wmf_height > (float) SP_WMF_MAXHEIGHT)) {
		if (max_flags == 0)
			max_flags = 1;
	}

	if (max_flags) { /* scale the image */
		ratio_wmf = wmf_height / wmf_width;
		ratio_bounds = (float) SP_WMF_MAXHEIGHT / (float) SP_WMF_MAXWIDTH;

		if (ratio_wmf > ratio_bounds) {
			ddata->height = SP_WMF_MAXHEIGHT;
			ddata->width  = (unsigned int) ((float) ddata->height / ratio_wmf);
		}
		else {
			ddata->width  = SP_WMF_MAXWIDTH;
			ddata->height = (unsigned int) ((float) ddata->width  * ratio_wmf);
		}
	}
	else {
		ddata->width  = (unsigned int) ceil ((double) wmf_width );
		ddata->height = (unsigned int) ceil ((double) wmf_height);
	}

	IC.API = API;
	IC.number = 0;
	IC.prefix = (char *) wmf_malloc (API, strlen (file_name) + 1);
	if (IC.prefix) {
		strcpy (IC.prefix, file_name);
		IC.prefix[strlen (file_name) - 4] = 0;
		ddata->image.context = (void *) (&IC);
		ddata->image.name = sp_wmf_image_name;
	}

	err = wmf_play (API, 0, &bbox);

	if (err == wmf_E_None) {
		wmf_stream_destroy (API, ddata->out, &buffer, &length);

		doc = xmlParseMemory (buffer, (int) length);
	}

	wmf_api_destroy (API);

	return (doc);
}

static char *
sp_wmf_image_name (void * context)
{
	SPImageContext * IC = (SPImageContext *) context;

	int length;

	char * name = 0;

	length = strlen (IC->prefix) + 16;

	name = wmf_malloc (IC->API, length);

	if (name == 0) return (0);

	IC->number++;

	sprintf (name,"%s-%d.png", IC->prefix, IC->number);

	return (name);
}

#endif /* HAVE_LIBWMF */
