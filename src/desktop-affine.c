#define SP_DESKTOP_AFFINE_C

#include "document.h"
#include "desktop.h"
#include "sp-item.h"
#include "desktop-affine.h"

gdouble *
sp_desktop_w2d_affine (SPDesktop * desktop, gdouble w2d[])
{
	gint i;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (w2d != NULL, NULL);

	for (i = 0; i < 6; i++) w2d[i] = desktop->w2d[i];

	return w2d;
}

gdouble *
sp_desktop_d2w_affine (SPDesktop * desktop, gdouble d2w[])
{
	gint i;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (d2w != NULL, NULL);

	for (i = 0; i < 6; i++) d2w[i] = desktop->d2w[i];

	return d2w;
}

gdouble *
sp_desktop_w2doc_affine (SPDesktop * desktop, gdouble w2doc[])
{
	gdouble d2doc[6];

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (w2doc != NULL, NULL);

	art_affine_invert (d2doc, SP_ITEM (sp_document_root (desktop->document))->affine);
	art_affine_multiply (w2doc, desktop->w2d, d2doc);

	return w2doc;
}

gdouble *
sp_desktop_doc2w_affine (SPDesktop * desktop, gdouble doc2w[])
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (doc2w != NULL, NULL);

	art_affine_multiply (doc2w, SP_ITEM (sp_document_root (desktop->document))->affine, desktop->d2w);

	return doc2w;
}

gdouble *
sp_desktop_d2doc_affine (SPDesktop * desktop, gdouble d2doc[])
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (d2doc != NULL, NULL);

	art_affine_invert (d2doc, SP_ITEM (sp_document_root (desktop->document))->affine);

	return d2doc;
}

gdouble *
sp_desktop_doc2d_affine (SPDesktop * desktop, gdouble doc2d[])
{
	gint i;

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (doc2d != NULL, NULL);

	for (i = 0; i < 6; i++) doc2d[i] = SP_ITEM (sp_document_root (desktop->document))->affine[i];

	return doc2d;
}

ArtPoint *
sp_desktop_w2d_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, desktop->w2d);

	return p;
}

ArtPoint *
sp_desktop_d2w_xy_point (SPDesktop * desktop, ArtPoint * p, double x, double y)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, desktop->d2w);

	return p;
}

ArtPoint *
sp_desktop_w2doc_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y)
{
	gdouble d2doc[6];

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	art_affine_invert (d2doc, SP_ITEM (sp_document_root (desktop->document))->affine);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, desktop->w2d);
	art_affine_point (p, p, d2doc);

	return p;
}

ArtPoint *
sp_desktop_doc2w_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, SP_ITEM (sp_document_root (desktop->document))->affine);
	art_affine_point (p, p, desktop->d2w);

	return p;
}

ArtPoint *
sp_desktop_d2doc_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y)
{
	gdouble d2doc[6];

	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	art_affine_invert (d2doc, SP_ITEM (sp_document_root (desktop->document))->affine);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, d2doc);

	return p;
}

ArtPoint *
sp_desktop_doc2d_xy_point (SPDesktop * desktop, ArtPoint * p, gdouble x, gdouble y)
{
	g_return_val_if_fail (desktop != NULL, NULL);
	g_return_val_if_fail (SP_IS_DESKTOP (desktop), NULL);
	g_return_val_if_fail (p != NULL, NULL);

	p->x = x;
	p->y = y;
	art_affine_point (p, p, SP_ITEM (sp_document_root (desktop->document))->affine);

	return p;
}

