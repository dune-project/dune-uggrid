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
static size_t DataSizePerElement;

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

static int Gather_VectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      *((DOUBLE *)data) = VVALUE(pv,VD_SCALCMP(ConsVector));

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_VectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type,vecskip;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      if (!VECSKIP(pv))
        VVALUE(pv,VD_SCALCMP(ConsVector)) += *((DOUBLE *)data);

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  vecskip = VECSKIP(pv);
  if (vecskip == 0)
    for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
      VVALUE(pv,Comp[i]) += ((DOUBLE *)data)[i];
  else
    for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
      if (! SKIP_CONT (vecskip, i))
        VVALUE(pv,Comp[i]) += ((DOUBLE *)data)[i];

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Builds the sum of the vector values on all copies

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function builds the sum of the vector values for all border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_vector_consistent (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_VectorComp);
  return (NUM_OK);
}


static int Scatter_VectorComp_noskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) += *((DOUBLE *)data);

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) += ((DOUBLE *)data)[i];

  return (NUM_OK);
}

/****************************************************************************/
/*D
   l_vector_minimum_noskip
      - stores the minimum of the vector values on master and all copies

   SYNOPSIS:
   INT l_vector_minimum_noskip (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
   .  g - pointer to grid
   .  x - vector data descriptor

   DESCRIPTION:
   This function finds and stores the minimum of the vector values of all border vectors

   \return <ul>
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
   D*/
/****************************************************************************/

static int Scatter_MinVectorComp_noskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = MIN( VVALUE(pv,VD_SCALCMP(ConsVector)),*((DOUBLE *)data) );

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = MIN( VVALUE(pv,Comp[i]) , ((DOUBLE *)data)[i] );

  return (NUM_OK);
}

INT l_vector_minimum_noskip (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_MinVectorComp_noskip);
  return (NUM_OK);
}


static int Scatter_MaxVectorComp_noskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = MAX( VVALUE(pv,VD_SCALCMP(ConsVector)) , *((DOUBLE *)data) );

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = MAX( VVALUE(pv,Comp[i]) , ((DOUBLE *)data)[i] );

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Stores the maximum of the vector values on master and all copies

   \param g - pointer to grid
   \param x - vector data descriptor

   This function finds and stores the maximum of the vector values of all border vectors

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT l_vector_maximum_noskip (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_MaxVectorComp_noskip);
  return (NUM_OK);
}

/****************************************************************************/
/** \brief
   l_vector_consistent_noskip - builds the sum of the vector values on all copies

   SYNOPSIS:
   INT l_vector_consistent_noskip (GRID *g, const VECDATA_DESC *x);

   PARAMETERS:
 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function builds the sum of the vector values for all border vectors.

   \return <ul>
   INT
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT l_vector_consistent_noskip (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_VectorComp_noskip);
  return (NUM_OK);
}

/****************************************************************************/
/** \brief Builds the sum of the vector values on all copies

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level
 * @param x - vector data descriptor


   This function builds the sum of the vector values for all border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX a_vector_consistent (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorComp, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorComp, Scatter_VectorComp);

  return (NUM_OK);
}

INT NS_DIM_PREFIX a_vector_consistent_noskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorComp, Scatter_VectorComp_noskip);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorComp, Scatter_VectorComp_noskip);

  return (NUM_OK);
}


#ifdef __BLOCK_VECTOR_DESC__
static int Gather_VectorCompBS (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;

  if( VMATCH(pv,ConsBvd, ConsBvdf) )
    /*{printf(PFMT"Gather_VectorCompBS: v[%d][%d] = %g\n",me,VINDEX(pv),ConsComp,VVALUE(pv,ConsComp));*/
    *((DOUBLE *)data) = VVALUE(pv,ConsComp);
  /*}*/
  return (NUM_OK);
}

static int Scatter_VectorCompBS (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;

  if( VMATCH(pv,ConsBvd, ConsBvdf) )
    /*{*/
    VVALUE(pv,ConsComp) += *((DOUBLE *)data);
  /*printf(PFMT"Scatter_VectorCompBS: v[%d][%d] = %g\n",me,VINDEX(pv),ConsComp,VVALUE(pv,ConsComp));}*/

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Builds the sum of the vector values within the blockvector on all copies

 * @param g - pointer to grid
 * @param bvd - description of the blockvector
 * @param bvdf - format to interpret bvd
 * @param x - vector data


   This function builds the sum of the vector values within the specified
   blockvector for all master and border vectors; the result is stored in all
   master and border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_vector_consistentBS (GRID *g, const BV_DESC *bvd, const BV_DESC_FORMAT *bvdf, INT x)
{
  ConsBvd = bvd;
  ConsBvdf = bvdf;
  ConsComp = x;

  ASSERT(g!=NULL);
  ASSERT(bvd!=NULL);
  ASSERT(bvdf!=NULL);

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), sizeof(DOUBLE),
                  Gather_VectorCompBS, Scatter_VectorCompBS);
  return (NUM_OK);
}
#endif


