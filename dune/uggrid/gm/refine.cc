// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************/
/*                                                                          */
/* File:      refine.c                                                      */
/*                                                                          */
/* Purpose:   unstructured grid adaption using a general element concept    */
/*            (dimension independent for 2/3D)                              */
/*                                                                          */
/* Author:    Stefan Lang                                                   */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/* email:     ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   08.08.95 begin serial algorithm, ug version 3.0               */
/*                                                                          */
/* Remarks:   - level 0 grid consists of red elements only                  */
/*            - the only restriction in the element hierarchy is that       */
/*              green or yellow elements might not have sons of class       */
/*              green or red                                                */
/*            - the rule set for refinement consists of regular (red)       */
/*              and irregular rules; regular rules create red elements      */
/*              while irregular rules result in green elements              */
/*              (green elements are needed for the closure of the grid,     */
/*              yellow elements, which are from copy rules, save            */
/*              the numerical properties of the solver and are handsome for */
/*              the discretisation                                          */
/*            - if the rule set for the red rules is not complete for build-*/
/*              up a consistent red refined region the FIFO might be used   */
/*              for some (hopefully not too much) iterations to find a      */
/*              consistent one                                              */
/*            - in 2D: exists a complete rule set for grids of triangles    */
/*              and quadrilaterals exclusively                              */
/*            - in 3D: exists a complete rule set for tetrahedrons          */
/*              and we assume after some analysation a complete set         */
/*              of rules described by an algorithm for hexahedrons          */
/*            - for mixed element types in arbitrary dimension no           */
/*              rule set for the closure exists                             */
/*            - BEFORE refinement we assume a situation where the error     */
/*              estimator has detected and marked the leaf elements for     */
/*              further refinement                                          */
/*            - AFTER refinement all elements are refined by a rule in way  */
/*              that no hanging nodes remain (this is the default mode)     */
/*              or with hanging nodes (in the hanging node mode)            */
/*              if you use inconsistent red refinement, you need to tell    */
/*              the algorithm explicitly to use the FIFO                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>

/* low module */
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/ugtypes.h>

/* dev module */
#include <dune/uggrid/ugdevices.h>

/* gm module */
#include "algebra.h"
#include "evm.h"
#include "gm.h"
#include "cw.h"
#include "refine.h"
#include "elements.h"
#include "rm.h"
#include "ugm.h"

/* parallel modules */
#ifdef ModelP
#include <dune/uggrid/parallel/ppif/ppif.h>
#include <dune/uggrid/parallel/ddd/include/ddd.h>
#include <dune/uggrid/parallel/dddif/debugger.h>
#include <dune/uggrid/parallel/dddif/identify.h>
#include <dune/uggrid/parallel/dddif/parallel.h>
#include "pargm.h"
#endif

#ifdef Debug
#include <dune/uggrid/low/architecture.h>
#endif

#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

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

/* undefine if overlap should be only updated where needed */
/* This does not work since the connection of the overlap needs the
   fatherelements on both sides (ghost and master sons) and this is
   not ensured.
   #define UPDATE_FULLOVERLAP
 */

/* define for use of DDD_ConsCheck() */
/*
   #define DDD_CONSCHECK(context)           if (DDD_ConsCheck(context)) assert(0);
 */
#define DDD_CONSCHECK(context)

#define MINVNCLASS      2           /* determines copies, dep. on discr. !  */

/* defines for side matching of elements 8 bits:                            */
/* _ _ _ _ (4 bits for corner of one element) _ _ _ _ (4 bits for the other)*/

#define LINEPOINTS              51                      /* 0011 0011 */
#define TRIPOINTS               119                     /* 0111 0111 */
#define QUADPOINTS              255                     /* 1111 1111 */

#define MAX_GREEN_SONS  32                      /* max num of sons for green refinement */

/* define to control element which is responsible */
/* equal side refinement of neighboring elements  */
#ifdef ModelP
#define _EID_   EGID
#define _ID_    GID
#else
#define _EID_   ID
#define _ID_    ID
#endif

#define EDGE_IN_PATTERN(p,i)            (((p)[(i)]) & 0x1)
#define SIDE_IN_PATTERN(e,p,i)          (((p)[EDGES_OF_ELEM(e)+(i)]) & 0x1)
#define EDGE_IN_PAT(p,i)                        (((p)>>(i)) & 0x1)
#define SIDE_IN_PAT(p,i)                        (((p)>>(i)) & 0x1)

#define MARK_BISECT_EDGE(r,i)           ((r)->pattern[(i)]==1)

#define REF_TYPE_CHANGES(e)                     ((REFINE(e)!=MARK(e)) || \
                                                 (REFINECLASS(e)!=MARKCLASS(e)))
#define MARKED(e)                                       (MARK(e)!=NO_REFINEMENT)

/* green marked elements were NEWGREEN is true are refined without rule */
#ifdef DUNE_UGGRID_TET_RULESET
#define NEWGREEN(e)                                     (TAG(e)==HEXAHEDRON || TAG(e)== PRISM || \
                                                         TAG(e)==PYRAMID)
#else
#define NEWGREEN(e)                                     (TAG(e)==HEXAHEDRON || TAG(e)== PRISM || \
                                                         TAG(e)==PYRAMID || TAG(e)== TETRAHEDRON)
#endif

/* marked elem with new green refinement (without rule, only 3D) */
#ifdef __ANISOTROPIC__
#define MARKED_NEW_GREEN(elem)                                               \
  (DIM==3 && ((NEWGREEN(elem) && MARKCLASS(elem)==GREEN_CLASS) ||  \
              ((TAG(elem)==PRISM && MARKCLASS(elem)==RED_CLASS && USED(elem)==1))))
#else
#define MARKED_NEW_GREEN(elem)                                               \
  (DIM==3 && NEWGREEN(elem) && MARKCLASS(elem)==GREEN_CLASS)
#endif

/** \brief Refined elem with new green refinement (without rule, only 3D) */
#define REFINED_NEW_GREEN(elem)                                              \
  (DIM==3 && NEWGREEN(elem) && REFINECLASS(elem)==GREEN_CLASS)

/** \brief Macro to test whether element changes refinement */
#define REFINEMENT_CHANGES(elem)                                             \
  (REF_TYPE_CHANGES(elem) ||                                           \
   (MARKED_NEW_GREEN(elem) &&                                           \
                  (REFINECLASS(elem)!=GREEN_CLASS || (REFINECLASS(elem)==GREEN_CLASS   \
                                                      && USED(elem)==1))))

/** @name Macros for storing sparse data needed in ExchangeClosureInfo() */
/*@{*/
#define MARKCLASSDATA_SHIFT     20
#define GETMARKCLASSDATA(elem,dataadr)                                       \
  (*(dataadr)) = (*(dataadr)) | ((MARKCLASS(elem))<<MARKCLASSDATA_SHIFT)
#define SETMARKCLASSDATA(elem,data)                                          \
  SETMARKCLASS((elem),((data)>>MARKCLASSDATA_SHIFT)&((1<<MARKCLASS_LEN)-1))
/*@}*/

/** @name Macros for storing sparse data needed in ExchangeClosureInfo() */
/*@{*/
#define MARKDATA_SHIFT  22
#define GETMARKDATA(elem,dataadr)                                            \
  (*(dataadr)) = (*(dataadr)) | ((MARK(elem))<<MARKDATA_SHIFT)
#define SETMARKDATA(elem,data)                                               \
  SETMARK((elem),MARK(elem)|(((data)>>MARKDATA_SHIFT)&((1<<MARK_LEN)-1)))
/*@}*/

/** @name Macros for storing sparse data needed in ExchangeClosureInfo() */
/*@{*/
#define COARSENDATA_SHIFT       19
#define GETCOARSENDATA(elem,dataadr)                                         \
  (*(dataadr)) = (*(dataadr)) | ((COARSEN(elem))<<COARSENDATA_SHIFT)
#define SETCOARSENDATA(elem,data)                                            \
  SETCOARSEN((elem),((data)>>COARSENDATA_SHIFT)&((1<<COARSEN_LEN)-1))
/*@}*/

/** @name Macros to get and set the edgepattern on an element */
/*@{*/
#define GetEdgeInfo(elem,patadr,macro)                                       \
  {                                                                    \
    INT i,pattern;                                                   \
    EDGE *theEdge;                                                   \
                                                                             \
    pattern = 0;                                                     \
    for (i=EDGES_OF_ELEM(elem)-1; i>=0; i--)                         \
    {                                                                \
      theEdge=GetEdge(CORNER_OF_EDGE_PTR((elem),i,0),              \
                      CORNER_OF_EDGE_PTR((elem),i,1));             \
      ASSERT(theEdge!=NULL);                                       \
                                                                             \
      pattern = (pattern<<1) | macro(theEdge);                     \
    }                                                                \
                                                                             \
    (*(patadr)) |= pattern;                                          \
  }

#define SetEdgeInfo(elem,pat,macro,OP)                                       \
  {                                                                    \
    INT i,pattern;                                                   \
    EDGE *theEdge;                                                   \
                                                                             \
    pattern = (pat);                                                 \
    for (i=0; i<EDGES_OF_ELEM(elem); i++)                            \
    {                                                                \
      theEdge = GetEdge(CORNER_OF_EDGE_PTR((elem),i,0),            \
                        CORNER_OF_EDGE_PTR((elem),i,1));           \
      ASSERT(theEdge!=NULL);                                       \
                                                                             \
      SET ## macro(theEdge, macro(theEdge) OP (pattern&0x1));      \
      pattern >>= 1;                                               \
    }                                                                \
  }
/*@}*/

#define REFINE_CONTEXT_LIST(d,context)                                       \
  IFDEBUG(gm,2)                                                            \
  {                                                                        \
    INT i;                                                               \
                                                                                                                                                         \
    UserWrite("  UpdateContext is :\n");                                 \
    for(i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)             \
      UserWriteF(" %3d",i);                                            \
    UserWrite("\n");                                                     \
    for(i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)             \
      if ((context)[i] != NULL)                                        \
        UserWriteF(" %3d",ID((context)[i]));                         \
      else                                                             \
        UserWriteF("    ");                                          \
    UserWrite("\n");                                                     \
  }                                                                        \
  ENDDEBUG

#ifndef STAT_OUT
#undef NEW_TIMER
#undef DEL_TIMER
#undef START_TIMER
#undef STOP_TIMER
#undef DIFF_TIMER
#undef SUM_TIMER
#undef EVAL_TIMER

#define NEW_TIMER(n)
#define DEL_TIMER(n)
#define START_TIMER(n)
#define STOP_TIMER(n)
#define DIFF_TIMER(n)
#define SUM_TIMER(n)
#define EVAL_TIMER(n)
#endif

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

typedef NODE *ELEMENTCONTEXT[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];


/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/* information used by the estimator and refine*/
REFINEINFO NS_DIM_PREFIX refine_info;

#ifdef ModelP
/* control words for identiftication of new nodes and edges */
INT NS_DIM_PREFIX ce_NEW_NIDENT;
INT NS_DIM_PREFIX ce_NEW_EDIDENT;
#endif

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/** \brief type of refinement                   */
static int rFlag=GM_REFINE_TRULY_LOCAL;

/** \brief refine with hanging nodes?		*/
static int hFlag=0;

/** \brief use fifo? 0=no 1=yes				*/
static int fifoFlag=0;

/** \brief fifo loop counter                            */
static int first;

/** \brief first element in fifo work list	*/
static ELEMENT *fifo_first=NULL;

/** \brief last element in fifo	work list	*/
static ELEMENT *fifo_last=NULL;

/** \brief first element in fifo insertlist */
static ELEMENT *fifo_insertfirst=NULL;

/** \brief last element in fifo insertlist	*/
static ELEMENT *fifo_insertlast=NULL;

/** \brief first element to consider for next loop   */
static ELEMENT *firstElement=NULL;

/** \brief Counter for green refinements doesn't need to be updated */
static INT No_Green_Update;

/** \brief green refined element counter	*/
static INT Green_Marks;

/** \brief 0/1: do/do not parallel part		*/
static INT refine_seq = 0;

/** \brief counter for FIFO loops			*/
static INT fifoloop = 0;

/** \brief count of adapted elements        */
static INT total_adapted = 0;

#ifdef STAT_OUT
/* timing variables */
static int adapt_timer,closure_timer,gridadapt_timer,gridadapti_timer;
static int gridadaptl_timer,ident_timer,overlap_timer,gridcons_timer;
static int algebra_timer;
#endif

#ifdef DUNE_UGGRID_TET_RULESET
/* determine number of edge from reduced (i.e. restricted to one side) edgepattern */
/* if there are two edges marked for bisection, if not deliver -1. If the edge-    */
/* is not reduced (i.e. marked edges lying on more than one side) deliver -2       */
static INT TriSectionEdge[64][2] =
{       {-1,-1},{-1,-1},{-1,-1},{ 1, 0},{-1,-1},{ 0, 2},{ 2, 1},{-1,-1},
        {-1,-1},{ 3, 0},{-2,-2},{-2,-2},{ 2, 3},{-2,-2},{-2,-2},{-2,-2},
        {-1,-1},{ 0, 4},{ 4, 1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
        { 4, 3},{-1,-1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
        {-1,-1},{-2,-2},{ 1, 5},{-2,-2},{ 5, 2},{-2,-2},{-2,-2},{-2,-2},
        { 3, 5},{-2,-2},{-2,-2},{-2,-2},{-1,-1},{-2,-2},{-2,-2},{-2,-2},
        { 5, 4},{-2,-2},{-1,-1},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},
        {-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2},{-2,-2}};

/* the indices of the edges of each side */
static INT CondensedEdgeOfSide[4] = {0x07,0x32,0x2C,0x19};
#endif

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

#ifdef ModelP
static void CheckConsistency (MULTIGRID *theMG, INT level ,INT debugstart, INT gmlevel, int *check);
#endif

/****************************************************************************/
/** \brief Fill refineinfo structure

   This function fills the refineinfo structure

   \return <ul>
   <li> GM_OK if ok </li>
   <li> GM_ERROR if an error occurs </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SetRefineInfo (MULTIGRID *theMG)
{
  if (MultiGridStatus(theMG,1,0,0,0) != GM_OK) return(GM_ERROR);

  return(GM_OK);
}


/****************************************************************************/
/** \brief Drop marks from leafelements to first regular

   This function drops marks from leafelements to first regular, and resets
   marks on all elements above (important for restrict marks)

   \return <ul>
   .n   GM_OK if ok
   .n   GM_ERROR if an error occurs
 */
/****************************************************************************/

static INT DropMarks (MULTIGRID *theMG)
{
  return(GM_OK);

  for (INT k = TOPLEVEL(theMG); k > 0; k--)
  {
    const GRID *theGrid = GRID_ON_LEVEL(theMG,k);
    for (ELEMENT *theElement = FIRSTELEMENT(theGrid);
         theElement != NULL;
         theElement=SUCCE(theElement))
      if ((MARKCLASS(theElement) == RED_CLASS) &&
          (ECLASS(theElement) != RED_CLASS))
      {
        INT Mark = MARK(theElement);
        /** \todo marks must be changed if element type changes */
        if (TAG(theElement)!=HEXAHEDRON &&
            TAG(EFATHER(theElement))==HEXAHEDRON) Mark = HEXA_RED;
        if (TAG(theElement)!=PYRAMID &&
            TAG(EFATHER(theElement))==PYRAMID) Mark = PYR_RED;
        ELEMENT *FatherElement = theElement;

        SETMARK(FatherElement,NO_REFINEMENT);
        SETMARKCLASS(FatherElement,NO_CLASS);
        FatherElement = EFATHER(FatherElement);

        SETMARK(FatherElement,Mark);
        SETMARKCLASS(FatherElement,RED_CLASS);
      }
  }
  return(GM_OK);
}

/* Functions for realizing the (parallel) closure FIFO */


/****************************************************************************/
/** \brief Function for realizing the (parallel) closure FIFO
 */
/****************************************************************************/

static INT InitClosureFIFO (void)
{
  fifo_first=fifo_last=fifo_insertfirst=fifo_insertlast=NULL;
  first = 1;
  fifoloop = 0;
  if (0) UserWriteF("Using FIFO: loop %d\n",fifoloop);

  return (GM_OK);
}


/****************************************************************************/
/** \brief Function for realizing the (parallel) closure FIFO

   \param  theGrid - pointer to grid structure
   .  theElement
   .  thePattern
   .  NewPattern

   DESCRIPTION:

 */
/****************************************************************************/

static INT UpdateFIFOLists (GRID *theGrid, ELEMENT *theElement, INT thePattern, INT NewPattern)
{

  if (MARKCLASS(theElement)==RED_CLASS && thePattern!=NewPattern)
  {
                #ifdef UG_DIM_2
    for (INT j = 0; j < EDGES_OF_ELEM(theElement); j++)
    {
      if (EDGE_IN_PAT(thePattern,j)==0 &&
          EDGE_IN_PAT(NewPattern,j))
      {

        EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement, j, 0),
                                CORNER_OF_EDGE_PTR(theElement, j, 1));
        ASSERT(theEdge != NULL);

        SETPATTERN(theEdge,1);

        /* boundary case */
        if (SIDE_ON_BND(theElement,j)) continue;

        /* add the element sharing this edge to fifo_queue */
        ELEMENT *NbElement = NBELEM(theElement, j);

        if (NbElement==NULL) continue;

        PRINTDEBUG(gm,1,("   ADDING to FIFO: NBID=%d\n",
                         ID(NbElement)))

        /* unlink element from element list */
        if (PREDE(NbElement) != NULL)
          SUCCE(PREDE(NbElement)) = SUCCE(NbElement);
        if (SUCCE(NbElement) != NULL)
          PREDE(SUCCE(NbElement)) = PREDE(NbElement);
        if (FIRSTELEMENT(theGrid) == NbElement)
          FIRSTELEMENT(theGrid) = SUCCE(NbElement);

        SUCCE(NbElement) = PREDE(NbElement) = NULL;
        /* insert into fifo */
        if (fifo_insertfirst == NULL)
        {
          fifo_insertfirst = fifo_insertlast = NbElement;
        }
        else
        {
          SUCCE(fifo_insertlast) = NbElement;
          PREDE(NbElement) = fifo_insertlast;
          fifo_insertlast = NbElement;
        }
      }

      if (EDGE_IN_PAT(thePattern,j) &&
          EDGE_IN_PAT(NewPattern,j)==0)
      {

        UserWriteF("UpdateFIFOLists(): ERROR EID=%d in fifo "
                   "thePattern=%d has edge=%d refined but "
                   "NewPattern=%d NOT!\n",
                   ID(theElement),thePattern,j,NewPattern);
        RETURN(-1);
      }
    }
                #endif
                #ifdef UG_DIM_3
    UserWriteF("UpdateFIFOLists(): ERROR fifo for 3D NOT implemented!\n");
    ASSERT(0);
                #endif
  }

  return(GM_OK);
}


/****************************************************************************/
/** \brief Function for realizing the (parallel) closure FIFO

   \param theGrid - pointer to grid structure

 */
/****************************************************************************/

static INT UpdateClosureFIFO (GRID *theGrid)
{
  /* insert fifo work list into elementlist */
  for (ELEMENT *theElement = fifo_last; theElement != nullptr;
       theElement=PREDE(theElement))
  {
    SUCCE(theElement) = FIRSTELEMENT(theGrid);
    PREDE(FIRSTELEMENT(theGrid)) = theElement;
    FIRSTELEMENT(theGrid) = theElement;
  }

  PREDE(FIRSTELEMENT(theGrid)) = NULL;

  if (fifo_insertfirst != NULL)
  {
    /* append fifo insert list to fifo work list */
    firstElement = fifo_first = fifo_insertfirst;
    fifo_last = fifo_insertlast;

    IFDEBUG(gm,2)
    UserWriteF(" FIFO Queue:");
    for (ELEMENT *theElement = fifo_first; theElement != nullptr;
         theElement=SUCCE(theElement))
      UserWriteF(" %d\n", ID(theElement));
    ENDDEBUG

      fifo_insertfirst = fifo_insertlast = NULL;
    first = 0;
    fifoloop++;
    UserWriteF(" loop %d",fifoloop);
    return(1);
  }

  return(0);
}


/****************************************************************************/
/*
   ManageParallelFIFO - function for realizing the (parallel) closure FIFO

   SYNOPSIS:
   static INT ManageParallelFIFO (ELEMENT *firstElement);

   PARAMETERS:
   \param firstElement

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT ManageParallelFIFO (const PPIF::PPIFContext& context, const ELEMENT *firstElement)
{
#if defined(FIFO) && defined(ModelP)
  if (context.procs() == 1) return(0);

  do
  {
    /* exchange FIFO flag and PATTERN from slaves to master */
    IF_FIF0AndPat_S2M(GLEVEL(grid));

    /* add all master elements of horizontal interface to FIFO */
    for (ELEMENT *theElement = firstElement; theElement != nullptr;
         theElement=SUCCE(theElement))
    {
      if (IS_HOR_MASTER(theElement) && FIFO(theElement))
      {
        AddToFIFIO(theElement);
        SETFIFO(theElement,0);
      }
    }

    /* check condition for termination of pattern adaptation */
    AllFIFOsEmpty = CheckGlobalFIFOStatus(fifo);
  }
  while (fifo==NULL && AllFIFOsEmpty==1);
#else
  return (0);
#endif
}

/****************************************************************************/
/** \brief Changes refinement of element

   \param theElement - pointer to element

   This function returns a boolean value to indicate, whether an element
   changes its refinement (1) or not (0).

   \return <ul>
   .n   0 if element will not change refinement
   .n   1 if it will
 */
/****************************************************************************/

INT NS_DIM_PREFIX Refinement_Changes (ELEMENT *theElement)
{
  return(REFINEMENT_CHANGES(theElement));
}

/****************************************************************************/
/*
   GridClosure - compute closure for next level

   SYNOPSIS:
   static INT PrepareGridClosure (GRID *theGrid);

   PARAMETERS:
   \param theGrid - pointer to grid structure

   DESCRIPTION:
   This function computes the closure for next level. A closure can only be
   determined if the rule set for the used elements is complete. This means
   that for all side and edge patterns possible for an element type exists a
   rule which closes the element. In this case a FIFO for computing the closure
   is not needed any more and the closure can be computed in one step.

   \return <ul>
   INT
   .n   >0 if elements will be refined
   .n   0 if no elements will be refined
   .n   -1 if an error occurred
 */
/****************************************************************************/

static INT PrepareGridClosure (const GRID *theGrid)
{
  /* reset USED flag of elements and PATTERN and */
  /* ADDPATTERN flag on the edges                */
  for (ELEMENT *theElement = PFIRSTELEMENT(theGrid); theElement != nullptr;
       theElement=SUCCE(theElement))
  {
    SETUSED(theElement,0);
    if (EGHOST(theElement))
    {
      SETCOARSEN(theElement,0);
      SETMARK(theElement,NO_REFINEMENT);
      SETMARKCLASS(theElement,0);
    }

    for (INT j = 0; j < EDGES_OF_ELEM(theElement); j++)
    {
      EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement, j, 0),
                              CORNER_OF_EDGE_PTR(theElement, j, 1));
      ASSERT(theEdge != NULL);

      SETPATTERN(theEdge,0);
      SETADDPATTERN(theEdge,1);                   /* needed in RestrictMarks() */
    }
  }

  return(GM_OK);
}

