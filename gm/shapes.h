/*! \file shapes.h
 * \ingroup gm
*/


/****************************************************************************/
/*                                                                          */
/* File:      shapes.h                                                      */
/*                                                                          */
/* Purpose:   header file for shape functions                               */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 368                                       */
/*            6900 Heidelberg                                               */
/* internet:  ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   28.11.95 begin, ug version 3.1                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __SHAPES__
#define __SHAPES__

#include <cmath>

#include "gm.h"
#include "evm.h"

#include <dune/uggrid/low/namespace.h>

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                          */
/****************************************************************************/

#ifdef __TWODIM__

#define CORNER_COORDINATES_TRIANGLE(e,n,x)                                \
                                   {(n)=3;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0))); \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1))); \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2)));}

#define CORNER_COORDINATES_QUADRILATERAL(e,n,x)                           \
                                   {(n)=4;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0))); \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1))); \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2))); \
                                                                   (x)[3]=CVECT(MYVERTEX(CORNER((e),3)));}

#define CORNER_COORDINATES(e,n,x)                               \
 {if (TAG((e))==TRIANGLE)                                       \
                 CORNER_COORDINATES_TRIANGLE((e),(n),(x))       \
  else           CORNER_COORDINATES_QUADRILATERAL((e),(n),(x))}

#endif /* __TWODIM__ */


#define LOCAL_TO_GLOBAL_TRIANGLE(x,local,global)          \
 {(global)[0] = (1.0-(local)[0]-(local)[1])*(x)[0][0]     \
   +(local)[0]*(x)[1][0] + (local)[1]*(x)[2][0];          \
  (global)[1] = (1.0-(local)[0]-(local)[1])*(x)[0][1]     \
   +(local)[0]*(x)[1][1] + (local)[1]*(x)[2][1]; }

#define LOCAL_TO_GLOBAL_QUADRILATERAL(x,local,global)         \
{(global)[0] = (1.0-(local)[0])*(1.0-(local)[1])*(x)[0][0]    \
   + (local)[0]*(1.0-(local)[1])*(x)[1][0]                    \
   + (local)[0]*(local)[1]*(x)[2][0]                          \
   + (1.0-(local)[0])*(local)[1]*(x)[3][0];                   \
 (global)[1] = (1.0-(local)[0])*(1.0-(local)[1])*(x)[0][1]    \
   + (local)[0]*(1.0-(local)[1])*(x)[1][1]                    \
   + (local)[0]*(local)[1]*(x)[2][1]                          \
   + (1.0-(local)[0])*(local)[1]*(x)[3][1];}

#define LOCAL_TO_GLOBAL_2D(n,x,local,global)                              \
 {if ((n) == 3)      LOCAL_TO_GLOBAL_TRIANGLE((x),(local),(global))       \
  else if ((n) == 4) LOCAL_TO_GLOBAL_QUADRILATERAL((x),(local),(global))}

#define AREA_OF_TRIANGLE(x,area)     {DOUBLE detJ; DOUBLE_VECTOR a,b;      \
                                                                          V2_SUBTRACT((x)[1],(x)[0],a);        \
                                                                          V2_SUBTRACT((x)[2],(x)[0],b);        \
                                                                          V2_VECTOR_PRODUCT(a,b,detJ);         \
                                                                          (area) = std::abs(detJ) * 0.5;}

#define AREA_OF_QUADRILATERAL(x,area)   {DOUBLE detJ,ar; DOUBLE_VECTOR a,b;  \
                                                                                 V2_SUBTRACT((x)[1],(x)[0],a);       \
                                                                                 V2_SUBTRACT((x)[2],(x)[0],b);       \
                                                                                 V2_VECTOR_PRODUCT(a,b,detJ);        \
                                                                                 ar = std::abs(detJ) * 0.5; \
                                                                                 V2_SUBTRACT((x)[3],(x)[0],a);       \
                                                                                 V2_VECTOR_PRODUCT(a,b,detJ);        \
                                                                                 (area) = std::abs(detJ) * 0.5 + ar;}

