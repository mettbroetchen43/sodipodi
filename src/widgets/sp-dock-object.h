/*
 * sp-dock-object.h - Abstract base class for all dock related objects
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 * Copyright (C) 2003 Biswapesh Chattopadhyay <biswapesh_chatterjee@tcscal.co.in>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 */

#ifndef __SP_DOCK_OBJECT_H__
#define __SP_DOCK_OBJECT_H__

#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

/* standard macros */
#define SP_TYPE_DOCK_OBJECT             (sp_dock_object_get_type ())
#define SP_DOCK_OBJECT(obj)             (GTK_CHECK_CAST ((obj), SP_TYPE_DOCK_OBJECT, SPDockObject))
#define SP_DOCK_OBJECT_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), SP_TYPE_DOCK_OBJECT, SPDockObjectClass))
#define SP_IS_DOCK_OBJECT(obj)          (GTK_CHECK_TYPE ((obj), SP_TYPE_DOCK_OBJECT))
#define SP_IS_DOCK_OBJECT_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), SP_TYPE_DOCK_OBJECT))
#define SP_DOCK_OBJECT_GET_CLASS(obj)   (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_DOCK_OBJECT, SPDockObjectClass))

/* data types & structures */
typedef enum {
    /* the parameter is to be exported for later layout rebuilding */
    SP_DOCK_PARAM_EXPORT = 1 << G_PARAM_USER_SHIFT,
    /* the parameter must be set after adding the children objects */
    SP_DOCK_PARAM_AFTER  = 1 << (G_PARAM_USER_SHIFT + 1)
} SPDockParamFlags;

#define SP_DOCK_NAME_PROPERTY    "name"
#define SP_DOCK_MASTER_PROPERTY  "master"

typedef enum /*< prefix=SP >*/
{
    SP_DOCK_AUTOMATIC  = 1 << 0,
    SP_DOCK_ATTACHED   = 1 << 1,
    SP_DOCK_IN_REFLOW  = 1 << 2,
    SP_DOCK_IN_DETACH  = 1 << 3
} SPDockObjectFlags;

#define SP_DOCK_OBJECT_FLAGS_SHIFT 8

typedef enum { /*< prefix=SP >*/
    SP_DOCK_NONE = 0,
    SP_DOCK_TOP,
    SP_DOCK_BOTTOM,
    SP_DOCK_RIGHT,
    SP_DOCK_LEFT,
    SP_DOCK_CENTER,
    SP_DOCK_FLOATING
} SPDockPlacement;

typedef struct _SPDockObject      SPDockObject;
typedef struct _SPDockObjectClass SPDockObjectClass;
typedef struct _SPDockRequest     SPDockRequest;
typedef struct _SPDockObjectPrivate SPDockObjectPrivate;

struct _SPDockRequest {
    SPDockObject    *applicant;
    SPDockObject    *target;
    SPDockPlacement  position;
    GdkRectangle      rect;
    GValue            extra;
};

struct _SPDockObject {
    GtkContainer        container;

    SPDockObjectFlags  flags;
    gint                freeze_count;
    
    GObject            *master;
    gchar              *name;
    gchar              *long_name;
    
    gboolean            reduce_pending;
	SPDockObjectPrivate *_priv;
};

struct _SPDockObjectClass {
    GtkContainerClass parent_class;

    gboolean          is_compound;
    
    void     (* detach)          (SPDockObject    *object,
                                  gboolean          recursive);
    void     (* reduce)          (SPDockObject    *object);

    gboolean (* dock_request)    (SPDockObject    *object,
                                  gint              x,
                                  gint              y,
                                  SPDockRequest   *request);

    void     (* dock)            (SPDockObject    *object,
                                  SPDockObject    *requestor,
                                  SPDockPlacement  position,
                                  GValue           *other_data);
    
    gboolean (* reorder)         (SPDockObject    *object,
                                  SPDockObject    *child,
                                  SPDockPlacement  new_position,
                                  GValue           *other_data);

    void     (* present)         (SPDockObject    *object,
                                  SPDockObject    *child);

    gboolean (* child_placement) (SPDockObject    *object,
                                  SPDockObject    *child,
                                  SPDockPlacement *placement);

	gpointer unused[2];
};

/* additional macros */
#define SP_DOCK_OBJECT_FLAGS(obj)  (SP_DOCK_OBJECT (obj)->flags)
#define SP_DOCK_OBJECT_AUTOMATIC(obj) \
    ((SP_DOCK_OBJECT_FLAGS (obj) & SP_DOCK_AUTOMATIC) != 0)
