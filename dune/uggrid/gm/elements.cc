// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      elements.c                                                    */
/*																			*/
/* Purpose:   implements a general element concept							*/
/*																			*/
/* Author:	  Peter Bastian, Stefan Lang                                                                    */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de							    */
/*																			*/
/* History:   24.03.95 begin, ug version 3.0                                */
/*			  18.03.96 ug3.1                                                                */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

#include <config.h>
#include <cassert>

#include <dune/uggrid/ugdevices.h>

#include "gm.h"
#include "ugm.h"

#ifdef ModelP
#include <dune/uggrid/parallel/dddif/parallel.h>
#endif

#include "elements.h"

USING_UG_NAMESPACE
USING_UGDIM_NAMESPACE

/****************************************************************************/
/*													*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

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

GENERAL_ELEMENT * NS_DIM_PREFIX element_descriptors[TAGS];

/****************************************************************************/
/*																			*/
/* definition of local variables											*/
/*																			*/
/****************************************************************************/

#ifdef UG_DIM_2
static GENERAL_ELEMENT def_triangle = {
  .tag = TRIANGLE,
  .sides_of_elem = 3,
  .corners_of_elem = 3,
  .local_corner = {{0.0,0.0},{1.0,0.0},{0.0,1.0}},
  .edges_of_elem = 3,
  .edges_of_side = {1,1,1,-1},
  .corners_of_side = {2,2,2,-1},
  .edge_of_side = {{0,-1,-1},{1,-1,-1},{2,-1,-1},{-1,-1,-1}},
  .corner_of_side = {{0,1,-1},{1,2,-1},{2,0,-1},{-1,-1,-1}},
  .corner_of_edge = {{0,1},{1,2},{2,0},{-1,-1},{-1,-1},{-1,-1}}
} ;

static GENERAL_ELEMENT def_quadrilateral = {
  .tag = QUADRILATERAL,
  .sides_of_elem = 4,
  .corners_of_elem = 4,
  .local_corner = {{0.0,0.0},{1.0,0.0},{1.0,1.0},{0.0,1.0}},
  .edges_of_elem = 4,
  .edges_of_side = {1,1,1,1},
  .corners_of_side = {2,2,2,2},
  .edge_of_side = {{0,-1,-1},{1,-1,-1},{2,-1,-1},{3,-1,-1}},
  .corner_of_side = {{0,1,-1},{1,2,-1},{2,3,-1},{3,0,-1}},
  .corner_of_edge = {{0,1},{1,2},{2,3},{3,0},{-1,-1},{-1,-1}}
} ;
#endif

#ifdef UG_DIM_3
static GENERAL_ELEMENT def_tetrahedron = {
  .tag = TETRAHEDRON,
  .sides_of_elem = 4,
  .corners_of_elem = 4,
  .local_corner = {{0.0,0.0,0.0},{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}},
  .edges_of_elem = 6,
  .edges_of_side = {3,3,3,3,-1,-1},
  .corners_of_side = {3,3,3,3,-1,-1},
  .edge_of_side = {{2,1,0,-1},{1,5,4,-1},{3,5,2,-1},{0,4,3,-1}},
  .corner_of_side = {{0,2,1,-1},{1,2,3,-1},{0,3,2,-1},{0,1,3,-1}},
  .corner_of_edge = {{0,1},{1,2},{0,2},{0,3},{1,3},{2,3} }
} ;

static GENERAL_ELEMENT def_pyramid = {
  .tag = PYRAMID,
  .sides_of_elem = 5,
  .corners_of_elem = 5,
  .local_corner = {{0.0,0.0,0.0},{1.0,0.0,0.0},{1.0,1.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}},
  .edges_of_elem = 8,
  .edges_of_side = {4,3,3,3,3,-1},
  .corners_of_side = {4,3,3,3,3,-1},
  .edge_of_side = {{3,2,1,0},{0,5,4,-1},{1,6,5,-1},{2,7,6,-1},{3,4,7,-1}},
  .corner_of_side = {{0,3,2,1},{0,1,4,-1},{1,2,4,-1},{2,3,4,-1},{3,0,4,-1}},
  .corner_of_edge = {{0,1},{1,2},{2,3},{3,0},{0,4},{1,4},{2,4},{3,4}}
} ;

