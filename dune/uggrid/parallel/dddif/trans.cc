// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  trans.c                                                                                                       */
/*																			*/
/* Purpose:   create new grid distribution according to lb-marks of master  */
/*            elements.                                                                                                 */
/*																			*/
/* Author:	  Klaus Birken, Stefan Lang                                                             */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   961216 kb  begin                                                                          */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/


#ifdef ModelP

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>
#include <cassert>

#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

#include "parallel.h"
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/domain/std_domain.h>
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

/* UG namespaces: */
USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

enum GhostCmds { GC_Keep, GC_ToMaster, GC_Delete };


#define XferElement(context, elem,dest,prio)                             \
  { PRINTDEBUG(dddif,1,("%4d: XferElement(): XferCopy elem=" EID_FMTX " dest=%d prio=%d\n", \
                        me,EID_PRTX(elem), dest, prio)); \
    XFERECOPYX(context, (elem), dest, prio,               \
               (OBJT(elem)==BEOBJ) ?   \
               BND_SIZE_TAG(TAG(elem)) :   \
               INNER_SIZE_TAG(TAG(elem)));  }

#ifdef DDD_PRIO_ENV
#undef DDD_PrioritySet
#endif

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/


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


/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/



/****************************************************************************/
/*
   Gather_ElemDest -

   SYNOPSIS:
   static int Gather_ElemDest (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Gather_ElemDest (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  *(DDD_PROC *)data = PARTITION(theElement);

  return 0;
}


/****************************************************************************/
/*
   Scatter_ElemDest -

   SYNOPSIS:
   static int Scatter_ElemDest (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Scatter_ElemDest (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PARTITION(theElement) = *(DDD_PROC *)data;

  return 0;
}


/****************************************************************************/
/*
   UpdateGhostDests -

   SYNOPSIS:
   static int UpdateGhostDests (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int UpdateGhostDests (MULTIGRID *theMG)
{
  auto& context = theMG->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  DDD_IFOneway(context,
               dddctrl.ElementIF, IF_FORWARD, sizeof(DDD_PROC),
               Gather_ElemDest, Scatter_ElemDest);

  DDD_IFOneway(context,
               dddctrl.ElementVIF, IF_FORWARD, sizeof(DDD_PROC),
               Gather_ElemDest, Scatter_ElemDest);

  return 0;
}



/****************************************************************************/


/****************************************************************************/
/*
   Gather_GhostCmd -

   SYNOPSIS:
   static int Gather_GhostCmd (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

   PARAMETERS:
   .  obj
   .  data
   .  proc
   .  prio

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Gather_GhostCmd (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;
  INT j;

  /* TODO: not needed anymore. kb 9070108 */
  if (PARTITION(theElement) == proc)
  {
    *((int *)data) = GC_ToMaster;

    return(0);
  }

  if (PARTITION(theElement) != proc)
  {
    *((int *)data) = GC_Delete;

    for(j=0; j<SIDES_OF_ELEM(theElement); j++)
    {
      ELEMENT *nb = NBELEM(theElement,j);

      if (nb!=NULL)
      {
        if (PARTITION(nb)==proc)
        {
          *((int *)data) = GC_Keep;
          break;
        }
      }
    }
    return(0);
  }

  return(1);
}


/****************************************************************************/
/*
   Scatter_GhostCmd -

   SYNOPSIS:
   static int Scatter_GhostCmd (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

   PARAMETERS:
   .  obj
   .  data
   .  proc
   .  prio

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Scatter_GhostCmd (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;
  ELEMENT *SonList[MAX_SONS];
  INT i;

  const auto& me = context.me();

  switch (*(int *)data)
  {
  case GC_Keep :
    /* do nothing */
    break;

  case GC_ToMaster :
    /* TODO: not needed anymore. kb 9070108
                            SETEPRIO(theElement, PrioMaster);*/
    break;

  case GC_Delete :
    if (NSONS(theElement) > 0)
    {
      if (GetAllSons(theElement,SonList) != 0) return(1);
      i = 0;
      while (i<MAX_SONS && SonList[i] != NULL)
      {
        if (PARTITION(SonList[i]) == me) return(0);
        i++;
      }
    }
    XFEREDELETE(context, theElement);
    break;

  default :
    assert(1);
  }

  return(0);
}


