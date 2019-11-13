/****************************************************************************/
/*                                                                          */
/* File:      gmcheck.c                                                     */
/*                                                                          */
/* Purpose:   checks of the data structure                                  */
/*                                                                          */
/* Author:    Stefan Lang                                                   */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   Juli 1, 1997 moved from ugm.c                                 */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*        defines to exclude functions                                      */
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
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>
#include <errno.h>

#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/fifo.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugstruct.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/ugdevices.h>

#include "evm.h"
#include "gm.h"
#include "rm.h"
#include "dlmgr.h"
#include "algebra.h"
#include "ugm.h"
#include "elements.h"
#include "shapes.h"
#include "refine.h"
#include <dune/uggrid/domain/domain.h>

#ifdef ModelP
#include <dune/uggrid/parallel/dddif/parallel.h>
#endif

#include "cw.h"

USING_UG_NAMESPACES
using namespace PPIF;

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/* compile time constants defining static data size (i.e. arrays)           */
/* other constants                                                          */
/* macros                                                                   */
/*                                                                          */
/****************************************************************************/

#define RESOLUTION       20     /* resolution for creating boundary midnode */
#define SMALL1 0.001

#define ORDERRES                1e-3    /* resolution for OrderNodesInGrid			*/
#define LINKTABLESIZE   32              /* max number of inks per node for ordering	*/

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
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

static DOUBLE hghost_overlap = 1.0;

REP_ERR_FILE;

/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined           */
/*                                                                          */
/****************************************************************************/

static INT CheckVertex (ELEMENT *theElement, NODE* theNode, VERTEX *theVertex)
{
  ELEMENT *theFather = VFATHER(theVertex);
  EDGE    *theEdge;
  INT i,nerrors,n;
  DOUBLE  *global,*local,diff;
  DOUBLE_VECTOR global1;
  DOUBLE *x[MAX_CORNERS_OF_ELEM];

  nerrors = 0;
  if (SONNODE(theNode) == NULL)
  {
    INT cnt = NOOFNODE(theVertex);
    NODE *Node = theNode;

    while (Node!=NULL && CORNERTYPE(Node)) {
      cnt--;
      Node = (NODE*) NFATHER(Node);
    }
    if (cnt != 1) {
      UserWriteF("elem=" EID_FMTX " node=" ID_FMTX
                 " vertex=" VID_FMTX
                 " NOOFNODE %d wrong\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),
                 VID_PRTX(theVertex), NOOFNODE(theVertex));
      nerrors = 1;
    }
  }

  if (theFather==NULL && MASTER(theNode) && LEVEL(theVertex)>0)
  {
        #ifdef ModelP
    if (!CORNERTYPE(theNode))
    {
      nerrors = 0;
      IFDEBUG(gm,1)
      nerrors = 1;
      ENDDEBUG
    }
    if (nerrors == 0) return(nerrors);
        #endif
    UserWriteF("elem=" EID_FMTX " node=" ID_FMTX " vertex=" VID_FMTX
               " VFATHER=NULL vertex needs VFATHER\n",EID_PRTX(theElement),ID_PRTX(theNode),
               VID_PRTX(theVertex));
    return(nerrors++);
  }

  if (theFather!=NULL && HEAPCHECK(theFather))
  {
    UserWriteF("elem=" EID_FMTX " node=" ID_FMTX " vertex=" VID_FMTX
               " VFATHER=%x is pointer to ZOMBIE\n",EID_PRTX(theElement),ID_PRTX(theNode),
               VID_PRTX(theVertex),theFather);
    return(nerrors++);
  }


  if (theFather!=NULL && MASTER(theNode) && EPRIO(theFather)==PrioHGhost)
  {
        #ifdef ModelP
    if (!CORNERTYPE(theNode))
    {
      nerrors = 0;
      IFDEBUG(gm,1)
      nerrors = 1;
      ENDDEBUG
    }
        #endif
    if (nerrors == 0) return(nerrors);
    UserWriteF("elem=" EID_FMTX " node=" ID_FMTX " vertex=" VID_FMTX
               " VFATHER=" EID_FMTX " vertex needs VFATHER with prio master or vghost\n",
               EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex),EID_PRTX(theFather));
    return(nerrors++);
  }

  if (theFather != NULL) {
    CORNER_COORDINATES(theFather,n,x);
    global = CVECT(theVertex);
    local = LCVECT(theVertex);
    LOCAL_TO_GLOBAL(n,x,local,global1);
    V_DIM_EUKLIDNORM_OF_DIFF(global1,global,diff);
    if (diff > MAX_PAR_DIST) {
      nerrors++;
                        #ifdef ModelP
      if (CORNERTYPE(theNode) || GHOST(theNode))
      {
        nerrors = 0;
        IFDEBUG(gm,1)
        nerrors = 1;
        ENDDEBUG
      }
                        #endif
      if (nerrors >= 1)
      {
        UserWriteF("elem=" EID_FMTX " node=" ID_FMTX "/%d vertex=" VID_FMTX
                   " WARNING VFATHER=%x WARNING diff %f local and global coordinates don't match\n",
                   EID_PRTX(theElement),ID_PRTX(theNode),
                   NTYPE(theNode),VID_PRTX(theVertex),theFather,diff);
      }
    }
  }

  switch (NTYPE(theNode))
  {
  case (CORNER_NODE) :
    if (LEVEL(theVertex)==0 && theFather != NULL)
    {
      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                 " VID=" VID_FMTX " CORNER_NODE has VFATHER\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex));
    }

                        #ifdef ModelP
    IFDEBUG(gm,0)
    /* break for ghost nodes if debugging off */
    if (GHOST(theNode)) break;
    ENDDEBUG
                        #endif

    if (LEVEL(theVertex)>0 && theFather == NULL)
    {
      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                 " VID=" VID_FMTX " CORNER_NODE has no VFATHER\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex));
    }
    break;

  case (MID_NODE) :
    /* check ONEDGE and VFATHER */
    if (theFather == NULL)
    {
                                #ifdef ModelP
      IFDEBUG(gm,0)
      /* break for ghost nodes if debugging off */
      if (GHOST(theNode)) break;
      ENDDEBUG
                                #endif

      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                 " VID=" VID_FMTX " MID_NODE VFATHER=NULL\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex));
      nerrors++;
      break;
    }
    i = ONEDGE(theVertex);
    theEdge = GetEdge(CORNER(theFather,CORNER_OF_EDGE(theFather,i,0)),
                      CORNER(theFather,CORNER_OF_EDGE(theFather,i,1)));

    if (theEdge==NULL || theNode!=MIDNODE(theEdge))
    {
      nerrors++;
                #ifdef ModelP
      if (EGHOST(theElement)) {
        nerrors = 0;
        IFDEBUG(gm,1)
        nerrors = 1;
        ENDDEBUG
      }
                #endif
      if (nerrors == 0) break;
      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX " VID=" VID_FMTX
                 " ONEDGE and VFATHER incompatible edgeptr=%08x\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),
                 VID_PRTX(theVertex),theEdge);
    }
    break;

                #ifdef __THREEDIM__
  case (SIDE_NODE) :
    if (theFather == NULL)
    {
      nerrors++;
                #ifdef ModelP
      if (EPRIO(theElement)==PrioHGhost) {
        nerrors = 0;
        IFDEBUG(gm,1)
        nerrors = 1;
        ENDDEBUG
      }
                #endif
      if (nerrors == 0) break;
      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                 " VID=" VID_FMTX " SIDE_NODE VFATHER=NULL\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex));
      break;
    }
    else {
      if (GetSideNode(theFather,ONSIDE(theVertex)) != theNode) {
        nerrors = 1;
        UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                   " VID=" VID_FMTX " inconsistent ONSIDE entry\n",
                   EID_PRTX(theElement),ID_PRTX(theNode),
                   VID_PRTX(theVertex));
      }
    }
    break;
        #endif

  case (CENTER_NODE) :
    if (theFather == NULL)
    {
      nerrors++;
                #ifdef ModelP
      if (EGHOST(theElement)) {
        nerrors = 0;
        IFDEBUG(gm,1)
        nerrors = 1;
        ENDDEBUG
      }
                #endif
      if (nerrors == 0) break;
      UserWriteF("EID=" EID_FMTX " NID=" ID_FMTX
                 " VID=" VID_FMTX " CENTER_NODE VFATHER=NULL\n",
                 EID_PRTX(theElement),ID_PRTX(theNode),VID_PRTX(theVertex));
      break;
    }
    break;
  }

  return(nerrors);
}

