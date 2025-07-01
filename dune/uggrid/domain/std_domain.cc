// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file std_domain.c
 * \ingroup std
 */

/** \addtogroup std
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      std_domain.c                                                  */
/*                                                                          */
/* Purpose:   standard ug domain description                                */
/*                                                                          */
/* Author:    Klaus Johannsen / Christian Wieners                           */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70550 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   Feb 18 1996 begin, ug version 3.1                             */
/*            Sep 12 1996 ug version 3.4                                    */
/*            Apr  9 1998 first step to Marc                                */
/*                                                                          */
/* Remarks:                                                                 */
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

/* standard C library */
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>

/* standard C++ library */
/* set needed in BVP_Init */
#include <set>

#include <dune/common/fvector.hh>

/* low modules */
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/bio.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/fileopen.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/gm/evm.h>

/* dev modules */
#include <dune/uggrid/ugdevices.h>

/* domain module */
#include <dune/uggrid/domain/std_domain.h>

USING_UGDIM_NAMESPACE
  USING_UG_NAMESPACE

#ifdef ModelP
  using namespace PPIF;
#endif

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)	    */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

#define SMALL_DIFF   SMALL_C*100


/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables	                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)	    */
/*                                                                          */
/****************************************************************************/

static STD_BVP *currBVP;


/****************************************************************************/
/* Methods for the class 'boundary_segment'                                 */
/****************************************************************************/

boundary_segment::boundary_segment(INT idA,
                                   const INT* pointsA,
                                   BndSegFuncPtr bndSegFuncA,
                                   void *dataA)
: id(idA), BndSegFunc(bndSegFuncA), data(dataA)
{
  for (INT i = 0; i < CORNERS_OF_BND_SEG; i++)
    points[i] = pointsA[i];
}


/****************************************************************************/
/* Methods for the class 'linear_segment'                                   */
/****************************************************************************/

linear_segment::linear_segment(INT idA,
                               INT nA,
                               const INT * point,
                               const std::array<FieldVector<DOUBLE,DIM>, CORNERS_OF_BND_SEG>& xA)
: id(idA), n(nA), x(xA)
{
  if (n > CORNERS_OF_BND_SEG)
    std::terminate();

  for (int i = 0; i < n; i++)
    points[i] = point[i];
}

UINT NS_DIM_PREFIX GetBoundarySegmentId(BNDS* boundarySegment)
{
  const BND_PS *ps = (BND_PS*)boundarySegment;
  const PATCH *patch = currBVP->patches[ps->patch_id];
  if (patch == nullptr) {
    PrintErrorMessageF ('E', "GetBoundarySegmentId", "invalid argument");
    return 0;
  }

  /* The ids in the patch data structure are consecutive but they
     start at currBVP->sideoffset instead of zero. */
  return PATCH_ID(patch) - currBVP->sideoffset;
}

static INT
GetNumberOfPatches (const PATCH * p)
{
  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    return (1);
  case POINT_PATCH_TYPE :
    return (POINT_PATCH_N (p));
#ifdef UG_DIM_3
  case LINE_PATCH_TYPE :
    return (LINE_PATCH_N (p));
#endif
  }

  return (-1);
}

static INT
GetPatchId (const PATCH * p, INT i)
{
  switch (PATCH_TYPE (p))
  {
  case LINEAR_PATCH_TYPE :
  case PARAMETRIC_PATCH_TYPE :
    return (PATCH_ID (p));
  case POINT_PATCH_TYPE :
    return (POINT_PATCH_PID (p, i));
#ifdef UG_DIM_3
  case LINE_PATCH_TYPE :
    return (LINE_PATCH_PID (p, i));
#endif
  }

  assert(0);
  return (-1);
}

static BNDP *
CreateBndPOnPoint (HEAP * Heap, PATCH * p)
{
  BND_PS *ps;
  PATCH *pp;
  INT j, l, m;

  if (PATCH_TYPE (p) != POINT_PATCH_TYPE)
    return (NULL);

  PRINTDEBUG (dom, 1, ("    p %d\n", PATCH_ID (p)));
  for (l = 0; l < GetNumberOfPatches (p); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p, l)));

  m = POINT_PATCH_N (p);
  ps = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS)
                                     + (m - 1) * sizeof (COORD_BND_VECTOR));
  if (ps == NULL)
    REP_ERR_RETURN (NULL);
  ps->n = m;
  ps->patch_id = PATCH_ID (p);

  for (j = 0; j < m; j++)
  {
    pp = currBVP->patches[POINT_PATCH_PID (p, j)];
    if (PATCH_TYPE (pp) == PARAMETRIC_PATCH_TYPE)
    {
      PRINTDEBUG (dom, 1, ("cp r %f %f %f %f\n",
                           PARAM_PATCH_RANGE (pp)[0][0],
                           PARAM_PATCH_RANGE (pp)[0][1],
                           PARAM_PATCH_RANGE (pp)[1][0],
                           PARAM_PATCH_RANGE (pp)[1][1]));
      switch (POINT_PATCH_CID (p, j))
      {
#ifdef UG_DIM_2
      case 0 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        break;
      case 1 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        break;
#endif
#ifdef UG_DIM_3
      case 0 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[0][1];
        break;
      case 1 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[0][1];
        break;
      case 2 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[1][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[1][1];
        break;
      case 3 :
        ps->local[j][0] = PARAM_PATCH_RANGE (pp)[0][0];
        ps->local[j][1] = PARAM_PATCH_RANGE (pp)[1][1];
        break;
#endif
      }
      PRINTDEBUG (dom, 1, ("mesh loc j %d pid %d cid %d loc %f %f\n",
                           j,
                           POINT_PATCH_PID (p, j),
                           POINT_PATCH_CID (p, j),
                           ps->local[j][0], ps->local[j][1]));
    }
    else if (PATCH_TYPE (pp) == LINEAR_PATCH_TYPE)
    {
      switch (POINT_PATCH_CID (p, j))
      {
#ifdef UG_DIM_2
      case 0 :
        ps->local[j][0] = 0.0;
        break;
      case 1 :
        ps->local[j][0] = 1.0;
        break;
#endif
#ifdef UG_DIM_3
      case 0 :
        ps->local[j][0] = 0.0;
        ps->local[j][1] = 0.0;
        break;
      case 1 :
        ps->local[j][0] = 1.0;
        ps->local[j][1] = 0.0;
        break;
      case 2 :
        /* This coordinate depends on whether we're looking at a triangle or a quadrilateral */
        ps->local[j][0] = (LINEAR_PATCH_N(pp)==3) ? 0.0 : 1.0;
        ps->local[j][1] = 1.0;
        break;
      case 3 :
        ps->local[j][0] = 0.0;
        ps->local[j][1] = 1.0;
        break;
#endif
      }
    }
  }
  return ((BNDP *) ps);
}