static int Scatter_GhostVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = *((DOUBLE *)data);

    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i];

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Copy values of masters to ghosts

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function copies the vector values of master vectors to ghost vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_ghostvector_consistent (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_VectorComp, Scatter_GhostVectorComp);

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Makes horizontal ghosts consistent

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level
 * @param x - vector data descriptor


   This function copies the vector values on the master vectors to the
   horizontal ghosts.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX a_outervector_consistent (MULTIGRID *mg, INT fl, INT tl,
                                            const VECDATA_DESC *x)
{
  INT tp,m,level;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(OuterVectorIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorComp, Scatter_GhostVectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(OuterVectorIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)), IF_FORWARD,
                    m * sizeof(DOUBLE),
                    Gather_VectorComp, Scatter_GhostVectorComp);

  return (NUM_OK);
}



static int Gather_EData (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;

  memcpy(data,EDATA(pe),DataSizePerElement);

  return (0);
}

static int Scatter_EData (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;

  memcpy(EDATA(pe),data,DataSizePerElement);

  return (0);
}

/****************************************************************************/
/** \brief Makes element data  consistent

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level


   This function copies the element data field form all masters to the
   copy elements.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/
INT NS_DIM_PREFIX a_elementdata_consistent (MULTIGRID *mg, INT fl, INT tl)
{
  INT level;

  DataSizePerElement = EDATA_DEF_IN_MG(mg);
  if (DataSizePerElement <= 0) return(NUM_OK);

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(ElementVHIF, IF_FORWARD, DataSizePerElement,
                 Gather_EData, Scatter_EData);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(ElementVHIF,GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                    IF_FORWARD, DataSizePerElement,
                    Gather_EData, Scatter_EData);

  return (NUM_OK);
}



static INT DataSizePerNode;

static int Gather_NData (DDD_OBJ obj, void *data)
{
  NODE *pn = (NODE *)obj;

  memcpy(data,NDATA(pn),DataSizePerNode);

  return (0);
}

static int Scatter_NData (DDD_OBJ obj, void *data)
{
  NODE *pn = (NODE *)obj;

  memcpy(NDATA(pn),data,DataSizePerNode);

  return (0);
}

/****************************************************************************/
/** \brief Makes node data  consistent

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level


   This function adds the node data field form all borders and masters.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/
INT NS_DIM_PREFIX a_nodedata_consistent (MULTIGRID *mg, INT fl, INT tl)
{
  INT level;

  DataSizePerNode = NDATA_DEF_IN_MG(mg);
  if (DataSizePerNode <= 0) return(NUM_OK);

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderNodeSymmIF, DataSizePerNode,
                   Gather_NData, Scatter_NData);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderNodeSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)), DataSizePerNode,
                      Gather_NData, Scatter_NData);

  return (NUM_OK);
}


static int Gather_ProjectVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  NODE *theNode;
  INT i,type;
  const SHORT *Comp;

  ((INT *)data)[0] = 1;
  if (VOTYPE(pv) == NODEVEC) {
    theNode = SONNODE(VMYNODE(pv));
    if (theNode != NULL)
      if (MASTER(NVECTOR(theNode))
          || (PRIO(NVECTOR(theNode)) == PrioBorder))
        ((INT *)data)[0] = 0;
  }
  if (((INT *)data)[0])
    return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      ((DOUBLE *)data)[1] = VVALUE(pv,VD_SCALCMP(ConsVector));
    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i+1] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_ProjectVectorComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  if (((INT *)data)[0])
    return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = ((DOUBLE *)data)[1];

    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i+1];

  return (NUM_OK);
}
/****************************************************************************/
/** \brief Copy values of ghosts to masters

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function copies the vector values of master vectors to ghost vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_ghostvector_project (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));
  m++;

  DDD_IFAOneway(VectorVAllIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_ProjectVectorComp, Scatter_ProjectVectorComp);

  return (NUM_OK);
}



static int Gather_VectorCompCollect (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT vc,i,type;
  const SHORT *Comp;

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv)) {
      vc = VD_SCALCMP(ConsVector);
      *((DOUBLE *)data) = VVALUE(pv,vc);
      VVALUE(pv,vc) = 0.0;
    }
    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++) {
    ((DOUBLE *)data)[i] = VVALUE(pv,Comp[i]);
    VVALUE(pv,Comp[i]) = 0.0;
  }

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Collects the vector values of all copies

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function collects the sum of the vector values for all border vectors
   to the master vector.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/
INT NS_DIM_PREFIX l_vector_collect (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(BorderVectorIF, GRID_ATTR(g), IF_FORWARD, m * sizeof(DOUBLE),
                Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Collect the vector values of all copies

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level
 * @param x - vector data descriptor


   This function collects the sum of the vector values for all border vectors
   to the master vector.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX a_vector_collect (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(BorderVectorIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorCompCollect, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(BorderVectorIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                    IF_FORWARD, m * sizeof(DOUBLE),
                    Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}

INT NS_DIM_PREFIX a_vector_collect_noskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(BorderVectorIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorCompCollect, Scatter_VectorComp_noskip);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(BorderVectorIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                    IF_FORWARD, m * sizeof(DOUBLE),
                    Gather_VectorCompCollect, Scatter_VectorComp_noskip);

  return (NUM_OK);
}


static int Gather_VectorVecskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  const SHORT *Comp;

  ((DOUBLE *) data)[0] = VECSKIP(pv);
  if (VECSKIP(pv) == 0) return (NUM_OK);
  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      ((DOUBLE *)data)[1] = VVALUE(pv,VD_SCALCMP(ConsVector));
    return (NUM_OK);
  }

  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    ((DOUBLE *)data)[i+1] = VVALUE(pv,Comp[i]);

  return (NUM_OK);
}

static int Scatter_VectorVecskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  UINT vecskip;
  const SHORT *Comp;

  vecskip = ((DOUBLE *) data)[0];
  if (vecskip == 0) return (NUM_OK);

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      if (vecskip) {
        if (VECSKIP(pv))
          VVALUE(pv,VD_SCALCMP(ConsVector)) = MAX(VVALUE(pv,VD_SCALCMP(ConsVector)),((DOUBLE *)data)[1]);
        else {
          VVALUE(pv,VD_SCALCMP(ConsVector)) = ((DOUBLE *)data)[1];
          VECSKIP(pv) = 1;
        }
      }
    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    if (SKIP_CONT (vecskip, i)) {
      if (SKIP_CONT (VECSKIP(pv), i))
        VVALUE(pv,Comp[i]) = MAX(VVALUE(pv,Comp[i]),((DOUBLE *)data)[i+1]);
      else {
        VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i+1];
        SET_SKIP_CONT (pv, i);
      }
    }

  return (NUM_OK);
}

static int Scatter_GhostVectorVecskip (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  INT i,type;
  UINT vecskip;
  const SHORT *Comp;

  vecskip = ((DOUBLE *) data)[0];
  VECSKIP(pv) = vecskip;
  if (vecskip == 0) return (NUM_OK);

  if (VD_IS_SCALAR(ConsVector)) {
    if (VD_SCALTYPEMASK(ConsVector) & VDATATYPE(pv))
      VVALUE(pv,VD_SCALCMP(ConsVector)) = ((DOUBLE *)data)[1];
    return (NUM_OK);
  }
  type = VTYPE(pv);
  Comp = VD_CMPPTR_OF_TYPE(ConsVector,type);
  for (i=0; i<VD_NCMPS_IN_TYPE(ConsVector,type); i++)
    if (SKIP_CONT (vecskip, i))
      VVALUE(pv,Comp[i]) = ((DOUBLE *)data)[i+1];

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Checks vecskip flags

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level
 * @param x - vector data descriptor


   This function checks the vecskip flags and exchanges Dirichlet values.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX a_vector_vecskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  m++;

  PRINTDEBUG(np,1,("%d: a_vector_vecskip begin  %d %d\n",me,fl,tl));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorVecskip, Scatter_VectorVecskip);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorVecskip, Scatter_VectorVecskip);

  PRINTDEBUG(np,1,("%d: a_vector_vecskip med %d %d\n",me,fl,tl));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFOneway(VectorVIF, IF_FORWARD, m * sizeof(DOUBLE),
                 Gather_VectorVecskip, Scatter_GhostVectorVecskip);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAOneway(VectorVIF,
                    GRID_ATTR(GRID_ON_LEVEL(mg,level)), IF_FORWARD,
                    m * sizeof(DOUBLE),
                    Gather_VectorVecskip, Scatter_GhostVectorVecskip);

  PRINTDEBUG(np,1,("%d: a_vector_vecskip end %d %d\n",me,fl,tl));

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Collects the vector values of all copies

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function collects the sum of the vector values for all ghost vectors
   to the master vector.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_ghostvector_collect (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_BACKWARD, m * sizeof(DOUBLE),
                Gather_VectorCompCollect, Scatter_VectorComp);

  return (NUM_OK);
}


/* !!! */
static int Gather_MatrixCollect (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;
  DOUBLE *mptr[MAX_NODAL_VALUES*MAX_NODAL_VALUES];
  INT i,m;

  m = GetElementMPtrs(pe,ConsMatrix,mptr);
  if (m < 0)
    for (i=0; i<DataSizePerMatrix; i++)
      ((DOUBLE *)data)[i] = 0.0;
  else
    for (i=0; i<MIN(DataSizePerMatrix,m*m); i++) {
      ((DOUBLE *)data)[i] = *mptr[i];
      *mptr[i] = 0.0;
    }

  return (NUM_OK);
}