#ifdef ModelP


/****************************************************************************/
/*
   Gather_ElementClosureInfo -

   SYNOPSIS:
   static int Gather_ElementClosureInfo (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Gather_ElementClosureInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,1,("Gather_ElementClosureInfo(): e=" EID_FMTX "\n",
                   EID_PRTX(theElement)))

  INT refinedata = 0;

        #ifdef UG_DIM_2
  GetEdgeInfo(theElement,&refinedata,PATTERN);
        #endif

  /* mark and sidepattern have same control word positions */
  /* if this changes sidepattern must be sent separately   */
  GETMARKDATA(theElement,&refinedata);
  GETMARKCLASSDATA(theElement,&refinedata);
  GETCOARSENDATA(theElement,&refinedata);
  ((INT *)data)[0] = refinedata;

  PRINTDEBUG(gm,1,("Gather_ElementClosureInfo(): refinedata=%08x "
                   "sidepattern=%d markclass=%d mark=%d coarse=%d\n",refinedata,
                   SIDEPATTERN(theElement),MARKCLASS(theElement),MARK(theElement),COARSEN(theElement)))

  return(GM_OK);
}



/****************************************************************************/
/*
   Scatter_ElementClosureInfo -

   SYNOPSIS:
   static int Scatter_ElementClosureInfo (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Scatter_ElementClosureInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,1,("Scatter_ElementClosureInfo(): e=" EID_FMTX "\n",
                   EID_PRTX(theElement)))

  INT refinedata = ((INT *)data)[0];

        #ifdef UG_DIM_2
  SetEdgeInfo(theElement,refinedata,PATTERN,|);
        #endif

  /* mark and sidepattern have same control word positions */
  /* if this changes sidepattern must be sent separately   */
  SETMARKDATA(theElement,refinedata);

  if (EMASTER(theElement)) return(GM_OK);
  if (EGHOST(theElement) && EGHOSTPRIO(prio)) return(GM_OK);

  SETMARKCLASSDATA(theElement,refinedata);
  SETCOARSENDATA(theElement,refinedata);

  PRINTDEBUG(gm,1,("Scatter_ElementClosureInfo(): refinedata=%08x "
                   "sidepattern=%d markclass=%d mark=%d coarse=%d\n",refinedata,
                   SIDEPATTERN(theElement),MARKCLASS(theElement),MARK(theElement),COARSEN(theElement)))

  return(GM_OK);
}

static INT ExchangeElementClosureInfo (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  /* exchange information of elements to compute closure */
  DDD_IFAOnewayX(context,
                 dddctrl.ElementSymmVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                 Gather_ElementClosureInfo, Scatter_ElementClosureInfo);

  return(GM_OK);
}

static int Gather_ElementRefine (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,1,("Gather_ElementRefine(): e=" EID_FMTX "\n",
                   EID_PRTX(theElement)))

    ((INT *)data)[0] = MARKCLASS(theElement);
  ((INT *)data)[1] = MARK(theElement);

  return(GM_OK);
}

static int Scatter_ElementRefine (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,1,("Scatter_ElementClosureInfo(): e=" EID_FMTX "\n",
                   EID_PRTX(theElement)))

  if (EMASTER(theElement)) return(GM_OK);
  if (EGHOST(theElement) && EGHOSTPRIO(prio)) return(GM_OK);

  SETMARKCLASS(theElement,((INT *)data)[0]);
  SETMARK(theElement,((INT *)data)[1]);

  return(GM_OK);
}

static INT ExchangeElementRefine (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  /* exchange information of elements to compute closure */
  DDD_IFAOnewayX(context,
                 dddctrl.ElementSymmVHIF,GRID_ATTR(theGrid),IF_FORWARD,2*sizeof(INT),
                 Gather_ElementRefine, Scatter_ElementRefine);

  return(GM_OK);
}

#ifdef UG_DIM_3

/****************************************************************************/
/*
   Gather_EdgeClosureInfo -

   SYNOPSIS:
   static int Gather_EdgeClosureInfo (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Gather_EdgeClosureInfo (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  EDGE    *theEdge = (EDGE *)obj;

  PRINTDEBUG(gm,1,("Gather_EdgeClosureInfo(): e=" ID_FMTX "pattern=%d \n",
                   ID_PRTX(theEdge),PATTERN(theEdge)))

  INT pattern = PATTERN(theEdge);

  ((INT *)data)[0] = pattern;

  return(GM_OK);
}



/****************************************************************************/
/*
   Scatter_EdgeClosureInfo -

   SYNOPSIS:
   static int Scatter_EdgeClosureInfo (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Scatter_EdgeClosureInfo (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  EDGE    *theEdge = (EDGE *)obj;

  INT pattern = std::max((INT)PATTERN(theEdge),((INT *)data)[0]);

  PRINTDEBUG(gm,1,("Gather_EdgeClosureInfo(): e=" ID_FMTX "pattern=%d \n",
                   ID_PRTX(theEdge),pattern))

  SETPATTERN(theEdge,pattern);

  return(GM_OK);
}

INT     ExchangeEdgeClosureInfo (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  /* exchange information of edges to compute closure */
  DDD_IFAOneway(context,
                dddctrl.EdgeVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_EdgeClosureInfo, Scatter_EdgeClosureInfo);

  return(GM_OK);
}
#endif

/****************************************************************************/
/*
   ExchangeClosureInfo -

   SYNOPSIS:
   static INT ExchangeClosureInfo (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT ExchangeClosureInfo (GRID *theGrid)
{

  /* exchange information of elements to compute closure */
  if (ExchangeElementClosureInfo(theGrid) != GM_OK) RETURN(GM_ERROR);

        #ifdef UG_DIM_3
  /* exchange information of edges to compute closure */
  if (ExchangeEdgeClosureInfo(theGrid) != GM_OK) RETURN(GM_ERROR);
        #endif

  return(GM_OK);
}
#endif


/****************************************************************************/
/*
   ComputePatterns -

   SYNOPSIS:
   static INT ComputePatterns (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT ComputePatterns (const GRID *theGrid)
{
  /* ComputePatterns works only on master elements      */
  /* since ghost elements have no information from      */
  /* RestrictMarks() up to this time and this may       */
  /* leed to inconsistency while coarsening             */

  /* reset EDGE/SIDEPATTERN in elements                 */
  /* set SIDEPATTERN in elements                        */
  /* set PATTERN on the edges                           */
  for (ELEMENT *theElement = PFIRSTELEMENT(theGrid); theElement != nullptr;
       theElement=SUCCE(theElement))
  {
                #ifdef ModelP
    if (EGHOST(theElement))
    {
                        #ifdef UG_DIM_3
      SETSIDEPATTERN(theElement,0);
                        #endif
      continue;
    }
                #endif

    if (MARKCLASS(theElement)==RED_CLASS)
    {
      INT Mark = MARK(theElement);
      SHORT *thePattern = MARK2PATTERN(theElement,Mark);

      for (INT i = 0; i < EDGES_OF_ELEM(theElement); i++)
        if (EDGE_IN_PATTERN(thePattern,i))
        {
          EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement, i, 0),
                                  CORNER_OF_EDGE_PTR(theElement, i, 1));

          ASSERT(theEdge != NULL);

          SETPATTERN(theEdge,1);
        }

                        #ifdef UG_DIM_3
      /* SIDEPATTERN must be reset here for master elements, */
      /* because it overlaps with MARK (980217 s.l.)         */
      SETSIDEPATTERN(theElement,0);
      for (INT i = 0; i < SIDES_OF_ELEM(theElement); i++)
      {
#ifdef DUNE_UGGRID_TET_RULESET
        if (CORNERS_OF_SIDE(theElement,i)==4)
        {
#endif
        /* set SIDEPATTERN if side has node */
        if(SIDE_IN_PATTERN(theElement,thePattern,i))
          SETSIDEPATTERN(theElement,
                         SIDEPATTERN(theElement) | 1<<i);
#ifdef DUNE_UGGRID_TET_RULESET
      }
#endif
      }
                        #endif
    }
    else
    {
                        #ifdef UG_DIM_3
      /* SIDEPATTERN must be reset here for master elements, */
      /* because it overlaps with MARK (980217 s.l.)         */
      SETSIDEPATTERN(theElement,0);
                        #endif
      SETMARKCLASS(theElement,NO_CLASS);
    }
  }

  return(GM_OK);
}

#ifdef UG_DIM_3
#ifdef DUNE_UGGRID_TET_RULESET

/****************************************************************************/
/*
   CorrectTetrahedronSidePattern -

   SYNOPSIS:
   static INT CorrectTetrahedronSidePattern (ELEMENT *theElement, INT i, ELEMENT *theNeighbor, INT j);

   PARAMETERS:
   .  theElement
   .  i
   .  theNeighbor
   .  j

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT CorrectTetrahedronSidePattern (ELEMENT *theElement, INT i, ELEMENT *theNeighbor, INT j)
{
  INT theEdgeNum,theEdgePattern=0;
  INT NbEdgeNum,NbEdgePattern,NbSidePattern,NbSideMask;

  if (TAG(theElement)==PYRAMID || TAG(theElement)==PRISM)
    return(GM_OK);

  for (INT k = EDGES_OF_ELEM(theElement) - 1; k >= 0; k--)
  {
    EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement, k, 0),
                            CORNER_OF_EDGE_PTR(theElement, k, 1));
    ASSERT(theEdge!=NULL);

    theEdgePattern = (theEdgePattern<<1) | PATTERN(theEdge);
  }

  /* because SIDEPATTERN is set to zero, */
  /* I choose TriSectionEdge[0] */
  theEdgeNum = TriSectionEdge[theEdgePattern
                              &CondensedEdgeOfSide[i]][0];

  if (theEdgeNum == -2) RETURN(-1);

  if (theEdgeNum == -1) return(GM_OK);

  switch (TAG(theNeighbor))
  {

  case TETRAHEDRON :

    NbEdgePattern = 0;

    for (INT k = 0; k < EDGES_OF_ELEM(theNeighbor); k++)
    {
      EDGE *NbEdge = GetEdge(CORNER_OF_EDGE_PTR(theNeighbor, k, 0),
                             CORNER_OF_EDGE_PTR(theNeighbor, k, 1));
      ASSERT(NbEdge!=NULL);
      NbEdgePattern = NbEdgePattern | (PATTERN(NbEdge)<<k);
    }

    NbEdgeNum = TriSectionEdge[NbEdgePattern
                               &CondensedEdgeOfSide[j]][0];

    if (NbEdgeNum == -2 || NbEdgeNum == -1)
      RETURN(-1);

    if (!(CORNER_OF_EDGE_PTR(theElement,theEdgeNum,0) ==
          CORNER_OF_EDGE_PTR(theNeighbor,NbEdgeNum,0) &&
          CORNER_OF_EDGE_PTR(theElement,theEdgeNum,1) ==
          CORNER_OF_EDGE_PTR(theNeighbor,NbEdgeNum,1)   )
        &&
        !(CORNER_OF_EDGE_PTR(theElement,theEdgeNum,0) ==
          CORNER_OF_EDGE_PTR(theNeighbor,NbEdgeNum,1) &&
          CORNER_OF_EDGE_PTR(theElement,theEdgeNum,1) ==
          CORNER_OF_EDGE_PTR(theNeighbor,NbEdgeNum,0)   ) )
    {
      NbSidePattern = SIDEPATTERN(theNeighbor);
      NbSideMask = (1<<j);

      if ( NbSidePattern & NbSideMask )
      {
        NbSidePattern &= ~NbSideMask;
                                        #ifdef ModelP
        /* in this case ExchangeSidePatterns() fails */
        /* does it occur ? (980217 s.l.)             */
        assert(0);
                                        #endif
      }
      else
        NbSidePattern |= NbSideMask;

      PRINTDEBUG(gm,1,("CorrectTetrahedronSidePattern(): nb=" EID_FMTX
                       " new nbsidepattern=%d\n",EID_PRTX(theNeighbor),NbSidePattern));
      SETSIDEPATTERN(theNeighbor,NbSidePattern);
    }
    break;

  case PYRAMID :
  case PRISM :
  {
    INT trisectionedge=-1;

    for (INT k = 0; k < CORNERS_OF_SIDE(theNeighbor, j); k++)
    {
      INT edge = EDGE_OF_SIDE(theElement, j, k);

      EDGE *NbEdge = GetEdge(CORNER_OF_EDGE_PTR(theNeighbor, edge, 0),
                             CORNER_OF_EDGE_PTR(theNeighbor, edge, 1));
      ASSERT(NbEdge!=NULL);

      if (PATTERN(NbEdge) && (edge>trisectionedge))
        trisectionedge = edge;
    }
    assert(trisectionedge != -1);

    if (theEdgeNum != trisectionedge)
      SETSIDEPATTERN(theNeighbor,
                     SIDEPATTERN(theNeighbor)|(1<<j));

    break;
  }

  default :
    ASSERT(0);
  }

  return(GM_OK);
}
#endif


/****************************************************************************/
/*
   CorrectElementSidePattern -

   SYNOPSIS:
   static INT CorrectElementSidePattern (ELEMENT *theElement, ELEMENT *theNeighbor, INT i);

   PARAMETERS:
   .  theElement
   .  theNeighbor
   .  i

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT CorrectElementSidePattern (ELEMENT *theElement, ELEMENT *theNeighbor, INT i)
{
  INT j;

#ifdef ModelP
  /* this case should not occur */
  if (theNeighbor == NULL)
  {
    ASSERT(EGHOST(theElement));
    UserWriteF("CorrectElementSidePattern(): error elem=" EID_FMTX " nb[%d]="
               EID_FMTX " nb=" EID_FMTX
               "\n", EID_PRTX(theElement),i,EID_PRTX(NBELEM(theElement,i)),
               EID_PRTX(theNeighbor));
    return(GM_OK);
  }
#endif

  /* search neighbors side */
  for (j=0; j<SIDES_OF_ELEM(theNeighbor); j++)
    if (NBELEM(theNeighbor,j) == theElement)
      break;
        #ifdef ModelP
  if (j >= SIDES_OF_ELEM(theNeighbor))
  {
    if (!(EGHOST(theElement) && EGHOST(theNeighbor)))
    {
      UserWriteF("CorrectElementSidePattern(): ERROR nbelem not found elem=%08x/"
                 EID_FMTX " nb=%08x/" EID_FMTX "\n",
                 theElement,EID_PRTX(theElement),theNeighbor,EID_PRTX(theNeighbor));
    }
    ASSERT(EGHOST(theElement) && EGHOST(theNeighbor));
    return(GM_OK);
  }
        #else
  ASSERT(j<SIDES_OF_ELEM(theNeighbor));
        #endif

  /* side is triangle or quadrilateral */
  switch (CORNERS_OF_SIDE(theElement,i))
  {
  case 3 :
                        #ifdef DUNE_UGGRID_TET_RULESET
    /* handle case with 2 edges of the side refined */
    if (CorrectTetrahedronSidePattern(theElement,i,theNeighbor,j) != GM_OK)
      RETURN(GM_ERROR);
                        #endif
    break;

  case 4 :
    /* if side of one of the neighboring elements has a */
    /* sidenode, then both need a sidenode              */
    if (SIDE_IN_PAT(SIDEPATTERN(theElement),i))
    {
      SETSIDEPATTERN(theNeighbor,
                     SIDEPATTERN(theNeighbor) | (1<<j));
    }
    else if (SIDE_IN_PAT(SIDEPATTERN(theNeighbor),j))
    {
      SETSIDEPATTERN(theElement,
                     SIDEPATTERN(theElement) | (1<<i));
    }
    break;

  default :
    ASSERT(0);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   SetElementSidePatterns -

   SYNOPSIS:
   static INT SetElementSidePatterns (GRID *theGrid, ELEMENT *firstElement);

   PARAMETERS:
   .  theGrid - pointer to grid structure
   .  firstElement

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT SetElementSidePatterns (GRID *theGrid, ELEMENT *firstElement)
{
  INT i;
  ELEMENT *theElement,*theNeighbor;

  /* set pattern (edge and side) on the elements */
  for (theElement=firstElement; theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    /* make edgepattern consistent with pattern of edges */
    SETUSED(theElement,1);

                #ifndef __ANISOTROPIC__
    /** \todo change this for red refinement of pyramids */
    if (DIM==3 && TAG(theElement)==PYRAMID) continue;
                #endif

    /* make sidepattern consistent with neighbors	*/
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      theNeighbor = NBELEM(theElement,i);
      if (theNeighbor == NULL) continue;

      /* only one of the neighboring elements does corrections */
      /* determine element for side correction by (g)id        */
      if (_EID_(theElement) < _EID_(theNeighbor)) continue;

      /* determine element for side correction by used flag    */
      /** \todo delete this:
         if (USED(theNeighbor)) continue;
       */

      /* edgepatterns from theElement and theNeighbor are in final state */

      if (CorrectElementSidePattern(theElement,theNeighbor,i) != GM_OK) RETURN(GM_ERROR);
    }
  }

  return(GM_OK);
}
#endif



/****************************************************************************/
/*
   SetElementRules -

   SYNOPSIS:
   static INT SetElementRules (GRID *theGrid, ELEMENT *firstElement, INT *cnt);

   PARAMETERS:
   .  theGrid - pointer to grid structure
   .  firstElement
   .  cnt

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT SetElementRules (GRID *theGrid, ELEMENT *firstElement, INT *cnt)
{
  INT Mark,NewPattern;
  INT thePattern,theEdgePattern,theSidePattern=0;
  ELEMENT *theElement;

  [[maybe_unused]] const int me = theGrid->ppifContext().me();

  /* set refinement rules from edge- and sidepattern */
  (*cnt) = 0;
  for (theElement=firstElement; theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    theEdgePattern = 0;

    /* compute element pattern */
    GetEdgeInfo(theElement,&theEdgePattern,PATTERN);

                #ifdef UG_DIM_2
    thePattern = theEdgePattern;
    PRINTDEBUG(gm,2,(PFMT "SetElementRules(): e=" EID_FMTX " edgepattern=%d\n",
                     me,EID_PRTX(theElement),theEdgePattern));
                #endif
                #ifdef UG_DIM_3
    theSidePattern = SIDEPATTERN(theElement);
    thePattern = theSidePattern<<EDGES_OF_ELEM(theElement) | theEdgePattern;
    PRINTDEBUG(gm,2,(PFMT "SetElementRules(): e=" EID_FMTX
                     " edgepattern=%03x sidepattern=%02x\n",
                     me,EID_PRTX(theElement),theEdgePattern,theSidePattern));
                #endif

    /* get Mark from pattern */
    Mark = PATTERN2MARK(theElement,thePattern);

    /* treat Mark according to mode */
    if (fifoFlag)
    {
      /* directed refinement */
      if (Mark == -1 && MARKCLASS(theElement)==RED_CLASS)
      {
        /* there is no rule for this pattern, switch to red */
        Mark = RED;
      }
      else
        ASSERT(Mark != -1);
    }
    else if (hFlag==0 && MARKCLASS(theElement)!=RED_CLASS)
    {
      /* refinement with hanging nodes */
      Mark = NO_REFINEMENT;
    }
    else
    {
      /* refinement with closure (default) */
                        #ifdef __ANISOTROPIC__
      if (MARKCLASS(theElement)==RED_CLASS && TAG(theElement)==PRISM)
      {
        ASSERT(USED(theElement)==1);
        if (Mark==-1)
        {
          ASSERT(TAG(theElement)==PRISM);
          /* to implement the anisotropic case for other elements */
          /* and anisotropic refinements the initial anisotropic  */
          /* rule is needed here. (980316 s.l.)                   */
          Mark = PRI_QUADSECT;
        }
        else
          SETUSED(theElement,0);
      }
                        #endif
      ASSERT(Mark != -1);

      /* switch green class to red class? */
      if (MARKCLASS(theElement)!=RED_CLASS &&
          SWITCHCLASS(CLASS_OF_RULE(MARK2RULEADR(theElement,Mark))))
      {
        IFDEBUG(gm,1)
        UserWriteF("   Switching MARKCLASS=%d for MARK=%d of EID=%d "
                   "to RED_CLASS\n",
                   MARKCLASS(theElement),Mark,ID(theElement));
        ENDDEBUG
        SETMARKCLASS(theElement,RED_CLASS);
      }
    }

    REFINE_ELEMENT_LIST(1,theElement,"");

                #ifdef UG_DIM_3
    /* choose best tet_red rule according to (*theFullRefRule)() */
    if (TAG(theElement)==TETRAHEDRON && MARKCLASS(theElement)==RED_CLASS)
    {
#ifndef DUNE_UGGRID_TET_RULESET
      if ((Mark==TET_RED || Mark==TET_RED_0_5 ||
           Mark==TET_RED_1_3))
#endif
      {
        PRINTDEBUG(gm,5,("FullRefRule() call with mark=%d\n",Mark))

        Mark = (*theFullRefRule)(theElement);
        assert( Mark==FULL_REFRULE_0_5 ||
                Mark==FULL_REFRULE_1_3 ||
                Mark==FULL_REFRULE_2_4);
      }
    }
                #endif

    /* get new pattern from mark */
    NewPattern = MARK2PAT(theElement,Mark);
    IFDEBUG(gm,2)
    UserWriteF("   thePattern=%d EdgePattern=%d SidePattern=%d NewPattern=%d Mark=%d\n",
               thePattern,theEdgePattern,theSidePattern,NewPattern,Mark);
    ENDDEBUG


    if (fifoFlag)
    {
      if (UpdateFIFOLists(theGrid,theElement,thePattern,NewPattern) != GM_OK) return(GM_OK);
    }

    if (Mark) (*cnt)++;
    SETMARK(theElement,Mark);
  }

  return(GM_OK);
}


#ifdef ModelP

/****************************************************************************/
/*
   Gather_AddEdgePattern -

   SYNOPSIS:
   static int Gather_AddEdgePattern (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Gather_AddEdgePattern (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
        #ifdef UG_DIM_2
  INT pat;
  ELEMENT *theElement = (ELEMENT *)obj;

  pat = 0;
  GetEdgeInfo(theElement,&pat,ADDPATTERN);

  ((INT *)data)[0] = pat;

  PRINTDEBUG(gm,4,("Gather_AddEdgePattern(): elem=" EID_FMTX "pat=%08x\n",
                   EID_PRTX(theElement),pat));
  return(GM_OK);
        #endif

        #ifdef UG_DIM_3
  INT addpattern;
  EDGE    *theEdge = (EDGE *)obj;

  addpattern = ADDPATTERN(theEdge);

  ((INT *)data)[0] = addpattern;

  PRINTDEBUG(gm,4,("Gather_AddEdgePattern(): edge=" ID_FMTX "pat=%08x\n",
                   ID_PRTX(theEdge),addpattern));
  return(GM_OK);
        #endif
}


/****************************************************************************/
/*
   Scatter_AddEdgePattern -

   SYNOPSIS:
   static int Scatter_AddEdgePattern (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Scatter_AddEdgePattern (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
        #ifdef UG_DIM_2
  INT pat;
  ELEMENT *theElement = (ELEMENT *)obj;

  /** \todo (HRR 971207): output after SetEdgeInfo (pat not init)? */
  PRINTDEBUG(gm,4,("Scatter_AddEdgePattern(): elem=" EID_FMTX "pat=%08x\n",
                   EID_PRTX(theElement),pat));

  pat = ((INT *)data)[0];
  SetEdgeInfo(theElement,pat,ADDPATTERN,&);

  return(GM_OK);
        #endif

        #ifdef UG_DIM_3
  INT addpattern;
  EDGE    *theEdge = (EDGE *)obj;

  addpattern = std::min((INT)ADDPATTERN(theEdge),((INT *)data)[0]);

  PRINTDEBUG(gm,4,("Gather_AddEdgePattern(): edge=" ID_FMTX "pat=%08x\n",
                   ID_PRTX(theEdge),addpattern));

  SETADDPATTERN(theEdge,addpattern);

  return(GM_OK);
        #endif
}