static INT
CreateCornerPoints (HEAP * Heap, STD_BVP * theBVP, BNDP ** bndp)
{
  int i;

  for (i = 0; i < theBVP->ncorners; i++)
  {
    bndp[i] = CreateBndPOnPoint (Heap, theBVP->patches[i]);
    if (bndp[i] == NULL)
      REP_ERR_RETURN (1);
  }

  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, (" id %d\n", PATCH_ID (theBVP->patches[i])));

  return (0);
}

#ifdef UG_DIM_3
/** \brief Add a line between two vertices to the boundary data structure

   \param i, j  The line goes from vertex i to vertex j
   \param corners Array with pointers to all vertices
 */
static void
CreateLine(INT i, INT j, HEAP *Heap, PATCH **corners, PATCH **lines, PATCH **sides,
           INT *nlines, INT *err)
{
  INT k, n, m;

  k = 0;
  for (n = 0; n < POINT_PATCH_N (corners[i]); n++)
    for (m = 0; m < POINT_PATCH_N (corners[j]); m++)
      if (POINT_PATCH_PID (corners[i], n) ==
          POINT_PATCH_PID (corners[j], m))
        k++;
  /* points share one patch only and lie on opposite corners of this patch */
  if (k < 2)
    return;

  PATCH* thePatch =
    (PATCH *) GetFreelistMemory (Heap, sizeof (LINE_PATCH)
                                 + (k -
                                    1) * sizeof (struct line_on_patch));
  if (thePatch == NULL)
    return;
  PATCH_TYPE (thePatch) = LINE_PATCH_TYPE;
  PATCH_ID (thePatch) = *nlines;
  LINE_PATCH_C0 (thePatch) = i;
  LINE_PATCH_C1 (thePatch) = j;
  k = 0;
  for (n = 0; n < POINT_PATCH_N (corners[i]); n++)
    for (m = 0; m < POINT_PATCH_N (corners[j]); m++)
      if (POINT_PATCH_PID (corners[i], n) ==
          POINT_PATCH_PID (corners[j], m))
      {
        LINE_PATCH_PID (thePatch, k) =
          POINT_PATCH_PID (corners[i], n);
        LINE_PATCH_CID0 (thePatch, k) =
          POINT_PATCH_CID (corners[i], n);
        LINE_PATCH_CID1 (thePatch, k) =
          POINT_PATCH_CID (corners[j], m);
        k++;
      }
  LINE_PATCH_N (thePatch) = k;

  for (n = 0; n < LINE_PATCH_N (thePatch); n++)
    PRINTDEBUG (dom, 1, (" pid %d cid %d %d",
                         LINE_PATCH_PID (thePatch, n),
                         LINE_PATCH_CID0 (thePatch, n),
                         LINE_PATCH_CID1 (thePatch, n)));
  PRINTDEBUG (dom, 1, ("\n"));

  IFDEBUG (dom, 10) if (k == 2)
  {
    INT o0, o1, s0, s1;

    s0 = LINE_PATCH_PID (thePatch, 0);
    s1 = LINE_PATCH_PID (thePatch, 1);
    o0 =
      (LINE_PATCH_CID0 (thePatch, 0) ==
       ((LINE_PATCH_CID1 (thePatch, 0) + 1) % (2 * DIM_OF_BND)));
    o1 =
      (LINE_PATCH_CID0 (thePatch, 1) ==
       ((LINE_PATCH_CID1 (thePatch, 1) + 1) % (2 * DIM_OF_BND)));
    if (o0 != o1)
    {
      if ((PARAM_PATCH_LEFT (sides[s0]) !=
           PARAM_PATCH_LEFT (sides[s1]))
          || ((PARAM_PATCH_RIGHT (sides[s0]) !=
               PARAM_PATCH_RIGHT (sides[s1]))))
      {
        PRINTDEBUG (dom, 0, ("patch %d and patch %d:"
                             "orientations do not match\n", s0, s1));
        (*err)++;
      }
    }
    else
    {
      if ((PARAM_PATCH_LEFT (sides[s0]) !=
           PARAM_PATCH_RIGHT (sides[s1]))
          || ((PARAM_PATCH_RIGHT (sides[s0]) !=
               PARAM_PATCH_LEFT (sides[s1]))))
      {
        PRINTDEBUG (dom, 0, ("patch %d and patch %d:"
                             "orientations do not match\n", s0, s1));
        (*err)++;
      }
    }
  }
  ENDDEBUG
  lines[(*nlines)++] = thePatch;
  PRINTDEBUG (dom, 1, ("lines id %d type %d n %d\n",
                       PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                       LINE_PATCH_N (thePatch)));


}
#endif