/* !!! */
static int Scatter_MatrixCollect (DDD_OBJ obj, void *data)
{
  ELEMENT *pe = (ELEMENT *)obj;
  DOUBLE *mptr[MAX_NODAL_VALUES*MAX_NODAL_VALUES];
  INT i,m;

  m = GetElementMPtrs(pe,ConsMatrix,mptr);
  if (m < 0)
    return (NUM_ERROR);
  for (i=0; i<MIN(DataSizePerMatrix,m*m); i++)
    *mptr[i] += ((DOUBLE *)data)[i];

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Collects ghostmatrix entries for Galerkin assembling

 * @param g - pointer to grid
 * @param A - matrix data descriptor


   This function collects the matrix entries of ghost elements.
   It is called in 'AssembleGalerkinByMatrix'.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_ghostmatrix_collect (GRID *g, const MATDATA_DESC *A)
{
  INT rtp,m;

  ConsMatrix = (MATDATA_DESC *)A;
  m = 0;
  for (rtp=0; rtp<NVECTYPES; rtp++)
    m += MD_NCMPS_IN_RT_CT(ConsMatrix,rtp,rtp) * max_vectors_of_type[rtp];
  m = MIN(m,MAX_NODAL_VALUES);
  DataSizePerMatrix = m * m;

  DDD_IFAOneway(ElementVIF, GRID_ATTR(g), IF_BACKWARD,
                DataSizePerMatrix * sizeof(DOUBLE),
                Gather_MatrixCollect, Scatter_MatrixCollect);

  return (NUM_OK);
}


/* !!! */
static int Gather_AMGMatrixCollect (DDD_OBJ obj, void *data)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i,mc,vtype,mtype,masc;
  const SHORT *Comp;

  *maxgid = 0;

  if (VSTART(pv) == NULL) {
    return (NUM_OK);
  }

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv)) {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=VSTART(pv); m!=NULL; m=MNEXT(m))
      {
        *msgbuf = MVALUE(m,mc);
        msgbuf++;

        gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
        (*maxgid)++;
      }
      m=VSTART(pv);
      MVALUE(m,mc) = 1.0;
      for (m=MNEXT(m); m!=NULL; m=MNEXT(m))
        MVALUE(m,mc) = 0.0;
      return (NUM_OK);
    }
  }

  vtype = VTYPE(pv);
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    mtype = MTP(vtype,MDESTTYPE(m));
    Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      msgbuf[i] = MVALUE(m,Comp[i]);
    msgbuf+=MaxBlockSize;

    gidbuf[*maxgid] = GID(MDEST(m));
    (*maxgid)++;
  }
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    mtype = MTP(vtype,MDESTTYPE(m));
    Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
    for (i=0; i<MD_COLS_IN_MTYPE(ConsMatrix,mtype)
         *MD_ROWS_IN_MTYPE(ConsMatrix,mtype); i++)
      MVALUE(m,Comp[i]) = 0.0;
  }

  return (NUM_OK);
}

