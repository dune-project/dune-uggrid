// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  overlap.c														*/
/*																			*/
/* Purpose:   management of grid overlap during adaption                    */
/*																			*/
/* Author:	  Stefan Lang                                                                           */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*																			*/
/* History:   970204 sl begin                                               */
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

/* standard C library */
#include <config.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/* low module */
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

/* dev module */
#include <dune/uggrid/ugdevices.h>

/* gm module */
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/pargm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/rm.h>
#include <dune/uggrid/gm/ugm.h>

/* parallel modules */
#include <dune/uggrid/parallel/ddd/include/ddd.h>
#include <dune/uggrid/parallel/ppif/ppif.h>
#include "identify.h"
#include "parallel.h"

/* UG namespaces: */
USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/* undefine if overlap should be only updated where needed */
/* This does not work since the connection of the overlap needs the
   fatherelements on both sides (ghost and master sons) and this is
   not ensured.
   #define UPDATE_FULLOVERLAP
 */

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
/*
   UpdateElementOverlap -

   SYNOPSIS:
   static INT UpdateElementOverlap (ELEMENT *theElement);

   PARAMETERS:
   .  theElement

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT UpdateElementOverlap (DDD::DDDContext& context, ELEMENT *theElement)
{
  INT i,s,prio;
  INT SonsOfSide,SonSides[MAX_SONS];
  ELEMENT *theNeighbor,*theSon;
  ELEMENT *SonList[MAX_SONS];

  /* yellow_class specific code:                                */
  /* update need to be done for all elements with THEFLAG set,  */
  /* execpt for yellow copies, since their neighbor need not be */
  /* refined (s.l. 971029)                                      */
#ifndef UPDATE_FULLOVERLAP
  if (!THEFLAG(theElement) && REFINECLASS(theElement)!=YELLOW_CLASS) return(GM_OK);
#endif
  /*
          if (!THEFLAG(theElement)) return(GM_OK);
   */

  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
  {
    theNeighbor = NBELEM(theElement,i);
    if (theNeighbor == NULL) continue;

    prio = EPRIO(theNeighbor);
    if (!IS_REFINED(theNeighbor) || !EHGHOSTPRIO(prio)) continue;

    /* yellow_class specific code:                                     */
    /* this is the special situation an update of the element overlap  */
    /* is needed, since the yellow element has now gotten a new yellow */
    /* neighbor (s.l. 971029)                                          */
    /* sending of yellow copies is now done in each situation. To send */
    /* a yellow copy only if needed, THEFLAG(theNeighbor) must be set  */
    /* properly in AdaptGrid() (980114 s.l.)                          */
                #ifndef UPDATE_FULLOVERLAP
    if ((REFINECLASS(theElement)==YELLOW_CLASS && !THEFLAG(theElement)) &&
        !THEFLAG(theNeighbor)) continue;
                #endif

    PRINTDEBUG(gm,1,("%d: EID=%d side=%d NbID=%d " "NbPARTITION=%d\n",me,
                     ID(theElement),i,ID(theNeighbor), EPROCPRIO(context, theNeighbor,PrioMaster)))

    Get_Sons_of_ElementSide(theElement,i,&SonsOfSide,
                            SonList,SonSides,1,0);
    PRINTDEBUG(gm,1,("%d: SonsOfSide=%d\n",me,SonsOfSide))

    for (s=0; s<SonsOfSide; s++)
    {
      theSon = SonList[s];
      ASSERT(theSon != NULL);

      PRINTDEBUG(gm,1,("%d: Sending Son=%08x/%x SonID=%d "
                       "SonLevel=%d to dest=%d\n", me,EGID(theSon),theSon,
                       ID(theSon),LEVEL(theSon), EPROCPRIO(context, theNeighbor,PrioMaster)))

      if (EPROCPRIO(context, theNeighbor,PrioMaster)>=context.procs()) break;

      XFERECOPYX(context, theSon,EPROCPRIO(context, theNeighbor,PrioMaster),PrioHGhost,
                 (OBJT(theSon)==BEOBJ) ? BND_SIZE_TAG(TAG(theSon)) :
                 INNER_SIZE_TAG(TAG(theSon)));
      /* send son to all elements where theNeighbor is master, vghost or vhghost */
      if (0)
      {
        for (auto&& [proc, currentPrio] : DDD_InfoProcListRange(context, PARHDRE(theNeighbor), false))
        {
          if (!EHGHOSTPRIO(currentPrio))
          {
            XFERECOPYX(context, theSon,proc,PrioHGhost,
                       (OBJT(theSon)==BEOBJ) ? BND_SIZE_TAG(TAG(theSon)) :
                       INNER_SIZE_TAG(TAG(theSon)));
          }
        }
      }
    }
  }

  return(GM_OK);
}

