// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      fvgeom.c                                                          */
/*                                                                          */
/* Purpose:   geometry related data for a general element in the box scheme */
/*                                                                          */
/* Author:	  Peter Bastian                                                                                         */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   07.05.96 begin, ug version 3.0								*/
/*            06.08.96 modification and extension (Henrik Rentz-Reichert)   */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "devices.h"
#include "enrol.h"
#include "compiler.h"
#include "misc.h"
#include "gm.h"
#include "evm.h"
#include "ugenv.h"
#include "ugm.h"
#include "algebra.h"
#include "shapes.h"
#include "cmdint.h"
#include "commands.h"
#include "helpmsg.h"
#include "debug.h"
#include "general.h"

#include "fvgeom.h"

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#ifdef __THREEDIM__

#define OneSixth 0.166666666666666667
#define C0 FVG_GCO(geo,0)
#define C1 FVG_GCO(geo,1)
#define C2 FVG_GCO(geo,2)
#define C3 FVG_GCO(geo,3)
#define C4 FVG_GCO(geo,4)
#define C5 FVG_GCO(geo,5)
#define C6 FVG_GCO(geo,6)
#define C7 FVG_GCO(geo,7)
#define E0 FVG_GEM(geo,0)
#define E1 FVG_GEM(geo,1)
#define E2 FVG_GEM(geo,2)
#define E3 FVG_GEM(geo,3)
#define E4 FVG_GEM(geo,4)
#define E5 FVG_GEM(geo,5)
#define E6 FVG_GEM(geo,6)
#define E7 FVG_GEM(geo,7)
#define E8 FVG_GEM(geo,8)
#define E9 FVG_GEM(geo,9)
#define E10 FVG_GEM(geo,10)
#define E11 FVG_GEM(geo,11)
#define S0 FVG_GSM(geo,0)
#define S1 FVG_GSM(geo,1)
#define S2 FVG_GSM(geo,2)
#define S3 FVG_GSM(geo,3)
#define S4 FVG_GSM(geo,4)
#define S5 FVG_GSM(geo,5)
#define S6 FVG_GSM(geo,6)
#define S7 FVG_GSM(geo,7)
#define SS FVG_GCM(geo)

/* Normal vectors multiplied with area, orientation is right handed ! */
#define Q_AREA_3D(x0, x1, x2, x3, n)   {DOUBLE_VECTOR a, b;                     \
                                        V3_SUBTRACT(x2,x0,a);           \
                                        V3_SUBTRACT(x3,x1,b);           \
                                        V3_VECTOR_PRODUCT(a,b,n);       \
                                        V3_SCALE(0.5,n);}

static DOUBLE Param[2][4][2] = {
  {{0.4166666666666,0.416666666666666},
   {0.6666666666666,0.416666666666666},
   {0.4166666666666,0.666666666666666},{0.0,0.0}},
  {{0.25,0.25},{0.75,0.25},{0.75,0.25},{0.25,0.75}}
};

#define PARAMETER(n,i,j) Param[n-3][i][j]

#endif

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

typedef struct {
  DOUBLE_VECTOR co[MAXNC];                      /* points in local space, corners       */
  DOUBLE_VECTOR em[MAXE];                       /* points in local space, edge midpoints*/
  DOUBLE_VECTOR sm[MAXS];                       /* points in local space, side midpoints*/
  DOUBLE_VECTOR cm;                                     /* points in local space, center        */
  DOUBLE_VECTOR ip[MAXF];                       /* sub control volume faces				*/
  DOUBLE_VECTOR bip[MAX_SIDES_OF_ELEM][MAX_CORNERS_OF_SIDE];
  /* boundary faces						*/

} LOCAL_DOUBLES;                                /* local geometry data for a general element	*/

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static LOCAL_DOUBLES LocalCoords[TAGS];

/* data for CVS */
RCSID("$Header$",UG_RCS_STRING)

/****************************************************************************/
/*																			*/
/* definition of functions													*/
/*																			*/
/****************************************************************************/

#ifdef __THREEDIM__

/* area of quadrilateral in 3D space */
static DOUBLE F_q (DOUBLE *x0, DOUBLE *x1, DOUBLE *x2, DOUBLE *x3)
{
  DOUBLE_VECTOR n;

  Q_AREA_3D (x0, x1, x2, x3, n);

  return(sqrt(V3_SCAL_PROD(n,n)));
}

/* volume computations, orientation is same as in general element definition ! */
static DOUBLE V_te (DOUBLE *x0, DOUBLE *x1, DOUBLE *x2, DOUBLE *x3)
{
  DOUBLE_VECTOR a, b, h, n;

  V3_SUBTRACT(x1,x0,a);
  V3_SUBTRACT(x2,x0,b);
  V3_SUBTRACT(x3,x0,h);
  V3_VECTOR_PRODUCT(a,b,n);

  return(OneSixth*V3_SCAL_PROD(n,h));
}

static DOUBLE V_py (DOUBLE *x0, DOUBLE *x1, DOUBLE *x2, DOUBLE *x3, DOUBLE *x4)
{
  DOUBLE_VECTOR a,b,h,n;

  V3_SUBTRACT(x2,x0,a);
  V3_SUBTRACT(x3,x1,b);
  V3_SUBTRACT(x4,x0,h);
  V3_VECTOR_PRODUCT(a,b,n);

  return(OneSixth*V3_SCAL_PROD(n,h));
}

static DOUBLE V_pr (DOUBLE *x0, DOUBLE *x1, DOUBLE *x2, DOUBLE *x3, DOUBLE *x4, DOUBLE *x5)
{
  DOUBLE_VECTOR a,b,c,d,e,m,n;

  V3_SUBTRACT(x4,x0,a);
  V3_SUBTRACT(x1,x3,b);
  V3_SUBTRACT(x1,x0,c);
  V3_SUBTRACT(x2,x0,d);
  V3_SUBTRACT(x5,x0,e);
  a[0] = x4[0]-x0[0]; a[1] = x4[1]-x0[1]; a[2] = x4[2]-x0[2];
  b[0] = x1[0]-x3[0]; b[1] = x1[1]-x3[1]; b[2] = x1[2]-x3[2];
  c[0] = x1[0]-x0[0]; c[1] = x1[1]-x0[1]; c[2] = x1[2]-x0[2];
  d[0] = x2[0]-x0[0]; d[1] = x2[1]-x0[1]; d[2] = x2[2]-x0[2];
  e[0] = x5[0]-x0[0]; e[1] = x5[1]-x0[1]; e[2] = x5[2]-x0[2];

  V3_VECTOR_PRODUCT(a,b,m);
  V3_VECTOR_PRODUCT(c,d,n);
  V3_ADD(n,m,n);

  return(OneSixth*V3_SCAL_PROD(n,e));
}

static DOUBLE V_he (DOUBLE *x0, DOUBLE *x1, DOUBLE *x2, DOUBLE *x3, DOUBLE *x4, DOUBLE *x5, DOUBLE *x6, DOUBLE *x7)
{
  return(V_pr(x0,x1,x2,x4,x5,x6)+V_pr(x0,x2,x3,x4,x6,x7));
}
#endif


/****************************************************************************/
/*D
   EvaluateFVGeometry - compute geometry information for given element

   SYNOPSIS:
   INT EvaluateFVGeometry (ELEMENT *e, FVElementGeometry *eg);

   PARAMETERS:
   .  e - given element
   .  eg - data structure to be filled

   DESCRIPTION:
   This routine fills the given data structure with geometry related
   values

   RETURN VALUES:
   0 when o.k.

   1 if an error occured.
   D*/
/****************************************************************************/

