/*
 * An Algorithm for Automatically Fitting Digitized Curves
 * by Philip J. Schneider
 * from "Graphics Gems", Academic Press, 1990
 */

#define BEZIER_DEBUG

#include <math.h>
#include "bezier-utils.h"

typedef ArtPoint * BezierCurve;

/* Forward declarations */
static void GenerateBezier (ArtPoint * b, const ArtPoint * d, gint first, gint last, double * uPrime, const ArtPoint *tHat1, const ArtPoint *tHat2);
static gdouble * Reparameterize(const ArtPoint * d, gint first, gint last, gdouble * u, BezierCurve bezCurve);
static gdouble NewtonRaphsonRootFind(BezierCurve Q, ArtPoint P, gdouble u);
static void BezierII (gint degree, ArtPoint * V, gdouble t, ArtPoint *result);
static gdouble B0 (gdouble u);
static gdouble B1 (gdouble u);
static gdouble B2 (gdouble u);
static gdouble B3 (gdouble u);

static void ComputeLeftTangent (const ArtPoint * d, gint end, ArtPoint *tHat1);
static void ComputeRightTangent (const ArtPoint * d, gint end, ArtPoint *tHat2);
static void ComputeCenterTangent (const ArtPoint * d, gint center, ArtPoint *tHatCenter);
static gdouble * ChordLengthParameterize(const ArtPoint * d, gint first, gint last);
static gdouble ComputeMaxError(const ArtPoint * d, gint first, gint last, BezierCurve bezCurve, gdouble * u, gint * splitPoint);
static void V2AddII (ArtPoint a, ArtPoint b, ArtPoint *result);
static void V2ScaleIII (ArtPoint v, gdouble s, ArtPoint *result);
static void V2SubII (ArtPoint a, ArtPoint b, ArtPoint *result);
static void V2Normalize (ArtPoint * v);
static void V2Negate(ArtPoint *v);

#define V2Dot(a,b) ((a)->x * (b)->x + (a)->y * (b)->y)

#ifdef BEZIER_DEBUG
#define DOUBLE_ASSERT(x) (g_assert (((x) > -8000.0) && ((x) < 8000.0)))
#define BEZIER_ASSERT(b) { \
      g_assert ((b[0].x > -8000.0) && (b[0].x < 8000.0)); \
      g_assert ((b[0].y > -8000.0) && (b[0].y < 8000.0)); \
      g_assert ((b[1].x > -8000.0) && (b[1].x < 8000.0)); \
      g_assert ((b[1].y > -8000.0) && (b[1].y < 8000.0)); \
      g_assert ((b[2].x > -8000.0) && (b[2].x < 8000.0)); \
      g_assert ((b[2].y > -8000.0) && (b[2].y < 8000.0)); \
      g_assert ((b[3].x > -8000.0) && (b[3].x < 8000.0)); \
      g_assert ((b[3].y > -8000.0) && (b[3].y < 8000.0)); \
      }
#else
#define DOUBLE_ASSERT(x)
#define BEZIER_ASSERT(b)
#endif

#define MAXPOINTS	1000		/* The most points you can have */

/*
 *  sp_bezier_fit_cubic :
 *  	Fit a Bezier curve to a (sub)set of digitized points
 */
gboolean
sp_bezier_fit_cubic (ArtPoint       *bezier, /* assume size==4 */
                     const ArtPoint *data,
                     gint            first,
                     gint            last,
                     gdouble         error)
{
  ArtPoint    tHat1;
  ArtPoint    tHat2;
  gint        fill;

  ComputeLeftTangent (data, first, &tHat1);
  ComputeRightTangent (data, last, &tHat2);

  /* call fit-cubic function without recursion */
  fill = sp_bezier_fit_cubic_full (bezier, data, first, last,
                                   &tHat1, &tHat2,error, 1);

  return (fill != -1) ? TRUE : FALSE;
}

/*
 *  sp_bezier_fit_cubic_r:
 *     fit-cubic with recursive
 *  attribute
 *    beizer:
 *      ArtPoint array to fill result bezier paths
 *      required size==4*2^(max_depth-1)
 *    return value:
 *      block size which filled bezier paths by fit-cubic fuction
 *        where 1 block size = 4 * sizeof(ArtPoint)
 *      -1 if failed.
 */
gint
sp_bezier_fit_cubic_r (ArtPoint       *bezier,
                       const ArtPoint *data,
                       gint            first,
                       gint            last,
                       gdouble         error,
                       gint            max_depth)
{
  ArtPoint    tHat1;
  ArtPoint    tHat2;

  ComputeLeftTangent (data, first, &tHat1);
  ComputeRightTangent (data, last, &tHat2);

  /* call fit-cubic function with recursion */
  return sp_bezier_fit_cubic_full (bezier, data, first, last,
                                   &tHat1, &tHat2,error, max_depth);
}

