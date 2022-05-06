// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file algebra.c
 * \ingroup gm
 */

/** \addtogroup gm
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      algebra.c                                                     */
/*                                                                          */
/* Purpose:   management for algebraic structures                           */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 294                                       */
/*            6900 Heidelberg                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* blockvector data structure:                                              */
/*           Christian Wrobel                                               */
/*           Institut fuer Computeranwendungen III                          */
/*           Universitaet Stuttgart	                                        */
/*           Pfaffenwaldring 27                                             */
/*           70569 Stuttgart                                                */
/* email:    ug@ica3.uni-stuttgart.de                                       */
/*                                                                          */
/* History:  1.12.93 begin, ug 3d                                           */
/*           26.10.94 begin combination 2D/3D version                       */
/*           28.09.95 blockvector implemented (Christian Wrobel)            */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* defines to exclude functions                                             */
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
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/fifo.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/ugdevices.h>

#include "algebra.h"
#include "cw.h"
#include "dlmgr.h"
#include "gm.h"
#include "elements.h"
#include "refine.h"
#include "ugm.h"
#include "evm.h"
#include "dlmgr.h"

#ifdef ModelP
#include <dune/uggrid/parallel/dddif/parallel.h>
#endif


USING_UG_NAMESPACE
USING_UGDIM_NAMESPACE
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

/** \brief For GetDomainPart indicating an element is meant rather than an element side */
#define NOSIDE          -1

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

#ifdef UG_DIM_2
static MULTIGRID *GBNV_mg;                      /* multigrid							*/
static INT GBNV_MarkKey;                        /* key for Mark/Release					*/
#endif
static INT GBNV_n;                                      /* list items							*/
static INT GBNV_curr;                           /* curr pos								*/
static VECTOR **GBNV_list=NULL;         /* list pointer							*/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

/* for LexOrderVectorsInGrid */
static DOUBLE InvMeshSize;


#ifdef ModelP
INT NS_DIM_PREFIX GetVectorSize (GRID *theGrid, INT VectorObjType, GEOM_OBJECT *object)
{
#ifdef UG_DIM_3
  return sizeof(double);
#else
  return 0;
#endif
}
#endif

/****************************************************************************/
/** \brief  Return pointer to a new vector structure
 *
 * @param  theGrid - grid where vector should be inserted
 * @param  DomPart - part of the domain where vector is created
 * @param  ObjType - one of the types defined in gm
 * @param  object  - associate vector with this object
 * @param  vHandle - handle of new vector, i.e. a pointer to a pointer where
                                a pointer to the new vector is placed.

   This function returns a pointer to a new vector structure.
   The vector type is determined by DomPart and ObjType
   First the free list is checked for a free entry, if none
   is available, a new structure is allocated from the heap.

 * @return <ul>
 *   <li>     0 if ok  </li>
 *   <li>     1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

static INT CreateVectorInPart (GRID *theGrid, INT DomPart, VectorType VectorObjType,
                               GEOM_OBJECT *object, VECTOR **vHandle)
{
  MULTIGRID *theMG;
  VECTOR *pv;
  INT ds, Size, vtype;

  *vHandle = NULL;

  theMG = MYMG(theGrid);
#ifdef UG_DIM_3
  vtype = SIDEVEC;
#else
  vtype = NOVTYPE;
#endif
  ds = FMT_S_VEC_TP(MGFORMAT(theMG),vtype);
  if (ds == 0)
    return (0);                         /* HRR: this is ok now, no XXXXVEC in part of the domain */

  Size = sizeof(VECTOR)-sizeof(DOUBLE)+ds;
  pv = (VECTOR *)GetMemoryForObject(theMG,Size,VEOBJ);
  if (pv==NULL)
    REP_ERR_RETURN(1);

  /* initialize data */
  SETOBJT(pv,VEOBJ);
  SETVTYPE(pv,vtype);
  SETVPART(pv,DomPart);
  ds = VPART(pv);
  if (ds!=DomPart)
    return (1);
  SETVDATATYPE(pv,BITWISE_TYPE(vtype));
  SETVOTYPE(pv,VectorObjType);
  SETVCLASS(pv,3);
  SETVNCLASS(pv,0);
  SETVBUILDCON(pv,1);
  SETVNEW(pv,1);
  /* SETPRIO(dddContext, pv,PrioMaster); */

#ifndef ModelP
  // Dune uses the id field for face indices in sequential grids
  pv->id = (theGrid->mg->vectorIdCounter)++;
#endif

        #ifdef ModelP
  DDD_AttrSet(PARHDR(pv),GRID_ATTR(theGrid));
        #endif

  VOBJECT(pv) = object;
  VINDEX(pv) = (long)NVEC(theGrid);
  SUCCVC(pv) = FIRSTVECTOR(theGrid);
  VSTART(pv) = NULL;

  GRID_LINK_VECTOR(theGrid,pv,PrioMaster);

  *vHandle = pv;

  return (0);
}

INT NS_DIM_PREFIX CreateSideVector (GRID *theGrid, INT side, GEOM_OBJECT *object, VECTOR **vHandle)
{
  *vHandle = NULL;

  if (CreateVectorInPart(theGrid,0 /*part*/,SIDEVEC,object,vHandle))
    REP_ERR_RETURN(1);

  SETVECTORSIDE(*vHandle,side);
  SETVCOUNT(*vHandle,1);

  return (0);
}

INT NS_DIM_PREFIX CreateElementList (GRID *theGrid, NODE *theNode, ELEMENT *theElement)
{
  ELEMENTLIST *pel;

  for (pel=NODE_ELEMENT_LIST(theNode); pel!=NULL; pel=NEXT(pel))
    if(pel->el==theElement)
      return(0);

  pel = (ELEMENTLIST *)GetMemoryForObject(theGrid->mg,
                                          sizeof(ELEMENTLIST),MAOBJ);
  if (pel == NULL)
    return(1);

  pel->next = NODE_ELEMENT_LIST(theNode);
  pel->el = theElement;
  NDATA(theNode) = (void *) pel;

  return(0);
}