/****************************************************************************/
/*
   ExchangeAddPatterns -

   SYNOPSIS:
   static INT ExchangeAddPatterns (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT ExchangeAddPatterns (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  /* exchange addpatterns of edges */
        #ifdef UG_DIM_2
  DDD_IFAOneway(context,
                dddctrl.ElementVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_AddEdgePattern, Scatter_AddEdgePattern);
        #endif
        #ifdef UG_DIM_3
  DDD_IFAOneway(context,
                dddctrl.EdgeVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_AddEdgePattern, Scatter_AddEdgePattern);
        #endif

  return(GM_OK);
}
#endif


/****************************************************************************/
/*
   SetAddPatterns -

   SYNOPSIS:
   static INT SetAddPatterns (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT SetAddPatterns (GRID *theGrid)
{
  INT j;
  ELEMENT *theElement;
  EDGE    *theEdge;

  /* set additional pattern on the edges */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (MARKCLASS(theElement)!=RED_CLASS) continue;

    REFINE_ELEMENT_LIST(1,theElement,"SetAddPatterns(): addpattern=0");

    for (j=0; j<EDGES_OF_ELEM(theElement); j++)
    {
      /* no green elements for this edge if there is no edge node */
      if (!NODE_OF_RULE(theElement,MARK(theElement),j))
        continue;

      theEdge=GetEdge(CORNER_OF_EDGE_PTR(theElement,j,0),
                      CORNER_OF_EDGE_PTR(theElement,j,1));
      ASSERT(theEdge != NULL);

      /* ADDPATTERN is now set to 0 for all edges of red elements */
      SETADDPATTERN(theEdge,0);
    }
  }

        #ifdef ModelP
  if (ExchangeAddPatterns(theGrid)) RETURN(GM_FATAL);
        #endif

  return(GM_OK);
}


/****************************************************************************/
/*
   BuildGreenClosure -

   SYNOPSIS:
   static INT BuildGreenClosure (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT BuildGreenClosure (const GRID *theGrid)
{
  INT i;
  ELEMENT *theElement;
  EDGE    *theEdge;
        #ifdef UG_DIM_3
  INT j;
        #endif

  /* build a green covering around the red elements */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
                #ifdef __ANISOTROPIC__
    if (MARKCLASS(theElement)==RED_CLASS &&
        !(TAG(theElement)==PRISM && MARK(theElement)==PRI_QUADSECT)) continue;

    ASSERT(MARKCLASS(theElement)!=RED_CLASS ||
           (MARKCLASS(theElement)==RED_CLASS && TAG(theElement)==PRISM
            && MARK(theElement)==PRI_QUADSECT));
                #else
    if (MARKCLASS(theElement)==RED_CLASS) continue;
                #endif

    SETUPDATE_GREEN(theElement,0);

    /* if edge node exists element needs to be green */
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      theEdge=GetEdge(CORNER_OF_EDGE_PTR(theElement,i,0),
                      CORNER_OF_EDGE_PTR(theElement,i,1));
      ASSERT(theEdge != NULL);

      /* if edge is refined this will be a green element */
      if (ADDPATTERN(theEdge) == 0)
      {
        /* for pyramids, prisms and hexhedra Patterns2Rules returns 0  */
        /* for non red elements, because there is no complete rule set */
        /* switch to mark COPY, because COPY rule refines no edges     */
#ifdef DUNE_UGGRID_TET_RULESET
        if (DIM==3 && TAG(theElement)!=TETRAHEDRON)
#else
        if (DIM==3)
#endif
        {
          /* set to no-empty rule, e.g. COPY rule */
                                        #ifdef __ANISOTROPIC__
          if (MARKCLASS(theElement) != RED_CLASS)
                                        #endif
          SETMARK(theElement,COPY);

          /* no existing edge node renew green refinement */
          if (MIDNODE(theEdge)==NULL)
          {
            SETUPDATE_GREEN(theElement,1);
          }
        }
        /* tetrahedra in 3D and 2D elements have a complete rule set */
        else if (MARK(theElement) == NO_REFINEMENT)
        {
          IFDEBUG(gm,2)
          UserWriteF("   ERROR: green tetrahedron with no rule! "
                     "EID=%d TAG=%d "
                     "REFINECLASS=%d REFINE=%d MARKCLASS=%d  MARK=%d\n",
                     ID(theElement),TAG(theElement),REFINECLASS(theElement),
                     REFINE(theElement),MARKCLASS(theElement),
                     MARK(theElement));
          ENDDEBUG
        }

                                #ifdef __ANISOTROPIC__
        if (MARKCLASS(theElement) != RED_CLASS)
                                #endif
        SETMARKCLASS(theElement,GREEN_CLASS);
      }
      else
      {
        /* existing edge node is deleted                         */
        /* renew green refinement if element will be a green one */
        if (MIDNODE(theEdge)!=NULL)
          SETUPDATE_GREEN(theElement,1);
      }
    }

                #ifdef UG_DIM_3
    /* if side node exists element needs to be green */
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      ELEMENT *theNeighbor;

      theNeighbor = NBELEM(theElement,i);

      if (theNeighbor==NULL) continue;

      for (j=0; j<SIDES_OF_ELEM(theNeighbor); j++)
        if (NBELEM(theNeighbor,j) == theElement)
          break;

                        #ifdef ModelP
      if (j >= SIDES_OF_ELEM(theNeighbor))
      {
        ASSERT(EGHOST(theElement) && EGHOST(theNeighbor));
        continue;
      }
                        #else
      ASSERT(j<SIDES_OF_ELEM(theNeighbor));
                        #endif

      if (NODE_OF_RULE(theNeighbor,MARK(theNeighbor),
                       EDGES_OF_ELEM(theNeighbor)+j))
      {
#ifdef DUNE_UGGRID_TET_RULESET
        if (TAG(theNeighbor)==TETRAHEDRON)
          printf("ERROR: no side nodes for tetrahedra! side=%d\n",j);
#endif
                                #ifdef __ANISOTROPIC__
        if (MARKCLASS(theElement) != RED_CLASS)
                                #endif
        SETMARKCLASS(theElement,GREEN_CLASS);
      }


      /* side node change? */
      if ((!NODE_OF_RULE(theNeighbor,REFINE(theNeighbor),
                         EDGES_OF_ELEM(theNeighbor)+j) &&
           NODE_OF_RULE(theNeighbor,MARK(theNeighbor),
                        EDGES_OF_ELEM(theNeighbor)+j)) ||
          (NODE_OF_RULE(theNeighbor,REFINE(theNeighbor),
                        EDGES_OF_ELEM(theNeighbor)+j) &&
           !NODE_OF_RULE(theNeighbor,MARK(theNeighbor),
                         EDGES_OF_ELEM(theNeighbor)+j)))
      {
        SETUPDATE_GREEN(theElement,1);
      }
    }
                #endif


#ifndef ModelP
    /* if element is green before refinement and will be green after */
    /* refinement and nothing changes -> reset USED flag             */
    /* in parallel case: one communication to determine the minimum  */
    /* over all copies of an green element would be needed           */
    if (REFINECLASS(theElement)==GREEN_CLASS &&
        MARKCLASS(theElement)==GREEN_CLASS && UPDATE_GREEN(theElement)==0)
    {
      /* do not renew green refinement */
      SETUSED(theElement,0);
    }
                #ifdef __ANISOTROPIC__
    if (MARKCLASS(theElement)==RED_CLASS && UPDATE_GREEN(theElement)==0)
    {
      ASSERT(TAG(theElement)==PRISM && MARK(theElement)==PRI_QUADSECT);
      SETUSED(theElement,0);
    }
                #endif
#endif
  }

  return(GM_OK);
}

#if defined(ModelP) && defined(Debug)

#define CEIL(n) ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))

/****************************************************************************/
/*
   Gather_ElementInfo -

   SYNOPSIS:
   static int Gather_ElementInfo (DDD_OBJ obj, void *Data);

   PARAMETERS:
   .  obj
   .  Data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Gather_ElementInfo (DDD::DDDContext&, DDD_OBJ obj, void *Data)
{
  INT epat,eaddpat;
  ELEMENT *theElement = (ELEMENT *)obj;
  char    *data = (char *)Data;

  PRINTDEBUG(gm,4,("Gather_ElementInfo(): elem=" EID_FMTX "\n",
                   EID_PRTX(theElement)));

  memcpy(data,theElement,sizeof(struct generic_element));
  data += CEIL(sizeof(struct generic_element));

  epat = 0;
  GetEdgeInfo(theElement,&epat,PATTERN);
  ((int *)data)[0] = epat;
  data += sizeof(INT);

  eaddpat = 0;
  GetEdgeInfo(theElement,&eaddpat,ADDPATTERN);
  ((int *)data)[0] = eaddpat;

  return(GM_OK);
}

/* this macro compares control word macros of two elements */
#define COMPARE_MACRO(elem0,elem1,macro,print)                               \
  if (macro(elem0) != macro(elem1))                                        \
  {                                                                        \
    print("e=" EID_FMTX " macro=%s differs value0=%d value1=%d \n",      \
          EID_PRTX(elem0),# macro,macro(elem0),macro(elem1));                   \
    assert(0);                                                           \
  }

#ifdef DUNE_UGGRID_TET_RULESET
#define COMPARE_MACROX(elem0,elem1,macro,print)                              \
     {                                                                        \
             INT _mark0,_mark1,_pat0,_pat1;                                       \
                                                                             \
             _mark0 = macro(elem0); _mark1 = macro(elem1);                        \
             _pat0 = MARK2PAT((elem0),_mark0); _pat1 = MARK2PAT((elem1),_mark1);  \
             if ((_pat0 & ((1<<10)-1)) != (_pat1 & ((1<<10)-1)))                  \
                     COMPARE_MACRO(elem0,elem1,macro,print)                           \
     }
#else
#define COMPARE_MACROX(elem0,elem1,macro,print)                              \
  COMPARE_MACRO(elem0,elem1,macro,print)
#endif

/* this macro compares two values */
#define COMPARE_VALUE(elem0,val0,val1,string,print)                          \
  if ((val0) != (val1))                                                    \
  {                                                                        \
    (print)("e=" EID_FMTX " %s differs value0=%d value1=%d \n",          \
            EID_PRTX(elem0),(string),(val0),(val1));                             \
    assert(0);                                                           \
  }


/****************************************************************************/
/*
   Scatter_ElementInfo -

   SYNOPSIS:
   static int Scatter_ElementInfo (DDD_OBJ obj, void *Data);

   PARAMETERS:
   .  obj
   .  Data

   DESCRIPTION:

   \return <ul>
   int
 */
/****************************************************************************/

static int Scatter_ElementInfo (DDD::DDDContext&, DDD_OBJ obj, void *Data)
{
  INT epat,mpat,eaddpat,maddpat;
  ELEMENT *theElement = (ELEMENT *)obj;
  struct generic_element ge;
  ELEMENT *theMaster = (ELEMENT *)&ge;
  char    *data = (char *)Data;

  memcpy(theMaster,data,sizeof(struct generic_element));
  data += CEIL(sizeof(struct generic_element));

  PRINTDEBUG(gm,4,("Scatter_ElementInfo(): Comparing elem="
                   EID_FMTX " master=" EID_FMTX "\n",EID_PRTX(theElement),
                   EID_PRTX(theMaster)));

  /* now compare the control entries of master with it local copy */
  COMPARE_MACRO(theElement,theMaster,REFINECLASS,PrintDebug)
  COMPARE_MACRO(theElement,theMaster,MARKCLASS,PrintDebug)
  COMPARE_MACROX(theElement,theMaster,REFINE,PrintDebug)
  COMPARE_MACROX(theElement,theMaster,MARK,PrintDebug)
  COMPARE_MACRO(theElement,theMaster,COARSEN,PrintDebug)
  COMPARE_MACRO(theElement,theMaster,USED,PrintDebug)
  /*	#ifndef DUNE_UGGRID_TET_RULESET */
        #ifdef UG_DIM_3
  COMPARE_MACRO(theElement,theMaster,SIDEPATTERN,PrintDebug)
        #endif
  /*	#endif */

  epat = 0;
  GetEdgeInfo(theElement,&epat,PATTERN);
  mpat = ((INT *)data)[0];
  data += sizeof(INT);
  COMPARE_VALUE(theElement,epat,mpat,"EdgePattern",PrintDebug)

  eaddpat = 0;
  GetEdgeInfo(theElement,&eaddpat,ADDPATTERN);
  maddpat = ((INT *)data)[0];
  COMPARE_VALUE(theElement,eaddpat,maddpat,"EdgeAddPattern",PrintDebug)

  return(GM_OK);
}

static INT CheckElementInfo (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  /* exchange element info */
  DDD_IFAOneway(context,
                dddctrl.ElementVHIF,GRID_ATTR(theGrid),IF_FORWARD,
                CEIL(sizeof(struct generic_element))+2*sizeof(INT),
                Gather_ElementInfo, Scatter_ElementInfo);

  return(GM_OK);
}
#endif


/****************************************************************************/
/*																			*/
/* Function:  GridClosure                                                                                                       */
/*																			*/
/* Purpose:   compute closure for next level. A closure can only be         */
/*			  determined if the rule set for the used elements is complete. */
/*                        This means that for all side and edge patterns possible for   */
/*			  an element type exists a rule which closes the element.       */
/*	          In this case a FIFO for computing the closure is not needed   */
/*	          any more and the closure can be computed in one step.			*/
/*																			*/
/* Param:	  GRID *theGrid: pointer to grid structure						*/
/*																			*/
/* return:	  INT >0: elements will be refined								*/
/*			  INT 0: no elements will be refined							*/
/*			  INT -1: an error occurred                                                              */
/*																			*/
/****************************************************************************/
/****************************************************************************/
/** \brief Compute closure for next level

   \param theGrid - pointer to grid structure

   This function computes closure for next level. A closure can only be
   determined if the rule set for the used elements is complete. This means
   that for all side and edge patterns possible for an element type exists
   a rule which closes the element.  In this case a FIFO for computing the
   closure is not needed any more and the closure can be computed in one step.

   \return <ul>
   .n   >0 elements will be refined
   .n   =0 no elements will be refined
   .n   =-1 an error occurred
 */
/****************************************************************************/

static int GridClosure (GRID *theGrid)
{
  INT cnt;

  /* initialize used control word entries */
  if (PrepareGridClosure(theGrid) != GM_OK) RETURN(GM_ERROR);

  /* compute pattern on edges and elements */
  if (ComputePatterns(theGrid) != GM_OK) RETURN(GM_ERROR);

  firstElement = PFIRSTELEMENT(theGrid);

  if (fifoFlag)
    if (InitClosureFIFO() != GM_OK) return(GM_OK);

  /* fifo loop */
  do
  {
                #ifdef UG_DIM_3
                #if defined(ModelP) && defined(DUNE_UGGRID_TET_RULESET)
    /* edge pattern is needed consistently in CorrectTetrahedronSidePattern() */
    if (!refine_seq)
    {
      if (ExchangeEdgeClosureInfo(theGrid) != GM_OK) return(GM_ERROR);
    }
                #endif

    /* set side patterns on the elements */
    if (SetElementSidePatterns(theGrid,firstElement) != GM_OK) RETURN(GM_ERROR);
                #endif

                #ifdef ModelP
    if (ExchangeClosureInfo(theGrid) != GM_OK) RETURN(GM_ERROR);
                #endif

    /* set rules on the elements */
    if (SetElementRules(theGrid,firstElement,&cnt) != GM_OK) RETURN(GM_ERROR);

  }
  /* exit only if fifo not active or fifo queue   */
  /* empty or all processor have finished closure */
  while (fifoFlag && UpdateClosureFIFO(theGrid) &&
         ManageParallelFIFO(theGrid->ppifContext(), firstElement));

  /* set patterns on all edges of red elements */
  if (SetAddPatterns(theGrid) != GM_OK) RETURN(GM_ERROR);

  /* build the closure around the red elements */
  if (BuildGreenClosure(theGrid) != GM_OK) RETURN(GM_ERROR);

        #if defined(Debug) && defined(ModelP)
  if (CheckElementInfo(theGrid)) RETURN(GM_ERROR);
        #endif

  return(cnt);
}



/****************************************************************************/
/*
   GetNeighborSons - fill SonList for theElement

   SYNOPSIS:
   static INT GetNeighborSons (ELEMENT *theElement, ELEMENT *theSon,
                                                        ELEMENT *SonList[MAX_SONS], int count, int nsons);

   PARAMETERS:
   .  theElement -	father element
   .  theSon - currently visited son
   .  SonList[MAX_SONS] - the list of sons
   .  count - son count
   .  sons - number of sons

   DESCRIPTION:
   This function fills SonList for theElement with a breadth first search

   \return <ul>
   INT
   .n   new son count
 */
/****************************************************************************/

static INT GetNeighborSons (ELEMENT *theElement, ELEMENT *theSon,
                            ELEMENT *SonList[MAX_SONS], int count, int nsons)
{
  ELEMENT *NbElement;
  int i,j, startson, stopson;

  startson = count;

  for (i=0; i<SIDES_OF_ELEM(theSon); i++)
  {
    NbElement = NBELEM(theSon,i);
    if (NbElement == NULL) continue;
    if (EFATHER(NbElement) == theElement)
    {
      /* is NbElement already in list */
      for (j=0; j<count; j++)
        if (SonList[j] == NbElement)
          break;
      if (j==count && count<nsons)
        SonList[count++] = NbElement;
    }
  }
  if (count == nsons) return(count);

  stopson = count;
  for (i=startson; i<stopson; i++)
  {
    if (count<nsons) count = GetNeighborSons(theElement,SonList[i],
                                             SonList,count,nsons);
    else return(count);
  }
  return(count);
}


#ifdef ModelP


/****************************************************************************/
/*
   GetSons - fill SonList for theElement

   SYNOPSIS:
   INT GetAllSons (ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);

   PARAMETERS:
   .  theElement
   .  SonList[MAX_SONS]

   DESCRIPTION:
   This function fills SonList for theElement

   \return <ul>
   INT
   .n   0 if ok
   .n   1 if an error occurs
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetAllSons (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS])
{
  ELEMENT *son;
  int SonID,i;

  ASSERT(theElement != NULL);

  for (SonID=0; SonID<MAX_SONS; SonID++)
    SonList[SonID] = NULL;

  if (NSONS(theElement) == 0) return(GM_OK);

  SonID = 0;

  for (i=0; i<2; i++)
  {
    if (i == 0)
      son = SON(theElement,PRIO2INDEX(PrioMaster));
    else
      son = SON(theElement,PRIO2INDEX(PrioHGhost));

    if (son == NULL)
      continue;
    else
      SonList[SonID++] = son;

    while (SUCCE(son) != NULL)
    {
      if (EFATHER(SUCCE(son)) == theElement
          && PRIO2INDEX(EPRIO(son))==PRIO2INDEX(EPRIO(SUCCE(son)))
          )
      {
        SonList[SonID++] = SUCCE(son);
        son = SUCCE(son);
        ASSERT(SonID <= MAX_SONS);
      }
      else
        break;
    }
  }

  return(GM_OK);
}
#endif


/****************************************************************************/
/*
   GetSons -

   SYNOPSIS:
   INT GetSons (ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);

   PARAMETERS:
   .  theElement
   .  SonList[MAX_SONS]

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetSons (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS])
{
  int SonID;
  ELEMENT *son;

  if (theElement==NULL) RETURN(GM_ERROR);

  for (SonID=0; SonID<MAX_SONS; SonID++)
    SonList[SonID] = NULL;

  if (NSONS(theElement) == 0) return(GM_OK);

  SonID = 0;
  SonList[SonID++] = son = SON(theElement,PRIO2INDEX(PrioMaster));

  if (son == NULL) return(GM_OK);

  while (SUCCE(son) != NULL)
  {
    if (EFATHER(SUCCE(son)) == theElement
                        #ifdef ModelP
        && PRIO2INDEX(EPRIO(son))==PRIO2INDEX(EPRIO(SUCCE(son)))
                        #endif
        )
    {
      SonList[SonID++] = SUCCE(son);
      son = SUCCE(son);
      ASSERT(SonID <= MAX_SONS);
    }
    else
      break;

  }

  return(GM_OK);
}

/****************************************************************************/
/*																			*/
/* Function:  RestrictElementMark							                */
/*																			*/
/* Purpose:   restrict refinement marks of an element whose sons are        */
/*		      further marked for refinement                                 */
/*																			*/
/* Param:	  ELEMENT *theElement: pointer to the element			        */
/*																			*/
/* return:	  INT: =0  ok													*/
/*				   >0  error												*/
/*																			*/
/****************************************************************************/
/****************************************************************************/
/*
   RestrictElementMark - restrict refinement marks

   SYNOPSIS:
   static INT RestrictElementMark(ELEMENT *theElement);

   PARAMETERS:
   .  theElement - pointer to the element

   DESCRIPTION:
   This function restricts refinement marks of an element whose sons are
   further marked for refinement

   \return <ul>
   INT
   .n   =0 - ok
   .n   >0 - error
 */
/****************************************************************************/

static INT RestrictElementMark(ELEMENT *theElement)
{
        #ifdef UG_DIM_3
        #ifdef DUNE_UGGRID_TET_RULESET
  EDGE *theEdge;
  int j,Rule,Pattern;
        #endif
        #endif

  if (MARKCLASS(theElement)==RED_CLASS)
  {
    /** \todo this mark is from DropMarks()!    */
    /* theElement is marked from outside       */
    /** \todo edit this for new element type or */
    /* for different restrictions              */
    switch (TAG(theElement))
    {
                        #ifdef UG_DIM_2
    case TRIANGLE :
      SETMARK(theElement,T_RED);
      break;
    case QUADRILATERAL :
      SETMARK(theElement,Q_RED);
      break;
                        #endif
                        #ifdef UG_DIM_3
    case TETRAHEDRON :
#ifdef DUNE_UGGRID_TET_RULESET
      if (MARK(theElement)!=RED)
        /** \todo Is REFINE always as red rule available? */
        SETMARK(theElement,REFINE(theElement));
#else
      SETMARK(theElement,TET_RED);
#endif
      break;
    case PYRAMID :
      SETMARK(theElement,PYR_RED);
      break;
    case PRISM :
      SETMARK(theElement,PRI_RED);
      break;
    case HEXAHEDRON :
      SETMARK(theElement,HEXA_RED);
      break;
                        #endif

    default :
      ASSERT(0);
    }
  }
  else
  {
    /** \todo edit this for new element type or for different restrictions */
    switch (TAG(theElement))
    {
                        #ifdef UG_DIM_2
    case TRIANGLE :
      SETMARK(theElement,T_RED);
      break;

    case QUADRILATERAL :
      SETMARK(theElement,Q_RED);
      break;
                        #endif
                        #ifdef UG_DIM_3
    case TETRAHEDRON :
#ifdef DUNE_UGGRID_TET_RULESET
      /* theElement is not marked from outside, */
      /* so find a reg. rule being consistent   */
      /* with those neighbors of all sons of    */
      /* theElement which are marked for refine.*/
      /* this choice will make sure these marks */
      /* will not be distroyed.				  */
      Pattern = RULE2PAT(theElement,
                         REFINE(theElement));
      for (j=0; j<EDGES_OF_ELEM(theElement); j++)
      {
        theEdge=GetEdge(CORNER_OF_EDGE_PTR(theElement,j,0),
                        CORNER_OF_EDGE_PTR(theElement,j,1));
        ASSERT(theEdge != NULL);

        /** \todo What's on when MIDNODE exists?? */
        if (MIDNODE(theEdge)==NULL)
        {
          theEdge=GetEdge(
            SONNODE(CORNER_OF_EDGE_PTR(theElement,j,0)),
            SONNODE(CORNER_OF_EDGE_PTR(theElement,j,1)));
          ASSERT(theEdge != NULL);

          /** \todo Is ADDPATTERN needed for fitting with other green elements?? */
          if (ADDPATTERN(theEdge))
            Pattern |= (1<<j);
          PRINTDEBUG(gm,4,("RestrictElementMark(): modified Pattern=%d bisects now edge=%d too\n",Pattern,j))
        }
      }
      Rule = PATTERN2RULE(theElement,Pattern);
      SETMARK(theElement,RULE2MARK(theElement,Rule));
      /** \todo delete this old code
         SETMARK(theElement,PATTERN2MARK(theElement,Pattern)); */
      /** \todo this would be the quick fix
         SETMARK(theElement,FULL_REFRULE); */
#else
      SETMARK(theElement,TET_RED);
#endif
      break;
    case PYRAMID :
      SETMARK(theElement,PYR_RED);
      break;
    case PRISM :
                                #ifdef __ANISOTROPIC__
      SETMARK(theElement,PRI_QUADSECT);
                                #else
      SETMARK(theElement,PRI_RED);
                                #endif
      break;
    case HEXAHEDRON :
      SETMARK(theElement,HEXA_RED);
      break;
                        #endif
    default :
      ASSERT(0);
    }
    SETMARKCLASS(theElement,RED_CLASS);
  }

  return(GM_OK);
}

/****************************************************************************/
/*
   RestrictMarks - restrict refinement marks when going down

   SYNOPSIS:
   static INT RestrictMarks (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:
   This function restricts refinement marks when going down

   \return <ul>
   INT
   .n   0 if ok
   .n  >0 if an error occurs
 */
/****************************************************************************/

static INT RestrictMarks (const GRID *theGrid)
{
  ELEMENT *theElement,*SonList[MAX_SONS];
  INT flag;

  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (GetSons(theElement,SonList)!=GM_OK) RETURN(GM_ERROR);

    if (hFlag)
    {
      if (
        /* if element is not refined anyway,                   */
        /* then there are no restrictions to apply             */
        REFINE(theElement) == NO_REFINEMENT ||

        /* irregular elements are marked by estimator,         */
        /* because they are leaf elements                      */
        ECLASS(theElement) == YELLOW_CLASS ||
        ECLASS(theElement) == GREEN_CLASS ||

        /* regular elements with YELLOW_CLASS copies are       */
        /* marked by estimator, because the marks are dropped  */
        REFINECLASS(theElement) == YELLOW_CLASS
        )
      {
        continue;
      }

      /* regular elements with GREEN_CLASS refinement */
      /* go to no refinement or red refinement        */
      if (REFINECLASS(theElement)==GREEN_CLASS)
      {
        for (UINT i=0; i<NSONS(theElement); i++)
        {
                                        #ifdef ModelP
          if (SonList[i] == NULL) break;
                                        #endif

          /* Is the son marked for further refinement */
          if (MARK(SonList[i])>NO_REFINEMENT)
          {
            if (RestrictElementMark(theElement)) RETURN(GM_ERROR);

            /* this must be done only once for each element */
            break;
          }
        }
        continue;
      }

      /* regular elements with regular refinement are */
      /* the only ones to coarsen                     */
      if (REFINECLASS(theElement) == RED_CLASS)
      {
                                #ifndef __ANISOTROPIC__
        SETMARK(theElement,REFINE(theElement));
        SETMARKCLASS(theElement,REFINECLASS(theElement));
                                #else
        ASSERT(MARK(theElement)>=1);
                                #endif
      }
    }

                #ifdef ModelP
    /* if no (or not all) sons are found by GetSons() on */
    /* this proc then coarsening is not allowed          */
    if (REFINECLASS(theElement)==RED_CLASS &&
        SonList[0]==NULL) continue;
                #endif

    flag = 0;
    for (UINT i=0; i < MAX_SONS && SonList[i]!=NULL; i++)
    {
      /* if not all sons are marked no unrefinement is possible */
      if (!COARSEN(SonList[i]) || REFINECLASS(SonList[i])==RED_CLASS)
      {
        flag = 1;
        break;
      }
    }

    if (flag) continue;

    /* preserve regular refinement marks */
    if (hFlag==0 && SonList[0]==NULL) continue;

    /* remove refinement */
    SETMARK(theElement,NO_REFINEMENT);
    SETMARKCLASS(theElement,NO_CLASS);
    SETCOARSEN(theElement,1);
  }

  return(GM_OK);
}