static INT CheckNode (ELEMENT *theElement, NODE* theNode, INT i)
{
  VERTEX  *theVertex      = MYVERTEX(theNode);
  NODE    *FatherNode;
  EDGE    *FatherEdge;
  INT nerrors         = 0;

  SETUSED(theNode,1);

  if (OBJT(theNode) != NDOBJ)
  {
    UserWriteF(" node=" ID_FMTX " has wrong OBJ=%d\n",
               ID_PRTX(theNode),OBJT(theNode));
    return(nerrors++);
  }

  if (NVECTOR(theNode)!=NULL && VOBJECT(NVECTOR(theNode)) == NULL)
  {
    UserWriteF(" node=" ID_FMTX " has vector" ID_FMTX "  with VOBJ=NULL\n",
               ID_PRTX(theNode),ID_PRTX(NVECTOR(theNode)));
    return(nerrors++);
  }

  switch (NTYPE(theNode))
  {
  case (LEVEL_0_NODE) :
    if (LEVEL(theNode) > 0) {
      UserWriteF(" node=" ID_FMTX " has NTYPE=LEVEL_0_NODE"
                 " but is on level=%d\n",
                 ID_PRTX(theNode),LEVEL(theNode));
      return(nerrors++);
    }
    break;
  case (CORNER_NODE) :
  {
    FatherNode = (NODE *)NFATHER(theNode);
    if (0) /* this code is for special debugging (980204 s.l.) */
      if (GID(theNode)==0x11011 && FatherNode!=NULL)
      {
        UserWriteF(" cornernode=" ID_FMTX " has father=" ID_FMTX "\n",
                   ID_PRTX(theNode),ID_PRTX(FatherNode));
      }

    if (FatherNode == NULL)
    {
                                        #ifdef ModelP
      if (MASTER(theNode))
      {
                                        #endif
      UserWriteF(" ERROR cornernode=" ID_FMTX " has no father level=%d\n",
                 ID_PRTX(theNode),LEVEL(theNode));
      UserWriteF(" elem=" EID_FMTX, EID_PRTX(theElement));
      if (EFATHER(theElement) != NULL)
      {
        INT i;
        ELEMENT *theFather = EFATHER(theElement);

        UserWriteF(" father=" EID_FMTX "\n",EID_PRTX(theFather));
        for (i=0; i<CORNERS_OF_ELEM(theFather); i++)
        {
          UserWriteF("son[%d]=" ID_FMTX "\n",i,ID_PRTX(CORNER(theFather,i)));
        }
      }
      else
        UserWriteF(" father=NULL\n");

      nerrors++;
                                        #ifdef ModelP
    }
    else
    {
      INT print = 0;
      IFDEBUG(gm,1)
      print = 1;
      ENDDEBUG
      if (print)
        UserWriteF(" WARN cornernode=" ID_FMTX " has no father level=%d\n",
                   ID_PRTX(theNode),LEVEL(theNode));
    }
                                        #endif
    }
    if (FatherNode != NULL)
    {
      if (HEAPCHECK(FatherNode))
      {
        UserWriteF("elem=" EID_FMTX " cornernode=%d NID=" ID_FMTX
                   " has father pointer to ZOMBIE\n",EID_PRTX(theElement),ID_PRTX(theNode));
        nerrors++;
        break;
      }

      if (OBJT(FatherNode) != NDOBJ)
      {
        UserWriteF(" cornernode=" ID_FMTX
                   " has father of wrong type=%d\n",
                   ID_PRTX(theNode),OBJT(FatherNode));
        nerrors++;
      }
      else
      {
        if (SONNODE(FatherNode) != theNode)
        {
          UserWriteF(" cornernode=" ID_FMTX
                     " has node father=" ID_FMTX " with wrong backptr=%x\n",
                     ID_PRTX(theNode),ID_PRTX(FatherNode),SONNODE(FatherNode));
          /* TODO: this should be deleted */
          if (0) SONNODE(FatherNode) = theNode;
          else nerrors++;
        }
      }
    }
  }
  break;

  case (MID_NODE) :
    if (LEVEL(theNode)>0)
    {
      FatherEdge = (EDGE *)NFATHER(theNode);
      if (FatherEdge == NULL)
      {
                                        #ifdef ModelP
        if (MASTER(theNode))
        {
                                        #endif
        UserWriteF(" ERROR midnode=" ID_FMTX " has no father level=%d\n",
                   ID_PRTX(theNode),LEVEL(theNode));
        UserWriteF(" elem=" EID_FMTX, EID_PRTX(theElement));
        if (EFATHER(theElement) != NULL)
          UserWriteF(" father=" EID_FMTX "\n",EID_PRTX(EFATHER(theElement)));
        else
          UserWriteF(" father=NULL\n");
        nerrors++;
                                        #ifdef ModelP
      }
      else
      {
        IFDEBUG(gm,1)
        UserWriteF(" WARN midnode=" ID_FMTX " has no father level=%d\n",
                   ID_PRTX(theNode),LEVEL(theNode));
        ENDDEBUG
      }
                                        #endif
      }
      if (FatherEdge != NULL)
      {
        if (HEAPCHECK(FatherEdge))
        {
          UserWriteF("elem=" EID_FMTX " edge=%d/%x midnode NID=" ID_FMTX
                     " fatherpointer to edge=%d/%x is ZOMBIE\n",EID_PRTX(theElement),
                     ID_PRTX(theNode),i,FatherEdge);
          nerrors++;
          break;
        }

        if (OBJT(FatherEdge) != EDOBJ)
        {
          UserWriteF(" midnode=" ID_FMTX
                     " has father of wrong type=%d obj=\n",
                     ID_PRTX(theNode),OBJT(FatherEdge));
          nerrors++;
        }
        else
        {

          if (MIDNODE(FatherEdge) != theNode)
          {
            UserWriteF(" midnode=" ID_FMTX
                       " has edge  father=" ID_FMTX " with wrong backptr=%x\n",
                       ID_PRTX(theNode),ID_PRTX(FatherEdge),MIDNODE(FatherEdge));
            /* TODO: this should be deleted */
            if (0) MIDNODE(FatherEdge) = theNode;
            else nerrors++;
            /*nerrors++; temp. auskommentiert, um Reperaturwirkung wirklich nutzen zu koennen */
          }
        }
      }
    }
    else
    {
      UserWriteF(" node=" ID_FMTX " is midnode BUT on level=%d\n",
                 ID_PRTX(theNode),LEVEL(theNode));
      nerrors++;
    }
    break;

  case (SIDE_NODE) :
    break;

  case (CENTER_NODE) :
    break;

  default :
    UserWriteF(" node=" ID_FMTX " has unrecognized NTYPE=%d\n",
               ID_PRTX(theNode),NTYPE(theNode));
    break;
  }

  if (theVertex != NULL)
  {
    CheckVertex(theElement,theNode,theVertex);
  }
  else
  {
    UserWriteF("elem=" EID_FMTX " node[%d]=" ID_FMTX " vertex=NULL\n",
               EID_PRTX(theElement),i,ID_PRTX(theNode));
    nerrors++;
  }

  return(nerrors);
}