#define AREA_OF_ELEMENT_2D(n,x,area)                       \
 {if ((n) == 3)      AREA_OF_TRIANGLE((x),(area))       \
  else if ((n) == 4) AREA_OF_QUADRILATERAL((x),(area)) }

#define TRANSFORMATION_OF_TRIANGLE(x,M)     \
    { V2_SUBTRACT((x)[1],(x)[0],(M)[0]);    \
          V2_SUBTRACT((x)[2],(x)[0],(M)[1]); }

#define TRANSFORMATION_OF_QUADRILATERAL(x,local,M)                        \
        { DOUBLE a;                                                           \
          a = 1.0 - (local)[1];                                               \
          (M)[0][0] = a*((x)[1][0]-x[0][0])+(local)[1]*((x)[2][0]-(x)[3][0]); \
          (M)[0][1] = a*((x)[1][1]-x[0][1])+(local)[1]*((x)[2][1]-(x)[3][1]); \
          a = 1.0 - (local)[0];                                               \
          (M)[1][0] = a*((x)[3][0]-x[0][0])+(local)[0]*((x)[2][0]-(x)[1][0]); \
          (M)[1][1] = a*((x)[3][1]-x[0][1])+(local)[0]*((x)[2][1]-(x)[1][1]); }

#define TRANSFORMATION_2D(n,x,local,M)                                      \
 {if ((n) == 3)      {TRANSFORMATION_OF_TRIANGLE((x),(M));}              \
  else TRANSFORMATION_OF_QUADRILATERAL((x),(local),(M)); }


#ifdef __THREEDIM__
#define CORNER_COORDINATES_TETRAHEDRON(e,n,x)                              \
                                  {(n) = 4;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0)));  \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1)));  \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2)));  \
                                                   (x)[3]=CVECT(MYVERTEX(CORNER((e),3)));}

#define CORNER_COORDINATES_PYRAMID(e,n,x)                                  \
                                  {(n) = 5;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0)));  \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1)));  \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2)));  \
                                   (x)[3]=CVECT(MYVERTEX(CORNER((e),3)));  \
                                   (x)[4]=CVECT(MYVERTEX(CORNER((e),4)));}

#define CORNER_COORDINATES_PRISM(e,n,x)                                    \
                                  {(n) = 6;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0)));  \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1)));  \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2)));  \
                                   (x)[3]=CVECT(MYVERTEX(CORNER((e),3)));  \
                                   (x)[4]=CVECT(MYVERTEX(CORNER((e),4)));  \
                                   (x)[5]=CVECT(MYVERTEX(CORNER((e),5)));}

#define CORNER_COORDINATES_HEXAHEDRON(e,n,x)                               \
                                  {(n) = 8;                                \
                                                                   (x)[0]=CVECT(MYVERTEX(CORNER((e),0)));  \
                                                                   (x)[1]=CVECT(MYVERTEX(CORNER((e),1)));  \
                                                                   (x)[2]=CVECT(MYVERTEX(CORNER((e),2)));  \
                                                   (x)[3]=CVECT(MYVERTEX(CORNER((e),3)));  \
                                                                   (x)[4]=CVECT(MYVERTEX(CORNER((e),4)));  \
                                                                   (x)[5]=CVECT(MYVERTEX(CORNER((e),5)));  \
                                                                   (x)[6]=CVECT(MYVERTEX(CORNER((e),6)));  \
                                                                   (x)[7]=CVECT(MYVERTEX(CORNER((e),7)));}

#define CORNER_COORDINATES(e,n,x)                                            \
  {if (TAG((e))==TETRAHEDRON)     CORNER_COORDINATES_TETRAHEDRON((e),(n),(x))\
   else if (TAG((e))==PYRAMID)    CORNER_COORDINATES_PYRAMID((e),(n),(x))    \
   else if (TAG((e))==PRISM)      CORNER_COORDINATES_PRISM((e),(n),(x))      \
   else CORNER_COORDINATES_HEXAHEDRON((e),(n),(x))}

#endif /* __THREEDIM__ */


