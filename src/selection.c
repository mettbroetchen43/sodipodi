#define SP_SELECTION_C

#include "desktop-handles.h"
#include "selection.h"

enum {
	CHANGED,
	LAST_SIGNAL
};

static void sp_selection_class_init (SPSelectionClass * klass);
static void sp_selection_init (SPSelection * selection);
static void sp_selection_destroy (GtkObject * object);

static void sp_selection_private_changed (SPSelection * selection);

static void sp_selection_frozen_empty (SPSelection * selection);

static GtkObjectClass * parent_class;
static guint selection_signals[LAST_SIGNAL] = { 0 };

GtkType
sp_selection_get_type (void)
{
	static GtkType selection_type = 0;

	if (!selection_type) {

		static const GtkTypeInfo selection_info = {
			"SPSelection",
			sizeof (SPSelection),
			sizeof (SPSelectionClass),
			(GtkClassInitFunc) sp_selection_class_init,
			(GtkObjectInitFunc) sp_selection_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		selection_type = gtk_type_unique (gtk_object_get_type (), &selection_info);
	}

	return selection_type;
}

static void
sp_selection_class_init (SPSelectionClass * klass)
{
	GtkObjectClass * object_class;

	object_class = (GtkObjectClass *) klass;

	parent_class = gtk_type_class (gtk_object_get_type ());

	selection_signals [CHANGED] =
		gtk_signal_new ("changed",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (SPSelectionClass, changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);
	gtk_object_class_add_signals (object_class, selection_signals, LAST_SIGNAL);

	object_class->destroy = sp_selection_destroy;

	klass->changed = sp_selection_private_changed;
}

static void
sp_selection_init (SPSelection * selection)
{
	selection->reprs = NULL;
	selection->items = NULL;
}

static void
sp_selection_destroy (GtkObject * object)
{
	SPSelection * selection;

	selection = SP_SELECTION (object);

	sp_selection_frozen_empty (selection);

	if (parent_class->destroy)
		(* parent_class->destroy) (object);
}

static void
sp_selection_private_changed (SPSelection * selection)
{
}

static void
sp_selection_selected_item_destroyed (SPItem * item, SPSelection * selection)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	selection->items = g_slist_remove (selection->items, item);

	sp_selection_changed (selection);
}

void
sp_selection_frozen_empty (SPSelection * selection)
{
	SPItem * i;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	while (selection->items) {
		i = (SPItem *) selection->items->data;
		gtk_signal_disconnect_by_data (GTK_OBJECT (i), selection);
		selection->items = g_slist_remove (selection->items, i);
	}
}

void
sp_selection_changed (SPSelection * selection)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	gtk_signal_emit (GTK_OBJECT (selection), selection_signals [CHANGED]);
}

SPSelection *
sp_selection_new (SPDesktop * desktop)
{
	SPSelection * selection;

	selection = gtk_type_new (SP_TYPE_SELECTION);
	g_assert (selection != NULL);

	selection->desktop = desktop;

	return selection;
}

gboolean
sp_selection_item_selected (SPSelection * selection, SPItem * item)
{
	g_return_val_if_fail (selection != NULL, FALSE);
	g_return_val_if_fail (SP_IS_SELECTION (selection), FALSE);
	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (SP_IS_ITEM (item), FALSE);

	return (g_slist_find (selection->items, item) != NULL);
}

gboolean
sp_selection_repr_selected (SPSelection * selection, SPRepr * repr)
{
	GSList * l;

	g_return_val_if_fail (selection != NULL, FALSE);
	g_return_val_if_fail (SP_IS_SELECTION (selection), FALSE);
	g_return_val_if_fail (repr != NULL, FALSE);

	for (l = selection->items; l != NULL; l = l->next) {
		if (((SPObject *)l->data)->repr == repr) return TRUE;
	}
	return FALSE;
}

void
sp_selection_add_item (SPSelection * selection, SPItem * item)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (!sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	selection->items = g_slist_prepend (selection->items, item);
	gtk_signal_connect (GTK_OBJECT (item), "destroy",
		GTK_SIGNAL_FUNC (sp_selection_selected_item_destroyed), selection);

	sp_selection_changed (selection);
}

void
sp_selection_add_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_add_item (selection, SP_ITEM (object));
}

void
sp_selection_set_item (SPSelection * selection, SPItem * item)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));

	sp_selection_frozen_empty (selection);

	sp_selection_add_item (selection, item);
}

void
sp_selection_set_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_set_item (selection, SP_ITEM (object));
}

