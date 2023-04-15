// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/*! \file elements.h
 * \ingroup gm
 */

/****************************************************************************/
/*                                                                          */
/* File:      elements.h                                                    */
/*                                                                          */
/* Purpose:   general element concept (header file)                         */
/*                                                                          */
/* Author:    Peter Bastian                                                 */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   24.03.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __ELEMENTS__
#define __ELEMENTS__

#include <dune/uggrid/low/namespace.h>

#include "gm.h"

START_UGDIM_NAMESPACE

/** \brief Number of different element types    */
#define TAGS 8

#ifdef UG_DIM_2
// The indexing of these arrays must match the definitions of the enum values
// TRIANGLE, QUADRILATERAL, etc in gm.h
constexpr INT n_offset[TAGS] = {-1,-1, -1,
                                (offsetof(triangle,n)      - offsetof(generic_element,refs))/sizeof(void*),
                                (offsetof(quadrilateral,n) - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT father_offset[TAGS] = {-1, -1, -1,
                                     (offsetof(triangle,father)      - offsetof(generic_element,refs))/sizeof(void*),
                                     (offsetof(quadrilateral,father) - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT sons_offset[TAGS] = {-1, -1, -1,
                                   (offsetof(triangle,sons)      - offsetof(generic_element,refs))/sizeof(void*),
                                   (offsetof(quadrilateral,sons) - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT nb_offset[TAGS] = {-1, -1, -1,
                                 (offsetof(triangle,nb)      - offsetof(generic_element,refs))/sizeof(void*),
                                 (offsetof(quadrilateral,nb) - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT side_offset[TAGS] = {-1, -1, -1,
                                   (offsetof(triangle,bnds)      - offsetof(generic_element,refs))/sizeof(void*),
                                   (offsetof(quadrilateral,bnds) - offsetof(generic_element,refs))/sizeof(void*)};
#endif
#ifdef UG_DIM_3
// The indexing of these arrays must match the definitions of the enum values
// TETRAHEDRON, PYRAMID, etc in gm.h
constexpr INT n_offset[TAGS] = {-1, -1, -1, -1,
                                (offsetof(tetrahedron,n) - offsetof(generic_element,refs))/sizeof(void*),
                                (offsetof(pyramid,n)     - offsetof(generic_element,refs))/sizeof(void*),
                                (offsetof(prism,n)       - offsetof(generic_element,refs))/sizeof(void*),
                                (offsetof(hexahedron,n)  - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT father_offset[TAGS] = {-1, -1, -1, -1,
                                     (offsetof(tetrahedron,father) - offsetof(generic_element,refs))/sizeof(void*),
                                     (offsetof(pyramid,father)     - offsetof(generic_element,refs))/sizeof(void*),
                                     (offsetof(prism,father)       - offsetof(generic_element,refs))/sizeof(void*),
                                     (offsetof(hexahedron,father)  - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT sons_offset[TAGS] = {-1, -1, -1, -1,
                                   (offsetof(tetrahedron,sons) - offsetof(generic_element,refs))/sizeof(void*),
                                   (offsetof(pyramid,sons)     - offsetof(generic_element,refs))/sizeof(void*),
                                   (offsetof(prism,sons)       - offsetof(generic_element,refs))/sizeof(void*),
                                   (offsetof(hexahedron,sons)  - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT nb_offset[TAGS] = {-1, -1, -1, -1,
                                 (offsetof(tetrahedron,nb) - offsetof(generic_element,refs))/sizeof(void*),
                                 (offsetof(pyramid,nb)     - offsetof(generic_element,refs))/sizeof(void*),
                                 (offsetof(prism,nb)       - offsetof(generic_element,refs))/sizeof(void*),
                                 (offsetof(hexahedron,nb)  - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT svector_offset[TAGS] = {-1, -1, -1, -1,
                                      (offsetof(tetrahedron,sidevector) - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(pyramid,sidevector)     - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(prism,sidevector)       - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(hexahedron,sidevector)  - offsetof(generic_element,refs))/sizeof(void*)};

constexpr INT side_offset[TAGS] = {-1, -1, -1, -1,
                                      (offsetof(tetrahedron,bnds) - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(pyramid,bnds)     - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(prism,bnds)       - offsetof(generic_element,refs))/sizeof(void*),
                                      (offsetof(hexahedron,bnds)  - offsetof(generic_element,refs))/sizeof(void*)};
#endif

/* the element descriptions are also globally available, these are pointers ! */
extern GENERAL_ELEMENT *element_descriptors[TAGS];

// The element tags, indexed by the number of element vertices
#ifdef UG_DIM_2
constexpr INT reference2tag[MAX_CORNERS_OF_ELEM+1] = {-1, -1, -1,
                                                      TRIANGLE, QUADRILATERAL};
#else
constexpr INT reference2tag[MAX_CORNERS_OF_ELEM+1] = {-1, -1, -1, -1,
                                                      TETRAHEDRON, PYRAMID, PRISM, HEXAHEDRON};
#endif


/****************************************************************************/
/*                                                                          */
/* function definitions                                                     */
/*                                                                          */
/****************************************************************************/

INT InitElementTypes();


END_UGDIM_NAMESPACE


#endif
