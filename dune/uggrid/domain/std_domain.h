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

Domains are created with the function \ref CreateDomain.
*/
struct domain
{
  /** \brief Fields for environment directory. Also stores the name of the domain */
  NS_PREFIX ENVDIR d;

  /** \brief Number of boundary segments */
  INT numOfSegments;

  /** \brief The boundary segments without a parametrization */
  std::vector<linear_segment> linearSegments;

  /** \brief Number of corner points */
  INT numOfCorners;
};


/*----------- typedef for functions ----------------------------------------*/
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

/** \brief ???
 *
 * \todo Please doc me!
 */
typedef INT (*BndCondProcPtr)(void *, void *, DOUBLE *, DOUBLE *, INT *);


/* --- public functions --- */


/* domain definition */
domain                   *CreateDomain                        (const char *name,
                                                               INT segments,
                                                               INT corners);

void RemoveDomain(const char* name);

void   *CreateBoundarySegment       (const char *name,
                                     INT id,
                                     const INT *point,
                                     const DOUBLE *alpha, const DOUBLE *beta,
                                     BndSegFuncPtr BndSegFunc,
                                     void *data);

/** \brief Access the id of the segment (used by DUNE) */
UINT GetBoundarySegmentId(BNDS* boundarySegment);

BVP   *CreateBoundaryValueProblem (const char *BVPname, BndCondProcPtr theBndCond,
                                   int numOfCoeffFct, CoeffProcPtr coeffs[],
                                   int numOfUserFct, UserProcPtr userfct[]);

END_UGDIM_NAMESPACE

/** @} */

#endif
