// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  identify.c													*/
/*																			*/
/* Purpose:   identification of distributed ug objects                                  */
/*																			*/
/* Author:	  Stefan Lang                                                                                   */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   26.11.96 begin, first version extracted from refine.c			*/
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
#include <cstdio>

#include <dune/uggrid/ugdevices.h>

#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/rm.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/parallel/ddd/include/ddd.h>
#include "identify.h"
#include "parallel.h"

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

/* flags for identification */
enum {CLEAR = 0,
      IDENT = 1};

/* maximum count of objects for identification */
#define MAX_OBJECT      3

/* maximum count of tokens for identification */
#define MAX_TOKEN       10

/* determine the ddd header for identification of a node */
#define NIDENT_HDR(node) ( (CORNERTYPE(node)) ? \
                           PARHDR((NODE *)NFATHER(node)) : PARHDR(node) )

/* mapping of flags used for identification */
#define NIDENT(p)                     THEFLAG(p)
#define SETNIDENT(p,n)                SETTHEFLAG(p,n)

#define EDIDENT(p)                    THEFLAG(p)
#define SETEDIDENT(p,n)               SETTHEFLAG(p,n)

/* strong checking of identification (0=off 1==on) */
#define NIDENTASSERT    1
#define EDIDENTASSERT   1

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

INT NS_DIM_PREFIX ident_mode = IDENT_OFF;

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/


/* this function is called for low level identification */
static INT (*Ident_FctPtr)(DDD::DDDContext& context, DDD_HDR *IdentObjectHdr, INT nobject,
                           const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident) = NULL;

static int check_nodetype = 0;

#ifdef Debug
static INT debug = 0;
static INT identlevel = 0;
#endif

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*
   ResetIdentFlags -

   SYNOPSIS:
   static void ResetIdentFlags (GRID *UpGrid);

   PARAMETERS:
   .  UpGrid

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void ResetIdentFlags (GRID *Grid)
{
  /* clear all IDENT flags */
  for (NODE* theNode=FIRSTNODE(Grid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    SETNIDENT(theNode,CLEAR);
    SETUSED(theNode,0);

    for (LINK* theLink=START(theNode); theLink!=NULL; theLink=NEXT(theLink))
    {
      EDGE* theEdge = MYEDGE(theLink);
      SETEDIDENT(theEdge,CLEAR);
    }
  }
}

#ifdef Debug


/****************************************************************************/
/*
   Print_Identify_ObjectList -

   SYNOPSIS:
   static INT Print_Identify_ObjectList (DDD_HDR *IdentObjectHdr, INT nobject,
                                const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident);

   PARAMETERS:
   .  IdentObjectHdr
   .  nobject
   .  proclist
   .  skiptag
   .  IdentHdr
   .  nident

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT Print_Identify_ObjectList (DDD_HDR *IdentObjectHdr, INT nobject,
                                      const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident)
{
  ASSERT(nobject>0);
  ASSERT(nident>0);
  ASSERT(!proclist.empty());

  /* print the interesting call parameters */
  PrintDebug ("%d:    Print_Identify_ObjectList(): nobject=%d nident=%d"
              " skiptag=%d\n",me,nobject,nident,skiptag);

  /* print line prefix */
  PrintDebug ("%d: l=%d",me,identlevel);

  /* print the objects used for identify */
  PrintDebug ("    IdentHdr:");
  for (INT i=0; i<nident; i++)
    PrintDebug (" %d",DDD_InfoGlobalId(IdentHdr[i]));

  /* print type of objects to identify */
  PrintDebug ("    IdentObjectType:");
  for (INT i=0; i<nobject; i++)
    PrintDebug (" %d",DDD_InfoType(IdentObjectHdr[i]));

  /* print the proclist to identify to */
  PrintDebug ("    ProcList: %d",me);
  for (auto&& [proc, prio] : proclist)
  {
    if (prio == skiptag)
      continue;

    PrintDebug (" %d", proc);
  }

  /* print my processor number */
  PrintDebug ("    me:%d",me);

  /* print the objects to identify */
  PrintDebug ("    IdentObjectHdr:");
  for (INT i=0; i<nobject; i++)
    PrintDebug (" %d",DDD_InfoGlobalId(IdentObjectHdr[i]));

  PrintDebug ("\n");

  return 0;

}
#endif

#ifdef Debug


/****************************************************************************/
/*
   Print_Identified_ObjectList -

   SYNOPSIS:
   static INT Print_Identified_ObjectList (DDD_HDR *IdentObjectHdr, INT nobject, const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident);

   PARAMETERS:
   .  IdentObjectHdr
   .  nobject
   .  proclist
   .  skiptag
   .  IdentHdr

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT Print_Identified_ObjectList (DDD_HDR *IdentObjectHdr, INT nobject,
                                        const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident)
{
  ASSERT(nobject>0);
  ASSERT(nident>0);
  ASSERT(!proclist.empty());

  /* print the interesting call parameters */
  PrintDebug ("%d:    Print_Identified_ObjectList(): nobject=%d nident=%d"
              " skiptag=%d\n",me,nobject,nident,skiptag);

  /* print the objects to identify */
  PrintDebug ("%d: l=%d   IdentObjectHdr:",me,identlevel);
  for (INT i=0; i<nobject; i++)
    PrintDebug (" %d",DDD_InfoGlobalId(IdentObjectHdr[i]));

  /* print the objects used for identify */
  PrintDebug ("    IdentHdr:");
  for (INT i=0; i<nident; i++)
    PrintDebug (" %d",DDD_InfoGlobalId(IdentHdr[i]));

  /* print the proclist to identify to */
  PrintDebug ("    ProcList: %d",me);
  for (auto&& [proc, prio] : proclist)
  {
    if (prio == skiptag)
      continue;

    PrintDebug (" %d", proc);
  }

  /* print my processor number */
  PrintDebug ("    me:%d",me);

  /* print type of objects to identify */
  PrintDebug ("    IdentObjectType:");
  for (INT i=0; i<nobject; i++)
    PrintDebug (" %d",DDD_InfoType(IdentObjectHdr[i]));

  PrintDebug ("\n");

  return 0;
}
#endif



/****************************************************************************/
/*
   Identify_by_ObjectList -

   SYNOPSIS:
   static INT Identify_by_ObjectList (DDD_HDR *IdentObjectHdr, INT nobject,
                                const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident);

   PARAMETERS:
   .  IdentObjectHdr
   .  nobject
   .  proclist
   .  skiptag
   .  IdentHdr
   .  nident

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT Identify_by_ObjectList (DDD::DDDContext& context, DDD_HDR *IdentObjectHdr, INT nobject,
                                   const DDD_InfoProcListRange& proclist, DDD_PRIO skiptag, DDD_HDR *IdentHdr, INT nident)
{
  ASSERT(nobject>0);
  ASSERT(nident>0);
  ASSERT(!proclist.empty());

#ifdef Debug
  IFDEBUG(dddif,1)
  Print_Identify_ObjectList(IdentObjectHdr,nobject,proclist,skiptag,
                            IdentHdr,nident);
  ENDDEBUG
#endif

  INT n = 0;
  for (auto&& [proc, prio] : proclist)
  {
    ASSERT(n < context.procs());

    if (prio == skiptag)
      continue;

    /* identify the object */
    for (INT j=0; j<nobject; j++)
    {
      for (INT i=0; i<nident; i++)
      {
        PRINTDEBUG(dddif,5,("%d: Identify_by_ObjectList(): Type=%d"
                            " IdentObjectHdr=%08x proc=%d IdentHdr=%08x me=%d\n",
                            me,DDD_InfoType(IdentObjectHdr[j]),
                            DDD_InfoGlobalId(IdentObjectHdr[j]),
                            proc,DDD_InfoGlobalId(IdentHdr[i]),me));

        /* hand identification hdr to ddd */
        DDD_IdentifyObject(context, IdentObjectHdr[j], proc, IdentHdr[i]);
      }
    }

    n++;
    assert(n < context.procs());
  }

  /* identification should occur to at least one other proc */
  ASSERT(n>0);

  return 0;
}

