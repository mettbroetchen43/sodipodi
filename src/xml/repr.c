#define SP_REPR_C

#include <gtk/gtksignal.h>
#include "repr.h"
#include "repr-private.h"

static void sp_repr_class_init (SPReprClass * klass);
static void sp_repr_init (SPRepr * repr);
static void sp_repr_destroy (GtkObject * object);

static void sp_marshal_BOOL__POINTER_POINTER (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg * args);
static gboolean sp_repr_hash_del_value (gpointer key, gpointer value, gpointer user_data);
static void sp_repr_attr_to_list (gpointer key, gpointer value, gpointer user_data);
static void sp_repr_hash_copy (gpointer key, gpointer value, gpointer new_hash);

#define dontDEBUG_REPR

#ifdef DEBUG_REPR
	gint num_repr = 0;
#endif

enum {
	CHILD_ADDED,
	REMOVE_CHILD,
	CHANGE_ATTR,
	ATTR_CHANGED,
	CHANGE_CONTENT,
	CONTENT_CHANGED,
	CHANGE_ORDER,
	ORDER_CHANGED,
	LAST_SIGNAL
};

typedef gboolean (*GtkSignal_BOOL__POINTER_POINTER) (GtkObject *object, gpointer arg1, gpointer arg2, gpointer user_data);
static GtkObjectClass * parent_class;
static guint repr_signals[LAST_SIGNAL] = { 0 };

GtkType
sp_repr_get_type (void)
{
	static GtkType repr_type = 0;
	if (!repr_type) {
		static const GtkTypeInfo repr_info = {
			"SPRepr",
			sizeof (SPRepr),
			sizeof (SPReprClass),
			(GtkClassInitFunc) sp_repr_class_init,
			(GtkObjectInitFunc) sp_repr_init,
			NULL, NULL,
			(GtkClassInitFunc) NULL
		};
		repr_type = gtk_type_unique (gtk_object_get_type (), &repr_info);
	}
	return repr_type;
}

static void
sp_repr_class_init (SPReprClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (GTK_TYPE_OBJECT);

	repr_signals [CHILD_ADDED] = gtk_signal_new ("child_added",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, child_added),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	repr_signals [REMOVE_CHILD] = gtk_signal_new ("remove_child",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, remove_child),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	/* fixme: */
	repr_signals [CHANGE_ATTR] = gtk_signal_new ("change_attr",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, change_attr),
		sp_marshal_BOOL__POINTER_POINTER,
		GTK_TYPE_BOOL, 2,
		GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	repr_signals [ATTR_CHANGED] = gtk_signal_new ("attr_changed",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, attr_changed),
		gtk_marshal_NONE__POINTER,
		GTK_TYPE_NONE, 1,
		GTK_TYPE_POINTER);
	repr_signals [CHANGE_CONTENT] = gtk_signal_new ("change_content",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, change_content),
		gtk_marshal_BOOL__POINTER,
		GTK_TYPE_BOOL, 1,
		GTK_TYPE_POINTER);
	repr_signals [CONTENT_CHANGED] = gtk_signal_new ("content_changed",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, content_changed),
		gtk_marshal_NONE__NONE,
		GTK_TYPE_NONE, 0);
	/* fixme: */
	repr_signals [CHANGE_ORDER] = gtk_signal_new ("change_order",
		GTK_RUN_LAST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, change_order),
		gtk_marshal_INT__INT,
		GTK_TYPE_BOOL, 1,
		GTK_TYPE_INT);
	repr_signals [ORDER_CHANGED] = gtk_signal_new ("order_changed",
		GTK_RUN_FIRST,
		object_class->type,
		GTK_SIGNAL_OFFSET (SPReprClass, order_changed),
		gtk_marshal_NONE__NONE,
		GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, repr_signals, LAST_SIGNAL);

	object_class->destroy = sp_repr_destroy;
}

static void
sp_repr_init (SPRepr * repr)
{
	repr->parent = NULL;
	/* repr->name = 0 */
	repr->content = NULL;
	repr->attr = g_hash_table_new (NULL, NULL);
	repr->children = NULL;
}

