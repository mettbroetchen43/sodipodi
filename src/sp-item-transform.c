#include "svg/svg.h"
#include "sp-item-transform.h"

#include <libart_lgpl/art_affine.h>

void art_affine_skew (double dst[6], double dx, double dy);

void
sp_item_rotate_rel (SPItem * item, double angle)
{
  ArtDRect  b;
  double rotate[6], s[6], t[6], u[6], v[6], newaff[6], curaff[6];
  double x,y;
  char tstr[80];

  sp_item_bbox_desktop (item,&b);
  x = b.x0 + (b.x1 - b.x0)/2;
  y = b.y0 + (b.y1 - b.y0)/2;
  art_affine_rotate (rotate,angle);
  art_affine_translate (s,x,y);
  art_affine_translate (t,-x,-y);


  tstr[79] = '\0';

  // rotate item
  sp_item_i2d_affine (item, curaff);
  art_affine_multiply (u, curaff, t);
  art_affine_multiply (v, u, rotate);
  art_affine_multiply (newaff, v, s);
  sp_item_set_i2d_affine (item, newaff);    

  //update repr
  if (sp_svg_write_affine (tstr, 79, item->affine)) {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
  } else {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
  }
}

void
sp_item_scale_rel (SPItem * item, double dx, double dy) {
  double scale[6], s[6], t[6] ,u[6] ,v[6] ,newaff[6], curaff[6];
  char tstr[80];
  double x,y;
  ArtDRect b;

  sp_item_bbox_desktop (item,&b);
  x = b.x0 + (b.x1 - b.x0)/2;
  y = b.y0 + (b.y1 - b.y0)/2;
  art_affine_scale (scale,dx,dy);
  art_affine_translate (s,x,y);
  art_affine_translate (t,-x,-y);

  tstr[79] = '\0';

  // scale item
  sp_item_i2d_affine (item, curaff);
  art_affine_multiply (u, curaff, t);
  art_affine_multiply (v, u, scale);
  art_affine_multiply (newaff, v, s);
  sp_item_set_i2d_affine (item, newaff);    
  //update repr
  if (sp_svg_write_affine (tstr, 79, item->affine)) {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
  } else {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
  }
} 

void
art_affine_skew (double dst[6], double dx, double dy)
{
  dst[0] = 1;
  dst[1] = dy;
  dst[2] = dx;
  dst[3] = 1;
  dst[4] = 0;
  dst[5] = 0;
}

void
sp_item_skew_rel (SPItem * item, double dx, double dy) {
  double skew[6], s[6], t[6] ,u[6] ,v[6] ,newaff[6], curaff[6];
  char tstr[80];
  double x,y;
  ArtDRect b;

  sp_item_bbox_desktop (item,&b);
  x = b.x0 + (b.x1 - b.x0)/2;
  y = b.y0 + (b.y1 - b.y0)/2;
  art_affine_skew (skew,dx,dy);
  art_affine_translate (s,x,y);
  art_affine_translate (t,-x,-y);

  tstr[79] = '\0';

  // skew item
  sp_item_i2d_affine (item, curaff);
  art_affine_multiply (u, curaff, t);
  art_affine_multiply (v, u, skew);
  art_affine_multiply (newaff, v, s);
  sp_item_set_i2d_affine (item, newaff);    
  //update repr
  if (sp_svg_write_affine (tstr, 79, item->affine)) {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
  } else {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
  }
} 

void
sp_item_move_rel (SPItem * item, double dx, double dy) {
  double move[6], newaff[6], curaff[6];
  char tstr[80];

  tstr[79] = '\0';

  // move item
  art_affine_translate (move, dx, dy);
  sp_item_i2d_affine (item, curaff);
  art_affine_multiply (newaff, curaff, move);
  sp_item_set_i2d_affine (item, newaff);    

  //update repr
  if (sp_svg_write_affine (tstr, 79, item->affine)) {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", tstr);
  } else {
	  sp_repr_set_attr (SP_OBJECT (item)->repr, "transform", NULL);
  }
} 