#ifdef UG_DIM_3


static void IdentifySideVector (DDD::DDDContext& context, ELEMENT* theElement, ELEMENT *theNeighbor,
                               ELEMENT *Son, INT SonSide)
{
  INT nident = 0;
  DDD_HDR IdentObjectHdr[MAX_OBJECT];
  DDD_HDR IdentHdr[MAX_TOKEN];

  IdentObjectHdr[0] = PARHDR(SVECTOR(Son,SonSide));

  /* identify using corner nodes */
  for (INT k=0; k<CORNERS_OF_SIDE(Son,SonSide); k++)
  {
    NODE* theNode = CORNER(Son,CORNER_OF_SIDE(Son,SonSide,k));
    if (CORNERTYPE(theNode))
      IdentHdr[nident++] = PARHDR((NODE *)NFATHER(theNode));
    else
      IdentHdr[nident++] = PARHDR(theNode);
  }

  const auto& proclist = DDD_InfoProcListRange(context, PARHDRE(theNeighbor), false);

  Ident_FctPtr(context, IdentObjectHdr,1,proclist,PrioHGhost,IdentHdr,nident);

}
#endif


/****************************************************************************/
/*
   IdentifyNode -

   SYNOPSIS:
   static void IdentifyNode (ELEMENT *theNeighbor, NODE *theNode,
                                NODE *Nodes[MAX_SIDE_NODES], INT node, INT ncorners);

   PARAMETERS:
   .  theNeighbor
   .  theNode
   .  Nodes[MAX_SIDE_NODES]
   .  node
   .  ncorners

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void IdentifyNode (GRID *theGrid, ELEMENT *theNeighbor, NODE *theNode,
                          NODE *Nodes[MAX_SIDE_NODES], INT node, INT ncorners)
{
  auto& context = theGrid->dddContext();

  INT nobject,nident;
  DDD_HDR IdentObjectHdr[MAX_OBJECT];
  DDD_HDR IdentHdr[MAX_TOKEN];

  nobject = nident = 0;

  /* is this node identified? */
        #ifdef Debug
  if (debug == 1) {
    if (NIDENT(theNode) == CLEAR) return;
  }
  else
        #endif

  /* return if not needed any more */
  if (!USED(theNode)) return;

  /* return if already identified */
  if (NIDENT(theNode) == IDENT) return;

  /* only new created nodes are identified */
  if (!NEW_NIDENT(theNode)) return;

  switch (NTYPE(theNode))
  {

  case (CORNER_NODE) :
  {

    /* identification of cornernodes is done */
    /* in Identify_SonNodes()     */
    return;

    PRINTDEBUG(dddif,1,("%d: Identify CORNERNODE gid=%08x node=%d\n",
                        me, DDD_InfoGlobalId(PARHDR(theNode)), node));

    IdentObjectHdr[nobject++] = PARHDR(theNode);

    /* identify to proclist of node */
    const auto& proclist = DDD_InfoProcListRange(context, PARHDR((NODE *)NFATHER(theNode)), false);

    /* identify using father node */
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(theNode));

    Ident_FctPtr(context, IdentObjectHdr, nobject,
                 proclist, PrioHGhost, IdentHdr, nident);

    break;
  }

  case (MID_NODE) :
  {
                        #ifdef UG_DIM_2
    NODE **EdgeNodes = Nodes;
                        #endif

    EDGE *theEdge;
                        #ifdef UG_DIM_3
    NODE *EdgeNodes[MAX_SIDE_NODES];

    /* identification of cornernodes is done */
    /* in Identify_SonEdges()     */
    return;

    EdgeNodes[0] = Nodes[node-ncorners];
    EdgeNodes[1] = Nodes[(node-ncorners+1)%ncorners];
    EdgeNodes[2] = theNode;
                        #endif

    ASSERT(EdgeNodes[0]!=NULL);
    ASSERT(EdgeNodes[1]!=NULL);
    ASSERT(EdgeNodes[2]!=NULL);

    PRINTDEBUG(dddif,1,("%d: Identify MIDNODE gid=%08x node=%d\n",
                        me, DDD_InfoGlobalId(PARHDR(theNode)), node));

    /* identify midnode, vertex, vector */
    IdentObjectHdr[nobject++] = PARHDR(theNode);
    IdentObjectHdr[nobject++] = PARHDRV(MYVERTEX(theNode));

                        #ifdef UG_DIM_2
    if (!NEW_NIDENT(theNode)) break;
                        #endif

    /* Identify to proclist of edge */
    theEdge = GetEdge((NODE *)NFATHER(EdgeNodes[0]),
                      (NODE *)NFATHER(EdgeNodes[1]));
    ASSERT(theEdge!=NULL);

    const auto& proclist = DDD_InfoProcListRange(context, PARHDR(theEdge), false);

    /* identify using edge nodes */
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(EdgeNodes[0]));
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(EdgeNodes[1]));

    /* this is the buggy case
                            IdentHdr[nident++] = PARHDR(EdgeNodes[0]);
                            IdentHdr[nident++] = PARHDR(EdgeNodes[1]);
     */

    Ident_FctPtr(context, IdentObjectHdr, nobject,
                 proclist, PrioHGhost, IdentHdr, nident);

    break;
  }

                #ifdef UG_DIM_3
  case (SIDE_NODE) :
  {
    PRINTDEBUG(dddif,1,("%d: Identify SIDENODE gid=%08x node=%d\n",
                        me,DDD_InfoGlobalId(PARHDR(theNode)),node));

    /* identify sidenode and vertex */
    IdentObjectHdr[nobject++] = PARHDR(theNode);
    IdentObjectHdr[nobject++] = PARHDRV(MYVERTEX(theNode));

    /* identify to proclist of neighbor element */
    const auto& proclist = DDD_InfoProcListRange(context, PARHDRE(theNeighbor), false);

    /* identify using corner nodes of side */
    for (INT i=0; i<ncorners; i++)
      IdentHdr[nident++] = PARHDR((NODE *)NFATHER(Nodes[i]));

    /* identify side node */
    Ident_FctPtr(context, IdentObjectHdr, nobject,
                 proclist, PrioHGhost, IdentHdr, nident);

    break;
  }
                #endif
  default :
    ASSERT(0);
    break;
  }

        #ifdef Debug
  if (debug == 1) {
    SETNIDENT(theNode,CLEAR);
  }
  else
        #endif
  /* lock this node for identification */
  SETNIDENT(theNode,IDENT);
}



