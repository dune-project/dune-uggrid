// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/*! \file evm.h
 * \ingroup gm
 */

/****************************************************************************/
/*                                                                          */
/* File:      evm.h                                                         */
/*                                                                          */
/* Purpose:   elementary vector manipulations, header for evm.c             */
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
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __EVM__
#define __EVM__

#include <cmath>

#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

#include "gm.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*   compile time constants defining static data size (i.e. arrays)         */
/*   other constants                                                        */
/*   macros                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* macros                                                                   */
/*                                                                          */
/****************************************************************************/


/* macros for 2D vector operations */
#define V2_LINCOMB(a,A,b,B,C)              {(C)[0] = (a)*(A)[0] + (b)*(B)[0];\
                                            (C)[1] = (a)*(A)[1] + (b)*(B)[1];}
#define V2_COPY(A,C)                               {(C)[0] = (A)[0];\
                                                    (C)[1] = (A)[1];}
#define V2_SUBTRACT(A,B,C)                         {(C)[0] = (A)[0] - (B)[0];\
                                                    (C)[1] = (A)[1] - (B)[1];}
#define V2_ADD(A,B,C)                              {(C)[0] = (A)[0] + (B)[0];\
                                                    (C)[1] = (A)[1] + (B)[1];}
#define V2_SCALE(c,C)                              {(C)[0] = (c)*(C)[0];\
                                                    (C)[1] = (c)*(C)[1];}
#define V2_VECTOR_PRODUCT(A,B,c)                (c) = (A)[0]*(B)[1] - (A)[1]*(B)[0];
#define V2_ISEQUAL(A,B)                                 ((std::abs((A)[0]-(B)[0])<SMALL_C)&&(std::abs((A)[1]-(B)[1])<SMALL_C))
#define V2_EUKLIDNORM(A,b)                              (b) = sqrt((double)((A)[0]*(A)[0]+(A)[1]*(A)[1]));
#define V2_EUKLIDNORM_OF_DIFF(A,B,b)    (b) = sqrt((double)(((A)[0]-(B)[0])*((A)[0]-(B)[0])+((A)[1]-(B)[1])*((A)[1]-(B)[1])));
#define V2_CLEAR(A)                                {(A)[0] = 0.0; (A)[1]= 0.0;}
#define V2_SCALAR_PRODUCT(A,B,c)                (c) = (A)[0]*(B)[0]+(A)[1]*(B)[1];
#define V2_SCAL_PROD(A,B)                               ((A)[0]*(B)[0]+(A)[1]*(B)[1])


/* macros for 2D matrix-vector operations */
#define M2_TIMES_V2(M,A,B)                         {(B)[0] = (M)[0]*(A)[0] + (M)[2]*(A)[1];\
                                                    (B)[1] = (M)[1]*(A)[0] + (M)[3]*(A)[1];}
#define MM2_TIMES_V2(M,A,B)                        {(B)[0] = (M)[0][0]*(A)[0] + (M)[0][1]*(A)[1];\
                                                    (B)[1] = (M)[1][0]*(A)[0] + (M)[1][1]*(A)[1];}

#define MT2_TIMES_V2(M,A,B)                        {(B)[0] = (M)[0][0]*(A)[0] + (M)[1][0]*(A)[1];\
                                                    (B)[1] = (M)[0][1]*(A)[0] + (M)[1][1]*(A)[1];}

#define MD2_TIMES_V2(M,A,B)                        {(B)[0] = (M)[0]*(A)[0];\
                                                    (B)[1] = (M)[1]*(A)[1];}

/* macros for matrix operations */
#define M2_DET(M)                                               ((M)[0]*(M)[3] - (M)[1]*(M)[2])

#define M2_INVERT(M,IM,det)                   \
  { DOUBLE invdet;                                  \
    det = (M)[0][0]*(M)[1][1]-(M)[1][0]*(M)[0][1];  \
    if (std::abs((det))<SMALL_D*SMALL_D) det= 0.;      \
    else {                                      \
      invdet = 1.0 / (det);                       \
      (IM)[0][0] =  (M)[1][1]*invdet;             \
      (IM)[1][0] = -(M)[1][0]*invdet;             \
      (IM)[0][1] = -(M)[0][1]*invdet;             \
      (IM)[1][1] =  (M)[0][0]*invdet;}}

/* macros for vector operations */
#define V3_LINCOMB(a,A,b,B,C)              {(C)[0] = (a)*(A)[0] + (b)*(B)[0];\
                                            (C)[1] = (a)*(A)[1] + (b)*(B)[1];\
                                            (C)[2] = (a)*(A)[2] + (b)*(B)[2];}
#define V3_COPY(A,C)                               {(C)[0] = (A)[0];\
                                                    (C)[1] = (A)[1];\
                                                    (C)[2] = (A)[2];}
#define V3_SUBTRACT(A,B,C)                         {(C)[0] = (A)[0] - (B)[0];\
                                                    (C)[1] = (A)[1] - (B)[1];\
                                                    (C)[2] = (A)[2] - (B)[2];}