#define LOCAL_TO_GLOBAL_TETRAHEDRON(x,local,global)                       \
 {(global)[0] = (1.0-(local)[0]-(local)[1]-(local)[2])*(x)[0][0]          \
   +(local)[0]*(x)[1][0] + (local)[1]*(x)[2][0] + (local)[2]*(x)[3][0];   \
  (global)[1] = (1.0-(local)[0]-(local)[1]-(local)[2])*(x)[0][1]          \
   +(local)[0]*(x)[1][1] + (local)[1]*(x)[2][1] + (local)[2]*(x)[3][1];   \
  (global)[2] = (1.0-(local)[0]-(local)[1]-(local)[2])*(x)[0][2]          \
   +(local)[0]*(x)[1][2] + (local)[1]*(x)[2][2] + (local)[2]*(x)[3][2]; }

#define LOCAL_TO_GLOBAL_PYRAMID(x,local,global)                             \
 {DOUBLE a,b,a0,a1,a2,a3;                                                   \
  a = 1.0 - (local)[0];                                                     \
  b = 1.0 - (local)[1];                                                     \
  if ((local)[0] > (local)[1]) {                                            \
  a0 = a * b - (local)[2] * b;                                              \
  a1 = (local)[0] * b - (local)[2]*(local)[1];                              \
  a2 = (local)[0] * (local)[1] + (local)[2]*(local)[1];                     \
  a3 = a * (local)[1] - (local)[2] * (local)[1];                            \
  (global)[0] =                                                               \
        a0*(x)[0][0]+a1*(x)[1][0]+a2*(x)[2][0]+a3*(x)[3][0]+(local)[2]*(x)[4][0]; \
  (global)[1] =                                                               \
        a0*(x)[0][1]+a1*(x)[1][1]+a2*(x)[2][1]+a3*(x)[3][1]+(local)[2]*(x)[4][1]; \
  (global)[2] =                                                               \
        a0*(x)[0][2]+a1*(x)[1][2]+a2*(x)[2][2]+a3*(x)[3][2]+(local)[2]*(x)[4][2];}\
  else {                                                                      \
  a0 = a * b - (local)[2] * a;                                                \
  a1 = (local)[0] * b - (local)[2]*(local)[0];                                \
  a2 = (local)[0] * (local)[1] + (local)[2]*(local)[0];                       \
  a3 = a * (local)[1] - (local)[2] * (local)[0];                              \
  (global)[0] =                                                               \
        a0*(x)[0][0]+a1*(x)[1][0]+a2*(x)[2][0]+a3*(x)[3][0]+(local)[2]*(x)[4][0]; \
  (global)[1] =                                                               \
        a0*(x)[0][1]+a1*(x)[1][1]+a2*(x)[2][1]+a3*(x)[3][1]+(local)[2]*(x)[4][1]; \
  (global)[2] =                                                               \
        a0*(x)[0][2]+a1*(x)[1][2]+a2*(x)[2][2]+a3*(x)[3][2]+(local)[2]*(x)[4][2];}}

#define LOCAL_TO_GLOBAL_PRISM(x,local,global)                               \
 {DOUBLE a,b,a0,a1,a2,a3,a4,a5;                                                 \
  a = 1.0 - (local)[0] - (local)[1];                                        \
  b = 1.0 - (local)[2];                                                     \
  a0 = a * b;                                                               \
  a1 = (local)[0] * b;                                                      \
  a2 = (local)[1] * b;                                                      \
  a3 = a * (local)[2];                                                      \
  a4 = (local)[0] * (local)[2];                                             \
  a5 = (local)[1] * (local)[2];                                             \
  (global)[0] =                                                             \
        a0*(x)[0][0]+a1*(x)[1][0]+a2*(x)[2][0]+a3*(x)[3][0]+                    \
    a4*(x)[4][0]+a5*(x)[5][0];                                              \
  (global)[1] =                                                             \
        a0*(x)[0][1]+a1*(x)[1][1]+a2*(x)[2][1]+a3*(x)[3][1]+                    \
        a4*(x)[4][1]+a5*(x)[5][1];                                              \
  (global)[2] =                                                             \
        a0*(x)[0][2]+a1*(x)[1][2]+a2*(x)[2][2]+a3*(x)[3][2]+                    \
        a4*(x)[4][2]+a5*(x)[5][2]; }