/****************************************************************************/
/*
   ComputeCopies - determine copy elements from node classes

   SYNOPSIS:
   static int ComputeCopies (GRID *theGrid);

   PARAMETERS:
   .  theGrid - pointer to grid structure

   DESCRIPTION:
   This function determines copy elements from node classes

   \return <ul>
   int
   .n   GM_OK if ok
 */
/****************************************************************************/

static int ComputeCopies (GRID *theGrid)
{
  ELEMENT *theElement;
  int flag;
  int cnt = 0;
  PRINTDEBUG(gm,1,("ComputeCopies on level %d\n",GLEVEL(theGrid)));

  [[maybe_unused]] const int me = theGrid->ppifContext().me();

  /* set class of all dofs on next level to 0 */
  ClearNextNodeClasses(theGrid);

  /* seed dofs of regularly and irregularly refined elements to 3 */
  flag = 0;
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (MARK(theElement)!=NO_REFINEMENT &&
        (MARKCLASS(theElement)==RED_CLASS ||
         MARKCLASS(theElement)==GREEN_CLASS))
    {
      SeedNextNodeClasses(theElement);
      flag=1;                   /* there is at least one element to be refined */
    }
  }

  /* copy all option or neighborhood */
  if (rFlag==GM_COPY_ALL)
  {
        #ifdef ModelP
    flag = UG_GlobalMaxINT(theGrid->ppifContext(), flag);
        #endif
    if (flag)
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
           theElement=SUCCE(theElement))
      {
        SeedNextNodeClasses(theElement);
      }
  }
  else
  {
    PropagateNextNodeClasses(theGrid);
  }

  /* an element is copied if it has a dof of class 2 and higher */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    INT maxclass = 0;

    if ((MARK(theElement)==NO_REFINEMENT)&&
        ((maxclass=MaxNextNodeClass(theElement))>=MINVNCLASS))
    {
      PRINTDEBUG(gm,1,(PFMT "ComputeCopies(): level=%d e=" EID_FMTX "yellow marked\n",
                       me,LEVEL(theElement),EID_PRTX(theElement)));
      SETMARK(theElement,COPY);
      SETMARKCLASS(theElement,YELLOW_CLASS);
      cnt++;
    }
    else
    {
      PRINTDEBUG(gm,1,(PFMT "ComputeCopies(): level=%d e=" EID_FMTX "not yellow marked"
                       " mark=%d maxclass=%d\n",
                       me,LEVEL(theElement),EID_PRTX(theElement),MARK(theElement),maxclass));
    }
  }

  return(cnt);
}

/****************************************************************************/
/*
   CheckElementContextConsistency - check NTYPE flags of nodes

   SYNOPSIS:
   static void CheckElementContextConsistency(ELEMENT *theElement);

   PARAMETERS:
   .  theElement - element to check

   DESCRIPTION:
   This function checks NTYPE flags of nodes in elementcontextt with the sons

   \return <ul>
   void
 */
/****************************************************************************/
#ifdef Debug
static void CheckElementContextConsistency(ELEMENT *theElement,
                                           ELEMENTCONTEXT theElementContext)
{
  int i;
  [[maybe_unused]] int errorflag = 0;
  int errortype[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];
  int correcttype[MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];

  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    errortype[i] = correcttype[i] = -1;


  /* check corner nodes */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    if (theElementContext[i] != NULL)
      if(!CORNERTYPE(theElementContext[i]))
      {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = CORNER_NODE;
      }

  /* check mid nodes */
  for (i=CORNERS_OF_ELEM(theElement);
       i<CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement);
       i++)
  {
    if (theElementContext[i] != NULL)
      if(NTYPE(theElementContext[i]) != MID_NODE)
      {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = MID_NODE;
      }
  }

        #ifdef UG_DIM_3
  /* check side nodes */
  for (i=CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement);
       i<CORNERS_OF_ELEM(theElement)+EDGES_OF_ELEM(theElement)+
       SIDES_OF_ELEM(theElement);
       i++)
  {
    if (theElementContext[i] != NULL)
      if(NTYPE(theElementContext[i]) != SIDE_NODE)
      {
        errortype[i] = NTYPE(theElementContext[i]);
        correcttype[i] = SIDE_NODE;
      }
  }

        #endif

  /* check center node */
  i = CORNERS_OF_ELEM(theElement)+CENTER_NODE_INDEX(theElement);
  if (theElementContext[i] != NULL)
    if(NTYPE(theElementContext[i]) != CENTER_NODE)
    {
      errortype[i] = NTYPE(theElementContext[i]);
      correcttype[i] = CENTER_NODE;
    }

  for (i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    if (errortype[i] != -1)
    {
      printf("ERROR: TAG=%d NTYPE(CONTEXT(i=%d)=%d should be %d\n",
             TAG(theElement),i,errortype[i],correcttype[i]);
      fflush(stdout);
      errorflag = 1;
    }

  ASSERT(errorflag == 0);
}
#endif

/****************************************************************************/
/*
   UpdateContext - assemble references

   SYNOPSIS:
   static int UpdateContext (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext);

   PARAMETERS:
   .  theGrid - grid level of the sons of theElement
   .  theElement - element to refine
   .  theContext - context structure to update

   DESCRIPTION:
   This function assembles references to objects which interact with the sons
   of the given element, i.e. objects are allocated, kept or deleted as
   indicated by MARK (i) corner nodes  (ii) nodes at midpoints of edges

   \return <ul>
   int
   .n   0 - ok
   .n   1 - fatal memory error
 */
/****************************************************************************/

static int UpdateContext (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext)
{
  /* reset context to NULL */
  for(INT i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    theElementContext[i] = NULL;

  /* is element to refine */
  if (!MARKED(theElement)) return(GM_OK);

  const INT Mark = MARK(theElement);

  /* allocate corner nodes if necessary */
  for (INT i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    NODE* theNode = CORNER(theElement,i);
    if (SONNODE(theNode)==NULL)
    {
      SONNODE(theNode) = CreateSonNode(theGrid,theNode);
      if (SONNODE(theNode)==NULL)
        RETURN(GM_FATAL);
                        #ifdef IDENT_ONLY_NEW
      SETNEW_NIDENT(SONNODE(theNode),1);
                        #endif
    }
    theElementContext[i] = SONNODE(theNode);
  }

  /* allocate edge midpoint nodes */
  /* nodes on refined edges */
  NODE** MidNodes = theElementContext+CORNERS_OF_ELEM(theElement);
  for (INT i=0; i<EDGES_OF_ELEM(theElement); i++)
  {
    const INT Corner0 = CORNER_OF_EDGE(theElement,i,0);
    const INT Corner1 = CORNER_OF_EDGE(theElement,i,1);

    bool toBisect = false;

    if (MARKED_NEW_GREEN(theElement))
    {
      const EDGE* theEdge = GetEdge(CORNER(theElement,Corner0),
                                    CORNER(theElement,Corner1));
      ASSERT(theEdge != NULL);

      if (ADDPATTERN(theEdge) == 0)
      {
        toBisect = true;
        MidNodes[i] = MIDNODE(theEdge);
      }
    }
                #ifndef __ANISOTROPIC__
    else
                #endif
    if (NODE_OF_RULE(theElement,Mark,i))
    {
      toBisect = true;
    }

    IFDEBUG(gm,2)
    if (MidNodes[i] == NULL)
      UserWriteF("\n    MidNodes[%d]: toBisect=%d ID(Corner0)=%d "
                 "ID(Corner1)=%d",
                 i,toBisect,ID(CORNER(theElement,Corner0)),
                 ID(CORNER(theElement,Corner1)));
    else
      UserWriteF("\n    MidNodes[%d]: toBisect=%d ID(Corner0)=%d "
                 "ID(Corner1)=%d"
                 " ID(MidNode)=%d",i,toBisect,ID(CORNER(theElement,Corner0)),
                 ID(CORNER(theElement,Corner1)),ID(MidNodes[i]));
    ENDDEBUG

    if (toBisect)
    {
      /* we need a midpoint node */
      if (MidNodes[i]!=NULL) continue;
      const NODE* Node0 = CORNER(theElement,Corner0);
      const NODE* Node1 = CORNER(theElement,Corner1);
      const EDGE* theEdge = GetEdge(Node0,Node1);
      if (theEdge == nullptr)
        RETURN(GM_FATAL);
      MidNodes[i] = MIDNODE(theEdge);
      /*			MidNodes[i] = GetMidNode(theElement,i); */
      if (MidNodes[i] == NULL)
      {
        MidNodes[i] = CreateMidNode(theGrid,theElement,NULL,i);
        if (MidNodes[i]==NULL)
          RETURN(GM_FATAL);
                                #ifdef IDENT_ONLY_NEW
        SETNEW_NIDENT(MidNodes[i],1);
                                #endif
        IFDEBUG(gm,2)
        UserWriteF(" created ID(MidNode)=%d for edge=%d",ID(MidNodes[i]),i);
        ENDDEBUG
      }
      assert(MidNodes[i]!=NULL);
    }
  }

  IFDEBUG(gm,2)
  UserWriteF("\n");
  ENDDEBUG

        #ifdef UG_DIM_3
  /* nodes on refined sides */
  NODE** SideNodes = theElementContext+CORNERS_OF_ELEM(theElement)+
              EDGES_OF_ELEM(theElement);
  for (INT i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    /* no side nodes for triangular sides yet */
#ifdef DUNE_UGGRID_TET_RULESET
    if (CORNERS_OF_SIDE(theElement,i) == 3) continue;
#endif

    bool toCreate = false;
    /* is side node needed */
    if (MARKED_NEW_GREEN(theElement))
    {
      const ELEMENT *theNeighbor = NBELEM(theElement,i);

      if (theNeighbor!=NULL)
      {
        if (MARKCLASS(theNeighbor)!=GREEN_CLASS &&
            MARKCLASS(theNeighbor)!=YELLOW_CLASS)
        {
          INT j;
          for (j=0; j<SIDES_OF_ELEM(theNeighbor); j++)
          {
            if (NBELEM(theNeighbor,j) == theElement) break;
          }
          ASSERT(j<SIDES_OF_ELEM(theNeighbor));
          if (NODE_OF_RULE(theNeighbor,MARK(theNeighbor),
                           EDGES_OF_ELEM(theNeighbor)+j))
            toCreate = true;
        }
      }
    }
                #ifndef __ANISOTROPIC__
    else
                #endif
    if (NODE_OF_RULE(theElement,Mark,EDGES_OF_ELEM(theElement)+i))
    {
      toCreate = true;
    }

    IFDEBUG(gm,2)
    if (SideNodes[i] == NULL)
      UserWriteF("    SideNode[%d]: create=%d old=%x",
                 i,toCreate,SideNodes[i]);
    else
      UserWriteF("    SideNode[%d]: create=%d old=%x oldID=%d",i,toCreate,
                 SideNodes[i],ID(SideNodes[i]));
    if (SideNodes[i] != NULL)
      if (START(SideNodes[i])!=NULL)
      {
        UserWriteF("\n NO_OF_ELEM of EDGES:");
        for (LINK *sidelink = START(SideNodes[i]);
             sidelink!=NULL;
             sidelink=NEXT(sidelink))
        {
          UserWriteF(" NO=%d NodeTo=%d",NO_OF_ELEM(MYEDGE(sidelink)),
                     ID(NBNODE(sidelink)));
        }
        UserWrite("\n");
      }
    ENDDEBUG

    if (toCreate)
    {
      const ELEMENT *theNeighbor = NBELEM(theElement,i);

      IFDEBUG(gm,1)
      if (theNeighbor != NULL)
      {
        IFDEBUG(gm,3)
        UserWriteF("    ID(theNeighbor)=%d nbadr=%x:\n",
                   ID(theNeighbor),theNeighbor);
        ENDDEBUG
      }
      else
      {
        /* this must be a boundary side */
        ASSERT(SIDE_ON_BND(theElement,i));
      }
      ENDDEBUG

      /*
                              if (MARKED_NEW_GREEN(theNeighbor) && USED(theNeighbor)==0)
       */
      if (theNeighbor !=NULL)
      {

        IFDEBUG(gm,3)
        UserWriteF("            Searching for side node already allocated:\n");
        ENDDEBUG

        /* check for side node */
          SideNodes[i] = GetSideNode(theElement,i);
      }

      if (SideNodes[i] == NULL)
      {

        /* allocate the sidenode */
        if ((SideNodes[i] = CreateSideNode(theGrid,theElement,NULL,i)) == NULL)
          RETURN(GM_FATAL);

                #ifdef IDENT_ONLY_NEW
        SETNEW_NIDENT(SideNodes[i],1);
                                #endif
      }

      IFDEBUG(gm,0)
      ASSERT(SideNodes[i]!=NULL);
      for (INT j = 0; j < EDGES_OF_SIDE(theElement,i); j++)
      {
        const EDGE *fatherEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement,EDGE_OF_SIDE(theElement,i,j),0),
                                         CORNER_OF_EDGE_PTR(theElement,EDGE_OF_SIDE(theElement,i,j),1));

        [[maybe_unused]] const NODE* Node0 = MIDNODE(fatherEdge);

        /* if side node exists all mid nodes must exist */
        ASSERT(Node0 != NULL);
      }
      ENDDEBUG
    }

    IFDEBUG(gm,2)
    if (SideNodes[i] != NULL)
      UserWriteF(" new=%x newID=%d\n",SideNodes[i],ID(SideNodes[i]));
    else
      UserWriteF(" new=%x\n",SideNodes[i]);
    ENDDEBUG
  }
        #endif

  /* allocate center node */
  NODE** CenterNode = MidNodes+CENTER_NODE_INDEX(theElement);
  CenterNode[0] = NULL;

  bool toCreate = false;
  if (CenterNode[0] == NULL)
  {

    if (MARKED_NEW_GREEN(theElement))
    {
      toCreate = true;
    }
                #ifndef __ANISOTROPIC__
    else
                #endif
    if (NODE_OF_RULE(theElement,Mark,CENTER_NODE_INDEX(theElement)))
    {
      toCreate = true;
    }
  }

  IFDEBUG(gm,2)
  if (CenterNode[0] == NULL)
    UserWriteF("    CenterNode: create=%d old=%x",toCreate,CenterNode[0]);
  else
    UserWriteF("    CenterNode: create=%d old=%x oldID=%d",toCreate,
               CenterNode[0],ID(CenterNode[0]));
  ENDDEBUG

  if (toCreate)
  {
    if ((CenterNode[0] = CreateCenterNode(theGrid,theElement,NULL)) == NULL)
      RETURN(GM_FATAL);
  }

#ifdef ModelP
  /* mark nodes as needed */
  for(INT i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
  {
    if (theElementContext[i] != NULL) SETUSED(theElementContext[i],1);
  }
#endif

  IFDEBUG(gm,2)
  if (CenterNode[0] != NULL)
    UserWriteF(" new=%x newID=%d\n",CenterNode[0],ID(CenterNode[0]));
  else
    UserWriteF(" new=%x\n",CenterNode[0]);
  ENDDEBUG

  return(GM_OK);
}


/****************************************************************************/
/*
   UnrefineElement - remove previous refinement

   SYNOPSIS:
   static INT UnrefineElement (GRID *theGrid, ELEMENT *theElement);

   PARAMETERS:
   .  theGrid: grid level of sons of theElement
   .  theElement: element to refine

   DESCRIPTION:
   This function removes previous refinement for an element and all sonelement
   recursively, deletes: (i) all connections, (ii) all interior nodes and edges
   are deleted, (iii) sons are deleted and references to sons reset to NULL.

   \return <ul>
   INT
 */
/****************************************************************************/

static INT UnrefineElement (GRID *theGrid, ELEMENT *theElement)
{
  int s;
  ELEMENT *theSon,*SonList[MAX_SONS];

  /* something to do ? */
  if ((REFINE(theElement)==NO_REFINEMENT)||(theGrid==NULL)) return(GM_OK);

  if (GetAllSons(theElement,SonList)!=GM_OK) RETURN(GM_FATAL);

  for (s=0; s<MAX_SONS && SonList[s]!=NULL; s++)
  {
    theSon = SonList[s];
    SETMARK(theSon,NO_REFINEMENT);
    if (IS_REFINED(theSon))
    {
      if (UnrefineElement(UPGRID(theGrid),theSon)) RETURN(GM_FATAL);
    }
  }

  /* remove son elements */
        #ifndef ModelP
  IFDEBUG(gm,1)
  if (!REFINED_NEW_GREEN(theElement))
  {
    if (NSONS(theElement) != NSONS_OF_RULE(MARK2RULEADR(theElement,
                                                        REFINE(theElement))))
    {
      UserWriteF("ERROR: NSONS=%d but rule.sons=%d\n",NSONS(theElement),
                 NSONS_OF_RULE(MARK2RULEADR(theElement,REFINE(theElement))));
    }
  }
  ENDDEBUG
        #endif

  [[maybe_unused]] const int me = theGrid->ppifContext().me();
  for (s=0; s<MAX_SONS && SonList[s]!=NULL; s++)
  {
    /** \todo delete special debug */
    /* if (ID(SonList[s])==11644) { RETURN(GM_FATAL); } */
    PRINTDEBUG(gm,1,(PFMT "UnrefineElement(): DisposeElement[%d]="
                     EID_FMTX "\n",me,s,EID_PRTX(SonList[s])));

    if (DisposeElement(theGrid,SonList[s])!=0)
      RETURN(GM_FATAL);
  }

  return (GM_OK);
}


START_UGDIM_NAMESPACE

struct compare_record
{
  /** \brief Element to connect */
  ELEMENT *elem;

  /** \brief Side of element to connect */
  INT side;

  /** \brief Number of nodes of side */
  INT nodes;

  /** \brief Number of nodes in descending order */
  NODE *nodeptr[4];
};
typedef struct compare_record COMPARE_RECORD;

END_UGDIM_NAMESPACE