void NS_DIM_PREFIX
BVP_Init (STD_BVP* theBVP, HEAP * Heap, MESH * Mesh, INT MarkKey)
{
  PATCH **corners, **sides;
  unsigned short* segmentsPerPoint, *cornerCounters;
  INT i, j, n, m, ncorners, nlines, nsides;
#       ifdef UG_DIM_3
  PATCH **lines;
  INT err;
  INT nn;
#       endif

  assert(theBVP);
  currBVP = theBVP;

  auto& theDomain = theBVP->Domain;
  assert(theDomain);

  /* fill in data of domain */
  ncorners = theDomain->numOfCorners;
  nsides = theDomain->numOfSegments;

  /* create parameter patches */
  sides = (PATCH **) GetTmpMem (Heap, nsides * sizeof (PATCH *), MarkKey);
  assert (sides);

  for (i = 0; i < nsides; i++)
    sides[i] = NULL;
  theBVP->nsides = nsides;
  for (const boundary_segment& theSegment : theDomain->boundarySegments)
  {
    assert((theSegment.id >= 0) && (theSegment.id < nsides));

    PATCH* thePatch = (PATCH *) GetFreelistMemory (Heap, sizeof (PARAMETER_PATCH));
    assert (thePatch);

    PATCH_TYPE (thePatch) = PARAMETRIC_PATCH_TYPE;
    PATCH_ID (thePatch) = theSegment.id;
    for (i = 0; i < 2 * DIM_OF_BND; i++)
      PARAM_PATCH_POINTS (thePatch, i) = theSegment.points[i];
    // TODO: Remove PARAM_PATCH_RANGE, because it is always (0,1)^d
    for (i = 0; i < DIM_OF_BND; i++)
    {
      PARAM_PATCH_RANGE (thePatch)[0][i] = 0;
      PARAM_PATCH_RANGE (thePatch)[1][i] = 1;
    }
    PARAM_PATCH_BS (thePatch) = theSegment.BndSegFunc;
    PARAM_PATCH_BSD (thePatch) = theSegment.data;
    sides[theSegment.id] = thePatch;
    PRINTDEBUG (dom, 1, ("sides id %d type %d left %d right %d\n",
                         PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                         PARAM_PATCH_LEFT (thePatch),
                         PARAM_PATCH_RIGHT (thePatch)));
    for (i = 0; i < 2 * DIM_OF_BND; i++)
      PRINTDEBUG (dom, 1,
                  ("   corners %d", PARAM_PATCH_POINTS (thePatch, i)));
    PRINTDEBUG (dom, 1, ("\n"));
  }
  for (const linear_segment& theLinSegment : theDomain->linearSegments)
  {
    assert((theLinSegment.id >= 0) && (theLinSegment.id < nsides));

    PATCH* thePatch = (PATCH *) GetFreelistMemory (Heap, sizeof (LINEAR_PATCH));
    assert(thePatch);

    PATCH_TYPE (thePatch) = LINEAR_PATCH_TYPE;
    PATCH_ID (thePatch) = theLinSegment.id;
    LINEAR_PATCH_N (thePatch) = theLinSegment.n;
    for (j = 0; j < theLinSegment.n; j++)
    {
      LINEAR_PATCH_POINTS (thePatch, j) = theLinSegment.points[j];
      for (i = 0; i < DIM; i++)
        LINEAR_PATCH_POS (thePatch, j)[i] = theLinSegment.x[j][i];
    }
    sides[theLinSegment.id] = thePatch;
    PRINTDEBUG (dom, 1, ("sides id %d type %d left %d right %d\n",
                         PATCH_ID (thePatch), PATCH_TYPE (thePatch),
                         LINEAR_PATCH_LEFT (thePatch),
                         LINEAR_PATCH_RIGHT (thePatch)));
  }
  for (i = 0; i < nsides; i++)
    assert(sides[i]);

  /* create point patches */
  corners = (PATCH **) GetTmpMem (Heap, ncorners * sizeof (PATCH *), MarkKey);
  assert(corners);

  theBVP->ncorners = ncorners;

  /* precompute the number of segments at each point patch */
  segmentsPerPoint = (unsigned short*)calloc(ncorners,sizeof(unsigned short));

  for (j = 0; j < nsides; j++) {

    if (PATCH_TYPE (sides[j]) == LINEAR_PATCH_TYPE) {
      for (n = 0; n < LINEAR_PATCH_N (sides[j]); n++)
        segmentsPerPoint[LINEAR_PATCH_POINTS (sides[j], n)]++;

    } else if (PATCH_TYPE (sides[j]) == PARAMETRIC_PATCH_TYPE) {
      for (n = 0; n < 2 * DIM_OF_BND; n++)
        if (PARAM_PATCH_POINTS (sides[j], n) >= 0)       /* Entry may be -1 for triangular faces */
          segmentsPerPoint[PARAM_PATCH_POINTS (sides[j], n)]++;
    }

  }

  /* Allocate point patches */
  for (i = 0; i < ncorners; i++) {

    m = segmentsPerPoint[i];

    PATCH* thePatch =
      (PATCH *) GetFreelistMemory (Heap, sizeof (POINT_PATCH)
                                   + (m-1) * sizeof (struct point_on_patch));
    assert(thePatch);

    PATCH_TYPE (thePatch) = POINT_PATCH_TYPE;
    PATCH_ID (thePatch) = i;

    POINT_PATCH_N (thePatch) = m;
    corners[i] = thePatch;

  }

  cornerCounters = (unsigned short*)calloc(ncorners,sizeof(unsigned short));

  for (j = 0; j < nsides; j++)
  {
    if (PATCH_TYPE (sides[j]) == LINEAR_PATCH_TYPE)
    {
      for (n = 0; n < LINEAR_PATCH_N (sides[j]); n++)
      {
        i = LINEAR_PATCH_POINTS (sides[j], n);
        POINT_PATCH_PID (corners[i], cornerCounters[i]) = j;
        POINT_PATCH_CID (corners[i], cornerCounters[i]++) = n;
      }
    }
    else if (PATCH_TYPE (sides[j]) == PARAMETRIC_PATCH_TYPE)
    {
      for (n = 0; n < 2 * DIM_OF_BND; n++)
      {
        i = PARAM_PATCH_POINTS (sides[j], n);
        if (i>=0 && i<ncorners) {
          POINT_PATCH_PID (corners[i], cornerCounters[i]) = j;
          POINT_PATCH_CID (corners[i], cornerCounters[i]++) = n;
        }
      }
    }
  }

  /* Return memory that will not be used anymore */
  free(segmentsPerPoint);
  free(cornerCounters);

  /* create line patches */
  nlines = 0;
#ifdef UG_DIM_3
  /* The maximum number of boundary lines is equal to the number of
     boundary faces (nsides) times the max number of lines per face (=4)
     divided by two. */
  lines =
    (PATCH **) GetTmpMem (Heap, nsides * 2 * sizeof (PATCH *),
                          MarkKey);
  assert(lines);

  err = 0;

  /* We create the set of all boundary lines by looping over the sides
     and for each side loop over the edges of this side.  That way, we
     meet each boundary line twice.  We cannot assume
     that the sides are properly oriented. Thus we introduce a c++-std::set
     which saves the index pair of the nodes defining the line.
     When a line is visited the second time, the pair is expunged from the
     set in order to save memory.
   */
  typedef std::set<std::pair<long,long> > set;
  set bnd_edges;

  for (int s=0; s<nsides; s++) {

    if (PATCH_TYPE (sides[s]) == LINEAR_PATCH_TYPE) {

      for (nn = 0; nn < LINEAR_PATCH_N (sides[s]); nn++)
      {
        i = LINEAR_PATCH_POINTS (sides[s], nn);
        j = LINEAR_PATCH_POINTS (sides[s], (nn+1)%LINEAR_PATCH_N(sides[s]));

        long max = std::max(i,j);
        long min = i + j - max;
        std::pair<long, long> z(min,max);

        if ( bnd_edges.insert(z).second )       /* true if bnd_edges didn't contain z yet */
        {
          /* Insert the line into the boundary data structure */
          CreateLine(min, max, Heap, corners, lines, sides, &nlines, &err);
        }
        else
          bnd_edges.erase(z);

      }

    } else if (PATCH_TYPE (sides[s]) == PARAMETRIC_PATCH_TYPE) {

      /* Handle edges of parametric patches */

      /* Determine whether the boundary segment is a triangle or a quadrilateral.
         We assume that it is a triangle if the fourth vertex is invalid, and
         a quadrilateral otherwise.  This is the best we can do.
       */

      int cornersOfParametricPatch = (PARAM_PATCH_POINTS (sides[s], 3) < 0 || PARAM_PATCH_POINTS (sides[s], 3) > ncorners) ? 3 : 4;

      for (nn = 0; nn < cornersOfParametricPatch; nn++)
      {
        i = PARAM_PATCH_POINTS (sides[s], nn);
        j = PARAM_PATCH_POINTS (sides[s], (nn+1)%cornersOfParametricPatch);

        long max = std::max(i,j);
        long min = i + j - max;
        std::pair<long, long> z(min,max);

        if ( bnd_edges.insert(z).second )            /* true if bnd_edges didn't contain z yet */
        {
          /* Insert the line into the boundary data structure */
          CreateLine(min, max, Heap, corners, lines, sides, &nlines, &err);
        }
        else
          bnd_edges.erase(z);

      }

    } else
      UserWrite("Error: unknown PATCH_TYPE found for a boundary side!\n");

  }
  ASSERT (err == 0);
#endif

  m = ncorners + nlines;
  theBVP->sideoffset = m;
  theBVP->patches.resize(m+nsides);
  n = 0;
  for (i = 0; i < ncorners; i++)
  {
    PATCH* thePatch = corners[i];
    for (j = 0; j < POINT_PATCH_N (thePatch); j++)
      POINT_PATCH_PID (thePatch, j) += m;
    theBVP->patches[n++] = thePatch;
  }
#ifdef UG_DIM_3
  for (i = 0; i < nlines; i++)
  {
    PATCH* thePatch = lines[i];
    PATCH_ID (thePatch) = n;
    for (j = 0; j < LINE_PATCH_N (thePatch); j++)
      LINE_PATCH_PID (thePatch, j) += m;
    theBVP->patches[n++] = thePatch;
  }
#endif
  for (i = 0; i < nsides; i++)
  {
    PATCH* thePatch = sides[i];
    PATCH_ID (thePatch) = n;
    theBVP->patches[n++] = thePatch;
  }

  PRINTDEBUG (dom, 1, ("ncorners %d\n", theBVP->ncorners));
  for (i = 0; i < theBVP->ncorners; i++)
    PRINTDEBUG (dom, 1, ("   id %d\n", PATCH_ID (theBVP->patches[i])));

  if (Mesh != NULL)
  {
    Mesh->mesh_status = MESHSTAT_CNODES;
    Mesh->nBndP = theBVP->ncorners;
    Mesh->nInnP = 0;
    Mesh->nElements = NULL;
    Mesh->VertexLevel = NULL;
    Mesh->VertexPrio = NULL;
    Mesh->ElementLevel = NULL;
    Mesh->ElementPrio = NULL;
    Mesh->ElemSideOnBnd = NULL;
    Mesh->theBndPs =
      (BNDP **) GetTmpMem (Heap, n * sizeof (BNDP *), MarkKey);
    assert(Mesh->theBndPs);

    CreateCornerPoints (Heap, theBVP, Mesh->theBndPs);

    PRINTDEBUG (dom, 1, ("mesh n %d\n", Mesh->nBndP));
    for (i = 0; i < theBVP->ncorners; i++)
      PRINTDEBUG (dom, 1, (" id %d\n",
                           BND_PATCH_ID ((BND_PS *) (Mesh->theBndPs[i]))));
  }
}