static INT CheckEdge (ELEMENT *theElement, EDGE* theEdge, INT i)
{
  INT nerrors = 0;
  NODE    *theNode;
  VERTEX  *theVertex;

  SETUSED(theEdge,1);

  /** \todo Commented out because it uses GetElemLink, which does not exist */
#if 0
#       if defined(__TWODIM__)
  {
    int elemlink,no_of_elem,No_Of_Elem;
    NODE *n0,*n1;
    LINK *l0,*l1;
    ELEMENT *e0,*e1;

    n0 = CORNER_OF_EDGE_PTR(theElement,i,0);
    n1 = CORNER_OF_EDGE_PTR(theElement,i,1);

    elemlink = GetElemLink(n0,n1,theElement);

    l0 = LINK0(theEdge);
    l1 = LINK1(theEdge);
    e0 = LELEM(l0);
    e1 = LELEM(l1);

    /* check number of edge neighbor elements */
    no_of_elem = 0;
    if (e0 != NULL)
    {
      no_of_elem++;
      HEAPFAULT(e0);
    }
    if (e1 != NULL)
    {
      no_of_elem++;
      HEAPFAULT(e1);
    }

    No_Of_Elem = NO_OF_ELEM(theEdge);

    if (no_of_elem==0 || No_Of_Elem!=no_of_elem)
    {
      UserWriteF("elem=" EID_FMTX " edge%d=" EDID_FMTX " NO_OF_ELEM wrong"
                 "NO_OF_ELEM=%d no_of_elem=%d\n",
                 EID_PRTX(theElement),i,EDID_PRTX(theEdge),No_Of_Elem,no_of_elem);
    }

    if (elemlink == 0)
    {
      if (e0 != theElement)
      {
        UserWriteF("elem=" EID_FMTX " edge%d=" EDID_FMTX " LELEM0 wrong"
                   "elemlink=%d LELEM0=%08x\n",
                   EID_PRTX(theElement),i,EDID_PRTX(theEdge),elemlink,
                   (e0!=NULL) ? EGID(e0) : 0);
        nerrors++;
      }
    }
    else
    {
      if (e1 != theElement)
      {
        UserWriteF("elem=" EID_FMTX " edge%d=" EDID_FMTX " LELEM0 wrong"
                   "elemlink=%d LELEM0=%08x\n",
                   EID_PRTX(theElement),i,EDID_PRTX(theEdge),elemlink,
                   (e1!=NULL) ? EGID(e1) : 0);
        nerrors++;
      }
    }
  }
#       endif
#endif

  theNode = MIDNODE(theEdge);
  if (theNode == NULL)
  {
#ifdef DUNE_UGGRID_TET_RULESET
    if (((REFINE(theElement) == RED) && (TAG(theElement) != TETRAHEDRON))
        || ((TAG(theElement) == TETRAHEDRON) && (NSONS(theElement) == 8)))
#else
    if (REFINE(theElement) == RED)
#endif
    {

        #ifdef ModelP
      IFDEBUG(gm,1)
            #endif
      UserWriteF("elem=" EID_FMTX " edge%d=" EDID_FMTX " midnode NID=NULL"
                 " BUT REFINE(elem)=RED\n",EID_PRTX(theElement),i,EDID_PRTX(theEdge));
      nerrors++;
                #ifdef ModelP
      ENDDEBUG
            #endif
      return(nerrors);
    }
    else
      return(nerrors);
  }

  if (HEAPCHECK(theNode))
  {
    UserWriteF("elem=" EID_FMTX " edge=%d/%x midnode NID=" ID_FMTX
               " is pointer to ZOMBIE\n",EID_PRTX(theElement),i,theEdge,ID_PRTX(theNode));
    return(nerrors++);
  }

  theVertex = MYVERTEX(theNode);
  if (theVertex == NULL)
  {
    UserWriteF("elem=" EID_FMTX " edge=%d/%x midnode NID=" ID_FMTX " vertex=NULL\n",
               EID_PRTX(theElement),i,theEdge,ID_PRTX(theNode));
    return(nerrors++);
  }

  if (VFATHER(theVertex) != theElement)
    return(nerrors);

  if (i != ONEDGE(theVertex))
  {
    if (EGHOST(theElement))
    {
      IFDEBUG(gm,1)
      UserWriteF("EID=" EID_FMTX " VID=" VID_FMTX
                 " WARNING edgenumber of vertex wrong\n",
                 EID_PRTX(theElement),VID_PRTX(theVertex));
      ENDDEBUG
    }
    else
    {
      UserWriteF("EID=" EID_FMTX " VID=" VID_FMTX
                 " ERROR edgenumber of vertex wrong\n",
                 EID_PRTX(theElement),VID_PRTX(theVertex));
      /*nerrors++;  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    }
    return(nerrors);
  }

  return(nerrors);
}

#ifdef ModelP
int EdgeHasTMasterCopy (DDD::DDDContext& context, ELEMENT *e, int i)
{
  int nmaster,nborder,nall;
  EDGE *edge;

  edge = GetEdge(CORNER_OF_EDGE_PTR(e,i,0),CORNER_OF_EDGE_PTR(e,i,1));
  assert(edge != NULL);

  nmaster = CheckProcListCons(PROCLIST(context, edge),PrioMaster);
  nborder = CheckProcListCons(PROCLIST(context, edge),PrioBorder);
  nall = nmaster + nborder;
  if (0)
    assert(nall==1 || nall==2);

  if (nall > 2)
  {
    UserWriteF("EID=" EID_FMTX " EDID=" EDID_FMTX
               " ERROR edge%d has mastertype prios=%d\n",
               EID_PRTX(e),EDID_PRTX(edge),i,nall);
  }

  return(nall-1);
}
#endif

static INT CheckElement (GRID *theGrid, ELEMENT *theElement, INT *SideError, INT *EdgeError,
                         INT *NodeError, INT *ESonError, INT *NSonError, INT *errors)
{
  INT i,j,k,l,n,nsons,bserror,nerrors;
  NODE    *theNode,*theNode1;
  EDGE    *theEdge;
  ELEMENT *NbElement,*theFather;
  ELEMENT *SonList[MAX_SONS];
  VERTEX  *theVertex,*Vertices[MAX_CORNERS_OF_ELEM];

#if defined(ModelP) && defined(__TWODIM__)
  auto& dddContext = theGrid->dddContext();
#endif

  *SideError = 0;
  *NodeError = 0;
  *EdgeError = 0;
  *ESonError = 0;
  *NSonError = 0;
  nerrors    = 0;

  bserror = 0;

  /* check level */
  if (GLEVEL(theGrid) != LEVEL(theElement))
  {
    UserWriteF("elem=" EID_FMTX " ERROR level=%2d but gridlevel=%2d\n",
               EID_PRTX(theElement),LEVEL(theElement),GLEVEL(theGrid));
    nerrors++;
  }

  /* check side information */
  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    if (OBJT(theElement) == BEOBJ)
      if (ELEM_BNDS(theElement,i) != NULL) {
        for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++) {
          k = CORNER_OF_SIDE(theElement,i,j);
          theNode = CORNER(theElement,k);
          if (NSUBDOM(theNode) != 0) {
            UserWriteF("wrong subdomain id(%d) on boundary node,"
                       "el =  " EID_FMTX ", side = %d, corner = %d, node = " ID_FMTX "\n",
                       NSUBDOM(theNode),EID_PRTX(theElement),i,k,ID_PRTX(theNode));
            bserror |= (1<<i);
            nerrors++;
          }
        }
        for (j=0; j<EDGES_OF_SIDE(theElement,i); j++) {
          k  = EDGE_OF_SIDE(theElement,i,j);
          theEdge = GetEdge(CORNER(theElement,
                                   CORNER_OF_EDGE(theElement,k,0)),
                            CORNER(theElement,
                                   CORNER_OF_EDGE(theElement,k,1)));
          ASSERT(theEdge != NULL);
          if (EDSUBDOM(theEdge) != 0) {
            UserWriteF("wrong subdomain id(%d) on boundary edge %d,"
                       "el =  " EID_FMTX ", side = %d, edge = %d, corner0 = " ID_FMTX ", corner1 = " ID_FMTX "\n",
                       EDSUBDOM(theEdge),k,EID_PRTX(theElement), i, j,
                       ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,k,0))),
                       ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,k,1))));
            bserror |= (1<<i);
            nerrors++;
          }
        }
      }

    /* TODO remove self healing */
    if (0)
      if (NBELEM(theElement,i)!=NULL)
        if (ID(NBELEM(theElement,i)) == -1)
        {
          if (EGHOST(theElement))
          {
            SET_NBELEM(theElement,i,NULL);
            UserWriteF("elem=" EID_FMTX " correcting nb error\n",
                       EID_PRTX(theElement));
          }
        }

    NbElement = NBELEM(theElement,i);
    if (NbElement != NULL)
    {
      HEAPFAULT(NbElement);

      /* lets see if NbElement has the neighbor theElement */
      for (j=0; j<SIDES_OF_ELEM(NbElement); j++)
        if (NBELEM(NbElement,j) == theElement)
          break;
      if (j == SIDES_OF_ELEM(NbElement))
      {
        *SideError |= (1<<i);
        UserWriteF("elem=" EID_FMTX " has side error\n",
                   EID_PRTX(theElement));
        nerrors++;
      }
      else
      {
        /* if this is a boundary side it has to be an inner boundary
           and the neighbor side is also a boundary side */
        /* TODO: check boundary side for NbElement==NULL */
        if (OBJT(theElement) == BEOBJ)
          if (SIDE_ON_BND(theElement,i))
          {
            INT err,id,nbid,id_nb,nbid_nb,part;

            err = BNDS_BndSDesc(ELEM_BNDS(theElement,i),&id,&nbid,&part);
            if (err)
            {
              bserror |= (1<<i);
              UserWriteF("elem=" EID_FMTX " ERROR BNDS_BndSDesc(%d) returned err=%d\n",
                         EID_PRTX(theElement),i,err);
            }
            else
            {
              if ((id==0) || (nbid==0))
              {
                /* no interior boundary */
                UserWriteF("elem=" EID_FMTX " ERROR BNDS_BndSDesc(%d) returned id=%d nbid=%d\n",
                           EID_PRTX(theElement),i,id,nbid);
                bserror |= (1<<i);
              }
              if (id==nbid)
              {
                /* should be avoided */
                UserWriteF("elem=" EID_FMTX " ERROR BNDS_BndSDesc(%d) returned id=%d nbid=%d\n",
                           EID_PRTX(theElement),i,id,nbid);
                bserror |= (1<<i);
              }

              /* check neighbour */
              if (!SIDE_ON_BND(NbElement,j))
              {
                UserWriteF("elem=" EID_FMTX " ERROR nb=" EID_FMTX " nbside=%d not on boundary id=%d nbid=%d\n",
                           EID_PRTX(theElement),EID_PRTX(theElement),j,id,nbid);
                bserror |= (1<<i);
              }
              else
              {
                if (BNDS_BndSDesc(ELEM_BNDS(NbElement,j),&id_nb,&nbid_nb,&part))
                {
                  UserWriteF("nb=" EID_FMTX " ERROR BNDS_BndSDesc(%d) returned id=%d nbid=%d\n",
                             EID_PRTX(NbElement),j,id,nbid);
                  bserror |= (1<<i);
                }
                else
                {
                  if (id!=nbid_nb)
                  {
                    UserWriteF("nb=" EID_FMTX " ERROR nbside=%d id=%d unequal nbid_nb=%d\n",
                               EID_PRTX(NbElement),j,id,nbid);
                    bserror |= (1<<i);
                  }
                  if (nbid!=id_nb)
                  {
                    UserWriteF("nb=" EID_FMTX " ERROR nbside=%d nbid=%d unequal id_nb=%d\n",
                               EID_PRTX(NbElement),j,id,nbid);
                    bserror |= (1<<i);
                  }
                }
              }
            }
            if (bserror)
            {
              UserWriteF("elem=" EID_FMTX " nb=" EID_FMTX
                         " elemsubdom=%d nbsubdom=%d\n",
                         EID_PRTX(theElement),EID_PRTX(NbElement),
                         SUBDOMAIN(theElement),SUBDOMAIN(NbElement));
            }
          }
      }

      if( ECLASS(theElement)==NO_CLASS)
      {
        UserWriteF("Element has no ECLASS set, el =  " EID_FMTX "\n",
                   EID_PRTX(theElement));
        nerrors++;
      }

      if (ECLASS(theElement)!=YELLOW_CLASS)
      {
        n = CORNERS_OF_SIDE(theElement,i);
        for (k=0; k<n; k++)
          if (CORNER(theElement,CORNER_OF_SIDE(theElement,i,k))
              == CORNER(NbElement,CORNER_OF_SIDE(NbElement,j,0)))
            break;
        if (k == n)
        {
          *SideError |= (1<<i);
          UserWriteF("no matching corner for CORNER_OF_SIDE(NbElement,j,0)=" ID_FMTX "\n",
                     ID_PRTX(CORNER(NbElement,CORNER_OF_SIDE(NbElement,j,0))));
        }
        if (TAG(theElement)!=TETRAHEDRON
                                #ifdef Debug
            || Debuggm>=1
                                #endif
            )
          for (l=1; l<n; l++)
            if (CORNER(theElement,
                       CORNER_OF_SIDE(theElement,i,(n+k-l)%n))
                != CORNER(NbElement,CORNER_OF_SIDE(NbElement,j,l)))
            {
              *SideError |= (1<<i);
              UserWriteF("corner mismatch side=%d cos=%d corner_el=" ID_FMTX " side=%d cos=%d corner_nb=" ID_FMTX " el = " EID_FMTX "\n",
                         i,(n+k-l)%n,
                         ID_PRTX(CORNER(theElement,CORNER_OF_SIDE(theElement,i,(n+k-l)%n))),
                         j,l,
                         ID_PRTX(CORNER(NbElement,CORNER_OF_SIDE(NbElement,j,l))),EID_PRTX(theElement));
            }
      }
    }
    else
    {
      if (ECLASS(theElement)!=YELLOW_CLASS)
        if (OBJT(theElement) == IEOBJ)
                                #ifdef ModelP
          if (EMASTER(theElement))
                                #if defined(__TWODIM__)
            if (hghost_overlap!=0.0 || EdgeHasTMasterCopy(dddContext, theElement,i)==0)
                                #endif
                                #endif
          *SideError |= (1<<(i+MAX_SIDES_OF_ELEM));

      if (OBJT(theElement) == BEOBJ)
      {
        if (SIDE_ON_BND(theElement,i))
        {
                                  #ifdef ModelP
          if (EMASTER(theElement))
                                  #if defined(__TWODIM__)
            if (hghost_overlap!=0.0 || EdgeHasTMasterCopy(dddContext, theElement,i)==0)
                                  #endif
                                  #endif
          if (INNER_SIDE(theElement,i)) {
            *SideError |= (1<<(i+2*MAX_SIDES_OF_ELEM));
            UserWriteF("no nb Element for inner boundary, el =  " EID_FMTX "\n",
                       EID_PRTX(theElement));
            nerrors++;
          }
          for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++)
          {
            theVertex = MYVERTEX(CORNER(theElement,(k=CORNER_OF_SIDE(theElement,i,j))));
            if (OBJT(theVertex) == IVOBJ)
              *NodeError |= (1<<(k+MAX_CORNERS_OF_ELEM));
          }
        }
        else if (ECLASS(theElement)!=YELLOW_CLASS)
                                        #ifdef ModelP
          if (EMASTER(theElement))
                                    #if defined(__TWODIM__)
            if (hghost_overlap!=0.0 || EdgeHasTMasterCopy(dddContext, theElement,i)==0)
                                        #endif
                                        #endif
          *SideError |= (1<<(i+2*MAX_SIDES_OF_ELEM));
      }
    }
  }

  /* check node information */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    theNode = CORNER(theElement,i);

    if (theNode != NULL)
      nerrors += CheckNode(theElement,theNode,i);
    else
    {
      UserWriteF("elem=" EID_FMTX " corner=%d nodeptr=NULL\n",
                 EID_PRTX(theElement),i);
      nerrors++;
    }
  }

  /* check edge information */
  for (i=0; i<EDGES_OF_ELEM(theElement); i++)
  {
    theNode  = CORNER(theElement,CORNER_OF_EDGE(theElement,i,0));
    theNode1 = CORNER(theElement,CORNER_OF_EDGE(theElement,i,1));

    if (theNode == NULL || theNode1 == NULL)
    {
      UserWriteF("elem=" EID_FMTX " edge=%d n0ptr=NULL or n1ptr=NULL\n",
                 EID_PRTX(theElement),i,theNode,theNode1);
      nerrors++;
      continue;
    }

    theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                      CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));

    if (theEdge != NULL)
      nerrors += CheckEdge(theElement,theEdge,i);
    else
    {
      UserWriteF("elem=" EID_FMTX " edge=%d n0=" ID_FMTX " n1="
                 ID_FMTX " edgeptr=NULL\n",
                 EID_PRTX(theElement),i,ID_PRTX(theNode),ID_PRTX(theNode1));
      nerrors++;
    }
  }

  /* check orientation */
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    Vertices[i] = MYVERTEX(CORNER(theElement,i));
  if (0)
    if (!CheckOrientation(CORNERS_OF_ELEM(theElement),Vertices))
    {
      UserWriteF("elem=" EID_FMTX " wrong orientation",EID_PRTX(theElement));
      nerrors++;
    }

  /* check father information */
  theFather = EFATHER(theElement);
  if (theFather != NULL)
  {
    HEAPFAULT(theFather);
    /* check MIDNODE information of father */
    for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    {
      theNode = CORNER(theElement,i);
      if (NTYPE(theNode) == MID_NODE)
      {
        for (j=0; j<EDGES_OF_ELEM(theFather); j++)
        {
          theEdge = GetEdge(CORNER(theFather,
                                   CORNER_OF_EDGE(theFather,j,0)),
                            CORNER(theFather,
                                   CORNER_OF_EDGE(theFather,j,1)));
          if (MIDNODE(theEdge) == theNode) break;
        }
        if (j >= EDGES_OF_ELEM(theFather))
        {
                                        #ifdef ModelP
          if (EMASTER(theFather)) {
            IFDEBUG(gm,1)
            UserWriteF("ELEM(" EID_FMTX ") WARNING MIDNODE=NULL"
                       " for mid node[%d]" ID_FMTX "\n",
                       EID_PRTX(theFather),i,ID_PRTX(theNode));
            ENDDEBUG
          }
                                        #else
          UserWriteF("ELEM(" EID_FMTX ") ERROR MIDNODE=NULL"
                     " for mid node[%d]=" ID_FMTX "\n",
                     EID_PRTX(theFather),i,ID_PRTX(theNode));
          nerrors++;
                                        #endif
        }
      }
    }

    /* check son information of father     */
    if (GetAllSons(theFather,SonList))
    {
      UserWrite("cannot get sons\n");
      return (1);
    }
    for (i=0; i<NSONS(theFather); i++)
    {
      if (SonList[i] == theElement) break;
    }
    if (i == NSONS(theFather))
    {
      UserWriteF("ELEM(" EID_FMTX ") FATHER(" EID_FMTX
                 ")element is not in SonList NSONS=%d\n",
                 EID_PRTX(theElement),EID_PRTX(theFather),
                 NSONS(theFather));
      /** \todo activate if NSONS is consistent */
      if (0) nerrors++;
    }
  }