INT NS_DIM_PREFIX GetSonSideNodes (const ELEMENT *theElement, INT side, INT *nodes,
                                   NODE *SideNodes[MAX_SIDE_NODES], INT ioflag)
{
  INT i,ncorners,nedges;

  ncorners = CORNERS_OF_SIDE(theElement,side);
  nedges = EDGES_OF_SIDE(theElement,side);
  (*nodes) = 0;


  /* reset pointers */
  for (i=0; i<MAX_SIDE_NODES; i++)
  {
    SideNodes[i] = NULL;
  }

  /* determine corner nodes */
  for (i=0; i<ncorners; i++)
  {
    SideNodes[i] = SONNODE(CORNER_OF_SIDE_PTR(theElement,side,i));
        #ifndef ModelP
    assert(SideNodes[i]!=NULL);
                #endif
    if (!ioflag)
      assert(SideNodes[i]==NULL || CORNERTYPE(SideNodes[i]));
    (*nodes)++;
  }

  /* determine mid nodes */
  for (i=0; i<nedges; i++)
  {
    /** \todo delete
       #ifdef UG_DIM_2
       ASSERT(OBJT(NFATHER(SideNodes[i])) == NDOBJ);
       ASSERT(OBJT(NFATHER(SideNodes[i+1])) == NDOBJ);
        theEdge = GetEdge((NODE *)NFATHER(SideNodes[i]),
                                          (NODE *)NFATHER(SideNodes[i+1]));
       #endif
       #ifdef UG_DIM_3
       ASSERT(OBJT(NFATHER(SideNodes[i])) == NDOBJ);
       ASSERT(OBJT(NFATHER(SideNodes[(i+1)%nedges])) == NDOBJ);
        theEdge = GetEdge((NODE *)NFATHER(SideNodes[i]),
                                          (NODE *)NFATHER(SideNodes[(i+1)%nedges]));
       #endif
        assert(theEdge != NULL);

        IFDEBUG(gm,4)
        UserWriteF("theEdge=%x midnode=%x\n",theEdge,MIDNODE(theEdge));
        ENDDEBUG

        if (MIDNODE(theEdge) != NULL)
        {
                SideNodes[ncorners+i] = MIDNODE(theEdge);
                assert(NTYPE(MIDNODE(theEdge)) == MID_NODE);
                (*nodes)++;
        }
     */

    SideNodes[ncorners+i] = GetMidNode(theElement,EDGE_OF_SIDE(theElement,side,i));
    if (SideNodes[ncorners+i] != NULL)
    {
      assert(NTYPE(SideNodes[ncorners+i]) == MID_NODE);
      (*nodes)++;
    }
  }

        #ifdef UG_DIM_3
  /* determine side node */
  {
    NODE *theNode;

    theNode = GetSideNode(theElement,side);
    if (theNode != NULL)
    {
      (*nodes)++;
    }

    SideNodes[ncorners+nedges] = theNode;

    IFDEBUG(gm,4)
    UserWriteF("sidenode=%x\n",theNode);
    ENDDEBUG
  }
        #endif

  IFDEBUG(gm,2)
  UserWriteF("GetSonSideNodes\n");
  for (i=0; i<MAX_SIDE_NODES; i++) UserWriteF(" %5d",i);
  UserWriteF("\n");
  for (i=0; i<MAX_SIDE_NODES; i++)
    if (SideNodes[i]!=NULL) UserWriteF(" %5d",ID(SideNodes[i]));
  UserWriteF("\n");
  ENDDEBUG

  return(GM_OK);
}


/****************************************************************************/
/*
   compare_node -

   SYNOPSIS:
   static bool compare_node (const NODE* a, const NODE* b)

   PARAMETERS:
   .  a
   .  b

   DESCRIPTION:

   \return <ul>
   bool
 */
/****************************************************************************/

static bool compare_node(const NODE* a, const NODE* b)
{
  return a > b;
}

/****************************************************************************/
/** \brief Get the sons of an element side

   \param[in] theElement Input element
   \param[in] side Input element side number
   \param[out] Sons_of_Side Number of topological sons of the element side
   \param[in,out] SonList[MAX_SONS] Output elements
   \param[out] SonSides Output element side numbers
   \param[in] NeedSons If this is false, the correct list of sons is expected to be
   provided in SonList.  If not it is recomputed.
   \param[in] ioflag An obsolete debugging flag
   \param[in] useRefineClass

   For a given side of an element, this routine computes all element sides
   on the next finer grid level which are topological sons of the input
   element side.
 */
/****************************************************************************/

INT NS_DIM_PREFIX Get_Sons_of_ElementSide (const ELEMENT *theElement, INT side, INT *Sons_of_Side,
                                           ELEMENT *SonList[MAX_SONS], INT *SonSides,
                                           INT NeedSons, INT ioflag,
                                           INT useRefineClass)
{
  enum MarkClass markclass;

  /* reset soncount */
  *Sons_of_Side = 0;

  /* get sons of element */
  if (NeedSons)
    if (GetAllSons(theElement,SonList) != GM_OK) RETURN(GM_FATAL);

  IFDEBUG(gm,2)
  UserWriteF("    Get_Sons_of_ElementSide():"
             " id=%d tag=%d, refineclass=%d markclass=%d refine=%d mark=%d coarse=%d"
             " used=%d nsons=%d side=%d needsons=%d\n",
             ID(theElement),TAG(theElement),REFINECLASS(theElement),MARKCLASS(theElement),
             REFINE(theElement),MARK(theElement),COARSEN(theElement),
             USED(theElement),NSONS(theElement),side,NeedSons);
  for (INT i = 0; i < MAX_SONS && SonList[i] != NULL; i++)
    UserWriteF("   son[%d]=" EID_FMTX "\n",i,EID_PRTX(SonList[i]));
  ENDDEBUG

        #ifdef UG_DIM_2
  markclass = RED_CLASS;
        #endif
        #ifdef UG_DIM_3
  /* The following line used to read: markclass = (enum MarkClass) MARKCLASS(theElement);
     This works well within the UG grid refinement context.  However, now I want to use this
     method within the DUNE UGGridLeafIntersectionIterator.  The problem is that the user
     may have randomly marked elements before calling the iterator.  In that case the
     mark classes may not be set the way this method expects it.  Hence we allow the option
     to use REFINECLASS instead.  To be absolutely certain we don't break existing code
     we keep the old behaviour as the default.*/
  markclass = (enum MarkClass) ((useRefineClass) ? REFINECLASS(theElement) : MARKCLASS(theElement));
        #endif

  /** \todo quick fix */
        #ifdef ModelP
  if (EHGHOST(theElement))
    markclass = GREEN_CLASS;
        #endif

  /* select sons to connect */
  switch (markclass)
  {

  case YELLOW_CLASS :
  {
    *Sons_of_Side = 1;
    SonSides[0] = side;
    break;
  }

  case GREEN_CLASS :
  case RED_CLASS :
  {
    /* determine sonnodes of side */
    NODE *SideNodes[MAX_SIDE_NODES];
    INT nodes;
    INT nsons = 0;

    /* determine nodes of sons on side of element */
    GetSonSideNodes(theElement,side,&nodes,SideNodes,ioflag);

    /* sort side nodes in descending address order */
    std::sort(SideNodes, SideNodes + MAX_SIDE_NODES, compare_node);

    IFDEBUG(gm,3)
    UserWriteF("After qsort:\n");
    for (INT i = 0; i < MAX_SIDE_NODES; i++) UserWriteF(" %8d",i);
    UserWriteF("\n");
    for (INT i = 0; i < MAX_SIDE_NODES; i++)
      if (SideNodes[i]!=NULL) UserWriteF(" %x",SideNodes[i]);
      else UserWriteF(" %8d",0);
    UserWriteF("\n");
    ENDDEBUG

    /* determine sonnode on side */
    /*			for (i=0; i<NSONS(theElement); i++) */
    for (INT i = 0; i<MAX_SONS && SonList[i] != NULL; i++)
    {
      INT n = 0;
      std::array<INT,MAX_CORNERS_OF_SIDE> corner;
      std::fill(corner.begin(), corner.end(), -1);

      IFDEBUG(gm,4)
      UserWriteF("son=%d\n",i);
      ENDDEBUG

      /* soncorners on side */
      for (INT j = 0; j < CORNERS_OF_ELEM(SonList[i]); j++)
      {
        const NODE *nd = CORNER(SonList[i], j);
        if (std::binary_search(SideNodes, SideNodes + nodes, nd, compare_node))
        {
          corner[n] = j;
          n++;
        }
      }
      assert(n<5);

      IFDEBUG(gm,4)
      UserWriteF("\n nodes on side n=%d:",n);
      for (INT j = 0; j < MAX_CORNERS_OF_SIDE; j++)
        UserWriteF(" %d",corner[j]);
      ENDDEBUG


      IFDEBUG(gm,0)
      if (n==3) assert(TAG(SonList[i])!=HEXAHEDRON);
      if (n==4) assert(TAG(SonList[i])!=TETRAHEDRON);
      ENDDEBUG

      /* sonside on side */
                                #ifdef UG_DIM_2
      assert(n<=2);
      if (n==2)
      {
        if (corner[0]+1 == corner[1])
          SonSides[nsons] = corner[0];
        else
        {
          /** \todo find proper assert
              assert(corner[1] == CORNERS_OF_ELEM(theElement)-1); */
          SonSides[nsons] = corner[1];
        }
        SonList[nsons] = SonList[i];
        nsons++;
      }
                                #endif
                                #ifdef UG_DIM_3
      if (n==3 || n==4)
      {
        /* determine side number */
        INT edge0 = EDGE_WITH_CORNERS(SonList[i],corner[0],corner[1]);
        INT edge1 = EDGE_WITH_CORNERS(SonList[i],corner[1],corner[2]);
        /* corners are not stored in local side numbering,      */
        /* therefore corner[x]-corner[y] might be the diagonal  */
        if (n==4 && edge0==-1)
          edge0 = EDGE_WITH_CORNERS(SonList[i],corner[0],
                                    corner[3]);
        if (n==4 && edge1==-1)
          edge1 = EDGE_WITH_CORNERS(SonList[i],corner[1],
                                    corner[3]);
        assert(edge0!=-1 && edge1!=-1);

        INT sonside = -1;
        for (INT side0 = 0; side0 < MAX_SIDES_OF_EDGE; side0++)
        {
          for (INT side1 = 0; side1 < MAX_SIDES_OF_EDGE; side1++)
          {
            IFDEBUG(gm,5)
            UserWriteF("edge0=%d side0=%d SIDE_WITH_EDGE=%d\n",
                       edge0, side0,
                       SIDE_WITH_EDGE(SonList[i],edge0,side0));
            UserWriteF("edge1=%d side1=%d SIDE_WITH_EDGE=%d\n",
                       edge1, side1,
                       SIDE_WITH_EDGE(SonList[i],edge1,side1));
            ENDDEBUG
            if (SIDE_WITH_EDGE(SonList[i],edge0,side0) ==
                SIDE_WITH_EDGE(SonList[i],edge1,side1))
            {
              sonside = SIDE_WITH_EDGE(SonList[i],edge0,side0);
              break;
            }
          }
          if (sonside != -1) break;
        }
        assert(sonside != -1);
        IFDEBUG(gm,4)
        UserWriteF(" son[%d]=%x with sonside=%d on eside=%d\n",
                   i,SonList[i],sonside,side);
        ENDDEBUG

        IFDEBUG(gm,3)

        for (INT k = 0; k < SIDES_OF_ELEM(SonList[i]); k++)
        {
          const ELEMENT *Nb = NBELEM(SonList[i],k);
          if (Nb!=NULL)
          {
            INT j;
            for (j=0; j<SIDES_OF_ELEM(Nb); j++)
            {
              if (NBELEM(Nb,j)==SonList[i]) break;
            }
            if (j<SIDES_OF_ELEM(Nb))
              UserWriteF(" sonside=%d has backptr to son "
                         "Nb=%x Nbside=%d\n",k,Nb,j);
          }
        }
        ENDDEBUG

        ASSERT(CORNERS_OF_SIDE(SonList[i],sonside) == n);

        SonSides[nsons] = sonside;
        SonList[nsons] = SonList[i];
        nsons++;
      }
                                #endif
    }
                        #ifndef ModelP
    assert(nsons>0 && nsons<6);
                        #endif

    IFDEBUG(gm,3)
    UserWriteF(" nsons on side=%d\n",nsons);
    ENDDEBUG

    *Sons_of_Side = nsons;
    break;
  }

    /* old style
       This case would work for the sequential case if SonList is ordered according to
       rule (e.g. introduce GetOrderedSonList()). The upper case, used now in each
       situation, is a factor of 2
       slower for uniform refinement!! (s.l. 981016)
       #ifndef ModelP
                    case RED_CLASS:
       #endif
     */
    {
      INT nsons = 0;
      for (INT i = 0; i < MAX_SONS && SonList[i] != NULL; i++)
      {
        const SONDATA *sondata = SON_OF_RULE(MARK2RULEADR(theElement, MARK(theElement)), i);

        for (INT j = 0; j < SIDES_OF_ELEM(SonList[i]); j++)
          if (SON_NB(sondata,j) == FATHER_SIDE_OFFSET+side)
          {
            SonSides[nsons] = j;
            SonList[nsons] = SonList[i];
            nsons ++;
          }
      }
      *Sons_of_Side = nsons;
      break;
    }

  default :
    RETURN(GM_FATAL);
  }

        #ifdef ModelP
  IFDEBUG(gm,4)
  UserWriteF("Sons_of_Side=%d\n",*Sons_of_Side);
  for (INT i = 0; i < *Sons_of_Side; i++)
    UserWriteF("son[%d]=" EID_FMTX " sonside[%d]=%d\n",i,
               EID_PRTX(SonList[i]),i,SonSides[i]);
  ENDDEBUG
        #endif

  for (INT i = *Sons_of_Side; i < MAX_SONS; i++)
    SonList[i] = NULL;

  return(GM_OK);
}


/****************************************************************************/
/*
   Sort_Node_Ptr -

   SYNOPSIS:
   static INT Sort_Node_Ptr (INT n,NODE **nodes);

   PARAMETERS:
   .  n
   .  nodes

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT Sort_Node_Ptr (INT n,NODE **nodes)
{
  NODE* nd;
  INT i,j,max;

  max = 0;

  switch (n)
  {

                #ifdef UG_DIM_2
  case 2 :
                #endif
                #ifdef UG_DIM_3
  case 3 :
  case 4 :
                #endif
    for (i=0; i<n; i++)
    {
      max = i;
      for (j=i+1; j<n; j++)
        if (nodes[max]<nodes[j]) max = j;
      if (i != max)
      {
        nd = nodes[i];
        nodes[i] = nodes[max];
        nodes[max] = nd;
      }
    }
    break;

  default :
    RETURN(GM_FATAL);
  }

  return(GM_OK);
}

/****************************************************************************/
/*
   Fill_Comp_Table -

   SYNOPSIS:
   static INT  Fill_Comp_Table (COMPARE_RECORD **SortTable, COMPARE_RECORD *Table, INT nelems, ELEMENT **Elements, INT *Sides);

   PARAMETERS:
   .  SortTable
   .  Table
   .  nelems
   .  Elements
   .  Sides

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

static INT      Fill_Comp_Table (COMPARE_RECORD **SortTable, COMPARE_RECORD *Table, INT nelems,
                                 ELEMENT **Elements, const INT *Sides)
{
  for (INT i = 0; i < nelems; i++)
  {
    SortTable[i] = Table+i;
    COMPARE_RECORD *Entry = Table+i;
    Entry->elem = Elements[i];
    Entry->side = Sides[i];
    Entry->nodes = CORNERS_OF_SIDE(Entry->elem,Entry->side);
    for (INT j = 0; j < CORNERS_OF_SIDE(Entry->elem, Entry->side); j++)
      Entry->nodeptr[j] = CORNER_OF_SIDE_PTR(Entry->elem,Entry->side,j);
    if (Sort_Node_Ptr(Entry->nodes,Entry->nodeptr)!=GM_OK) RETURN(GM_FATAL);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   compare_nodes -

   SYNOPSIS:
   static bool compare_nodes(const COMPARE_RECORD* a, const COMPARE_RECORD* b)

   PARAMETERS:
   .  a
   .  b

   DESCRIPTION:

   \return <ul>
   bool
 */
/****************************************************************************/

static bool compare_nodes(const COMPARE_RECORD* a, const COMPARE_RECORD* b)
{
  const int n = (a->nodes == 4 && b->nodes == 4) ? 4 : 3;

  for (int i = 0; i < n; ++i) {
    if (a->nodeptr[i] > b->nodeptr[i])
      return true;
    else if (a->nodeptr[i] < b->nodeptr[i])
      return false;
  }

  return false;
}

/****************************************************************************/
/*
   Connect_Sons_of_ElementSide -

   SYNOPSIS:
   INT Connect_Sons_of_ElementSide (GRID *theGrid, ELEMENT *theElement, INT side, INT Sons_of_Side, ELEMENT **Sons_of_Side_List, INT *SonSides, INT ioflag);

   PARAMETERS:
   .  theGrid
   .  theElement
   .  side
   .  Sons_of_Side
   .  Sons_of_Side_List
   .  SonSides
   .  ioflag

   DESCRIPTION:

   \return <ul>
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX Connect_Sons_of_ElementSide (GRID *theGrid, ELEMENT *theElement, INT side, INT Sons_of_Side, ELEMENT **Sons_of_Side_List, INT *SonSides, INT ioflag)
{
  COMPARE_RECORD ElemSonTable[MAX_SONS];
  COMPARE_RECORD NbSonTable[MAX_SONS];
  COMPARE_RECORD *ElemSortTable[MAX_SONS];
  COMPARE_RECORD *NbSortTable[MAX_SONS];

  ELEMENT *theNeighbor;
  ELEMENT *Sons_of_NbSide_List[MAX_SONS];
  INT nbside,Sons_of_NbSide,NbSonSides[MAX_SONS];
  INT k;

  IFDEBUG(gm,2)
  UserWriteF("Connect_Sons_of_ElementSide: ID(elem)=%d side=%d "
             "Sons_of_Side=%d\n",ID(theElement),side,Sons_of_Side);
  REFINE_ELEMENT_LIST(0,theElement,"theElement:");
  ENDDEBUG

  if (Sons_of_Side <= 0) return(GM_OK);

  /* connect to boundary */
  if (OBJT(theElement)==BEOBJ && SIDE_ON_BND(theElement,side))
  {
    /** \todo connect change test */

    for (int i = 0; i < Sons_of_Side; i++)
    {

      assert(OBJT(Sons_of_Side_List[i])==BEOBJ);
      if (CreateSonElementSide(theGrid,theElement,side,
                               Sons_of_Side_List[i],SonSides[i]) != GM_OK)
      {
        return(GM_FATAL);
      }
    }

    /* internal boundaries not connected */
    /*		return(GM_OK); */
  }

  /* connect to neighbor element */
  theNeighbor = NBELEM(theElement,side);

  if (theNeighbor==NULL) return(GM_OK);

  /* master elements only connect to master elements     */
  /* ghost elements connect to ghost and master elements */
        #ifdef ModelP
  if (!ioflag && EMASTER(theElement) && EHGHOST(theNeighbor))
    return(GM_OK);
        #endif

  /* only yellow elements may have no neighbors */
  if (MARKCLASS(theNeighbor)==NO_CLASS)
  {

    if (hFlag) assert(MARKCLASS(theElement)==YELLOW_CLASS);

    return(GM_OK);
  }

  if (REFINEMENT_CHANGES(theNeighbor)) return(GM_OK);

  /* determine corresponding side of neighbor */
  for (nbside=0; nbside<SIDES_OF_ELEM(theNeighbor); nbside++)
    if (NBELEM(theNeighbor,nbside) == theElement) break;
  assert(nbside<SIDES_OF_ELEM(theNeighbor));

  /* get sons of neighbor to connect */
  Get_Sons_of_ElementSide(theNeighbor,nbside,&Sons_of_NbSide,
                          Sons_of_NbSide_List,NbSonSides,1,ioflag);

        #ifdef ModelP
  if (!ioflag)
  {
        #endif
  /* match exactly */
  if (1) ASSERT(Sons_of_Side == Sons_of_NbSide && Sons_of_NbSide>0
                && Sons_of_NbSide<6);
  else
  if (!(Sons_of_Side == Sons_of_NbSide && Sons_of_NbSide>0
        && Sons_of_NbSide<6))
  {
    ELEMENT *elem=NULL;
    ELEMENT *SonList[MAX_SONS];

    REFINE_ELEMENT_LIST(0,theElement,"theElement:");
    REFINE_ELEMENT_LIST(0,theNeighbor,"theNeighbor:");
    UserWriteF("elem=" EID_FMTX " nb=" EID_FMTX
               "Sons_of_Side=%d Sons_of_NbSide=%d\n",EID_PRTX(theElement),
               EID_PRTX(theNeighbor),Sons_of_Side,Sons_of_NbSide);
    fflush(stdout);
    GetAllSons(theElement,SonList);
    for (int i = 0; i < MAX_SONS && SonList[i] != NULL; i++)
    {
      REFINE_ELEMENT_LIST(0,SonList[i],"son:");
    }
    GetAllSons(theNeighbor,SonList);
    for (int i = 0; i < MAX_SONS && SonList[i] != NULL; i++)
    {
      REFINE_ELEMENT_LIST(0,SonList[i],"nbson:");
    }

    /* sig bus error to see stack in totalview */
    SET_NBELEM(elem,0,NULL);
    assert(0);

  }
#ifdef ModelP
  }
