// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ugblas.c                                                      */
/*                                                                          */
/* Purpose:   basic linear algebra routines                                 */
/*            working on the matrix-vector and                              */
/*            matrix-blockvector structure                                  */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*                                                                          */
/* blockvector routines from:                                               */
/*            Christian Wrobel                                              */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*                                                                          */
/* email:     ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   06.03.95 begin, ug version 3.0                                */
/*            28.09.95 blockvector routines implemented (Christian Wrobel)  */
/*            22.08.03 corrections concering skip flags for many components */
/*                     in a vector data descriptor. Not adapted for the     */
/*                     block vectors, AMG and Galerkin approximations!      */
/*                     Switch on by macro _XXL_SKIPFLAGS_ (else not active).*/
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/* system include files                                                     */
/* application include files                                                */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include "ugtypes.h"
#include "architecture.h"
#include "misc.h"
#include "evm.h"
#include "gm.h"
#include "algebra.h"
#include <dev/ugdevices.h>
#include "general.h"
#include "debug.h"
#ifdef ModelP
#include "pargm.h"
#include "parallel.h"
#endif

#include "np.h"
#include "disctools.h"
#include "ugblas.h"
#include "blasm.h"

USING_UG_NAMESPACES
  using namespace PPIF;

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)        */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

#undef _XXL_SKIPFLAGS_

#define VERBOSE_BLAS    10

#define MATARRAYSIZE 512

/** @name Macros to define VEC_SCALAR, VECDATA_DESC and MATDATA_DESC components */
/*@{ */
#define DEFINE_VS_CMPS(a)                               DOUBLE a ## 0,a ## 1,a ## 2
#define DEFINE_VD_CMPS(x)                               INT x ## 0,x ## 1,x ## 2
#define DEFINE_MD_CMPS(m)                               INT m ## 00,m ## 01,m ## 02,m ## 10,m ## 11,m ## 12,m ## 20,m ## 21,m ## 22
/*@}*/

/** @name Macros to set VEC_SCALAR components */
/*@{ */
#define SET_VS_CMP_1(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]];}
#define SET_VS_CMP_2(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]]; a ## 1 = (A)[(off)[tp]+1];}
#define SET_VS_CMP_3(a,A,off,tp)                {a ## 0 = (A)[(off)[tp]]; a ## 1 = (A)[(off)[tp]+1]; a ## 2 = (A)[(off)[tp]+2];}
/*@}*/

/** @name Macros to set VECDATA_DESC components */
/*@{ */
#define SET_VD_CMP_1(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0);}
#define SET_VD_CMP_2(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0); x ## 1 = VD_CMP_OF_TYPE(v,tp,1);}
#define SET_VD_CMP_3(x,v,tp)                    {x ## 0 = VD_CMP_OF_TYPE(v,tp,0); x ## 1 = VD_CMP_OF_TYPE(v,tp,1); x ## 2 = VD_CMP_OF_TYPE(v,tp,2);}

#define SET_VD_CMP_N(x,v,tp)                    switch (VD_NCMPS_IN_TYPE(v,tp)) {case 1 : SET_VD_CMP_1(x,v,tp); break; \
                                                                               case 2 : SET_VD_CMP_2(x,v,tp); break; \
                                                                               case 3 : SET_VD_CMP_3(x,v,tp); break;}
/*@}*/