#define LOCAL_TO_GLOBAL_HEXAHEDRON(x,local,global)                          \
 {DOUBLE a,b,c,a0,a1,a2,a3,a4,a5,a6,a7;                                     \
  a = 1.0 - (local)[0];                                                     \
  b = 1.0 - (local)[1];                                                     \
  c = 1.0 - (local)[2];                                                     \
  a0 = a * b * c;                                                           \
  a1 = (local)[0] * b * c;                                                  \
  a2 = (local)[0] * (local)[1] * c;                                         \
  a3 = a * (local)[1] * c;                                                  \
  a4 = a * b * (local)[2];                                                  \
  a5 = (local)[0] * b * (local)[2];                                         \
  a6 = (local)[0] * (local)[1] * (local)[2];                                \
  a7 = a * (local)[1] * (local)[2];                                         \
  (global)[0] =                                                             \
        a0*(x)[0][0]+a1*(x)[1][0]+a2*(x)[2][0]+a3*(x)[3][0]+                    \
        a4*(x)[4][0]+a5*(x)[5][0]+a6*(x)[6][0]+a7*(x)[7][0];                    \
  (global)[1] =                                                             \
        a0*(x)[0][1]+a1*(x)[1][1]+a2*(x)[2][1]+a3*(x)[3][1]+                    \
        a4*(x)[4][1]+a5*(x)[5][1]+a6*(x)[6][1]+a7*(x)[7][1];                    \
  (global)[2] =                                                             \
        a0*(x)[0][2]+a1*(x)[1][2]+a2*(x)[2][2]+a3*(x)[3][2]+                    \
        a4*(x)[4][2]+a5*(x)[5][2]+a6*(x)[6][2]+a7*(x)[7][2]; }

#define LOCAL_TO_GLOBAL_3D(n,x,local,global)                              \
 {if ((n) == 4)      LOCAL_TO_GLOBAL_TETRAHEDRON((x),(local),(global))    \
  else if ((n) == 5) LOCAL_TO_GLOBAL_PYRAMID((x),(local),(global))        \
  else if ((n) == 6) LOCAL_TO_GLOBAL_PRISM((x),(local),(global))          \
  else if ((n) == 8) LOCAL_TO_GLOBAL_HEXAHEDRON((x),(local),(global))}

#define AREA_OF_TETRAHEDRON(x,area)  {DOUBLE detJ; DOUBLE_VECTOR a,b,c;    \
                                                                          V3_SUBTRACT((x)[1],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[2],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[3],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          (area) = std::abs(detJ)/6.0;}

#define AREA_OF_PYRAMID(x,area)      {DOUBLE detJ,ar; DOUBLE_VECTOR a,b,c,d;\
                                                                          V3_SUBTRACT((x)[1],(x)[0],a);         \
                                                                          V3_SUBTRACT((x)[2],(x)[0],b);         \
                                                                          V3_VECTOR_PRODUCT(a,b,c);             \
                                                                          V3_SUBTRACT((x)[4],(x)[0],d);         \
                                                                          V3_SCALAR_PRODUCT(c,d,detJ);          \
                                                                          ar = std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[3],(x)[0],a);         \
                                                                          V3_VECTOR_PRODUCT(a,b,c);             \
                                                                          V3_SCALAR_PRODUCT(c,d,detJ);          \
                                                                          (area) = std::abs(detJ)/6.0 + ar;}

#define AREA_OF_PRISM(x,area)   {DOUBLE detJ,ar; DOUBLE_VECTOR a,b,c;      \
                                                                          V3_SUBTRACT((x)[1],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[2],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[3],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar = std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[2],(x)[1],a);        \
                                                                          V3_SUBTRACT((x)[3],(x)[1],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[4],(x)[1],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar += std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[2],(x)[5],a);        \
                                                                          V3_SUBTRACT((x)[3],(x)[5],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[4],(x)[5],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          (area) = std::abs(detJ)/6.0 + ar;}