#endif

  IFDEBUG(gm,2)
  UserWriteF("Connect_Sons_of_ElementSide: NBID(elem)=%d side=%d "
             "Sons_of_Side=%d\n",ID(theNeighbor),nbside,Sons_of_NbSide);
  REFINE_ELEMENT_LIST(0,theNeighbor,"theNeighbor:");
  ENDDEBUG

  /* fill sort and comparison tables */
  Fill_Comp_Table(ElemSortTable,ElemSonTable,Sons_of_Side,Sons_of_Side_List,
                  SonSides);
  Fill_Comp_Table(NbSortTable,NbSonTable,Sons_of_NbSide,Sons_of_NbSide_List,
                  NbSonSides);

  IFDEBUG(gm,5)

  if (!ioflag)
  {
    UserWriteF("BEFORE qsort\n");

    /* test whether all entries are corresponding */
    for (int i = 0; i < Sons_of_Side; i++)
    {
      COMPARE_RECORD *Entry, *NbEntry;

      Entry = ElemSortTable[i];
      NbEntry = NbSortTable[i];

      if (Entry->nodes != NbEntry->nodes)
        UserWriteF("Connect_Sons_of_ElementSide(): LIST Sorttables[%d]"
                   " eNodes=%d nbNodes=%d\n",
                   i,Entry->nodes,NbEntry->nodes);
      for (int j = 0; j < Entry->nodes; j++)
        UserWriteF("Connect_Sons_of_ElementSide(): LIST Sorttables[%d][%d]"
                   " eNodePtr=%d/%8x/%d nbNodePtr=%d/%8x/%d\n",
                   i,j,
                   ID(Entry->nodeptr[j]),Entry->nodeptr[j],NTYPE(Entry->nodeptr[j]),
                   ID(NbEntry->nodeptr[j]),NbEntry->nodeptr[j],NTYPE(NbEntry->nodeptr[j]));
      UserWriteF("\n");
    }
    UserWriteF("\n\n");
  }
  ENDDEBUG

  /* qsort the tables using nodeptrs */
  std::sort(ElemSortTable, ElemSortTable + Sons_of_Side, compare_nodes);
  std::sort(NbSortTable, NbSortTable + Sons_of_NbSide, compare_nodes);

        #ifdef ModelP
  if (!ioflag && Sons_of_NbSide!=Sons_of_Side) ASSERT(0);
        #endif

        #ifdef Debug
  if (!ioflag)
    /* check whether both sort table match exactly */
    for (int i = 0; i < Sons_of_Side; i++)
    {
      COMPARE_RECORD *Entry, *NbEntry;

      Entry = ElemSortTable[i];
      NbEntry = NbSortTable[i];

      if (Entry->nodes != NbEntry->nodes)
      {
        printf("Connect_Sons_of_ElementSide(): ERROR Sorttables[%d]"\
               " eNodes=%d nbNodes=%d\n",i,Entry->nodes,NbEntry->nodes);
        assert(0);
      }
      for (int j = 0; j < Entry->nodes; j++)
        if (Entry->nodeptr[j] != NbEntry->nodeptr[j])
        {
          printf("Connect_Sons_of_ElementSide(): "
                 "ERROR Sorttables[%d][%d]"\
                 " eNodePtr=%p nbNodePtr=%p\n",
                 i,j,Entry->nodeptr[j],NbEntry->nodeptr[j]);
          /*				assert(0);*/
        }
    }
        #endif

  IFDEBUG(gm,4)
  if (!ioflag)
  {
    UserWriteF("After qsort\n");

    /* test whether all entries are corresponding */
    UserWriteF("SORTTABLELIST:\n");
    for (int i = 0; i < Sons_of_Side; i++)
    {
      COMPARE_RECORD *Entry, *NbEntry;

      Entry = ElemSortTable[i];
      NbEntry = NbSortTable[i];

      UserWriteF("EAdr=%x side=%d realNbAdr=%x    NbAdr=%x nbside=%x "
                 "realNbAdr=%x\n",
                 Entry->elem, Entry->side, NBELEM(Entry->elem,Entry->side),
                 NbEntry->elem, NbEntry->side,NBELEM(NbEntry->elem,NbEntry->side));
    }

    for (int i = 0; i < Sons_of_Side; i++)
    {
      COMPARE_RECORD *Entry, *NbEntry;

      Entry = ElemSortTable[i];
      NbEntry = NbSortTable[i];

      if (NBELEM(Entry->elem,Entry->side)!=NbEntry->elem)
      {
        UserWriteF("NOTEQUAL for i=%d elem=%x: elemrealnb=%x "
                   "elemsortnb=%x\n",
                   i,Entry->elem,NBELEM(Entry->elem,Entry->side),NbEntry->elem);
        REFINE_ELEMENT_LIST(0,theElement,"theElement:");
        REFINE_ELEMENT_LIST(0,theNeighbor,"theNeighbor:");
      }
      if (NBELEM(NbEntry->elem,NbEntry->side)!=Entry->elem)
      {
        UserWriteF("NOTEQUAL for i=%d nb=%x: nbrealnb=%x nbsortnb=%x\n",
                   i,NbEntry->elem,NBELEM(NbEntry->elem,NbEntry->side),
                   Entry->elem);
        REFINE_ELEMENT_LIST(0,theElement,"theE:");
        REFINE_ELEMENT_LIST(0,theNeighbor,"theN:");
      }
    }
    UserWriteF("\n\n");
  }
  ENDDEBUG

  /* set neighborship relations */
  if (ioflag)
  {
    for (int i = 0; i < Sons_of_Side; i++)
    {
      const COMPARE_RECORD *Entry = ElemSortTable[i];
      for (k=0; k<Sons_of_NbSide; k++)
      {
        const COMPARE_RECORD *NbEntry = NbSortTable[k];

        if (Entry->nodes != NbEntry->nodes) continue;

        int idx;
        for (idx = 0; idx < Entry->nodes; idx++)
          if (Entry->nodeptr[idx] != NbEntry->nodeptr[idx])
            break;
        if (idx == Entry->nodes)
        {
          SET_NBELEM(ElemSortTable[i]->elem,ElemSortTable[i]->side,
                     NbSortTable[k]->elem);
          SET_NBELEM(NbSortTable[k]->elem,NbSortTable[k]->side,
                     ElemSortTable[i]->elem);
        }
      }
    }
  }
  else
    /* all entries need to match exactly */
    for (int i = 0; i < Sons_of_Side; i++)
    {
      SET_NBELEM(ElemSortTable[i]->elem,ElemSortTable[i]->side,
                 NbSortTable[i]->elem);
      SET_NBELEM(NbSortTable[i]->elem,NbSortTable[i]->side,
                 ElemSortTable[i]->elem);
#ifdef UG_DIM_3
      if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
        if (DisposeDoubledSideVector(theGrid,ElemSortTable[i]->elem,
                                     ElemSortTable[i]->side,
                                     NbSortTable[i]->elem,
                                     NbSortTable[i]->side))
          RETURN(GM_FATAL);
#endif
    }

  return(GM_OK);
}

/****************************************************************************/
/** \brief  Copy an element

   \param theGrid - grid level of sons of theElement
   \param theElement - element to refine
   \param theContext - nodes needed for new elements

   This function copies an element, (i) corner nodes are already allocated, (iv) create son and set references to sons

   \return <ul>
   <li> 0 - ok </li>
   <li> 1 - fatal memory error </li>
   </ul>
 */
/****************************************************************************/
static INT RefineElementYellow (GRID *theGrid, ELEMENT *theElement, NODE **theContext)
{
  INT i;
  bool boundaryelement = false;
  const int me = theGrid->ppifContext().me();

  /* check for boundary */
  if (OBJT(theElement) == BEOBJ)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      /* at the boundary */
      if (SIDE_ON_BND(theElement,i))
      {
        boundaryelement = true;
        break;
      }
    }

        #ifdef Debug
  /* check son nodes validity */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    assert (theContext[i] != NULL);
  }
        #endif

  /* create son */
  const INT theSonType = boundaryelement ? BEOBJ : IEOBJ;
  ELEMENT* theSon = CreateElement(theGrid,TAG(theElement), theSonType,
                                  theContext,theElement,1);
  if (theSon==NULL) RETURN(GM_ERROR);
  SETECLASS(theSon,MARKCLASS(theElement));

  /* connect son */
  IFDEBUG(gm,2)
  UserWriteF(PFMT "CONNECTING elem=" EID_FMTX "\n",me,EID_PRTX(theElement));
  ENDDEBUG
  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    INT j,Sons_of_Side;
    ELEMENT *Sons_of_Side_List[MAX_SONS];
    INT SonSides[MAX_SIDE_NODES];

    IFDEBUG(gm,2)
    UserWriteF(PFMT "  CONNECT side=%i of elem=" EID_FMTX "\n",me,i,EID_PRTX(theElement));
    ENDDEBUG

    for (j=0; j<MAX_SONS; j++)
      Sons_of_Side_List[j] = NULL;

    Sons_of_Side = 1;
    Sons_of_Side_List[0] = theSon;
    SonSides[0] = i;

    if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                    Sons_of_Side_List,SonSides,0)!=GM_OK) RETURN(GM_FATAL);

                #ifdef ModelP
    if (Identify_Objects_of_ElementSide(theGrid,theElement,i)) RETURN(GM_FATAL);
                #endif
  }

  return(GM_OK);
}


/****************************************************************************/
/** \brief Refine an element without context

   \param theGrid - grid level of sons of theElement
   \param theElement - element to refine
   \param theContext - nodes needed for new elements

   This function refines an element without context,
   (i) corner and midnodes are already allocated,
   (ii) edges between corner and midnodes are ok,
   (iii) create interior nodes and edges,
   (iv) create sons and set references to sons.

   \return <ul>
   INT
   .n   0 - ok
   .n   1 - fatal memory error
 */
/****************************************************************************/
static int RefineElementGreen (GRID *theGrid, ELEMENT *theElement, NODE **theContext)
{
  struct greensondata
  {
    /** \brief Element type */
    short tag;
    /** \brief Boundary element: yes (=1) or no (=0) */
    short bdy;
    std::array<NODE*, MAX_CORNERS_OF_ELEM> corners;
    int nb[MAX_SIDES_OF_ELEM];
    ELEMENT         *theSon;
  };
  typedef struct greensondata GREENSONDATA;

  std::array<GREENSONDATA, MAX_GREEN_SONS> sons;

  int j, k, l;
  int node0;
  bool bdy;

  IFDEBUG(gm,1)
  UserWriteF("RefineElementGreen(): ELEMENT ID=%d\n",ID(theElement));
  ENDDEBUG

  /* init son data array */
  for (int i = 0; i < MAX_GREEN_SONS; i++)
  {
    sons[i].tag = -1;
    sons[i].bdy = -1;
    for (j=0; j<MAX_CORNERS_OF_ELEM; j++) sons[i].corners[j] = NULL;
    for (j=0; j<MAX_SIDES_OF_ELEM; j++) sons[i].nb[j] = -1;
    sons[i].theSon = NULL;
  }

  IFDEBUG(gm,2)
  UserWriteF("         Element ID=%d actual CONTEXT is:\n",ID(theElement));
  for (int i = 0; i < MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    UserWriteF(" %3d",i);
  UserWrite("\n");
  for (int i = 0; i < MAX_CORNERS_OF_ELEM + MAX_NEW_CORNERS_DIM; i++)
    if (theContext[i] != NULL)
      UserWriteF(" %3d",ID(theContext[i]));
    else
      UserWriteF("    ");
  UserWrite("\n");
  ENDDEBUG
  int corner;
  IFDEBUG(gm,3)
  for (corner = 0; corner < MAX_CORNERS_OF_ELEM + MAX_NEW_CORNERS_DIM; corner++)
  {
    if ((theContext[corner] != nullptr) && (NDOBJ != OBJT(theContext[corner])))
      UserWriteF(" ERROR NO NDOBJ(5) OBJT(corner=%d)=%d ID=%d adr=%x\n",\
                 corner,OBJT(theContext[corner]),ID(theContext[corner]),theContext[corner]);
  }
  ENDDEBUG

  /* init indices for son elements */
  /* outer side for tetrahedra is side 0 */
  const int tetNode0 = CORNER_OF_SIDE_TAG(TETRAHEDRON, 0, 0);
  const int tetNode1 = CORNER_OF_SIDE_TAG(TETRAHEDRON, 0, 1);
  const int tetNode2 = CORNER_OF_SIDE_TAG(TETRAHEDRON, 0, 2);

  const int tetEdge0 = EDGE_OF_SIDE_TAG(TETRAHEDRON, 0, 0);
  const int tetEdge1 = EDGE_OF_SIDE_TAG(TETRAHEDRON, 0, 1);
  const int tetEdge2 = EDGE_OF_SIDE_TAG(TETRAHEDRON, 0, 2);

  int tetSideNode0Node1 = SIDE_WITH_EDGE_TAG(TETRAHEDRON, tetEdge0, 0);
  if (tetSideNode0Node1 == 0)
    tetSideNode0Node1 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge0,1);

  int tetSideNode1Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON, tetEdge1, 0);
  if (tetSideNode1Node2 == 0)
    tetSideNode1Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge1,1);

  int tetSideNode0Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON, tetEdge2, 0);
  if (tetSideNode0Node2 == 0)
    tetSideNode0Node2 = SIDE_WITH_EDGE_TAG(TETRAHEDRON,tetEdge2,1);

  /* outer side for pyramid has 4 corners */
  for (corner = 0; corner < SIDES_OF_TAG(PYRAMID); corner++)
    if (CORNERS_OF_SIDE_TAG(PYRAMID, corner) == 4)
      break;
  const int pyrNode0 = CORNER_OF_SIDE_TAG(PYRAMID, corner, 0);
  const int pyrNode1 = CORNER_OF_SIDE_TAG(PYRAMID, corner, 1);
  const int pyrNode2 = CORNER_OF_SIDE_TAG(PYRAMID, corner, 2);
  const int pyrNode3 = CORNER_OF_SIDE_TAG(PYRAMID, corner, 3);

  const int pyrEdge0 = EDGE_OF_SIDE_TAG(PYRAMID, corner, 0);
  const int pyrEdge1 = EDGE_OF_SIDE_TAG(PYRAMID, corner, 1);
  const int pyrEdge2 = EDGE_OF_SIDE_TAG(PYRAMID, corner, 2);
  const int pyrEdge3 = EDGE_OF_SIDE_TAG(PYRAMID, corner, 3);

  int pyrSideNode0Node1 = SIDE_WITH_EDGE_TAG(PYRAMID, pyrEdge0, 1);
  if (pyrSideNode0Node1 == corner)
    pyrSideNode0Node1 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge0,0);

  int pyrSideNode1Node2 = SIDE_WITH_EDGE_TAG(PYRAMID, pyrEdge1, 1);
  if (pyrSideNode1Node2 == corner)
    pyrSideNode1Node2 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge1,0);

  int pyrSideNode2Node3 = SIDE_WITH_EDGE_TAG(PYRAMID, pyrEdge2, 1);
  if (pyrSideNode2Node3 == corner)
    pyrSideNode2Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge2,0);

  int pyrSideNode0Node3 = SIDE_WITH_EDGE_TAG(PYRAMID, pyrEdge3, 1);
  if (pyrSideNode0Node3 == corner)
    pyrSideNode0Node3 = SIDE_WITH_EDGE_TAG(PYRAMID,pyrEdge3,0);

  /* create edges on inner of sides, create son elements and connect them */
  std::array<int, 4> sides;
  std::array<NODE*, 8> theSideNodes;

  for (int i = 0; i < SIDES_OF_ELEM(theElement); i++)
  {
    NODE *theNode = theContext[CORNERS_OF_ELEM(theElement)+
                         EDGES_OF_ELEM(theElement)+i];
    const int nedges = EDGES_OF_SIDE(theElement,i);

    bdy = (OBJT(theElement) == BEOBJ && SIDE_ON_BND(theElement,i));

    int nelem = 5*i;                   /* A face in 3d gets subdivided into at most 5 (yes, 5) parts */
    for (j=nelem; j<(nelem+5); j++)
      sons[j].bdy = bdy;

    k = 0;
    for (j=0; j<EDGES_OF_SIDE(theElement,i); j++)
    {
      const int edge = EDGE_OF_SIDE(theElement,i,j);
      for (l=0; l<MAX_SIDES_OF_ELEM; l++)
        if (SIDE_WITH_EDGE(theElement,edge,l) != i)
        {
          sides[k++] = SIDE_WITH_EDGE(theElement,edge,l)+
                       MAX_GREEN_SONS;
          break;
        }
      ASSERT(l<2);
    }

    k = 0;
    for (j=0; j<nedges; j++)
    {
      theSideNodes[2*j] = theContext[CORNER_OF_SIDE(theElement,i,j)];
      theSideNodes[2*j+1] = theContext[CORNERS_OF_ELEM(theElement)+
                                       EDGE_OF_SIDE(theElement,i,j)];
      if (theSideNodes[2*j+1] != NULL) k++;
    }

    IFDEBUG(gm,2)
    UserWriteF("    SIDE %d has %d nodes and sidenode=%x\n",i,k,theNode);
    ENDDEBUG

    switch (CORNERS_OF_SIDE(theElement,i))
    {

    case 4 :

      /* theNode points to a potential new side node */
      if (theNode == NULL)
      {
        switch (k)                                   /* The number of nodes on the edges of this side */
        {
        case 0 :
          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[0];
          sons[nelem].corners[pyrNode1] = theSideNodes[2];
          sons[nelem].corners[pyrNode2] = theSideNodes[4];
          sons[nelem].corners[pyrNode3] = theSideNodes[6];

          sons[nelem].nb[pyrSideNode0Node1] = sides[0];
          sons[nelem].nb[pyrSideNode1Node2] = sides[1];
          sons[nelem].nb[pyrSideNode2Node3] = sides[2];
          sons[nelem].nb[pyrSideNode0Node3] = sides[3];

          break;

        case 1 :
          for (j=0; j<nedges; j++)
          {
            node0 = 2*j+1;
            if (theSideNodes[node0] != NULL)
            {

              /* define the son corners and inner side relations */
              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = sides[j];
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = nelem+2;
              nelem++;

              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+7)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = nelem+1;
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = sides[j];
              nelem++;

              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = nelem-2;
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = nelem-1;

              break;
            }
          }
          break;

        case 2 :
          /* two cases: sidenodes are not on neighboring edges OR are on neighboring edges */
          for (j=0; j<nedges; j++)
          {
            node0 = 2*j+1;
            if (theSideNodes[node0] != NULL)
              break;
          }
          if (theSideNodes[(node0+6)%(2*nedges)] != NULL)
          {
            node0 = (node0+6)%(2*nedges);
            j = (j+3)%nedges;
          }
          if (theSideNodes[(node0+4)%(2*nedges)] == NULL)
          {

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+3;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+7)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem+2;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[(j)%nedges];
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+1;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+2)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-3;
            sons[nelem].nb[tetSideNode1Node2] = nelem-1;
            sons[nelem].nb[tetSideNode0Node2] = nelem-2;
          }
          else
          {
            sons[nelem].tag = PYRAMID;
            sons[nelem].corners[pyrNode0] = theSideNodes[node0];
            sons[nelem].corners[pyrNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[pyrNode2] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[pyrNode3] = theSideNodes[(node0+4)%(2*nedges)];

            sons[nelem].nb[pyrSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[pyrSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[pyrSideNode2Node3] = sides[(j+2)%nedges];
            sons[nelem].nb[pyrSideNode0Node3] = nelem+1;
            nelem++;

            sons[nelem].tag = PYRAMID;
            sons[nelem].corners[pyrNode0] = theSideNodes[(node0+4)%(2*nedges)];
            sons[nelem].corners[pyrNode1] = theSideNodes[(node0+5)%(2*nedges)];
            sons[nelem].corners[pyrNode2] = theSideNodes[(node0+7)%(2*nedges)];
            sons[nelem].corners[pyrNode3] = theSideNodes[(node0+8)%(2*nedges)];

            sons[nelem].nb[pyrSideNode0Node1] = sides[(j+2)%nedges];
            sons[nelem].nb[pyrSideNode1Node2] = sides[(j+3)%nedges];
            sons[nelem].nb[pyrSideNode2Node3] = sides[(j)%nedges];
            sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
          }
          break;

        case 3 :
          for (j=0; j<nedges; j++)
          {
            node0 = 2*j+1;
            if (theSideNodes[node0] == NULL)
              break;
          }

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[pyrNode1] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[pyrNode2] = theSideNodes[(node0+6)%(2*nedges)];
          sons[nelem].corners[pyrNode3] = theSideNodes[(node0+7)%(2*nedges)];

          sons[nelem].nb[pyrSideNode0Node1] = sides[(j+1)%nedges];
          sons[nelem].nb[pyrSideNode1Node2] = nelem+3;
          sons[nelem].nb[pyrSideNode2Node3] = sides[(j+3)%nedges];
          sons[nelem].nb[pyrSideNode0Node3] = sides[(j)%nedges];
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+2;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+5)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+6)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+3)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+1;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+6)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-2;
          sons[nelem].nb[tetSideNode1Node2] = nelem-1;
          sons[nelem].nb[tetSideNode0Node2] = nelem-3;

          break;

        case 4 :
          for (j=0; j<nedges; j++)
          {
            node0 = 2*j+1;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[(j)%nedges];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+(nedges-j);
            nelem++;
          }

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[1];
          sons[nelem].corners[pyrNode1] = theSideNodes[3];
          sons[nelem].corners[pyrNode2] = theSideNodes[5];
          sons[nelem].corners[pyrNode3] = theSideNodes[7];

          sons[nelem].nb[pyrSideNode0Node1] = nelem-4;
          sons[nelem].nb[pyrSideNode1Node2] = nelem-3;
          sons[nelem].nb[pyrSideNode2Node3] = nelem-2;
          sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
          break;

        default :
          RETURN(GM_FATAL);
        }
      }
      else                              /* theNode != NULL */
      {
        /* create the four side edges */
        for (j=0; j<nedges; j++)
        {
          node0 = 2*j+1;
          if (theSideNodes[node0] == NULL) break;

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[node0%(2*nedges)];
          sons[nelem].corners[pyrNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[pyrNode2] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[pyrNode3] = theNode;

          sons[nelem].nb[pyrSideNode0Node1] = sides[(j)%nedges];
          sons[nelem].nb[pyrSideNode1Node2] = sides[(j+1)%nedges];
          if (j == 3)
            sons[nelem].nb[pyrSideNode2Node3] = nelem-3;
          else
            sons[nelem].nb[pyrSideNode2Node3] = nelem+1;
          if (j == 0)
            sons[nelem].nb[pyrSideNode0Node3] = nelem+3;
          else
            sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
          nelem++;

        }
        ASSERT(j==4);
      }

      break;

    case 3 :
      if (theNode == NULL)
      {
        switch (k)
        {
        case 0 :
          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[0];
          sons[nelem].corners[tetNode1] = theSideNodes[2];
          sons[nelem].corners[tetNode2] = theSideNodes[4];

          sons[nelem].nb[tetSideNode0Node1] = sides[0];
          sons[nelem].nb[tetSideNode1Node2] = sides[1];
          sons[nelem].nb[tetSideNode0Node2] = sides[2];

          break;

        case 1 :
          for (j=0; j<nedges; j++)
          {
            node0 = 2*j+1;
            if (theSideNodes[node0] != NULL)
            {

              /* define the son corners and inner side relations */
              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = sides[j];
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = nelem+1;
              nelem++;

              sons[nelem].tag = TETRAHEDRON;
              sons[nelem].corners[tetNode0] = theSideNodes[node0];
              sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
              sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

              sons[nelem].nb[tetSideNode0Node1] = nelem-1;
              sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
              sons[nelem].nb[tetSideNode0Node2] = sides[j];

              break;
            }
          }
          break;

        case 2 :
        {
          INT node;
          /*INT maxedge=-1;*/
                                                        #ifdef ModelP
          unsigned int maxid = 0;
                                                        #else
          INT maxid = -1;
                                                        #endif

          node0 = -1;
          for (k=0; k<nedges; k++)
          {
            node = (2*k+3)%(2*nedges);
            if (theSideNodes[node] == NULL)
            {
              node0 = 2*k+1;
              j = k;
            }
            /*
               if (EDGE_OF_SIDE(theElement,i,k)>maxedge)
                    maxedge = 2*k+1;
             */
            /* neighboring elements need to refine in the same way      */
            /* in parallel case all copies of the elements also         */
            if (theSideNodes[2*k+1]!=NULL && _ID_(theSideNodes[2*k+1])>maxid)
              maxid = _ID_(theSideNodes[2*k+1]);
          }
#if !defined ModelP  // For an unknown reason the maxid variable is unsigned when ModelP is set, and signed otherwise.
          assert(maxid != -1);
#endif
          assert(node0 != -1);

          /* if (node0 == maxedge && ((SIDEPATTERN(theElement)&(1<<i)) == 0)) */
          if (_ID_(theSideNodes[node0]) == maxid)
          {

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[j];
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem+2;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem+1;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[j];
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-2;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = nelem-1;
          }
          else
          {
            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = sides[j];
            sons[nelem].nb[tetSideNode1Node2] = nelem+1;
            sons[nelem].nb[tetSideNode0Node2] = nelem+2;
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[(node0+4)%(2*nedges)];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+3)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-1;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[(j+2)%nedges];
            nelem++;

            sons[nelem].tag = TETRAHEDRON;
            sons[nelem].corners[tetNode0] = theSideNodes[node0];
            sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
            sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

            sons[nelem].nb[tetSideNode0Node1] = nelem-2;
            sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
            sons[nelem].nb[tetSideNode0Node2] = sides[j];
          }

          break;
        }
        case 3 :
          j = 0;
          node0 = 1;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+2)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[j];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+3;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+4)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+5)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem+2;
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = sides[j];
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0+2];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+3)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = sides[(j+1)%nedges];
          sons[nelem].nb[tetSideNode1Node2] = sides[(j+2)%nedges];
          sons[nelem].nb[tetSideNode0Node2] = nelem+1;
          nelem++;

          sons[nelem].tag = TETRAHEDRON;
          sons[nelem].corners[tetNode0] = theSideNodes[node0];
          sons[nelem].corners[tetNode1] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[tetNode2] = theSideNodes[(node0+4)%(2*nedges)];

          sons[nelem].nb[tetSideNode0Node1] = nelem-3;
          sons[nelem].nb[tetSideNode1Node2] = nelem-1;
          sons[nelem].nb[tetSideNode0Node2] = nelem-2;

          break;

        default :
          assert(0);
          break;
        }
      }

      else                               /* theNode != NULL */
      {
        /* create the four side edges */
        for (j=0; j<nedges; j++)
        {
          node0 = 2*j+1;
          if (theSideNodes[node0] == NULL) break;

          sons[nelem].tag = PYRAMID;
          sons[nelem].corners[pyrNode0] = theSideNodes[node0%(2*nedges)];
          sons[nelem].corners[pyrNode1] = theSideNodes[(node0+1)%(2*nedges)];
          sons[nelem].corners[pyrNode2] = theSideNodes[(node0+2)%(2*nedges)];
          sons[nelem].corners[pyrNode3] = theNode;

          sons[nelem].nb[pyrSideNode0Node1] = sides[(j)%nedges];
          sons[nelem].nb[pyrSideNode1Node2] = sides[(j+1)%nedges];
          if (j == 2)
            sons[nelem].nb[pyrSideNode2Node3] = nelem-2;
          else
            sons[nelem].nb[pyrSideNode2Node3] = nelem+1;
          if (j == 0)
            sons[nelem].nb[pyrSideNode0Node3] = nelem+2;
          else
            sons[nelem].nb[pyrSideNode0Node3] = nelem-1;
        }
        ASSERT(j==nedges);
      }

      break;

    default :                      /* Side with neither 3 nor 4 vertices found */
      assert(0);
      break;
    }
  }

  /* connect elements over edges */
  for (int i = 0; i < EDGES_OF_ELEM(theElement); i++)
  {
    const int side0 = SIDE_WITH_EDGE(theElement, i, 0);
    const int side1 = SIDE_WITH_EDGE(theElement, i, 1);

    if (theContext[i+CORNERS_OF_ELEM(theElement)] == NULL)              /* No new node in the middle of this edge */
    {
      /* two elements share this edge */

      /* get son elements for this edge */
      bool found = false;
      for (j=side0*5; j<(side0*5+5); j++)
      {
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[j].nb[k]-MAX_GREEN_SONS)==side1)
          {
            found = true;
            break;
          }
        if (found) break;
      }
      ASSERT(j<side0*5+5);

      found = false;
      int m;
      for (l=side1*5; l<side1*5+5; l++)
      {
        for (m=0; m<MAX_SIDES_OF_ELEM; m++)
          if ((sons[l].nb[m]-MAX_GREEN_SONS)==side0)
          {
            found = true;
            break;
          }
        if (found) break;
      }
      ASSERT(l<side1*5+5);

      sons[j].nb[k] = l;
      sons[l].nb[m] = j;
    }
    else
    {
      /* four elements share this edge */

      /* get son elements for this edge */
      std::array<int, 5> elementsSide0;
      l = 0;
      for (j=side0*5; j<(side0*5+5); j++)
      {
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[j].nb[k]-MAX_GREEN_SONS)==side1)
            elementsSide0[l++] = j;
      }
      ASSERT(l==2);

      std::array<int, 5> elementsSide1;
      l = 0;
      for (j=side1*5; j<(side1*5+5); j++)
      {
        for (int m = 0; m < MAX_SIDES_OF_ELEM; m++)
          if ((sons[j].nb[m]-MAX_GREEN_SONS)==side0)
            elementsSide1[l++] = j;
      }
      ASSERT(l==2);

      /* determine neighboring elements */
      for (j=0; j<CORNERS_OF_EDGE; j++)
      {
        const NODE *theNode1 = theContext[CORNER_OF_EDGE(theElement,i,j)];
        bool found = false;
        for (l=0; l<2; l++)
        {
          for (k=0; k<MAX_CORNERS_OF_ELEM; k++)
          {
            if (theNode1 == sons[elementsSide0[l]].corners[k])
            {
              found = true;
              break;
            }
          }
          if (found) break;
        }
        ASSERT(k<MAX_CORNERS_OF_ELEM);
        ASSERT(l<2);

        found = false;
        int m;
        for (m=0; m<2; m++)
        {
          for (k=0; k<MAX_CORNERS_OF_ELEM; k++)
          {
            if (theNode1 == sons[elementsSide1[m]].corners[k])
            {
              found = true;
              break;
            }
          }
          if (found) break;
        }
        ASSERT(k<MAX_CORNERS_OF_ELEM);
        ASSERT(m<2);

        /* init neighbor field */
        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[elementsSide0[l]].nb[k]-MAX_GREEN_SONS)==side1)
            break;
        ASSERT(k<MAX_SIDES_OF_ELEM);
        sons[elementsSide0[l]].nb[k] = elementsSide1[m];

        for (k=0; k<MAX_SIDES_OF_ELEM; k++)
          if ((sons[elementsSide1[m]].nb[k]-MAX_GREEN_SONS)==side0)
            break;
        ASSERT(k<MAX_SIDES_OF_ELEM);
        sons[elementsSide1[m]].nb[k] = elementsSide0[l];
      }
    }
  }

  /* create son elements */
  IFDEBUG(gm,1)
  UserWriteF("    Creating SON elements for element ID=%d:\n",ID(theElement));
  ENDDEBUG
  int n = 0;
  for (int i = 0; i < MAX_GREEN_SONS; i++)
  {
    if (sons[i].tag >= 0)
    {

      IFDEBUG(gm,2)
      if (i%5 == 0)
        UserWriteF("     SIDE %d:\n",i/5);
      ENDDEBUG

      NODE *ElementNodes[MAX_CORNERS_OF_ELEM];
        l = 0;
      for (j=0; j<CORNERS_OF_TAG(sons[i].tag); j++)
      {
        if (sons[i].corners[j] == NULL)
        {
          sons[i].corners[j] = theContext[CORNERS_OF_ELEM(theElement)+
                                          CENTER_NODE_INDEX(theElement)];
          l++;
        }
        ElementNodes[j] = sons[i].corners[j];
      }
      ASSERT(l == 1);

      if (sons[i].bdy == 1)
        sons[i].theSon = CreateElement(theGrid,sons[i].tag,BEOBJ,
                                       ElementNodes,theElement,1);
      else
        sons[i].theSon = CreateElement(theGrid,sons[i].tag,IEOBJ,
                                       ElementNodes,theElement,1);
      if (sons[i].theSon==NULL) RETURN(GM_FATAL);

      IFDEBUG(gm,0)
      for (j=0; j<CORNERS_OF_ELEM(sons[i].theSon); j++)
      {
        for (int m = 0; m < CORNERS_OF_ELEM(sons[i].theSon); m++)
        {
          if (sons[i].corners[j] == NULL || sons[i].corners[m] == NULL)
          {
            if ((m!=j) && (sons[i].corners[j] == sons[i].corners[m]))
            {
              UserWriteF("     ERROR: son %d has equivalent corners "
                         "%d=%d adr=%x adr=%x\n",
                         n,j,m,sons[i].corners[j],sons[i].corners[m]);
            }
          }
          else if ((m != j)
                   && (sons[i].corners[j] == sons[i].corners[m] || (_ID_(sons[i].corners[j]) == _ID_(sons[i].corners[m]))))
          {
            UserWriteF("     ERROR: son %d has equivalent corners "
                       "%d=%d  ID=%d ID=%d adr=%x adr=%x\n",
                       n,j,m,_ID_(sons[i].corners[j]),_ID_(sons[i].corners[m]),
                       sons[i].corners[j],sons[i].corners[m]);
          }
        }
      }
      ENDDEBUG

      IFDEBUG(gm,2)
      UserWriteF("      SONS[i=%d] ID=%d: CORNERS ",i,ID(sons[i].theSon));
      for (j=0; j<CORNERS_OF_ELEM(sons[i].theSon); j++)
      {
        if (sons[i].corners[j] != NULL)
          UserWriteF(" %d",_ID_(sons[i].corners[j]));
      }
      UserWriteF("\n");
      ENDDEBUG

                        #ifdef __ANISOTROPIC__
      if (MARK(theElement) != COPY)
        SETECLASS(sons[i].theSon,RED_CLASS);
      else
        SETECLASS(sons[i].theSon,GREEN_CLASS);
                        #else
      SETECLASS(sons[i].theSon,GREEN_CLASS);
                        #endif
      if (i == 0) SET_SON(theElement,0,sons[i].theSon);
      for (int s = 0; s < SIDES_OF_ELEM(sons[i].theSon); s++)
        SET_NBELEM(sons[i].theSon,s,NULL);

      n++;
    }
  }
  IFDEBUG(gm,1)
  UserWriteF("    n=%d sons created NSONS=%d\n",n,NSONS(theElement));
  ENDDEBUG

  /* translate neighbor information */
  for (int i = 0; i < MAX_GREEN_SONS; i++)
  {
    if (sons[i].tag >= 0)              /* valid son entry */
    {
      l = 0;
      IFDEBUG(gm,0)
      for (j=0; j<SIDES_OF_ELEM(sons[i].theSon); j++)
      {
        for (int m = 0; m < SIDES_OF_ELEM(sons[i].theSon); m++)
        {
          if (sons[i].nb[j] == sons[i].nb[m] && (m!=j))
            UserWriteF("     ERROR: son %d has equivalent "
                       "neighbors %d=%d  NB=%d\n",n,j,m,sons[i].nb[m]);
        }
      }
      ENDDEBUG
      for (j=0; j<SIDES_OF_ELEM(sons[i].theSon); j++)
      {
        if (sons[i].nb[j] != -1)
          SET_NBELEM(sons[i].theSon,j,sons[sons[i].nb[j]].theSon);
        else
          l++;
      }
      /* l counts the number of element sides without a neighboring element.
       * Since all elements are pyramids/tetrahedra with exactly one vertex in the interior,
       * this value must be one. */
      ASSERT(l == 1);
    }
  }