/****************************************************************************/
/** \brief Remove vector from the data structure
 *
 * @param  theGrid - grid level where theVector is in.
 * @param  theVector - VECTOR to be disposed.

   This function removes vector from the data structure and places
   it in the free list.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeVector (GRID *theGrid, VECTOR *theVector)
{
  INT Size;

  if (theVector == NULL)
    return(0);

  /* now remove vector from vector list */
  GRID_UNLINK_VECTOR(theGrid,theVector);

  /* reset count flags */
  SETVCOUNT(theVector,0);


  /* delete the vector itself */
  Size = sizeof(VECTOR)-sizeof(DOUBLE)
         + FMT_S_VEC_TP(MGFORMAT(MYMG(theGrid)),VTYPE(theVector));
  if (PutFreeObject(theGrid->mg,theVector,Size,VEOBJ))
    RETURN(1);

  return(0);
}

/****************************************************************************/
/* \brief Change vector allocated with wrong part

 * @param  g - grid level where the element is in.
 * @param  elem - element of side vector
 * @param  side - element side
 * @param  vHandle - handle to side vector (inialized with old, may be changed)

   This changes a side vector which was allocated with the wrong part (maybe unknown for side vectors
   when creating an element).

 * @return <ul>
 *   <li>   GM_OK if ok
 *   <li>   GM_ERROR if error occured.
 */
/****************************************************************************/

INT NS_DIM_PREFIX ReinspectSonSideVector (GRID *g, ELEMENT *elem, INT side, VECTOR **vHandle)
{
  MULTIGRID *mg;
  VECTOR *vold,*vnew;
  INT partnew,partold,vtnew;

  mg  = MYMG(g);

  vold = *vHandle;

  /* check whether part has actually changed */
  partold = (vold!=NULL) ? VPART(vold) : BVPD_S2P(MG_BVPD(mg),SUBDOMAIN(elem));
  partnew = 0;
  if (partnew==partold)
    return (GM_OK);

  /* check whether size has actually changed */
    if (vold!=NULL)
    {
      /* just change part and type */
      SETVTYPE(vold,vtnew);
      SETVPART(vold,partnew);

      SETVBUILDCON(vold,1);
    }
    PRINTDEBUG(gm,1,("SIDEVEC (%d,%d): part and type\n",ID(elem),side));
    return (GM_OK);

  PRINTDEBUG(gm,1,("SIDEVEC (%d,%d): part, type and size\n",ID(elem),side));

  /* create new vector */
  if (CreateVectorInPart(g,partnew,SIDEVEC,(GEOM_OBJECT*)elem,&vnew))
    REP_ERR_RETURN(GM_ERROR);

  /* now we can dispose the old vector */
  if (DisposeVector(g,vold))
    REP_ERR_RETURN(GM_ERROR);

  *vHandle = vnew;

  return (GM_OK);
}

/****************************************************************************/
/** \brief Dispose one of two vectors associated to a face
 *
 * @param  theGrid - pointer to grid
 * @param  Elem0,side0 - first element and side
 * @param  Elem1,side1 - second element and side

   After grid refinement it may happen that two side vector are associated to a face
   in the grid.  The elements on both sides of the face each have their own side
   vector.  This is an inconsistent state -- there should be only one vector per face.
   This routine deletes one of the two side vectors and makes the data structure
   consistent again.  This is easier than only creating one unique side vector
   to begin with.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

#ifdef UG_DIM_3
INT NS_DIM_PREFIX DisposeDoubledSideVector (GRID *theGrid, ELEMENT *Elem0, INT Side0, ELEMENT *Elem1, INT Side1)
{
  VECTOR *Vector0, *Vector1;

  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    assert(NBELEM(Elem0,Side0)==Elem1 && NBELEM(Elem1,Side1)==Elem0);
    Vector0 = SVECTOR(Elem0,Side0);
    Vector1 = SVECTOR(Elem1,Side1);
    if (Vector0 == Vector1)
      return (0);
    if ((Vector0==NULL) || (Vector1==NULL))
      /* this is the case at boundaries between different parts
         the part not using vectors in SIDEs will not need a pointer
         to the side vector */
      return (0);
    assert(VCOUNT(Vector0)==1 && VCOUNT(Vector1)==1);
    assert(VSTART(Vector0)==NULL || VSTART(Vector1)==NULL);
    if (VSTART(Vector0)==NULL)
    {
      SET_SVECTOR(Elem0,Side0,Vector1);
      SETVCOUNT(Vector1,2);
      if (DisposeVector (theGrid,Vector0))
        RETURN (1);
    }
    else
    {
      SET_SVECTOR(Elem1,Side1,Vector0);
      SETVCOUNT(Vector0,2);
      if (DisposeVector (theGrid,Vector1))
        RETURN (1);
    }
    return (0);

  }

  RETURN (1);
}
#endif


INT NS_DIM_PREFIX DisposeElementFromElementList (GRID *theGrid, NODE *theNode,
                                                 ELEMENT *theElement)
{
  ELEMENTLIST *pel,*next;

  pel = NODE_ELEMENT_LIST(theNode);
  if (pel == NULL) return(0);
  if (pel->el == theElement) {
    NDATA(theNode) = (void *) pel->next;
    return(PutFreeObject(theGrid->mg,pel,sizeof(ELEMENTLIST),MAOBJ));
  }
  next = pel->next;
  while (next != NULL) {
    if (next->el == theElement) {
      pel->next = next->next;
      return(PutFreeObject(theGrid->mg,next,sizeof(ELEMENTLIST),MAOBJ));
    }
    pel = next;
    next = pel->next;
  }

  return(0);
}

INT NS_DIM_PREFIX DisposeElementList (GRID *theGrid, NODE *theNode)
{
  ELEMENTLIST *pel,*next;

  pel = NODE_ELEMENT_LIST(theNode);
  while (pel != NULL) {
    next = pel->next;
    if (PutFreeObject(theGrid->mg,pel,sizeof(ELEMENTLIST),MAOBJ))
      return(1);
    pel = next;
  }
  NDATA(theNode) = NULL;

  return(0);
}