INT EvaluateFVGeometry (const ELEMENT *e, FVElementGeometry *geo)
{
  INT i,j,k,l,n,coe,eoe;
  VERTEX *v;
  LOCAL_DOUBLES *lc;
  SubControlVolume *scv;
  SubControlVolumeFace *scvf;
  BoundaryFace *bf;
  DOUBLE scale;
  DOUBLE_VECTOR s;
#       ifdef __TWODIM__
  DOUBLE_VECTOR x;
  INT iminus1;
#       endif
#       ifdef __THREEDIM__
  INT r,kminus1;
#       endif

  /* general info */
  FVG_ELEM(geo)   = e;
  FVG_TAG(geo)    = TAG(e);
  FVG_NSCV(geo)   = coe = CORNERS_OF_ELEM(e);
  FVG_NSCVF(geo)  = eoe = EDGES_OF_ELEM(e);
  FVG_NSCVBF(geo) = 0;       /* initially */

  lc = LocalCoords+TAG(e);

  /* corners */
  for (i=0; i<coe; i++)
  {
    v = MYVERTEX(CORNER(e,i));
    V_DIM_COPY(CVECT(v),FVG_GCO(geo,i));
    V_DIM_COPY(lc->co[i],FVG_LCO(geo,i));
  }

  /* edge midpoints */
  for (k=0; k<eoe; k++)
  {
    i = CORNER_OF_EDGE(e,k,0);
    j = CORNER_OF_EDGE(e,k,1);
    V_DIM_COPY(lc->em[k],FVG_LEM(geo,k));
    V_DIM_AVG2(FVG_GCO(geo,i),FVG_GCO(geo,j),FVG_GEM(geo,k));
  }

  /* side midpoints */
  for (k=0; k<SIDES_OF_ELEM(e); k++)
  {
    scale = 1.0/((DOUBLE)CORNERS_OF_SIDE(e,k));

    V_DIM_CLEAR(s);
    for (l=0; l<CORNERS_OF_SIDE(e,k); l++)
      V_DIM_ADD1(FVG_GCO(geo,CORNER_OF_SIDE(e,k,l)),s);
    V_DIM_SCALE(scale,s);
    V_DIM_COPY(s,FVG_GSM(geo,k));

    V_DIM_COPY(lc->sm[k],FVG_LSM(geo,k));
  }

  /* center of mass */
  scale = 1.0/((DOUBLE)coe);
  V_DIM_CLEAR(s);
  for (l=0; l<coe; l++)
    V_DIM_ADD1(FVG_GCO(geo,l),s);
  V_DIM_SCALE(scale,s);
  V_DIM_COPY(s,FVG_GCM(geo));

  V_DIM_COPY(lc->cm,FVG_LCM(geo));

  /* sub control volumes */
  for (i=0; i<coe; i++)
  {
    scv = FVG_SCV(geo,i);
    SCV_CO(scv)     = i;
    V_DIM_COPY(FVG_GCO(geo,i),SCV_GCO(scv));
    SCV_NDPROP(scv) =  NPROP(CORNER(e,i));
  }
  switch (TAG(e))
  {
#               ifdef __TWODIM__
  case TRIANGLE :
  case QUADRILATERAL :
    for (i=0; i<coe; i++)
    {
      iminus1 = (i+coe-1)%coe;
      scv = FVG_SCV(geo,i);
      SCV_VOL(scv) = qarea(FVG_GCO(geo,i)[0],FVG_GCO(geo,i)[1],
                           FVG_GEM(geo,i)[0],FVG_GEM(geo,i)[1],
                           FVG_GCM(geo)[0],FVG_GCM(geo)[1],
                           FVG_GEM(geo,iminus1)[0],FVG_GEM(geo,iminus1)[1]);
    }
    break;
#               endif

#               ifdef __THREEDIM__
  case TETRAHEDRON :
    SCV_VOL(FVG_SCV(geo,0)) =
      SCV_VOL(FVG_SCV(geo,1)) =
        SCV_VOL(FVG_SCV(geo,2)) =
          SCV_VOL(FVG_SCV(geo,3)) = 0.25*V_te(C0,C1,C2,C3);
    break;
  case PYRAMID :
    SCV_VOL(FVG_SCV(geo,0)) = V_he(C0,E0,S0,E3,E4,S1,SS,S4);
    SCV_VOL(FVG_SCV(geo,1)) = V_he(C1,E1,S0,E0,E5,S2,SS,S1);
    SCV_VOL(FVG_SCV(geo,2)) = V_he(C2,E2,S0,E1,E6,S3,SS,S2);
    SCV_VOL(FVG_SCV(geo,3)) = V_he(C3,E3,S0,E2,E7,S4,SS,S3);
    SCV_VOL(FVG_SCV(geo,4)) = V_py(C0,C1,C2,C3,C4)
                              -SCV_VOL(FVG_SCV(geo,0))
                              -SCV_VOL(FVG_SCV(geo,1))
                              -SCV_VOL(FVG_SCV(geo,2))
                              -SCV_VOL(FVG_SCV(geo,3));
    break;
  case PRISM :
    SCV_VOL(FVG_SCV(geo,0)) = V_he(C0,E0,S0,E2,E3,S1,SS,S3);
    SCV_VOL(FVG_SCV(geo,1)) = V_he(C1,E1,S0,E0,E4,S2,SS,S1);
    SCV_VOL(FVG_SCV(geo,2)) = V_he(C2,E2,S0,E1,E5,S3,SS,S2);
    SCV_VOL(FVG_SCV(geo,3)) = V_he(E3,S1,SS,S3,C3,E6,S4,E8);
    SCV_VOL(FVG_SCV(geo,4)) = V_he(E4,S2,SS,S1,C4,E7,S4,E6);
    SCV_VOL(FVG_SCV(geo,5)) = V_he(E5,S3,SS,S2,C5,E8,S4,E7);
    break;
  case HEXAHEDRON :
    SCV_VOL(FVG_SCV(geo,0)) = V_he(C0,E0,S0,E3,E4,S1,SS,S4);
    SCV_VOL(FVG_SCV(geo,1)) = V_he(C1,E1,S0,E0,E5,S2,SS,S1);
    SCV_VOL(FVG_SCV(geo,2)) = V_he(C2,E2,S0,E1,E6,S3,SS,S2);
    SCV_VOL(FVG_SCV(geo,3)) = V_he(C3,E3,S0,E2,E7,S4,SS,S3);
    SCV_VOL(FVG_SCV(geo,4)) = V_he(E4,S1,SS,S4,C4,E8,S5,E11);
    SCV_VOL(FVG_SCV(geo,5)) = V_he(E5,S2,SS,S1,C5,E9,S5,E8);
    SCV_VOL(FVG_SCV(geo,6)) = V_he(E6,S3,SS,S2,C6,E10,S5,E9);
    SCV_VOL(FVG_SCV(geo,7)) = V_he(E7,S4,SS,S3,C7,E11,S5,E10);
    break;
#                       endif

  default :
    PrintErrorMessage('E',"EvaluateFVGeometry","unknown element");
    return(__LINE__);
  }

  IFDEBUG(numerics,0)
  for (k=0; k<coe; k++)               /* check sign */
    if (SCV_VOL(FVG_SCV(geo,k))<0.0)
      UserWriteF("w: scv negative e=%5d k=%1d v=%10.4lg\n",ID(e),k,SCV_VOL(FVG_SCV(geo,k)));
  ENDDEBUG

  /* sub control volume faces */
  for (k=0; k<eoe; k++)
  {
    i = CORNER_OF_EDGE(e,k,0); j = CORNER_OF_EDGE(e,k,1);
    scvf = FVG_SCVF(geo,k);
    SCVF_FROM(scvf) = i; SCVF_TO(scvf) = j;

    V_DIM_COPY(lc->ip[k],SCVF_LIP(scvf));

#               ifdef __TWODIM__
    V2_AVG2(FVG_GEM(geo,i),FVG_GCM(geo),SCVF_GIP(scvf));
    V2_SUBTRACT(FVG_GCM(geo),FVG_GEM(geo,i),s);
    V2_NORMAL(s,SCVF_NORMAL(scvf));
#               endif

#               ifdef __THREEDIM__
    r = SIDE_WITH_EDGE(e,k,0); l = SIDE_WITH_EDGE(e,k,1);             /* REQUIRES CORRECT ORIENTATION ! */
    V_DIM_AVG4(FVG_GEM(geo,k),FVG_GSM(geo,r),FVG_GCM(geo),FVG_GSM(geo,l),SCVF_GIP(scvf));
    Q_AREA_3D(FVG_GEM(geo,k),FVG_GSM(geo,r),FVG_GCM(geo),FVG_GSM(geo,l),SCVF_NORMAL(scvf));
#               endif

    IFDEBUG(numerics,0)
    /* check sign */
    V_DIM_SUBTRACT(FVG_GCO(geo,j),FVG_GCO(geo,i),s);
    if (V_DIM_SCAL_PROD(s,SCVF_NORMAL(scvf))<0.0)
      UserWriteF("W: scvf normal w. edge negative e=%5d i=%2d j=%2d\n",
                 ID(e),i,j);
    ENDDEBUG
  }

  /* boundary integration points (this is in parameter space !) */
  if (OBJT(e)==BEOBJ)
    for (i=0; i<SIDES_OF_ELEM(e); i++)
    {
      if (INNER_SIDE(e,i)) continue;
      /* interpolate in parameter and local space on side: center of mass */
      n = CORNERS_OF_SIDE(e,i);

      /* fill boundary face */
      for (k=0; k<n; k++)
      {
        bf = FVG_SCVBF(geo,FVG_NSCVBF(geo));

        SCVBF_FROM(bf) = CORNER_OF_SIDE(e,i,k);
        SCVBF_SIDE(bf) = i;

        /* bip coord in local space */
        V_DIM_COPY(lc->bip[i][k],SCVBF_LIP(bf));

#                               ifdef __TWODIM__
        /* normal, assumes correct numbering of edges relative to corners (of side) !! */
        if(k==0) V2_SUBTRACT(FVG_GEM(geo,i),FVG_GCO(geo,CORNER_OF_SIDE(e,i,0)),x);
        if(k==1) V2_SUBTRACT(FVG_GCO(geo,CORNER_OF_SIDE(e,i,1)),FVG_GEM(geo,i),x);
        V2_EUKLIDNORM(x,SCVBF_AREA(bf));
        SCVBF_PARAM(bf,0) = 0.25 + k * 0.5;
        V2_NORMAL(x,SCVBF_NORMAL(bf));
#                               endif

#                               ifdef __THREEDIM__
        kminus1 = (k+n-1)%n;
        /* normal, assumes correct numbering of edges relative to corners (of side) !! */
        Q_AREA_3D(      FVG_GCO(geo,CORNER_OF_SIDE(e,i,k)),
                        FVG_GEM(geo,EDGE_OF_SIDE(e,i,k)),
                        FVG_GSM(geo,i),
                        FVG_GEM(geo,EDGE_OF_SIDE(e,i,kminus1)),
                        SCVBF_NORMAL(bf));
        SCVBF_AREA(bf) = F_q(FVG_GCO(geo,CORNER_OF_SIDE(e,i,k)),
                             FVG_GEM(geo,EDGE_OF_SIDE(e,i,k)),
                             FVG_GSM(geo,i),
                             FVG_GEM(geo,EDGE_OF_SIDE(e,i,kminus1)));
        SCVBF_PARAM(bf,0) = PARAMETER(n,k,0);
        SCVBF_PARAM(bf,1) = PARAMETER(n,k,1);
#                               endif

        FVG_NSCVBF(geo)++;
      }
    }

  return (0);
}