static void
sp_repr_destroy (GtkObject * object)
{
	SPRepr * repr;
	SPRepr * child;

	repr = (SPRepr *) object;

	/* parents have to do refcounting !!! */

	g_assert (repr->parent == NULL);

	while (repr->children) {
		child = (SPRepr *) repr->children->data;
		sp_repr_remove_child (repr, child);
		repr->children = g_list_remove (repr->children, child);
	}

	if (repr->attr) {
		g_hash_table_foreach_remove (repr->attr, sp_repr_hash_del_value, repr);
		g_hash_table_destroy (repr->attr);
	}

#ifdef DEBUG_REPR
		num_repr--;
		g_print ("num_repr = %d\n", num_repr);
#endif

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


SPRepr *
sp_repr_new (const gchar * name)
{
	SPRepr * repr;

	g_return_val_if_fail (name != NULL, NULL);

	repr = gtk_type_new (SP_TYPE_REPR);

#ifdef DEBUG_REPR
	num_repr++;
	g_print ("num_repr = %d\n", num_repr);
#endif

	repr->name = g_quark_from_string (name);

	return repr;
}

void
sp_repr_ref (SPRepr * repr)
{
	g_return_if_fail (repr != NULL);
	g_return_if_fail (SP_IS_REPR (repr));

	gtk_object_ref (GTK_OBJECT (repr));
}

void
sp_repr_unref (SPRepr * repr)
{
	g_return_if_fail (repr != NULL);
	g_return_if_fail (SP_IS_REPR (repr));

	gtk_object_unref (GTK_OBJECT (repr));
}

SPRepr * sp_repr_copy (SPRepr * repr)
{
	SPRepr * new, * child, * newchild;
	GList * list;
	gint n;

	g_return_val_if_fail (repr != NULL, NULL);

	new = sp_repr_new (g_quark_to_string (repr->name));
	g_assert (new != NULL);

	if (repr->content != NULL)
		new->content = g_strdup (repr->content);

	n = 0;
	for (list = repr->children; list != NULL; list = list->next) {
		child = (SPRepr *) list->data;
		newchild = sp_repr_copy (child);
		g_assert (newchild != NULL);
		sp_repr_add_child (new, newchild, n);
		sp_repr_unref (newchild);
		n++;
	}

	g_hash_table_foreach (repr->attr, sp_repr_hash_copy, new->attr);

	return new;
}

const gchar *
sp_repr_name (SPRepr * repr)
{
	g_return_val_if_fail (repr != NULL, NULL);

	return g_quark_to_string (repr->name);
}

gint
sp_repr_set_content (SPRepr * repr, const gchar * content)
{
	gboolean allowed;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (SP_IS_REPR (repr), FALSE);

	allowed = TRUE;

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[CHANGE_CONTENT],
		content,
		&allowed);

	if (allowed) {
		if (repr->content) g_free (repr->content);

		if (content) {
			repr->content = g_strdup (content);
		} else {
			repr->content = NULL;
		}

		gtk_signal_emit (GTK_OBJECT (repr),
			repr_signals[CONTENT_CHANGED]);
	}

	return allowed;
}

const gchar *
sp_repr_content (SPRepr * repr)
{
	g_assert (repr != NULL);

	return repr->content;
}

gboolean sp_repr_set_attr (SPRepr * repr, const gchar * key, const gchar * value)
{
	gboolean allowed;
	GQuark q;
	gchar * old_value;

	g_return_val_if_fail (repr != NULL, FALSE);
	g_return_val_if_fail (SP_IS_REPR (repr), FALSE);
	g_return_val_if_fail (key != NULL, FALSE);

	allowed = TRUE;

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[CHANGE_ATTR],
		key,
		value,
		&allowed);

	if (allowed) {
		q = g_quark_from_string (key);
		old_value = g_hash_table_lookup (repr->attr, GINT_TO_POINTER (q));

		if (value == NULL) {
			g_hash_table_remove (repr->attr, GINT_TO_POINTER (q));
		} else {
			g_hash_table_insert (repr->attr, GINT_TO_POINTER (q), g_strdup (value));
		}

		if (old_value) g_free (old_value);

		gtk_signal_emit (GTK_OBJECT (repr),
			repr_signals[ATTR_CHANGED],
			key);
	}

	return allowed;
}

const gchar * sp_repr_attr (SPRepr * repr, const gchar * key)
{
	GQuark q;

	g_return_val_if_fail (repr != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	q = g_quark_from_string (key);
	return g_hash_table_lookup (repr->attr, GINT_TO_POINTER (q));
}

SPRepr *
sp_repr_parent (SPRepr * repr)
{
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));

	return repr->parent;
}

const GList *
sp_repr_children (SPRepr * repr)
{
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));

	return repr->children;
}

void sp_repr_add_child (SPRepr * repr, SPRepr * child, gint position)
{
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));
	g_assert (child != NULL);
	g_assert (SP_IS_REPR (child));
	g_assert (child->parent == NULL);
	g_assert (position >= 0);
	g_assert (position <= g_list_length (repr->children));

	repr->children = g_list_insert (repr->children, child, position);
	child->parent = repr;
	sp_repr_ref (child);

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[CHILD_ADDED],
		child);
}

void
sp_repr_remove_child (SPRepr * repr, SPRepr * child)
{
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));
	g_assert (child != NULL);
	g_assert (SP_IS_REPR (child));
	g_assert (child->parent == repr);

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[REMOVE_CHILD],
		child);

	repr->children = g_list_remove (repr->children, child);
	child->parent = NULL;
	sp_repr_unref (child);
}