/****************************************************************************/
/** \brief Get a pointer list to all side data

 * @param theElement - that element
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function gets a pointer array to all VECTORs in sides of the given element.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

#ifdef UG_DIM_3
INT NS_DIM_PREFIX GetVectorsOfSides (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  INT i;

  *cnt = 0;

  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    if (SVECTOR(theElement,i) != NULL)
      vList[(*cnt)++] = SVECTOR(theElement,i);

  IFDEBUG(gm,0)
  for (i=0; i<(*cnt); i++)
  {
    assert(vList[i] != NULL);
    assert(VOTYPE(vList[i]) == SIDEVEC);
  }
  ENDDEBUG

  return(GM_OK);
}
#endif

/****************************************************************************/
/** \brief Get a pointer list to all vector data of specified object type

 * @param theElement -  that element
 * @param type - fill array with vectors of this type
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function returns a pointer array to all VECTORs of the element of the specified type.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfOType (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList)
{
  switch (type)
  {
        #ifdef UG_DIM_3
  case SIDEVEC :
    return (GetVectorsOfSides(theElement,cnt,vList));
        #endif
  }
  RETURN (GM_ERROR);
}

/****************************************************************************/
/** \brief Remove vectors with non-matching type from list

 * @param dt - data type including all vtypes needed
 * @param vec - vector list
 * @param cnt  - number of vectors return in the list

   This function removes vectors with non-matching types from the list.

 * @return
 *   Number of remaining vectors in the list
 */
/****************************************************************************/

INT NS_DIM_PREFIX DataTypeFilterVList (INT dt, VECTOR **vec, INT *cnt)
{
  INT i,n;

  n = *cnt;
  *cnt = 0;
  for (i=0; i<n; i++)
    if (VDATATYPE(vec[i]) & dt)
      vec[(*cnt)++] = vec[i];

  return (*cnt);
}

/****************************************************************************/
/** \brief Get vector list including all vectors of specified vtypes

 * @param theElement - pointer to an element
 * @param dt  - data type including all vtypes needed
 * @param obj - flags for objects types needed
 * @param cnt - number of vectors return in the list
 * @param vec - vector list

   This function gets a list of vectors of the specified vtypes corresponding to an element.

 * @return <ul>
 *   <li>   GM_OK if ok </li>
 *   <li>   GM_ERROR if error occured </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfDataTypesInObjects (const ELEMENT *theElement, INT dt, INT obj, INT *cnt, VECTOR *VecList[])
{
  INT n;

  *cnt = n = 0;

    #ifdef UG_DIM_3
  if (obj & BITWISE_TYPE(SIDEVEC))
  {
    INT i;
    if (GetVectorsOfSides(theElement,&i,VecList+n) != GM_OK)
      return(GM_ERROR);
    n += i;
  }
    #endif

  *cnt = n;

  /* remove vectors of types not needed */
  DataTypeFilterVList(dt,VecList,cnt);

  return (GM_OK);
}