/****************************************************************************/
/*
   SideIsCut - return YES if side is cut together with global coordinates of cutting point

   SYNOPSIS:
   INT SideIsCut (INT tag,  const DOUBLE_VECTOR *x, const DOUBLE_VECTOR ip, const DOUBLE_VECTOR vel, INT side, DOUBLE_VECTOR y)

   PARAMETERS:
   .  tag - element type
   .  x - global corner coordinates
   .  ip - global coordinates of integration point
   .  vel - velocity vector at integration point
   .  side - examine this side
   .  y - resulting cutting point iff

   DESCRIPTION:
   This function returns YES if the side is cut together with global coordinates of cutting point.
   NO is returned else.

   RETURN VALUES:
   0 when o.k.
 */

static INT SideIsCut (INT tag,  const DOUBLE_VECTOR *x, const DOUBLE_VECTOR ip, const DOUBLE_VECTOR vel, INT side, DOUBLE_VECTOR y)
{
#       ifdef __TWODIM__
  DOUBLE_VECTOR v,r,coeff,M[DIM],MI[DIM];
  DOUBLE det;
  INT next;

  /* we search the cutting point of line xs+c0*(xn-xs) with ip-c1*vel by solving the system
                                             T
          (xn0-xs0  xn1-xs1)    (c0)   (ip0-xs0)
          (				 )    (  ) = (		)
          (vel0     vel1   )    (c1)   (ip1-xs1)
   */
  next = (side+1)%CORNERS_OF_TAG(tag);                                                  /* succ of node[side] */
  V2_SUBTRACT(x[next],x[side],v);                                                               /* vector from xs to xn */
  V2_COPY(v,M[0]);                                                                                              /* transposed coefficient matrix for cut of lines */
  V2_COPY(vel,M[1]);
  M2_INVERT(M,MI,det);                                                                                  /* inverse */
  if (det==0.0)
    return (NO);                                                                                                /* lines are parallel */

  V2_SUBTRACT(ip,x[side],r);                                                                            /* right hand side */
  MT2_TIMES_V2(MI,r,coeff);                                                                             /* solve for coefficients */
  if (coeff[1]>0.0)                                                                                             /* we search an upwind point */
    if ((-SMALL_C<coeff[0]) && (coeff[0]<1.0+SMALL_C))                          /* local param on side in (0,1)? */
    {
      V2_LINCOMB(1.0,x[side],coeff[0],v,y);                                             /* global cordinates of cutting point */
      return (YES);
    }
#       endif


#       ifdef __THREEDIM__
  DOUBLE_VECTOR v1,v2,r,coeff,M[DIM],MI[DIM];
  DOUBLE det,sum;
  INT a,b,c;

  /* we search the cutting point of plane xa+c0*(xb-xa)+c1*(xc-xa) with ip-c2*vel by solving the system
                                                             T
          (xb0-xa0  xb1-xa1  xb2-xa2)    (c0)   (ip0-xs0)
          (xc0-xa0  xc1-xa1  xb2-xa2)    (c1)   (ip1-xs1)
          (vel0     vel1     vel2   )    (c2)   (ip2-xs2)
   */

  /* check if in traingle 0,1,2 */
  a = CORNER_OF_SIDE_TAG(tag,side,0);                                                           /* corner 0 of side */
  b = CORNER_OF_SIDE_TAG(tag,side,1);                                                           /* corner 1 of side */
  c = CORNER_OF_SIDE_TAG(tag,side,2);                                                           /* corner 2 of side */
  V3_SUBTRACT(x[b],x[a],v1);                                                                            /* vector from xa to xb */
  V3_SUBTRACT(x[c],x[a],v2);                                                                            /* vector from xa to xb */
  V3_COPY(v1,M[0]);                                                                                             /* transposed coefficient matrix for cut of lines */
  V3_COPY(v2,M[1]);
  V3_COPY(vel,M[2]);
  M3_INVERT(M,MI,det);                                                                                  /* inverse */
  if (det==0.0)
    return (NO);                                                                                                /* lines is parallel to plane */

  V3_SUBTRACT(ip,x[side],r);                                                                            /* right hand side */
  MT3_TIMES_V3(MI,r,coeff);                                                                             /* solve for coefficients */
  if (coeff[2]>0.0)                                                                                             /* we search an upwind point */
  {
    sum = coeff[0] + coeff[1];
    if ((coeff[0]>-SMALL_C) && (coeff[1]>-SMALL_C))                             /* inside plane sector b,a,c? */
    {
      if (sum<1.0+SMALL_C)                                                                              /* inside triangle a,b,c? */
      {
        V3_LINCOMB(1.0,x[a],coeff[0],v1,y);                                                     /* global cordinates of cutting point */
        V3_LINCOMB(1.0,y,       coeff[1],v2,y);
        return (YES);
      }
    }
    else if (CORNERS_OF_SIDE_TAG(tag,side)==4)                                          /* is side a quadrilateral? */
    {
      /* check if in triangle 3,0,2 */
      a = CORNER_OF_SIDE_TAG(tag,side,3);                                                                       /* corner 0 of side */
      b = CORNER_OF_SIDE_TAG(tag,side,0);                                                                       /* corner 1 of side */
      c = CORNER_OF_SIDE_TAG(tag,side,2);                                                                       /* corner 2 of side */
      V3_SUBTRACT(x[b],x[a],v1);                                                                                        /* vector from xa to xb */
      V3_SUBTRACT(x[c],x[a],v2);                                                                                        /* vector from xa to xb */
      V3_COPY(v1,M[0]);                                                                                                         /* transposed coefficient matrix for cut of lines */
      V3_COPY(v2,M[1]);
      V3_COPY(vel,M[2]);
      M3_INVERT(M,MI,det);                                                                                              /* inverse */
      if (det==0.0)
        return (NO);                                                                                                            /* lines is parallel to plane */

      V3_SUBTRACT(ip,x[side],r);                                                                                        /* right hand side */
      MT3_TIMES_V3(MI,r,coeff);                                                                                         /* solve for coefficients */
      if (coeff[2]>0.0)                                                                                                         /* we search an upwind point */
      {
        sum = coeff[0] + coeff[1];
        if ((coeff[0]>-SMALL_C) && (coeff[1]>-SMALL_C))                                         /* inside plane sector b,a,c? */
        {
          if (sum<1.0+SMALL_C)                                                                                          /* inside triangle a,b,c? */
          {
            V3_LINCOMB(1.0,x[a],coeff[0],v1,y);                                                                 /* global cordinates of cutting point */
            V3_LINCOMB(1.0,y,       coeff[1],v2,y);
            return (YES);
          }
        }
        else
          PrintErrorMessage('W',"SideIsCut","Huh???");
      }
    }
  }
#       endif

  return (NO);
}

