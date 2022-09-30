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

#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/namespace.h>

#include "domain.h"

START_UGDIM_NAMESPACE

#undef  CORNERS_OF_BND_SEG
#define CORNERS_OF_BND_SEG               2*DIM_OF_BND


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
typedef INT (*BndSegFuncPtr)(void *,DOUBLE *,DOUBLE *);

/** \brief ???
 *
 * \todo Please doc me!
 */
typedef INT (*BndCondProcPtr)(void *, void *, DOUBLE *, DOUBLE *, INT *);


/* --- public functions --- */


/* domain definition */
void                     *CreateDomainWithParts       (const char *name,
                                                       INT segments,
                                                       INT corners);

void                     *CreateDomain                        (const char *name,
                                                               INT segments,
                                                               INT corners);

void RemoveDomain(const char* name);

void   *CreateBoundarySegment       (const char *name, INT left, INT right,
                                     INT id,
                                     const INT *point,
                                     const DOUBLE *alpha, const DOUBLE *beta,
                                     BndSegFuncPtr BndSegFunc,
                                     void *data);

void *CreateLinearSegment (const char *name,
                           INT left, INT right,INT id,
                           INT n, const INT *point,
                           DOUBLE x[CORNERS_OF_BND_SEG][DIM]);

/** \brief Access the id of the segment (used by DUNE) */
UINT GetBoundarySegmentId(BNDS* boundarySegment);

BVP   *CreateBoundaryValueProblem (const char *BVPname, BndCondProcPtr theBndCond,
                                   int numOfCoeffFct, CoeffProcPtr coeffs[],
                                   int numOfUserFct, UserProcPtr userfct[]);

END_UGDIM_NAMESPACE

/** @} */

#endif