#ifdef ModelP
  else
  {
    if (LEVEL(theElement) > 0)
    {
      if (EMASTER(theElement))
      {
        UserWriteF("ELEM(" EID_FMTX ") ERROR father=NULL\n",
                   EID_PRTX(theElement));
        nerrors++;
      }
    }
  }
#endif

  /* check son information */
  if (NSONS(theElement)!=0)
  {
    nsons = NSONS(theElement);

    if (GetAllSons(theElement,SonList))
    {
      UserWrite("cannot get sons\n");
      return (1);
    }
    for (i=0; (SonList[i]!=NULL || i<nsons) && i<MAX_SONS; i++)
    {
      IFDEBUG(gm,1)
      if (REFINE(theElement)==0)
      {
        UserWriteF("ELEM(" EID_FMTX "): element is not refined "
                   "but has NSONS=%d\n",EID_PRTX(theElement),nsons);
      }
      ENDDEBUG

      if (i >= nsons)
      {
        UserWriteF("ELEM(" EID_FMTX "): element has nsons=%d but "
                   " son[%d]=" EID_FMTX " exists\n", EID_PRTX(theElement),
                   NSONS(theElement),i,EID_PRTX(SonList[i]));
        /* TODO: activate if NSONS is consistent */
        if (0) nerrors++;
      }

      if (SonList[i] == NULL)
      {
        UserWriteF("ELEM(" EID_FMTX "): element has nsons=%d but "
                   " son[%d]=NULL\n", EID_PRTX(theElement),nsons,i);
        *ESonError |= (1<<i);
        nerrors++;
        continue;
      }
      if (EFATHER(SonList[i])!=theElement)
      {
        UserWriteF("i=%d theElement=" EID_FMTX
                   " SonList[i]=" EID_FMTX "\n",
                   i,EID_PRTX(theElement),EID_PRTX(SonList[i]));
        *ESonError |= (1<<i);
        nerrors++;
      }
    }
  }

  if (bserror)
  {
    UserWriteF("theElement=" EID_FMTX
               " bserror=%d\n",
               EID_PRTX(theElement),bserror);
    nerrors++;
  }
  if (nerrors > 0)
  {
    UserWriteF("ELEM(" EID_FMTX "): element has %d errors\n",
               EID_PRTX(theElement),nerrors);
    *errors = nerrors;
  }

  if (*SideError || *EdgeError || *NodeError || *ESonError || *NSonError)
    return (1);

  return (0);
}

