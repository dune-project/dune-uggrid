// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      evm.c                                                         */
/*                                                                          */
/* Purpose:   elementary vector manipulations                               */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Institut fuer Computeranwendungen                             */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            internet: ug@ica3.uni-stuttgart.de                            */
/*                                                                          */
/* History:   8.12.94 begin, ug3-version                                    */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*        system include files                                              */
/*        application include files                                         */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstddef>

#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/low/ugtypes.h>

#include "elements.h"
#include "evm.h"

USING_UG_NAMESPACE
  USING_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#define OneSixth 0.166666666666666667

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* forward declarations of macros                                           */
/*                                                                          */
/****************************************************************************/

#define MIN_DETERMINANT                                 0.0001*SMALL_C

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****																	 ****/
/****		2D routines                                                                                              ****/
/****																	 ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   M3_Invert - Calculate inverse of a 3x3 DOUBLE matrix

   SYNOPSIS:
   INT M3_Invert (DOUBLE *Inverse, const DOUBLE *Matrix);

   PARAMETERS:
   .  Inverse - inverse of matrix
   .  Matrix - matrix

   DESCRIPTION:
   This function calculates inverse of a 3x3 DOUBLE matrix.
   The entries of the matrices are given in a linear array with the
   following order -

   .vb
 | Matrix[0] Matrix[1] Matrix[2]|
 | Matrix[3] Matrix[4] Matrix[5]|
 | Matrix[6] Matrix[7] Matrix[8]|
   .ve

   RETURN VALUE:
   INT
   .n    0 when ok
   .n    1 when matrix is nearly singular.
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX M3_Invert (DOUBLE *Inverse, const DOUBLE *Matrix)
{
  DOUBLE determinant,invdet;
  INT i,i1,i2, j,j1,j2;

  for (i=0; i<3; i++)
  {
    i1 = (i+1)%3;
    i2 = (i+2)%3;
    for ( j=0; j<3; j++)
    {
      j1 = (j+1)%3;
      j2 = (j+2)%3;
      Inverse[j+3*i] = Matrix[i1+3*j1]*Matrix[i2+3*j2] - Matrix[i1+3*j2]*Matrix[i2+3*j1];
    }
  }
  determinant = Inverse[0+3*0]*Matrix[0+3*0] + Inverse[0+3*1]*Matrix[1+3*0] + Inverse[0+3*2]*Matrix[2+3*0];

  /* check the determinant */
  if (fabs(determinant) > MIN_DETERMINANT)
  {
    invdet = 1.0/determinant;
    for (i=0; i<3; i++)
      for (j=0; j<3; j++)
        Inverse[i+3*j] *= invdet;
    return(0);
  }

  return(1);
}

/****************************************************************************/
/*D
   vp - Return positive number if vector 2 is "left" of vector 1

   SYNOPSIS:
   DOUBLE vp (const DOUBLE x1, const DOUBLE y1, const DOUBLE x2, const DOUBLE y2);

   PARAMETERS:
   .  x1,y1 - coordinates of a 2D vector
   .  x2,y2 - coordinates of a 2D vector

   DESCRIPTION:
   This function returns positive number if vector 2 is "left" of vector 1, i.e.
   the third component of the vector product of (x1,y1,0) and (x2,y2,0).

   RETURN VALUE:
   DOUBLE
   D*/
/****************************************************************************/

DOUBLE NS_DIM_PREFIX vp (const DOUBLE x1, const DOUBLE y1, const DOUBLE x2, const DOUBLE y2)
{
  DOUBLE l1,l2;

  l1 = sqrt(x1*x1+y1*y1);
  l2 = sqrt(x2*x2+y2*y2);
  if ((l1<SMALL_D)||(l2<SMALL_D))
    return(0.0);
  else
    return((x1*y2-y1*x2)/(l1*l2));
}