#define AREA_OF_HEXAHEDRON(x,area)   {DOUBLE detJ,ar; DOUBLE_VECTOR a,b,c; \
                                                                          V3_SUBTRACT((x)[1],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[2],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[5],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar = std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[2],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[5],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[6],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar += std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[4],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[5],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[6],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar += std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[2],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[3],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[6],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar += std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[3],(x)[0],a);        \
                                                                          V3_SUBTRACT((x)[4],(x)[0],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[6],(x)[0],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          ar += std::abs(detJ)/6.0; \
                                                                          V3_SUBTRACT((x)[3],(x)[7],a);        \
                                                                          V3_SUBTRACT((x)[4],(x)[7],b);        \
                                                                          V3_VECTOR_PRODUCT(a,b,c);            \
                                                                          V3_SUBTRACT((x)[6],(x)[7],a);        \
                                                                          V3_SCALAR_PRODUCT(a,c,detJ);         \
                                                                          (area) = std::abs(detJ)/6.0 + ar;}

#define AREA_OF_ELEMENT_3D(n,x,area)                        \
 {if ((n) == 4)      {AREA_OF_TETRAHEDRON((x),(area));}  \
  else if ((n) == 5) {AREA_OF_PYRAMID((x),(area));}      \
  else if ((n) == 6) {AREA_OF_PRISM((x),(area));}        \
  else if ((n) == 8) {AREA_OF_HEXAHEDRON((x),(area));}}

#define TRANSFORMATION_OF_TETRAHEDRON(x,M)   \
        { V3_SUBTRACT((x)[1],(x)[0],(M)[0]);     \
          V3_SUBTRACT((x)[2],(x)[0],(M)[1]);     \
          V3_SUBTRACT((x)[3],(x)[0],(M)[2]);}

#define TRANSFORMATION_OF_PYRAMID(x,local,M)                              \
        { DOUBLE a,b,c;                                                       \
          a = (x)[0][0]-(x)[1][0]+(x)[2][0]-(x)[3][0];                        \
          b = (x)[0][1]-(x)[1][1]+(x)[2][1]-(x)[3][1];                        \
          c = (x)[0][2]-(x)[1][2]+(x)[2][2]-(x)[3][2];                        \
          if ((local)[0] > (local)[1]) {                                      \
          (M)[0][0] = (x)[1][0]-(x)[0][0]+(local)[1]*a;                       \
          (M)[0][1] = (x)[1][1]-(x)[0][1]+(local)[1]*b;                       \
          (M)[0][2] = (x)[1][2]-(x)[0][2]+(local)[1]*c;                       \
          (M)[1][0] = (x)[3][0]-(x)[0][0]+((local)[0]+(local)[2])*a;          \
          (M)[1][1] = (x)[3][1]-(x)[0][1]+((local)[0]+(local)[2])*b;          \
          (M)[1][2] = (x)[3][2]-(x)[0][2]+((local)[0]+(local)[2])*c;          \
          (M)[2][0] = (x)[4][0]-(x)[0][0]+(local)[1]*a;                       \
          (M)[2][1] = (x)[4][1]-(x)[0][1]+(local)[1]*b;                       \
          (M)[2][2] = (x)[4][2]-(x)[0][2]+(local)[1]*c;}                      \
          else {                                                              \
          (M)[0][0] = (x)[1][0]-(x)[0][0]+((local)[1]+(local)[2])*a;          \
          (M)[0][1] = (x)[1][1]-(x)[0][1]+((local)[1]+(local)[2])*b;          \
          (M)[0][2] = (x)[1][2]-(x)[0][2]+((local)[1]+(local)[2])*c;          \
          (M)[1][0] = (x)[3][0]-(x)[0][0]+(local)[0]*a;                       \
          (M)[1][1] = (x)[3][1]-(x)[0][1]+(local)[0]*b;                       \
          (M)[1][2] = (x)[3][2]-(x)[0][2]+(local)[0]*c;                       \
          (M)[2][0] = (x)[4][0]-(x)[0][0]+(local)[0]*a;                       \
          (M)[2][1] = (x)[4][1]-(x)[0][1]+(local)[0]*b;                       \
          (M)[2][2] = (x)[4][2]-(x)[0][2]+(local)[0]*c;}  }