/* !!! */
static int Scatter_AMGMatrixCollect (DDD_OBJ obj, void *data)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  INT igid = 0;
  int j,k,mc,vtype,mtype,ncomp,rcomp,vecskip,masc;
  const SHORT *Comp;


  if (VSTART(pv) == NULL) return (NUM_OK);
  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix)  & VDATATYPE(pv))
    {
      if (VECSKIP(pv) != 0) return (NUM_OK);
      mc = MD_SCALCMP(ConsMatrix);
      masc =MD_SCAL_CTYPEMASK(ConsMatrix);
      for (m=VSTART(pv); m!=NULL; m=MNEXT(m)) {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        if (igid<*maxgid && (gidbuf[igid]==dest)) {
          MVALUE(m,mc) += *msgbuf;
          msgbuf++;
          igid++;
        }
      }
    }
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  vecskip = VECSKIP(pv);
  rcomp = MD_ROWS_IN_MTYPE(ConsMatrix,MTP(vtype,vtype));
  if (vecskip == 0)
    for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      DDD_GID dest = GID(MDEST(m));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf+=MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        mtype = MTP(vtype,MDESTTYPE(m));
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        for (j=0; j<MD_COLS_IN_MTYPE(ConsMatrix,mtype)*rcomp; j++)
          MVALUE(m,Comp[j]) += msgbuf[j];
        msgbuf+=MaxBlockSize;
        igid++;
      }
    }
  else
    for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf+=MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        mtype = MTP(vtype,MDESTTYPE(m));
        ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        for (k=0; k<rcomp; k++)
          if (!(vecskip & (1<<k)))
            for (j=k*ncomp; j<(k+1)*ncomp; j++)
              MVALUE(m,Comp[j]) += msgbuf[j];
        msgbuf+=MaxBlockSize;
        igid++;
      }
    }

  IFDEBUG(np,2)
  igid = 0;
  msgbuf = (DOUBLE *)data;
  for (m=(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

    while (igid<*maxgid && (gidbuf[igid]<dest))  {
      msgbuf+=MaxBlockSize;
      igid++;
    }

    if (igid<*maxgid && (gidbuf[igid]==dest)) {
      printf("%d: " GID_FMT "->" GID_FMT ":",me,GID(pv),GID(MDEST(m)));
      mtype = MTP(vtype,MDESTTYPE(m));
      ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
      Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
      for (k=0; k<rcomp; k++)
        for (j=k*ncomp; j<(k+1)*ncomp; j++)
          printf(" %f",MVALUE(m,Comp[j]));
      msgbuf+=MaxBlockSize;
      igid++;
      printf("\n");
    }
  }
  ENDDEBUG

  return (NUM_OK);
}

static int sort_MatArray (const void *e1, const void *e2)
{
  MATRIX  *m1 = *((MATRIX **)e1);
  MATRIX  *m2 = *((MATRIX **)e2);
  DDD_GID g1 = DDD_InfoGlobalId(PARHDR(MDEST(m1)));
  DDD_GID g2 = DDD_InfoGlobalId(PARHDR(MDEST(m2)));

  if (g1<g2) return(-1);
  if (g1>g2) return(1);
  return (NUM_OK);
}


static int CountAndSortMatrices (DDD_OBJ obj)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  int n, j;

  /* sort MATRIX-list according to gid of destination vector */
  if (VSTART(pv) == NULL)
    return(0);
  n = 0;
  ASSERT(MDEST(VSTART(pv))==pv);

  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
    MatArrayRemote[n++] = m;
  if (n>1) {
    qsort(MatArrayRemote,MIN(n,MATARRAYSIZE),
          sizeof(MATRIX *),sort_MatArray);
    m=VSTART(pv);
    for(j=0; j<n; j++) {
      MNEXT(m) = MatArrayRemote[j];
      m = MNEXT(m);
    }
    MNEXT(m)=NULL;
  }
  n++;
  if (PRIO(pv) == PrioVGhost)
    if (MaximumInconsMatrices < n)
      MaximumInconsMatrices = n;

  return(0);
}