/****************************************************************************/
/*D
   c_tarea - Compute area of triangle

   SYNOPSIS:
   DOUBLE c_tarea (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2)

   PARAMETERS:
   .  x0 - Array with coordinates of first point
   .  x1 - Array with coordinates of second point
   .  x2 - Array with coordinates of third point

   DESCRIPTION:
   This function computes the area of a triangle.

   RETURN VALUE:
   DOUBLE area
   D*/
/****************************************************************************/
DOUBLE NS_DIM_PREFIX  c_tarea (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2)
{
  return(0.5*fabs((x1[1]-x0[1])*(x2[0]-x0[0])-(x1[0]-x0[0])*(x2[1]-x0[1])));
}
/****************************************************************************/
/*D
   c_qarea - Compute area of a convex quadrilateral

   SYNOPSIS:
   DOUBLE c_qarea (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2, const DOUBLE *x3);

   PARAMETERS:
   .  x0 - Array with coordinates of first point
   .  x1 - Array with coordinates of second point
   .  x2 - Array with coordinates of third point
   .  x3 - Array with coordinates of fourth point

   DESCRIPTION:
   This function computes the area of a convex quadrilateral.

   RETURN VALUE:
   DOUBLE area
   D*/
/****************************************************************************/
DOUBLE NS_DIM_PREFIX c_qarea (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2, const DOUBLE *x3)
{
  return( 0.5*fabs( (x3[1]-x1[1])*(x2[0]-x0[0])-(x3[0]-x1[0])*(x2[1]-x0[1]) ) );
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****																	 ****/
/****		3D routines                                                                                              ****/
/****																	 ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   V3_Normalize	- Normalize vector a 3D vector

   SYNOPSIS:
   INT V3_Normalize (DOUBLE *a);

   PARAMETERS:
   .  a - 3D vector

   DESCRIPTION:
   This function normalizes vector a.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    2 if vector is nearly 0.
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX V3_Normalize (FieldVector<DOUBLE,3>& a)
{
  DOUBLE norm;

  V3_EUKLIDNORM(a,norm);
  if (norm < SMALL_C) return(2);
  V3_SCALE(1.0/norm,a);

  return(0);
}

/****************************************************************************/
/*D
   V3_Project - Project a vector onto another vector.

   SYNOPSIS:
   INT V3_Project (const DOUBLE *a, const DOUBLE *b, DOUBLE *r);

   PARAMETERS:
   .  a - vector to project
   .  b - vector onto project
   .  r - resulting vector

   DESCRIPTION:
   This function projects vector 'a' onto 'b' store in 'r'.

   RETURN VALUE:
   INT
   .n    0 if o.k.
   .n    1 if error occurred.
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX V3_Project (const DOUBLE *a, const DOUBLE *b, DOUBLE *r)
{
  DOUBLE normb, scprd;

  normb = b[0]*b[0]+b[1]*b[1]+b[2]*b[2];
  if (normb==0.0)
    return (1);
  else
  {
    V3_SCALAR_PRODUCT(a,b,scprd)
    scprd /= normb;
    V3_COPY(b,r)
    V3_SCALE(scprd,r)
  }

  return (0);
}

/* Volume computations, orientation is same as in general element definition !
   Idea of the computations:
   All the shapes are subdivided into pyramids whose bases are either triangles
   or quadrilaterals. The volume of any pyramid is V = S h / 3 where S is
   area of the base and h is height of the pyramid. The areas of the triangles
   and the quadrilaterals are computed by means of vector products. The
   height of the pyramid is then taken into account using scalar products.
 */

/* 1. Volume of a tetrahedron.
   A tetrahedron is a prism with a triangular base.
 */
DOUBLE NS_DIM_PREFIX V_te (const DOUBLE *x0, const DOUBLE *x1,
                           const DOUBLE *x2, const DOUBLE *x3)
{
  FieldVector<DOUBLE,3> a, b, h, n;

  V3_SUBTRACT(x1,x0,a);
  V3_SUBTRACT(x2,x0,b);
  V3_SUBTRACT(x3,x0,h);
  V3_VECTOR_PRODUCT(a,b,n);

  return(OneSixth*V3_SCAL_PROD(n,h));
}

/* 2. Volume of an UG pyramid.
   An UG pyramid is a pyramid with a quadrilateral as the base.
 */
DOUBLE NS_DIM_PREFIX V_py (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2,
                           const DOUBLE *x3, const DOUBLE *x4)
{
  FieldVector<DOUBLE,3> a,b,h,n;

  V3_SUBTRACT(x2,x0,a);
  V3_SUBTRACT(x3,x1,b);
  V3_SUBTRACT(x4,x0,h);
  V3_VECTOR_PRODUCT(a,b,n);

  return(OneSixth*V3_SCAL_PROD(n,h));
}

/* 3. Volume of an UG prism.
   An UG prism is a shape with two (in general non-parallel) bases that
   are triangles and four sides that are quadrilaterals. This 'prism'
   is subdivided into two pyramids:
   a) {x0, x_1, x_4, x_3; x_5}
   b) {x_0, x_1, x_2; x_5} (a tetrahedron)
 */
DOUBLE NS_DIM_PREFIX V_pr (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2,
                           const DOUBLE *x3, const DOUBLE *x4, const DOUBLE *x5)
{
  FieldVector<DOUBLE,3> a,b,c,d,e,m,n;

  V3_SUBTRACT(x4,x0,a);
  V3_SUBTRACT(x1,x3,b);
  V3_SUBTRACT(x1,x0,c);
  V3_SUBTRACT(x2,x0,d);
  V3_SUBTRACT(x5,x0,e);

  V3_VECTOR_PRODUCT(a,b,m);
  V3_VECTOR_PRODUCT(c,d,n);
  V3_ADD(n,m,n);

  return(OneSixth*V3_SCAL_PROD(n,e));
}

/* 4. Volume of an UG hexahedron.
   An UG hexahedron is subdivided into two UG prisms.
 */
DOUBLE NS_DIM_PREFIX V_he (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2, const DOUBLE *x3,
                           const DOUBLE *x4, const DOUBLE *x5, const DOUBLE *x6, const DOUBLE *x7)
{
  return(V_pr(x0,x1,x2,x4,x5,x6)+V_pr(x0,x2,x3,x4,x6,x7));
}

DOUBLE NS_DIM_PREFIX GeneralElementVolume (INT tag, DOUBLE *x_co[])
{
  switch (tag)
  {
#               ifdef UG_DIM_2
  case TRIANGLE :
    return(c_tarea (x_co[0],x_co[1],x_co[2]));

  case QUADRILATERAL :
    return(c_qarea (x_co[0],x_co[1],x_co[2],x_co[3]));
#               endif

#               ifdef UG_DIM_3
  case TETRAHEDRON :
    return(V_te(x_co[0],x_co[1],x_co[2],x_co[3]));

  case PYRAMID :
    return (V_py(x_co[0],x_co[1],x_co[2],x_co[3],x_co[4]));

  case PRISM :
    return (V_pr(x_co[0],x_co[1],x_co[2],x_co[3],x_co[4],x_co[5]));

  case HEXAHEDRON :
    return(V_he(x_co[0],x_co[1],x_co[2],x_co[3],x_co[4],x_co[5],x_co[6],x_co[7]));
#                       endif

  default :
    PrintErrorMessage('E',"GeneralElementVolume","unknown element");
    return(0.0);
  }
}

DOUBLE NS_DIM_PREFIX ElementVolume (const ELEMENT *elem)
{
  DOUBLE *x_co[MAX_CORNERS_OF_ELEM];
  INT i;

  for (i=0; i<CORNERS_OF_ELEM(elem); i++)
    x_co[i] = CVECT(MYVERTEX(CORNER(elem,i))).data();

  return (GeneralElementVolume(TAG(elem),x_co));
}
