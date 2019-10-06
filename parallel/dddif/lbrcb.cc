// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  lbrcb.c														*/
/*																			*/
/* Purpose:   simple static load balancing scheme for testing initial		*/
/*            grid distribution												*/
/*																			*/
/* Author:	  Klaus Birken                                                                          */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: birken@ica3.uni-stuttgart.de							*/
/*																			*/
/* History:   940416 kb  begin                                                                          */
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

#include <algorithm>
#include <vector>

#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

#include "parallel.h"
#include "general.h"
#include <dev/ugdevices.h>
#include <gm/evm.h>
#include <gm/ugm.h>

#include "namespace.h"

USING_UG_NAMESPACES
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


#define SMALL_DOUBLE         1.0E-5      /* resolution when comparing DOUBLEs */



/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

struct LB_INFO {
  ELEMENT *elem;
  DOUBLE center[DIM];
};


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
   sort_rcb_x -

   SYNOPSIS:
   static int sort_rcb_x (const void *e1, const void *e2);

   PARAMETERS:
   .  e1
   .  e2

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

/**
 * compare entities according to center coordinate.
 * This function implements a lexiographic order by the
 * `d0`-th, `d1`-th and `d2`-th component of the center coordinate.
 */
template<int d0, int d1, int d2>
static bool sort_rcb(const LB_INFO& a, const LB_INFO& b)
{
  if (a.center[0] < b.center[0] -SMALL_DOUBLE) return true;
  if (a.center[0] > b.center[0] +SMALL_DOUBLE) return false;

  /* x coordinates are considered to be equal, compare y now */
  if (a.center[1] < b.center[1] -SMALL_DOUBLE) return true;
  if (a.center[1] > b.center[1] +SMALL_DOUBLE) return false;

#ifdef __THREEDIM__
  /* x and y coordinates are considered to be equal, compare y now */
  if (a.center[2] < b.center[2] -SMALL_DOUBLE) return true;
  if (a.center[2] > b.center[2] +SMALL_DOUBLE) return false;
#endif

  return false;
}

/****************************************************************************/
/*
   theRCB - balance all local triangles

   SYNOPSIS:
   static void theRCB (LB_INFO *theItems, int nItems, int px, int py, int dx, int dy, int dim);

   PARAMETERS:
   .  theItems - LB_INFO array
   .  nItems - length of array
   .  px - bottom left position in 2D processor array
   .  py - bottom left position in 2D processor array
   .  dx - size of 2D processor array
   .  dy - size of 2D processor array
   .  dim - sort dimension 0=x, 1=y, 2=z

   DESCRIPTION:
   This function, a simple load balancing algorithm, balances all local triangles using a 'recursive coordinate bisection' scheme,

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void theRCB (const PPIF::PPIFContext& ppifContext, LB_INFO *theItems, int nItems, int px, int py, int dx, int dy, int dim)
{
  int i, part0, part1, ni0, ni1;
  bool (*sort_function)(const LB_INFO&, const LB_INFO&);

  /* determine sort function */
  switch (dim) {
  case 0 :
    sort_function = sort_rcb<0, 1, 2>;
    break;
  case 1 :
    sort_function = sort_rcb<1, 0, 2>;
    break;
                #ifdef __THREEDIM__
  case 2 :
    sort_function = sort_rcb<2, 1, 0>;
    break;
                #endif
  default :
    printf("%d: theRCB(): ERROR no valid sort dimension specified\n", ppifContext.me());
    std::abort();
    break;
  }

  if (nItems==0)
    return;

  if ((dx<=1)&&(dy<=1))
  {
    for(i=0; i<nItems; i++)
    {
      int dest = py*ppifContext.dimX()+px;
      PARTITION(theItems[i].elem) = dest;
    }
    return;
  }

  if (dx>=dy)
  {
    if (nItems>1) std::sort(theItems, theItems + nItems, sort_function);

    part0 = dx/2;
    part1 = dx-part0;

    ni0 = (int)(((double)part0)/((double)(dx))*((double)nItems));
    ni1 = nItems-ni0;

    theRCB(ppifContext, theItems,     ni0, px,       py, part0, dy,(dim+1)%DIM);
    theRCB(ppifContext, theItems+ni0, ni1, px+part0, py, part1, dy,(dim+1)%DIM);

  }
  else
  {
    if (nItems>1) std::sort(theItems, theItems + nItems, sort_function);

    part0 = dy/2;
    part1 = dy-part0;

    ni0 = (int)(((double)part0)/((double)(dy))*((double)nItems));
    ni1 = nItems-ni0;

    theRCB(ppifContext, theItems,     ni0, px, py      , dx, part0,(dim+1)%DIM);
    theRCB(ppifContext, theItems+ni0, ni1, px, py+part0, dx, part1,(dim+1)%DIM);
  }
}