/** @name Macros to set MATDATA_DESC components */
/*@{ */
#define SET_MD_CMP_11(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0);}
#define SET_MD_CMP_12(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1);}
#define SET_MD_CMP_13(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2);}
#define SET_MD_CMP_21(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,1);}
#define SET_MD_CMP_22(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,2); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,3);}
#define SET_MD_CMP_23(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,3); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 12 = MD_MCMP_OF_RT_CT(M,rt,ct,5);}
#define SET_MD_CMP_31(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,2);}
#define SET_MD_CMP_32(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,2); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,3); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 21 = MD_MCMP_OF_RT_CT(M,rt,ct,5);}
#define SET_MD_CMP_33(m,M,rt,ct)                {m ## 00 = MD_MCMP_OF_RT_CT(M,rt,ct,0); m ## 01 = MD_MCMP_OF_RT_CT(M,rt,ct,1); m ## 02 = MD_MCMP_OF_RT_CT(M,rt,ct,2); \
                                                 m ## 10 = MD_MCMP_OF_RT_CT(M,rt,ct,3); m ## 11 = MD_MCMP_OF_RT_CT(M,rt,ct,4); m ## 12 = MD_MCMP_OF_RT_CT(M,rt,ct,5); \
                                                 m ## 20 = MD_MCMP_OF_RT_CT(M,rt,ct,6); m ## 21 = MD_MCMP_OF_RT_CT(M,rt,ct,7); m ## 22 = MD_MCMP_OF_RT_CT(M,rt,ct,8);}

#ifdef Debug
#define PRINTVEC(x)             {PrintDebug("contents of " STR(x) ":\n");PrintVectorX(GRID_ON_LEVEL(mg,tl),x,3,3,PrintDebug);}
#else
#define PRINTVEC(x)             {PrintDebug("contents of " STR(x) ":\n");PrintVectorX(GRID_ON_LEVEL(mg,tl),x,3,3,printf);}
#endif
/*@}*/

#define CEIL(n)          ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))


/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

#ifdef ModelP
static VECDATA_DESC *ConsVector;
static MATDATA_DESC *ConsMatrix;
static GRID *ConsGrid;
static INT MaximumInconsMatrices;
static MATRIX *MatArrayLocal[MATARRAYSIZE];
static MATRIX *MatArrayRemote[MATARRAYSIZE];
static INT MaxBlockSize;
static size_t DataSizePerVector;
static size_t DataSizePerMatrix;

#ifdef __TWODIM__
static INT max_vectors_of_type[NVECTYPES] =
{ MAX_CORNERS_OF_ELEM, MAX_EDGES_OF_ELEM, 1};
#endif

#ifdef __THREEDIM__
static INT max_vectors_of_type[NVECTYPES] =
{ MAX_CORNERS_OF_ELEM, MAX_EDGES_OF_ELEM, 1, MAX_SIDES_OF_ELEM};
#endif

#ifdef __BLOCK_VECTOR_DESC__
static const BV_DESC *ConsBvd;
static const BV_DESC_FORMAT *ConsBvdf;
static INT ConsComp;
#endif

#ifndef _XXL_SKIPFLAGS_
#define SKIP_CONT(skip,i) ((skip) & (1 << (i)))
#define SET_SKIP_CONT(v,i) (VECSKIP(v) |= (1 << (i)))
#else
#define SKIP_CONT(skip,i) ((i < sizeof (INT) * 8) ? \
                           (skip) & (1 << (i)) \
                           : (skip) & (1 << (sizeof (INT) * 8 - 1)))
#define SET_SKIP_CONT(v,i) (VECSKIP(v) |= ((i) < sizeof(INT) * 8) ? 1 << (i) \
                                          : 1 << (sizeof (INT) * 8 - 1))
#endif

#endif

static INT trace_ugblas=0;


REP_ERR_FILE

/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined           */
/*                                                                          */
/****************************************************************************/

INT NS_DIM_PREFIX TraceUGBlas (INT trace)
{
  return (trace_ugblas = trace);
}

/****************************************************************************/
/** \brief Check wether two VECDATA_DESCs match

 * @param x - vector data descriptor
 * @param y - vector data descriptor

   This function checks wether the two VECDATA_DESCs match.

   \return <ul>
   INT
   .n    NUM_OK if ok
   .n    NUM_DESC_MISMATCH if the type descriptors does not match
 */
/****************************************************************************/

