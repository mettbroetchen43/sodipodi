#define SP_OBJECT_REPR_C

#include "document.h"
#include "sp-item.h"
#include "sp-defs.h"
#include "sp-use.h"
#include "sp-root.h"
#include "sp-namedview.h"
#include "sp-guide.h"
#include "sp-image.h"
#include "sp-rect.h" 
#include "sp-ellipse.h"
#include "sp-line.h"
#include "sp-polyline.h"
#include "sp-polygon.h"
#include "sp-text.h" 
#include "sp-gradient.h"
#include "sp-object-repr.h"

SPObject *
sp_object_repr_build_tree (SPDocument * document, SPRepr * repr)
{
	const gchar * name;
	GtkType type;
	SPObject * object;

	g_assert (document != NULL);
	g_assert (SP_IS_DOCUMENT (document));
	g_assert (repr != NULL);

	name = sp_repr_name (repr);
	g_assert (name != NULL);
	type = sp_object_type_lookup (name);
	g_assert (gtk_type_is_a (type, SP_TYPE_ROOT));
	object = gtk_type_new (type);
	g_assert (object != NULL);
	sp_object_invoke_build (object, document, repr, FALSE);

	return object;
}

GtkType
sp_object_type_lookup (const gchar * name)
{
	static GHashTable * dtable = NULL;
	gpointer data;

	if (dtable == NULL) {
		dtable = g_hash_table_new (g_str_hash, g_str_equal);
		g_assert (dtable != NULL);
		g_hash_table_insert (dtable, "defs", GINT_TO_POINTER (SP_TYPE_DEFS));
		g_hash_table_insert (dtable, "use", GINT_TO_POINTER (SP_TYPE_USE));
		g_hash_table_insert (dtable, "g", GINT_TO_POINTER (SP_TYPE_GROUP));
		/* fixme: implement <a> */
		g_hash_table_insert (dtable, "a", GINT_TO_POINTER (SP_TYPE_GROUP));
		g_hash_table_insert (dtable, "sodipodi:namedview", GINT_TO_POINTER (SP_TYPE_NAMEDVIEW));
		g_hash_table_insert (dtable, "sodipodi:guide", GINT_TO_POINTER (SP_TYPE_GUIDE));
		g_hash_table_insert (dtable, "svg", GINT_TO_POINTER (SP_TYPE_ROOT));
		g_hash_table_insert (dtable, "path", GINT_TO_POINTER (SP_TYPE_SHAPE));
		g_hash_table_insert (dtable, "rect", GINT_TO_POINTER (SP_TYPE_RECT));
		g_hash_table_insert (dtable, "ellipse", GINT_TO_POINTER (SP_TYPE_ELLIPSE));
		g_hash_table_insert (dtable, "line", GINT_TO_POINTER (SP_TYPE_LINE));
		g_hash_table_insert (dtable, "polyline", GINT_TO_POINTER (SP_TYPE_POLYLINE));
		g_hash_table_insert (dtable, "polygon", GINT_TO_POINTER (SP_TYPE_POLYGON));
		g_hash_table_insert (dtable, "text", GINT_TO_POINTER (SP_TYPE_TEXT));
		g_hash_table_insert (dtable, "image", GINT_TO_POINTER (SP_TYPE_IMAGE));
		g_hash_table_insert (dtable, "linearGradient", GINT_TO_POINTER (SP_TYPE_LINEARGRADIENT));
		g_hash_table_insert (dtable, "stop", GINT_TO_POINTER (SP_TYPE_STOP));
	}

	data = g_hash_table_lookup (dtable, name);

	if (data == NULL) return SP_TYPE_OBJECT;

	return GPOINTER_TO_INT (data);
}