#if defined(__TWODIM__) || defined(ModelP)

INT CheckSubdomains (MULTIGRID *theMG)
{
  return (0);
}

#else

static INT CheckElementSubdomains (GRID *theGrid, ELEMENT *theElement, INT *NodeError, INT *EdgeError, INT *NbError, INT *FatherError, INT *errors)
{
  INT side,found,i,j,k,nerrors,sdid,sc;
  NODE    *theNode,*n1,*n2,*nbn1,*nbn2,*nbn3,*nbn4;
  EDGE    *theEdge,*father_edge;
  ELEMENT *theFather;
  VERTEX  *theVertex;

  *NodeError = 0;
  *EdgeError = 0;
  *NbError = 0;
  *FatherError = 0;
  nerrors    = 0;

  /* check side information */
  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    if (OBJT(theElement)==BEOBJ && ELEM_BNDS(theElement,i)!=NULL)
    {
      for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++)
      {
        k = CORNER_OF_SIDE(theElement,i,j);
        theNode = CORNER(theElement,k);
        if (NSUBDOM(theNode)!=0)
        {
          UserWriteF("wrong subdomain id(%d) on boundary node," "el =  " EID_FMTX ", side = %d, corner = %d, node = " ID_FMTX "\n",
                     NSUBDOM(theNode),EID_PRTX(theElement),i,k,ID_PRTX(theNode));
          *NodeError |= (1<<k);
          nerrors++;
        }
      }
      for (j=0; j<EDGES_OF_SIDE(theElement,i); j++)
      {
        k  = EDGE_OF_SIDE(theElement,i,j);
        theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,k,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,k,1)));
        ASSERT(theEdge!=NULL);
        if (EDSUBDOM(theEdge)!=0)
        {
          UserWriteF("wrong subdomain id(%d) on boundary edge %d,"
                     "el =  " EID_FMTX ", side = %d, edge = %d, corner0 = " ID_FMTX ", corner1 = " ID_FMTX "\n",
                     EDSUBDOM(theEdge),k,EID_PRTX(theElement), i, j,
                     ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,k,0))),
                     ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,k,1))));
          *EdgeError |= (1<<j);
          nerrors++;
        }
      }
    }

    if (NBELEM(theElement,i)!=NULL)
    {
      if (OBJT(theElement)==BEOBJ && ELEM_BNDS(theElement,i)!=NULL)
      {
        if (SUBDOMAIN(theElement)==SUBDOMAIN(NBELEM(theElement,i)))
        {
          UserWriteF("wrong subdomain id(%d)[==%d] of neighbor element," "el =  " EID_FMTX ", side = %d, nb = EID_FMTX\n",
                     SUBDOMAIN(NBELEM(theElement,i)),SUBDOMAIN(theElement),EID_PRTX(theElement),i,EID_PRTX(NBELEM(theElement,i)));
          *NbError |= (1<<i);
          nerrors++;
        }
      }
      else
      {
        if (SUBDOMAIN(theElement)!=SUBDOMAIN(NBELEM(theElement,i)))
        {
          UserWriteF("wrong subdomain id(%d)[!=%d] of neighbor element," "el =  " EID_FMTX ", side = %d, nb = EID_FMTX\n",
                     SUBDOMAIN(NBELEM(theElement,i)),SUBDOMAIN(theElement),EID_PRTX(theElement),i,EID_PRTX(NBELEM(theElement,i)));
          *NbError |= (1<<i);
          nerrors++;
        }
      }
    }
  }

  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    theNode = CORNER(theElement,i);
    if (OBJT(MYVERTEX(theNode))==BVOBJ) continue;
    if (NSUBDOM(theNode)==SUBDOMAIN(theElement)) continue;
    UserWriteF("wrong subdomain id(%d)[==%d] of node," "el =  " EID_FMTX ", nd = " ID_FMTX "\n",
               NSUBDOM(theNode),SUBDOMAIN(theElement),EID_PRTX(theElement),ID_PRTX(theNode));
    *NodeError |= (1<<i);
    nerrors++;
  }

  if (EFATHER(theElement)!=NULL)
    if (SUBDOMAIN(EFATHER(theElement))!=SUBDOMAIN(theElement))
    {
      UserWriteF("wrong subdomain id(%d)[==%d] of father," "el =  " EID_FMTX ", fa = " EID_FMTX "\n",
                 SUBDOMAIN(EFATHER(theElement)),SUBDOMAIN(theElement),EID_PRTX(theElement),EID_PRTX(EFATHER(theElement)));
      *FatherError = 1;
      nerrors++;
    }

  if (GLEVEL(theGrid)==0)
  {
    /* extended check on level 0 */
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
      ASSERT(theEdge!=NULL);
      if (USED(theEdge))
      {
        if (EDSUBDOM(theEdge)!=SUBDOMAIN(theElement))
        {
          UserWriteF("wrong subdomain id(%d)[!=%d] of edge," "el =  " EID_FMTX ", ed = %d \n",
                     EDSUBDOM(theEdge),SUBDOMAIN(theElement),EID_PRTX(theElement),i);
          *EdgeError = (1<<i);
          nerrors++;
        }
      }
      else
      {
        if (EDSUBDOM(theEdge)!=0)
        {
          UserWriteF("wrong subdomain id(%d)[!=0] of edge," "el =  " EID_FMTX ", ed = %d \n",
                     EDSUBDOM(theEdge),EID_PRTX(theElement),i);
          *EdgeError = (1<<i);
          nerrors++;
        }
      }
    }
  }
  else if (EFATHER(theElement)!=NULL)
  {
    /* extended check on higher levels */
    for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    {
      sdid=EDSUBDOM(EFATHER(theElement));
      n1 = CORNER(theElement,CORNER_OF_EDGE(theElement,i,0));
      n2 = CORNER(theElement,CORNER_OF_EDGE(theElement,i,1));
      if (NTYPE(n1)>NTYPE(n2)) {theNode=n1; n1=n2; n2=theNode;}
      switch (NTYPE(n1)|(NTYPE(n2)<<4))
      {
      case (CORNER_NODE | (CORNER_NODE<<4)) :
        father_edge = GetEdge(NFATHER(n1),NFATHER(n2));
        if (father_edge!=NULL) sdid=EDSUBDOM(father_edge);
        else
        {
          /* do fathers of n1, n2 lies on a side (of the father) which has BNDS? */
          for (j=0; j<SIDES_OF_ELEM(theFather); j++)
          {
            found=0;
            for (k=0; k<CORNERS_OF_SIDE(theFather,j); k++)
            {
              sc = CORNER_OF_SIDE(theFather,j,k);
              if (CORNER(theFather,sc)==NFATHER(n1) || CORNER(theFather,sc)==NFATHER(n2)) found++;
            }
            if (found==2 && (OBJT(theFather)==BEOBJ) && SIDE_ON_BND(theFather,j))
            {
              sdid=0;
              break;
            }
          }
        }
        break;
      case (CORNER_NODE | (MID_NODE<<4)) :
        father_edge = NFATHEREDGE(n2);
        assert(father_edge!=NULL);
        nbn1 = NBNODE(LINK0(father_edge));
        nbn2 = NBNODE(LINK1(father_edge));
        if (nbn1==NFATHER(n1) || nbn2==NFATHER(n1)) sdid=EDSUBDOM(father_edge);
        else
        {
          /* do all nodes n1, nbn1, nbn2 ly on the same side of father? */
          side=-1;
          for (j=0; j<SIDES_OF_ELEM(theFather); j++)
          {
            found=0;
            for (k=0; k<CORNERS_OF_SIDE(theFather,j); k++)
            {
              sc = CORNER_OF_SIDE(theFather,j,k);
              if (CORNER(theFather,sc)==NFATHER(n1) || CORNER(theFather,sc)==nbn1 || CORNER(theFather,sc)==nbn2) found++;
            }
            if (found==3)
            {
              side = j;
              break;
            }
          }
          if (side>=0  && (OBJT(theFather)==BEOBJ) && SIDE_ON_BND(theFather,side)) sdid=0;
        }
        break;
      case (MID_NODE | (MID_NODE<<4)) :
        father_edge = NFATHEREDGE(n1);
        assert(father_edge!=NULL);
        nbn1 = NBNODE(LINK0(father_edge));
        nbn2 = NBNODE(LINK1(father_edge));
        father_edge = NFATHEREDGE(n2);
        assert(father_edge!=NULL);
        nbn3 = NBNODE(LINK0(father_edge));
        nbn4 = NBNODE(LINK1(father_edge));

        /* do all nodes nbn1, nbn2, nbn3, nbn4 ly on the same side of father? */
        side=-1;
        for (j=0; j<SIDES_OF_ELEM(theFather); j++)
        {
          found=0;
          for (k=0; k<CORNERS_OF_SIDE(theFather,j); k++)
          {
            sc = CORNER_OF_SIDE(theFather,j,k);
            if (CORNER(theFather,sc)==nbn1) found++;
            if (CORNER(theFather,sc)==nbn2) found++;
            if (CORNER(theFather,sc)==nbn3) found++;
            if (CORNER(theFather,sc)==nbn4) found++;
          }
          if (found==4)
          {
            side = j;
            break;
          }
        }
        if (side>=0 && (OBJT(theFather)==BEOBJ) && SIDE_ON_BND(theFather,side)) sdid=0;
        break;
      case (CORNER_NODE | (SIDE_NODE<<4)) :
        theVertex = MYVERTEX(n2);
        if (VFATHER(theVertex)==theFather) side = ONSIDE(theVertex);
        else side = ONNBSIDE(theVertex);
        if ((OBJT(theFather)==BEOBJ) && SIDE_ON_BND(theFather,side))
          for (k=0; k<CORNERS_OF_SIDE(theFather,side); k++)
            if (CORNER(theFather,CORNER_OF_SIDE(theFather,side,k))==NFATHER(n1))
            {
              sdid=0;
              break;
            }
        break;
      case (MID_NODE | (SIDE_NODE<<4)) :
        theVertex = MYVERTEX(n2);
        if (VFATHER(theVertex)==theFather) side = ONSIDE(theVertex);
        else side = ONNBSIDE(theVertex);
        if ((OBJT(theFather)==BEOBJ) && SIDE_ON_BND(theFather,side))
        {
          found=0;
          father_edge = NFATHEREDGE(n1);
          assert(father_edge!=NULL);
          nbn1 = NBNODE(LINK0(father_edge));
          nbn2 = NBNODE(LINK1(father_edge));
          for (k=0; k<CORNERS_OF_SIDE(theFather,side); k++)
            if (CORNER(theFather,CORNER_OF_SIDE(theFather,side,k))==nbn1 || CORNER(theFather,CORNER_OF_SIDE(theFather,side,k))==nbn2)
              found++;
          if (found==2) sdid=0;
        }
        break;
      default :
        break;
      }
      if (EDSUBDOM(GetEdge(n1,n2))!=sdid)
      {
        *EdgeError |= (1<<i);
        nerrors++;
      }
    }
  }

  if (nerrors>0)
  {
    UserWriteF("ELEM(" EID_FMTX "): element has %d errors\n",EID_PRTX(theElement),nerrors);
    *errors = nerrors;
  }

  if (*NodeError || *EdgeError || *NbError || *FatherError)
    return (1);

  return (0);
}