gint
sp_bezier_fit_cubic_full (ArtPoint       *bezier,
                          const ArtPoint *data,
                          gint            first,
                          gint            last,
                          ArtPoint       *tHat1,
                          ArtPoint       *tHat2,
                          gdouble         error,
                          gint            max_depth)
{
  double	*u;		/*  Parameter values for point  */
  double	*uPrime;	/*  Improved parameter values */
  double	maxError;	/*  Maximum fitting error	 */
  int		splitPoint;	/*  Point to split point set at	 */
  int		nPts;		/*  Number of points in subset  */
  double	iterationError; /*Error below which you try iterating  */
  int		maxIterations = 4; /*  Max times to try iterating  */

  int		i;		

  iterationError = error * error;
  nPts = last - first + 1;

  /*  Use heuristic if region only has two points in it */
  if (nPts == 2) {
    double dist = hypot (data[last].x - data[first].x, data[last].y - data[first].y) / 3.0;

    bezier[0] = data[first];
    bezier[3] = data[last];
    bezier[1].x = tHat1->x * dist + bezier[0].x;
    bezier[1].y = tHat1->y * dist + bezier[0].y;
    bezier[2].x = tHat2->x * dist + bezier[3].x;
    bezier[2].y = tHat2->y * dist + bezier[3].y;

    BEZIER_ASSERT (bezier);
    return 1;
  }

  /*  Parameterize points, and attempt to fit curve */
  u = ChordLengthParameterize(data, first, last);
  GenerateBezier(bezier, data, first, last, u, tHat1, tHat2);

  /*  Find max deviation of points to fitted curve */
  maxError = ComputeMaxError(data, first, last, bezier, u, &splitPoint);

  if (maxError < error) {
    BEZIER_ASSERT (bezier);
    g_free (u);
    return 1;
  }

  /*  If error not too large, try some reparameterization  */
  /*  and iteration */
  if (maxError < iterationError) {
    for (i = 0; i < maxIterations; i++) {
      uPrime = Reparameterize(data, first, last, u, bezier);
      GenerateBezier(bezier, data, first, last, uPrime, tHat1, tHat2);
      maxError = ComputeMaxError(data, first, last, bezier, uPrime, &splitPoint);
      if (maxError < error) {
        BEZIER_ASSERT (bezier);
        g_free (u);
        g_free (uPrime);
        return 1;
      }
      g_free (u);
      u = uPrime;
    }
  }

  g_free (u);

  if (max_depth > 1)
    {
      /*
       *  Fitting failed -- split at max error point and fit recursively
       */
      ArtPoint	tHatCenter; /* Unit tangent vector at splitPoint */
      gint  depth1, depth2;

      max_depth--;

      ComputeCenterTangent(data, splitPoint, &tHatCenter);
      depth1 = sp_bezier_fit_cubic_full (bezier, data, first,  splitPoint, tHat1, &tHatCenter, error, max_depth);
      if (depth1 == -1)
        {
#ifdef BEZIER_DEBUG
          g_print ("fit_cubic[1]: fail on max_depth:%d\n", max_depth);
#endif
          return -1;
        }
      V2Negate(&tHatCenter);
      depth2 = sp_bezier_fit_cubic_full (bezier + depth1*4, data, splitPoint, last, &tHatCenter, tHat2, error, max_depth);
      if (depth2 == -1)
        {
#ifdef BEZIER_DEBUG
          g_print ("fit_cubic[2]: fail on max_depth:%d\n", max_depth);
#endif
          return -1;
        }

#ifdef BEZIER_DEBUG
      g_print("fit_cubic: success[depth: %d+%d=%d] on max_depth:%d\n",
              depth1, depth2, depth1+depth2, max_depth+1);
#endif
      return depth1 + depth2;
    }
  else
    return -1;
}

/*
 *  GenerateBezier :
 *  Use least-squares method to find Bezier control points for region.
 *
 */