/****************************************************************************/
/*
   GetNodeNextToCut - return corner number of node next to upstream point on element boundary

   SYNOPSIS:
   INT GetNodeNextToCut (INT tag, const DOUBLE_VECTOR *x, const DOUBLE_VECTOR ip, const DOUBLE_VECTOR vel, INT *corn)

   PARAMETERS:
   .  tag - element type
   .  x - global corner coordinates
   .  ip - global coordinates of integration point
   .  vel - velocity vector at integration point
   .  corn - resulting corner

   DESCRIPTION:
   This function computes the corner number of the node next to the upstream point on the element boundary.

   RETURN VALUES:
   0 when o.k.
 */
/****************************************************************************/

static INT GetNodeNextToCut (INT tag, const DOUBLE_VECTOR *x, const DOUBLE_VECTOR ip, const DOUBLE_VECTOR vel, INT *corn)
{
  DOUBLE_VECTOR y,d;
  DOUBLE min,l;
  INT i,sd,co,next_co;

  /* find upwind point on element side */
  for (sd=0; sd<SIDES_OF_TAG(tag); sd++)
    if (SideIsCut(tag,x,ip,vel,sd,y))
      break;

  /* determine next node */
  min = MAX_D;
  for (i=0; i<CORNERS_OF_SIDE_TAG(tag,sd); i++)
  {
    co = CORNER_OF_SIDE_TAG(tag,sd,i);
    V_DIM_SUBTRACT(y,x[co],d)
    V_DIM_SCALAR_PRODUCT(d,d,l);

    if (l<min)
    {
      min = l;
      next_co = co;
    }
  }

  *corn = next_co;

  return (0);
}

INT Intersect2d (INT nco, const DOUBLE_VECTOR *x, const DOUBLE_VECTOR vel, const DOUBLE_VECTOR pt,
                 INT *Side, DOUBLE lambda[DIM_OF_BND])
{
  DOUBLE_VECTOR v,r,coeff,M[DIM],MI[DIM];
  DOUBLE det;
  INT side,next;

  for (side=0; side<nco; side++)
  {
    /* we search the cutting point of line xs+c0*(xn-xs) with pt-c1*vel by solving the system
                                               T
            (xn0-xs0  xn1-xs1)    (c0)   (pt0-xs0)
            (				 )    (  ) = (		)
            (vel0     vel1   )    (c1)   (pt1-xs1)
     */
    next = (side+1)%nco;                                                                                        /* succ of node[side] */
    V2_SUBTRACT(x[next],x[side],v);                                                                     /* vector from xs to xn */
    V2_COPY(v,M[0]);                                                                                                    /* transposed coefficient matrix for cut of lines */
    V2_COPY(vel,M[1]);
    M2_INVERT(M,MI,det);                                                                                        /* inverse */
    if (det==0.0)
      continue;                                                                                                                 /* lines are parallel */

    V2_SUBTRACT(pt,x[side],r);                                                                                  /* right hand side */
    MT2_TIMES_V2(MI,r,coeff);                                                                                   /* solve for coefficients */
    if (coeff[1]>0.0)                                                                                                   /* we search an upwind point */
      if ((-SMALL_C<coeff[0]) && (coeff[0]<1.0+SMALL_C))                                /* local param on side in (0,1)? */
      {
        *lambda = coeff[0];                                                                                             /* local param on side */
        *Side   = side;
        return (0);
      }
  }
  return (__LINE__);
}