/****************************************************************************/
/*
   IdentifySideEdge - idenify edge shared only between two neighbor elements

   SYNOPSIS:
   static INT IdentifySideEdge (GRID *theGrid, EDGE *theEdge, ELEMENT *theElement, ELEMENT *theNeighbor, INT Vec);

   PARAMETERS:
   .  theGrid
   .  theNeighbor
   .  theEdge
   .  Vec

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT IdentifySideEdge (GRID *theGrid, EDGE *theEdge, ELEMENT *theElement, ELEMENT *theNeighbor, INT Vec)
{
  INT nobject = 0, nident = 0;
  DDD_HDR IdentObjectHdr[MAX_OBJECT];
  DDD_HDR IdentHdr[MAX_TOKEN];
  auto& context = theGrid->dddContext();

        #ifdef UG_DIM_2
  /* no identfication to nonrefined neighbors */
  if (MARK(theNeighbor) == NO_REFINEMENT) return(0);
        #endif

        #ifdef UG_DIM_3
  /* identification of sonedges is done in Identify_SonEdges() */
  {
    EDGE* FatherEdge = GetFatherEdge(theEdge);
    if (FatherEdge != NULL) return(0);
  }
        #endif

  /* only newly created edges are identified */
  if (!NEW_EDIDENT(theEdge)) return(0);

  /* edge unlocked -> no debugging occurs */
        #ifdef Debug
  if (debug == 1) {
    if (EDIDENT(theEdge) == CLEAR) return(0);
  }
  else
        #endif

  /* edge locked -> already identified */
  if (EDIDENT(theEdge) == IDENT) return(0);

        #ifdef UG_DIM_3
  IdentObjectHdr[nobject++] = PARHDR(theEdge);
        #endif

  /* identify to proclist of neighbor */
  const auto& proclist = DDD_InfoProcListRange(context, PARHDRE(theNeighbor), false);

  /* now choose identificator objects */
  NODE* theNode0 = NBNODE(LINK0(theEdge));
  NODE* theNode1 = NBNODE(LINK1(theEdge));
  ASSERT(!CENTERTYPE(theNode0));
  ASSERT(!CENTERTYPE(theNode1));

  if (CORNERTYPE(theNode0))
  {
    ASSERT(NFATHER(theNode0)!=NULL);
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(theNode0));
  }
        #ifdef UG_DIM_3
  /* since midnodes are identified later in Debug case */
  /* choose fatheredge here (s.l. 980227)              */
  else if (MIDTYPE(theNode0))
  {
    ASSERT(NFATHER(theNode0)!=NULL);
    IdentHdr[nident++] = PARHDR((EDGE *)NFATHER(theNode0));
  }
        #endif
  else
  {
    /* side node */
                #ifdef UG_DIM_3
    ASSERT(SIDETYPE(theNode0));
                #endif
    IdentHdr[nident++] = PARHDR(theNode0);
  }

  if (CORNERTYPE(theNode1))
  {
    ASSERT(NFATHER(theNode1)!=NULL);
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(theNode1));
  }
        #ifdef UG_DIM_3
  /* since midnodes are identified later in Debug case */
  /* choose fatheredge here (s.l. 980227)              */
  else if (MIDTYPE(theNode1))
  {
    ASSERT(NFATHER(theNode1)!=NULL);
    IdentHdr[nident++] = PARHDR((EDGE *)NFATHER(theNode1));
  }
        #endif
  else
  {
    /* side node */
                #ifdef UG_DIM_3
    ASSERT(SIDETYPE(theNode1));
                #endif
    IdentHdr[nident++] = PARHDR(theNode1);
  }

  if (nobject > 0)
    Ident_FctPtr(context, IdentObjectHdr, nobject,
                 proclist, PrioHGhost, IdentHdr, nident);

  /* debugging unlocks the edge */
        #ifdef Debug
  if (debug == 1) {
    SETEDIDENT(theEdge,CLEAR);
  }
  else
        #endif
  /* lock this edge for identification */
  SETEDIDENT(theEdge,IDENT);

  return 0;
}