/* !!! */
/****************************************************************************/
/** \brief Collects ghostmatrix entries for AMG method

 * @param g - pointer to grid
 * @param A - matrix data descriptor


   This function collects the matrix entries of vertical ghosts on the
   first AMG-level stored on one processor.
   It is called in 'AMGAgglomerate'.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_amgmatrix_collect (GRID *g, const MATDATA_DESC *A)
{
  INT mt;
  size_t sizePerVector;

  PRINTDEBUG(np,2,("%3d: entering l_amgmatrix_collect...\n",me));
  PRINTDEBUG(np,2,("%3d: Gridlevel %d\n",me,GLEVEL(g)));

  ConsMatrix = (MATDATA_DESC *)A;
  MaxBlockSize = 0;
  for (mt=0; mt<NMATTYPES; mt++)
    MaxBlockSize = MAX(MaxBlockSize,MD_COLS_IN_MTYPE(ConsMatrix,mt)
                       *MD_ROWS_IN_MTYPE(ConsMatrix,mt));
  MaximumInconsMatrices=0;
  DDD_IFAExecLocal(VectorVIF, GRID_ATTR(g), CountAndSortMatrices);
  MaximumInconsMatrices = UG_GlobalMaxINT(MaximumInconsMatrices);
  DataSizePerVector = MaximumInconsMatrices * MaxBlockSize * sizeof(DOUBLE);
  DataSizePerVector = CEIL(DataSizePerVector);

  PRINTDEBUG(np,2,("%3d: MaximumInconsMatrices: %d\n",me,MaximumInconsMatrices));
  PRINTDEBUG(np,2,("%3d: MaxBlockSize: %d\n",me,MaxBlockSize));
  PRINTDEBUG(np,2,("%3d: DataSizePerVector: %d\n",me,DataSizePerVector));

  /* overall data sent per vector is its matrix entry data plus
     the number of valid entries plus a table of DDD-GIDs of
          destination vectors */
  sizePerVector = DataSizePerVector + sizeof(INT)
                  + MaximumInconsMatrices*sizeof(DDD_GID);
  sizePerVector = CEIL(sizePerVector);

  PRINTDEBUG(np,2,("%3d: sizePerVector: %d\n",me,sizePerVector));

  DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_BACKWARD, sizePerVector,
                Gather_AMGMatrixCollect, Scatter_AMGMatrixCollect);

  PRINTDEBUG(np,2,("%3d: exiting l_amgmatrix_collect...\n",me));

  return (NUM_OK);
}


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

static INT l_vector_average (GRID *g, const VECDATA_DESC *x)
{
  VECTOR *v;
  DOUBLE fac;
  INT vc,i,type,mask,n,m,vecskip;
  const SHORT *Comp;

  if (VD_IS_SCALAR(x)) {
    mask = VD_SCALTYPEMASK(x);
    vc = VD_SCALCMP(x);
    for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
      if ((VECSKIP(v) == 0) && (mask & VDATATYPE(v))) {
        m = DDD_InfoPrioCopies(PARHDR(v));
        if (m > 0)
          VVALUE(v,vc) *= 1.0 / (m+1.0);
      }
  }
  else
    for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v)) {
      type = VTYPE(v);
      n = VD_NCMPS_IN_TYPE(x,type);
      if (n == 0) continue;
      vecskip = VECSKIP(v);
      Comp = VD_CMPPTR_OF_TYPE(x,type);
      m = DDD_InfoPrioCopies(PARHDR(v));
      if (m == 0) continue;
      fac = 1.0 / (m + 1.0);
      if (vecskip == 0)
        for (i=0; i<n; i++)
          VVALUE(v,Comp[i]) *= fac;
      else
        for (i=0; i<n; i++)
          if (! SKIP_CONT (vecskip, i))
            VVALUE(v,Comp[i]) *= fac;
    }

  return(NUM_OK);
}