/****************************************************************************/
/*D
   GetFullUpwindShapes - compute shape functions for full upwinding

   SYNOPSIS:
   INT GetFullUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF], DOUBLE Shape[MAXF][MAXNC])

   PARAMETERS:
   .  geo - finite volume element geometry
   .  IPVel - velocity vectors at integration points
   .  Shape - resulting shape functions

   DESCRIPTION:
   This function computes the shape functions for full upwinding. The weight of the upstream
   node at the scv-faces edge is 1, all others are 0.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT GetFullUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF], DOUBLE Shape[MAXF][MAXNC])
{
  DOUBLE sp;
  INT ip,corn;

  for (ip=0; ip<FVG_NSCVF(geo); ip++)
  {
    for (corn=0; corn<FVG_NSCV(geo); corn++)
      Shape[ip][corn] = 0.0;

    V_DIM_SCALAR_PRODUCT(SCVF_NORMAL(FVG_SCVF(geo,ip)),IPVel[ip],sp);
    if (sp>0.0)
      Shape[ip][CORNER_OF_EDGE(FVG_ELEM(geo),ip,0)] = 1.0;
    else
      Shape[ip][CORNER_OF_EDGE(FVG_ELEM(geo),ip,1)] = 1.0;
  }
  return (0);
}

/****************************************************************************/
/*D
   GetSkewedUpwindShapes - compute shape functions for skewed upwinding

   SYNOPSIS:
   INT GetSkewedUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF], DOUBLE Shape[MAXF][MAXNC])

   PARAMETERS:
   .  geo - finite volume element geometry
   .  IPVel - velocity vectors at integration points
   .  Shape - resulting shape functions

   DESCRIPTION:
   This function computes the shape functions for skewed upwinding. The node next to
   the upwind intersection with the element boundary has weight 1, all others are 0.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT GetSkewedUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF], DOUBLE Shape[MAXF][MAXNC])
{
  const DOUBLE_VECTOR *x;
  INT ip,corn,tag;

  x = FVG_GCOPTR(geo);
  tag = FVG_TAG(geo);

  for (ip=0; ip<FVG_NSCVF(geo); ip++)
  {
    for (corn=0; corn<FVG_NSCV(geo); corn++)
      Shape[ip][corn] = 0.0;

    if (V2_ISZERO(IPVel[ip]))
      continue;

    GetNodeNextToCut(tag,x,SCVF_GIP(FVG_SCVF(geo,ip)),IPVel[ip],&corn);
    Shape[ip][corn] = 1.0;
  }
  return (0);
}

/****************************************************************************/
/*D
   GetMJRawRegularUpwindShapes - compute nodal and ip shape functions for MJ Raws regular upwinding (2D)

   SYNOPSIS:
   INT GetMJRawRegularUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF],
                                                                        DOUBLE NodalShape[MAXF][MAXNC], DOUBLE IPShape[MAXF][MAXF])

   PARAMETERS:
   .  geo - finite volume element geometry
   .  IPVel - velocity vectors at integration points
   .  NodalShape - resulting nodal shape functions
   .  IPShape - resulting ip shape functions

   DESCRIPTION:
   This function computes nodal and ip shape functions for MJ Raws regular upwinding (2D).
   Not only nodes but also ips are involved. The system of equations for ip values has
   to be solved later.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT GetMJRawRegularUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF],
                                 DOUBLE NodalShape[MAXF][MAXNC], DOUBLE IPShape[MAXF][MAXF])
{
#       ifdef __TWODIM__

  DOUBLE ipflow,dir;
  DOUBLE lambda;
  DOUBLE_VECTOR SCVCorners[MAXF];
  INT i,ip,corn,nc,side;

  nc = FVG_NSCV(geo);

  for (ip=0; ip<FVG_NSCVF(geo); ip++)
  {
    for (corn=0; corn<nc; corn++)
      NodalShape[ip][corn] = 0.0;
    for (i=0; i<FVG_NSCVF(geo); i++)
      IPShape[ip][i] = 0.0;

    if (V2_ISZERO(IPVel[ip]))
      continue;

    /* determine upwind node and upwind ip */
    V2_SCALAR_PRODUCT(IPVel[ip],SCVF_NORMAL(FVG_SCVF(geo,ip)),ipflow);
    if (fabs(ipflow)<100*SMALL_C)       /* unfortunately this threshold is crucial for */
    /* Intersect2d (maybe there's a better asolution) */
    {
      /* the convection is parallel to the subcontrol volume surface */
      V2_VECTOR_PRODUCT(IPVel[ip],SCVF_NORMAL(FVG_SCVF(geo,ip)),dir);
      if (dir>0)
      {
        /* the velocity is pointing to the element boundary */
        /* take lin comb of pred and succ ip */
        IPShape[ip][(ip+nc-1)%nc] = IPShape[ip][(ip+1)%nc] = 0.5;
      }
      else
      {
        /* the velocity is pointing to the element midpoint */
        /* take lin comb of pred and succ node */
        NodalShape[ip][ip] = NodalShape[ip][(ip+1)%nc] = 0.5;
      }
      continue;
    }
    else if (ipflow>0)
    {
      /* cut with subcontrol volume surface of predecessor node */
      V2_COPY(FVG_GCO(geo,ip),                    SCVCorners[0]);
      V2_COPY(FVG_GEM(geo,ip),                    SCVCorners[1]);
      V2_COPY(FVG_GCM(geo),                               SCVCorners[2]);
      V2_COPY(FVG_GEM(geo,(ip+nc-1)%nc),  SCVCorners[3]);

      if (Intersect2d(4,SCVCorners,IPVel[ip],SCVF_GIP(FVG_SCVF(geo,ip)),&side,&lambda)!=0)
        continue;

      switch (side)
      {
      case 0 :
        /* take linear profile between nodes on element side ip */
        NodalShape[ip][ip]                = 1.0-0.5*lambda;
        NodalShape[ip][(ip+1)%nc] = 0.5*lambda;
        break;

      case 1 :
        /* this should not happen */
        return (1);

      case 2 :
        if (lambda>0.5)
        {
          /* take linear profile between mean of nodes of side ip-1 and ip-value at ip-1 */
          NodalShape[ip][(ip+nc-1)%nc] =
            NodalShape[ip][ip]                       = 0.5*2*(lambda-0.5);
          IPShape[ip][(ip+nc-1)%nc]        = 1.0-2*(lambda-0.5);
        }
        else
        {
          /* take linear profile between ip-values ip-1 and ip+1 */
          IPShape[ip][(ip+nc-1)%nc] = 0.5+lambda;
          IPShape[ip][(ip+1)%nc]    = 1.0-(0.5+lambda);
        }
        break;

      case 3 :
        /* take linear profile between nodes on element side ip-1 */
        NodalShape[ip][(ip+nc-1)%nc] = 1.0-0.5*(lambda+1.0);
        NodalShape[ip][ip]                       = 0.5*(lambda+1.0);
        break;
      }
    }
    else
    {
      /* cut with subcontrol volume surface of successor node */
      V2_COPY(FVG_GCO(geo,(ip+1)%nc),     SCVCorners[0]);
      V2_COPY(FVG_GEM(geo,(ip+1)%nc),     SCVCorners[1]);
      V2_COPY(FVG_GCM(geo),                       SCVCorners[2]);
      V2_COPY(FVG_GEM(geo,ip),            SCVCorners[3]);

      if (Intersect2d(4,SCVCorners,IPVel[ip],SCVF_GIP(FVG_SCVF(geo,ip)),&side,&lambda)!=0)
        continue;

      switch (side)
      {
      case 0 :
        /* take linear profile between nodes on element side ip+1 */
        NodalShape[ip][(ip+1)%nc] = 1.0-0.5*lambda;
        NodalShape[ip][(ip+2)%nc] = 0.5*lambda;
        break;

      case 1 :
        if (lambda<0.5)
        {
          /* take linear profile between mean of nodes of side ip+1 and ip-value at ip+1 */
          NodalShape[ip][(ip+1)%nc] =
            NodalShape[ip][(ip+2)%nc] = 0.5*(1.0-2*lambda);
          IPShape[ip][(ip+1)%nc]    = 2*lambda;
        }
        else
        {
          /* take linear profile between ip-values ip-1 and ip+1 */
          IPShape[ip][(ip+nc-1)%nc] = lambda-0.5;
          IPShape[ip][(ip+1)%nc]    = 1.0-(lambda-0.5);
        }
        break;

      case 2 :
        /* this should not happen */
        return (1);

      case 3 :
        /* take linear profile between nodes on element side ip */
        NodalShape[ip][ip]                = 1.0-0.5*(lambda+1.0);
        NodalShape[ip][(ip+1)%nc] = 0.5*(lambda+1.0);
        break;
      }
    }
  }

#   endif

#   ifdef __THREEDIM__
  PrintErrorMessage('E',"GetMJRawRegularUpwindShapes","not implemented for 3D");
  return (__LINE__);
#   endif

  return (0);
}

