#define SP_MDI_CHILD_C

#include "mdi-child.h"
#include "desktop.h"

static void sp_mdi_child_class_init (SPMDIChildClass * klass);
static void sp_mdi_child_init (SPMDIChild * app_child);
static void sp_mdi_child_destroy (GtkObject * object);

static GtkWidget * sp_mdi_child_create_view (GnomeMDIChild * mdi_child, gpointer data);
static GList * sp_mdi_child_create_menus (GnomeMDIChild * mdi_child, GtkWidget * widget, gpointer data);
static gchar * sp_mdi_child_get_config_string (GnomeMDIChild * mdi_child, gpointer data);
static GtkWidget * sp_mdi_child_set_label (GnomeMDIChild * mdi_child, GtkWidget * widget, gpointer data);

static void sp_mdi_child_document_destroyed (SPDocument * document, SPMDIChild * app_child);

static GnomeMDIChildClass * parent_class = NULL;

GtkType
sp_mdi_child_get_type (void)
{
	static GtkType app_child_type = 0;

	if (!app_child_type) {
		static const GtkTypeInfo app_child_info = {
			"SPMDIChild",
			sizeof (SPMDIChild),
			sizeof (SPMDIChildClass),
			(GtkClassInitFunc) sp_mdi_child_class_init,
			(GtkObjectInitFunc) sp_mdi_child_init,
			NULL,
			NULL,
			(GtkClassInitFunc) NULL
		};

		app_child_type = gtk_type_unique (gnome_mdi_child_get_type (), &app_child_info);
	}

	return app_child_type;
}

static void
sp_mdi_child_class_init (SPMDIChildClass * klass)
{
	GtkObjectClass * object_class;
	GnomeMDIChildClass * mdi_child_class;

	object_class = (GtkObjectClass *) klass;
	mdi_child_class = (GnomeMDIChildClass *) klass;

	parent_class = gtk_type_class (gnome_mdi_child_get_type ());

	object_class->destroy = sp_mdi_child_destroy;

	mdi_child_class->create_view = sp_mdi_child_create_view;
	mdi_child_class->create_menus = sp_mdi_child_create_menus;
	mdi_child_class->get_config_string = sp_mdi_child_get_config_string;
	mdi_child_class->set_label = sp_mdi_child_set_label;
}

static void
sp_mdi_child_init (SPMDIChild * app_child)
{
	app_child->document = NULL;
}

static void
sp_mdi_child_destroy (GtkObject * object)
{
	SPMDIChild * app_child;

	app_child = SP_MDI_CHILD (object);

#if 0
	/* fixme: */
	if (app_child->document)
		sp_document_unref (app_child->document);
#endif

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Constructor */

SPMDIChild *
sp_mdi_child_new (SPDocument * document)
{
	SPMDIChild * app_child;

	g_return_val_if_fail (document != NULL, NULL);
	g_return_val_if_fail (SP_IS_DOCUMENT (document), NULL);

	app_child = (SPMDIChild *) gtk_type_new (sp_mdi_child_get_type ());

	app_child->document = document;
	/* fixme: this shouldn't happen! */
	gtk_signal_connect (GTK_OBJECT (document), "destroy",
		GTK_SIGNAL_FUNC (sp_mdi_child_document_destroyed), app_child);

	return app_child;
}

static GtkWidget *
sp_mdi_child_create_view (GnomeMDIChild * mdi_child, gpointer data)
{
	SPMDIChild * app_child;
	GtkWidget * desktop;

	app_child = SP_MDI_CHILD (mdi_child);

	desktop = (GtkWidget *) sp_desktop_new (app_child->document);

	return desktop;
}

static GList *
sp_mdi_child_create_menus (GnomeMDIChild * mdi_child, GtkWidget * widget, gpointer data)
{
	g_print ("create_menus\n");
	return NULL;
}

static gchar *
sp_mdi_child_get_config_string (GnomeMDIChild * mdi_child, gpointer data)
{
	SPMDIChild * spchild;
	SPRepr * repr;
	const gchar * docname;

	spchild = SP_MDI_CHILD (mdi_child);

	/* fixme: */
	repr = SP_ITEM (spchild->document)->repr;
	docname = sp_repr_attr (repr, "SP-DOCNAME");

	if (docname != NULL) return g_strdup (docname);
	return g_strdup ("Taara");
}

static GtkWidget *
sp_mdi_child_set_label (GnomeMDIChild * mdi_child, GtkWidget * widget, gpointer data)
{
	return parent_class->set_label (mdi_child, widget, data);
}

static void
sp_mdi_child_document_destroyed (SPDocument * document, SPMDIChild * app_child)
{
	gtk_object_destroy (GTK_OBJECT (app_child));
}