INT CheckSubdomains (MULTIGRID *theMG)
{
  GRID *theGrid;
  NODE *theNode;
  EDGE *theEdge;
  LINK *theLink;
  ELEMENT *theElement;
  INT i,j,k,nerror,NodeError,EdgeError,NbError,FatherError,sd_errors;

  /* init */
  nerror=0;

  /* first level 0 */
  theGrid = GRID_ON_LEVEL(theMG,0);
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
    for (theLink=START(theNode); theLink!=NULL; theLink=NEXT(theLink))
      SETUSED(MYEDGE(theLink),1);
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
    if (OBJT(theElement)==BEOBJ)
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
        if (ELEM_BNDS(theElement,i)!=NULL)
          for (j=0; j<EDGES_OF_SIDE(theElement,i); j++)
          {
            k  = EDGE_OF_SIDE(theElement,i,j);
            theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,k,0)),CORNER(theElement,CORNER_OF_EDGE(theElement,k,1)));
            ASSERT(theEdge!=NULL);
            SETUSED(theEdge,0);
          }

  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
      if (CheckElementSubdomains (theGrid,theElement,&NodeError,&EdgeError,&NbError,&FatherError,&sd_errors))
        nerror++;

    if (nerror) UserWriteF("[%d: subdom-ids: %d errors] ",(int)i,(int)nerror);
    else UserWriteF("[%d: subdom-ids: ok] ",(int)i);
    if (nerror && i<TOPLEVEL(theMG)) UserWrite("[check aborted] ");
  }
  UserWrite("\n");

  /* return */
  return (nerror);
}
#endif

