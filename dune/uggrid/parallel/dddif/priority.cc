// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  priority.c													*/
/*																			*/
/* Purpose:   functions for managing priorities of distributed objects      */
/*																			*/
/* Author:	  Stefan Lang                                                                       */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: birken@ica3.uni-stuttgart.de							*/
/*																			*/
/* History:   980201 sl  begin                                                                                          */
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
#include <cstdlib>

#include "parallel.h"
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/shapes.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/namespace.h>

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

/* macros for merge new priority with objects existing one */
/* valid only for all types of ghost priorities            */
#define PRIO_CALC(e) ((USED(e) && THEFLAG(e)) ? PrioVHGhost :                \
                      (THEFLAG(e)) ? PrioVGhost : (USED(e)) ?              \
                      PrioHGhost : (assert(0),0))

#ifdef __PERIODIC_BOUNDARY__
/* to set correct priority of periodic vectors having master nodes */
#define SETPRIOPV SETPRIO
#else
#define SETPRIOPV SETPRIOX
#endif

/* macros for setting object priorities with related objects */
inline void NODE_PRIORITY_SET(DDD::DDDContext& context, GRID* grid, NODE* node, INT prio)
{
  /* set priorities of node */
  SETPRIOX(context, node, prio);

  if (VEC_DEF_IN_OBJ_OF_GRID(grid, NODEVEC) and NVECTOR(node) != nullptr)
    SETPRIOPV(context, NVECTOR(node),prio);
}

inline void PRIO_SET_EDGE(DDD::DDDContext& context, EDGE* edge, INT prio)
{
  SETPRIOX(context, edge, prio);
}

inline void EDGE_PRIORITY_SET(DDD::DDDContext& context, GRID* grid, EDGE* edge, INT prio)
{
  /* set priorities of node for 3D */
  PRIO_SET_EDGE(context, edge, prio);

  /* set priority of edge vector */
  if (VEC_DEF_IN_OBJ_OF_GRID(grid, EDGEVEC) and EDVECTOR(edge) != nullptr)
    SETPRIOX(context, EDVECTOR(edge),prio);
}


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

REP_ERR_FILE



/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/*
        for all PrioMaster-nodes with remote copies, set exactly one
        to PrioMaster, the other copies to PrioBorder in order to establish
        the BorderNodeIF. this is done for one grid.
 */


/****************************************************************************/
/*
   ComputeNodeBorderPrios -

   SYNOPSIS:
   static int ComputeNodeBorderPrios (DDD_OBJ obj);

   PARAMETERS:
   .  obj -

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int ComputeNodeBorderPrios (DDD::DDDContext& context, DDD_OBJ obj)
{
  NODE    *node  = (NODE *)obj;
  int     *plist = DDD_InfoProcList(context, PARHDR(node));
  int i, min_proc = context.procs();

  /*
          minimum processor number will get Master-node,
          all others get Border-nodes
   */
  for(i=0; plist[i]>=0; i+=2)
  {
    if (plist[i+1]==PrioMaster && plist[i]<min_proc)
      min_proc = plist[i];
  }

  if (min_proc == context.procs())
    return(0);

  if (context.me() != min_proc)
    SETPRIO(context, node, PrioBorder);
  return 0;
}


/****************************************************************************/
/*
   ComputeVectorBorderPrios -

   SYNOPSIS:
   static int ComputeVectorBorderPrios (DDD_OBJ obj);

   PARAMETERS:
   .  obj -

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int ComputeVectorBorderPrios (DDD::DDDContext& context, DDD_OBJ obj)
{
  VECTOR  *vector  = (VECTOR *)obj;
  int     *plist = DDD_InfoProcList(context, PARHDR(vector));
  int i, min_proc = context.procs();

  /*
          minimum processor number will get Master-node,
          all others get Border-nodes
   */
  for(i=0; plist[i]>=0; i+=2)
  {
    if (plist[i+1]==PrioMaster && plist[i]<min_proc)
      min_proc = plist[i];
  }

  if (min_proc == context.procs())
    return(0);

  if (context.me() != min_proc)
    SETPRIO(context, vector, PrioBorder);
  return 0;
}