static GENERAL_ELEMENT def_prism = {
  .tag = PRISM,
  .sides_of_elem = 5,
  .corners_of_elem = 6,
  .local_corner = {{0.0,0.0,0.0},{1.0,0.0,0.0},{0.0,1.0,0.0},
   {0.0,0.0,1.0},{1.0,0.0,1.0},{0.0,1.0,1.0}},
  .edges_of_elem = 9,
  .edges_of_side = {3,4,4,4,3,-1},
  .corners_of_side = {3,4,4,4,3,-1},
  .edge_of_side = {{2,1,0,-1},{0,4,6,3},{1,5,7,4},{2,3,8,5},{6,7,8,-1}},
  .corner_of_side = {{0,2,1,-1},{0,1,4,3},{1,2,5,4},{2,0,3,5},{3,4,5,-1}},
  .corner_of_edge = {{0,1},{1,2},{2,0},{0,3},{1,4},{2,5},{3,4},{4,5},{5,3}}
} ;

static GENERAL_ELEMENT def_hexahedron = {
  .tag = HEXAHEDRON,
  .sides_of_elem = 6,
  .corners_of_elem = 8,
  .local_corner = {{0.0,0.0,0.0},{1.0,0.0,0.0},
   {1.0,1.0,0.0},{0.0,1.0,0.0},
   {0.0,0.0,1.0},{1.0,0.0,1.0},
   {1.0,1.0,1.0},{0.0,1.0,1.0}},
  .edges_of_elem = 12,
  .edges_of_side = {4,4,4,4,4,4},
  .corners_of_side = {4,4,4,4,4,4},
  .edge_of_side = {{3,2,1,0},{0,5,8,4},{1,6,9,5},{2,7,10,6},{3,4,11,7},{8,9,10,11}},
  .corner_of_side = {{0,3,2,1},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7},{4,5,6,7}},
  .corner_of_edge = {{0,1},{1,2},{2,3},{3,0},{0,4},{1,5},{2,6},{3,7},{4,5},{5,6},{6,7},{7,4}}
} ;
#endif

/****************************************************************************/
/** \brief Compute index fields for a given element type

   \param el pointer to an element description

   This function processes a topology description of an element type and computes
   index mappings. Currently descriptions for triangles,
   quadrilaterals and tetrahedra are included. Hexahedral elements have been implemented
   in a prototype version.

   CAUTION: The above data structure is filled UP TO appropriate sizes for memory allocation
   as well as offsets in the 'refs' array of the 'generic_element'! For complete filling
   you will have to call 'ProcessElementDescription'

   Only the following components of the GENERAL_ELEMENT structure must be provided.
   All other components are derived from the given information.

   . tag - New tag for the element which will be delivered by the 'TAG' macro.
   . sides_of_elem - Number of sides for that element type.
   . corners_of_elem - Number of corners for that element type.
   . edges_of_elem - Number of edges for that element type.
   . edges_of_side - Number of edges for each side.
   . corners_of_side - Number of corners of each side.
   . edge_of_side[s][e] - The edges are numbered in the element and in each side of the
   element. This array provides a mapping that tells you the number of edge 'e' of side
   's' with respect to the numbering in the element.
   . corner_of_side[s][c] - The corners edges are numbered in the element and in each side of the
   element. This array provides a mapping that tells you the number of corner 'c' of side
   's' with respect to the numbering in the element.
   . corner_of_edge[e][c] - Tells you the number of corner 'c' in edge 'e' with respect
   to the numbering in the element.
 */
/****************************************************************************/