#define TRANSFORMATION_OF_PRISM(x,local,M)                                \
        { DOUBLE a0,a1,a2,b0,b1,b2;                                           \
          a0 = (x)[0][0]-(x)[1][0]-(x)[3][0]+(x)[4][0];                       \
          a1 = (x)[0][1]-(x)[1][1]-(x)[3][1]+(x)[4][1];                       \
          a2 = (x)[0][2]-(x)[1][2]-(x)[3][2]+(x)[4][2];                       \
          b0 = (x)[0][0]-(x)[2][0]-(x)[3][0]+(x)[5][0];                       \
          b1 = (x)[0][1]-(x)[2][1]-(x)[3][1]+(x)[5][1];                       \
          b2 = (x)[0][2]-(x)[2][2]-(x)[3][2]+(x)[5][2];                       \
          (M)[0][0] = (x)[1][0]-(x)[0][0]+(local)[2]*a0;                      \
          (M)[0][1] = (x)[1][1]-(x)[0][1]+(local)[2]*a1;                      \
          (M)[0][2] = (x)[1][2]-(x)[0][2]+(local)[2]*a2;                      \
          (M)[1][0] = (x)[2][0]-(x)[0][0]+(local)[2]*b0;                      \
          (M)[1][1] = (x)[2][1]-(x)[0][1]+(local)[2]*b1;                      \
          (M)[1][2] = (x)[2][2]-(x)[0][2]+(local)[2]*b2;                      \
          (M)[2][0] = (x)[3][0]-(x)[0][0]+(local)[0]*a0+(local)[1]*b0;        \
          (M)[2][1] = (x)[3][1]-(x)[0][1]+(local)[0]*a1+(local)[1]*b1;        \
          (M)[2][2] = (x)[3][2]-(x)[0][2]+(local)[0]*a2+(local)[1]*b2;}

#define TRANSFORMATION_OF_HEXAHEDRON(x,local,M)                           \
        { DOUBLE a,b,c,a0,a1,a2,a3;                                           \
          a = 1.0 - (local)[0];                                               \
          b = 1.0 - (local)[1];                                               \
          c = 1.0 - (local)[2];                                               \
      a0 = b * c;                                                         \
      a1 = (local)[1] * c;                                                \
      a2 = (local)[1] * (local)[2];                                       \
      a3 = b * (local)[2];                                                \
          (M)[0][0] = a0*((x)[1][0]-x[0][0])+a1*((x)[2][0]-(x)[3][0])         \
                    + a2*((x)[6][0]-x[7][0])+a3*((x)[5][0]-(x)[4][0]);        \
          (M)[0][1] = a0*((x)[1][1]-x[0][1])+a1*((x)[2][1]-(x)[3][1])         \
                    + a2*((x)[6][1]-x[7][1])+a3*((x)[5][1]-(x)[4][1]);        \
          (M)[0][2] = a0*((x)[1][2]-x[0][2])+a1*((x)[2][2]-(x)[3][2])         \
                    + a2*((x)[6][2]-x[7][2])+a3*((x)[5][2]-(x)[4][2]);        \
      a0 = a * c;                                                         \
      a1 = (local)[0] * c;                                                \
      a2 = (local)[0] * (local)[2];                                       \
      a3 = a * (local)[2];                                                \
          (M)[1][0] = a0*((x)[3][0]-x[0][0])+a1*((x)[2][0]-(x)[1][0])         \
                    + a2*((x)[6][0]-x[5][0])+a3*((x)[7][0]-(x)[4][0]);        \
          (M)[1][1] = a0*((x)[3][1]-x[0][1])+a1*((x)[2][1]-(x)[1][1])         \
                    + a2*((x)[6][1]-x[5][1])+a3*((x)[7][1]-(x)[4][1]);        \
          (M)[1][2] = a0*((x)[3][2]-x[0][2])+a1*((x)[2][2]-(x)[1][2])         \
                    + a2*((x)[6][2]-x[5][2])+a3*((x)[7][2]-(x)[4][2]);        \
      a0 = a * b;                                                         \
      a1 = (local)[0] * b;                                                \
      a2 = (local)[0] * (local)[1];                                       \
      a3 = a * (local)[1];                                                \
          (M)[2][0] = a0*((x)[4][0]-x[0][0])+a1*((x)[5][0]-(x)[1][0])         \
                    + a2*((x)[6][0]-x[2][0])+a3*((x)[7][0]-(x)[3][0]);        \
          (M)[2][1] = a0*((x)[4][1]-x[0][1])+a1*((x)[5][1]-(x)[1][1])         \
                    + a2*((x)[6][1]-x[2][1])+a3*((x)[7][1]-(x)[3][1]);        \
          (M)[2][2] = a0*((x)[4][2]-x[0][2])+a1*((x)[5][2]-(x)[1][2])         \
                    + a2*((x)[6][2]-x[2][2])+a3*((x)[7][2]-(x)[3][2]); }