static void PrintVectorTriple (int i)
{
  VECTOR *vec0 = GBNV_list[i];
  VECTOR *vec1 = GBNV_list[i+1];
  VECTOR *vec2 = GBNV_list[i+2];
  [[maybe_unused]] VERTEX *vtx0 = MYVERTEX((NODE*)VOBJECT(vec0));
  [[maybe_unused]] VERTEX *vtx1 = MYVERTEX((NODE*)VOBJECT(vec1));
  [[maybe_unused]] VERTEX *vtx2 = MYVERTEX((NODE*)VOBJECT(vec2));
  PrintDebug("0: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec0),XC(vtx0),YC(vtx0));
  PrintDebug("1: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec1),XC(vtx1),YC(vtx1));
  PrintDebug("2: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec2),XC(vtx2),YC(vtx2));
}

/****************************************************************************/
/** \brief Prepare GetBoundaryNeighbourVectors

 * @param theGrid - grid level
 * @param MaxListLen - max size of neighbourhood and therefore max list lenght of the
   VecList array of GetBoundaryNeighbourVectors

   This function stores lists of boundary vector neighbourhoods in temp storage
   of the multigrid. The neighborhoods of the boundary vectors can be received
   via a call of GetBoundaryNeighbourVectors one by one.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   GetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX PrepareGetBoundaryNeighbourVectors (GRID *theGrid, INT *MaxListLen)
{
#ifdef UG_DIM_2
  ELEMENT *elem;
  VECTOR *v0,*v1;
  INT i;

  if (GBNV_list!=NULL)
    /* last process not finished properly by call of GetBoundaryNeighbourVectors */
    REP_ERR_RETURN(1);

  /* count boundary node vectors */
  GBNV_n = 0;

  /* allocate list storage: 3 pointers each */
  GBNV_mg = MYMG(theGrid);
  MarkTmpMem(MGHEAP(GBNV_mg),&GBNV_MarkKey);
  GBNV_list = (VECTOR **) GetTmpMem(MGHEAP(GBNV_mg),3*GBNV_n*sizeof(VECTOR *),GBNV_MarkKey);
  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  /* store offset in vector index field */
  i = 0;

  /* loop elements and fill neighbours */
  /* TODO: parallel also orphan(?) elements to be complete */
  for (elem=FIRSTELEMENT(theGrid); elem!=NULL; elem=SUCCE(elem))
    if (OBJT(elem)==BEOBJ)
      for (i=0; i<SIDES_OF_ELEM(elem); i++)
        if (ELEM_BNDS(elem,i)!=NULL)
        {
          /* 2D: two corners */
          v0 = NVECTOR(CORNER(elem,CORNER_OF_SIDE(elem,i,0)));
          v1 = NVECTOR(CORNER(elem,CORNER_OF_SIDE(elem,i,1)));

          ASSERT(GBNV_list[VINDEX(v0)]==v0);
          ASSERT(GBNV_list[VINDEX(v1)]==v1);
          GBNV_list[VINDEX(v0)+2] = v1;
          GBNV_list[VINDEX(v1)+1] = v0;
        }
  GBNV_curr = 0;

  /* this is simple in 2D: center, pred and succ in positive sense */
  *MaxListLen = 3;

  IFDEBUG(gm,2)
  PrintDebug("PrepareGetBoundaryNeighbourVectors:\n");
  for (i=0; i<GBNV_n; i++)
    PrintVectorTriple(3*i);
  ENDDEBUG

  return (0);

#endif /* UG_DIM_2 */

#ifdef UG_DIM_3
  /* 3D requires somewhat more work! */

  /* but it should be possible to create an oriented list
     of neighbours for each boundary vector */

  REP_ERR_RETURN (1);
#endif /* UG_DIM_3 */
}

/****************************************************************************/
/** \brief Reset current neighbourhood to begin of list

   This function resets the pointer to the current neighbourhood to the beginning of
   the list. GetBoundaryNeighbourVectors will start again with the first one.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   GetBoundaryNeighbourVectors, PrepareGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX ResetGetBoundaryNeighbourVectors (void)
{
  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  GBNV_curr = 0;
  return (0);
}

/****************************************************************************/
/** \brief Get a neighbourhood of boundary vectors

 * @param dt - datatypes of center vectors (bitwise)
 * @param obj - object types of center vectors (bitwise)
 * @param cnt - vector list length
 * @param VecList - vector list
 * @param end - if YES the currently returned list was the last one

   This function returns a neighbourhood of boundary vectors, center first and the
   remainder oriented in positiv sense. The boundary vector heighbourhood lists
   are created by a call of PrepareGetBoundaryNeighbourVectors. Use
   ResetGetBoundaryNeighbourVectors to begin again with the first
   neighbourhood and finish processing the boundary vectors with a call of
   FinishBoundaryNeighbourVectors which releases the temporary storage
   occupied by PrepareGetBoundaryNeighbourVectors..

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>
 * \sa
   PrepareGetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetBoundaryNeighbourVectors (INT dt, INT obj, INT *cnt, VECTOR *VecList[])
{
  VECTOR *vec;

  *cnt = 0;

  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  /* find next center vec matching data type */
  for (vec=GBNV_list[GBNV_curr]; GBNV_curr<3*GBNV_n; GBNV_curr+=3, vec=GBNV_list[GBNV_curr])
    if (BITWISE_TYPE(VTYPE(vec)) & dt)
      break;
  if (GBNV_curr>=3*GBNV_n)
    /* no (more) vector found */
    return (0);

  IFDEBUG(gm,2)
  PrintDebug("GetBoundaryNeighbourVectors:\n");
  PrintVectorTriple(GBNV_curr);
  ENDDEBUG

  /* vector, pre and succ in positive sense */
    VecList[(*cnt)++] = GBNV_list[GBNV_curr];
  VecList[(*cnt)++] = GBNV_list[GBNV_curr+1];
  VecList[(*cnt)++] = GBNV_list[GBNV_curr+2];

  /* move on to next position */
  GBNV_curr += 3;

  return (0);
}

/****************************************************************************/
/** \brief Finish processing of boundary vectors

   This function releases the temporary memory allocated by PrepareGetBoundaryNeighbourVectors
   from the multigrid heap.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   PrepareGetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, GetBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX FinishBoundaryNeighbourVectors ()
{
        #ifdef __TWODIM
  if (ReleaseTmpMem(MGHEAP(GBNV_mg)),GBNV_MarkKey)
    REP_ERR_RETURN(1);
        #endif

  GBNV_list = NULL;

  return (0);
}

/****************************************************************************/
/** \brief Get vector list

 * @param theGrid - pointer to a grid
 * @param theElement - pointer to an element
 * @param vec - vector list

   This function gets a list of vectors corresponding to an element.

 * @return <ul>
 *   <li>   number of components </li>
 *   <li>   -1 if error occured </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetAllVectorsOfElement (GRID *theGrid, ELEMENT *theElement, VECTOR **vec)
{
  INT cnt;

  cnt = 0;
    #ifdef UG_DIM_3
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    INT i;
    if (GetVectorsOfSides(theElement,&i,vec+cnt) == GM_ERROR)
      RETURN(-1);
    cnt += i;
  }
    #endif

  return (cnt);
}

/****************************************************************************/
/** \brief Get pointers to elements having a common side

 * @param theVector - given vector associated with a side of an element in 3D
 * @param Elements - array to be filled with two pointers of elements
 * @param Sides - array to be filled with side number within respective element

   Given a VECTOR associated with the side of an element, this function
   retrieves pointers to at most two elements having this side in common.
   If the side is part of the exterior boundary of the domain, then
   Elements[1] will be the NULL pointer.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    1 if error occured.
   </ul>
 */
/****************************************************************************/

#ifdef UG_DIM_3
INT NS_DIM_PREFIX GetElementInfoFromSideVector (const VECTOR *theVector, ELEMENT **Elements, INT *Sides)
{
  INT i;
  ELEMENT *theNeighbor;

  if (VOTYPE(theVector) != SIDEVEC)
    RETURN (1);
  Elements[0] = (ELEMENT *)VOBJECT(theVector);
  Sides[0] = VECTORSIDE(theVector);

  /* find neighbor */
  Elements[1] = theNeighbor = NBELEM(Elements[0],Sides[0]);
  if (theNeighbor == NULL) return (0);

  /* search the side */
  for (i=0; i<SIDES_OF_ELEM(theNeighbor); i++)
    if (NBELEM(theNeighbor,i) == Elements[0])
      break;

  /* found ? */
  if (i<SIDES_OF_ELEM(theNeighbor))
  {
    Sides[1] = i;
    return (0);
  }
  RETURN (1);
}
#endif

/****************************************************************************/
/** \brief Reset all USED flags in neighborhood of an element

 * @param theElement - given element
 * @param ActDepth - recursion depth
 * @param MaxDepth - end of recursion

   This function calls itself recursively and resets all USED flags in the
   neighborhood of depth MaxDepth of theElement. For the first call
   ActDepth should be zero.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT ResetUsedFlagInNeighborhood (ELEMENT *theElement, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0) SETUSED(theElement,0);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ResetUsedFlagInNeighborhood(NBELEM(theElement,i),ActDepth+1,MaxDepth)) RETURN (1);

  return (0);
}


#ifdef ModelP
static int Gather_VectorVNew (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  ((INT *)data)[0] = VNEW(theVector);

  return(0);
}

static int Scatter_VectorVNew (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNEW(theVector,MAX(VNEW(theVector),((INT *)data)[0]));

  return(0);
}

static int Scatter_GhostVectorVNew (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNEW(theVector,((INT *)data)[0]);

  return(0);
}
#endif

INT NS_DIM_PREFIX SetSurfaceClasses (MULTIGRID *theMG)
{
  GRID *theGrid;
  ELEMENT *theElement;        VECTOR *v;
  INT level,fullrefine;

  level = TOPLEVEL(theMG);
  if (level > 0) {
    theGrid = GRID_ON_LEVEL(theMG,TOPLEVEL(theMG));
    ClearVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (MinNodeClass(theElement)==3)
        SeedVectorClasses(theGrid,theElement);
    }
    PropagateVectorClasses(theGrid);
    theGrid = GRID_ON_LEVEL(theMG,0);
    ClearNextVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (MinNextNodeClass(theElement)==3)
        SeedNextVectorClasses(theGrid,theElement);
    }
    PropagateNextVectorClasses(theGrid);
  }
  for (level--; level>0; level--)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    ClearVectorClasses(theGrid);
    ClearNextVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement)) {
      if (MinNodeClass(theElement)==3)
        SeedVectorClasses(theGrid,theElement);
      if (MinNextNodeClass(theElement)==3)
        SeedNextVectorClasses(theGrid,theElement);
    }
    PropagateVectorClasses(theGrid);
    PropagateNextVectorClasses(theGrid);
  }

  fullrefine = TOPLEVEL(theMG);
  for (level=TOPLEVEL(theMG); level>=0; level--)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    for (v=PFIRSTVECTOR(theGrid); v!= NULL; v=SUCCVC(v)) {
      SETNEW_DEFECT(v,(VCLASS(v)>=2));
      SETFINE_GRID_DOF(v,((VCLASS(v)>=2)&&(VNCLASS(v)<=1)));
      if (FINE_GRID_DOF(v))
        fullrefine = level;
    }
  }
        #ifdef ModelP
  fullrefine = UG_GlobalMinINT(theMG->ppifContext(), fullrefine);
        #endif

  FULLREFINELEVEL(theMG) = fullrefine;

  return(0);
}

/****************************************************************************/
/** \brief Creates the algebra for a grid

 * @param theGrid - pointer to grid

   This function allocates VECTORs in all geometrical objects of the grid.

 * @return <ul>
 *   <li>    GM_OK if ok
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateAlgebra (MULTIGRID *theMG)
{
  GRID *g;
#ifdef UG_DIM_3
  VECTOR *nbvec;
  ELEMENT *nbelem;
  INT j,n;
#endif
  ELEMENT *elem;
  INT i;


  if (MG_COARSE_FIXED(theMG) == false) {
    for (i=0; i<=TOPLEVEL(theMG); i++) {
      g = GRID_ON_LEVEL(theMG,i);

      if (NVEC(g)>0)
        continue;                               /* skip this level */

      /* loop elements and element sides */
      for (elem=PFIRSTELEMENT(g); elem!=NULL; elem=SUCCE(elem)) {
        /* to tell GridCreateConnection to build connections */
        if (EMASTER(elem)) SETEBUILDCON(elem,1);

        /* side vectors */
#ifdef UG_DIM_3
          for (INT side=0; side<SIDES_OF_ELEM(elem); side++)
            if (SVECTOR(elem,side)==NULL) {
              VECTOR *vec;
              if (CreateSideVector (g,side,
                                    (GEOM_OBJECT *)elem,&vec))
                REP_ERR_RETURN (GM_ERROR);
              SET_SVECTOR(elem,side,vec);
            }
#endif
      }
    }