#define V3_ADD(A,B,C)                              {(C)[0] = (A)[0] + (B)[0];\
                                                    (C)[1] = (A)[1] + (B)[1];\
                                                    (C)[2] = (A)[2] + (B)[2];}
#define V3_SCALE(c,C)                              {(C)[0] = (c)*(C)[0];\
                                                    (C)[1] = (c)*(C)[1];\
                                                    (C)[2] = (c)*(C)[2];}
#define V3_VECTOR_PRODUCT(A,B,C)           {(C)[0] = (A)[1]*(B)[2] - (A)[2]*(B)[1];\
                                            (C)[1] = (A)[2]*(B)[0] - (A)[0]*(B)[2];\
                                            (C)[2] = (A)[0]*(B)[1] - (A)[1]*(B)[0];}
#define V3_EUKLIDNORM(A,b)                              (b) = (sqrt((double)((A)[0]*(A)[0]+(A)[1]*(A)[1]+(A)[2]*(A)[2])));
#define V3_ISEQUAL(A,B)                                 ((std::abs((A)[0]-(B)[0])<SMALL_C)&&(std::abs((A)[1]-(B)[1])<SMALL_C)&&(std::abs((A)[2]-(B)[2])<SMALL_C))
#define V3_EUKLIDNORM_OF_DIFF(A,B,b)    (b) = (sqrt((double)(((A)[0]-(B)[0])*((A)[0]-(B)[0])+((A)[1]-(B)[1])*((A)[1]-(B)[1])+((A)[2]-(B)[2])*((A)[2]-(B)[2]))));
#define V3_CLEAR(A)                                {(A)[0] = 0.0; (A)[1]= 0.0; (A)[2] = 0.0;}
#define V3_SCALAR_PRODUCT(A,B,c)                (c) = ((A)[0]*(B)[0]+(A)[1]*(B)[1]+(A)[2]*(B)[2]);
#define V3_SCAL_PROD(A,B)                       ((A)[0]*(B)[0]+(A)[1]*(B)[1]+(A)[2]*(B)[2])

/* macros for matrix-vector operations */
#define M3_TIMES_V3(M,A,B)                         {(B)[0] = (M)[0]*(A)[0] + (M)[3]*(A)[1] + (M)[6]*(A)[2];\
                                                    (B)[1] = (M)[1]*(A)[0] + (M)[4]*(A)[1] + (M)[7]*(A)[2];\
                                                    (B)[2] = (M)[2]*(A)[0] + (M)[5]*(A)[1] + (M)[8]*(A)[2];}

#define MT3_TIMES_V3(M,A,B)                        {(B)[0] = (M)[0][0]*(A)[0] + (M)[1][0]*(A)[1] + (M)[2][0]*(A)[2];\
                                                    (B)[1] = (M)[0][1]*(A)[0] + (M)[1][1]*(A)[1] + (M)[2][1]*(A)[2];\
                                                    (B)[2] = (M)[0][2]*(A)[0] + (M)[1][2]*(A)[1] + (M)[2][2]*(A)[2];}

/* macros for matrix operations */
#define M3_DET(M)                                               ((M)[0]*(M)[4]*(M)[8] + (M)[1]*(M)[5]*(M)[6] + (M)[2]*(M)[3]*(M)[7] \
                                                                 -(M)[2]*(M)[4]*(M)[6] - (M)[0]*(M)[5]*(M)[7] - (M)[1]*(M)[3]*(M)[8])

#define M3_INVERT(M,IM,det)                       \
  { DOUBLE invdet;                                  \
    (det) = (M)[0][0]*(M)[1][1]*(M)[2][2]           \
            + (M)[0][1]*(M)[1][2]*(M)[2][0]               \
            + (M)[0][2]*(M)[1][0]*(M)[2][1]             \
            - (M)[0][2]*(M)[1][1]*(M)[2][0]           \
            - (M)[0][0]*(M)[1][2]*(M)[2][1]         \
            - (M)[0][1]*(M)[1][0]*(M)[2][2];      \
    if (std::abs((det))<SMALL_D*SMALL_D)                \
      return (1);                                    \
    invdet = 1.0 / (det);                           \
    (IM)[0][0] = ( (M)[1][1]*(M)[2][2] - (M)[1][2]*(M)[2][1]) * invdet;  \
    (IM)[0][1] = (-(M)[0][1]*(M)[2][2] + (M)[0][2]*(M)[2][1]) * invdet;  \
    (IM)[0][2] = ( (M)[0][1]*(M)[1][2] - (M)[0][2]*(M)[1][1]) * invdet;  \
    (IM)[1][0] = (-(M)[1][0]*(M)[2][2] + (M)[1][2]*(M)[2][0]) * invdet;  \
    (IM)[1][1] = ( (M)[0][0]*(M)[2][2] - (M)[0][2]*(M)[2][0]) * invdet;  \
    (IM)[1][2] = (-(M)[0][0]*(M)[1][2] + (M)[0][2]*(M)[1][0]) * invdet;  \
    (IM)[2][0] = ( (M)[1][0]*(M)[2][1] - (M)[1][1]*(M)[2][0]) * invdet;  \
    (IM)[2][1] = (-(M)[0][0]*(M)[2][1] + (M)[0][1]*(M)[2][0]) * invdet;  \
    (IM)[2][2] = ( (M)[0][0]*(M)[1][1] - (M)[0][1]*(M)[1][0]) * invdet;}

