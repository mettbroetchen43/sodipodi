#ifndef NODEPATH_H
#define NODEPATH_H

#include <libgnomeui/gnome-canvas-line.h>
#include "xml/repr.h"
#include "helper/sodipodi-ctrl.h"
#include "helper/sp-ctrlline.h"
#include "sp-path.h"
#include "desktop-handles.h"

typedef struct _SPNodePath SPNodePath;
typedef struct _SPNodeSubPath SPNodeSubPath;
typedef struct _SPPathNode SPPathNode;
typedef struct _SPPathNodeSide SPPathNodeSide;
typedef struct _SPPathLine SPPathLine;

typedef enum {
	SP_PATHNODE_NONE,
	SP_PATHNODE_CUSP,
	SP_PATHNODE_SMOOTH,
	SP_PATHNODE_SYMM
} SPPathNodeType;

struct _SPNodePath {
	SPDesktop * desktop;
	SPRepr * repr;
	SPPath * path;
	double i2d[6], d2i[6];
	gint n_bpaths;
	gint max_bpaths;
	GList * subpaths;
	GList * sel;
	ArtBpath * bpath;
	gchar * typestr;
};

struct _SPNodeSubPath {
	SPNodePath * nodepath;
	gint path_pos;
	gint n_bpaths;
	gboolean closed;
	GList * nodes;
	GList * lines;
};

struct _SPPathNodeSide {
	SPPathLine * line;
	ArtPoint cpoint;
	SPCtrl * cknot;
	SPCtrlLine * cline;
};

struct _SPPathNode {
	SPNodeSubPath * subpath;
	SPPathNodeType type;
	ArtPoint pos;
	gint subpath_pos, subpath_end;
	SPPathNodeSide prev, next;
	gboolean selected;
	SPCtrl * ctrl;
};

struct _SPPathLine {
	SPNodeSubPath * subpath;
	ArtPathcode code;
	SPPathNode * start;
	SPPathNode * end;
};

/*
 * allocators
 *
 */

SPNodeSubPath * sp_nodepath_subpath_new (void);
SPPathNode * sp_nodepath_node_new (void);
SPPathLine * sp_nodepath_line_new (void);

/*
 * Constructors
 *
 */

SPNodePath * sp_nodepath_new (SPDesktop * desktop, SPItem * item);
#if 0
SPNodeSubPath * sp_nodepath_subpath_new (
#endif

/*
 * Destructors
 *
 * These DO NOT update bpath nor sppath
 *
 */

void sp_nodepath_destroy (SPNodePath * nodepath, gboolean free_bpath, gboolean free_typestr);
void sp_nodepath_subpath_destroy (SPNodeSubPath * subpath);

/*
 * Node destructor
 *
 * this should be called ONLY if deleting entire subpath
 * it does not change node-next-start-line-end-prev-node links
 *
 */

void sp_nodepath_node_destroy (SPPathNode * node);

/*
 * Remove & destroy
 *
 * does not update bpath nor sppath
 *
 */

void sp_nodepath_subpath_remove (SPNodeSubPath * subpath);
void sp_nodepath_node_remove (SPPathNode * node);

#if 0
/* update nodepath's repr */
void sp_nodepath_flush (SPNodePath * nodepath);
#endif

void sp_nodepath_node_select (SPPathNode * node, gboolean incremental); 
void sp_nodepath_node_select_rect (SPNodePath * nodepath, ArtDRect * b, gboolean incremental);

/* possibly private functions */

void sp_node_selected_add_node (void);
void sp_node_selected_delete (void);
void sp_node_selected_break (void);
void sp_node_selected_join (void);
void sp_node_selected_set_type (SPPathNodeType type);
void sp_node_selected_set_line_type (ArtPathcode code);

#if 0
void sp_node_ensure_ctrls (SPPathNode * node);
#endif

void sp_node_update_bpath (SPPathNode * pathnode);
void sp_nodepath_update_bpath (SPNodePath * NodePath);

void sp_nodepath_update_object (SPNodePath * nodepath);

#endif
