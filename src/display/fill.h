#ifndef SP_FILL_H
#define SP_FILL_H

/*
 * Fill params
 *
 */

#include <glib.h>
#include <libart_lgpl/art_rect.h>

/* Fill types */

/* Does not use underlying color. x & y are canvas pixel coords 
 * example: gradient fill */

typedef guint32 SPFillInd (gint x, gint y, gpointer object);

/* Does use underlying color - may be much slower
 * example - desaturation fill */

typedef guint32 SPFillDep (gint x, gint y, guint32 color, gpointer object);

/* Placeholder now */

typedef guint32 SPFillBuf (gpointer data);

typedef struct {
	gint (* pre) (ArtDRect * rect, double affine, gpointer object);
	void (* post) (gint id, gpointer object);
	union {
		SPFillDep * dep;
		SPFillInd * ind;
		SPFillBuf * buf;
	} pixel;
	gpointer data;
	gchar * name;
} SPFillHandler;

typedef enum {
	SP_FILL_NONE,
	SP_FILL_COLOR,
	SP_FILL_IND,
	SP_FILL_DEP,
	SP_FILL_BUF
} SPFillType;

#if 0
typedef struct {
	SPFillType type;
	guint32 color;
	SPFillHandler handler;
} SPFillContext;
#endif

typedef struct _SPFill SPFill;

struct _SPFill {
	gint ref_count;

	SPFillType type;
	guint32 color;
	SPFillHandler handler;
};

SPFill * sp_fill_new (void);
void sp_fill_ref (SPFill * fill);
void sp_fill_unref (SPFill * fill);

SPFill * sp_fill_copy (SPFill * fill);
SPFill * sp_fill_default (void);

/* Convenience functions */

SPFill * sp_fill_new_colored (guint32 fill_color);

#endif