/****************************************************************************/
/*
   UpdateGridOverlap -

   SYNOPSIS:
   INT UpdateGridOverlap (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT UpdateGridOverlap (GRID *theGrid)
{
  DDD::DDDContext& context = theGrid->dddContext();
  ELEMENT *theElement;

  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    if (IS_REFINED(theElement))
      UpdateElementOverlap(context, theElement);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   UpdateMultiGridOverlap -

   SYNOPSIS:
   static INT UpdateMultiGridOverlap (MULTIGRID *theMG, INT FromLevel);

   PARAMETERS:
   .  theMG
   .  FromLevel

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT UpdateMultiGridOverlap (MULTIGRID *theMG, INT FromLevel)
{
  ddd_HandlerInit(theMG->dddContext(), HSET_REFINE);

  for (INT l = FromLevel; l < TOPLEVEL(theMG); l++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,l);
    UpdateGridOverlap(theGrid);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   DropUsedFlags -

   SYNOPSIS:
   static INT DropUsedFlags (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT DropUsedFlags (GRID *theGrid)
{
  ELEMENT *theElement;

  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    if (USED(theElement) == 1)
    {
      REFINE_ELEMENT_LIST(1,theElement,"drop mark");

      ASSERT(EFATHER(theElement)!=NULL);

      /* this father has to be connected */
      SETUSED(EFATHER(theElement),1);
      SETUSED(theElement,0);
    }
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   ConnectGridOverlap -

   SYNOPSIS:
   INT	ConnectGridOverlap (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT ConnectGridOverlap (GRID *theGrid)
{
  INT i,j,Sons_of_Side,prio;
  INT SonSides[MAX_SIDE_NODES];
  ELEMENT *theElement;
  ELEMENT *theNeighbor;
  ELEMENT *theSon;
  ELEMENT *Sons_of_Side_List[MAX_SONS];

  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    prio = EPRIO(theElement);

    /* connect only FROM hgost copies */
    if (!IS_REFINED(theElement) || !EHGHOSTPRIO(prio)) continue;

    PRINTDEBUG(gm,1,("Connecting e=%08x/%x ID=%d eLevel=%d\n",
                     DDD_InfoGlobalId(PARHDRE(theElement)),
                     theElement,ID(theElement),
                     LEVEL(theElement)));

    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      if (OBJT(theElement)==BEOBJ
          && SIDE_ON_BND(theElement,i)
          && !INNER_BOUNDARY(theElement,i)) continue;

      theNeighbor = NBELEM(theElement,i);
      if (theNeighbor == NULL) continue;

      prio = EPRIO(theNeighbor);
      /* overlap situation hasn't changed */
      if (!THEFLAG(theElement) && !THEFLAG(theNeighbor)) continue;

      /* connect only TO master copies */
                        #ifdef UG_DIM_2
      if (!IS_REFINED(theNeighbor) || !MASTERPRIO(prio)) continue;
                        #endif
                        #ifdef UG_DIM_3
      if (!IS_REFINED(theNeighbor)) continue;
                        #endif

      if (Get_Sons_of_ElementSide(theElement,i,&Sons_of_Side,
                                  Sons_of_Side_List,SonSides,1,0)!=GM_OK) RETURN(GM_FATAL);

      IFDEBUG(gm,1)
      UserWriteF("                 side=%d NSONS=%d Sons_of_Side=%d:\n",
                 i,NSONS(theElement),Sons_of_Side);
      for (j=0; j<Sons_of_Side; j++)
        UserWriteF("            son=%08x/%x sonside=%d\n",
                   EGID(Sons_of_Side_List[j]),
                   Sons_of_Side_List[j],SonSides[j]);
      printf("        connecting ghostelements:\n");
      ENDDEBUG

      /* the ioflag=1 is needed, since not all sended ghosts are needed! */
      if (Connect_Sons_of_ElementSide(theGrid,theElement,i,
                                      Sons_of_Side,Sons_of_Side_List,SonSides,1)!=GM_OK)
        RETURN(GM_FATAL);
    }

    /* 1. yellow_class specific code:                          */
    /* check whether is a valid ghost, which as in minimum one */
    /* master element as neighbor                              */
    /* TODO: move this functionality to ComputeCopies          */
    /* then disposing of theSon can be done in AdaptGrid       */
    /* and the extra Xfer env around ConnectGridOverlap()      */
    /* can be deleted (s.l. 971029)                            */

    /* 2. ghost-ghost neighborship specific code:              */
    /* reset in 3D all unsymmetric neighbor relationships      */
    /* to avoid referencing of zombie pointers.                */
    /* this happened e.g. in CorrectElementSidePattern()       */
    /* (s.l. 980223)                                           */
    {
      ELEMENT *SonList[MAX_SONS];

      GetAllSons(theElement,SonList);
      for (i=0; SonList[i]!=NULL; i++)
      {
        INT ok = 0;
        theSon = SonList[i];
        if (!EHGHOST(theSon)) continue;
        for (j=0; j<SIDES_OF_ELEM(theSon); j++)
        {
          ELEMENT *NbSon = NBELEM(theSon,j);

          if (NbSon == NULL) continue;

          if (EMASTER(NbSon))
          {
            ok = 1;
          }
          /* reset unsymmetric pointer relation ship */
          /* TODO: delete this is done in ElementObjMkCons()
                                                  else
                                                  {
                                                          INT k;
                                                          for (k=0; k<SIDES_OF_ELEM(NbSon); k++)
                                                          {
                                                                  if (NBELEM(NbSon,k)==theSon) break;
                                                          }
                                                          if (k>=SIDES_OF_ELEM(NbSon)) SET_NBELEM(theSon,j,NULL);
                                                  }
           */
        }
        if (!ok)
        {
          if (ECLASS(theSon) == YELLOW_CLASS)
          {
            UserWriteF("ConnectGridOverlap(): disposing useless yellow ghost  e=" EID_FMTX
                       "f=" EID_FMTX "this ghost is useless!\n",
                       EID_PRTX(theSon),EID_PRTX(theElement));
            DisposeElement(UPGRID(theGrid),theSon);
          }
          else
          {
            UserWriteF("ConnectGridOverlap(): ERROR e=" EID_FMTX
                       "f=" EID_FMTX "this ghost is useless!\n",
                       EID_PRTX(theSon),EID_PRTX(theElement));

            /* TODO: better do this
               assert(0); */
          }
        }
      }
    }
  }

  return(GM_OK);
}