/****************************************************************************/
/*
   Gather_VHGhostCmd -

   SYNOPSIS:
   static int Gather_VHGhostCmd (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

   PARAMETERS:
   .  obj
   .  data
   .  proc
   .  prio

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Gather_VHGhostCmd (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;
  // ELEMENT *theFather      = EFATHER(theElement);
  INT j;

  if (PARTITION(theElement) != proc)
  {
    *((int *)data) = GC_Delete;

    for(j=0; j<SIDES_OF_ELEM(theElement); j++)
    {
      ELEMENT *theNeighbor = NBELEM(theElement,j);

      if (theNeighbor != NULL)
      {
        if (PARTITION(theNeighbor) == proc)
        {
          *((int *)data) = GC_Keep;
          return (0);
        }
      }
    }

    /* wrong:		if (LEVEL(theElement) > 0)
                    {
                            ASSERT(theFather != NULL);

                            if (PARTITION(theFather) == proc)
                            {
       *((int *)data) = GC_Keep;
                                    return (0);
                            }
                    } */
    return (0);
  }

  return(1);
}


/****************************************************************************/
/*
   Scatter_VHGhostCmd -

   SYNOPSIS:
   static int Scatter_VHGhostCmd (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

   PARAMETERS:
   .  obj
   .  data
   .  proc
   .  prio

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Scatter_VHGhostCmd (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  const auto& me = context.me();

  ELEMENT *theElement = (ELEMENT *)obj;
  ELEMENT *SonList[MAX_SONS];

  /* if element is needed after transfer here */
  if ((*(int *)data) == GC_Keep) return(0);

  /* element becomes master copy */
  if (PARTITION(theElement) == me) return(0);

  /* if a son resides as master keep element as vghost */
  if (GetAllSons(theElement,SonList) != GM_OK) return(0);
  INT i = 0;
  while (i<MAX_SONS && SonList[i] != NULL)
  {
    if (PARTITION(SonList[i]) == me) return(0);
    i++;
  }
  /* element is not needed on me any more */
  if ((*(int *)data) == GC_Delete)
  {
    XFEREDELETE(context, theElement);
    return(0);
  }

  return(1);
}


/****************************************************************************/
/*
   ComputeGhostCmds -

   SYNOPSIS:
   static int ComputeGhostCmds (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int ComputeGhostCmds (MULTIGRID *theMG)
{
  auto& context = theMG->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  DDD_IFOnewayX(context,
                dddctrl.ElementVHIF, IF_FORWARD, sizeof(int),
                Gather_VHGhostCmd, Scatter_VHGhostCmd);

  return(0);
}

/****************************************************************************/
/*
   XferGridWithOverlap - send elements to other procs, keep overlapping region of one element, maintain correct priorities at interfaces.

   SYNOPSIS:
   static void XferGridWithOverlap (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:
   This function sends elements to other procs, keeps overlapping region of one element and maintains correct priorities at interfaces. The destination procs have been computed by the RecursiveCoordinateBisection function and put into the elements' PARTITION-entries.

   RETURN VALUE:
   void
 */
/****************************************************************************/

static int XferGridWithOverlap (GRID *theGrid)
{
  ELEMENT *theElement, *theFather, *theNeighbor;
  ELEMENT *SonList[MAX_SONS];
  INT i,j,overlap_elem,part;
  INT migrated = 0;

  DDD::DDDContext& context = theGrid->dddContext();
  const auto& me = context.me();

  for(theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* goal processor */
    part = PARTITION(theElement);

    /* create Master copy */
    XferElement(context, theElement, part, PrioMaster);

                #ifdef STAT_OUT
    /* count elems */
    if (part != me) migrated++;
                #endif
  }

  /* create grid overlap */
  for(theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    overlap_elem = 0;

    /* create 1-overlapping of horizontal elements */
    for(j=0; j<SIDES_OF_ELEM(theElement); j++)
    {
      theNeighbor = NBELEM(theElement,j);

      if (theNeighbor != NULL)
      {
        if (PARTITION(theElement)!=PARTITION(theNeighbor))
        {
          /* create Ghost copy */
          XferElement(context, theElement, PARTITION(theNeighbor), PrioHGhost);
        }

        /* remember any local neighbour element */
        if (PARTITION(theNeighbor)==me)
          overlap_elem = 1;
      }
    }

    /* create 1-overlapping of vertical elements */
    theFather = EFATHER(theElement);
    if (theFather != NULL)
    {
      if (PARTITION(theFather) != PARTITION(theElement) ||
          !EMASTER(theFather))
        /* create VGhost copy */
        XferElement(context, theFather, PARTITION(theElement), PrioVGhost);
    }
    else
    {
      ASSERT(LEVEL(theElement) == 0);
    }

    /* consider elements on master-proc */
    if (PARTITION(theElement)!=me)
    {
      if (NSONS(theElement) > 0)
      {
        if (GetAllSons(theElement,SonList) != 0) assert(0);
        i = 0;
        while (i<MAX_SONS && SonList[i] != NULL)
        {
          if (PARTITION(SonList[i]) == me)
          {
            overlap_elem += 2;
            break;
          }
          i++;
        }
      }

      PRINTDEBUG(dddif,1,("%d: XferGridWithOverlap(): elem=" EID_FMTX " p=%d new prio=%d\n",
                          me,EGID(theElement),PARTITION(theElement),overlap_elem));

      if (overlap_elem > 0)
      {
        /* element is needed, set new prio */
        switch (overlap_elem)
        {
        case (1) :
          SETEPRIO(context, theElement,PrioHGhost);
          break;

        case (2) :
          SETEPRIO(context, theElement,PrioVGhost);
          break;

        case (3) :
          SETEPRIO(context, theElement,PrioVGhost);
          break;

        default :
          assert(0);
        }
      }
      else
      {
        /* element isn't needed */
        PRINTDEBUG(dddif,2,("%d: XferGridWithOverlap(): XferDel elem=%d to p=%d prio=%d\n",
                            me,EGID(theElement),PARTITION(theElement),PrioHGhost));

        XFEREDELETE(context, theElement);
      }
    }
  }

  return(migrated);
}