#ifdef UG_DIM_3
    /* dispose doubled side vectors */
      for (elem=PFIRSTELEMENT(g); elem!=NULL; elem=SUCCE(elem))
      {
        for(INT side=0; side<SIDES_OF_ELEM(elem); side++)
        {
          if(OBJT(elem)==BEOBJ)
          {
            if(INNER_SIDE(elem,side))
            {
              nbelem = NBELEM(elem,side);
              ASSERT(nbelem!=NULL);
              VECTOR *vec=SVECTOR(elem,side);
              n=0;
              for(j=0; j<SIDES_OF_ELEM(nbelem); j++)
              {
                nbvec=SVECTOR(nbelem,j);

                /* doubled sidevectors ? */
                if(NBELEM(nbelem,j)==elem)
                {
                  if(vec!=nbvec)
                  {
                    if(DisposeVector(g,nbvec))
                      REP_ERR_RETURN(GM_ERROR);
                    SET_SVECTOR(nbelem,j,vec);
                    SETVCOUNT(vec,2);                                                                     /* PB, 25 Sep 2005: changed from 1 to 2 */
                  }
                }
              }
              n++;
              ASSERT(n==1);
            }
          }else
          {
            nbelem = NBELEM(elem,side);
            ASSERT(nbelem!=NULL);
            VECTOR* vec=SVECTOR(elem,side);
            n=0;
            for(j=0; j<SIDES_OF_ELEM(nbelem); j++)
            {
              nbvec=SVECTOR(nbelem,j);
              ASSERT(nbvec!=NULL);

              /* doubled sidevectors ? */
              if(NBELEM(nbelem,j)==elem)
              {
                if(vec!=nbvec)
                {
                  if(DisposeVector(g,nbvec))
                    REP_ERR_RETURN(GM_ERROR);
                  SET_SVECTOR(nbelem,j,vec);
                  SETVCOUNT(vec,2);                                                               /* PB, 25 Sep 2005: changed from 1 to 2 */
                }
              }
            }
            n++;
            ASSERT(n==1);
          }
        }
      }
#endif
    MG_COARSE_FIXED(theMG) = true;
  }

  /* now we should be safe to clear the InsertElement face map */
  theMG->facemap.clear();

    #ifdef ModelP
  /* update VNEW-flags */
  auto& context = theMG->dddContext();
  const auto& dddctrl = ddd_ctrl(context);
  DDD_IFExchange(context,
                 dddctrl.BorderVectorSymmIF,sizeof(INT),
                 Gather_VectorVNew,Scatter_VectorVNew);
  DDD_IFOneway(context,
               dddctrl.VectorIF,IF_FORWARD,sizeof(INT),
               Gather_VectorVNew,Scatter_GhostVectorVNew);
    #endif

  SetSurfaceClasses(theMG);

  return(GM_OK);
}