void
sp_repr_set_position_absolute (SPRepr * repr, gint pos)
{
	SPRepr * parent;
	gint allowed;
	gint nsiblings;

	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));
	g_assert (repr->parent != NULL);

	parent = repr->parent;

	nsiblings = g_list_length (parent->children);
	if ((pos < 0) || (pos >= nsiblings)) pos = nsiblings - 1;
	if (pos == sp_repr_position (repr)) return;

	allowed = TRUE;

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[CHANGE_ORDER],
		pos,
		&allowed);

	if (!allowed) return;

	parent->children = g_list_remove (parent->children, repr);
	parent->children = g_list_insert (parent->children, repr, pos);

	gtk_signal_emit (GTK_OBJECT (repr),
		repr_signals[ORDER_CHANGED]);
}


void
sp_repr_set_signal (SPRepr * repr, const gchar * name, gpointer func, gpointer data)
{
	static gboolean warned = FALSE;

	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));
	g_assert (name != NULL);
	g_assert (data != NULL);
	g_assert (GTK_IS_OBJECT (data));

	if (!warned) {
		g_warning ("implement signal setting in sp-object");
		warned = TRUE;
	}

	gtk_signal_connect_while_alive (GTK_OBJECT (repr), name,
		GTK_SIGNAL_FUNC (func), data, GTK_OBJECT (data));
}

GList *
sp_repr_attributes (SPRepr * repr)
{
	GList * listptr;

	g_assert (repr != NULL);

	listptr = NULL;

	g_hash_table_foreach (repr->attr, sp_repr_attr_to_list, &listptr);

	return listptr;
}

/* Documents - 1st step in migrating to real XML */
/* fixme: Do this somewhere, somehow The Right Way (TM) */

SPReprDoc *
sp_repr_document_new (void)
{
	SPRepr * repr, * root;

	repr = sp_repr_new ("?xml");
	sp_repr_set_attr (repr, "version", "1.0");
	sp_repr_set_attr (repr, "standalone", "no");
	sp_repr_set_attr (repr, "doctype",
		"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG August 1999//EN\"\n"
		"\"http://www.w3.org/Graphics/SVG/SVG-19990812.dtd\">");

	root = sp_repr_new ("svg");
	sp_repr_add_child (repr, root, 0);

	return (SPReprDoc *) repr;
}

void
sp_repr_document_ref (SPReprDoc * doc)
{
	sp_repr_ref ((SPRepr *) doc);
}

void
sp_repr_document_unref (SPReprDoc * doc)
{
	sp_repr_unref ((SPRepr *) doc);
}

void
sp_repr_document_set_root (SPReprDoc * doc, SPRepr * repr)
{
	SPRepr * rdoc;

	g_assert (doc != NULL);
	g_assert (SP_IS_REPR (doc));
	g_assert (repr != NULL);
	g_assert (SP_IS_REPR (repr));

	rdoc = SP_REPR (doc);

	g_assert (rdoc->children != NULL);

	sp_repr_remove_child (rdoc, SP_REPR (rdoc->children->data));
	sp_repr_add_child (rdoc, repr, 0);
}

SPReprDoc *
sp_repr_document (SPRepr * repr)
{
	g_warning ("sp_repr_document not implemented");
	return NULL;
}

SPRepr *
sp_repr_document_root (SPReprDoc * doc)
{
	SPRepr * repr;

	repr = SP_REPR (doc);
	g_return_val_if_fail (repr->children != NULL, NULL);

	return SP_REPR (repr->children->data);
}


/* Helpers */

static gboolean
sp_repr_hash_del_value (gpointer key, gpointer value, gpointer user_data)
{
	g_free (value);

	return TRUE;
}

static void
sp_repr_attr_to_list (gpointer key, gpointer value, gpointer user_data)
{
	GList ** ptr;

	ptr = (GList **) user_data;

	* ptr = g_list_prepend (* ptr, g_quark_to_string (GPOINTER_TO_INT (key)));
}

static void
sp_repr_hash_copy (gpointer key, gpointer value, gpointer new_hash)
{
	g_hash_table_insert (new_hash, key, g_strdup (value));
}

/* This is not in Gtk+ :-( */

static void sp_marshal_BOOL__POINTER_POINTER (GtkObject    *object, 
                            GtkSignalFunc func, 
                            gpointer      func_data, 
                            GtkArg       *args)
{
  GtkSignal_BOOL__POINTER_POINTER rfunc;
  gboolean  *return_val;
  return_val = GTK_RETLOC_BOOL (args[2]);
  rfunc = (GtkSignal_BOOL__POINTER_POINTER) func;
  *return_val =  (* rfunc) (object,
                      GTK_VALUE_POINTER(args[0]),
                      GTK_VALUE_POINTER(args[1]),
                             func_data);
}