/****************************************************************************/
/*
   ConnectMultiGridOverlap -

   SYNOPSIS:
   static INT	ConnectMultiGridOverlap (MULTIGRID *theMG, INT FromLevel);

   PARAMETERS:
   .  theMG
   .  FromLevel

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT      ConnectMultiGridOverlap (MULTIGRID *theMG, INT FromLevel)
{
  INT l;
  GRID *theGrid;

  /* drop used marks to fathers */
  for (l=FromLevel+1; l<=TOPLEVEL(theMG); l++)
  {
    theGrid = GRID_ON_LEVEL(theMG,l);
    if (DropUsedFlags(theGrid)) RETURN(GM_FATAL);
  }

  /* connect sons of elements with used flag set */
  for (l=FromLevel; l<TOPLEVEL(theMG); l++)
  {

    theGrid = GRID_ON_LEVEL(theMG,l);
    if (ConnectGridOverlap(theGrid)) RETURN(GM_FATAL);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   ConnectVerticalOverlap - reconstruct fathers and sons of HGHOSTS

   SYNOPSIS:
   INT	ConnectVerticalOverlap (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG - pointer to multigrid

   DESCRIPTION:
   IO and Loadbalancing to not consider father-son relations of HGHOSTS;
   here, this information is reconstructed.

   RETURN VALUE:
   INT

   GM_OK  if ok.
 */
/****************************************************************************/

static INT CompareSide (ELEMENT *theElement, INT s, ELEMENT *theFather, INT t)
{
  NODE *Nodes[MAX_SIDE_NODES];
  INT n;
  INT m = CORNERS_OF_SIDE(theElement,s);
  INT k = 0;
  INT i,j;

  GetSonSideNodes(theFather,t,&n,Nodes,0);

  for (i=0; i<m; i++)
  {
    NODE *theNode = CORNER(theElement,CORNER_OF_SIDE(theElement,s,i));

    for (j=0; j<MAX_SIDE_NODES; j++)
      if (theNode == Nodes[j]) {
        k++;
        break;
      }
  }

  return((k == m));
}

INT ConnectVerticalOverlap (MULTIGRID *theMG)
{
  INT l;

  for (l=1; l<=TOPLEVEL(theMG); l++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,l);
    ELEMENT     *theElement;

    for (theElement=PFIRSTELEMENT(theGrid);
         theElement!=NULL; theElement=SUCCE(theElement))
    {
      INT prio = EPRIO(theElement);
      bool neflag = false;
      INT i;

      if (prio == PrioMaster) break;
      if (prio == PrioVGhost) continue;
      if (EFATHER(theElement) != NULL) continue;
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        ELEMENT *theNeighbor = NBELEM(theElement,i);

        if (theNeighbor == NULL) continue;
        if (EMASTER(theNeighbor))
        {
          ELEMENT *theFather = EFATHER(theNeighbor);
          INT j;

          ASSERT(theFather != NULL);
          for (j=0; j<SIDES_OF_ELEM(theFather); j++)
          {
            ELEMENT *el = NBELEM(theFather,j);

            if (el == NULL) continue;
            if (EMASTER(el)) continue;
            if (EVGHOST(el)) continue;
            if (CompareSide(theElement,i,theFather,j))
            {
              INT where = PRIO2INDEX(EPRIO(theElement));
              PRINTDEBUG(dddif,0,(PFMT " ConnectVerticalOverlap "
                                  " e=" EID_FMTX
                                  " n=" EID_FMTX
                                  " nf=" EID_FMTX
                                  " f=" EID_FMTX "\n",
                                  me,
                                  EID_PRTX(theElement),
                                  EID_PRTX(theNeighbor),
                                  EID_PRTX(theFather),
                                  EID_PRTX(el)));
              SET_EFATHER(theElement,el);
              if (NSONS(el) == 0)
              {
                SET_SON(el,where,theElement);
                /* Father found, hence exit loop. Otherwise multiple fixes lead to element list inconsistency
                   and endless element loop */
                /* Achim 030506 */
                neflag = true;
                break;
              }
              else
              {
                ELEMENT *theSon = SON(el,where);

                assert(PRIO2INDEX(EPRIO(theSon)) == where);
                GRID_UNLINK_ELEMENT(theGrid,theElement);
                GRID_LINKX_ELEMENT(theGrid,theElement,
                                   EPRIO(theElement),theSon);
                /* Father found, hence exit loop. Otherwise multiple fixes lead to element list inconsistency
                   and endless element loop */
                /* Achim 030506 */
                neflag = true;
                break;
              }
              SETNSONS(el,NSONS(el)+1);
            }
          }
          /* Father found, hence exit loop. Otherwise multiple fixes lead to element list inconsistency
             and endless element loop */
          /* Achim 030506 */
          if (neflag == true)
            break;
        }
      }
    }
  }

  return(GM_OK);
}