/****************************************************************************/
/*
   IdentifyEdge -

   SYNOPSIS:
   static INT IdentifyEdge (ELEMENT *theElement, ELEMENT *theNeighbor,
                        NODE **SideNodes, INT ncorners, ELEMENT *Son, INT SonSide, INT edgeofside, INT Vec);

   PARAMETERS:
   .  theElement
   .  theNeighbor
   .  SideNodes
   .  ncorners
   .  Son
   .  SonSide
   .  edgeofside
   .  Vec

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT IdentifyEdge (GRID *theGrid,
                         ELEMENT *theElement, ELEMENT *theNeighbor,
                         NODE **SideNodes, INT ncorners, ELEMENT *Son,
                         INT SonSide, INT edgeofside, INT Vec)
{
  NODE *Nodes[2];
  EDGE *theEdge;
  INT nobject,nident;
        #ifdef UG_DIM_3
  INT edge,corner0,corner1;
        #endif
  DDD_HDR IdentObjectHdr[MAX_OBJECT];
  DDD_HDR IdentHdr[MAX_TOKEN];
  auto& context = theGrid->dddContext();

  nobject = nident = 0;

        #ifdef UG_DIM_2
  Nodes[0] = CORNER(Son,CORNER_OF_EDGE(Son,SonSide,0));
  Nodes[1] = CORNER(Son,CORNER_OF_EDGE(Son,SonSide,1));
        #endif

        #ifdef UG_DIM_3
  edge = EDGE_OF_SIDE(Son,SonSide,edgeofside);
  corner0 = CORNER_OF_EDGE(Son,edge,0);
  corner1 = CORNER_OF_EDGE(Son,edge,1);

  Nodes[0] = CORNER(Son,corner0);
  Nodes[1] = CORNER(Son,corner1);
  PRINTDEBUG(dddif,5,("%4d: edge=%d corner0=%d corner1=%d Nodes[0]=%d "
                      "Nodes[1]=%d\n",
                      me,edge, corner0, corner1, ID(Nodes[0]), ID(Nodes[1])));
        #endif

  ASSERT(Nodes[0]!=NULL);
  ASSERT(Nodes[1]!=NULL);

  theEdge = GetEdge(Nodes[0],Nodes[1]);
  ASSERT(theEdge!=NULL);

        #ifdef UG_DIM_2
  /* no identfication to nonrefined neighbors */
  if (MARK(theNeighbor) == NO_REFINEMENT) return(0);
        #endif

        #ifdef UG_DIM_3
  /* identification of sonedges is done in Identify_SonEdges() */
  if (0)
    if (CORNERTYPE(Nodes[0]) && CORNERTYPE(Nodes[1]))
    {
      [[maybe_unused]] EDGE *FatherEdge;
      FatherEdge = GetEdge((NODE *)NFATHER(Nodes[0]),(NODE *)NFATHER(Nodes[1]));
      ASSERT(FatherEdge != NULL);
      return(0);
      /*
              if (FatherEdge != NULL) return(0);
       */
    }
  {
    EDGE *FatherEdge;
    FatherEdge = GetFatherEdge(theEdge);
    if (FatherEdge != NULL) return(0);
  }
        #endif

  /* only newly created edges are identified */
  if (!NEW_EDIDENT(theEdge)) return(0);

  /* edge unlocked -> no debugging occurs */
        #ifdef Debug
  if (debug == 1) {
    if (EDIDENT(theEdge) == CLEAR) return(0);
  }
  else
        #endif

  /* edge locked -> already identified */
  if (EDIDENT(theEdge) == IDENT) return(0);

  PRINTDEBUG(dddif,1,("%d: Identify EDGE edgeofside=%d pe=%08x/%x eID=%d"
                      " ntype0=%d  ntype1=%d Vec=%d\n",me,edgeofside,
                      DDD_InfoGlobalId(PARHDRE(Son)),Son,ID(Son),
                      NTYPE(Nodes[0]), NTYPE(Nodes[1]), Vec))

        #ifdef UG_DIM_3
  IdentObjectHdr[nobject++] = PARHDR(theEdge);
        #endif

        #ifdef UG_DIM_2
  /* identify to proclist of neighbor */
  const auto& proclist = DDD_InfoProcListRange(context, PARHDRE(theNeighbor), false);
        #endif

  /* identify to proclist of father edge or neighbor*/
        #ifdef UG_DIM_3
  DDD_HDR hdr;
  if (0)
  {
    EDGE *fatherEdge = NULL;

    /* check whether edge inside the side of the element */
    fatherEdge = FatherEdge(SideNodes,ncorners,Nodes,theEdge);

    if (fatherEdge != NULL)
      hdr = PARHDR(fatherEdge);
    else
      hdr = PARHDRE(theNeighbor);
  }
  hdr = PARHDRE(theNeighbor);
  const auto& proclist = DDD_InfoProcListRange(context, hdr, false);
        #endif

  if (CORNERTYPE(Nodes[0]))
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(Nodes[0]));
        #ifdef UG_DIM_3
  /* since midnodes are identified later in Debug case */
  /* choose fatheredge here (s.l. 980227)              */
  else if (MIDTYPE(Nodes[0]))
    IdentHdr[nident++] = PARHDR((EDGE *)NFATHER(Nodes[0]));
        #endif
  else
    IdentHdr[nident++] = PARHDR(Nodes[0]);

  if (CORNERTYPE(Nodes[1]))
    IdentHdr[nident++] = PARHDR((NODE *)NFATHER(Nodes[1]));
        #ifdef UG_DIM_3
  /* since midnodes are identified later in Debug case */
  /* choose fatheredge here (s.l. 980227)              */
  else if (MIDTYPE(Nodes[1]))
    IdentHdr[nident++] = PARHDR((EDGE *)NFATHER(Nodes[1]));
        #endif
  else
    IdentHdr[nident++] = PARHDR(Nodes[1]);

  if (nobject > 0)
    Ident_FctPtr(context, IdentObjectHdr, nobject,
                 proclist, PrioHGhost, IdentHdr, nident);

  /* debugging unlocks the edge */
        #ifdef Debug
  if (debug == 1) {
    SETEDIDENT(theEdge,CLEAR);
  }
  else
        #endif
  /* lock this edge for identification */
  SETEDIDENT(theEdge,IDENT);

  return 0;
}


/****************************************************************************/
/*
   IdentifyObjectsOfElementSide -

   SYNOPSIS:
   static INT IdentifyObjectsOfElementSide(GRID *theGrid, ELEMENT *theElement,
                                                INT i, ELEMENT *theNeighbor);

   PARAMETERS:
   .  theGrid
   .  theElement
   .  i
   .  theNeighbor

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT IdentifyObjectsOfElementSide(GRID *theGrid, ELEMENT *theElement,
                                        INT i, ELEMENT *theNeighbor)
{
  INT nodes, j;
#ifdef Debug
  INT n = 0;
#endif
  NODE *SideNodes[MAX_SIDE_NODES];
  INT ncorners;
  NODE *theNode;

  GetSonSideNodes(theElement,i,&nodes,SideNodes,0);
  ncorners = CORNERS_OF_SIDE(theElement,i);

  PRINTDEBUG(dddif,1,("%d: IdentifyObjectsOfElementSide():identify NODES "
                      "ncorners=%d nodes=%d\n",me,ncorners,nodes));

  /* identify nodes, vertices and node vectors of son elements */
  for (j=0; j<MAX_SIDE_NODES; j++)
  {
    theNode = SideNodes[j];
    if (theNode == NULL) continue;

     // identify new node including its vertex
     IdentifyNode(theGrid,theNeighbor, theNode, SideNodes, j, ncorners);
#ifdef Debug
     n++;
#endif
  }
  ASSERT(n == nodes);

  /* identify edge vectors (2D); edges, edge and side vectors (3D) */
  if (/*VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC) ||*/ DIM==3)
  {
    ELEMENT *SonList[MAX_SONS];
    INT SonsOfSide,SonSides[MAX_SONS];
    INT j;

    PRINTDEBUG(dddif,1,("%d: IdentifyObjectsOfElementSide(): identify "
                        "EDGES and VECTORS\n",me));

    if (Get_Sons_of_ElementSide(theElement,i,&SonsOfSide,
                                SonList,SonSides,1,0)!=GM_OK)
      RETURN(GM_FATAL);

    for (j=0; j<SonsOfSide; j++) {

      if (/*VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC) ||*/ DIM==3)
      {
        INT edgeofside;
        INT nedges = EDGES_OF_SIDE(SonList[j],SonSides[j]);

        /* identify the edge and vector */
        for (edgeofside=0; edgeofside<nedges; edgeofside++) {
          EDGE *theEdge;
          INT edge = EDGE_OF_SIDE(SonList[j],SonSides[j],edgeofside);

          theEdge = GetEdge(CORNER_OF_EDGE_PTR(SonList[j],edge,0),
                            CORNER_OF_EDGE_PTR(SonList[j],edge,1));
          ASSERT(theEdge!=NULL);

//          IdentifySideEdge(theGrid, theEdge, theElement, theNeighbor,
//                           VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC));
          IdentifySideEdge(theGrid, theEdge, theElement, theNeighbor, 0);
        }
      }

                        #ifdef UG_DIM_3
      if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
      {
        auto& context = theGrid->dddContext();
        IdentifySideVector(context, theElement,theNeighbor,SonList[j],SonSides[j]);
      }
                        #endif
    }
  }

  return GM_OK;
}