/****************************************************************************/
/*
   ComputeEdgeBorderPrios -

   SYNOPSIS:
   static int ComputeEdgeBorderPrios (DDD_OBJ obj);

   PARAMETERS:
   .  obj -

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static int ComputeEdgeBorderPrios (DDD::DDDContext& context, DDD_OBJ obj)
{
  EDGE    *edge  =        (EDGE *)obj;
  int             *plist =        DDD_InfoProcList(context, PARHDR(edge));
  int i, min_proc     = context.procs();

  /*
          minimum processor number will get Master-node,
          all others get Border-nodes
   */
  for(i=0; plist[i]>=0; i+=2)
  {
    if (plist[i+1]==PrioMaster && plist[i]<min_proc)
      min_proc = plist[i];
  }

  if (min_proc == context.procs())
    return(0);

  if (context.me() != min_proc)
    SETPRIO(context, edge, PrioBorder);
  return 0;
}


/****************************************************************************/
/*
   SetGhostObjectPriorities -

   SYNOPSIS:
   void SetGhostObjectPriorities (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void NS_DIM_PREFIX SetGhostObjectPriorities (GRID *theGrid)
{
  ELEMENT *theElement,*theNeighbor,*SonList[MAX_SONS];
  NODE    *theNode;
  EDGE    *theEdge;
  VECTOR  *theVector;
  INT i,prio,hghost,vghost;

  auto& context = theGrid->dddContext();
  const auto& me = context.me();

  /* reset USED flag for objects of ghostelements */
  for (theElement=PFIRSTELEMENT(theGrid);
       theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    SETUSED(theElement,0); SETTHEFLAG(theElement,0);
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                        CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      ASSERT(theEdge != NULL);
      SETUSED(theEdge,0); SETTHEFLAG(theEdge,0);
    }
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        theVector = SVECTOR(theElement,i);
        if (theVector != NULL) {
          SETUSED(theVector,0);
          SETTHEFLAG(theVector,0);
        }
      }
  }
  /* to reset also nodes which are at corners of the boundary */
  /* reset of nodes need to be done through the node list     */
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    SETUSED(theNode,0); SETTHEFLAG(theNode,0);
    SETMODIFIED(theNode,0);
  }

  /* set FLAG for objects of horizontal and vertical overlap */
  for (theElement=PFIRSTELEMENT(theGrid);
       theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (PARTITION(theElement) == me) continue;

    /* check for horizontal ghost */
    hghost = 0;
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      theNeighbor = NBELEM(theElement,i);
      if (theNeighbor == NULL) continue;

      if (PARTITION(theNeighbor) == me)
      {
        hghost = 1;
        break;
      }
    }

    /* check for vertical ghost */
    vghost = 0;
    GetAllSons(theElement,SonList);
    for (i=0; SonList[i]!=NULL; i++)
    {
      if (PARTITION(SonList[i]) == me)
      {
        vghost = 1;
        break;
      }
    }

    /* one or both of vghost and hghost should be true here   */
    /* except for elements which will be disposed during Xfer */

    if (vghost) SETTHEFLAG(theElement,1);
    if (hghost) SETUSED(theElement,1);
    for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    {
      theNode = CORNER(theElement,i);
      if (vghost) SETTHEFLAG(theNode,1);
      if (hghost) SETUSED(theNode,1);
    }
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement,i,0),
                        CORNER_OF_EDGE_PTR(theElement,i,1));
      ASSERT(theEdge != NULL);
      if (vghost) SETTHEFLAG(theEdge,1);
      if (hghost) SETUSED(theEdge,1);
    }
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        theVector = SVECTOR(theElement,i);
        if (theVector != NULL) {
          if (vghost) SETTHEFLAG(theVector,1);
          if (hghost) SETUSED(theVector,1);
        }
      }
  }

  /* set USED flag for objects of master elements */
  /* reset FLAG for objects of master elements  */
  for (theElement=PFIRSTELEMENT(theGrid);
       theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (PARTITION(theElement) != me) continue;

    SETUSED(theElement,0); SETTHEFLAG(theElement,0);
    for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    {
      theNode = CORNER(theElement,i);
      SETUSED(theNode,0); SETTHEFLAG(theNode,0);
      SETMODIFIED(theNode,1);
    }
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement,i,0),
                        CORNER_OF_EDGE_PTR(theElement,i,1));
      ASSERT(theEdge != NULL);
      SETUSED(theEdge,0); SETTHEFLAG(theEdge,0);
    }
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        theVector = SVECTOR(theElement,i);
        if (theVector != NULL) {
          SETUSED(theVector,0);
          SETTHEFLAG(theVector,0);
        }
      }
  }

  /* set object priorities for ghostelements */
  for (theElement=PFIRSTELEMENT(theGrid);
       theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (PARTITION(theElement) == me) continue;

    if (USED(theElement) || THEFLAG(theElement))
    {
      prio = PRIO_CALC(theElement);
      PRINTDEBUG(gm,1,("SetGhostObjectPriorities(): e=" EID_FMTX " new prio=%d\n",
                       EID_PRTX(theElement),prio))
      SETEPRIOX(context, theElement, prio);

      if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
      {
        theVector = EVECTOR(theElement);
        if (theVector != NULL)
          SETPRIOX(context, theVector,prio);
      }
    }

    /* set edge priorities */
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {

      theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                        CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      ASSERT(theEdge != NULL);

      if (USED(theEdge) || THEFLAG(theEdge))
      {
        PRINTDEBUG(dddif,3,(PFMT " dddif_SetGhostObjectPriorities():"
                            " downgrade edge=" EDID_FMTX " from=%d to PrioHGhost\n",
                            me,EDID_PRTX(theEdge),prio));

        EDGE_PRIORITY_SET(context, theGrid,theEdge,PRIO_CALC(theEdge));
      }
      else
        EDGE_PRIORITY_SET(context, theGrid,theEdge,PrioMaster);
    }

                        #ifdef __THREEDIM__
    /* if one(all) of the side nodes is (are) a hghost (vghost) node   */
    /* then its a hghost (vghost) side vector                          */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        if (USED(theVector) || THEFLAG(theVector))
          SETPRIOX(context, theVector,PRIO_CALC(theVector));
      }
                        #endif

  }
  /* to set also nodes which are at corners of the boundary   */
  /* set them through the node list                           */
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    /* check if its a master node */
    if (USED(theNode) || THEFLAG(theNode))
    {
      PRINTDEBUG(dddif,3,(PFMT " dddif_SetGhostObjectPriorities():"
                          " downgrade node=" ID_FMTX " from=%d to PrioHGhost\n",
                          me,ID_PRTX(theNode),prio));

      /* set node priorities of node to ghost */
      NODE_PRIORITY_SET(context, theGrid,theNode,PRIO_CALC(theNode));
    }
    else if (MODIFIED(theNode) == 0)
    {
      /* this is a node of the boundary without connection to master elements */
      NODE_PRIORITY_SET(context, theGrid,theNode,PrioHGhost);
    }
    /* this is needed only for consistency after refinement */
    /* ghost nodes which belong after refinement to master  */
    /* elements have to be upgraded explicitly (980126 s.l.)*/
    else if (MODIFIED(theNode) == 1)
    {
      NODE_PRIORITY_SET(context, theGrid,theNode,PrioMaster);
    }
  }
}