/****************************************************************************/
/*D
   GetMJRawPositiveUpwindShapes - compute nodal and ip shape functions for MJ Raws positve upwinding (2D)

   SYNOPSIS:
   INT GetMJRawPositiveUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF],
                                                                                DOUBLE NodalShape[MAXF][MAXNC], DOUBLE IPShape[MAXF][MAXF])

   PARAMETERS:
   .  geo - finite volume element geometry
   .  IPVel - velocity vectors at integration points
   .  NodalShape - resulting nodal shape functions
   .  IPShape - resulting ip shape functions

   DESCRIPTION:
   This function computes nodal and ip shape functions for MJ Raws positve upwinding (2D).
   Not only nodes but also ips are involved. The system of equations for ip values has
   to be solved later.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT GetMJRawPositiveUpwindShapes (const FVElementGeometry *geo, const DOUBLE_VECTOR IPVel[MAXF],
                                  DOUBLE NodalShape[MAXF][MAXNC], DOUBLE IPShape[MAXF][MAXF])
{
#   ifdef __TWODIM__

  DOUBLE ipflow,upflow,ratio,coeff,dir;
  INT i,ip,corn,node,upip,nc;

  nc = FVG_NSCV(geo);
  for (ip=0; ip<FVG_NSCVF(geo); ip++)
  {
    for (corn=0; corn<nc; corn++)
      NodalShape[ip][corn] = 0.0;
    for (i=0; i<FVG_NSCVF(geo); i++)
      IPShape[ip][i] = 0.0;

    if (V2_ISZERO(IPVel[ip]))
      continue;

    /* determine upwind node and upwind ip */
    V2_SCALAR_PRODUCT(IPVel[ip],SCVF_NORMAL(FVG_SCVF(geo,ip)),ipflow);
    if (fabs(ipflow)<SMALL_C)
    {
      /* the convection is parallel to the subcontrol volume surface */
      V2_VECTOR_PRODUCT(IPVel[ip],SCVF_NORMAL(FVG_SCVF(geo,ip)),dir);
      if (dir>0)
      {
        /* the velocity is pointing to the element boundary */
        /* take lin comb of pred and succ ip */
        IPShape[ip][(ip+nc-1)%nc] = IPShape[ip][(ip+1)%nc] = 0.5;
      }
      else
      {
        /* the velocity is pointing to the element midpoint */
        /* take lin comb of pred and succ node */
        NodalShape[ip][ip] = NodalShape[ip][(ip+1)%nc] = 0.5;
      }
      continue;
    }
    else if (ipflow>0)
    {
      /* predecessor node and ip */
      node = ip;
      upip = (ip+nc-1)%nc;
    }
    else
    {
      /* successor node and ip */
      node = (ip+1)%nc;
      upip = (ip+1)%nc;
    }
    V2_SCALAR_PRODUCT(IPVel[upip],SCVF_NORMAL(FVG_SCVF(geo,upip)),upflow);

    ratio = upflow/ipflow;
    coeff = MAX(MIN(ratio,1.0),0.0);
    IPShape[ip][upip]    = coeff;
    NodalShape[ip][node] = 1.0-coeff;
  }

#   endif

#   ifdef __THREEDIM__
  PrintErrorMessage('E',"GetMJRawPositiveUpwindShapes","not implemented for 3D");
  return (__LINE__);
#   endif

  return (0);
}

/****************************************************************************/
/*D
   AFVGeometry - compute geometrical data for aligned finite volumes

   SYNOPSIS:
   INT AFVGeometry (const ELEMENT *theElement, FVElementGeometry *geo, DOUBLE_VECTOR Convection)

   PARAMETERS:
   .  theElement - given element
   .  geo - finite volume element geometry
   .  Convection - given velocity

   DESCRIPTION:
   This function computes the subcontrol volumes for a given element aligned
   to a given velocity.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT AFVGeometry (const ELEMENT *theElement, FVElementGeometry *geo, DOUBLE_VECTOR Convection)
{
  SubControlVolumeFace *scvf;
  SD_VALUES *sdv;
  DOUBLE_VECTOR deriv;
  const DOUBLE *CornerPtrs[MAXNC];
  INT i,j;
  INT coe,eoe;
    #ifdef __TWODIM__
  DOUBLE fact1,fact2;
  DOUBLE_VECTOR b;
  DOUBLE_VECTOR emp[MAXE],edge[MAXS];
  INT outflow[MAXE],noutflow,ninflow,inflow[MAXE];
        #endif
    #ifdef __THREEDIM__
  DOUBLE_VECTOR ScvfLip[MAXE],ScvfGip[MAXE],ScvfNormal[MAXE];
    #endif

  if (V_DIM_ISZERO(Convection))
    return(EvaluateFVGeometry (theElement,geo));

  FVG_ELEM(geo)   = theElement;
  FVG_TAG(geo)    = TAG(theElement);
  FVG_NSCV(geo)   = coe = CORNERS_OF_ELEM(theElement);
  FVG_NSCVF(geo)  = eoe = EDGES_OF_ELEM(theElement);
  switch (coe)
  {
        #ifdef __TWODIM__
  case TRIANGLE :
    /* corners */
    for (i=0; i<coe; i++)
    {
      CornerPtrs[i] = CVECT(MYVERTEX(CORNER(theElement,i)));
      V2_COPY(CornerPtrs[i],FVG_GCO(geo,i));
    }

    /* surface normals */
    ninflow = noutflow = 0;
    for (i=0; i<3; i++)
    {
      scvf = FVG_SCVF(geo,i);
      V2_CLEAR(SCVF_NORMAL(scvf));

      V2_LINCOMB(0.5,CornerPtrs[(i+1)%3],0.5,CornerPtrs[i],emp[i])
      V2_SUBTRACT(CornerPtrs[CORNER_OF_EDGE(theElement,i,1)],CornerPtrs[CORNER_OF_EDGE(theElement,i,0)],edge[i])
      j = (2*CORNER_OF_EDGE(theElement,i,1)+2*CORNER_OF_EDGE(theElement,i,0))%3;
      V2_SUBTRACT(CornerPtrs[j],CornerPtrs[CORNER_OF_EDGE(theElement,i,0)],b)
      V2_VECTOR_PRODUCT(edge[i],b,fact1)
      V2_VECTOR_PRODUCT(edge[i],Convection,fact2)
      if (fact1*fact2>=0.0)
      {
        inflow[ninflow] = i;
        ninflow++;
      }
      else
      {
        outflow[noutflow] = i;
        noutflow++;
      }
    }
    switch (ninflow)
    {
    case 1 :
      /* surface normals */
      SCVF_NORMAL(FVG_SCVF(geo,outflow[0]))[_X_] = emp[outflow[0]][_Y_] - emp[inflow[0]] [_Y_];
      SCVF_NORMAL(FVG_SCVF(geo,outflow[0]))[_Y_] = emp[inflow[0]] [_X_] - emp[outflow[0]][_X_];
      V2_SCALAR_PRODUCT(SCVF_NORMAL(FVG_SCVF(geo,outflow[0])),edge[outflow[0]],fact1)
      if (fact1<0.0)
        V2_SCALE(-1.0,SCVF_NORMAL(FVG_SCVF(geo,outflow[0])))
        SCVF_NORMAL(FVG_SCVF(geo,outflow[1]))[_X_] = emp[outflow[1]][_Y_] - emp[inflow[0]] [_Y_];
      SCVF_NORMAL(FVG_SCVF(geo,outflow[1]))[_Y_] = emp[inflow[0]] [_X_] - emp[outflow[1]][_X_];
      V2_SCALAR_PRODUCT(SCVF_NORMAL(FVG_SCVF(geo,outflow[1])),edge[outflow[1]],fact1)
      if (fact1<0.0)
        V2_SCALE(-1.0,SCVF_NORMAL(FVG_SCVF(geo,outflow[1])))

        /* global and local integration points */
        V2_LINCOMB(0.5,emp[inflow[0]],0.5,emp[outflow[0]],SCVF_GIP(FVG_SCVF(geo,outflow[0])))
        V2_LINCOMB(0.5,emp[inflow[0]],0.5,emp[outflow[1]],SCVF_GIP(FVG_SCVF(geo,outflow[1])))
        if (UG_GlobalToLocal(FVG_NSCV(geo),CornerPtrs,SCVF_GIP(FVG_SCVF(geo,outflow[0])),SCVF_LIP(FVG_SCVF(geo,outflow[0])))) return (1);
      if (UG_GlobalToLocal(FVG_NSCV(geo),CornerPtrs,SCVF_GIP(FVG_SCVF(geo,outflow[1])),SCVF_LIP(FVG_SCVF(geo,outflow[1])))) return (1);
      V2_CLEAR(SCVF_GIP(FVG_SCVF(geo,inflow[0]))) V2_CLEAR(SCVF_LIP(FVG_SCVF(geo,inflow[0])))
      break;
    case 2 :
      /* surface normals */
      SCVF_NORMAL(FVG_SCVF(geo,inflow[0]))[_X_] = emp[outflow[0]][_Y_] - emp[inflow[0]] [_Y_];
      SCVF_NORMAL(FVG_SCVF(geo,inflow[0]))[_Y_] = emp[inflow[0]] [_X_] - emp[outflow[0]][_X_];
      V2_SCALAR_PRODUCT(SCVF_NORMAL(FVG_SCVF(geo,inflow[0])),edge[inflow[0]],fact1)
      if (fact1<0.0)
        V2_SCALE(-1.0,SCVF_NORMAL(FVG_SCVF(geo,inflow[0])))
        SCVF_NORMAL(FVG_SCVF(geo,inflow[1]))[_X_] = emp[outflow[0]][_Y_] - emp[inflow[1]] [_Y_];
      SCVF_NORMAL(FVG_SCVF(geo,inflow[1]))[_Y_] = emp[inflow[1]] [_X_] - emp[outflow[0]][_X_];
      V2_SCALAR_PRODUCT(SCVF_NORMAL(FVG_SCVF(geo,inflow[1])),edge[inflow[1]],fact1)
      if (fact1<0.0)
        V2_SCALE(-1.0,SCVF_NORMAL(FVG_SCVF(geo,inflow[1])))

        /* global and local integration points */
        V2_LINCOMB(0.5,emp[inflow[0]],0.5,emp[outflow[0]],SCVF_GIP(FVG_SCVF(geo,inflow[0])))
        V2_LINCOMB(0.5,emp[inflow[1]],0.5,emp[outflow[0]],SCVF_GIP(FVG_SCVF(geo,inflow[1])))
        if (UG_GlobalToLocal(FVG_NSCV(geo),CornerPtrs,SCVF_GIP(FVG_SCVF(geo,inflow[0])),SCVF_LIP(FVG_SCVF(geo,inflow[0])))) return (1);
      if (UG_GlobalToLocal(FVG_NSCV(geo),CornerPtrs,SCVF_GIP(FVG_SCVF(geo,inflow[1])),SCVF_LIP(FVG_SCVF(geo,inflow[1])))) return (1);
      V2_CLEAR(SCVF_GIP(FVG_SCVF(geo,outflow[0]))) V2_CLEAR(SCVF_LIP(FVG_SCVF(geo,outflow[0])))
      break;
    default :
      PrintErrorMessage('E',"AFVGeometry","unknown case");
      return(__LINE__);
    }
    break;
  case QUADRILATERAL :
    for (i=0; i<coe; i++)
    {
      CornerPtrs[i] = CVECT(MYVERTEX(CORNER(theElement,i)));
      V2_COPY(CornerPtrs[i],FVG_GCO(geo,i));
    }

    /* surface normals */
    break;
                #endif

        #ifdef __THREEDIM__
  case TETRAHEDRON :
    /* get coordinates */
    for (i=0; i<coe; i++)
    {
      CornerPtrs[i] = CVECT(MYVERTEX(CORNER(theElement,i)));
      V3_COPY(CornerPtrs[i],FVG_GCO(geo,i));
    }

    /* surface normals */
    FV_AliTetInfo(CornerPtrs,ScvfNormal,Convection,ScvfGip,ScvfLip);
    for (i=0; i<eoe; i++)
    {
      V3_COPY(ScvfNormal[i],SCVF_NORMAL(FVG_SCVF(geo,i)));
      V3_COPY(ScvfGip[i],SCVF_GIP(FVG_SCVF(geo,i)));
      V3_COPY(ScvfLip[i],SCVF_LIP(FVG_SCVF(geo,i)));
    }
    break;
        #endif /* __THREEDIM__ */

  default :
    PrintErrorMessage('E',"AFVGeometry","unknown elementtype");
    return(__LINE__);
  }

  for (i=0; i<eoe; i++)
  {
    sdv  = FVG_IPSDV(geo,i);
    scvf = FVG_SCVF(geo,i);

    /* shape functions */
    if (GNs(coe,(DOUBLE *)SCVF_LIP(scvf),SDV_SHAPEPTR(sdv)))
    {
      PrintErrorMessage('E',"AFVGeometry","something wrong with shape functions");
      return(__LINE__);
    }

    /* gradients at IPs */
    for (j=0; j<coe; j++)
    {
      if (D_GN(coe,j,(DOUBLE *)SCVF_LIP(scvf),deriv))
      {
        PrintErrorMessage('E',"AFVGeometry","something wrong with derivatives of shape functions");
        return(__LINE__);
      }
      MM_TIMES_V_DIM(SDV_JINV(sdv),deriv,SDV_GRADPTR(sdv,j));
    }
  }

  return (0);
}