static void
GenerateBezier (ArtPoint * bezier,
                const ArtPoint * data,
                gint first,
                gint last,
                double * uPrime,
                const ArtPoint *tHat1,
                const ArtPoint *tHat2)
{
    int 	i;
    ArtPoint 	A[MAXPOINTS][2];	/* Precomputed rhs for eqn	*/
    int 	nPts;			/* Number of pts in sub-curve */
    double 	C[2][2];			/* Matrix C		*/
    double 	X[2];			/* Matrix X			*/
    double 	det_C0_C1,		/* Determinants of matrices	*/
    	   	det_C0_X,
	   		det_X_C1;
    double 	alpha_l,		/* Alpha values, left and right	*/
    	   	alpha_r;
    ArtPoint 	tmp;			/* Utility variable		*/
#if 0
    BezierCurve	bezCurve;	/* RETURN bezier curve ctl pts	*/
#endif

    nPts = last - first + 1;
 
    /* Compute the A's	*/
    for (i = 0; i < nPts; i++) {
		ArtPoint       	v1, v2;
		gdouble b1, b2;

		b1 = B1(uPrime[i]);
		b2 = B2(uPrime[i]);
		v1.x = tHat1->x * b1;
		v1.y = tHat1->y * b1;
		v2.x = tHat2->x * b2;
		v2.y = tHat2->y * b2;
#if 0
		V2Scale(&v1, B1(uPrime[i]));
		V2Scale(&v2, B2(uPrime[i]));
#endif
		A[i][0] = v1;
		A[i][1] = v2;
    }

    /* Create the C and X matrices	*/
    C[0][0] = 0.0;
    C[0][1] = 0.0;
    C[1][0] = 0.0;
    C[1][1] = 0.0;
    X[0]    = 0.0;
    X[1]    = 0.0;

    for (i = 0; i < nPts; i++) {
      ArtPoint tmp1, tmp2, tmp3, tmp4;
        C[0][0] += V2Dot(&A[i][0], &A[i][0]);
		C[0][1] += V2Dot(&A[i][0], &A[i][1]);
/*					C[1][0] += V2Dot(&A[i][0], &A[i][1]);*/	
		C[1][0] = C[0][1];
		C[1][1] += V2Dot(&A[i][1], &A[i][1]);

                V2ScaleIII(data[last], B2(uPrime[i]), &tmp1);
                V2ScaleIII(data[last], B3(uPrime[i]), &tmp2);
                V2AddII(tmp1, tmp2, &tmp3);
                V2ScaleIII(data[first], B0(uPrime[i]), &tmp1);
                V2ScaleIII(data[first], B1(uPrime[i]), &tmp2);
                V2AddII(tmp2, tmp3, &tmp4);
                V2AddII(tmp1, tmp4, &tmp2);
		V2SubII(data[first + i], tmp2, &tmp);
	

	X[0] += V2Dot(&A[i][0], &tmp);
	X[1] += V2Dot(&A[i][1], &tmp);
    }

    /* Compute the determinants of C and X	*/
    det_C0_C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
    det_C0_X  = C[0][0] * X[1]    - C[0][1] * X[0];
    det_X_C1  = X[0]    * C[1][1] - X[1]    * C[0][1];

    /* Finally, derive alpha values	*/
    if (det_C0_C1 == 0.0) {
		det_C0_C1 = (C[0][0] * C[1][1]) * 10e-12;
    }
    alpha_l = det_X_C1 / det_C0_C1;
    alpha_r = det_C0_X / det_C0_C1;


    /*  If alpha negative, use the Wu/Barsky heuristic (see text) */
	/* (if alpha is 0, you get coincident control points that lead to
	 * divide by zero in any subsequent NewtonRaphsonRootFind() call. */
    if (alpha_l < 1.0e-6 || alpha_r < 1.0e-6) {
		double	dist;

		dist = hypot (data[last].x - data[first].x, data[last].y - data[first].y) / 3.0;

		bezier[0] = data[first];
		bezier[3] = data[last];
#if 0
		V2Add(&bezCurve[0], V2Scale(tHat1, dist), &bezCurve[1]);
		V2Add(&bezCurve[3], V2Scale(tHat2, dist), &bezCurve[2]);
#else
		bezier[1].x = tHat1->x * dist + bezier[0].x;
		bezier[1].y = tHat1->y * dist + bezier[0].y;
		bezier[2].x = tHat2->x * dist + bezier[3].x;
		bezier[2].y = tHat2->y * dist + bezier[3].y;
#endif
		return;
    }

    /*  First and last control points of the Bezier curve are */
    /*  positioned exactly at the first and last data points */
    /*  Control points 1 and 2 are positioned an alpha distance out */
    /*  on the tangent vectors, left and right, respectively */
    bezier[0] = data[first];
    bezier[3] = data[last];
#if 0
    V2Add(&bezCurve[0], V2Scale(tHat1, alpha_l), &bezCurve[1]);
    V2Add(&bezCurve[3], V2Scale(tHat2, alpha_r), &bezCurve[2]);
#else
    bezier[1].x = tHat1->x * alpha_l + bezier[0].x;
    bezier[1].y = tHat1->y * alpha_l + bezier[0].y;
    bezier[2].x = tHat2->x * alpha_r + bezier[3].x;
    bezier[2].y = tHat2->y * alpha_r + bezier[3].y;
#endif
    return;
}