static void PreProcessElementDescription (GENERAL_ELEMENT *el)
{
  INT i,j,k;

  INT tag = el->tag;

  /* derive additional index fields */

  /* edge_with_corners(i,j) : number of edge between corners i and j, -1 if no such edge ex. */
  for (i=0; i<MAX_CORNERS_OF_ELEM; i++)
    for (j=0; j<MAX_CORNERS_OF_ELEM; j++) el->edge_with_corners[i][j] = -1;
  for (i=0; i<el->edges_of_elem; i++)
  {
    el->edge_with_corners[el->corner_of_edge[i][0]][el->corner_of_edge[i][1]] = i;
    el->edge_with_corners[el->corner_of_edge[i][1]][el->corner_of_edge[i][0]] = i;
  }

  /* side_with_edge(i,j) : edge i is an edge of side side_with_edge(i,j) */
  for (i=0; i<MAX_EDGES_OF_ELEM; i++)
    for (j=0; j<MAX_SIDES_OF_EDGE; j++) el->side_with_edge[i][j] = -1;
  for (k=0; k<el->edges_of_elem; k++)
  {
    const INT from = el->corner_of_edge[k][0];
    const INT to   = el->corner_of_edge[k][1];

    for (i=0; i<el->sides_of_elem; i++) {
      INT n = el->corners_of_side[i];
      for (j=0; j<n; j++)
      {
        if ((el->corner_of_side[i][j]==from)&&(el->corner_of_side[i][(j+1)%n]==to))
          el->side_with_edge[k][1] = i;
        if ((el->corner_of_side[i][j]==to)&&(el->corner_of_side[i][(j+1)%n]==from))
          el->side_with_edge[k][0] = i;
      }
    }
  }

  /* corner_of_side_inv(i,j) : j is number of a corner in the element. Then this
     array returns the local number of this corner within side i or -1 if side i
     does not contain this corner. */
  for (i=0; i<MAX_SIDES_OF_ELEM; i++)
    for (j=0; j<MAX_CORNERS_OF_ELEM; j++) el->corner_of_side_inv[i][j] = -1;
  for (i=0; i<el->sides_of_elem; i++)
    for (j=0; j<el->corners_of_side[i]; j++)
    {
      const INT n = el->corner_of_side[i][j];
      el->corner_of_side_inv[i][n] = j;
    }

  /* edges_of_corner(i,j) : i is a number of a corner within the element, then
     edges_of_corner(i,j) gives the number of an edge adjacent to corner i or -1 */
  for (i=0; i<MAX_CORNERS_OF_ELEM; i++)
    for (j=0; j<MAX_EDGES_OF_ELEM; j++) el->edges_of_corner[i][j] = -1;
  for (i=0; i<el->edges_of_elem; i++)
    for (j=0; j<CORNERS_OF_EDGE; j++)
    {
      const INT n = el->corner_of_edge[i][j];
      for (k=0; k<MAX_EDGES_OF_ELEM; k++)
        if (el->edges_of_corner[n][k]<0)
        {
          el->edges_of_corner[n][k] = i;
          break;
        }
    }


  /* fields not valid for all elements */

  /* corner_opp_to_side(i) */
  for (i=0; i<MAX_SIDES_OF_ELEM; i++)
    el->corner_opp_to_side[i] = -1;

  /* opposite_edge(i) */
  for (i=0; i<MAX_EDGES_OF_ELEM; i++)
    el->opposite_edge[i] = -1;

  /* side_opp_to_corner(i) */
  for (i=0; i<MAX_CORNERS_OF_ELEM; i++)
    el->side_opp_to_corner[i] = -1;

  /* edge_of_corner(i,j) */
  for (i=0; i<MAX_CORNERS_OF_ELEM; i++)
    for (j=0; j<MAX_EDGES_OF_ELEM; j++)
      el->edge_of_corner[i][j] = -1;

  /* edge_of_corner(i,j)	  */
  for (i=0; i<el->edges_of_elem; i++) {
    for (j=0; j<CORNERS_OF_EDGE; j++) {
      if (el->corner_of_edge[i][j] >=0) {
        for (k=0; k<el->edges_of_elem; k++)
          if (el->edge_of_corner[el->corner_of_edge[i][j]][k] < 0)
            break;
        assert(k<el->edges_of_elem);
        el->edge_of_corner[el->corner_of_edge[i][j]][k] = i;
      }
    }
  }

#ifdef UG_DIM_2
  switch (tag)
  {
  case TRIANGLE :
    /* corner_opp_to_side(i)  */
    /* is not defined!		  */

    /* opposite_edge(i)		  */
    /* is not defined!		  */

    /* side_opp_to_corner(i)  */
    /* is not defined!		  */

    break;

  case QUADRILATERAL :
    /* corner_opp_to_side(i)  */
    /* is not defined!		  */

    /* opposite_edge(i)		  */
    for (i=0; i<el->edges_of_elem; i++) {
      INT n = 0;
      for (j=0; j<CORNERS_OF_EDGE; j++) {
        for (k=0; k<el->edges_of_elem; k++) {
          if (el->edges_of_corner[el->corner_of_edge[i][j]][k] >= 0)
            n |= (0x1<<(el->edges_of_corner[el->corner_of_edge[i][j]][k]));
        }
      }
      for (j=0; j<el->edges_of_elem; j++)
        if (((n>>j) & 0x1) == 0)
          break;
      assert(j<el->edges_of_elem);

      el->opposite_edge[i] = j;
    }

    /* side_opp_to_corner(i)  */
    /* is not defined!		  */

    break;
  }
#endif

#ifdef UG_DIM_3
  /* edge_of_two_sides(i,j) */
  for (i=0; i<MAX_SIDES_OF_ELEM; i++)
    for (j=0; j<MAX_SIDES_OF_ELEM; j++)
      el->edge_of_two_sides[i][j] = -1;

  switch (tag)
  {
  case TETRAHEDRON :

    /* corner_opp_to_side(i) */
    for (i=0; i<el->sides_of_elem; i++) {
      INT n = 0;
      for (j=0; j<el->corners_of_side[i]; j++) {
        n |= (0x1<<(el->corner_of_side[i][j]));
      }
      for (j=0; j<el->corners_of_elem; j++) {
        if (((n>>j) & 0x1) == 0)
          break;
      }
      assert(j<el->corners_of_elem);
      el->corner_opp_to_side[i] = j;
    }

    /* opposite_edge(i)		  */
    for (i=0; i<el->edges_of_elem; i++) {
      INT n = 0;
      for (j=0; j<CORNERS_OF_EDGE; j++) {
        for (k=0; k<el->edges_of_elem; k++) {
          if (el->edges_of_corner[el->corner_of_edge[i][j]][k] >= 0)
            n |= (0x1<<(el->edges_of_corner[el->corner_of_edge[i][j]][k]));
        }
      }
      for (j=0; j<el->edges_of_elem; j++)
        if (((n>>j) & 0x1) == 0)
          break;
      assert(j<el->edges_of_elem);

      el->opposite_edge[i] = j;
    }

    /* side_opp_to_corner(i)  */
    for (i=0; i<el->corners_of_elem; i++) {
      for (j=0; j<el->sides_of_elem; j++) {
        INT n = 0;
        for (k=0; k<el->corners_of_side[j]; k++)
          n |= (0x1<<(el->corner_of_side[j][k]));
        if (((n>>i) & 0x1) == 0) {
          el->side_opp_to_corner[i] = j;
          break;
        }
      }
      assert(j<el->sides_of_elem);
    }

    break;

  case PYRAMID :

    /* corner_opp_to_side(i) */
    for (i=0; i<el->sides_of_elem; i++) {
      if (el->corners_of_side[i] == 4) {
        INT n = 0;
        for (j=0; j<el->corners_of_side[i]; j++) {
          n |= (0x1<<(el->corner_of_side[i][j]));
        }
        for (j=0; j<el->corners_of_elem; j++) {
          if (((n>>j) & 0x1) == 0)
            break;
        }
        assert(j<el->corners_of_elem);
        el->corner_opp_to_side[i] = j;
      }
    }

    /* opposite_edge(i)		  */
    /* is not defined!		  */

    /* side_opp_to_corner(i)  */
    for (i=0; i<el->corners_of_elem; i++) {
      for (j=0; j<el->sides_of_elem; j++) {
        INT n = 0;
        for (k=0; k<el->corners_of_side[j]; k++)
          n |= (0x1<<(el->corner_of_side[j][k]));
        if (((n>>i) & 0x1) == 0) {
          el->side_opp_to_corner[i] = j;
          break;
        }
      }
      assert(j<el->sides_of_elem);
    }

    break;

  case PRISM :

    /* corner_opp_to_side(i)  */
    /* is not defined!		  */

    /* opposite_edge(i)		  */
    /* is not defined!		  */

    /* side_opp_to_corner(i)  */
    /* is not defined!		  */

    break;

  case HEXAHEDRON :

    /* corner_opp_to_side(i)  */
    /* is not defined!		  */

    /* opposite_edge(i)		  */
    for (i=0; i<el->edges_of_elem; i++) {
      INT n = 0;
      for (j=0; j<CORNERS_OF_EDGE; j++) {
        const INT n1 = el->corner_of_edge[i][j];
        for (k=0; k<el->edges_of_elem; k++) {
          if (el->edges_of_corner[n1][k] >= 0) {
            n |= (0x1<<(el->edges_of_corner[n1][k]));
            for (INT l=0; l<CORNERS_OF_EDGE; l++) {
              const INT n2 = el->corner_of_edge[el->edges_of_corner[n1][k]][l];
              if (n2 != n1) {
                for (INT m=0; m<el->edges_of_elem; m++) {
                  if (el->edges_of_corner[n2][m] >= 0)
                    n |= (0x1<<(el->edges_of_corner[n2][m]));
                }
              }
            }
          }
        }
      }
      for (k=0; k<el->edges_of_elem; k++)
        if (((n>>k) & 0x1) == 0)
          break;
      assert(k<el->edges_of_elem);

      el->opposite_edge[i] = k;
    }

    /* side_opp_to_corner(i)  */
    /* is not defined!		  */

    break;
  }

  for (i=0; i<el->sides_of_elem; i++)
    for (j=0; j<el->sides_of_elem; j++)
      for (k=0; k<el->edges_of_side[i]; k++)
        for (INT l=0; l<el->edges_of_side[j]; l++)
          if (el->edge_of_side[i][k] == el->edge_of_side[j][l])
          {
            assert( (i==j) || (el->edge_of_two_sides[i][j]==-1) ||
                    (el->edge_of_two_sides[i][j]==el->edge_of_side[i][k]) );
            el->edge_of_two_sides[i][j] = el->edge_of_side[i][k];
          }
#endif

  /* make description globally available */
  element_descriptors[tag] = el;
}