/* macros for exact solver (EX) */
#define EX_MAT(m,b,i,j)                 ((m)[2*(b)*(i) + (j)])

/****************************************************************************/
/*                                                                          */
/* typedef of DIM-routines                                                  */
/*                                                                          */
/****************************************************************************/

#ifdef UG_DIM_2

#define V_DIM_LINCOMB(a,A,b,B,C)                V2_LINCOMB(a,A,b,B,C)
#define V_DIM_COPY(A,C)                         V2_COPY(A,C)
#define V_DIM_SUBTRACT(A,B,C)                   V2_SUBTRACT(A,B,C)
#define V_DIM_ADD(A,B,C)                        V2_ADD(A,B,C)
#define V_DIM_SCALE(c,C)                        V2_SCALE(c,C)
#define V_DIM_ISEQUAL(A,B)                      V2_ISEQUAL(A,B)
#define V_DIM_EUKLIDNORM(A,b)                   V2_EUKLIDNORM(A,b)
#define V_DIM_EUKLIDNORM_OF_DIFF(A,B,b)         V2_EUKLIDNORM_OF_DIFF(A,B,b)
#define V_DIM_CLEAR(A)                          V2_CLEAR(A)
#define V_DIM_SCALAR_PRODUCT(A,B,c)             V2_SCALAR_PRODUCT(A,B,c)
#define V_DIM_SCAL_PROD(A,B)                    V2_SCAL_PROD(A,B)

#define MT_TIMES_V_DIM(M,A,B)                   MT2_TIMES_V2(M,A,B)
#define M_DIM_INVERT(M,IM,det)                  M2_INVERT(M,IM,det)

#endif

#ifdef UG_DIM_3

#define V_DIM_LINCOMB(a,A,b,B,C)                V3_LINCOMB(a,A,b,B,C)
#define V_DIM_COPY(A,C)                         V3_COPY(A,C)
#define V_DIM_SUBTRACT(A,B,C)                   V3_SUBTRACT(A,B,C)
#define V_DIM_ADD(A,B,C)                        V3_ADD(A,B,C)
#define V_DIM_SCALE(c,C)                        V3_SCALE(c,C)
#define V_DIM_ISEQUAL(A,B)                      V3_ISEQUAL(A,B)
#define V_DIM_EUKLIDNORM(A,b)                   V3_EUKLIDNORM(A,b)
#define V_DIM_EUKLIDNORM_OF_DIFF(A,B,b)         V3_EUKLIDNORM_OF_DIFF(A,B,b)
#define V_DIM_CLEAR(A)                          V3_CLEAR(A)
#define V_DIM_SCALAR_PRODUCT(A,B,c)             V3_SCALAR_PRODUCT(A,B,c)
#define V_DIM_SCAL_PROD(A,B)                    V3_SCAL_PROD(A,B)

#define MT_TIMES_V_DIM(M,A,B)                   MT3_TIMES_V3(M,A,B)
#define M_DIM_INVERT(M,IM,det)                  M3_INVERT(M,IM,det)

#endif


/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* 2D routines */
DOUBLE          vp                                                                      (const DOUBLE x1, const DOUBLE y1, const DOUBLE x2, const DOUBLE y2);
DOUBLE          c_tarea                                                         (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2);
DOUBLE          c_qarea                                                         (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2, const DOUBLE *x3);
DOUBLE          V_te                                                            (const DOUBLE *x0, const DOUBLE *x1,
                                                                                 const DOUBLE *x2, const DOUBLE *x3);
DOUBLE          V_py                                                            (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2,
                                                                                 const DOUBLE *x3, const DOUBLE *x4);
DOUBLE          V_pr                                                            (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2,
                                                                                 const DOUBLE *x3, const DOUBLE *x4, const DOUBLE *x5);
DOUBLE          V_he                                                            (const DOUBLE *x0, const DOUBLE *x1, const DOUBLE *x2, const DOUBLE *x3,
                                                                                 const DOUBLE *x4, const DOUBLE *x5, const DOUBLE *x6, const DOUBLE *x7);


/* 3D routines */
INT             M3_Invert                                                       (DOUBLE *Inverse, const DOUBLE *Matrix);
INT             V3_Normalize                                                    (FieldVector<DOUBLE,3>& a);


/* volume calculations*/
DOUBLE GeneralElementVolume                            (INT tag, DOUBLE *x_co[]);
DOUBLE          ElementVolume                                           (const ELEMENT *elem);

END_UGDIM_NAMESPACE

#endif