static INT ConnectOverlapVerticalGrid (GRID *theGrid)
{
  INT i,j,k,found,edgenode0,edgenode1;
  ELEMENT *theElement,*theSon,*SonList[MAX_SONS];
  VERTEX  *theVertex;
  DOUBLE diff;
  DOUBLE_VECTOR global;

  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    /* reconstruct node relations using element relations */
    if (GetAllSons(theElement,SonList) != GM_OK) REP_ERR_RETURN(1);
    for (i=0; SonList[i]!=NULL; i++)
    {
      theSon = SonList[i];
      for (j=0; j<CORNERS_OF_ELEM(theSon); j++)
      {
        found = 0;
        NODE *SonNode = CORNER(theSon, j);
        switch(NTYPE(SonNode))
        {
        case CORNER_NODE :
        {
          const NODE *FatherNode = (NODE *) NFATHER(SonNode);
          if (FatherNode != nullptr)
          {
            assert(SONNODE(FatherNode) == SonNode);
            break;
          }
          assert(!MOVED(MYVERTEX(SonNode)));
          for (k=0; k<CORNERS_OF_ELEM(theElement); k++)
          {
            NODE *theNode = CORNER(theElement,k);
            if (MYVERTEX(theNode) == MYVERTEX(SonNode))
            {
              assert(found == 0);
              assert (SONNODE(theNode)==NULL ||
                      SONNODE(theNode) == SonNode);
              printf("ConnectOverlapVerticalGrid(): new "
                     " sonnode relation between theNode=" ID_FMTX
                     " SonNode=" ID_FMTX "\n",
                     ID_PRTX(theNode),ID_PRTX(SonNode));
              SETNFATHER(SonNode,(GEOM_OBJECT *)theNode);
              SONNODE(theNode) = SonNode;
              found ++;
            }
          }
          break;
        }
        case MID_NODE :
        {
          const EDGE *FatherEdge = (EDGE *) NFATHER(SonNode);
          if (FatherEdge != nullptr)
          {
            assert(MIDNODE(FatherEdge) == SonNode);
            break;
          }
          assert(!MOVED(MYVERTEX(SonNode)));
          for (k=0; k<EDGES_OF_ELEM(theElement); k++)
          {
            edgenode0 = CORNER_OF_EDGE(theElement,k,0);
            edgenode1 = CORNER_OF_EDGE(theElement,k,1);
            const NODE *EdgeNode0 = CORNER(theElement, edgenode0);
            const NODE *EdgeNode1 = CORNER(theElement, edgenode1);
            assert(EdgeNode0!=NULL && EdgeNode1!=NULL);

            EDGE *theEdge = GetEdge(EdgeNode0, EdgeNode1);
            assert(theEdge != nullptr);
            const FieldVector<DOUBLE,DIM>& songlobal = CVECT(MYVERTEX(SonNode));
            V_DIM_LINCOMB(0.5, CVECT(MYVERTEX(EdgeNode0)),
                          0.5, CVECT(MYVERTEX(EdgeNode1)),global);
            V_DIM_EUKLIDNORM_OF_DIFF(songlobal,global,diff);
            if (diff <= MAX_PAR_DIST)
            {
              assert(found == 0);
              assert (MIDNODE(theEdge)==NULL ||
                      MIDNODE(theEdge) == SonNode);

                                                                #ifdef UG_DIM_2
              IFDEBUG(dddif,1)
              printf("ConnectOverlapVerticalGrid(): new "
                     " midnode relation between theEdge=%p"
                     " SonNode=" ID_FMTX "Vertex=" VID_FMTX "\n",
                     (void*) theEdge,ID_PRTX(SonNode),
                     VID_PRTX(MYVERTEX(SonNode)));
              ENDDEBUG
                                                                #endif
                                                                #ifdef UG_DIM_3
              printf("ConnectOverlapVerticalGrid(): new "
                     " midnode relation between theEdge=" ID_FMTX
                     " SonNode=" ID_FMTX "\n",
                     ID_PRTX(theEdge),ID_PRTX(SonNode));
                                                                #endif
              SETNFATHER(SonNode,(GEOM_OBJECT *)theEdge);
              MIDNODE(theEdge) = SonNode;
              found ++;

              /* reconstruct vertex information */
              theVertex = MYVERTEX(SonNode);
              V_DIM_LINCOMB(0.5, LOCAL_COORD_OF_ELEM(theElement,edgenode0),
                            0.5, LOCAL_COORD_OF_ELEM(theElement,edgenode1),
                            LCVECT(theVertex));
              SETONEDGE(theVertex,k);
              VFATHER(theVertex) = theElement;
            }
          }
          break;
        }
        case SIDE_NODE :
        case CENTER_NODE :
          /* do nothing */
          break;
        default :
          assert(0);
        }
      }
    }
  }

  return(GM_OK);
}

static INT ConnectOverlapVerticalMultiGrid (MULTIGRID *theMG)
{
  for (INT i = 0; i <= TOPLEVEL(theMG); i++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,i);
    if (ConnectOverlapVerticalGrid(theGrid)) return(GM_ERROR);
  }
  return(GM_OK);
}

END_UGDIM_NAMESPACE

#endif