/*
 *  Reparameterize:
 *	Given set of points and their parameterization, try to find
 *   a better parameterization.
 *
 */
static gdouble *
Reparameterize(const ArtPoint *d,
               gint            first,
               gint            last,
               gdouble        *u,
               BezierCurve     bezCurve)
{
#if 0
    Point2	*d;			/*  Array of digitized points	*/
    int		first, last;		/*  Indices defining region	*/
    double	*u;			/*  Current parameter values	*/
    BezierCurve	bezCurve;	/*  Current fitted curve	*/
#endif
    int 	nPts = last-first+1;	
    int 	i;
    gdouble	*uPrime;		/*  New parameter values	*/

    uPrime = g_new (gdouble, nPts);

    for (i = first; i <= last; i++) {
      uPrime[i-first] = NewtonRaphsonRootFind(bezCurve, d[i], u[i- first]);
    }
    return (uPrime);
}

/*
 *  NewtonRaphsonRootFind :
 *	Use Newton-Raphson iteration to find better root.
 *  Arguments:
 *      Q : Current fitted curve
 *      P : Digitized point
 *      u : Parameter value for "P"
 *  Return value:
 *      Improved u
 */
static gdouble
NewtonRaphsonRootFind(BezierCurve Q, ArtPoint P, gdouble u)
{
    double 		numerator, denominator;
    ArtPoint 		Q1[3], Q2[2];	/*  Q' and Q''			*/
    ArtPoint		Q_u, Q1_u, Q2_u; /*u evaluated at Q, Q', & Q''	*/
    double 		uPrime;		/*  Improved u			*/
    int 		i;

    DOUBLE_ASSERT (u);

    /* Compute Q(u)	*/
    BezierII(3, Q, u, &Q_u);
    
    /* Generate control vertices for Q'	*/
    for (i = 0; i <= 2; i++) {
		Q1[i].x = (Q[i+1].x - Q[i].x) * 3.0;
		Q1[i].y = (Q[i+1].y - Q[i].y) * 3.0;
    }
    
    /* Generate control vertices for Q'' */
    for (i = 0; i <= 1; i++) {
		Q2[i].x = (Q1[i+1].x - Q1[i].x) * 2.0;
		Q2[i].y = (Q1[i+1].y - Q1[i].y) * 2.0;
    }
    
    /* Compute Q'(u) and Q''(u)	*/
    BezierII(2, Q1, u, &Q1_u);
    BezierII(1, Q2, u, &Q2_u);
    
    /* Compute f(u)/f'(u) */
    numerator = (Q_u.x - P.x) * (Q1_u.x) + (Q_u.y - P.y) * (Q1_u.y);
    denominator = (Q1_u.x) * (Q1_u.x) + (Q1_u.y) * (Q1_u.y) +
		      	  (Q_u.x - P.x) * (Q2_u.x) + (Q_u.y - P.y) * (Q2_u.y);
    
    /* u = u - f(u)/f'(u) */
    uPrime = u - (numerator/denominator);
    DOUBLE_ASSERT (uPrime);
    return (uPrime);
}

/*
 *  Bezier :
 *  	Evaluate a Bezier curve at a particular parameter value
 * 
 */
static void
BezierII (gint degree, ArtPoint * V, gdouble t, ArtPoint *Q)
{
  /* ArtPoint 	Q;	        Point on curve at parameter t	*/
    int 	i, j;		
    ArtPoint 	*Vtemp;		/* Local copy of control points		*/

    /* Copy array	*/
    Vtemp = g_new (ArtPoint, degree + 1);

    for (i = 0; i <= degree; i++) {
		Vtemp[i] = V[i];
    }

    /* Triangle computation	*/
    for (i = 1; i <= degree; i++) {	
		for (j = 0; j <= degree-i; j++) {
	    	Vtemp[j].x = (1.0 - t) * Vtemp[j].x + t * Vtemp[j+1].x;
	    	Vtemp[j].y = (1.0 - t) * Vtemp[j].y + t * Vtemp[j+1].y;
		}
    }

    *Q = Vtemp[0];
    g_free (Vtemp);
}

/*
 *  B0, B1, B2, B3 :
 *	Bezier multipliers
 */