/****************************************************************************/
/*
   SetBorderPriorities -

   SYNOPSIS:
   INT SetBorderPriorities (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX SetBorderPriorities (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  DDD_IFAExecLocal(context,
                   dddctrl.BorderNodeSymmIF,GRID_ATTR(theGrid),
                   ComputeNodeBorderPrios);

  /* TODO: distinguish two cases:
     1. only nodevectors then setting of vector prios can
          be done in ComputeNodeBorderPrios without extra communiction
     2. with other vectortypes (side and/or edgevectors) use
          ComputeVectorBorderPrios
   */
  DDD_IFAExecLocal(context,
                   dddctrl.BorderVectorSymmIF,GRID_ATTR(theGrid),
                   ComputeVectorBorderPrios);

  DDD_IFAExecLocal(context,
                   dddctrl.BorderEdgeSymmIF,GRID_ATTR(theGrid),
                   ComputeEdgeBorderPrios);

  return(GM_OK);
}


/****************************************************************************/
/*
   SetGridBorderPriorities -

   SYNOPSIS:
   INT SetGridBorderPriorities (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/


INT NS_DIM_PREFIX SetGridBorderPriorities (GRID *theGrid)
{
  /* set border priorities on next higher level */
  if (SetBorderPriorities(UPGRID(theGrid)) != GM_OK) return(GM_FATAL);

  return(GM_OK);
}

#endif