#ifdef UG_DIM_3
  /* If there are side vectors for the elements, then the CreateElement methods called above have
   * allocated one side vector for each new element face.  Therefore, for each face shared by
   * two elements there are now two side vectors, even though there should be only one (shared).
   * The following loops gets rid of the second redundant side vector.  The redundant side
   * vectors shared with elements in the rest of the grid are treated in the method Connect_Sons_of_ElementSide,
   * called further below.
   */
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    for (int i = 0; i < MAX_GREEN_SONS; i++)
    {
      if (sons[i].tag < 0)        /* empty son entry */
        continue;

      for (j=0; j<SIDES_OF_ELEM(sons[i].theSon); j++)
      {
        if (sons[i].nb[j] != -1)        /* We have neighbor on the j-th face */
        {
          if (sons[i].nb[j] <= i)        /* Visit every element pair only once */
            continue;

          ELEMENT* theNeighbor = sons[sons[i].nb[j]].theSon;
          /* what neighbor are we for the neighbor? */
          for (l=0; l<SIDES_OF_ELEM(theNeighbor); l++)
            if (sons[sons[i].nb[j]].nb[l] == i)
              break;
          ASSERT(l < SIDES_OF_ELEM(theNeighbor));

          DisposeDoubledSideVector(theGrid, sons[i].theSon, j, theNeighbor, l);
        }
      }
    }
  }
#endif

  /* connect sons over outer sides */
  for (int i = 0; i < SIDES_OF_ELEM(theElement); i++)
  {
    INT Sons_of_Side;
    ELEMENT *Sons_of_Side_List[MAX_SONS];
    INT SonSides[MAX_SIDE_NODES];

    Sons_of_Side = 0;

    for (j=0; j<MAX_SONS; j++)
      Sons_of_Side_List[j] = NULL;

    for (j=0; j<5; j++)
    {
      if (sons[i*5+j].tag < 0) break;
      Sons_of_Side_List[j] = sons[i*5+j].theSon;
      Sons_of_Side++;
      SonSides[j] = 0;
      if (sons[i*5+j].tag == PYRAMID)
      {
        for (k=0; k<SIDES_OF_TAG(PYRAMID); k++)
          if (CORNERS_OF_SIDE_TAG(PYRAMID,k) == 4)
            break;
        SonSides[j] = k;
      }
    }
    assert(Sons_of_Side>0 && Sons_of_Side<6);

    if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                    Sons_of_Side_List,SonSides,0)!=GM_OK) RETURN(GM_FATAL);

                #ifdef ModelP
    if (Identify_Objects_of_ElementSide(theGrid,theElement,i)) RETURN(GM_FATAL);
                #endif
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   RefineElementRed - refine an element in the given context

   SYNOPSIS:
   static int RefineElementRed (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext);

   PARAMETERS:
   \param theGrid - grid level of sons of theElement
   \param theElement - element to refine
   \param theContext - current context of element

   DESCRIPTION:
   This function refines an element in the given context,
   (i) corner and midnodes are already allocated,
   (ii) edges between corner and midnodes are ok,
   (iii) create interior nodes and edges,
   (iv) create sons and set references to sons.

   \return <ul>
   INT
   .n   GM_OK - ok
   .n   GM_FATAL - fatal memory error
 */
/****************************************************************************/
static int RefineElementRed (GRID *theGrid, ELEMENT *theElement, NODE **theElementContext)
{
  const int me = theGrid->ppifContext().me();

  /* is something to do ? */
  if (!MARKED(theElement)) return(GM_OK);

  std::array<ELEMENT*, MAX_SONS> SonList = {};

  REFRULE const* rule = MARK2RULEADR(theElement,MARK(theElement));

  /* create elements */
  for (INT s=0; s<NSONS_OF_RULE(rule); s++)
  {
    bool boundaryelement = false;
    /** \todo how can boundary detection be generalized */
    if (OBJT(theElement) == BEOBJ)
    {
      for (INT i=0; i<SIDES_OF_TAG(SON_TAG_OF_RULE(rule,s)); i++)
      {
        const INT side = SON_NB_OF_RULE(rule,s,i);
        /* exterior side */
        if (side >= FATHER_SIDE_OFFSET)
        {
          /* at the boundary */
          if (SIDE_ON_BND(theElement,side-FATHER_SIDE_OFFSET))
          {
            boundaryelement = true;
            break;
          }
        }
      }
    }

    NODE* ElementNodes[MAX_CORNERS_OF_ELEM];
    for (INT i=0; i<CORNERS_OF_TAG(SON_TAG_OF_RULE(rule,s)); i++)
    {
      ASSERT(theElementContext[SON_CORNER_OF_RULE(rule,s,i)]!=NULL);
      ElementNodes[i] = theElementContext[SON_CORNER_OF_RULE(rule,s,i)];
    }

    const INT theSonType = boundaryelement ? BEOBJ : IEOBJ;
    ELEMENT* theSon = CreateElement(theGrid,SON_TAG_OF_RULE(rule,s),theSonType,
                                    ElementNodes,theElement,1);
    if (theSon==NULL) RETURN(GM_ERROR);

    /* fill in son data */
    SonList[s] = theSon;
    SETECLASS(theSon,MARKCLASS(theElement));
  }

  /* connect elements */
  for (INT s=0; s<NSONS_OF_RULE(rule); s++)
  {
    SONDATA const* sdata = SON_OF_RULE(rule,s);
    for (INT i=0; i<SIDES_OF_ELEM(SonList[s]); i++)
    {
      SET_NBELEM(SonList[s],i,NULL);

      const INT side = SON_NB(sdata,i);
      /* an interior face */
      if (side < FATHER_SIDE_OFFSET)
      {
        SET_NBELEM(SonList[s],i,SonList[side]);

        IFDEBUG(gm,3)
        UserWriteF("elid=%3d: side:",ID(SonList[s]));
        for (INT p=0; p<CORNERS_OF_SIDE(SonList[s],i); p++)
          UserWriteF(" %2d",ID(CORNER_OF_SIDE_PTR(SonList[s],i,p)));
        UserWriteF(" INSIDE of father");
        UserWriteF("\nnbid=%3d: side:",ID(SonList[side]));
        {
          int ss,qq,pp,f,pts;
          f=0;
          for (ss=0; ss<SIDES_OF_ELEM(SonList[side]); ss++)
          {
            pts = 0;
            for (pp=0; pp<CORNERS_OF_SIDE(SonList[s],i); pp++)
              for (qq=0; qq<CORNERS_OF_SIDE(SonList[side],ss); qq++)
                if (CORNER_OF_SIDE_PTR(SonList[s],i,pp) ==
                    CORNER_OF_SIDE_PTR(SonList[side],ss,qq))
                {
                  pts |= ((1<<pp) | (16<<qq));
                  break;
                }
            switch (pts)
            {
                                                        #ifdef UG_DIM_2
            case (LINEPOINTS) :
                                                        #endif
                                                        #ifdef UG_DIM_3
            case (TRIPOINTS) :
            case (QUADPOINTS) :
                                                        #endif
              if (pts==TRIPOINTS &&
                  CORNERS_OF_SIDE(SonList[s],i)==4)
              {
                PrintErrorMessage('E',"RefineElement",
                                  "quad side with 3 equal nodes");
                RETURN(GM_FATAL);
              }
              f=1;
              break;
            default :
              break;
            }
            if (f) break;
          }
          ASSERT(f==1);
          for (pp=0; pp<CORNERS_OF_SIDE(SonList[side],ss); pp++)
            UserWriteF(" %2d",ID(CORNER_OF_SIDE_PTR(SonList[side],ss,pp)));
        }
        UserWriteF("\n\n");
        ENDDEBUG

        ASSERT(SonList[side]!=NULL);

        /* dispose doubled side vectors if */
                                #ifdef UG_DIM_3
        if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
        {
          INT l;
          for (l=0; l<SIDES_OF_ELEM(SonList[side]); l++)
            if (NBELEM(SonList[side],l)==SonList[s])
              break;

          if (l<SIDES_OF_ELEM(SonList[side]))
          {
            /* assert consistency of rule set */
            ASSERT(SON_NB_OF_RULE(rule,side,l)==s);
            ASSERT(SON_NB_OF_RULE(rule,s,i)==side);
            ASSERT(NBELEM(SonList[s],i)==SonList[side]
                   && NBELEM(SonList[side],l)==SonList[s]);
            if (DisposeDoubledSideVector(theGrid,SonList[s],i,
                                         SonList[side],l)) RETURN(GM_FATAL);
          }
        }
                #endif
        continue;
      }
    }
  }

  IFDEBUG(gm,2)
  UserWriteF(PFMT "CONNECTING elem=" EID_FMTX "\n",me,EID_PRTX(theElement));
  ENDDEBUG
  for (INT i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    INT Sons_of_Side;
    ELEMENT *Sons_of_Side_List[MAX_SONS];
    INT SonSides[MAX_SIDE_NODES];

    IFDEBUG(gm,2)
    UserWriteF(PFMT "  CONNECT side=%i of elem=" EID_FMTX "\n",me,i,EID_PRTX(theElement));
    ENDDEBUG

    for (INT j=0; j<MAX_SONS; j++)
      Sons_of_Side_List[j] = NULL;

    for (INT j=0; j<NSONS_OF_RULE(rule); j++)
      Sons_of_Side_List[j] = SonList[j];

    if (Get_Sons_of_ElementSide(theElement,i,&Sons_of_Side,
                                Sons_of_Side_List,SonSides,0,0)!=GM_OK) RETURN(GM_FATAL);

    if (Connect_Sons_of_ElementSide(theGrid,theElement,i,Sons_of_Side,
                                    Sons_of_Side_List,SonSides,0)!=GM_OK) RETURN(GM_FATAL);

                #ifdef ModelP
    if (Identify_Objects_of_ElementSide(theGrid,theElement,i)) RETURN(GM_FATAL);
                #endif
  }

  return(GM_OK);
}


/****************************************************************************/
/*
    - refine an element

   SYNOPSIS:
   static INT RefineElement (GRID *UpGrid, ELEMENT *theElement,NODE** theNodeContext);

   PARAMETERS:
   \param UpGrid - grid level to refine
   \param theElement - element to refine
   \param theNodeContext - nodecontext for refinement

   DESCRIPTION:
   This function refines an element
   \return <ul>
   INT
   .n  GM_OK - ok
   .n  GM_FATAL - fatal memory error
 */
/****************************************************************************/