/* domain interface function: for description see domain.h */
NS_DIM_PREFIX std_BoundaryValueProblem::~std_BoundaryValueProblem()
{
  for (auto&& p : patches)
    free (p);
}

void NS_DIM_PREFIX
Set_Current_BVP(STD_BVP *theBVP)
{
  currBVP = theBVP;
}

static INT
GetNumberOfCommonPatches (const PATCH * p0, const PATCH * p1, INT * Pid)
{
  INT i, j, cnt, np0, np1, pid;

  cnt = 0;
  np0 = GetNumberOfPatches (p0);
  np1 = GetNumberOfPatches (p1);
  for (i = 0; i < np0; i++)
  {
    pid = GetPatchId (p0, i);
    for (j = 0; j < np1; j++)
      if (pid == GetPatchId (p1, j))
      {
        if (!cnt)
          *Pid = pid;
        cnt++;
      }
  }

  return (cnt);
}

#ifdef UG_DIM_3
static INT
GetCommonPatchId (const PATCH * p0, const PATCH * p1, INT k)
{
  INT i, j, cnt;

  cnt = 0;
  for (i = 0; i < GetNumberOfPatches (p0); i++)
    for (j = 0; j < GetNumberOfPatches (p1); j++)
      if (GetPatchId (p0, i) == GetPatchId (p1, j))
        if (k == cnt++)
          return (GetPatchId (p1, j));

  return (-1);
}