INT NS_DIM_PREFIX VecCheckConsistency (const VECDATA_DESC *x, const VECDATA_DESC *y)
{
  INT vtype;

  for (vtype=0; vtype<NVECTYPES; vtype++)
    if (VD_ISDEF_IN_TYPE(x,vtype))
    {
      /* consistency check: the x-types should include the y-types */
      if (!VD_ISDEF_IN_TYPE(y,vtype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);

      /* consistency check: the x-nComp should be equal to the y-nComp */
      if (VD_NCMPS_IN_TYPE(x,vtype) != VD_NCMPS_IN_TYPE(y,vtype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);
    }
  return (NUM_OK);
}

/****************************************************************************/
/** \brief Check the consistency of the data descriptors

 * @param x - vector data descriptor
 * @param M - matrix data descriptor
 * @param y - vector data descriptor


   This function checks whether the VECDATA_DESCs and the MATDATA_DESC
   match.

   \return <ul>
   INT
   .n    NUM_OK if ok
   .n    NUM_DESC_MISMATCH if the type descriptors not match
   .n    NUM_BLOCK_TOO_LARGE if the blocks are larger as MAX_SINGLE_VEC_COMP
 */
/****************************************************************************/

INT NS_DIM_PREFIX MatmulCheckConsistency (const VECDATA_DESC *x, const MATDATA_DESC *M, const VECDATA_DESC *y)
{
  INT rtype,ctype,mtype,maxsmallblock;

  /* consistency check: the formats should match */
  maxsmallblock = 0;
  for (mtype=0; mtype<NMATTYPES; mtype++)
    if (MD_ISDEF_IN_MTYPE(M,mtype)>0)
    {
      rtype = MTYPE_RT(mtype);
      ctype = MTYPE_CT(mtype);
      if (MD_ROWS_IN_MTYPE(M,mtype) != VD_NCMPS_IN_TYPE(x,rtype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);
      if (MD_COLS_IN_MTYPE(M,mtype) != VD_NCMPS_IN_TYPE(y,ctype))
        REP_ERR_RETURN (NUM_DESC_MISMATCH);

      maxsmallblock = MAX(maxsmallblock, VD_NCMPS_IN_TYPE(x,rtype));
      maxsmallblock = MAX(maxsmallblock, VD_NCMPS_IN_TYPE(y,ctype));
    }

  /* check size of the largest small block, if too small:
     increase MAX_SINGLE_VEC_COMP and recompile */
  assert (maxsmallblock <= MAX_SINGLE_VEC_COMP);

        #ifdef NDEBUG
  /* check also in case NDEBUG is defined (assert off)	*/
  if (maxsmallblock > MAX_SINGLE_VEC_COMP)
    REP_ERR_RETURN (NUM_BLOCK_TOO_LARGE);
        #endif

  return (NUM_OK);
}

/****************************************************************************/
/* naming convention:                                                       */
/*                                                                          */
/* all names have the form                                                  */
/*                                                                          */
/* ?_function                                                               */
/*                                                                          */
/* where ? can be one of the letters:                                       */
/*                                                                          */
/* l	operation working on a grid level                                    */
/* s	operation working on all fine grid dof's (surface)                   */
/* a	operation working on all dofs on all levels                          */
/*                                                                          */
/* (blockvector routines see below in this file)                            */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/****************************************************************************/
/* first parallel routines                                                  */
/****************************************************************************/
/****************************************************************************/

#ifdef ModelP

int NS_DIM_PREFIX DDD_InfoPrioCopies (DDD_HDR hdr)
{
  INT i,n;
  int *proclist;

  if (DDD_InfoNCopies(hdr) == 0)
    return(0);

  proclist = DDD_InfoProcList(hdr);
  n = 0;
  for(i=2; proclist[i]>=0; i+=2)
    if (!GHOSTPRIO(proclist[i+1]))
      n++;

  return(n);
}

#endif /* ModelP */

/****************************************************************************/
/****************************************************************************/
/* end of parallel routines                                                 */
/****************************************************************************/
/****************************************************************************/