/****************************************************************************/
/*
   IdentifyDistributedObjects -

   SYNOPSIS:
   INT	IdentifyDistributedObjects (MULTIGRID *theMG, INT FromLevel, INT ToLevel);

   PARAMETERS:
   .  theMG
   .  FromLevel
   .  ToLevel

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT IdentifyDistributedObjects (MULTIGRID *theMG, INT FromLevel, INT ToLevel)
{
  DDD_PRIO prio;

  PRINTDEBUG(dddif,1,("%d: IdentifyDistributedObjects(): FromLevel=%d "
                      "ToLevel=%d\n",me,FromLevel,ToLevel));

  /* identify distributed objects */
  for (INT l=FromLevel; l<ToLevel; l++)
  {
    PRINTDEBUG(dddif,1,("%d: IdentifyDistributedObjects(): identification "
                        "level=%d\n",me,l));

    GRID* theGrid = GRID_ON_LEVEL(theMG,l);

                #ifdef Debug
    identlevel = l;
                #endif

    /* check control word flags for ident on upper level */
                #ifdef Debug
    if (debug != 1)
                #endif
    ResetIdentFlags(GRID_ON_LEVEL(theMG,l+1));

    for (ELEMENT* theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      prio = EPRIO(theElement);

      if (!IS_REFINED(theElement) || EGHOSTPRIO(prio)) continue;

      for (INT i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        ELEMENT* theNeighbor = NBELEM(theElement,i);
        if (theNeighbor == NULL) continue;

        /* TODO: change for full dynamic element distribution */
        prio = EPRIO(theNeighbor);
        if (!HGHOSTPRIO(prio) || NSONS(theNeighbor)!=0) continue;

        PRINTDEBUG(dddif,1,("%d: Identify element: pe=%08x/%x eID=%d "
                            "side=%d\n",me,DDD_InfoGlobalId(PARHDRE(theElement)),
                            theElement,ID(theElement),i));

        IdentifyObjectsOfElementSide(theGrid,theElement,i,theNeighbor);

      }
    }
  }

  return GM_OK;
}


#ifdef IDENT_ONLY_NEW
static int Gather_NewNodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE *theNode = (NODE *)obj;
  NODE *SonNode = SONNODE(theNode);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  if (SonNode!=NULL && NEW_NIDENT(SonNode))
  {
    IFDEBUG(dddif,1)
    UserWriteF("new son node=" ID_FMTX  "node=" ID_FMTX "\n",
               ID_PRTX(SonNode),ID_PRTX(theNode));
    ENDDEBUG
    *((int *)data) = 1;
  }
  else
    *((int *)data) = 0;

  return 0;
}

static int Scatter_NewNodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode        = (NODE *)obj;
  NODE    *SonNode        = SONNODE(theNode);
  int has_newsonnode  = *((int *)data);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  if (SonNode!=NULL && has_newsonnode) SETNEW_NIDENT(SonNode,1);

  return 0;
}

/*************************/

static int Gather_NodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE *theNode = (NODE *)obj;

  ASSERT(identlevel == LEVEL(theNode));

  if (!(NTYPE(theNode)==check_nodetype))
  {
    *((int *)data) = 0;
    return 0;
  }

  *((int *)data) = NEW_NIDENT(theNode);

  return 0;
}

static int Scatter_NodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode = (NODE *)obj;
  int nprop    = *((int *)data);

  ASSERT(identlevel == LEVEL(theNode));

  if (!(NTYPE(theNode)==check_nodetype)) return 0;

  if (NIDENTASSERT) if (NEW_NIDENT(theNode)) assert(NFATHER(theNode) != NULL);

  if (nprop)
  {
    if (0) SETNEW_NIDENT(theNode,1);
    if (NFATHER(theNode) == NULL)
    {
      UserWriteF("isolated node=" ID_FMTX "\n",
                 ID_PRTX(theNode));
      if (NIDENTASSERT) assert(0);
    }
    if (NIDENTASSERT) assert(NFATHER(theNode) != NULL);
  }

  return 0;
}

/*************************/

static int Gather_TestNodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE *theNode = (NODE *)obj;

  ASSERT(identlevel == LEVEL(theNode));

  ((int *)data)[0] = NEW_NIDENT(theNode);
  if (NEW_NIDENT(theNode)) assert(NFATHER(theNode) != NULL);

  return 0;
}

static int Scatter_TestNodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode        = (NODE *)obj;
  int nprop   = *((int *)data);

  ASSERT(identlevel == LEVEL(theNode));

  if (NEW_NIDENT(theNode) != nprop)
  {
    UserWriteF("nprop wrong mynprop=%d hisnprop=%d theNode=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d\n",
               NEW_NIDENT(theNode),nprop,ID_PRTX(theNode),LEVEL(theNode),proc,prio);
    fflush(stdout);
    assert(0);
  }

  return 0;
}

/*************************/

static int Gather_IdentSonNode (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE *theNode = (NODE *)obj;
  NODE *SonNode = SONNODE(theNode);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  ((int *)data)[0] = 0;
  ((int *)data)[1] = 0;

  if (SonNode != NULL)
  {
    ((int *)data)[0] = 1;
    ((int *)data)[1] = NEW_NIDENT(SonNode);
  }

  return 0;
}

static int Scatter_IdentSonNode (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode        = (NODE *)obj;
  NODE    *SonNode        = SONNODE(theNode);
  int sonnode         = ((int *)data)[0];
  int newsonnode      = ((int *)data)[1];

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  if (SonNode!=NULL)
  {
    /*
            if (1 || NEW_NIDENT(SonNode))
     */
    if (NEW_NIDENT(SonNode))
    {
      if(sonnode)
      {
        if (!newsonnode)
        {
          UserWriteF("theNode=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d sonnprop=%d\n",
                     ID_PRTX(theNode),LEVEL(theNode),proc,prio,NEW_NIDENT(SonNode));
          fflush(stdout);
          assert(0);
        }

        DDD_IdentifyObject(context, PARHDR(SonNode),proc,PARHDR(theNode));
      }
    }
    else
    {
      if (newsonnode)
      {
        UserWriteF("theNode=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d sonnprop=%d\n",
                   ID_PRTX(theNode),LEVEL(theNode),proc,prio,NEW_NIDENT(SonNode));
        fflush(stdout);
        assert(0);
      }
    }
  }

  return 0;
}

/* callback functions for edge identification */
static int Gather_NewObjectInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  INT ident_needed;
  EDGE *theEdge   = (EDGE *)obj;
  EDGE *SonEdges[MAX_SON_EDGES];
  NODE *MidNode   = MIDNODE(theEdge);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  *((int *)data) = 0;

  GetSonEdges(theEdge,SonEdges);

  /* identification is done if one objects of MidNode and the one */
  /* or two sonedge have NEW_XXIDENT flags set.                   */
  ident_needed = ((MidNode!=NULL && NEW_NIDENT(MidNode)) ||
                  (SonEdges[0]!=NULL && NEW_EDIDENT(SonEdges[0])) ||
                  (SonEdges[1]!=NULL && NEW_EDIDENT(SonEdges[1])));

  if (ident_needed)
  {
    /* send number of objects that need identification */
    /* must be equal on all procs                      */
    if (MidNode!=NULL && NEW_NIDENT(MidNode)) ((int *)data)[0] = 1;
    if (SonEdges[0]!=NULL && NEW_EDIDENT(SonEdges[0])) ((int *)data)[0] += 2;
    if (SonEdges[1]!=NULL && NEW_EDIDENT(SonEdges[1])) ((int *)data)[0] += 4;
  }

  return 0;
}