/****************************************************************************/
/*D
   EvaluateShapesAndDerivatives - compute shape functions and their derivatives for a given element geometry

   SYNOPSIS:
   INT EvaluateShapesAndDerivatives (FVElementGeometry *geo, INT flags)

   PARAMETERS:
   .  geo - given element geometry and where data are to be stored
   .  flags - specify here which data are to be computed (cf. FILL_* macros in the header file)

   DESCRIPTION:
   This routine fills the given data structure with the values of the genral shape functions
   and their derivatives in (boundary) integration points and corners (if specified).

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.

   SEE ALSO:
   EvaluateFVGeometry
   D*/
/****************************************************************************/

INT EvaluateShapesAndDerivatives (FVElementGeometry *geo, INT flags)
{
  SD_VALUES *sdv,*sdv0;
  const SubControlVolumeFace *scvf;
  const BoundaryFace *scvbf;
  DOUBLE_VECTOR deriv;
  DOUBLE Jot[DIM][DIM];
  INT i,j,nco,nip,nbip,lin;
  INT co_sdv,shapes,Jinv,grad,J;

  co_sdv = READ_FLAG(flags,FILL_CORNER_DATA);
  shapes = READ_FLAG(flags,FILL_SHAPES);
  Jinv   = READ_FLAG(flags,FILL_DERIV);
  grad   = READ_FLAG(flags,FILL_GRAD);
  J      = READ_FLAG(flags,FILL_J);

  if (grad)
    Jinv = TRUE;

  lin = LinearTrafo(DIM,FVG_TAG(geo));

  nco  = FVG_NSCV(geo);

  /* corners */
  if (co_sdv)
  {
    sdv0 = FVG_COSDV(geo,0);
    for (i=0; i<nco; i++)
    {
      sdv = FVG_COSDV(geo,i);

      /* shape functions */
      if (shapes)
        for (j=0; j<nco; j++)
          SDV_SHAPE(sdv,j) = (i==j) ? 1.0 : 0.0;

      /* inverse of transformation */
      if (Jinv)
      {
        if (lin && (i>0))
        {
          SDV_DETJ(sdv) = SDV_DETJ(sdv0);
          MM_DIM_COPY(SDV_JINV(sdv0),SDV_JINV(sdv));
        }
        else
        {
          TRANSFORMATION(nco,FVG_GCOPTR(geo),FVG_LCO(geo,i),Jot);
          M_DIM_INVERT(Jot,SDV_JINV(sdv),SDV_DETJ(sdv));
          if (SDV_DETJ(sdv)==0) return (__LINE__);
        }

        if (J) MM_DIM_COPY(Jot,SDV_J(sdv));

        /* gradients */
        if (grad)
          for (j=0; j<nco; j++)
          {
            if (D_GN(nco,j,FVG_LCO(geo,i),deriv))
            {
              PrintErrorMessage('E',"EvaluateShapesAndDerivatives","something wrong with derivatives of shape functions");
              return(__LINE__);
            }
            MM_TIMES_V_DIM(SDV_JINV(sdv),deriv,SDV_GRADPTR(sdv,j));
          }
      }
    }
  }

  /* integration points */
  nip  = FVG_NSCVF(geo);
  sdv0 = FVG_IPSDV(geo,0);
  for (i=0; i<nip; i++)
  {
    sdv  = FVG_IPSDV(geo,i);
    scvf = FVG_SCVF(geo,i);

    /* shape functions */
    if (shapes)
      if (GNs(nco,(DOUBLE *)SCVF_LIP(scvf),SDV_SHAPEPTR(sdv)))
      {
        PrintErrorMessage('E',"EvaluateShapesAndDerivatives","something wrong with shape functions");
        return(__LINE__);
      }

    /* inverse of transformation */
    if (Jinv)
    {
      if (lin && (i>0))
      {
        SDV_DETJ(sdv) = SDV_DETJ(sdv0);
        MM_DIM_COPY(SDV_JINV(sdv0),SDV_JINV(sdv));
      }
      else
      {
        TRANSFORMATION(nco,FVG_GCOPTR(geo),SCVF_LIP(scvf),Jot);
        M_DIM_INVERT(Jot,SDV_JINV(sdv),SDV_DETJ(sdv));
        if (SDV_DETJ(sdv)==0) return (__LINE__);
      }

      /* gradients */
      if (grad)
        for (j=0; j<nco; j++)
        {
          if (D_GN(nco,j,(DOUBLE *)SCVF_LIP(scvf),deriv))
          {
            PrintErrorMessage('E',"EvaluateShapesAndDerivatives","something wrong with derivatives of shape functions");
            return(__LINE__);
          }
          MM_TIMES_V_DIM(SDV_JINV(sdv),deriv,SDV_GRADPTR(sdv,j));
        }
    }
  }

  /* boundary integration points */
  nbip = FVG_NSCVBF(geo);
  sdv0 = FVG_IPSDV(geo,0);
  for (i=0; i<nbip; i++)
  {
    sdv   = FVG_BIPSDV(geo,i);
    scvbf = FVG_SCVBF(geo,i);

    /* shape functions */
    if (shapes)
      if (GNs(nco,(DOUBLE *)SCVBF_LIP(scvbf),SDV_SHAPEPTR(sdv)))
      {
        PrintErrorMessage('E',"EvaluateShapesAndDerivatives","something wrong with shape functions");
        return(__LINE__);
      }

    /* inverse of transformation */
    if (Jinv)
    {
      if (lin && (i>0))
      {
        SDV_DETJ(sdv) = SDV_DETJ(sdv0);
        MM_DIM_COPY(SDV_JINV(sdv0),SDV_JINV(sdv));
      }
      else
      {
        TRANSFORMATION(nco,FVG_GCOPTR(geo),SCVBF_LIP(scvbf),Jot);
        M_DIM_INVERT(Jot,SDV_JINV(sdv),SDV_DETJ(sdv));
        if (SDV_DETJ(sdv)==0) return (__LINE__);
      }

      /* gradients */
      if (grad)
        for (j=0; j<nco; j++)
        {
          if (D_GN(nco,j,(DOUBLE *)SCVBF_LIP(scvbf),deriv))
          {
            PrintErrorMessage('E',"EvaluateShapesAndDerivatives","something wrong with derivatives of shape functions");
            return(__LINE__);
          }
          MM_TIMES_V_DIM(SDV_JINV(sdv),deriv,SDV_GRADPTR(sdv,j));
        }
    }
  }


  return (0);
}