static gdouble B0 (gdouble u)
{
    double tmp = 1.0 - u;
    return (tmp * tmp * tmp);
}


static gdouble B1 (gdouble u)
{
    double tmp = 1.0 - u;
    return (3 * u * (tmp * tmp));
}

static gdouble B2 (gdouble u)
{
    double tmp = 1.0 - u;
    return (3 * u * u * tmp);
}

static gdouble B3(gdouble u)
{
    return (u * u * u);
}



/*
 * ComputeLeftTangent, ComputeRightTangent, ComputeCenterTangent :
 *Approximate unit tangents at endpoints and "center" of digitized curve
 */
static void
ComputeLeftTangent (const ArtPoint *d,
                    gint            end,
                    ArtPoint       *tHat1)
{
    V2SubII(d[end+1], d[end], tHat1);
    V2Normalize(tHat1);
}

static void
ComputeRightTangent (const ArtPoint *d,
                     gint            end,
                     ArtPoint       *tHat2)
{
    V2SubII(d[end-1], d[end], tHat2);
    V2Normalize(tHat2);
}

static void
ComputeCenterTangent(const ArtPoint *d,
                     gint            center,
                     ArtPoint       *tHatCenter)
{
    ArtPoint	V1, V2;

    V2SubII(d[center-1], d[center], &V1);
    V2SubII(d[center], d[center+1], &V2);
    tHatCenter->x = (V1.x + V2.x)/2.0;
    tHatCenter->y = (V1.y + V2.y)/2.0;
    V2Normalize(tHatCenter);
}

/*
 *  ChordLengthParameterize :
 *	Assign parameter values to digitized points 
 *	using relative distances between points.
 */
static gdouble *
ChordLengthParameterize(const ArtPoint * d, gint first, gint last)
{
    int		i;	
    gdouble	*u;			/*  Parameterization		*/

    u = g_new (gdouble, last - first + 1);

    u[0] = 0.0;
    for (i = first+1; i <= last; i++) {
		u[i-first] = u[i-first-1] +
			hypot (d[i].x - d[i-1].x, d[i].y - d[i-1].y);
    }

    for (i = first + 1; i <= last; i++) {
		u[i-first] = u[i-first] / u[last-first];
    }

#ifdef BEZIER_DEBUG
    g_assert (u[0] == 0.0 && u[last - first] == 1.0);
    for (i = 1; i<= last - first; i++) {
      g_assert (u[i] > u[i-1]);
    }
#endif
    return(u);
}




/*
 *  ComputeMaxError :
 *	Find the maximum squared distance of digitized points
 *	to fitted curve.
*/
static gdouble
ComputeMaxError(const ArtPoint   *d,
                gint              first,
                gint              last,
                const BezierCurve bezCurve,
                gdouble          *u,
                gint             *splitPoint)
{
#if 0
  Point2	*d;			/*  Array of digitized points	*/
  int		first, last;		/*  Indices defining region	*/
  BezierCurve	bezCurve;		/*  Fitted Bezier curve		*/
  double	*u;			/*  Parameterization of points	*/
  int		*splitPoint;		/*  Point of maximum error	*/
#endif
  int		i;
  double	maxDist;		/*  Maximum error		*/
  double	dist;		/*  Current error		*/
  ArtPoint	P;			/*  Point on curve		*/
  ArtPoint	v;			/*  Vector from point to curve	*/
  
  *splitPoint = (last - first + 1)/2;
  maxDist = 0.0;
  for (i = first + 1; i < last; i++) {
    BezierII(3, bezCurve, u[i-first], &P);
    V2SubII(P, d[i], &v);
    dist = v.x * v.x + v.y * v.y;
    if (dist >= maxDist) {
      maxDist = dist;
      *splitPoint = i;
    }
  }
  return (maxDist);
}

static void
V2AddII (ArtPoint a, ArtPoint b, ArtPoint *c)
{
  c->x = a.x + b.x;
  c->y = a.y + b.y;
}

static void
V2ScaleIII (ArtPoint v, gdouble s, ArtPoint *result)
{
  result->x = v.x * s;
  result->y = v.y * s;
}

static void
V2SubII (ArtPoint a, ArtPoint b, ArtPoint *c)
{
  c->x = a.x - b.x;
  c->y = a.y - b.y;
}

static void
V2Normalize (ArtPoint * v)
{
  gdouble len;

  len = hypot (v->x, v->y);
  v->x /= len;
  v->y /= len;
}

static void
V2Negate(ArtPoint *v)
{
  v->x = -v->x;
  v->y = -v->y;
}