/****************************************************************************/


/****************************************************************************/
/*
   InheritPartitionBottomTop -

   SYNOPSIS:
   static void InheritPartitionBottomTop (ELEMENT *e);

   PARAMETERS:
   .  e

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void InheritPartitionBottomTop (ELEMENT *e)
{
  int i;
  ELEMENT *SonList[MAX_SONS];

  if (GetSons(e,SonList) != GM_OK) assert(0);

  for(i=0; i<MAX_SONS; i++)
  {
    ELEMENT *son = SonList[i];
    if (son==NULL) break;

    PARTITION(son) = PARTITION(e);
    InheritPartitionBottomTop(son);
  }
}


/****************************************************************************/


/****************************************************************************/
/*
   TransferGridFromLevel -

   SYNOPSIS:
   int TransferGridFromLevel (MULTIGRID *theMG, INT level);

   PARAMETERS:
   .  theMG
   .  level

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

int NS_DIM_PREFIX TransferGridFromLevel (MULTIGRID *theMG, INT level)
{
#ifdef STAT_OUT
  INT migrated = 0;       /* number of elements moved */
  DOUBLE trans_begin, trans_end, cons_end;
#endif

#ifdef STAT_OUT
  trans_begin = CURRENT_TIME;
#endif

  /* send new destination to ghost elements */
  UpdateGhostDests(theMG);

  /* init transfer */
  ddd_HandlerInit(theMG->dddContext(), HSET_XFER);

  /* start physical transfer */
  DDD_XferBegin(theMG->dddContext());

  {
    /* send 'commands' to ghosts in old partitioning */
    ComputeGhostCmds(theMG);

    /* send all grids */
    for (INT g=0; g<=TOPLEVEL(theMG); g++)
    {
      GRID *theGrid = GRID_ON_LEVEL(theMG,g);
      if (NT(theGrid)>0)
      {
#ifdef STAT_OUT
        migrated += XferGridWithOverlap(theGrid);
#else
        XferGridWithOverlap(theGrid);
#endif
      }
    }
  }

  DDD_XferEnd(theMG->dddContext());

#ifdef STAT_OUT
  trans_end = CURRENT_TIME;
#endif

  /* set priorities of border nodes */
  /* TODO this is an extra communication. eventually integrate this
              with grid distribution phase. */
  ConstructConsistentMultiGrid(theMG);

  /* the grid has changed at least on one processor, thus reset MGSTATUS on all processors */
  RESETMGSTATUS(theMG);

        #ifdef STAT_OUT
  cons_end = CURRENT_TIME;

  /* sum up moved elements */
  migrated = UG_GlobalSumINT(theMG->ppifContext(), migrated);

  UserWriteF("MIGRATION: migrated=%d t_migrate=%.2f t_cons=%.2f\n",
             migrated,trans_end-trans_begin,cons_end-trans_end);
        #endif

        #ifdef Debug
  if (0)
    DDD_ConsCheck(theMG->dddContext());
        #endif

  return 0;
}


/****************************************************************************/
/*
   TransferGrid -

   SYNOPSIS:
   int TransferGrid (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

int NS_DIM_PREFIX TransferGrid (MULTIGRID *theMG)
{
  return TransferGridFromLevel(theMG,0);
}

/****************************************************************************/

#endif  /* ModelP */