#define TRANSFORMATION_3D(n,x,local,M)                                     \
 {if ((n) == 4)      {TRANSFORMATION_OF_TETRAHEDRON((x),(M));}          \
  else if ((n) == 5) {TRANSFORMATION_OF_PYRAMID((x),(local),(M));}      \
  else if ((n) == 6) {TRANSFORMATION_OF_PRISM((x),(local),(M));}        \
  else TRANSFORMATION_OF_HEXAHEDRON((x),(local),(M));}


#ifdef __TWODIM__
#define LOCAL_TO_GLOBAL(n,x,local,global)                       LOCAL_TO_GLOBAL_2D(n,x,local,global)
#define AREA_OF_ELEMENT(n,x,area)                                       AREA_OF_ELEMENT_2D(n,x,area)
#define TRANSFORMATION(n,x,local,M)                                     TRANSFORMATION_2D(n,x,local,M)
#endif /* __TWODIM__ */

#ifdef __THREEDIM__
#define TRANSFORMATION_BND(n,x,local,M)                         TRANSFORMATION_2D(n,x,local,M)
#define LOCAL_TO_GLOBAL_BND(n,x,local,global)           LOCAL_TO_GLOBAL_2D(n,x,local,global)
#define AREA_OF_ELEMENT_BND(n,x,area)                           AREA_OF_ELEMENT_2D(n,x,area)
#define AREA_OF_REF_BND(n,area)                                         AREA_OF_REF_2D(n,area)

#define LOCAL_TO_GLOBAL(n,x,local,global)                       LOCAL_TO_GLOBAL_3D(n,x,local,global)
#define AREA_OF_ELEMENT(n,x,area)                                       AREA_OF_ELEMENT_3D(n,x,area)
#define TRANSFORMATION(n,x,local,M)                                     TRANSFORMATION_3D(n,x,local,M)
#endif /* __THREEDIM__ */

#define INVERSE_TRANSFORMATION(n,x,local,Jinv,Jdet)   \
{   DOUBLE_VECTOR J[DIM];                             \
    TRANSFORMATION((n),(x),(local),J);                \
        M_DIM_INVERT(J,(Jinv),(Jdet)); }

/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

DOUBLE  *LMP                  (INT n);
INT      UG_GlobalToLocal     (INT n, const DOUBLE **Corners, const DOUBLE *EvalPoint, DOUBLE *LocalCoord);

#ifdef __THREEDIM__
DOUBLE  N                   (const INT i, const DOUBLE *LocalCoord);
INT     TetraSideNormals    (ELEMENT *theElement, DOUBLE **theCorners, DOUBLE_VECTOR theNormals[MAX_SIDES_OF_ELEM]);
INT     TetMaxSideAngle     (ELEMENT *theElement, const DOUBLE **theCorners, DOUBLE *MaxAngle);
INT     TetAngleAndLength   (ELEMENT *theElement, const DOUBLE **theCorners, DOUBLE *Angle, DOUBLE *Length);
#endif

END_UGDIM_NAMESPACE

#endif