/****************************************************************************/
/** \brief Averages the vector values of all copies

 * @param g - pointer to grid
 * @param x - vector data descriptor


   This function builds the mean value of all vector values on border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX l_vector_meanvalue (GRID *g, const VECDATA_DESC *x)
{
  INT tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g), m * sizeof(DOUBLE),
                  Gather_VectorComp, Scatter_VectorComp);

  if (l_vector_average(g,x) != NUM_OK)
    REP_ERR_RETURN(NUM_ERROR);

  return (NUM_OK);
}

/****************************************************************************/
/** \brief Averages the vector values of all copies

 * @param mg - pointer to multigrid
 * @param fl - from level
 * @param tl - from level
 * @param x - vector data descriptor


   This function builds the mean value of all vector values on border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/

INT NS_DIM_PREFIX a_vector_meanvalue (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x)
{
  INT level,tp,m;

  ConsVector = (VECDATA_DESC *)x;

  m = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    m = MAX(m,VD_NCMPS_IN_TYPE(ConsVector,tp));

  if ((fl==BOTTOMLEVEL(mg)) && (tl==TOPLEVEL(mg)))
    DDD_IFExchange(BorderVectorSymmIF, m * sizeof(DOUBLE),
                   Gather_VectorComp, Scatter_VectorComp);
  else
    for (level=fl; level<=tl; level++)
      DDD_IFAExchange(BorderVectorSymmIF,
                      GRID_ATTR(GRID_ON_LEVEL(mg,level)),
                      m * sizeof(DOUBLE),
                      Gather_VectorComp, Scatter_VectorComp);

  for (level=fl; level<=tl; level++)
    if (l_vector_average(GRID_ON_LEVEL(mg,level),x) != NUM_OK)
      REP_ERR_RETURN(NUM_ERROR);

  return (NUM_OK);
}

static int Gather_DiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT i,vtype,mtype;
  SPARSE_MATRIX *sm;

  if (MD_IS_SCALAR(ConsMatrix)) {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      *((DOUBLE *)data) = MVALUE(VSTART(pv),MD_SCALCMP(ConsMatrix));
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  mtype = DMTP(vtype);
  m = VSTART(pv);
  sm = MD_SM(ConsMatrix,mtype);
  if (sm!=NULL)
    for (i=0; i<sm->N; i++)
      ((DOUBLE *)data)[i] = MVALUE(m, sm->offset[i]);

  return (NUM_OK);
}

static int Scatter_DiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT i,j,vtype,mtype,vecskip;
  SPARSE_MATRIX *sm;

  if (MD_IS_SCALAR(ConsMatrix))
  {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      if (!VECSKIP(pv))
        MVALUE(VSTART(pv), MD_SCALCMP(ConsMatrix)) += *((DOUBLE *)data);
    return (NUM_OK);
  }

  vtype = VTYPE(pv);
  mtype = DMTP(vtype);
  m = VSTART(pv);
  sm = MD_SM(ConsMatrix, mtype);
  vecskip = VECSKIP(pv);
  if (sm!=NULL)
  {
    for (i=0; i<sm->nrows; i++)
      if (! SKIP_CONT (vecskip, i))
        for (j=sm->row_start[i]; j<sm->row_start[i+1]; j++)
          MVALUE(m, sm->offset[j]) += ((DOUBLE *)data)[j];
  }

  return (NUM_OK);
}

/* !?! */
static int Scatter_GhostDiagMatrixComp (DDD_OBJ obj, void *data)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  INT i, vtype, mtype;
  SPARSE_MATRIX *sm;

  m = VSTART(pv);
  if (m == NULL)
    m = CreateExtraConnection(ConsGrid,pv,pv);
  if (m == NULL)
    return(1);

  if (MD_IS_SCALAR(ConsMatrix))
  {
    if (MD_SCAL_RTYPEMASK(ConsMatrix) & VDATATYPE(pv))
      MVALUE(m, MD_SCALCMP(ConsMatrix)) = *((DOUBLE *)data);
  }
  else
  {
    vtype = VTYPE(pv);
    mtype = DMTP(vtype);
    sm = MD_SM(ConsMatrix, mtype);
    if (sm!=NULL)
      for (i=0; i<sm->N; i++)
        MVALUE(m, sm->offset[i]) = ((DOUBLE *)data)[i];
  }

  return (NUM_OK);
}

/* !?! */
static int Gather_OffDiagMatrixComp (DDD_OBJ obj, void *data,
                                     DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i, *proclist,vtype,mtype;
  SPARSE_MATRIX *sm;

  *maxgid = 0;

  if (VSTART(pv) == NULL)
    return (NUM_OK);

  vtype = VTYPE(pv);
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      mtype = MTP(vtype, MDESTTYPE(m));
      sm = MD_SM(ConsMatrix, mtype);
      if (sm!=NULL)
        for (i=0; i<sm->N; i++)
          msgbuf[i] = MVALUE(m, sm->offset[i]);
      msgbuf += MaxBlockSize;

      gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
      (*maxgid)++;
    }
  }

  return (NUM_OK);
}

/* !?! */
static int Gather_OffDiagMatrixCompCollect (DDD_OBJ obj, void *data,
                                            DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  int i, *proclist,vtype,mtype;
  SPARSE_MATRIX *sm;

  *maxgid = 0;

  if (VSTART(pv) == NULL)
    return (NUM_OK);

  vtype = VTYPE(pv);
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      mtype = MTP(vtype,MDESTTYPE(m));
      sm = MD_SM(ConsMatrix, mtype);
      if (sm!=NULL)
      {
        for (i=0; i<sm->N; i++)
        {
          msgbuf[i] = MVALUE(m, sm->offset[i]);
          MVALUE(m, sm->offset[i]) = 0.0;
        }
        msgbuf+=MaxBlockSize;

        gidbuf[*maxgid] = DDD_InfoGlobalId(PARHDR(MDEST(m)));
        (*maxgid)++;
      }
    }
  }

  return (NUM_OK);
}