INT NS_DIM_PREFIX PrepareAlgebraModification (MULTIGRID *theMG)
{
  int j,k;
  ELEMENT *theElement;
  VECTOR *theVector;

  j = theMG->topLevel;
  for (k=0; k<=j; k++)
  {
    for (theElement=PFIRSTELEMENT(GRID_ON_LEVEL(theMG,k)); theElement!=NULL; theElement=SUCCE(theElement))
    {
      SETUSED(theElement,0);
      SETEBUILDCON(theElement,0);
    }
    for (theVector=PFIRSTVECTOR(GRID_ON_LEVEL(theMG,k)); theVector!= NULL; theVector=SUCCVC(theVector))
      SETVBUILDCON(theVector,0);
    for (theVector=PFIRSTVECTOR(GRID_ON_LEVEL(theMG,k)); theVector!= NULL; theVector=SUCCVC(theVector))
    {
      SETVNEW(theVector,0);
    }
  }

  return (0);
}

/****************************************************************************/
/** \brief Check connection of two elements

 * @param theGrid -  grid level to check
 * @param Elem0,Elem1 - elements to be checked.
 * @param ActDepth - recursion depth
 * @param ConDepth - connection depth as provided by format.
 * @param MatSize - matrix sizes as provided by format

   This function recursively checks connection of two elements.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    != 0 if errors occured.
   </ul>
 */
/****************************************************************************/

static INT ElementElementCheck (GRID *theGrid, ELEMENT *Elem0, ELEMENT *Elem1, INT ActDepth, INT *ConDepth, INT *MatSize)
{
  INT cnt0,cnt1,i,j,itype,jtype,mtype,size;
  VECTOR *vec0[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];
  VECTOR *vec1[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];
  char msg[128];
  INT errors = 0;

  sprintf(msg, " ERROR: missing connection between elem0=" EID_FMTX " elem1=" EID_FMTX,
          EID_PRTX(Elem0),EID_PRTX(Elem1));

  cnt0 = GetAllVectorsOfElement(theGrid,Elem0,vec0);
  if (Elem0 == Elem1)
  {
    for (i=0; i<cnt0; i++)
    {
      itype=VTYPE(vec0[i]);
      for (j=0; j<cnt0; j++)
      {
        if (i==j)
        {
          mtype = DIAGMATRIXTYPE(itype);
          size  = MatSize[mtype];
        }
        else
        {
          jtype = VTYPE(vec0[j]);
          size  = MatSize[MATRIXTYPE(jtype,itype)];
          mtype = MATRIXTYPE(itype,jtype);
          if (MatSize[mtype]>size) size=MatSize[mtype];
        }
      }
    }
    return (errors);
  }

  cnt1 = GetAllVectorsOfElement(theGrid,Elem1,vec1);
  for (i=0; i<cnt0; i++)
  {
    itype=VTYPE(vec0[i]);
    for (j=0; j<cnt1; j++)
    {
      if (i==j)
      {
        mtype = DIAGMATRIXTYPE(itype);
        size  = MatSize[mtype];
      }
      else
      {
        jtype = VTYPE(vec1[j]);
        size  = MatSize[MATRIXTYPE(jtype,itype)];
        mtype = MATRIXTYPE(itype,jtype);
        if (MatSize[mtype]>size) size=MatSize[mtype];
      }
    }
  }

  return (errors);
}

static INT CheckNeighborhood (GRID *theGrid, ELEMENT *theElement, ELEMENT *centerElement, INT *ConDepth, INT ActDepth, INT MaxDepth, INT *MatSize)
{
  INT i,errors = 0;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if ((errors+=ElementElementCheck(theGrid,centerElement,theElement,ActDepth,ConDepth,MatSize))!=0)
      return (errors);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if ((errors+=CheckNeighborhood(theGrid,NBELEM(theElement,i),centerElement,ConDepth,ActDepth+1,MaxDepth,MatSize))!=0)
        return(errors);

  return (0);
}


/****************************************************************************/
/** \brief Checks validity of geom_object	and its vector

 * @param fmt - FORMAT of associated multigrid
 * @param s2p - subdomain to part table
 * @param theObject - the object which points to theVector
 * @param ObjectString - for error message
 * @param theVector - the vector of theObject
 * @param VectorObjType - NODEVEC,...
 * @param side - element side for SIDEVEC, NOSIDE else

   This function checks the consistency between an geom_object and
   its vector.

 * @return <ul>
 *   <li>    GM_OK if ok
 *   <li>    GM_ERROR	if error occured.
   </ul>
 */
/****************************************************************************/

