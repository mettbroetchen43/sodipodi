#ifndef NODEPATH_H
#define NODEPATH_H

#include "xml/repr.h"
#include "knot.h"
#include "sp-path.h"
#include "desktop-handles.h"

typedef struct _SPNodePath SPNodePath;
typedef struct _SPNodeSubPath SPNodeSubPath;
typedef struct _SPPathNode SPPathNode;

typedef enum {
	SP_PATHNODE_NONE,
	SP_PATHNODE_CUSP,
	SP_PATHNODE_SMOOTH,
	SP_PATHNODE_SYMM
} SPPathNodeType;

struct _SPNodePath {
	SPDesktop * desktop;
	SPPath * path;
	GSList * subpaths;
	GSList * selected;
	double i2d[6], d2i[6];
};

struct _SPNodeSubPath {
	SPNodePath * nodepath;
	gboolean closed;
	GSList * nodes;
	SPPathNode * first;
	SPPathNode * last;
};

typedef struct {
	SPPathNode * other;
	ArtPoint pos;
	SPKnot * knot;
	SPCanvasItem * line;
} SPPathNodeSide;

struct _SPPathNode {
	SPNodeSubPath * subpath;
	guint type : 4;
	guint code : 4;
	guint selected : 1;
	ArtPoint pos;
	SPKnot * knot;
	SPPathNodeSide n;
	SPPathNodeSide p;
};

SPNodePath * sp_nodepath_new (SPDesktop * desktop, SPItem * item);
void sp_nodepath_destroy (SPNodePath * nodepath);

void sp_nodepath_select_rect (SPNodePath * nodepath, ArtDRect * b, gboolean incremental);
gboolean node_key (GdkEvent * event);

/* possibly private functions */

void sp_node_selected_add_node (void);
void sp_node_selected_delete (void);
void sp_node_selected_break (void);
void sp_node_selected_join (void);
void sp_node_selected_set_type (SPPathNodeType type);
void sp_node_selected_set_line_type (ArtPathcode code);

#endif