/* !?! */
static int Scatter_OffDiagMatrixComp (DDD_OBJ obj, void *data,
                                      DDD_PROC proc, DDD_PRIO prio)
{
  VECTOR  *pv = (VECTOR *)obj;
  MATRIX  *m;
  DOUBLE  *msgbuf = (DOUBLE *)           data;
  INT     *maxgid = (INT *)    (((char *)data)+DataSizePerVector);
  DDD_GID *gidbuf = (DDD_GID *)(((char *)data)+DataSizePerVector+sizeof(INT));
  INT igid = 0;
  int i,j,k, *proclist,vtype,mtype,ncomp,rcomp,vecskip;
  const SHORT *Comp;
  SPARSE_MATRIX *sm;

  PRINTDEBUG(np,2,("%d: Scatter_OffDiagMatrixComp %d: maxgid %d\n",
                   me,GID(pv),*maxgid));

  if (VSTART(pv) == NULL)
    return (NUM_OK);

  vtype = VTYPE(pv);
  vecskip = VECSKIP(pv);
  rcomp = MD_ROWS_IN_MTYPE(ConsMatrix,MTP(vtype,vtype));
  if (vecskip == 0)
  {
    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      if (XFERMATX(m)==0) break;
      proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for (i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
        ;
      if (((DDD_PROC)proclist[i])==proc &&
          (!GHOSTPRIO(proclist[i+1])))
      {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        while (igid<*maxgid && (gidbuf[igid]<dest))
        {
          msgbuf+=MaxBlockSize;
          igid++;
        }

        if (igid<*maxgid && (gidbuf[igid]==dest))
        {
          mtype = MTP(vtype,MDESTTYPE(m));
          sm = MD_SM(ConsMatrix, mtype);
          if (sm!=NULL)
            for (j=0; j<sm->N; j++)
              MVALUE(m,sm->offset[j]) += msgbuf[j];
          msgbuf += MaxBlockSize;
          igid++;
        }
      }
    }
  }
  else
  {
    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
      if (XFERMATX(m)==0) break;
      proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
        ;
      if (((DDD_PROC)proclist[i])==proc &&
          (!GHOSTPRIO(proclist[i+1])))
      {
        DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

        while (igid<*maxgid && (gidbuf[igid]<dest))
        {
          msgbuf += MaxBlockSize;
          igid++;
        }

        if (igid<*maxgid && (gidbuf[igid]==dest))
        {
          mtype = MTP(vtype,MDESTTYPE(m));
          sm = MD_SM(ConsMatrix, mtype);

          if (sm!=NULL)
            for (k=0; k<sm->nrows; k++)
              if (! SKIP_CONT (vecskip, k))
                for (j=sm->row_start[k]; j<sm->row_start[k+1]; j++)
                  MVALUE(m, sm->offset[j]) += msgbuf[j];

          msgbuf += MaxBlockSize;
          igid++;
        }
      }
    }
  }

  IFDEBUG(np,2)
  igid = 0;
  msgbuf = (DOUBLE *)data;
  for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m)) {
    if (XFERMATX(m)==0) break;
    proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
    for(i=2; proclist[i]>=0 && ((DDD_PROC)proclist[i])!=proc; i+=2)
      ;
    if (((DDD_PROC)proclist[i])==proc &&
        (!GHOSTPRIO(proclist[i+1])))
    {
      DDD_GID dest = DDD_InfoGlobalId(PARHDR(MDEST(m)));

      while (igid<*maxgid && (gidbuf[igid]<dest))
      {
        msgbuf += MaxBlockSize;
        igid++;
      }

      if (igid<*maxgid && (gidbuf[igid]==dest))
      {
        printf("%d: " GID_FMT "->" GID_FMT ":",me,GID(pv),GID(MDEST(m)));
        mtype = MTP(vtype,MDESTTYPE(m));
        sm = MD_SM(ConsMatrix, mtype);
        ncomp = MD_COLS_IN_MTYPE(ConsMatrix,mtype);
        Comp = MD_MCMPPTR_OF_MTYPE(ConsMatrix,mtype);
        if (sm!=NULL)
          for (k=0; k<sm->nrows; k++)
            for (j=sm->row_start[k]; j<sm->row_start[k+1]; j++)
              printf(" %f", MVALUE(m, sm->offset[j]));
        msgbuf += MaxBlockSize;
        igid++;
        printf("\n");
      }
    }
  }
  ENDDEBUG

  return (NUM_OK);
}

static int PrepareCountAndSortInconsMatrices (DDD_OBJ obj)
/* set VCUSED=1; thus each vector will be processed only once
   in CountAndSortInconsMatrices */
{
  VECTOR *pv = (VECTOR *)obj;

  SETVCUSED( pv, 1 );

  return 0;
}