static INT CheckVector (const INT s2p[], GEOM_OBJECT *theObject, const char *ObjectString,
                        VECTOR *theVector, INT VectorObjType, INT side)
{
  GEOM_OBJECT *VecObject;
  INT errors = 0,DomPart;

  if (theVector == NULL)
  {
    /* check if size is really 0 */
#ifdef UG_DIM_3
    VectorType vtype = SIDEVEC;
#else
    VectorType vtype = NOVTYPE;
#endif
    INT ds = FMT_S_VEC_TP(nullptr,vtype);
    if (ds>0)
    {
      errors++;
      UserWriteF("%s ID=%ld  has NO VECTOR", ObjectString,
                 ID(theObject));
      UserWrite("\n");
    }
  }
  else
  {
    INT ds = FMT_S_VEC_TP(fmt,VTYPE(theVector));
    if (ds==0)
    {
      errors++;
      UserWriteF("%s ID=%ld  exists but should not\n", ObjectString,
                 ID(theObject));
    }
    SETVCUSED(theVector,1);

    VecObject = VOBJECT(theVector);
    if (VecObject == NULL)
    {
      errors++;
      UserWriteF("vector=" VINDEX_FMTX " %s GID=" GID_FMT " has NO BACKPTR\n",
                 VINDEX_PRTX(theVector), ObjectString,
                 (OBJT(theObject)==BEOBJ || OBJT(theObject)==IEOBJ) ?
                 EGID(&(theObject->el)) : (OBJT(theObject)==NDOBJ) ?
                 GID(&(theObject->nd)) :
                 GID(&(theObject->ed))
                 );
    }
    else
    {
      if (VOTYPE(theVector) != VectorObjType)
      {
        errors++;
        UserWriteF("%s vector=" VINDEX_FMTX " has incompatible type=%d\n",
                   ObjectString, VINDEX_PRTX(theVector), VTYPE(theVector));
      }

      if (VecObject != theObject)
      {
        if (OBJT(VecObject) != OBJT(theObject))
        {
          int error = 1;

          /* both objects may be elements */
          if ((OBJT(VecObject)==BEOBJ || OBJT(VecObject)==IEOBJ) &&
              (OBJT(theObject)==BEOBJ || OBJT(theObject)==IEOBJ) )
          {
            ELEMENT *theElement = (ELEMENT *)theObject;
            ELEMENT *vecElement = (ELEMENT *)VecObject;
            int i;

                                                #ifdef ModelP
            if (EMASTER(theElement) ||
                EMASTER(vecElement) )
            {
                                                #endif
            for (i=0; i<SIDES_OF_ELEM(theElement); i++)
              if (NBELEM(theElement,i) == vecElement)
              {
                /* they are neighbors -> ok */
                error = 0;
                break;
              }
                                                #ifdef ModelP
          }
                                                #endif
            if (error)
            {
              UserWriteF("vector=" VINDEX_FMTX " has type %s, but points "
                         "to wrong vecobj=" EID_FMTX " NO NB of obj=" EID_FMTX "\n",
                         VINDEX_PRTX(theVector),ObjectString,
                         EID_PRTX(vecElement),EID_PRTX(theElement));
            }
          }
          else
          {
            errors++;
            UserWriteF("vector=" VINDEX_FMTX " has type %s, but points "
                       "to wrong obj=%d type OBJT=%d\n",
                       VINDEX_PRTX(theVector),ObjectString,ID(VecObject),
                       OBJT(VecObject));
          }
        }
        else
        {
                                        #ifdef UG_DIM_3
          if (VectorObjType == SIDEVEC)
          {
            /* TODO: check side vector */
          }
          else
                                        #endif
          {
            errors++;
            UserWriteF("%s vector=" VINDEX_FMTX " is referenced by "
                       "obj0=%x, but points to wrong obj1=%x\n",
                       ObjectString, VINDEX_PRTX(theVector),
                       theObject, VecObject);
                                                #ifdef ModelP
            if (strcmp(ObjectString,"EDGE")==0)
              UserWriteF("obj0: n0=%d n1=%d  obj1: "
                         "n0=%d n1=%d\n",
                         GID(NBNODE(LINK0(&(theObject->ed)))),
                         GID(NBNODE(LINK1(&(theObject->ed)))),
                         GID(NBNODE(LINK0(&(VecObject->ed)))),
                         GID(NBNODE(LINK1(&(VecObject->ed)))) );
                                                #endif
          }
        }
      }
    }
  }

  return(errors);
}

/****************************************************************************/
/** \brief Check the algebraic part of the data structure

 * @param theGrid -  grid level to check

   This function checks the consistency of the algebraic data structures
   including the interconnections between the geometric part.
   This function assumes a correct geometric data structure.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CheckAlgebra (GRID *theGrid)
{
  ELEMENT *theElement;
  VECTOR *theVector;
  INT errors;
  INT *s2p;

  errors = 0;

  if ((GLEVEL(theGrid)==0) && !MG_COARSE_FIXED(MYMG(theGrid)))
  {
    if (NVEC(theGrid)>0)
    {
      errors++;
      UserWriteF("coarse grid not fixed but vectors allocated\n");
    }
    return(errors);
  }

  s2p = BVPD_S2P_PTR(MG_BVPD(MYMG(theGrid)));

  /* reset USED flag */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    SETVCUSED(theVector,0);
  }

  /* check pointers to element, side, edge vector */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
                #ifdef UG_DIM_3
    /* check side vectors */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
    {
      for (INT i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        theVector = SVECTOR(theElement,i);
        errors += CheckVector(s2p,(GEOM_OBJECT *) theElement, "ELEMSIDE",
                              theVector, SIDEVEC,i);
      }
    }
                #endif
  }

  /* check USED flag */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    if (VCUSED(theVector) != 1)
    {
      errors++;
      UserWriteF("vector" VINDEX_FMTX " NOT referenced by an geom_object: "
                 "vtype=%d, objptr=%x",
                 VINDEX_PRTX(theVector), VTYPE(theVector), VOBJECT(theVector));
      if (VOBJECT(theVector) != NULL)
        UserWriteF(" objtype=%d\n",OBJT(VOBJECT(theVector)));
      else
        UserWrite("\n");
    }
    else
      SETVCUSED(theVector,0);
  }

  return(errors);
}