/****************************************************************************/
/*
   FillLocalCoords - fill local coord tables for given element type

   SYNOPSIS:
   static INT FillLocalCoords (INT tag)

   PARAMETERS:
   .  tag - element type

   DESCRIPTION:
   This function fills local coord tables for a given element type.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
 */
/****************************************************************************/

static INT FillLocalCoords (INT tag)
{
  LOCAL_DOUBLES *lc;
  DOUBLE scale;
  DOUBLE_VECTOR s;
  INT i,j,k,l,n,nco,ned,nsi;
#       ifdef __THREEDIM__
  INT r,kminus1;
#       endif

  lc = LocalCoords+tag;

  nco = CORNERS_OF_TAG(tag);
  ned = EDGES_OF_TAG(tag);
  nsi = SIDES_OF_TAG(tag);

  /* corners */
  for (i=0; i<nco; i++)
    V_DIM_COPY(LOCAL_COORD_OF_TAG(tag,i),lc->co[i]);

  /* edge midpoints */
  for (k=0; k<ned; k++)
  {
    i = CORNER_OF_EDGE_TAG(tag,k,0);
    j = CORNER_OF_EDGE_TAG(tag,k,1);
    V_DIM_AVG2(lc->co[i],lc->co[j],lc->em[k]);
  }

  /* side midpoints */
  for (k=0; k<nsi; k++)
  {
    scale = 1.0/((DOUBLE)CORNERS_OF_SIDE_TAG(tag,k));
    V_DIM_CLEAR(s);
    for (l=0; l<CORNERS_OF_SIDE_TAG(tag,k); l++)
      V_DIM_ADD1(lc->co[CORNER_OF_SIDE_TAG(tag,k,l)],s);
    V_DIM_SCALE(scale,s);
    V_DIM_COPY(s,lc->sm[k]);
  }

  /* center of mass */
  scale = 1.0/((DOUBLE)nco);
  V_DIM_CLEAR(s);
  for (l=0; l<nco; l++)
    V_DIM_ADD1(lc->co[l],s);
  V_DIM_SCALE(scale,s);
  V_DIM_COPY(s,lc->cm);

  /* sub control volume faces */
  for (k=0; k<ned; k++)
  {
#               ifdef __TWODIM__
    V2_AVG2(lc->em[k],lc->cm,lc->ip[k]);
#               endif

#               ifdef __THREEDIM__
    i = CORNER_OF_EDGE_TAG(tag,k,0);
    j = CORNER_OF_EDGE_TAG(tag,k,1);
    r = SIDE_WITH_EDGE_TAG(tag,k,0);
    l = SIDE_WITH_EDGE_TAG(tag,k,1);
    V_DIM_AVG4(lc->em[k],lc->sm[r],lc->cm,lc->sm[l],lc->ip[k]);
#               endif
  }

  /* boundary integration points */
  for (i=0; i<nsi; i++)
  {
    n = CORNERS_OF_SIDE_TAG(tag,i);

    for (k=0; k<n; k++)
    {
#                       ifdef __TWODIM__
      V_DIM_AVG2(lc->co[CORNER_OF_SIDE_TAG(tag,i,k)],lc->sm[i],lc->bip[i][k]);
#                       endif

#                       ifdef __THREEDIM__
      kminus1 = (k+n-1)%n;
      V_DIM_AVG4( lc->co[CORNER_OF_SIDE_TAG(tag,i,k)],
                  lc->em[EDGE_OF_SIDE_TAG(tag,i,k)],
                  lc->sm[i],
                  lc->em[EDGE_OF_SIDE_TAG(tag,i,kminus1)],
                  lc->bip[i][k]);
#                       endif
    }
  }

  return (0);
}

/****************************************************************************/
/*D
   InitFiniteVolumeTools - init the finite volume tools module

   SYNOPSIS:
   INT InitFiniteVolumeTools (void)

   PARAMETERS:
   none

   DESCRIPTION:
   This function initializes the local geometry arrays for the different element
   types.

   RETURN VALUES:
   0 when o.k.

   __LINE__ if an error occured.
   D*/
/****************************************************************************/

INT InitFiniteVolumeGeom (void)
{
  if (DIM==2)
  {
    if (FillLocalCoords(TRIANGLE)) return (__LINE__);
    if (FillLocalCoords(QUADRILATERAL)) return (__LINE__);
  }
  else if (DIM==3)
  {
    if (FillLocalCoords(TETRAHEDRON)) return (__LINE__);
    if (FillLocalCoords(PYRAMID)) return (__LINE__);
    if (FillLocalCoords(PRISM)) return (__LINE__);
    if (FillLocalCoords(HEXAHEDRON)) return (__LINE__);
  }

  return (0);
}