/****************************************************************************/
/** \brief Compute offsets and size for a given element type

   \param el    pointer to an element description

   This function processes a topology description of an element type and computes
   the appropriate sizes for memory allocation and offsets in the 'refs' array of the
   'generic_element'. All other data are fixed and do not depend on the multigrid or format.
   Before calling this function 'PreProcessElementDescription' has to be called once
   during initialization.

   SEE ALSO:

   'ELEMENT', 'PreProcessElementDescription'.

   \return <ul>
   <li> GM_OK if ok </li>
   <li> GM_ERROR if error occurred. </li>
   </ul>
 */
/****************************************************************************/

static INT ProcessElementDescription (GENERAL_ELEMENT *el)
{
#ifdef UG_DIM_2
  switch (el->tag)
  {
  case TRIANGLE :
    el->inner_size = offsetof(triangle,bnds);
    el->bnd_size = sizeof(triangle);
    break;
  case QUADRILATERAL :
    el->inner_size = offsetof(quadrilateral,bnds);
    el->bnd_size = sizeof(quadrilateral);
    break;
  }
#endif
#ifdef UG_DIM_3
  switch (el->tag)
  {
  case TETRAHEDRON :
    el->inner_size = offsetof(tetrahedron,bnds);
    el->bnd_size = sizeof(tetrahedron);
    break;
  case PYRAMID :
    el->inner_size = offsetof(pyramid,bnds);
    el->bnd_size = sizeof(pyramid);
    break;
  case PRISM :
    el->inner_size = offsetof(prism,bnds);
    el->bnd_size = sizeof(prism);
    break;
  case HEXAHEDRON :
    el->inner_size = offsetof(hexahedron,bnds);
    el->bnd_size = sizeof(hexahedron);
    break;
  }
#endif

  /* get a free object id for free list */
  if (el->mapped_inner_objt < 0)
    el->mapped_inner_objt = GetFreeOBJT();
  if (el->mapped_inner_objt < 0) return(GM_ERROR);

  if (el->mapped_bnd_objt < 0)
    el->mapped_bnd_objt = GetFreeOBJT();
  if (el->mapped_bnd_objt < 0) return(GM_ERROR);

  return(GM_OK);
}