static int Scatter_NewObjectInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  int newsonobjects   = *((int *)data);
  EDGE    *theEdge                = (EDGE *)obj;
  EDGE    *SonEdges[MAX_SON_EDGES];
  NODE    *MidNode        = MIDNODE(theEdge);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  GetSonEdges(theEdge,SonEdges);

  if (newsonobjects)
  {
    if (MidNode == NULL)
    {
      if (SonEdges[0]!=NULL)
        if (newsonobjects & 0x2) SETNEW_EDIDENT(SonEdges[0],1);
    }
    else
    {
      if (newsonobjects & 0x1)
        SETNEW_NIDENT(MidNode,1);
      if (SonEdges[0]!=NULL)
        if (newsonobjects & 0x2)
          SETNEW_EDIDENT(SonEdges[0],1);
      if (SonEdges[1]!=NULL)
        if (newsonobjects & 0x4)
          SETNEW_EDIDENT(SonEdges[1],1);
    }
  }

  return 0;
}

/*************************/

static int Gather_EdgeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE *theEdge = (EDGE *)obj;

  ASSERT(identlevel == LEVEL(theEdge));

  if (GetFatherEdge(theEdge) == NULL)
  {
    *((int *)data) = 0;
    return 0;
  }

  *((int *)data) = NEW_EDIDENT(theEdge);

  return 0;
}

static int Scatter_EdgeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE    *theEdge = (EDGE *)obj;
  int nprop    = *((int *)data);
  NODE *theNode0 = NBNODE(LINK0(theEdge));
  NODE *theNode1 = NBNODE(LINK1(theEdge));

  ASSERT(identlevel == LEVEL(theEdge));

  if (!CORNERTYPE(theNode0) && !CORNERTYPE(theNode1)) return(0);

  if (nprop)
  {
    if (0) SETNEW_EDIDENT(theEdge,1);
    if (GetFatherEdge(theEdge) == NULL)
    {
      UserWriteF("isolated edge=" ID_FMTX "\n",
                 ID_PRTX(theEdge));
      if (EDIDENTASSERT) assert(0);
    }
    if (EDIDENTASSERT) assert(GetFatherEdge(theEdge) != NULL);
  }

  return 0;
}

/*************************/

static int Gather_TestEdgeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE *theEdge = (EDGE *)obj;

  ASSERT(identlevel == LEVEL(theEdge));

  ((int *)data)[0] = NEW_EDIDENT(theEdge);
  if (NEW_EDIDENT(theEdge)) assert(GetFatherEdge(theEdge) != NULL);

  return 0;
}

static int Scatter_TestEdgeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE    *theEdge        = (EDGE *)obj;
  int nprop   = *((int *)data);

  ASSERT(identlevel == LEVEL(theEdge));

  if (NEW_EDIDENT(theEdge) != nprop)
  {
    UserWriteF("nprop wrong mynprop=%d hisnprop=%d theEdge=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d\n",
               NEW_EDIDENT(theEdge),nprop,ID_PRTX(theEdge),LEVEL(theEdge),proc,prio);
    fflush(stdout);
    assert(0);
  }

  return 0;
}

/*************************/

static int Gather_IdentSonEdge (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE *theEdge = (EDGE *)obj;

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  ((int *)data)[0] = 0;
  ((int *)data)[1] = 0;

  EDGE *SonEdge = GetSonEdge(theEdge);
  if (SonEdge)
  {
    ((int *)data)[0] = 1;
    ((int *)data)[1] = NEW_EDIDENT(SonEdge);
  }

  return 0;
}

static int Scatter_IdentSonEdge (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE    *theEdge        = (EDGE *)obj;
  int sonedge         = ((int *)data)[0];
  int newsonedge      = ((int *)data)[1];

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  EDGE *SonEdge = GetSonEdge(theEdge);
  if (SonEdge)
  {
    /*
            if (1 || NEW_EDIDENT(SonEdge))
     */
    if (NEW_EDIDENT(SonEdge))
    {
      if(sonedge)
      {
        if (!newsonedge)
        {
          UserWriteF("theEdge=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d sonnprop=%d\n",
                     ID_PRTX(theEdge),LEVEL(theEdge),proc,prio,NEW_EDIDENT(SonEdge));
          fflush(stdout);
          assert(0);
        }

        DDD_IdentifyObject(context, PARHDR(SonEdge),proc,PARHDR(theEdge));
        if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdge)!=NULL)
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdge)),proc,PARHDR(theEdge));
      }
    }
    else
    {
      if (newsonedge)
      {
        UserWriteF("theEdge=" ID_FMTX " LEVEL=%d PROC=%d PRIO=%d sonnprop=%d\n",
                   ID_PRTX(theEdge),LEVEL(theEdge),proc,prio,NEW_EDIDENT(SonEdge));
        fflush(stdout);
        assert(0);
      }
    }
  }

  return 0;
}

/*************************/

static int Gather_IdentSonObjects (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  INT ident_needed;
  EDGE *theEdge   = (EDGE *)obj;
  EDGE *SonEdges[2];
  NODE *MidNode   = MIDNODE(theEdge);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  ((int *)data)[0] = 0;

  GetSonEdges(theEdge, SonEdges);

  /* identification is done if one objects of MidNode and the one */
  /* or two sonedge have NEW_XXIDENT flags set.                   */
  ident_needed = ((MidNode!=NULL && NEW_NIDENT(MidNode)) ||
                  (SonEdges[0]!=NULL && NEW_EDIDENT(SonEdges[0])) ||
                  (SonEdges[1]!=NULL && NEW_EDIDENT(SonEdges[1])));

  if (ident_needed)
  {
    /* send number of objects that need identification */
    /* must be equal on all procs                      */
    if (MidNode!=NULL && NEW_NIDENT(MidNode)) ((int *)data)[0] = 1;
    if (SonEdges[0]!=NULL && NEW_EDIDENT(SonEdges[0])) ((int *)data)[0] += 2;
    if (SonEdges[1]!=NULL && NEW_EDIDENT(SonEdges[1])) ((int *)data)[0] += 4;
  }

  return 0;
}