static INT RefineElement (GRID *UpGrid, ELEMENT *theElement,NODE** theNodeContext)
{
  switch (MARKCLASS(theElement))
  {
  case (YELLOW_CLASS) :
    if (RefineElementYellow(UpGrid,theElement,theNodeContext)!=GM_OK)
      RETURN(GM_FATAL);
    break;

  case (GREEN_CLASS) :
                #ifdef __ANISOTROPIC__
  case (RED_CLASS) :
                #endif
    if (MARKED_NEW_GREEN(theElement))
    {
      /* elements with incomplete rules set */
      if (RefineElementGreen(UpGrid,theElement,theNodeContext) != GM_OK)
        RETURN(GM_FATAL);
    }
    else
    {
      /* elements with complete rules set */
      if (RefineElementRed(UpGrid,theElement,theNodeContext)!=GM_OK)
        RETURN(GM_FATAL);
    }
    break;

                #ifndef __ANISOTROPIC__
  case (RED_CLASS) :
    if (RefineElementRed(UpGrid,theElement,theNodeContext)!=GM_OK)
      RETURN(GM_FATAL);
    break;
                #endif

  default :
    RETURN(GM_FATAL);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   AdaptGrid - adapt one level of the multigrid

   SYNOPSIS:
   static int AdaptGrid (GRID *theGrid, INT *nadapted)

   PARAMETERS:
   \param theGrid - grid level to refine
   \param nadapted - number elements have been changed

   DESCRIPTION:
   This function refines one level of the grid

   \return <ul>
   INT
   .n   GM_OK - ok
   .n   GM_FATAL - fatal memory error
 */
/****************************************************************************/

#ifdef ModelP
static int AdaptLocalGrid (GRID *theGrid, INT *nadapted)
#else
static int AdaptGrid (GRID *theGrid, INT *nadapted)
#endif
{
  INT modified = 0;
  ELEMENT *NextElement;
#ifdef ModelP
  const int me = theGrid->ppifContext().me();
  auto& dddContext = theGrid->dddContext();
#endif

  GRID *UpGrid = UPGRID(theGrid);
  if (UpGrid == nullptr)
    RETURN(GM_FATAL);

  REFINE_GRID_LIST(1,MYMG(theGrid),GLEVEL(theGrid),("AdaptGrid(%d):\n",GLEVEL(theGrid)),"");

        #ifdef IDENT_ONLY_NEW
  /* reset ident flags for old objects */
  for (NODE *theNode = PFIRSTNODE(UpGrid); theNode != nullptr; theNode = SUCCN(theNode))
    SETNEW_NIDENT(theNode, 0);

  for (ELEMENT* theElement = PFIRSTELEMENT(UpGrid); theElement != nullptr; theElement = SUCCE(theElement))
  {
    for (INT i = 0; i < EDGES_OF_ELEM(theElement); i++)
    {
      EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement, i, 0),
                              CORNER_OF_EDGE_PTR(theElement, i, 1));
      SETNEW_EDIDENT(theEdge, 0);
    }
  }
        #endif

  /* refine elements                                                  */
  /* ModelP: first loop over master elems, then loop over ghost elems */
  /* this assures that no unnecessary disposures of objects are done  */
  /* which may cause trouble during identification (s.l. 9803020      */
  for (ELEMENT *theElement = FIRSTELEMENT(theGrid); theElement != nullptr;
       theElement=NextElement)
  {
    NextElement = SUCCE(theElement);
                #ifdef ModelP
    /* loop over master elems first, then over ghost elems */
    if (NextElement == NULL) NextElement=PFIRSTELEMENT(theGrid);
    if (NextElement == FIRSTELEMENT(theGrid)) NextElement = NULL;
                #endif

                #ifdef ModelP
    /* reset update overlap flag */
    SETTHEFLAG(theElement,0);
                #endif

    /* do not change PrioVGhost elements */
    if (EVGHOST(theElement)) continue;

    if (REFINEMENT_CHANGES(theElement))
    {
                        #ifdef ModelP
      {
        /* check for valid load balancing */
        const auto& proclist = DDD_InfoProcListRange(dddContext, PARHDRE(theElement), false);
        for (auto&& [proc, prio] : proclist)
        {
          if (prio != PrioMaster && prio != PrioHGhost)
          {
            UserWriteF(PFMT "ERROR invalid load balancing: element="
                       EID_FMTX " has copies of type=%d on proc=%d\n",
                       me, EID_PRTX(theElement), prio, proc);
            REFINE_ELEMENT_LIST(0,theElement,"ERROR element: ");
            assert(0);
          }
        }
      }
                        #endif

      if (hFlag==0 && MARKCLASS(theElement)!=RED_CLASS)
      {
        /* remove copy marks */
        SETMARK(theElement,NO_REFINEMENT);
        SETMARKCLASS(theElement,NO_CLASS);
        /*				continue;  */
      }

      REFINE_ELEMENT_LIST(1,theElement,"REFINING element: ");

      if (UnrefineElement(UpGrid,theElement))
        RETURN(GM_FATAL);

                        #ifdef ModelP
      /* dispose hghost elements with EFATHER==NULL */
      /** \todo how handle this situation?                       */
      /* situation possibly some elements to be coarsened are   */
      /* disconnected from their fathers (970109 s.l.)          */
      if (0)
        if (EHGHOST(theElement) && COARSEN(theElement))
        {
          if (LEVEL(theElement)>0 && EFATHER(theElement)==NULL)
          {
            DisposeElement(theGrid,theElement);
            continue;
          }
        }
                        #endif

      if (EMASTER(theElement))
      {
        ELEMENTCONTEXT theContext;
        if (UpdateContext(UpGrid,theElement,theContext)!=0)
          RETURN(GM_FATAL);

        REFINE_CONTEXT_LIST(2,theContext);

                                #ifdef Debug
        CheckElementContextConsistency(theElement,theContext);
                                #endif

        /* is something to do ? */
        if (MARKED(theElement))
          if (RefineElement(UpGrid,theElement,theContext))
            RETURN(GM_FATAL);
      }

      /* refine and refineclass flag */
      SETREFINE(theElement,MARK(theElement));
      SETREFINECLASS(theElement,MARKCLASS(theElement));
      SETUSED(theElement,0);

                        #ifdef ModelP
      /* set update overlap flag */
      SETTHEFLAG(theElement,1);
                        #endif

      /* this grid is modified */
      modified++;
    }
    else
    {
                        #ifdef ModelP
      /* dispose hghost elements with EFATHER==NULL */
      /** \todo how handle this situation?                       */
      /* situation possibly some elements to be coarsened are   */
      /* disconnected from their fathers (970109 s.l.)          */
      if (0)
        if (EHGHOST(theElement) && COARSEN(theElement))
        {
          if (LEVEL(theElement)>0 && EFATHER(theElement)==NULL)
          {
            DisposeElement(theGrid,theElement);
            continue;
          }
        }
                        #endif

                        #ifdef __ANISOTROPIC__
      if (USED(theElement)==0 && MARKCLASS(theElement)==GREEN_CLASS)
                        #else
      if (USED(theElement)==0)
                        #endif
      {
        /* count not updated green refinements */
        No_Green_Update++;
      }
    }

    /* count green marks */
    if (MARKCLASS(theElement) == GREEN_CLASS) Green_Marks++;

    /* reset coarse flag */
    SETCOARSEN(theElement,0);
  }

  if (UG_GlobalMaxINT(theGrid->ppifContext(), modified))
  {
    /* reset (multi)grid status */
    SETGLOBALGSTATUS(UpGrid);
    RESETMGSTATUS(MYMG(UpGrid));
  }
  REFINE_GRID_LIST(1,MYMG(theGrid),GLEVEL(theGrid),
                   ("END AdaptGrid(%d):\n",GLEVEL(theGrid)),"");

  *nadapted = modified;

  return(GM_OK);
}

#ifdef ModelP
static int AdaptGrid (GRID *theGrid, INT toplevel, INT level, INT newlevel, INT *nadapted)
{
  GRID *FinerGrid = UPGRID(theGrid);

  START_TIMER(gridadapti_timer)

        #ifdef UPDATE_FULLOVERLAP
  DDD_XferBegin(theGrid->dddContext());
  {
    for (ELEMENT *theElement = PFIRSTELEMENT(FinerGrid);
         theElement != nullptr;
         theElement=SUCCE(theElement))
    {
      if (EPRIO(theElement) == PrioHGhost) DisposeElement(FinerGrid,theElement,true);
    }
  }
  DDD_XferEnd(theGrid->dddContext());
        #endif

  DDD_IdentifyBegin(theGrid->dddContext());
  SET_IDENT_MODE(IDENT_ON);
  DDD_XferBegin(theGrid->dddContext());

  DDD_CONSCHECK(theGrid->dddContext());

  /* now really manipulate the next finer level */
  START_TIMER(gridadaptl_timer)

        #ifdef DDDOBJMGR
  DDD_ObjMgrBegin();
        #endif
  if (level<toplevel || newlevel)
    if (AdaptLocalGrid(theGrid,nadapted)!=GM_OK)
      RETURN(GM_FATAL);
        #ifdef DDDOBJMGR
  DDD_ObjMgrEnd();
        #endif

  DDD_XferEnd(theGrid->dddContext());

  SUM_TIMER(gridadaptl_timer)

  DDD_CONSCHECK(theGrid->dddContext());

  {
    int check=1;
    int debugstart=3;
#ifdef Debug
    int gmlevel=Debuggm;
#else
    int gmlevel=0;
#endif


    if (IDENT_IN_STEPS)
    {
      DDD_IdentifyEnd(theGrid->dddContext());
    }

    /* if no grid adaption has occurred adapt next level */
    *nadapted = UG_GlobalSumINT(theGrid->ppifContext(), *nadapted);
    if (*nadapted == 0)
    {
      if (!IDENT_IN_STEPS)
      {
        SET_IDENT_MODE(IDENT_OFF);
        DDD_IdentifyEnd(theGrid->dddContext());
      }

      SUM_TIMER(gridadapti_timer)

      return(GM_OK);
    }

    /* DDD_JoinBegin(); */
    if (IDENT_IN_STEPS)
      DDD_IdentifyBegin(theGrid->dddContext());

    DDD_CONSCHECK(theGrid->dddContext());

    START_TIMER(ident_timer)

    if (Identify_SonObjects(theGrid)) RETURN(GM_FATAL);

    SET_IDENT_MODE(IDENT_OFF);
    DDD_IdentifyEnd(theGrid->dddContext());

    SUM_TIMER(ident_timer)
    /* DDD_JoinEnd(); */


    DDD_CONSCHECK(theGrid->dddContext());

    if (level<toplevel || newlevel)
    {
      START_TIMER(overlap_timer)
      DDD_XferBegin(theGrid->dddContext());
      if (0) /* delete sine this is already done in     */
        /* ConstructConsistentGrid() (s.l. 980522) */
        if (SetGridBorderPriorities(theGrid)) RETURN(GM_FATAL);
      if (UpdateGridOverlap(theGrid)) RETURN(GM_FATAL);


      DDD_XferEnd(theGrid->dddContext());

      DDD_CONSCHECK(theGrid->dddContext());

      DDD_XferBegin(theGrid->dddContext());
      if (ConnectGridOverlap(theGrid)) RETURN(GM_FATAL);
      DDD_XferEnd(theGrid->dddContext());

      DDD_CONSCHECK(theGrid->dddContext());

      /* this is needed due to special cases while coarsening */
      /* sample scene: a ghost element is needed as overlap     */
      /* for two master elements, one of the master elements  */
      /* is coarsened, then the prio of nodes of the ghost    */
      /* element must eventually be downgraded from master    */
      /* to ghost prio (s.l. 971020)                          */
      /* this is done as postprocessing step, since this needs      */
      /* 2 XferBegin/Ends here per modified gridlevel (s.l. 980522) */
      /*
         #ifndef NEW_GRIDCONS_STYLE
         ConstructConsistentGrid(FinerGrid);
         #endif
       */
      SUM_TIMER(overlap_timer)
    }

    DDD_CONSCHECK(theGrid->dddContext());


    CheckConsistency(MYMG(theGrid),(INT)level,(INT)debugstart,(INT)gmlevel,&check);


  }

  if (0) CheckGrid(FinerGrid,1,0,1,1);

  SUM_TIMER(gridadapti_timer)

  return(GM_OK);
}
#endif


#ifdef ModelP
/* parameters for CheckGrid() */
#define GHOSTS  1
#define GEOM    1
#define ALG     0
#define LIST    1
#define IF      1


/****************************************************************************/
/*
   CheckConsistency -

   SYNOPSIS:
   void CheckConsistency (MULTIGRID *theMG, INT level ,INT debugstart, INT gmlevel, INT *check);

   PARAMETERS:
   .  theMG
   .  level
   .  debugstart
   .  gmlevel
   .  check

 */
/****************************************************************************/

static void CheckConsistency (MULTIGRID *theMG, INT level ,INT debugstart, INT gmlevel, int *check)
{
  GRID *theGrid = GRID_ON_LEVEL(theMG,level);

  IFDEBUG(gm,debugstart)
  printf(PFMT "AdaptMultiGrid(): %d. ConsCheck() on level=%d\n",theMG->ppifContext().me(),(*check)++,level);
                #ifdef Debug
  Debuggm = GHOSTS;
                #endif
  CheckGrid(theGrid,GEOM,ALG,LIST,IF);
                #ifdef Debug
  Debuggm=gmlevel;
                #endif
  if (DDD_ConsCheck(theMG->dddContext()) > 0) buggy(theMG);
  ENDDEBUG
}
#endif


#ifdef STAT_OUT
void NS_DIM_PREFIX Manage_Adapt_Timer (int alloc)
{
  if (alloc)
  {
    NEW_TIMER(adapt_timer)
    NEW_TIMER(closure_timer)
    NEW_TIMER(gridadapt_timer)
    NEW_TIMER(gridadapti_timer)
    NEW_TIMER(gridadaptl_timer)
    NEW_TIMER(ident_timer)
    NEW_TIMER(overlap_timer)
    NEW_TIMER(gridcons_timer)
    NEW_TIMER(algebra_timer)
  }
  else
  {
    DEL_TIMER(adapt_timer)
    DEL_TIMER(closure_timer)
    DEL_TIMER(gridadapt_timer)
    DEL_TIMER(gridadapti_timer)
    DEL_TIMER(gridadaptl_timer)
    DEL_TIMER(ident_timer)
    DEL_TIMER(overlap_timer)
    DEL_TIMER(gridcons_timer)
    DEL_TIMER(algebra_timer)
  }
}

void NS_DIM_PREFIX Print_Adapt_Timer (const MULTIGRID* theMG, int total_adapted)
{
  UserWriteF("ADAPT: total_adapted=%d t_adapt=%.2f: t_closure=%.2f t_gridadapt=%.2f t_gridadapti=%.2f "
             "t_gridadaptl=%.2f t_overlap=%.2f t_ident=%.2f t_gridcons=%.2f t_algebra=%.2f\n",
             total_adapted,EVAL_TIMER(adapt_timer),EVAL_TIMER(closure_timer),EVAL_TIMER(gridadapt_timer),
             EVAL_TIMER(gridadapti_timer),EVAL_TIMER(gridadaptl_timer),EVAL_TIMER(overlap_timer),
             EVAL_TIMER(ident_timer),EVAL_TIMER(gridcons_timer),EVAL_TIMER(algebra_timer));
  UserWriteF("ADAPTMAX: total_adapted=%d t_adapt=%.2f: t_closure=%.2f t_gridadapt=%.2f "
             "t_gridadapti=%.2f "
             "t_gridadaptl=%.2f t_overlap=%.2f t_ident=%.2f t_gridcons=%.2f t_algebra=%.2f\n",
             total_adapted,UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(adapt_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(closure_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(gridadapt_timer)),UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(gridadapti_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(gridadaptl_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(overlap_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(ident_timer)),UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(gridcons_timer)),
             UG_GlobalMaxDOUBLE(theMG->ppifContext(), EVAL_TIMER(algebra_timer)));
}
#endif

static INT      PreProcessAdaptMultiGrid(MULTIGRID *theMG)
{
  /* The matrices for the calculation are removed, to remember the
     recalculating the MGSTATUS is set to 1 */

  MGSTATUS(theMG) = 1 ;

  return(0);
}

static INT      PostProcessAdaptMultiGrid(MULTIGRID *theMG)
{
  START_TIMER(algebra_timer)
  if (CreateAlgebra(theMG)) REP_ERR_RETURN(1);
  SUM_TIMER(algebra_timer)

  REFINE_MULTIGRID_LIST(1,theMG,"END AdaptMultiGrid():\n","","");

  /*
          if (hFlag)
                  UserWriteF(" Number of green refinements not updated: "
                          "%d (%d green marks)\n",No_Green_Update,Green_Marks);
   */

  /* increment step count */
  SETREFINESTEP(REFINEINFO(theMG),REFINESTEP(REFINEINFO(theMG))+1);

  SUM_TIMER(adapt_timer)
  /*
     CheckMultiGrid(theMG);
   */

        #ifdef STAT_OUT
  Print_Adapt_Timer(theMG, total_adapted);
  Manage_Adapt_Timer(0);
        #endif

  return(0);
}

/****************************************************************************/
/** \brief Adapt whole multigrid structure

   \param theMG - multigrid to refine
   \param flag - flag for switching between different yellow closures

   This function refines whole multigrid structure

   \return <ul>
   <li> 0 - ok
   <li> 1 - out of memory, but data structure as before
   <li> 2 - fatal memory error, data structure corrupted
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX AdaptMultiGrid (MULTIGRID *theMG, INT flag, INT seq, INT mgtest)
{
  INT nrefined,nadapted;

  /* check necessary condition */
  if (!MG_COARSE_FIXED(theMG))
    return (GM_COARSE_NOT_FIXED);

  if (PreProcessAdaptMultiGrid(theMG)) REP_ERR_RETURN(1);

#ifdef ModelP
  {
    /* check and restrict partitioning of elements */
    if (CheckPartitioning(theMG))
    {
      /* Each call to RestrictPartitioning fixes the partitionings of children with
       * respect to their fathers.  To fix also fix the partitionings of the grandchildren,
       * the method has to be called again.  To be on the save side we call it once for
       * every level.
       * If I omit the loop (the way it was until 11.4.2014 I get assertion failures here
       * when mixing load balancing and adaptive refinement.  I am not really sure whether
       * the loop is the correct fix, or whether it just papers over the problem.
       * Anyway, no crashes for now.
       */
      for (int level = 0; level < TOPLEVEL(theMG); level++)
        if (RestrictPartitioning(theMG)) RETURN(GM_FATAL);
      if (CheckPartitioning(theMG)) assert(0);
    }
  }
#endif

        #ifdef STAT_OUT
  Manage_Adapt_Timer(1);
        #endif

  START_TIMER(adapt_timer)

  /* set up information in refine_info */
        #ifndef ModelP
  if (TOPLEVEL(theMG) == 0)
        #else
  if (UG_GlobalMaxINT(theMG->ppifContext(), TOPLEVEL(theMG)) == 0)
        #endif
  {
    SETREFINESTEP(REFINEINFO(theMG),0);
  }

  /* set info for refinement prediction */
  SetRefineInfo(theMG);
  /* evaluate prediction */
  if (mgtest)
  {
    UserWriteF("refinetest: predicted_new0=%9.0f predicted_new1=%9.0f\n",
               PREDNEW0(REFINEINFO(theMG)), PREDNEW1(REFINEINFO(theMG)));
  }

  /* set flags for different modes */
  rFlag=flag & 0x03;                    /* copy local or all */
  hFlag=!((flag>>2)&0x1);       /* use hanging nodes */
  fifoFlag=(flag>>3)&0x1;       /* use fifo              */

  refine_seq = seq;

  No_Green_Update=0;
  Green_Marks=0;

  /* drop marks to regular elements */
  if (hFlag)
    if (DropMarks(theMG)) RETURN(GM_ERROR);

  /* prepare algebra (set internal flags correctly) */
  START_TIMER(algebra_timer)

  PrepareAlgebraModification(theMG);

  SUM_TIMER(algebra_timer)

  const INT toplevel = TOPLEVEL(theMG);

  REFINE_MULTIGRID_LIST(1,theMG,"AdaptMultiGrid()","","")

  /* compute modification of coarser levels from above */
  START_TIMER(closure_timer)

  for (INT level = toplevel; level > 0; level--)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,level);

    if (hFlag)
    {
      PRINTDEBUG(gm,1,("Begin GridClosure(%d,down):\n",level))

      if ((nrefined = GridClosure(GRID_ON_LEVEL(theMG,level)))<0)
      {
        PrintErrorMessage('E',"AdaptMultiGrid","error in GridClosure");
        RETURN(GM_ERROR);
      }

      REFINE_GRID_LIST(1,theMG,level,("End GridClosure(%d,down):\n",level),"");
    }
                #ifdef ModelP
    else
    {
      ExchangeElementRefine(theGrid);
    }
                #endif

    /* restrict marks on next lower grid level */
    if (RestrictMarks(GRID_ON_LEVEL(theMG,level-1))!=GM_OK) RETURN(GM_ERROR);

    REFINE_GRID_LIST(1,theMG,level-1,("End RestrictMarks(%d,down):\n",level),"");
  }

  SUM_TIMER(closure_timer)


        #ifdef ModelP
  IdentifyInit(theMG);
        #endif

  INT newlevel = 0;
  for (INT level = 0; level <= toplevel; level++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,level);
    GRID *FinerGrid = nullptr;
    if (level < toplevel)
      FinerGrid = GRID_ON_LEVEL(theMG,level+1);

    START_TIMER(closure_timer)

    /* reset MODIFIED flags for grid and nodes */
    SETMODIFIED(theGrid,0);
    for (NODE *theNode = FIRSTNODE(theGrid); theNode != nullptr; theNode = SUCCN(theNode))
    {
      SETMODIFIED(theNode,0);
    }

    if (hFlag)
    {
      /* leave only regular marks */
      for (ELEMENT *theElement = PFIRSTELEMENT(theGrid); theElement != nullptr; theElement = SUCCE(theElement))
      {
        if ((ECLASS(theElement)==RED_CLASS) && MARKCLASS(theElement)==RED_CLASS) continue;
        SETMARK(theElement,NO_REFINEMENT);
      }

      PRINTDEBUG(gm,1,("Begin GridClosure(%d,up):\n",level));

      /* determine regular and irregular elements on next level */
      if ((nrefined = GridClosure(theGrid))<0)
      {
        PrintErrorMessage('E',"AdaptMultiGrid","error in 2. GridClosure");
        RETURN(GM_ERROR);
      }

      REFINE_GRID_LIST(1,theMG,level,("End GridClosure(%d,up):\n",level),"");
    }
                #ifdef ModelP
    else
    {
      ExchangeElementRefine(theGrid);
    }
                #endif

    nrefined += ComputeCopies(theGrid);

    /** \todo bug fix to force new level creation */
    if (!hFlag)
    {
      /* set this variable>0 */
      nrefined = 1;
    }

    /* create a new grid level, if at least one element is refined on finest level */
    if (nrefined>0 && level==toplevel) newlevel = 1;
#ifdef ModelP
    newlevel = UG_GlobalMaxINT(theMG->ppifContext(), newlevel);
#endif
    if (newlevel)
    {
      if (CreateNewLevel(theMG)==NULL)
        RETURN(GM_FATAL);
      FinerGrid = GRID_ON_LEVEL(theMG,toplevel+1);
    }

    PRINTDEBUG(gm,1,(PFMT "AdaptMultiGrid(): toplevel=%d nrefined=%d newlevel=%d\n",
                     me,toplevel,nrefined,newlevel));

    SUM_TIMER(closure_timer)

    /* now really manipulate the next finer level */
    START_TIMER(gridadapt_timer)

    nadapted = 0;

    if (level<toplevel || newlevel)
                        #ifndef ModelP
      if (AdaptGrid(theGrid,&nadapted)!=GM_OK)
        RETURN(GM_FATAL);
                        #else
      if (AdaptGrid(theGrid,toplevel,level,newlevel,&nadapted)!=GM_OK)
        RETURN(GM_FATAL);
                        #endif

    SUM_TIMER(gridadapt_timer)

    /* if no grid adaption has occurred adapt next level */
    if (nadapted == 0) continue;

    total_adapted += nadapted;

    if (level<toplevel || newlevel)
    {
      START_TIMER(algebra_timer)

      /* and compute the vector classes on the new (or changed) level */
      ClearNodeClasses(FinerGrid);

      for (ELEMENT *theElement = FIRSTELEMENT(FinerGrid); theElement != nullptr; theElement = SUCCE(theElement))
        if (ECLASS(theElement)>=GREEN_CLASS || (rFlag==GM_COPY_ALL)) {
          SeedNodeClasses(theElement);
        }

      PropagateNodeClasses(FinerGrid);

      SUM_TIMER(algebra_timer)
    }
  }

        #ifdef ModelP
  IdentifyExit();

  /* now repair inconsistencies                   */
  /* former done on each grid level (s.l. 980522) */
  START_TIMER(gridcons_timer);

  ConstructConsistentMultiGrid(theMG);

  SUM_TIMER(gridcons_timer)
        #endif

  DisposeTopLevel(theMG);
  if (TOPLEVEL(theMG) > 0) DisposeTopLevel(theMG);
  CURRENTLEVEL(theMG) = TOPLEVEL(theMG);

  if (PostProcessAdaptMultiGrid(theMG)) REP_ERR_RETURN(1);

  return(GM_OK);
}