/****************************************************************************/


/****************************************************************************/
/*
   CenterOfMass -

   SYNOPSIS:
   static void CenterOfMass (ELEMENT *e, DOUBLE *pos);

   PARAMETERS:
   .  e
   .  pos

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void CenterOfMass (ELEMENT *e, DOUBLE *pos)
{
  int i;

  V_DIM_CLEAR(pos)

  for(i=0; i<CORNERS_OF_ELEM(e); i++)
  {
    V_DIM_LINCOMB(1.0,pos,1.0,CVECT(MYVERTEX(CORNER(e,i))),pos)
  }

  V_DIM_SCALE(1.0/(float)CORNERS_OF_ELEM(e),pos)
}



/****************************************************************************/


/****************************************************************************/
/*
   InheritPartition -

   SYNOPSIS:
   static void InheritPartition (ELEMENT *e);

   PARAMETERS:
   .  e

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void InheritPartition (ELEMENT *e)
{
  int i;
  ELEMENT *SonList[MAX_SONS];


  if (GetAllSons(e,SonList)==0)
  {
    for(i=0; SonList[i]!=NULL; i++)
    {
      PARTITION(SonList[i]) = PARTITION(e);
      InheritPartition(SonList[i]);
    }
  }
}


/****************************************************************************/
/*
   BalanceGridRCB -

   SYNOPSIS:
   int BalanceGridRCB (MULTIGRID *theMG, int level);

   PARAMETERS:
   .  theMG
   .  level

   DESCRIPTION:

   RETURN VALUE:
   int
 */
/****************************************************************************/

int BalanceGridRCB (MULTIGRID *theMG, int level)
{
  GRID *theGrid = GRID_ON_LEVEL(theMG,level);       /* balance grid of level */
  ELEMENT *e;
  int i;

  DDD::DDDContext& context = theMG->dddContext();
  const PPIF::PPIFContext& ppifContext = theMG->ppifContext();

  /* distributed grids cannot be redistributed by this function */
  if (not context.isMaster() && FIRSTELEMENT(theGrid) != NULL)
  {
    printf("Error: Redistributing distributed grids using recursive coordinate bisection is not implemented!\n");
    return (1);
  }

  if (context.isMaster())
  {
    if (NT(theGrid) == 0)
    {
      UserWriteF("WARNING in BalanceGridRCB: no elements in grid\n");
      return (1);
    }

    /* construct LB_INFO list */
    std::vector<LB_INFO> lbinfo(NT(theGrid));
    for (i=0, e=FIRSTELEMENT(theGrid); e!=NULL; i++, e=SUCCE(e))
    {
      lbinfo[i].elem = e;
      CenterOfMass(e, lbinfo[i].center);
    }

    /* apply coordinate bisection strategy */
    theRCB(ppifContext, lbinfo.data(), lbinfo.size(), 0, 0, ppifContext.dimX(), ppifContext.dimY(), 0);

    IFDEBUG(dddif,1)
    for (e=FIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
    {
      UserWriteF("elem %08x has dest=%d\n",
                 DDD_InfoGlobalId(PARHDRE(e)), PARTITION(e));
    }
    ENDDEBUG

    for (i=0, e=FIRSTELEMENT(theGrid); e!=NULL; i++, e=SUCCE(e))
    {
      InheritPartition (e);
    }
  }

  return 0;
}


/****************************************************************************/

END_UGDIM_NAMESPACE

#endif  /* ModelP */