static INT
GetCommonLinePatchId (PATCH * p0, PATCH * p1)
{
  INT i, k, l, cnt, cnt1;

  if (PATCH_TYPE (p0) == LINE_PATCH_TYPE)
    return (PATCH_ID (p0));
  else if (PATCH_TYPE (p1) == LINE_PATCH_TYPE)
    return (PATCH_ID (p1));

  cnt = GetNumberOfCommonPatches (p0, p1, &i);

  if (cnt < 1)
    return (-1);

  for (k = currBVP->ncorners; k < currBVP->sideoffset; k++)
  {
    PATCH *p = currBVP->patches[k];
    if (LINE_PATCH_N (p) != cnt)
      continue;
    cnt1 = 0;
    for (i = 0; i < cnt; i++)
      for (l = 0; l < LINE_PATCH_N (p); l++)
        if (GetCommonPatchId (p0, p1, i) == LINE_PATCH_PID (p, l))
          cnt1++;
    if (cnt == cnt1)
      return (k);
  }

  return (-1);
}
#endif


/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* functions for BNDS                                                       */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/

static INT
PatchGlobal (const PATCH * p, DOUBLE * lambda, Dune::FieldVector<DOUBLE,DIM>& global)
{
  if (PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
    return ((*PARAM_PATCH_BS (p))(PARAM_PATCH_BSD (p), lambda, global));
  else if (PATCH_TYPE (p) == LINEAR_PATCH_TYPE)
  {
#ifdef UG_DIM_2
    global[0] = (1 - lambda[0]) * LINEAR_PATCH_POS (p, 0)[0]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[0];
    global[1] = (1 - lambda[0]) * LINEAR_PATCH_POS (p, 0)[1]
                + lambda[0] * LINEAR_PATCH_POS (p, 1)[1];
#endif
#ifdef UG_DIM_3

    if (LINEAR_PATCH_N(p) ==3) {
      /* Linear interpolation for a triangle boundary segment */
      int i;
      for (i=0; i<3; i++)
        global[i] = (1 - lambda[0] - lambda[1]) * LINEAR_PATCH_POS (p, 0)[i]
                    + lambda[0] * LINEAR_PATCH_POS (p, 1)[i]
                    + lambda[1] * LINEAR_PATCH_POS (p, 2)[i];

    } else {
      /* Bilinear interpolation for a quadrilateral boundary segment */
      int i;
      for (i=0; i<3; i++)
        global[i] = LINEAR_PATCH_POS (p, 0)[i]
                    + lambda[0]*(LINEAR_PATCH_POS (p, 1)[i] - LINEAR_PATCH_POS (p, 0)[i])
                    + lambda[1]*(LINEAR_PATCH_POS (p, 3)[i] - LINEAR_PATCH_POS (p, 0)[i])
                    + lambda[0]*lambda[1]*(LINEAR_PATCH_POS (p, 0)[i]+LINEAR_PATCH_POS (p, 2)[i]-LINEAR_PATCH_POS (p, 1)[i]-LINEAR_PATCH_POS (p, 3)[i]);
    }
#endif
    return (0);
  }

  return (1);
}

static INT
local2lambda (BND_PS * ps, const Dune::FieldVector<DOUBLE,DIM_OF_BND>& local, DOUBLE lambda[])
{
  const PATCH *p = currBVP->patches[ps->patch_id];

  if ((PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
      || (PATCH_TYPE (p) == LINEAR_PATCH_TYPE))
#ifdef UG_DIM_2
    lambda[0] =
      (1.0 - local[0]) * ps->local[0][0] + local[0] * ps->local[1][0];
#endif
#ifdef UG_DIM_3
    switch (ps->n)
    {
    case 3 :
      lambda[0] = (1.0 - local[0] - local[1]) * ps->local[0][0]
                  + local[0] * ps->local[1][0] + local[1] * ps->local[2][0];
      lambda[1] = (1.0 - local[0] - local[1]) * ps->local[0][1]
                  + local[0] * ps->local[1][1] + local[1] * ps->local[2][1];
      break;
    case 4 :
      lambda[0] = (1.0 - local[0]) * (1.0 - local[1]) * ps->local[0][0]
                  + local[0] * (1.0 - local[1]) * ps->local[1][0]
                  + local[0] * local[1] * ps->local[2][0]
                  + (1.0 - local[0]) * local[1] * ps->local[3][0];
      lambda[1] = (1.0 - local[0]) * (1.0 - local[1]) * ps->local[0][1]
                  + local[0] * (1.0 - local[1]) * ps->local[1][1]
                  + local[0] * local[1] * ps->local[2][1]
                  + (1.0 - local[0]) * local[1] * ps->local[3][1];
      break;
    }
#endif
    else
      return (1);

  return (0);
}

static bool
SideIsCooriented (BND_PS * ps)
{
#       ifdef UG_DIM_2
  if (BND_LOCAL (ps, 1)[0] > BND_LOCAL (ps, 0)[0])
    return true;
  else
    return false;
#       endif

#       ifdef UG_DIM_3
  DOUBLE vp, x0[2], x1[2];

  ASSERT (BND_N (ps) >= 3);

  /* check whether an (arbitrary) angle of the side is > 180 degree */
  V2_SUBTRACT (BND_LOCAL (ps, 1), BND_LOCAL (ps, 0), x0);
  V2_SUBTRACT (BND_LOCAL (ps, 2), BND_LOCAL (ps, 0), x1);
  V2_VECTOR_PRODUCT (x1, x0, vp);

  ASSERT (fabs (vp) > SMALL_C);

  if (vp > SMALL_C)
    return true;
  else
    return false;
#       endif
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDS_BndSDesc (BNDS * theBndS, INT * id, INT * nbid)
{
  BND_PS *ps;
  PATCH *p;
  INT left, right;

  ps = (BND_PS *) theBndS;
  p = currBVP->patches[ps->patch_id];

  if (PATCH_TYPE (p) == PARAMETRIC_PATCH_TYPE)
  {
    left = PARAM_PATCH_LEFT (p);
    right = PARAM_PATCH_RIGHT (p);
  }
  else if (PATCH_TYPE (p) == LINEAR_PATCH_TYPE)
  {
    left = LINEAR_PATCH_LEFT (p);
    right = LINEAR_PATCH_RIGHT (p);
  }
  else
    return (1);

  /* check orientation */
  if (SideIsCooriented (ps))
  {
    /* patch and side are co-oriented */
    *id = left;
    *nbid = right;
  }
  else
  {
    /* patch and side are anti-oriented */
    *id = right;
    *nbid = left;
  }

  return (0);
}

/* domain interface function: for description see domain.h */
BNDP *NS_DIM_PREFIX
BNDS_CreateBndP (HEAP * Heap, BNDS * aBndS, const FieldVector<DOUBLE,DIM_OF_BND>& local)
{
  BND_PS *ps, *pp;

  if (aBndS == NULL)
    return (NULL);

  ps = (BND_PS *) aBndS;

  pp = (BND_PS *) GetFreelistMemory (Heap, sizeof (BND_PS));
  if (pp == NULL)
    return (NULL);

  pp->patch_id = ps->patch_id;
  pp->n = 1;

  if (local2lambda (ps, local, pp->local[0]))
    return (NULL);

  PRINTDEBUG (dom, 1, (" BNDP s %d\n", pp->patch_id));

  return ((BNDP *) pp);
}

/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/* functions for BNDP                                                       */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_Global (const BNDP * aBndP, FieldVector<DOUBLE,DIM>& global)
{
  using std::abs;
  BND_PS *ps;
  PATCH *p, *s;
  INT j, k;
  FieldVector<DOUBLE,DIM> pglobal;

  ps = (BND_PS *) aBndP;
  p = currBVP->patches[ps->patch_id];

  PRINTDEBUG (dom, 1, (" bndp pid %d %d %d\n", ps->patch_id,
                       PATCH_ID (p), PATCH_TYPE (p)));

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    return (PatchGlobal (p, ps->local[0], global));
  case POINT_PATCH_TYPE :

    s = currBVP->patches[POINT_PATCH_PID (p, 0)];

    PRINTDEBUG (dom, 1, (" bndp n %d %d loc %f %f gl \n",
                         POINT_PATCH_N (p),
                         POINT_PATCH_PID (p, 0),
                         ps->local[0][0], ps->local[0][1]));

    PatchGlobal(s, ps->local[0], global);

    for (j = 1; j < POINT_PATCH_N (p); j++)
    {
      s = currBVP->patches[POINT_PATCH_PID (p, j)];

      if (PatchGlobal(s, ps->local[j], pglobal))
        REP_ERR_RETURN (1);

      PRINTDEBUG (dom,1, (" bndp    j %d %d loc %f %f gl %f %f %f\n", j,
                          POINT_PATCH_PID (p, j),
                          ps->local[j][0],
                          ps->local[j][1],
                          pglobal[0], pglobal[1], pglobal[2]));
      for (k = 0; k < DIM; k++)
        if (abs(pglobal[k] - global[k]) > SMALL_DIFF)
          REP_ERR_RETURN (1);
    }

    return (0);
#ifdef UG_DIM_3
  case LINE_PATCH_TYPE :
    s = currBVP->patches[LINE_PATCH_PID (p, 0)];

    if (PatchGlobal(s, ps->local[0], global))
      REP_ERR_RETURN (1);

    PRINTDEBUG (dom, 1, (" bndp    n %d %d loc %f %f gl %f %f %f\n",
                         POINT_PATCH_N (p),
                         LINE_PATCH_PID (p, 0),
                         ps->local[0][0],
                         ps->local[0][1], global[0], global[1], global[2]));
    for (j = 1; j < LINE_PATCH_N (p); j++)
    {
      s = currBVP->patches[LINE_PATCH_PID (p, j)];

      if (PatchGlobal(s, ps->local[j], pglobal))
        REP_ERR_RETURN (1);

      PRINTDEBUG (dom, 1, (" bndp    j %d %d loc %f %f gl %f %f %f\n", j,
                           LINE_PATCH_PID (p, j),
                           ps->local[j][0],
                           ps->local[j][1],
                           pglobal[0], pglobal[1], pglobal[2]));
      for (k = 0; k < DIM; k++)
        if (abs(pglobal[k] - global[k]) > SMALL_DIFF)
          REP_ERR_RETURN (1);
    }
    return (0);
#endif
  }

  REP_ERR_RETURN (1);
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_BndPDesc (BNDP * theBndP, INT * move)
{
  BND_PS *ps;
  PATCH *p;

  ps = (BND_PS *) theBndP;
  p = STD_BVP_PATCH (currBVP, ps->patch_id);

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    *move = DIM_OF_BND;
    return (0);

  case POINT_PATCH_TYPE :
    *move = 0;
    return (0);

#               ifdef UG_DIM_3
  case LINE_PATCH_TYPE :
    *move = 1;
    return (0);
#               endif
  }

  return (1);
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_BndEDesc (BNDP * aBndP0, BNDP * aBndP1)
{
  const BND_PS *bp0 = (BND_PS*)aBndP0;
  const BND_PS *bp1 = (BND_PS*)aBndP1;

  if ((bp0 == nullptr) || (bp1 == nullptr))
    REP_ERR_RETURN (1);

  return (0);
}

/* domain interface function: for description see domain.h */
BNDS *NS_DIM_PREFIX
BNDP_CreateBndS (HEAP * Heap, BNDP ** aBndP, INT n)
{
  BND_PS *bp[4], *bs;
  PATCH *p[4];
  DOUBLE *lambda[4];
  INT i, j, l, pid;
#       ifdef UG_DIM_3
  INT k;
#       endif

  PRINTDEBUG (dom, 1, ("Create BNDS:\n"));
  for (i = 0; i < n; i++)
  {
    bp[i] = (BND_PS *) aBndP[i];
    p[i] = currBVP->patches[bp[i]->patch_id];

    PRINTDEBUG (dom, 1, (" bp %d p %d n %d\n",
                         bp[i]->patch_id, PATCH_ID (p[i]), n));
    for (l = 0; l < GetNumberOfPatches (p[i]); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p[i], l)));
  }

  pid = -1;
#ifdef UG_DIM_2
  if (n != 2)
    return (NULL);
  /* find common patch of boundary points */
  for (i = 0; i < GetNumberOfPatches (p[0]); i++)
    for (j = 0; j < GetNumberOfPatches (p[1]); j++)
    {
      PRINTDEBUG (dom, 1, (" pid i, j %d %d %d %d\n", i, j,
                           GetPatchId (p[0], i), GetPatchId (p[1], j)));
      if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
      {
        pid = GetPatchId (p[0], i);
        lambda[0] = bp[0]->local[i];
        lambda[1] = bp[1]->local[j];
        break;
        PRINTDEBUG (dom, 1, (" pid %d \n", pid));
      }
    }
#endif
#ifdef UG_DIM_3
  switch (n)
  {
  case 3 :
    for (i = 0; i < GetNumberOfPatches (p[0]); i++)
      for (j = 0; j < GetNumberOfPatches (p[1]); j++)
        if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
          for (k = 0; k < GetNumberOfPatches (p[2]); k++)
            if (GetPatchId (p[0], i) == GetPatchId (p[2], k))
            {
              pid = GetPatchId (p[0], i);
              lambda[0] = bp[0]->local[i];
              lambda[1] = bp[1]->local[j];
              lambda[2] = bp[2]->local[k];
              break;
            }
    break;
  case 4 :
    for (i = 0; i < GetNumberOfPatches (p[0]); i++)
      for (j = 0; j < GetNumberOfPatches (p[1]); j++)
        if (GetPatchId (p[0], i) == GetPatchId (p[1], j))
          for (k = 0; k < GetNumberOfPatches (p[2]); k++)
            if (GetPatchId (p[0], i) == GetPatchId (p[2], k))
              for (l = 0; l < GetNumberOfPatches (p[3]); l++)
                if (GetPatchId (p[0], i) == GetPatchId (p[3], l))
                {
                  pid = GetPatchId (p[0], i);
                  lambda[0] = bp[0]->local[i];
                  lambda[1] = bp[1]->local[j];
                  lambda[2] = bp[2]->local[k];
                  lambda[3] = bp[3]->local[l];
                  break;
                }
    break;
  }
#endif

  if (pid == -1)
    return (NULL);

  bs = (BND_PS *) GetFreelistMemory (Heap, (n - 1) * sizeof (COORD_BND_VECTOR)
                                     + sizeof (BND_PS));
  if (bs == NULL)
    return (NULL);
  bs->n = n;
  bs->patch_id = pid;
  for (i = 0; i < n; i++)
    for (j = 0; j < DIM_OF_BND; j++)
      bs->local[i][j] = lambda[i][j];

  for (i = 0; i < n; i++)
    for (j = 0; j < DIM_OF_BND; j++)
      PRINTDEBUG (dom, 1, (" bnds i, j %d %d %f\n", i, j, lambda[i][j]));

  PRINTDEBUG (dom, 1, (" Create BNDS %x %d\n", bs, pid));

  return ((BNDS *) bs);
}

/* domain interface function: for description see domain.h */
BNDP *NS_DIM_PREFIX
BNDP_CreateBndP (HEAP * Heap, BNDP * aBndP0, BNDP * aBndP1, DOUBLE lcoord)
{
  BND_PS *bp0, *bp1, *bp;
  PATCH *p0, *p1;
  INT i, j, k, l, cnt;

  bp0 = (BND_PS *) aBndP0;
  bp1 = (BND_PS *) aBndP1;

  if ((bp0 == NULL) || (bp1 == NULL))
    return (NULL);

  p0 = currBVP->patches[bp0->patch_id];
  p1 = currBVP->patches[bp1->patch_id];

  PRINTDEBUG (dom, 1, ("   bp0 %d pid %d\n", bp0->patch_id, PATCH_ID (p0)));
  for (l = 0; l < GetNumberOfPatches (p0); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
  PRINTDEBUG (dom, 1, ("   bp1 %d pid %d\n", bp1->patch_id, PATCH_ID (p1)));
  for (l = 0; l < GetNumberOfPatches (p1); l++)
    PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

  cnt = GetNumberOfCommonPatches (p0, p1, &i);
  if (cnt == 0)
    return (NULL);

  bp =
    (BND_PS *) GetFreelistMemory (Heap,
                                  (cnt - 1) * sizeof (COORD_BND_VECTOR) +
                                  sizeof (BND_PS));
  if (bp == NULL)
    return (NULL);
  bp->n = cnt;

#ifdef UG_DIM_3
  if (cnt > 1)
  {
    PATCH *p;
    k = GetCommonLinePatchId (p0, p1);
    if ((k < currBVP->ncorners) || (k >= currBVP->sideoffset))
      return (NULL);
    p = currBVP->patches[k];
    bp->patch_id = k;

    PRINTDEBUG (dom, 1, (" Create BNDP line %d cnt %d\n", k, cnt));
    PRINTDEBUG (dom, 1, ("   bp0 %d pid %d\n",
                         bp0->patch_id, PATCH_ID (p0)));
    for (l = 0; l < GetNumberOfPatches (p0); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p0, l)));
    PRINTDEBUG (dom, 1, ("   bp1 %d pid %d\n",
                         bp1->patch_id, PATCH_ID (p1)));
    for (l = 0; l < GetNumberOfPatches (p1); l++)
      PRINTDEBUG (dom, 1, ("    bp pid %d\n", GetPatchId (p1, l)));

    for (l = 0; l < LINE_PATCH_N (p); l++)
    {
      for (i = 0; i < GetNumberOfPatches (p0); i++)
        if (GetPatchId (p0, i) == LINE_PATCH_PID (p, l))
          for (j = 0; j < GetNumberOfPatches (p1); j++)
            if (GetPatchId (p1, j) == LINE_PATCH_PID (p, l))
              for (k = 0; k < DIM_OF_BND; k++)
                bp->local[l][k] = (1.0 - lcoord) * bp0->local[i][k]
                                  + lcoord * bp1->local[j][k];
    }

    for (l = 0; l < LINE_PATCH_N (p); l++)
      PRINTDEBUG (dom, 1, (" Create BNDP line %d l %d %f %f\n",
                           LINE_PATCH_PID (p, l), l,
                           bp->local[l][0], bp->local[l][1]));

    return ((BNDP *) bp);
  }
#endif

  for (i = 0; i < GetNumberOfPatches (p0); i++)
    for (j = 0; j < GetNumberOfPatches (p1); j++)
      if (GetPatchId (p0, i) == GetPatchId (p1, j))
      {
        bp->patch_id = GetPatchId (p0, i);
        for (k = 0; k < DIM_OF_BND; k++)
          bp->local[0][k] = (1.0 - lcoord) * bp0->local[i][k]
                            + lcoord * bp1->local[j][k];
        PRINTDEBUG (dom, 1,
                    (" Create BNDP param %d \n", GetPatchId (p0, i)));

        break;
      }

  return ((BNDP *) bp);
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_SaveInsertedBndP (BNDP * theBndP, char *data, INT max_data_size)
{
  BND_PS *bp;
  PATCH *p;
  INT pid;

  bp = (BND_PS *) theBndP;

  if (bp == NULL)
    return (1);

  pid = bp->patch_id;
  p = currBVP->patches[pid];

  switch (PATCH_TYPE (p))
  {
  case PARAMETRIC_PATCH_TYPE :
  case LINEAR_PATCH_TYPE :
    pid -= currBVP->sideoffset;
    break;
  case POINT_PATCH_TYPE :
    pid = POINT_PATCH_PID (p, 0) - currBVP->sideoffset;
    break;
#ifdef UG_DIM_3
  case LINE_PATCH_TYPE :
    pid = LINE_PATCH_PID (p, 0) - currBVP->sideoffset;
    break;
#endif
  }

  PRINTDEBUG (dom, 1, (" Insert pid %d %d\n", bp->patch_id, pid));

#ifdef UG_DIM_2
  if (snprintf (data, max_data_size, "bn %d %f", pid, (float) bp->local[0][0]) >
      max_data_size)
    return (1);
#endif

#ifdef UG_DIM_3
  if (snprintf (data, max_data_size, "bn %d %f %f", (int) pid,
               (float) bp->local[0][0],
               (float) bp->local[0][1]) > max_data_size)
    return (1);
#endif

  return (0);
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_SurfaceId (BNDP * aBndP, INT * n, INT i)
{
  BND_PS *ps;

  if (i < 0)
    return (1);

  ps = (BND_PS *) aBndP;
  if (ps == NULL)
    return (-1);

  return ps->patch_id;
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_Dispose (HEAP * Heap, BNDP * theBndP)
{
  BND_PS *ps;

  if (theBndP == NULL)
    return (0);

  ps = (BND_PS *) theBndP;
  DisposeMem(Heap, ps);
  return 0;
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDS_Dispose (HEAP * Heap, BNDS * theBndS)
{
  BND_PS *ps;

  if (theBndS == NULL)
    return (0);

  ps = (BND_PS *) theBndS;
  DisposeMem(Heap, ps);
  return 0;
}

/* domain interface function: for description see domain.h */
INT NS_DIM_PREFIX
BNDP_SaveBndP (BNDP * BndP)
{
  BND_PS *bp;
  INT i, j;
  int iList[2];
  double dList[DIM];

  iList[0] = BND_PATCH_ID (BndP);
  iList[1] = BND_N (BndP);
  if (Bio_Write_mint (2, iList))
    return (1);

  bp = (BND_PS *) BndP;
  for (i = 0; i < BND_N (BndP); i++)
  {
    for (j = 0; j < DIM - 1; j++)
      dList[j] = bp->local[i][j];
    if (Bio_Write_mdouble (DIM - 1, dList))
      return (1);
  }

  return (0);
}


INT NS_DIM_PREFIX
BNDP_SaveBndP_Ext (BNDP * BndP)
{
  BND_PS *bp;
  INT i, j;
  int iList[2];
  double dList[DIM];

  /* TODO: save free boundary points */
  iList[0] = BND_PATCH_ID (BndP);
  iList[1] = BND_N (BndP);
  if (Bio_Write_mint (2, iList))
    return (1);

  bp = (BND_PS *) BndP;
  for (i = 0; i < BND_N (BndP); i++)
  {
    for (j = 0; j < DIM - 1; j++)
      dList[j] = bp->local[i][j];
    if (Bio_Write_mdouble (DIM - 1, dList))
      return (1);
  }

  return (0);
}

/* domain interface function: for description see domain.h */
BNDP *NS_DIM_PREFIX
BNDP_LoadBndP (STD_BVP *theBVP, HEAP * Heap)
{
  BND_PS *bp;
  int i, j, pid, n;
  int iList[2];
  double dList[DIM];

  if (Bio_Read_mint (2, iList))
    return (NULL);
  pid = iList[0];
  n = iList[1];
  bp =
    (BND_PS *) GetFreelistMemory (Heap,
                                  (n - 1) * sizeof (COORD_BND_VECTOR) +
                                  sizeof (BND_PS));
  bp->n = n;
  bp->patch_id = pid;
  for (i = 0; i < n; i++)
  {
    if (Bio_Read_mdouble (DIM - 1, dList))
      return (NULL);
    for (j = 0; j < DIM - 1; j++)
      bp->local[i][j] = dList[j];
  }

  return ((BNDP *) bp);
}

BNDP *NS_DIM_PREFIX
BNDP_LoadBndP_Ext (void)
{
  BND_PS *bp;
  int i, j, pid, n;
  int iList[2];
  double dList[DIM - 1];

  if (Bio_Read_mint (2, iList))
    return (NULL);
  pid = iList[0];
  n = iList[1];
  bp =
    (BND_PS *) malloc ((n - 1) * sizeof (COORD_BND_VECTOR) + sizeof (BND_PS));
  bp->n = n;
  bp->patch_id = pid;
  for (i = 0; i < n; i++)
  {
    if (Bio_Read_mdouble (DIM - 1, dList))
      return (NULL);
    for (j = 0; j < DIM - 1; j++)
      bp->local[i][j] = dList[j];
  }

  return ((BNDP *) bp);
}

/** @} */