static INT CheckGeometry (GRID *theGrid)
{
  NODE *theNode;
  ELEMENT *theElement;
  EDGE *theEdge;
  LINK *theLink;
  int i,j;
  INT SideError, EdgeError, NodeError, ESonError, NSonError, count;
  INT errors = 0;

  /* reset used flags */
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    SETUSED(theNode,0);
    for (theLink=START(theNode); theLink!=NULL; theLink=NEXT(theLink))
      SETUSED(MYEDGE(theLink),0);
  }

  /* check elements */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (CheckElement(theGrid,theElement,&SideError,&EdgeError,
                     &NodeError,&ESonError,&NSonError,&errors)==0) continue;

    UserWriteF("ELEM=" EID_FMTX "\n",EID_PRTX(theElement));

    /* evaluate side information */
    if (SideError)
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        /* back pointer failure */
        if (SideError & 1<<i)
        {
          UserWriteF("   SIDE[%d]=(",i);
          for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++)
          {
            UserWriteF(ID_FMTX,ID_PRTX(CORNER(theElement,
                                              CORNER_OF_SIDE(theElement,i,j))));

            if (j<CORNERS_OF_SIDE(theElement,i)-1) UserWrite(",");
          }
          UserWriteF(") has neighbour=" EID_FMTX " but a backPtr does not exist\n",
                     EID_PRTX(NBELEM(theElement,i)));

          errors++;
        }

        /* neighbor pointer failure */
        if (SideError & 1<<(i+MAX_SIDES_OF_ELEM))
        {
          errors++;

          UserWriteF("   SIDE[%d]=(",i);
          for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++)
          {
            UserWriteF(ID_FMTX,ID_PRTX(CORNER(theElement,
                                              CORNER_OF_SIDE(theElement,i,j))));

            if (j<CORNERS_OF_SIDE(theElement,i)-1) UserWrite(",");
          }
          UserWrite(") ERROR: has no neighbor but element is IEOBJ\n");

          UserWriteF(" Eclass=%d Efather=" EID_FMTX "FECLASS=%d FREFINE=%d\n",
                     ECLASS(theElement),EID_PRTX(EFATHER(theElement)),
                     ECLASS(EFATHER(theElement)),REFINE(EFATHER(theElement)));
          {
            INT i;
            ELEMENT *theFather = EFATHER(theElement);
            ELEMENT *theNeighbor;

            for (i=0; i<SIDES_OF_ELEM(theFather); i++)
            {
              theNeighbor = NBELEM(theFather,i);
              if (theNeighbor != NULL)
              {
                UserWriteF("NB[%d]=" EID_FMTX " NBREFINE=%d\n",
                           i,EID_PRTX(theNeighbor),REFINE(theNeighbor));

              }
            }
          }

        }

        /* boundary failure */
        if (SideError & 1<<(i+2*MAX_SIDES_OF_ELEM))
        {
          errors++;

          UserWriteF("   SIDE[%d]=(",i);
          for (j=0; j<CORNERS_OF_SIDE(theElement,i); j++)
          {
            UserWriteF(ID_FMTX,ID_PRTX(CORNER(theElement,
                                              CORNER_OF_SIDE(theElement,i,j))));

            if (j<CORNERS_OF_SIDE(theElement,i)-1) UserWrite(",");
          }
          UserWrite(") ERROR: has no neighbor, element is BEOBJ "
                    "but there is no SIDE\n");
        }
      }

    /* evaluate edge information */
    if (EdgeError)
      for (i=0; i<EDGES_OF_ELEM(theElement); i++)
      {
        if (!(EdgeError & 1<<i)) continue;

        errors++;
        UserWriteF("   EDGE(" ID_FMTX " , " ID_FMTX ") is missing\n",
                   ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0))),
                   ID_PRTX(CORNER(theElement,CORNER_OF_EDGE(theElement,i,1))));
      }

    /* evaluate node information */
    if (NodeError)
      for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
      {
        if (NodeError & (1<<i))
        {
          errors++;
          UserWriteF("   CORNER=" ID_FMTX " is BVOBJ,"
                     " ids from elementside "
                     "and vertexsegment are not consistent\n",
                     ID_PRTX(CORNER(theElement,i)));
        }
        if (NodeError & (1<<(i+MAX_CORNERS_OF_ELEM)))
        {
          errors++;
          UserWriteF("   CORNER " ID_FMTX " is IVOBJ, but lies on "
                     "elementside\n",ID_PRTX(CORNER(theElement,i)));
        }
      }

    /* evaluate son information */
    if (ESonError)
    {
      for (i=0; i<NSONS(theElement); i++)
      {
        if ((ESonError & 1<<i))
        {
          errors++;
          UserWriteF("   ESON(%d) has wrong EFATHER "
                     "pointer\n",i);
        }
      }
    }

    if (NSonError)
    {
      for (i=0; i<MAX_CORNERS_OF_ELEM; i++)
      {
        if (NSonError & (1<<i))
        {
          errors++;
          UserWriteF("   SONNODE(CORNER %d) != CORNER(ESON)\n",i);
        }
        if (NSonError & (1<<(i+MAX_CORNERS_OF_ELEM)))
        {
          errors++;
          UserWriteF("   CORNER %d != EFATHER(CORNER(ESON))\n",i);
        }
      }

      for (i=0; i<MAX_EDGES_OF_ELEM; i++)
      {

        if (NSonError & (1<<(i+MAX_CORNERS_OF_ELEM)))
        {
          errors++;
          UserWriteF("   MIDNODE(edge %d) != CORNER(ESON)\n",i);
        }
      }

      if (NSonError & (1<<(MAX_EDGES_OF_ELEM+2*MAX_CORNERS_OF_ELEM)))
      {
        errors++;
        UserWriteF("   NFATHER(CENTERNODE(ESON)) != NULL\n");
      }
    }
  }

  /* look for dead edges */
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    for (theLink=START(theNode); theLink!=NULL; theLink=NEXT(theLink))
    {
      theEdge = MYEDGE(theLink);
      if (!USED(theEdge))
      {
        errors++;
        UserWriteF("edge"
                   ID_FMTX
                   " between " ID_FMTX " and " ID_FMTX
                   " has no element, NO_OF_ELEM=%d \n",
                   ID_PRTX(theEdge),
                   ID_PRTX(theNode),ID_PRTX(NBNODE(theLink)),
                   NO_OF_ELEM(theEdge));

                                #ifdef Debug
        {
          NODE *nb;
          LINK *theLink1;

          nb = NBNODE(theLink);
          UserWriteF("linklist of nbnode %d:",ID(nb));

          for (theLink1=START(nb); theLink1!=NULL;
               theLink1=NEXT(theLink1))
            UserWriteF(" %d-%d",ID(NBNODE(theLink1)),
                       ID(NBNODE(REVERSE(theLink1))));
          UserWrite("\n");
        }
                                #endif
      }
    }
  }

  /* look for dead nodes */
  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {
    if (!USED(theNode))
    {
#if defined  __OVERLAP2__
      IFDEBUG(np,1)
      UserWriteF("Info: node=" ID_FMTX " has no element\n",ID_PRTX(theNode));
      ENDDEBUG
#else
      errors++;
      UserWriteF("node=" ID_FMTX " is dead\n",ID_PRTX(theNode));
#endif
    }
    else
      SETUSED(theNode,0);
  }

  /* check number of elem and their pointers */
  count = 0;
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if (SUCCE(theElement)!=NULL)
    {
      if (OBJT(SUCCE(theElement))!=IEOBJ &&
          OBJT(SUCCE(theElement))!=BEOBJ)
      {
        errors++;
        UserWriteF("pointer of ELEM(" EID_FMTX ") (number %ld) "
                   "to next element is no pointer to an element\n",
                   EID_PRTX(theElement),(long)count);
        break;
      }
      if (PREDE(SUCCE(theElement))!=NULL)
      {
        if (PREDE(SUCCE(theElement))!=theElement)
        {
          errors++;
          UserWriteF("pointer of ELEM(" EID_FMTX ") (number %ld) "
                     "to previous element is not the previous element\n",
                     EID_PRTX(SUCCE(theElement)),(long)(count+1));
        }
      }
                        #ifndef ModelP
      else
      {
        errors++;
        UserWriteF("pointer of ELEM(" EID_FMTX ") (number %ld) "
                   "to previous element is NULL\n",
                   EID_PRTX(SUCCE(theElement)),(long)(count+1));
      }
                        #endif
    }
    count++;
  }

  if (FIRSTELEMENT(theGrid) != NULL)
    if (PREDE(FIRSTELEMENT(theGrid)) != NULL)
    {
      errors++;
      UserWriteF("first element of the grid has a previous 'element'\n");
    }

  if (LASTELEMENT(theGrid) != NULL)
    if (SUCCE(LASTELEMENT(theGrid)) != NULL)
    {
      errors++;
      UserWriteF("last element of the grid has a following 'element'\n");
    }

  if (count != NT(theGrid))
  {
    errors++;
    UserWriteF("there are %ld elements but %ld expected\n",(long)(count),
               (long)NT(theGrid));
  }

  return(errors);
}