static int Scatter_IdentSonObjects (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  int newsonobjects   = *((int *)data);
  EDGE    *theEdge                = (EDGE *)obj;
  EDGE    *SonEdges[2];
  NODE    *MidNode        = MIDNODE(theEdge);
  NODE    *SonNode0,*SonNode1,*IdentNode;
  NODE    *Node0,*Node1;

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  [[maybe_unused]] INT nedges = GetSonEdges(theEdge,SonEdges);

  if (newsonobjects)
  {
    if (0)
      ASSERT((MidNode==NULL && nedges!=2)  ||
             (MidNode!=NULL && nedges==2));

    if (MidNode==NULL)
    {
      if (SonEdges[0]!=NULL &&  NEW_EDIDENT(SonEdges[0]))
      {
        ASSERT(newsonobjects & 0x2);

        DDD_IdentifyObject(context, PARHDR(SonEdges[0]),proc,PARHDR(theEdge));
        if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdges[0])!=NULL)
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdges[0])),proc,PARHDR(theEdge));
      }
    }
    else
    {

      /* identify midnode */
      if (MidNode!=NULL && NEW_NIDENT(MidNode))
      {
        ASSERT(newsonobjects & 0x1);
        if (0) {
          ASSERT(MidNode!=NULL && SonEdges[0]!=NULL && SonEdges[1]!=NULL);
          ASSERT(nedges==2);
        }

        if (1)
        {
          DDD_IdentifyObject(context, PARHDR(MidNode),proc,PARHDR(theEdge));
          DDD_IdentifyObject(context, PARHDRV(MYVERTEX(MidNode)),proc,PARHDR(theEdge));
        }
        else
        {
          Node0 = NBNODE(LINK0(theEdge));
          Node1 = NBNODE(LINK1(theEdge));
          DDD_IdentifyObject(context, PARHDR(MidNode),proc,PARHDR(Node0));
          DDD_IdentifyObject(context, PARHDR(MidNode),proc,PARHDR(Node1));
          DDD_IdentifyObject(context, PARHDRV(MYVERTEX(MidNode)),proc,PARHDR(Node0));
          DDD_IdentifyObject(context, PARHDRV(MYVERTEX(MidNode)),proc,PARHDR(Node1));
        }
      }

      if (SonEdges[0]!=NULL &&  NEW_EDIDENT(SonEdges[0]) && (newsonobjects & 0x2))
      {
        if (0) {
          ASSERT(MidNode!=NULL && SonEdges[0]!=NULL && SonEdges[1]!=NULL);
          ASSERT(nedges==2);
        }
        /* identify edge0 */
        SonNode0 = NBNODE(LINK0(SonEdges[0]));
        SonNode1 = NBNODE(LINK1(SonEdges[0]));
        if (CORNERTYPE(SonNode0))
        {
          ASSERT(NFATHER(SonNode0)!=NULL);
          IdentNode = SonNode0;
        }
        else
        {
          ASSERT(CORNERTYPE(SonNode1));
          ASSERT(NFATHER(SonNode1)!=NULL);
          IdentNode = SonNode1;
        }
        DDD_IdentifyObject(context, PARHDR(SonEdges[0]),proc,PARHDR(theEdge));
        DDD_IdentifyObject(context, PARHDR(SonEdges[0]),proc,PARHDR((NODE *)NFATHER(IdentNode)));
        if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdges[0])!=NULL)
        {
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdges[0])),proc,PARHDR(theEdge));
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdges[0])),proc,PARHDR((NODE *)NFATHER(IdentNode)));
        }
      }

      if (SonEdges[1]!=NULL &&  NEW_EDIDENT(SonEdges[1]) && (newsonobjects & 0x4))
      {
        if (0) {
          ASSERT(MidNode!=NULL && SonEdges[0]!=NULL && SonEdges[1]!=NULL);
          ASSERT(nedges==2);
        }
        /* identify edge1 */
        SonNode0 = NBNODE(LINK0(SonEdges[1]));
        SonNode1 = NBNODE(LINK1(SonEdges[1]));
        if (CORNERTYPE(SonNode0))
        {
          ASSERT(NFATHER(SonNode0)!=NULL);
          IdentNode = SonNode0;
        }
        else
        {
          ASSERT(CORNERTYPE(SonNode1));
          ASSERT(NFATHER(SonNode1)!=NULL);
          IdentNode = SonNode1;
        }
        DDD_IdentifyObject(context, PARHDR(SonEdges[1]),proc,PARHDR(theEdge));
        DDD_IdentifyObject(context, PARHDR(SonEdges[1]),proc,PARHDR((NODE *)NFATHER(IdentNode)));
        if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdges[1])!=NULL)
        {
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdges[1])),proc,PARHDR(theEdge));
          DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdges[1])),proc,PARHDR((NODE *)NFATHER(IdentNode)));
        }
      }
    }
  }

  return 0;
}

#endif



#ifndef IDENT_ONLY_NEW

/****************************************************************************/
/*
   Gather_SonNodeInfo -

   SYNOPSIS:
   static int Gather_SonNodeInfo (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Gather_SonNodeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE *theNode = (NODE *)obj;

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  if (SONNODE(theNode) != NULL)
    *((int *)data) = 1;
  else
    *((int *)data) = 0;

  return 0;
}

/****************************************************************************/
/*
   Scatter_SonNodeInfo -

   SYNOPSIS:
   static int Scatter_SonNodeInfo (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Scatter_SonNodeInfo (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode        = (NODE *)obj;
  NODE    *SonNode        = SONNODE(theNode);
  INT has_sonnode     = *((int *)data);

  /* identification is only done between master objects */
  ASSERT(identlevel-1 == LEVEL(theNode));

  if (SonNode != NULL)
  {
    if (has_sonnode)
    {
      DDD_IdentifyObject(context, PARHDR(SonNode),proc,PARHDR(theNode));
      if (ddd_ctrl(context).nodeData && NVECTOR(SonNode)!=NULL)
        DDD_IdentifyObject(context, PARHDR(NVECTOR(SonNode)),proc,PARHDR(theNode));
      IFDEBUG(dddif,1)
      if (ddd_ctrl(context).nodeData && NVECTOR(SonNode)!=NULL)
        PrintDebug ("l=%d IdentHdr: %d Proc: %d me:%d IdentObjectHdr: %d %d\n",
                    identlevel,GID(theNode),proc,me,GID(SonNode),GID(EDVECTOR(SonNode)));
      else
        PrintDebug ("l=%d IdentHdr: %d Proc: %d me:%d IdentObjectHdr: %d\n",
                    identlevel,GID(theNode),proc,me,GID(SonNode));
      ENDDEBUG
    }
  }

  return 0;
}


#ifdef UG_DIM_3

/****************************************************************************/
/*
   Gather_SonEdgeInfo  -

   SYNOPSIS:
   static int Gather_SonEdgeInfo (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Gather_SonEdgeInfo (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE *theEdge = (EDGE *)obj;

  /* identification has to be done between all copies of an objects */
  /* otherwise this can result in unsymmetric interfaces            */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  EDGE *SonEdge = GetSonEdge(theEdge);
  if (SonEdge)
    *((int *)data) = 1;
  else
    *((int *)data) = 0;

  return 0;
}


/****************************************************************************/
/*
   Scatter_SonEdgeInfo -

   SYNOPSIS:
   static int Scatter_SonEdgeInfo (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Scatter_SonEdgeInfo (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  EDGE    *theEdge        = (EDGE *)obj;
  INT has_sonedge     = *((int *)data);

  /* identification has to be done between all copies of an objects */
  /* otherwise this can result in unsymmetric interfaces            */
  ASSERT(identlevel-1 == LEVEL(theEdge));

  EDGE *SonEdge = GetSonEdge(theEdge);
  if (SonEdge)
  {
    if (has_sonedge)
    {
      DDD_IdentifyObject(context, PARHDR(SonEdge),proc,PARHDR(theEdge));
      if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdge)!=NULL)
        DDD_IdentifyObject(context, PARHDR(EDVECTOR(SonEdge)),proc,PARHDR(theEdge));
      IFDEBUG(dddif,1)
      if (ddd_ctrl(context).edgeData && EDVECTOR(SonEdge)!=NULL)
        PrintDebug ("l=%d IdentHdr: %d Proc: %d me:%d IdentObjectHdr: %d %d\n",
                    identlevel,GID(theEdge),proc,me,GID(SonEdge),GID(EDVECTOR(SonEdge)));
      else
        PrintDebug ("l=%d IdentHdr: %d Proc: %d me:%d IdentObjectHdr: %d\n",
                    identlevel,GID(theEdge),proc,me,GID(SonEdge));
      ENDDEBUG
    }
  }

  return 0;
}
#endif
#endif