#define SP_DOCK_OBJECT_ATTACHED(obj) \
    ((SP_DOCK_OBJECT_FLAGS (obj) & SP_DOCK_ATTACHED) != 0)
#define SP_DOCK_OBJECT_IN_REFLOW(obj) \
    ((SP_DOCK_OBJECT_FLAGS (obj) & SP_DOCK_IN_REFLOW) != 0)
#define SP_DOCK_OBJECT_IN_DETACH(obj) \
    ((SP_DOCK_OBJECT_FLAGS (obj) & SP_DOCK_IN_DETACH) != 0)

#define SP_DOCK_OBJECT_SET_FLAGS(obj,flag) \
    G_STMT_START { (SP_DOCK_OBJECT_FLAGS (obj) |= (flag)); } G_STMT_END
#define SP_DOCK_OBJECT_UNSET_FLAGS(obj,flag) \
    G_STMT_START { (SP_DOCK_OBJECT_FLAGS (obj) &= ~(flag)); } G_STMT_END
 
#define SP_DOCK_OBJECT_FROZEN(obj) (SP_DOCK_OBJECT (obj)->freeze_count > 0)


/* public interface */
 
GType          sp_dock_object_get_type          (void);

gboolean       sp_dock_object_is_compound       (SPDockObject    *object);

void           sp_dock_object_detach            (SPDockObject    *object,
                                                  gboolean          recursive);

SPDockObject *sp_dock_object_get_parent_object (SPDockObject    *object);

void           sp_dock_object_freeze            (SPDockObject    *object);
void           sp_dock_object_thaw              (SPDockObject    *object);

void           sp_dock_object_reduce            (SPDockObject    *object);

gboolean       sp_dock_object_dock_request      (SPDockObject    *object,
                                                  gint              x,
                                                  gint              y,
                                                  SPDockRequest   *request);
void           sp_dock_object_dock              (SPDockObject    *object,
                                                  SPDockObject    *requestor,
                                                  SPDockPlacement  position,
                                                  GValue           *other_data);

void           sp_dock_object_bind              (SPDockObject    *object,
                                                  GObject          *master);
void           sp_dock_object_unbind            (SPDockObject    *object);
gboolean       sp_dock_object_is_bound          (SPDockObject    *object);

gboolean       sp_dock_object_reorder           (SPDockObject    *object,
                                                  SPDockObject    *child,
                                                  SPDockPlacement  new_position,
                                                  GValue           *other_data);

void           sp_dock_object_present           (SPDockObject    *object,
                                                  SPDockObject    *child);

gboolean       sp_dock_object_child_placement   (SPDockObject    *object,
                                                  SPDockObject    *child,
                                                  SPDockPlacement *placement);

/* other types */

/* this type derives from G_TYPE_STRING and is meant to be the basic
   type for serializing object parameters which are exported
   (i.e. those that are needed for layout rebuilding) */
#define SP_TYPE_DOCK_PARAM   (sp_dock_param_get_type ())

GType sp_dock_param_get_type (void);

/* functions for setting/retrieving nick names for serializing SPDockObject types */
G_CONST_RETURN gchar *sp_dock_object_nick_from_type    (GType        type);
GType                 sp_dock_object_type_from_nick    (const gchar *nick);
GType                 sp_dock_object_set_type_for_nick (const gchar *nick,
                                                         GType        type);


/* helper macros */
#define SP_TRACE_OBJECT(object, format, args...) \
    G_STMT_START {                            \
    g_log (G_LOG_DOMAIN,                      \
	   G_LOG_LEVEL_DEBUG,                 \
           "%s:%d (%s) %s [%p %d%s:%d]: "format, \
	   __FILE__,                          \
	   __LINE__,                          \
	   __PRETTY_FUNCTION__,               \
           G_OBJECT_TYPE_NAME (object), object, \
           G_OBJECT (object)->ref_count, \
           (GTK_IS_OBJECT (object) && GTK_OBJECT_FLOATING (object)) ? "(float)" : "", \
           SP_IS_DOCK_OBJECT (object) ? SP_DOCK_OBJECT (object)->freeze_count : -1, \
	   ##args); } G_STMT_END                   
    


G_END_DECLS

#endif  /* __SP_DOCK_OBJECT_H__ */