/****************************************************************************/
/** \brief Initialize topological information for element types

   This function initializes topological information for element types and
   is called once during startup. Add your initialization of a new element
   type here.

   \return <ul>
   <li> GM_OK if ok </li>
   <li> GM_ERROR if error occurred </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitElementTypes()
{
  INT err;

  // The splitting between PreProcessElementDescription and ProcessElementDescription
  // is historical and can be removed.
#ifdef UG_DIM_2
  PreProcessElementDescription(&def_triangle);
  err = ProcessElementDescription(&def_triangle);
  if (err!=GM_OK)
    return err;
  PreProcessElementDescription(&def_quadrilateral);
  err = ProcessElementDescription(&def_quadrilateral);
  if (err!=GM_OK)
    return err;
#endif

#ifdef UG_DIM_3
  PreProcessElementDescription(&def_tetrahedron);
  err = ProcessElementDescription(&def_tetrahedron);
  if (err!=GM_OK)
    return err;
  PreProcessElementDescription(&def_pyramid);
  err = ProcessElementDescription(&def_pyramid);
  if (err!=GM_OK)
    return err;
  PreProcessElementDescription(&def_prism);
  err = ProcessElementDescription(&def_prism);
  if (err!=GM_OK)
    return err;
  PreProcessElementDescription(&def_hexahedron);
  err = ProcessElementDescription(&def_hexahedron);
  if (err!=GM_OK)
    return err;
#endif

  return (0);
}
