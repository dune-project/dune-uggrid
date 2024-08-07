// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  partition.c													*/
/*																			*/
/* Purpose:   check and restrict partitioning for grid adaption             */
/*																			*/
/* Author:	  Stefan Lang                                                                           */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*																			*/
/* History:   970204 sl begin                                               */
/*																			*/
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
   CheckPartitioning - check whether all master copies have master copies of the sons

   SYNOPSIS:
   INT CheckPartitioning (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:
   This function checks whether all master copies of elements which may be involved in next refinement step have master copies of the sons (if existing) at the same processor.

   RETURN VALUE:
   INT
   .n   GM_OK - ok
   .n   GM_ERROR - error
 */
/****************************************************************************/

INT NS_DIM_PREFIX CheckPartitioning (MULTIGRID *theMG)
{
  INT i,_restrict_;
  ELEMENT *theElement;
  ELEMENT *theFather;

  _restrict_ = 0;

  /* reset used flags */
  for (i=TOPLEVEL(theMG); i>0; i--)
  {
    const GRID *theGrid = GRID_ON_LEVEL(theMG,i);
    for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (LEAFELEM(theElement))
      {
        theFather = theElement;
        while (EMASTER(theFather) && ECLASS(theFather)!=RED_CLASS
               && LEVEL(theFather)>0)
        {
          theFather = EFATHER(theFather);
        }

        /* if element with red element class does not exist */
        /* or is ghost -> partitioning must be restricted   */
        if (!EMASTER(theFather))
        {
          UserWriteF("elem=" EID_FMTX  " cannot be refined\n",
                     EID_PRTX(theFather));
          _restrict_ = 1;
          continue;
        }
        if (COARSEN(theFather))
        {
          /* level 0 elements cannot be coarsened */
          if (LEVEL(theFather)<=1) continue;
          if (!EMASTER(EFATHER(theFather)))
          {
            UserWriteF("elem=" EID_FMTX " cannot be coarsened\n",
                       EID_PRTX(theFather));
            _restrict_ = 1;
          }
        }
      }
    }
  }

  _restrict_ = UG_GlobalMaxINT(theMG->ppifContext(), _restrict_);
  if (theMG->dddContext().isMaster() && _restrict_==1)
  {
    UserWriteF("CheckPartitioning(): partitioning is not valid for refinement\n");
    UserWriteF("                     cleaning up ...\n");
  }

  return(_restrict_);
}

/****************************************************************************/
/*
   Gather_ElementRestriction -

   SYNOPSIS:
   static int Gather_ElementRestriction (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Gather_ElementRestriction (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,4,(PFMT "Gather_ElementRestriction(): e=" EID_FMTX "\n",
                   me,EID_PRTX(theElement)))
    ((int *)data)[0] = USED(theElement);

  return(GM_OK);
}


/****************************************************************************/
/*
   Scatter_ElementRestriction -

   SYNOPSIS:
   static int Scatter_ElementRestriction (DDD_OBJ obj, void *data);

   PARAMETERS:
   .  obj
   .  data

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

static int Scatter_ElementRestriction (DDD::DDDContext&, DDD_OBJ obj, void *data)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  PRINTDEBUG(gm,4,(PFMT "Scatter_ElementRestriction(): e=" EID_FMTX "\n",
                   me,EID_PRTX(theElement)))
  if (EMASTER(theElement))
  {
    PRINTDEBUG(gm,4,(PFMT "Scatter_ElementRestriction(): restricting sons of e=" EID_FMTX "\n",
                     me,EID_PRTX(theElement)))
    int used = std::max((INT)USED(theElement),((int *)data)[0]);
    SETUSED(theElement,used);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   Gather_RestrictedPartition -

   SYNOPSIS:
   static int Gather_RestrictedPartition (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Gather_RestrictedPartition (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  if (EMASTER(theElement))
  {
    PRINTDEBUG(gm,4,(PFMT "Gather_RestrictedPartition(): e=" EID_FMTX "\n",
                     me,EID_PRTX(theElement)))
      ((int *)data)[0] = PARTITION(theElement);
  }

  return(GM_OK);
}


/****************************************************************************/
/*
   Scatter_RestrictedPartition -

   SYNOPSIS:
   static int Scatter_RestrictedPartition (DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio);

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

static int Scatter_RestrictedPartition (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;
  ELEMENT *SonList[MAX_SONS];

  if (USED(theElement) && EMASTERPRIO(prio))
  {
    PRINTDEBUG(gm,4,(PFMT "Scatter_ElementRestriction(): restricting sons of e=" EID_FMTX "\n",
                     me,EID_PRTX(theElement)))

    int partition = ((int *)data)[0];
    /* send master sons to master element partition */
    if (GetSons(theElement,SonList)) RETURN(GM_ERROR);
    for (int i = 0; SonList[i] != NULL; i++)
      PARTITION(SonList[i]) = partition;
  }

  return(GM_OK);
}

/****************************************************************************/
/*
   RestrictPartitioning -

   SYNOPSIS:
   INT RestrictPartitioning (MULTIGRID *theMG);

   PARAMETERS:
   .  theMG

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX RestrictPartitioning (MULTIGRID *theMG)
{
  auto& context = theMG->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  INT i,j;
  ELEMENT *theElement;
  ELEMENT *theFather;
  ELEMENT *SonList[MAX_SONS];
  GRID    *theGrid;

  /* reset used flags */
  for (i=TOPLEVEL(theMG); i>=0; i--)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      SETUSED(theElement,0);
    }
  }

  /* set flags on elements which violate restriction */
  for (i=TOPLEVEL(theMG); i>=0; i--)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (GLEVEL(theGrid) == 0) break;
      if (LEAFELEM(theElement) || USED(theElement))
      {
        theFather = theElement;
        while (EMASTER(theFather) && ECLASS(theFather)!=RED_CLASS
               && LEVEL(theFather)>0)
        {
          theFather = EFATHER(theFather);
        }

        /* if father with red refine class is not master */
        /* partitioning must be restricted                */
        if (!EMASTER(theFather))
        {
          /* the sons of father will be sent to partition of father */
          SETUSED(theFather,1);
        }

        /* if element is marked for coarsening and father    */
        /* of element is not master -> restriction is needed */
        if (COARSEN(theFather))
        {
          /* level 0 elements are not coarsened */
          if (LEVEL(theFather)<=1) continue;
          if (!EMASTER(EFATHER(theFather)))
            SETUSED(EFATHER(theFather),1);
        }
      }
    }
    /* transfer restriction flags to master copies of father */
    DDD_IFAOneway(context,
                  dddctrl.ElementVHIF,GRID_ATTR(theGrid),IF_BACKWARD,sizeof(INT),
                  Gather_ElementRestriction, Scatter_ElementRestriction);
  }

  /* send restricted sons to partition of father */
  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);

    /* transfer (new) partitions of elements to non master copies */
    DDD_IFAOnewayX(context,
                   dddctrl.ElementVHIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                   Gather_RestrictedPartition, Scatter_RestrictedPartition);

    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (!USED(theElement)) continue;

      /* push partition to the sons */
      GetAllSons(theElement,SonList);
      for (j=0; SonList[j]!=NULL; j++)
      {
        SETUSED(SonList[j],1);
        if (EMASTER(SonList[j]))
          PARTITION(SonList[j]) = PARTITION(theElement);
      }
    }
  }

  if (TransferGrid(theMG) != 0) RETURN(GM_FATAL);

  return(GM_OK);
}

#endif
