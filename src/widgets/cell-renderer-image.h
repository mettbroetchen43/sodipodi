#ifndef __SP_CELL_RENDERER_IMAGE_H__
#define __SP_CELL_RENDERER_IMAGE_H__

/*
 * Multi image cellrenderer
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * This code is in public domain
 */

typedef struct _SPCellRendererImage SPCellRendererImage;
typedef struct _SPCellRendererImageClass SPCellRendererImageClass;

#define SP_TYPE_CELL_RENDERER_IMAGE (sp_cell_renderer_image_get_type ())
#define SP_CELL_RENDERER_IMAGE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), SP_TYPE_CELL_RENDERER_IMAGE, SPCellRendererImage))
#define SP_IS_CELL_RENDERER_IMAGE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), SP_TYPE_CELL_RENDERER_IMAGE))

#include <gtk/gtkcellrenderer.h>

struct _SPCellRendererImage {
	GtkCellRenderer renderer;

	unsigned int numimages : 4;
	unsigned int value : 4;
	unsigned int activatable : 1;
	unsigned int size : 8;

	/* Due to Gtk+ dumbness we have to use pixbufs for rendering :-( */
	/* We just pretend here we are not... */
	void **images;
};

struct _SPCellRendererImageClass {
	GtkCellRendererClass  parent_class;

	/* void (* pressed) (SPCellRendererImage *cr); */
	/* void (* released) (SPCellRendererImage *cr); */
	void (* clicked) (SPCellRendererImage *cr, const unsigned char *path);
	void (* toggled) (SPCellRendererImage *cr, const unsigned char *path);
	/* void (* selected) (SPCellRendererImage *cr); */
};


GType sp_cell_renderer_image_get_type (void);

GtkCellRenderer *sp_cell_renderer_image_new (unsigned int numimages, unsigned int size);

void sp_cell_renderer_image_set_image (SPCellRendererImage *cri, int imageid, const unsigned char *pxname);

#endif