/****************************************************************************/
/*
   Identify_SonNodes - identify son nodes (type CORNERNODE)

   SYNOPSIS:
   INT Identify_SonNodes (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT Identify_SonNodes (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

#ifdef IDENT_ONLY_NEW
  DDD_IFAOnewayX(context,
                 dddctrl.NodeAllIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(int),
                 Gather_NewNodeInfo,Scatter_NewNodeInfo);

  if (UPGRID(theGrid) != NULL)
  {
    check_nodetype = CORNER_NODE;
    if (NIDENTASSERT)
      DDD_IFAOnewayX(context,
                     dddctrl.NodeAllIF,GRID_ATTR(UPGRID(theGrid)),IF_FORWARD,sizeof(int),
                     Gather_NodeInfo,Scatter_NodeInfo);
    if (0)
      DDD_IFAOnewayX(context,
                     dddctrl.NodeAllIF,GRID_ATTR(UPGRID(theGrid)),IF_FORWARD,sizeof(int),
                     Gather_TestNodeInfo,Scatter_TestNodeInfo);
  }

  DDD_IFAOnewayX(context,
                 dddctrl.NodeAllIF,GRID_ATTR(theGrid),IF_FORWARD,2*sizeof(int),
                 Gather_IdentSonNode,Scatter_IdentSonNode);

#else

  DDD_IFAOnewayX(context,
                 dddctrl.NodeAllIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(int),
                 Gather_SonNodeInfo,Scatter_SonNodeInfo);

#endif
  return GM_OK;
}


/****************************************************************************/
/*
   Identify_SonEdges - identify son edges

   SYNOPSIS:
   INT Identify_SonEdges (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT Identify_SonEdges (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

#ifdef IDENT_ONLY_NEW

  DDD_IFAOnewayX(context,
                 dddctrl.EdgeSymmVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(int),
                 Gather_NewObjectInfo,Scatter_NewObjectInfo);

  if (UPGRID(theGrid) != NULL)
  {
    check_nodetype = MID_NODE;
    DDD_IFAOnewayX(context,
                   dddctrl.NodeAllIF,GRID_ATTR(UPGRID(theGrid)),IF_FORWARD,sizeof(int),
                   Gather_NodeInfo,Scatter_NodeInfo);
    if (EDIDENTASSERT)
      DDD_IFAOnewayX(context,
                     dddctrl.EdgeSymmVHIF,GRID_ATTR(UPGRID(theGrid)),IF_FORWARD,sizeof(int),
                     Gather_EdgeInfo,Scatter_EdgeInfo);
    if (0)
      DDD_IFAOnewayX(context,
                     dddctrl.EdgeSymmVHIF,GRID_ATTR(UPGRID(theGrid)),IF_FORWARD,sizeof(int),
                     Gather_TestEdgeInfo,Scatter_TestEdgeInfo);
  }

  DDD_IFAOnewayX(context,
                 dddctrl.EdgeSymmVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(int),
                 Gather_IdentSonObjects,Scatter_IdentSonObjects);

#else

  /* identify the sonedges */
  DDD_IFAOnewayX(context,
                 dddctrl.EdgeSymmVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(int),
                 Gather_SonEdgeInfo,Scatter_SonEdgeInfo);

#endif

  return GM_OK;
}


/****************************************************************************/
/*
   Identify_SonObjects - identify son objects

   SYNOPSIS:
   INT Identify_SonObjects (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:
   This function identifies all objects which are not symmetrically created
   during grid adaption. These are edges and nodes of the type used by
   yellow elements, son nodes of type CORNERNODE and son edges.

   RETURN VALUE:
   INT
 */
/****************************************************************************/

#define NODESFIRST 1

INT NS_DIM_PREFIX Identify_SonObjects (GRID *theGrid)
{
    #ifdef Debug
  identlevel = GLEVEL(theGrid)+1;
        #endif

  if (NODESFIRST)
  {
    if (Identify_SonNodes (theGrid) != GM_OK) RETURN(GM_ERROR);
  }
  else
  {
    if (Identify_SonEdges (theGrid) != GM_OK) RETURN(GM_ERROR);
  }

  if (IDENT_IN_STEPS)
  {
    DDD_IdentifyEnd(theGrid->dddContext());
    DDD_IdentifyBegin(theGrid->dddContext());
  }


  if (!NODESFIRST)
  {
    if (Identify_SonNodes (theGrid) != GM_OK) RETURN(GM_ERROR);
  }
  else
  {
    if (Identify_SonEdges (theGrid) != GM_OK) RETURN(GM_ERROR);
  }

  return GM_OK;
}

/****************************************************************************/
/*
   Identify_Objects_of_ElementSide -

   SYNOPSIS:
   INT Identify_Objects_of_ElementSide(GRID *theGrid, ELEMENT *theElement, INT i);

   PARAMETERS:
   .  theGrid
   .  theElement
   .  i

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX Identify_Objects_of_ElementSide(GRID *theGrid, ELEMENT *theElement, INT i)
{
  ELEMENT* theNeighbor = NBELEM(theElement,i);
  if (theNeighbor == NULL) return GM_OK;

  const DDD_PRIO prio = EPRIO(theNeighbor);
  /* identification is only needed if theNeighbor removed his refinement  */
  /* or was not refined before, thus has NSONS==0, if NSONS>0 the objects */
  /* shared between the element sides are already identified and no new   */
  /* objects are created for this element side which need identification  */
  /* (980217 s.l.)                                                        */
  /*
          if (!EHGHOSTPRIO(prio) || NSONS(theNeighbor)!=0) return(GM_OK);
   */
  if (!EHGHOSTPRIO(prio) || !MARKED(theNeighbor)) return(GM_OK);

        #ifdef Debug
  identlevel = GLEVEL(theGrid);
        #endif
  if (IdentifyObjectsOfElementSide(theGrid,theElement,i,theNeighbor)) RETURN(GM_FATAL);

  return GM_OK;
}


/****************************************************************************/
/*
    IdentifyInit-

   SYNOPSIS:
   void IdentifyInit (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void NS_DIM_PREFIX IdentifyInit (MULTIGRID *theMG)
{
        #ifdef Debug
  debug = 0;
        #endif

  /* allocate a control word entry to lock nodes */
  if (AllocateControlEntry(NODE_CW,NEW_NIDENT_LEN,&ce_NEW_NIDENT) != GM_OK)
    assert(0);

  /* allocate a control word entry to lock edges */
  if (AllocateControlEntry(EDGE_CW,NEW_EDIDENT_LEN,&ce_NEW_EDIDENT) != GM_OK)
    assert(0);

  for (INT i=0; i<=TOPLEVEL(theMG); i++)
    ResetIdentFlags(GRID_ON_LEVEL(theMG,i));

  /* set Ident_FctPtr to identification mode */
  Ident_FctPtr = Identify_by_ObjectList;

}


/****************************************************************************/
/*
   IdentifyExit -

   SYNOPSIS:
   void IdentifyExit (void);

   PARAMETERS:
   .  void

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void NS_DIM_PREFIX IdentifyExit (void)
{
  FreeControlEntry(ce_NEW_NIDENT);
  FreeControlEntry(ce_NEW_EDIDENT);
}

#endif /* end ModelP */