static INT CheckElementList (GRID *theGrid)
{
  ELEMENT *theElement;

  if (GLEVEL(theGrid) <= 0) return(0);

  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    ELEMENT *Father = EFATHER(theElement);
    PAR(    INT prio    = EPRIO(theElement);               ) ENDPAR

    if (EMASTER(theElement) && Father==NULL)
    {
      UserWriteF("ERROR: element=" EID_FMTX " has no father\n",
                 EID_PRTX(theElement));
    }
    if (Father == NULL) continue;
    if (theElement == SON(Father,PRIO2INDEX(prio)))
    {
      if (PREDE(theElement) != NULL)
        if (EFATHER(PREDE(theElement))==Father
            PAR(                            && EPRIO(theElement)==EPRIO(PREDE(theElement)) ) ENDPAR )
        {
          UserWriteF(" ERROR element=" EID_FMTX " is not first"
                     "son in list pred elem=" EID_FMTX " father=" EID_FMTX
                     "\n", EID_PRTX(theElement),EID_PRTX(PREDE(theElement)),
                     EID_PRTX(Father));
        }
    }
    else
    {
      if (PREDE(theElement)==NULL ||
          EFATHER(PREDE(theElement))!=Father)
      {
        UserWriteF(" ERROR element=" EID_FMTX " has no"
                   "PREDE with same father=" EID_FMTX
                   "\n", EID_PRTX(theElement),EID_PRTX(Father));
      }
    }
  }
  return (0);
}

INT NS_DIM_PREFIX CheckLists (GRID *theGrid)
{
  /* perform gm dependent check */
  CheckElementList(theGrid);

  /* perform standard list check */
  GRID_CHECK_ELEMENT_LIST(theGrid);
  GRID_CHECK_NODE_LIST(theGrid);
  GRID_CHECK_VERTEX_LIST(theGrid);
  GRID_CHECK_VECTOR_LIST(theGrid);

  return(GM_OK);
}

/****************************************************************************/
/*D
   CheckGrid - Check consistency of data structure

   SYNOPSIS:
   INT CheckGrid (GRID *theGrid, INT checkgeom, INT checkalgebra, INT checklists, INT checkif)

   PARAMETERS:
   .  theGrid - grid to check
   .  checkgeom - check geomtry
   .  checkalgebra - check algebra
   .  checklists - checklists
   .  checkif - check the processor interfaces

   DESCRIPTION:
   This function checks the consistency of data structure.

   RETURN VALUE:
   INT
   .n   GM_OK if ok
   .n   GM_ERROR if an error occured.
   D*/
/****************************************************************************/

#ifndef ModelP
INT NS_DIM_PREFIX CheckGrid (GRID *theGrid, INT checkgeom, INT checkalgebra, INT checklists)
#else
INT NS_DIM_PREFIX CheckGrid (GRID *theGrid, INT checkgeom, INT checkalgebra, INT checklists,
                             INT checkif)
#endif
{
  INT error               = 0;
  INT errors              = 0;
  INT totalerrors = 0;

  if (GetStringValue(":conf:hghost_overlap",&hghost_overlap) !=0)
    UserWriteF("CheckGrid: warning %s not set\n",":conf:hghost_overlap");

  /* check geometrical data structures */
  if (checkgeom)
  {
    UserWrite(" geometry:");

    errors = CheckGeometry(theGrid);
        #ifdef ModelP
    errors = UG_GlobalSumINT(theGrid->ppifContext(), errors);
            #endif
    if (errors != GM_OK)
    {
      totalerrors += errors;
      error++;
      UserWriteF(" geometry BAD: %d errors",errors);
    }
    else
      UserWrite(" ok");
  }

  /* check algebraic data structures */
  if (checkalgebra)
  {
    UserWrite(", algebra:");

    errors = CheckAlgebra(theGrid);
        #ifdef ModelP
    errors = UG_GlobalSumINT(theGrid->ppifContext(), errors);
            #endif
    if (errors != GM_OK)
    {
      totalerrors += errors;
      error++;
      UserWriteF(" algebra BAD: %d errors",errors);
    }
    else
      UserWrite(" ok");
  }

  /* check lists and counters */
  if (checklists)
  {
    UserWrite(", lists:");

    errors = CheckLists(theGrid);
        #ifdef ModelP
    errors = UG_GlobalSumINT(theGrid->ppifContext(), errors);
            #endif
    if (errors != GM_OK)
    {
      totalerrors += errors;
      error++;
      UserWriteF(" lists BAD: %d errors",errors);
    }
    else
      UserWrite(" ok");
  }

        #ifdef ModelP
  /* check interfaces to other procs */
  if (checkif)
  {
    UserWrite(", interface:");

    errors = CheckInterfaces(theGrid);
        #ifdef ModelP
    errors = UG_GlobalSumINT(theGrid->ppifContext(), errors);
            #endif
    if (errors != GM_OK)
    {
      totalerrors += errors;
      error++;
      UserWriteF(" interfaces BAD: %d errors",errors);
    }
    else
      UserWrite(" ok");
  }
        #endif

  if (totalerrors)
    UserWriteF(", grid BAD: %d check(s) with %d totalerror(s)",
               error,totalerrors);
  else
    UserWrite(", grid ok");

  return(error);
}
