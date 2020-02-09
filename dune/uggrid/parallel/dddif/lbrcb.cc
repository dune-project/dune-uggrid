// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/* File:	  lbrcb.c														*/
/* Purpose:   simple static load balancing scheme for testing initial		*/
/*            grid distribution												*/
/****************************************************************************/

#ifdef ModelP

#include <config.h>

#include <algorithm>
#include <vector>
#include <array>

#include <dune/common/exceptions.hh>
#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

#include "parallel.h"
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/ugm.h>

USING_UG_NAMESPACES
using namespace PPIF;

START_UGDIM_NAMESPACE

// only used in this source file
struct LB_INFO {
  ELEMENT *elem;
  std::array<DOUBLE, DIM> center;
};

/**
 * compare entities according to center coordinate.
 * This function implements a lexiographic order by the
 * `d0`-th, `d1`-th and `d2`-th component of the center coordinate.
 */
template<int d0, int d1, int d2>
static bool sort_rcb(const LB_INFO& a, const LB_INFO& b)
{
  constexpr DOUBLE eps = 1e-5;

  if (a.center[d0] < b.center[d0] - eps) return true;
  if (a.center[d0] > b.center[d0] + eps) return false;

  /* x coordinates are considered to be equal, compare y now */
  if (a.center[d1] < b.center[d1] - eps) return true;
  if (a.center[d1] > b.center[d1] + eps) return false;

#ifdef __THREEDIM__
  /* x and y coordinates are considered to be equal, compare y now */
  if (a.center[d2] < b.center[d2] - eps) return true;
  if (a.center[d2] > b.center[d2] + eps) return false;
#endif

  return false;
}

/****************************************************************************/
/*
   RecursiveCoordinateBisection - balance all local triangles

   PARAMETERS:
   .  begin - iterator to LB_INFO vector
   .  end - iterator to LB_INFO vector
   .  px - bottom left position in 2D processor array
   .  py - bottom left position in 2D processor array
   .  dx - size of 2D processor array
   .  dy - size of 2D processor array
   .  bisectionAxis - axis along which we sort: 0=x, 1=y, 2=z

   DESCRIPTION:
   This function, a simple load balancing algorithm, balances all local triangles using a 'recursive coordinate bisection' scheme,

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void RecursiveCoordinateBisection (const PPIF::PPIFContext& ppifContext,
                                          const std::vector<LB_INFO>::iterator& begin,
                                          const std::vector<LB_INFO>::iterator& end,
                                          int px, int py, int dx, int dy,
                                          int bisectionAxis)
{
  bool (*sort_function)(const LB_INFO&, const LB_INFO&);

  /* determine sort function */
  switch (bisectionAxis) {
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
  default:
    DUNE_THROW(Dune::Exception, "Rank " << ppifContext.me() << ": "
                     << "RecursiveCoordinateBisection(): Not a valid bisection axis in "
                     << DIM << " dimensions!");
  }

  // empty element range: nothing to do
  if (begin == end)
    return;

  // we found a single destination rank for this element subrange: end recursion
  if ((dx<=1)&&(dy<=1))
  {
    for (auto it = begin; it != end; ++it)
    {
      const int destinationRank = py*ppifContext.dimX()+px;
      PARTITION(it->elem) = destinationRank;
    }
    return;
  }

  if (dx>=dy)
  {
    const int part0 = dx/2;
    const int part1 = dx-part0;

    // partial sort such that middle iterator points to the bisection element
    auto middle = begin + (int)(((double)part0)/((double)(dx))*((double)(end-begin)));
    std::nth_element(begin, middle, end, sort_function);

    const int nextBisectionAxis = (bisectionAxis+1)%DIM;
    RecursiveCoordinateBisection(ppifContext, begin, middle, px, py, part0, dy, nextBisectionAxis);
    RecursiveCoordinateBisection(ppifContext, middle, end, px+part0, py, part1, dy, nextBisectionAxis);

  }
  else
  {
    const int part0 = dy/2;
    const int part1 = dy-part0;

    // partial sort such that middle iterator points to the bisection element
    auto middle = begin + (int)(((double)part0)/((double)(dy))*((double)(end-begin)));
    std::nth_element(begin, middle, end, sort_function);

    const int nextBisectionAxis = (bisectionAxis+1)%DIM;
    RecursiveCoordinateBisection(ppifContext, begin, middle, px, py, dx, part0, nextBisectionAxis);
    RecursiveCoordinateBisection(ppifContext, middle, end, px, py+part0, dx, part1, nextBisectionAxis);
  }
}

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

static void CenterOfMass (ELEMENT *e, std::array<DOUBLE, DIM>& pos)
{
  V_DIM_CLEAR(pos.data())

  for(int i=0; i<CORNERS_OF_ELEM(e); i++)
  {
    V_DIM_LINCOMB(1.0,pos.data(),1.0,CVECT(MYVERTEX(CORNER(e,i))),pos)
  }

  V_DIM_SCALE(1.0/(float)CORNERS_OF_ELEM(e),pos.data())
}

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
  ELEMENT *SonList[MAX_SONS];

  if (GetAllSons(e,SonList)==0)
  {
    for(int i=0; SonList[i]!=NULL; i++)
    {
      PARTITION(SonList[i]) = PARTITION(e);
      InheritPartition(SonList[i]);
    }
  }
}


/****************************************************************************/
/*
   BalanceGridRCB -

   PARAMETERS:
   .  theMG
   .  level

   DESCRIPTION:
   Load balance one level of a multigrid hierarchy
   using recursive coordinate bisection

   RETURN VALUE:
   void
 */
/****************************************************************************/

void BalanceGridRCB (MULTIGRID *theMG, int level)
{
  GRID *theGrid = GRID_ON_LEVEL(theMG,level);
  DDD::DDDContext& context = theMG->dddContext();
  const PPIF::PPIFContext& ppifContext = theMG->ppifContext();

  /* distributed grids cannot be redistributed by this function */
  if (not context.isMaster() && FIRSTELEMENT(theGrid) != NULL)
    DUNE_THROW(Dune::NotImplemented, "Redistributing distributed grids using recursive coordinate bisection is not implemented!");

  if (context.isMaster())
  {
    if (NT(theGrid) == 0)
    {
      UserWriteF("WARNING in BalanceGridRCB: no elements in grid\n");
      return;
    }

    std::vector<LB_INFO> lbinfo(NT(theGrid));
    int i = 0;
    for (auto e=FIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
    {
      lbinfo[i].elem = e;
      CenterOfMass(e, lbinfo[i].center);
      ++i;
    }

    RecursiveCoordinateBisection(ppifContext, lbinfo.begin(), lbinfo.end(), 0, 0, ppifContext.dimX(), ppifContext.dimY(), 0);

IFDEBUG(dddif,1)
    for (auto e=FIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
      UserWriteF("elem %08x has dest=%d\n", DDD_InfoGlobalId(PARHDRE(e)), PARTITION(e));
ENDDEBUG

    for (auto e=FIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
      InheritPartition (e);

  }
}

END_UGDIM_NAMESPACE

#endif  /* ModelP */