void
sp_selection_remove_item (SPSelection * selection, SPItem * item)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (item != NULL);
	g_return_if_fail (SP_IS_ITEM (item));
	g_return_if_fail (sp_selection_item_selected (selection, item));

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	gtk_signal_disconnect_by_data (GTK_OBJECT (item), selection);
	selection->items = g_slist_remove (selection->items, item);

	sp_selection_changed (selection);
}

void
sp_selection_remove_repr (SPSelection * selection, SPRepr * repr)
{
	const gchar * id;
	SPObject * object;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));
	g_return_if_fail (repr != NULL);

	id = sp_repr_attr (repr, "id");
	g_return_if_fail (id != NULL);

	object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
	g_return_if_fail (object != NULL);
	g_return_if_fail (SP_IS_ITEM (object));

	sp_selection_remove_item (selection, SP_ITEM (object));
}

void
sp_selection_set_item_list (SPSelection * selection, const GSList * list)
{
	SPItem * i;
	const GSList * l;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	sp_selection_frozen_empty (selection);

	if (list != NULL) {
		for (l = list; l != NULL; l = l->next) {
			i = (SPItem *) l->data;
			if (!SP_IS_ITEM (i)) break;
			selection->items = g_slist_prepend (selection->items, i);
			gtk_signal_connect (GTK_OBJECT (i), "destroy",
				GTK_SIGNAL_FUNC (sp_selection_selected_item_destroyed), selection);
		}
	}

	sp_selection_changed (selection);
}

void
sp_selection_set_repr_list (SPSelection * selection, const GSList * list)
{
	GSList * itemlist;
	const GSList * l;
	SPRepr * repr;
	const gchar * id;
	SPObject * object;

	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	itemlist = NULL;

	for (l = list; l != NULL; l = l->next) {
		repr = (SPRepr *) l->data;
		g_return_if_fail (repr != NULL);
		id = sp_repr_attr (repr, "id");
		g_return_if_fail (id != NULL);

		object = sp_document_lookup_id (SP_DT_DOCUMENT (selection->desktop), id);
		g_return_if_fail (object != NULL);
		g_return_if_fail (SP_IS_ITEM (object));

		itemlist = g_slist_prepend (itemlist, object);
	}

	sp_selection_set_item_list (selection, itemlist);

	g_slist_free (itemlist);
}

void
sp_selection_empty (SPSelection * selection)
{
	g_return_if_fail (selection != NULL);
	g_return_if_fail (SP_IS_SELECTION (selection));

	sp_selection_frozen_empty (selection);

	sp_selection_changed (selection);
}

const GSList *
sp_selection_item_list (SPSelection * selection)
{
	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	return selection->items;
}

const GSList *
sp_selection_repr_list (SPSelection * selection)
{
	SPItem * i;
	GSList * l;

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	g_slist_free (selection->reprs);
	selection->reprs = NULL;

	for (l = selection->items; l != NULL; l = l->next) {
		i = (SPItem *) l->data;
		selection->reprs = g_slist_prepend (selection->reprs, SP_OBJECT (i)->repr);
	}

	return selection->reprs;
}

SPItem *
sp_selection_item (SPSelection * selection)
{
	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	if (selection->items == NULL) return NULL;
	if (selection->items->next != NULL) return NULL;

	return SP_ITEM (selection->items->data);
}

SPRepr *
sp_selection_repr (SPSelection * selection)
{
	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);

	if (selection->items == NULL) return NULL;
	if (selection->items->next != NULL) return NULL;

	return SP_OBJECT (selection->items->data)->repr;
}

ArtDRect *
sp_selection_bbox (SPSelection * selection, ArtDRect * bbox)
{
	SPItem * item;
	ArtDRect b;
	GSList * l;

	g_return_val_if_fail (selection != NULL, NULL);
	g_return_val_if_fail (SP_IS_SELECTION (selection), NULL);
	g_return_val_if_fail (bbox != NULL, NULL);

	if (sp_selection_is_empty (selection)) {
		bbox->x0 = bbox->y0 = bbox->x1 = bbox->y1 = 0.0;
		return bbox;
	}

	bbox->x0 = bbox->y0 = 1e18;
	bbox->x1 = bbox->y1 = -1e18;

	for (l = selection->items; l != NULL; l = l-> next) {
		item = SP_ITEM (l->data);
		sp_item_bbox (item, &b);
		if (b.x0 < bbox->x0) bbox->x0 = b.x0;
		if (b.y0 < bbox->y0) bbox->y0 = b.y0;
		if (b.x1 > bbox->x1) bbox->x1 = b.x1;
		if (b.y1 > bbox->y1) bbox->y1 = b.y1;
	}

	return bbox;
}