static int CountAndSortInconsMatrices (DDD_OBJ obj)
{
  VECTOR *pv = (VECTOR *)obj;
  MATRIX *m;
  int nLocal, nRemote, j;

  /* process each vector only once */
  if( VCUSED(pv)==0 )
    return (0);                 /* already visited */
  SETVCUSED( pv, 0 );

  /* sort MATRIX-list according to gid of destination vector */
  nLocal=nRemote=0;
  if (VSTART(pv)!=NULL)
  {
    ASSERT(MDEST(VSTART(pv))==pv);

    for (m=MNEXT(VSTART(pv)); m!=NULL; m=MNEXT(m))
    {
      int i, *proclist = DDD_InfoProcList(PARHDR(MDEST(m)));
      for(i=0; proclist[i]>=0 && GHOSTPRIO(proclist[i+1]); i+=2)
        ;
      ASSERT(MDEST(m)!=pv);

      if (proclist[i]<0)
      {
        /* MDEST has only copies with PrioHGhost (if any) */
        MatArrayLocal[nLocal++] = m;
      }
      else
      {
        /* MDEST has copies != PrioHGhost */
        MatArrayRemote[nRemote++] = m;
      }
    }
  }
  if (nRemote>0)
  {
    if (nRemote>1)
      qsort(MatArrayRemote,MIN(nRemote,MATARRAYSIZE),sizeof(MATRIX *),sort_MatArray);

    m=VSTART(pv);
    for(j=0; j<nRemote; j++)
    {
      MNEXT(m) = MatArrayRemote[j];
      m = MNEXT(m);
      SETXFERMATX(m, 1);
    }
    for(j=0; j<nLocal; j++)
    {
      MNEXT(m) = MatArrayLocal[j];
      m = MNEXT(m);
      SETXFERMATX(m, 0);
    }
    MNEXT(m)=NULL;
  }
  else
  {
    if (VSTART(pv) != NULL)
      if (MNEXT(VSTART(pv))!=NULL)
        SETXFERMATX(MNEXT(VSTART(pv)), 0);
  }

  /* TODO: MaximumInconsMatrices ist eigentlich <nRemote, naemlich
          das maximum der teilmenge aus der matrixliste mit einer
          Kopie von MDEST auf processor proc, hier vernachlaessigt. */
  if (MaximumInconsMatrices<nRemote)
    MaximumInconsMatrices = nRemote;
  return (0);
}


/** \todo  perhaps it would make sense to have two routines,
        one for diagonal matrix entries only and the other for
        diag. and off-diag. matrix entries. */
/****************************************************************************/
/** \brief Builds the sum of the matrix values on all copies

 * @param g - pointer to grid
 * @param M - matrix data descriptor
 * @param mode - consistence of the diagonal (MAT_DIAG_CONS), the complete row for
          all master vectors (MAT_MASTER_CONS) or all (MAT_CONS)


   This function builds the sum of the matrix values for
   the matrix list of all border vectors.

   \return <ul>
   .n    NUM_OK      if ok
   .n    NUM_ERROR   if error occurrs
 */
/****************************************************************************/
INT NS_DIM_PREFIX l_matrix_consistent (GRID *g, const MATDATA_DESC *M, INT mode)
{
  INT mt;
  size_t sizePerVector;

  ConsMatrix = (MATDATA_DESC *)M;
  MaxBlockSize = 0;
  for (mt=0; mt<NMATTYPES; mt++)
    MaxBlockSize = MAX(MaxBlockSize,MD_COLS_IN_MTYPE(ConsMatrix,mt)
                       *MD_ROWS_IN_MTYPE(ConsMatrix,mt));

  PRINTDEBUG(np,2,("%2d: l_matrix_consistent mode\n",me,mode));

  /** \todo make consistency of diags and off-diags in one communication! */
  DDD_IFAExchange(BorderVectorSymmIF, GRID_ATTR(g),
                  MaxBlockSize*sizeof(DOUBLE),
                  Gather_DiagMatrixComp, Scatter_DiagMatrixComp);
  if (mode == MAT_DIAG_CONS) return (NUM_OK);

  if (mode == MAT_GHOST_DIAG_CONS) {
    ConsGrid = g;
    DDD_IFAOneway(VectorVIF, GRID_ATTR(g), IF_FORWARD,
                  MaxBlockSize * sizeof(DOUBLE),
                  Gather_DiagMatrixComp, Scatter_GhostDiagMatrixComp);
    return (NUM_OK);
  }

  /* now make off-diagonal entries consistent */
  MaximumInconsMatrices=0;
  DDD_IFAExecLocal(BorderVectorSymmIF, GRID_ATTR(g), PrepareCountAndSortInconsMatrices);
  DDD_IFAExecLocal(BorderVectorSymmIF, GRID_ATTR(g), CountAndSortInconsMatrices);
  MaximumInconsMatrices = UG_GlobalMaxINT(MaximumInconsMatrices);
  DataSizePerVector = MaximumInconsMatrices * MaxBlockSize * sizeof(DOUBLE);
  DataSizePerVector = CEIL(DataSizePerVector);

  /* overall data sent per vector is its matrix entry data plus
     the number of valid entries plus a table of DDD-GIDs of
          destination vectors */
  sizePerVector = DataSizePerVector +
                  sizeof(INT) + MaximumInconsMatrices*sizeof(DDD_GID);
  sizePerVector = CEIL(sizePerVector);

  if (mode == MAT_CONS) {
    PRINTDEBUG(np,2,("%d: MAT_CONS\n",me));
    DDD_IFAExchangeX(BorderVectorSymmIF, GRID_ATTR(g), sizePerVector,
                     Gather_OffDiagMatrixComp, Scatter_OffDiagMatrixComp);
  }
  else if (mode == MAT_MASTER_CONS)
  {
    DDD_IFAOnewayX(BorderVectorIF, GRID_ATTR(g),IF_FORWARD, sizePerVector,
                   Gather_OffDiagMatrixCompCollect,
                   Scatter_OffDiagMatrixComp);
  }

  return (NUM_OK);
}
#endif /* ModelP */

/****************************************************************************/
/****************************************************************************/
/* end of parallel routines                                                 */
/****************************************************************************/
/****************************************************************************/
