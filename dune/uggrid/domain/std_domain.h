// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/** \defgroup std The Standard Domain
 * \ingroup dom
 */
/*! \file std_domain.h
 * \ingroup std
 */

/** \addtogroup std
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      std_domain.h                                                  */
/*                                                                          */
/* Purpose:   standard domain declaration                                   */
/*                                                                          */
/* Author:    Peter Bastian/Klaus Johannsen                                 */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70550 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __STD_DOMAIN__
#define __STD_DOMAIN__

#include <array>

#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/namespace.h>

#include "domain.h"

START_UGDIM_NAMESPACE

#undef  CORNERS_OF_BND_SEG
#define CORNERS_OF_BND_SEG               2*DIM_OF_BND


/** \brief Data type of the functions mapping parameter space to world space
 *
 * The first argument of type void * is the user data pointer from
 * the corresponding BOUNDARY_SEGMENT. The second parameter of type DOUBLE *
 * provides an array containing the parameters where the boundary segment
 * function should be evaluated (one number in 2D, two numbers in 3D). The
 * third parameter of type DOUBLE * provides an array where the result can be placed
 * (x,y values in 2D, x,y,z values in 3D).
 */
typedef INT (*BndSegFuncPtr)(void *,DOUBLE *, FieldVector<DOUBLE,DIM>&);


/** \brief Data structure defining part of the boundary of a domain

A domain for UG is described as a set of boundary segments defined
by the \ref boundary_segment structure. Each boundary segment is a mapping
from `d`-1 dimensional parameter space to `d` dimensional Euclidean space.
The parameter space is an interval [0,1] in two dimensions or a square
[0,1]x[0,1] in three-dimensional applications.

For all boundary segments the points in `d` dimensional space corresponding to the parameters
0 and 1 in two dimensions ((0,0),(0,1),(1,0),(1,1) in three dimensions)
are called `corners` of the domain. Locally for each boundary segment the
corners are numbered like shown in the figures above.
The corners are numbered `globally` in a consecutive way beginning with 0.
\warning boundary segments must be defined in such a way that no two
corners are identical!
*/
struct boundary_segment
{
  /** \brief Create a new BOUNDARY_SEGMENT
   *
   * @param  id - id of this boundary segment
   * @param  point - the endpoints of the boundary segment
   * @param  BndSegFunc - function mapping parameters
   * @param  data - user defined space
   */
  boundary_segment(INT id,
                   const INT* point,
                   BndSegFuncPtr BndSegFunc,
                   void *data);

  /** \brief Number of the boundary segment beginning with zero */
  INT id;

  /** \brief Numbers of the vertices (ID)
   *
   * Mapping of local numbers of corners to global numbers. Remember, all
   * global numbers of corners must be different.
   */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief Pointer to a function describing the mapping from parameter space to world space
   */
  BndSegFuncPtr BndSegFunc;

  /** \brief User defined pointer
   *
   * This pointer is passed as the first argument to the BndSegFunc of the segment.
   * This pointer can be used to construct an interface to geometry data files,
   * e.g. from a CAD system.
   */
  void *data;
};


/** \brief Boundary segment with a (multi-)linear geometry
 */
struct linear_segment
{
  /** \brief  Constructor
   *
   * @param  idA id of this boundary segment
   * @param  nA Number of corners
   * @param  point The ids of the vertices that make up the boundary segment
   * @param  xA Coordinates of the vertices
   */
  linear_segment(INT idA,
                 INT nA,
                 const INT * point,
                 const std::array<FieldVector<DOUBLE,DIM>, CORNERS_OF_BND_SEG>& xA);

  /** \brief  Unique id of that segment */
  INT id;

  /** \brief  Number of corners */
  INT n;

  /** \brief  Numbers of the vertices (ID) */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief  Coordinates */
  std::array<FieldVector<DOUBLE,DIM>, CORNERS_OF_BND_SEG> x;
};

/** \brief Data type describing a domain

The \ref domain structure describes a two- or three-dimensional domain (boundary). This geometry
information is used by UG when it refines a grid, i.e., complex geometries are approximated
better when the grid is refined.

A domain is made up of one or several `boundary segments` which are defined by the
\ref boundary_segment structure. The points where boundary segments join are
called corners of the domain. For each corner a \ref node is automatically created.
*/
struct domain
{
  /** \brief Number of boundary segments */
  INT numOfSegments;

  /** \brief The boundary segments with a parametrization */
  std::vector<boundary_segment> boundarySegments;

  /** \brief The boundary segments without a parametrization */
  std::vector<linear_segment> linearSegments;

  /** \brief Number of corner points */
  INT numOfCorners;
};


/* --- public functions --- */

INT STD_BVP_Configure(const std::string& BVPName, std::unique_ptr<domain>&& theDomain);

/** \brief Access the id of the segment (used by DUNE) */
UINT GetBoundarySegmentId(BNDS* boundarySegment);

BVP *CreateBoundaryValueProblem (const char *BVPname);

END_UGDIM_NAMESPACE

/** @} */

#endif