/****************************************************************************/
/** \brief Decide whether a vector corresponds to an element or not

 * @param theElement - pointer to element
 * @param theVector - pointer to a vector

   This function decides whether a given vector belongs to the given element, or
   one of its sides, edges or nodes.

 * @return <ul>
 *   <li>   0 if does not correspond </li>
 *   <li>   1 if does correspond.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX VectorInElement (ELEMENT *theElement, VECTOR *theVector)
{
  VECTOR *vList[20];

        #ifdef UG_DIM_3
  if (VOTYPE(theVector) == SIDEVEC)
  {
    INT cnt;
    GetVectorsOfSides(theElement,&cnt,vList);
    for (INT i=0; i<cnt; i++)
      if (vList[i]==theVector) RETURN(1);
  }
    #endif

  return (0);
}

/****************************************************************************/
/** \brief Calc coordinate position of vector

 * @param theVector - a given vector
 * @param position - array to be filled

   This function calcs physical position of vector. For edge vectors the
   midpoint is returned, and for sides and elements the center of mass.

 * @return <ul>
 *   <li>    0 if ok                     </li>
 *   <li>    1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX VectorPosition (const VECTOR *theVector, DOUBLE *position)
{
        #ifdef UG_DIM_3
  ELEMENT *theElement;
  INT theSide,j;
        #endif

  ASSERT(theVector != NULL);

        #if defined __OVERLAP2__
  if( VOBJECT(theVector) == NULL )
  {
    for (i=0; i<DIM; i++)
      position[i] = -MAX_D;
    return (0);
  }
        #endif

  switch (VOTYPE(theVector))
  {
                #ifdef UG_DIM_3
  case SIDEVEC :
    theElement = (ELEMENT *)VOBJECT(theVector);
    theSide = VECTORSIDE(theVector);
    for (INT i=0; i<DIM; i++)
    {
      position[i] = 0.0;
      for(j=0; j<CORNERS_OF_SIDE(theElement,theSide); j++)
        position[i] += CVECT(MYVERTEX(CORNER(theElement,CORNER_OF_SIDE(theElement,theSide,j))))[i];
      position[i] /= CORNERS_OF_SIDE(theElement,theSide);
    }
    return (0);
                #endif

  default : PrintErrorMessage('E',"VectorPosition","unrecognized object type for vector");
    assert(0);
  }

  RETURN (GM_ERROR);
}


/****************************************************************************/
/** \brief Initialize vector classes

 * @param theGrid - given grid
 * @param theElement - given element

   Initialize vector class in all vectors associated with given element with 3.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SeedVectorClasses (GRID *theGrid, ELEMENT *theElement)
{
        #ifdef UG_DIM_3
  INT cnt;

  VECTOR *vList[20];
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (INT i=0; i<cnt; i++)
      SETVCLASS(vList[i],3);
  }
    #endif
  return (0);
}

/****************************************************************************/
/** \brief Reset vector classes

 * @param theGrid - pointer to grid

   Reset all vector classes in all vectors of given grid to 0.

 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX ClearVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    SETVCLASS(theVector,0);

  return(0);
}


#ifdef ModelP
static int Gather_VectorVClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  PRINTDEBUG(gm,1,("Gather_VectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   VINDEX_PRTX(theVector),VCLASS(theVector)))

    ((INT *)data)[0] = VCLASS(theVector);

  return(0);
}

static int Scatter_VectorVClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVCLASS(theVector,MAX(VCLASS(theVector),((INT *)data)[0]));

  PRINTDEBUG(gm,2,("Scatter_VectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   VINDEX_PRTX(theVector),VCLASS(theVector)))

  return(0);
}

static int Scatter_GhostVectorVClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVCLASS(theVector,((INT *)data)[0]);

  PRINTDEBUG(gm,1,("Scatter_GhostVectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   VINDEX_PRTX(theVector),VCLASS(theVector)))

  return(0);
}
#endif


/****************************************************************************/
/** \brief Compute vector classes after initialization

 * @param theGrid - pointer to grid

   After vector classes have been reset and initialized, this function
   now computes the class 2 and class 1 vectors.

 * @return <ul>
 *   <li>     0 if ok </li>
 *   <li>     1 if error occured	 </li>
   </ul>
 */
/****************************************************************************/
INT NS_DIM_PREFIX PropagateVectorClasses (GRID *theGrid)
{
    #ifdef ModelP
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  PRINTDEBUG(gm,1,("\nPropagateVectorClasses():"
                   " 1. communication on level %d\n",GLEVEL(theGrid)))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\nPropagateVectorClasses(): 2. communication\n"))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\nPropagateVectorClasses(): 3. communication\n"))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

        #ifdef ModelP
  /* send VCLASS to ghosts */
  DDD_IFAOneway(context,
                dddctrl.VectorIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_VectorVClass, Scatter_GhostVectorVClass);
    #endif

  return(0);
}

/****************************************************************************/
/** \brief Reset class of the vectors on the next level

 * @param theGrid - pointer to grid

   This function clears VNCLASS flag in all vectors. This is the first step to
   compute the class of the dofs on the *NEXT* level, which
   is also the basis for determining copies.

 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured.			 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX ClearNextVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    SETVNCLASS(theVector,0);

  /* now the refinement algorithm will initialize the class 3 vectors */
  /* on the *NEXT* level.                                                                                       */
  return(0);
}

/****************************************************************************/
/** \brief Set VNCLASS in all vectors associated with element

 * @param theGrid - given grid
 * @param theElement - pointer to element

   Set VNCLASS in all vectors associated with the element to 3.

 * @return <ul>
 *   <li>    0 if ok  </li>
 *   <li>    1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SeedNextVectorClasses (GRID *theGrid, ELEMENT *theElement)
{

        #ifdef UG_DIM_3
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    VECTOR *vList[20];
    INT cnt;
    GetVectorsOfSides(theElement,&cnt,vList);
    for (INT i=0; i<cnt; i++)
      SETVNCLASS(vList[i],3);
  }
        #endif
  return (0);
}




#ifdef ModelP
static int Gather_VectorVNClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  PRINTDEBUG(gm,2,("Gather_VectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   VINDEX_PRTX(theVector),VNCLASS(theVector)))

    ((INT *)data)[0] = VNCLASS(theVector);

  return(GM_OK);
}

static int Scatter_VectorVNClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNCLASS(theVector,MAX(VNCLASS(theVector),((INT *)data)[0]));

  PRINTDEBUG(gm,2,("Scatter_VectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   VINDEX_PRTX(theVector),VNCLASS(theVector)))

  return(GM_OK);
}

static int Scatter_GhostVectorVNClass (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNCLASS(theVector,((INT *)data)[0]);

  PRINTDEBUG(gm,2,("Scatter_GhostVectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   VINDEX_PRTX(theVector),VNCLASS(theVector)))

  return(GM_OK);
}
#endif

/****************************************************************************/
/** \brief Compute VNCLASS in all vectors of a grid level

 * @param theGrid - pointer to grid

   Computes values of VNCLASS field in all vectors after seed.

 * @return <ul>
 *   <li>   0 if ok              </li>
 *   <li>   1 if error occured </li>
   </ul>
 */
/****************************************************************************/
INT NS_DIM_PREFIX PropagateNextVectorClasses (GRID *theGrid)
{
    #ifdef ModelP
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  PRINTDEBUG(gm,1,("\nPropagateNextVectorClasses(): 1. communication\n"))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
    #endif

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\nPropagateNextVectorClasses(): 2. communication\n"))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
    #endif

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\nPropagateNextVectorClasses(): 3. communication\n"))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(context,
                  dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);

  /* send VCLASS to ghosts */
  DDD_IFAOneway(context,
                dddctrl.VectorIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_VectorVNClass, Scatter_GhostVectorVNClass);
    #endif

  return(0);
}

/*@}*/
