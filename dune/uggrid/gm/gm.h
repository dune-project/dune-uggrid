// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/*! \file gm.h
 * \ingroup gm
 */

/******************************************************************************/
/*                                                                            */
/* File:      gm.h                                                            */
/*                                                                            */
/* Purpose:   grid manager header file (the heart of ug)                      */
/*                                                                            */
/* Author:    Peter Bastian, Klaus Johannsen                                  */
/*                                                                            */
/*            Institut fuer Computeranwendungen III                           */
/*            Universitaet Stuttgart                                          */
/*            Pfaffenwaldring 27                                              */
/*            70569 Stuttgart                                                 */
/*            email: ug@ica3.uni-stuttgart.de                                 */
/*                                                                            */
/*       blockvector data structure:                                          */
/*            Christian Wrobel                                                */
/*            Institut fuer Computeranwendungen III                           */
/*            Universitaet Stuttgart                                          */
/*            Pfaffenwaldring 27                                              */
/*            70569 Stuttgart                                                 */
/*            email: ug@ica3.uni-stuttgart.de                                 */
/*                                                                            */
/* History:   09.03.92 begin, ug version 2.0  (as ugtypes2.h)                 */
/*            13.12.94 begin, ug version 3.0                                  */
/*            27.09.95 blockvector implemented (Christian Wrobel)             */
/*                                                                            */
/* Remarks:                                                                   */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __GM__
#define __GM__

#include <climits>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <memory>

#include <unordered_map>
#include <array>
#include <numeric>

#include <dune/common/fvector.hh>

#include <dune/uggrid/domain/domain.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugtypes.h>
#include "pargm.h"
#include "cw.h"

#include <dune/uggrid/parallel/ppif/ppiftypes.hh>
#ifdef ModelP
#  include <dune/uggrid/parallel/ddd/dddcontext.hh>
#endif

/****************************************************************************/
/*                                                                          */
/* derive additional switches from commandline specified basic switches     */
/*                                                                          */
/****************************************************************************/

#ifdef Debug
#define DEBUG_MODE "ON"
#else
#define DEBUG_MODE "OFF"
#endif

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* "hard" switches for interpolation matrix and block-vectors               */
/*                                                                          */
/****************************************************************************/

/** \brief Define to have matrices > 4KB (control word too small, adds integer to matrix struct) */
#define __XXL_MSIZE__

/** \brief If pointer between element/centernode is stored */
#undef __CENTERNODE__

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)        */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

/* Necessary for most C runtime libraries */
#undef DOMAIN

/** @name Some size parameters */
/*@{*/
/** \brief  maximum depth of triangulation               */
#define MAXLEVEL                                32
/** \brief  use 5 bits for object identification */
#define MAXOBJECTS                              32
/*@}*/

/** @name Some size macros for allocation purposes */
/*@{*/
#ifdef UG_DIM_2

/** \brief max number of sides of an element */
#define MAX_SIDES_OF_ELEM               4
/** \brief max number of edges of an element */
#define MAX_EDGES_OF_ELEM               4
/** \brief max number of corners of an element */
#define MAX_CORNERS_OF_ELEM             4
/** \brief max number of edges of a side */
#define MAX_EDGES_OF_SIDE               1
/** \brief max number of edges meeting in a corner */
#define MAX_EDGES_OF_CORNER             2
/** \brief max number of corners of a side */
#define MAX_CORNERS_OF_SIDE             2
/** \brief two sides have one edge in common */
#define MAX_SIDES_OF_EDGE               2
/** \brief max number of sons of an element */
enum {MAX_SONS = 4};
/** \brief max number of nodes on elem side */
#define MAX_SIDE_NODES                  3

#else  // UG_DIM_3

/** \brief max number of sides of an element */
#define MAX_SIDES_OF_ELEM               6
/** \brief max number of edges of an element */
#define MAX_EDGES_OF_ELEM               12
/** \brief max number of corners of an element */
#define MAX_CORNERS_OF_ELEM             8
/** \brief max number of edges of a side */
#define MAX_EDGES_OF_SIDE               4
/** \brief max number of edges meeting in a corner */
#define MAX_EDGES_OF_CORNER             4
/** \brief max number of corners of a side */
#define MAX_CORNERS_OF_SIDE     4
/** \brief two sides have one edge in common */
#define MAX_SIDES_OF_EDGE               2
/** \brief max number of sons of an element */
enum {MAX_SONS = 30};
/** \brief max number of nodes on elem side */
#define MAX_SIDE_NODES                  9

#endif

/** \brief an edge has always two corners.. */
#define CORNERS_OF_EDGE             2
/** \brief max number of son edges of edge  */
#define MAX_SON_EDGES                   2

/** \brief max number of doubles in a vector or matrix mod 32 */
#define MAX_NDOF_MOD_32        256
/** \brief max number of doubles in a vector or matrix */
#define MAX_NDOF 32*MAX_NDOF_MOD_32
/*@}*/


/****************************************************************************/
/*                                                                          */
/* defines for algebra                                                      */
/*                                                                          */
/****************************************************************************/

/** \brief Number of different data types                                    */
#define MAXVOBJECTS                                             1
/** \brief max number of abstract vector types                  */
#define MAXVECTORS                                              1
#if (MAXVECTORS<MAXVOBJECTS)
        #error *** MAXVECTORS must not be smaller than MAXVOBJECTS ***
#endif

/** \brief transforms type into bitpattern                              */
#define BITWISE_TYPE(t) (1<<(t))

/* derived sizes for algebra */
/** \brief max number of diff. matrix types                 */
#define MAXMATRICES             MAXVECTORS*MAXVECTORS
/** \brief max number of diff. connections              */
#define MAXCONNECTIONS  (MAXMATRICES + MAXVECTORS)

/** \todo Please doc me! */
#define MATRIXTYPE(rt,ct)   ((rt)*MAXVECTORS+(ct))
/** \todo Please doc me! */
#define DIAGMATRIXTYPE(rt)  (MAXMATRICES+rt)

/** \brief Type of geometric entity which a certain vector is attached to */
enum VectorType {NOVTYPE=-1,  //** Undefined */
                 SIDEVEC    /**< Vector associated to an element side */
};

/****************************************************************************/
/*                                                                          */
/* various defines                                                          */
/*                                                                          */
/****************************************************************************/

/** @name result codes of user supplied functions*/
/*@{*/

/** \brief Use of GSTATUS (for grids), use power of 2 */
enum {GSTATUS_BDF         = 1,
      GSTATUS_INTERPOLATE = 2,
      GSTATUS_ASSEMBLED   = 4,
      GSTATUS_ORDERED     = 8};
/*@}*/

/** \brief Possible values for rule in MarkForRefinement */
enum RefinementRule
{NO_REFINEMENT = 0,
 COPY = 1,
 RED =  2,
#ifdef UG_DIM_2
 BLUE = 3,   // For quadrilaterals
#endif
 COARSE = 4,
#ifdef UG_DIM_2
 // The BISECTION* rules are all triangle rules
 BISECTION_1 = 5,
 BISECTION_2_Q = 6,
 BISECTION_2_T1 = 7,
 BISECTION_2_T2 = 8,
 BISECTION_3 = 9
#endif
#ifdef UG_DIM_3

 TETRA_RED_HEX = 5,

 PRISM_BISECT_1_2 = 9,
 PRISM_QUADSECT = 7,
 PRISM_BISECT_HEX0 = 5,
 PRISM_BISECT_HEX1 = 8,
 PRISM_BISECT_HEX2 = 6,
 PRISM_ROTATE_LEFT = 10,
 PRISM_ROTATE_RGHT = 11,
 PRISM_QUADSECT_HEXPRI0 = 14,
 PRISM_RED_HEX = 15,
 PRISM_BISECT_0_1,      /* No explicit numbers.  Maybe they are not necessary? */
 PRISM_BISECT_0_2,
 PRISM_BISECT_0_3,

 HEX_BISECT_0_1 = 5,
 HEX_BISECT_0_2 = 6,
 HEX_BISECT_0_3 = 7,
 HEX_TRISECT_0 = 8,
 HEX_TRISECT_5 = 9,
 HEX_QUADSECT_0 = 12,
 HEX_QUADSECT_1 = 13,
 HEX_QUADSECT_2 = 14,
 HEX_BISECT_HEXPRI0 = 15,
 HEX_BISECT_HEXPRI1 = 16

#endif
};

/** \brief Values for element class */
enum MarkClass {NO_CLASS,
                YELLOW_CLASS,
                GREEN_CLASS,
                RED_CLASS,
                SWITCH_CLASS};

/** \brief Values for node types (relative to the father element of the vertex) */
enum NodeType {CORNER_NODE,
               MID_NODE,
               SIDE_NODE,
               CENTER_NODE,
               LEVEL_0_NODE};

/* REMARK: TOPNODE no more available since 970411
   because of problems in parallelisation
   to use it in serial version uncomment define
   #define TOPNODE(p)              ((p)->iv.topnode)
 */

/****************************************************************************/
/*                                                                          */
/* format definition data structure                                         */
/*                                                                          */
/****************************************************************************/

/*----------- general typedefs ---------------------------------------------*/

/** @name General typedefs */
/*@{*/
using DOUBLE_VECTOR = FieldVector<DOUBLE,DIM>;
/*@}*/


/*----------- definition of structs ----------------------------------------*/

/****************************************************************************/
/*                                                                          */
/* matrix/vector/blockvector data structure                                 */
/*                                                                          */
/****************************************************************************/

/** \brief Data type for unknowns in sparse matrix structure

The VECTOR data type is part of the sparse matrix vector data structure.
A VECTOR stores a user definable number of DOUBLE values and
is associated with a geometric object of the mesh (nodes, edges, sides and elements).
The size of the VECTOR can be specified differently for each type of
geometric object, but not (yet) for each individual VECTOR object.
Since there are four different geometric objects that can have degrees
of freedom, there are four different `vector types`. The vector type
is stored in the control word (see VTYPE macro) but currently it can only have
the value SIDEVECTOR.

The VECTOR provides access to the rows of the sparse matrix belonging to
all degrees of freedom stored in the vector.

MACROS:

`General macros in control word`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (VECTOR *p);                 27  5   Object type VEOBJ
void SETOBJT (VECTOR *p, INT n);

INT USED (VECTOR *p);                 23  1   used only temporarily
void SETUSED (VECTOR *p, INT n);

INT TAG (VECTOR *p);                  24  3   not used
void SETTAG (VECTOR *p, INT n);

INT THEFLAG (VECTOR *p);              16  1   used only temporarily
void SETTHEFLAG (VECTOR *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros for the VECTOR structure in control`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
VTYPE (VECTOR *v);                    0   2   Type of geometric object the
SETVTYPE (VECTOR *v, INT n);                  vector belongs to. Values:
                                              NODEVECTOR, EDGEVECTOR, SIDEVECTOR
                                              or ELEMVECTOR.

VCOUNT (VECTOR *v);                   5   2   internal use
SETVCOUNT (VECTOR *v, INT n);

VECTORSIDE (VECTOR *v);               7   2   Side in VOBJECT where VECTOR
SETVECTORSIDE (VECTOR *v, INT n);             belongs to (SIDEVECTOR only)

VCLASS (VECTOR *v);                   11  2   Vector class for local multigrid
SETVCLASS (VECTOR *v, INT n);                 between 0 and 3.

VDATATYPE (VECTOR *v);                13  4   This is 2^VTYPE(v)
SETVDATATYPE (VECTOR *v, INT n);

VNCLASS (VECTOR *v);                  17  2   Vector class of vector at same
SETVNCLASS (VECTOR *v, INT n);                position on finer level

VNEW (VECTOR *v);                     19  1   True if VECTOR has been created
SETVNEW (VECTOR *v, INT n);                   in last refinement step.

VCNEW (VECTOR *v);                    20  1   True if VECTOR got a new
SETVCNEW (VECTOR *v, INT n);                  connection in refinement
-----------------------------------------------------------------------------
\endverbatim

\sa NODE, EDGE, ELEMENT.

*/
struct vector {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief associated geometric object */
  union geom_object *object;

#ifdef ModelP
  /** \todo Please doc me! */
  DDD_HEADER ddd;
#endif

  /** \brief double linked list of vectors                */
  struct vector *pred,*succ;

  /** \brief ordering of unknowns                                 */
  UINT index;

  /** \brief Index if the vector is part of the leaf grid */
  UINT leafIndex;

#ifndef ModelP   // Dune uses ddd.gid for ids in parallel
  /** \brief A unique and persistent, but not necessarily consecutive index

      Used to implement face ids for Dune.
   */
  INT id;
#endif

  /** \brief User data
   *
   * Array of double values. This array is allocated dynamically with the
   * size specified in the FORMAT structure.
   */
  DOUBLE value[1];
};
typedef struct vector VECTOR;


/****************************************************************************/
/*                                                                          */
/* unstructured grid data structures                                        */
/*                                                                          */
/****************************************************************************/

/*----------- typedef for functions ----------------------------------------*/


/*----------- definition of structs ----------------------------------------*/

/** \brief Inner vertex data structure
 Data type storing level-independent node information

The VERTEX structure stores the level independent information of
a NODE. If several NODEs on different levels of the MULTIGRID structure
are at the same position then they share a common VERTEX object.
The VERTEX is a union of inner vertex ivertex and boundary vertex bvertex.
The bvertex has all of the ivertex plus some extra information where
the vertex is on the boundary of the domain. In the code one always works with
pointers of type VERTEX * and accesses the components through the macros described
below.

CONTROL WORDS:

The first word of many UG data structure is used bitwise for various purposes.

`General macros available for all objects`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (VERTEX *p);                 27  5   Object type IVOBJ for inner
void SETOBJT (VERTEX *p, INT n);              and BVOBJ for boundary vertex

INT USED (VERTEX *p);                 23  1   used only temporarily
void SETUSED (VERTEX *p, INT n);

INT TAG (VERTEX *p);                  24  3   available for user
void SETTAG (VERTEX *p, INT n);

INT LEVEL (VERTEX *p);                17  5   level on which vertex is
void SETLEVEL (VERTEX *p, INT n);             defined

INT THEFLAG (VERTEX *p);              16  1   used only temporarily
void SETTHEFLAG (VERTEX *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros specific to the VERTEX structure`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT MOVE (VERTEX *p);                 4   2   Number of dimensions in which
void SETMOVE (VERTEX *p, INT n);              a vertex can be moved:
                                                0: vertices at corners
                                                1: boundary vertex in 2D
                                                2: inner in 2D, boundary in 3D
                                                3: inner in 3D

INT MOVED (VERTEX *p);                0   1   TRUE if vertex has been moved
void SETMOVED (VERTEX *p, INT n);             away from initial position.

INT ONEDGE (VERTEX *p);               1   3   Edge in father element of vertex
void SETONEDGE (VERTEX *p, INT n);            on which this vertex resides.
                                              2D: only valid for boundary vertex!
-----------------------------------------------------------------------------
\endverbatim
D*/
struct ivertex {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Vertex position                                              */
  FieldVector<DOUBLE,DIM> x;

  /** \brief Local coordinates in father element
   *
   * This is used to represent the multigrid hierarchy. The father component points
   * to an ELEMENT of the next coarser grid level and xi gives the position of the VERTEX
   * within this element in the local coordinate system of the ELEMENT. In this way
   * also loosely coupled grids can be represented although the grid refinement algorithm
   * is currently not able to produce such grids. The solvers however would work
   * also on much more general multigrid hierarchies.
   */
  FieldVector<DOUBLE,DIM> xi;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per node */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \todo Please doc me! */
  DDD_HEADER ddd;
#endif

  /** \brief Doubly linked list of vertices
   *
   * Pointers realizing a double linked list of VERTEX objects per level.
   * The beginning of this list is in the GRID structure.
   */
  union vertex *pred,*succ;

  /** \brief Associated user data structure */
  void *data;

  /** \brief Father element */
  union element *father;

#ifdef TOPNODE
  /** \brief Highest node where defect is valid
      \todo REMARK: TOPNODE no more available since 970411
     because of problems in parallelisation
     to use it in serial version uncomment define TOPNODE

    If several NODES share this VERTEX then this pointer refers
    to the NODE on the highest level. This pointer will probably `not` be supported
    in the next version, its use is not recommended!
  */
  struct node *topnode;
#endif
};

/** \brief Boundary vertex data structure

Data type storing level-independent node information

The VERTEX structure stores the level independent information of
a NODE. If several NODEs on different levels of the MULTIGRID structure
are at the same position then they share a common VERTEX object.
The VERTEX is a union of inner vertex ivertex and boundary vertex bvertex.
The bvertex has all of the ivertex plus some extra information where
the vertex is on the boundary of the domain. In the code one always works with
pointers of type VERTEX * and accesses the components through the macros described
below.

CONTROL WORDS:

The first word of many UG data structure is used bitwise for various purposes.

`General macros available for all objects`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (VERTEX *p);                 27  5   Object type IVOBJ for inner
void SETOBJT (VERTEX *p, INT n);              and BVOBJ for boundary vertex

INT USED (VERTEX *p);                 23  1   used only temporarily
void SETUSED (VERTEX *p, INT n);

INT TAG (VERTEX *p);                  24  3   available for user
void SETTAG (VERTEX *p, INT n);

INT LEVEL (VERTEX *p);                17  5   level on which vertex is
void SETLEVEL (VERTEX *p, INT n);             defined

INT THEFLAG (VERTEX *p);              16  1   used only temporarily
void SETTHEFLAG (VERTEX *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros specific to the VERTEX structure`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT MOVE (VERTEX *p);                 4   2   Number of dimensions in which
void SETMOVE (VERTEX *p, INT n);              a vertex can be moved:
                                                0: vertices at corners
                                                1: boundary vertex in 2D
                                                2: inner in 2D, boundary in 3D
                                                3: inner in 3D

INT MOVED (VERTEX *p);                0   1   TRUE if vertex has been moved
void SETMOVED (VERTEX *p, INT n);             away from initial position.

INT ONEDGE (VERTEX *p);               1   3   Edge in father element of vertex
void SETONEDGE (VERTEX *p, INT n);            on which this vertex resides.
                                              2D: only valid for boundary vertex!
-----------------------------------------------------------------------------
\endverbatim
D*/
struct bvertex {

  /* variables */
  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Vertex position */
  FieldVector<DOUBLE,DIM> x;

  /** \brief Local coordinates in father element
   *
   * This is used to represent the multigrid hierarchy. The father component points
   * to an ELEMENT of the next coarser grid level and xi gives the position of the VERTEX
   * within this element in the local coordinate system of the ELEMENT. In this way
   * also loosely coupled grids can be represented although the grid refinement algorithm
   * is currently not able to produce such grids. The solvers however would work
   * also on much more general multigrid hierarchies.
   */
  FieldVector<DOUBLE,DIM> xi;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per node */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Information about the parallelization of this object */
  DDD_HEADER ddd;
#endif

  /* pointers */
  /** \brief Doubly linked list of vertices               */
  union vertex *pred,*succ;

  /** \brief Associated user data structure */
  void *data;

  /** \brief Father element */
  union element *father;

#ifdef TOPNODE
  /** \brief Highest node where defect is valid
      \todo REMARK: TOPNODE no more available since 970411
     because of problems in parallelisation
     to use it in serial version uncomment define TOPNODE */
  struct node *topnode;
#endif

  /** \brief Pointer to boundary point descriptor */
  BNDP *bndp;
};

/** \brief Only used to define pointer to vertex */
union vertex {
  struct ivertex iv;
  struct bvertex bv;
};

/** \brief Level-dependent part of a vertex

Each node of the mesh is represented by a NODE structure. When a mesh is refined
then new NODE objects are allocated on the new grid level
even at those positions where nodes were already in the coarse mesh. However, NODEs
at the same position share a common VERTEX structure.
A node provides access to all neighboring nodes via the LINK structure. There
is `no` access from a node to all the elements having the node as a corner!

CONTROL WORDS:

`General macros available for all objects`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (NODE *p);                   27  5   Object type NDOBJ for a node.
void SETOBJT (NODE *p, INT n);

INT USED (NODE *p);                   23  1   used only temporarily
void SETUSED (NODE *p, INT n);

INT TAG (NODE *p);                    24  3   available for user
void SETTAG (NODE *p, INT n);

INT LEVEL (NODE *p);                  17  5   level on which node is
void SETLEVEL (NODE *p, INT n);               allocated

INT THEFLAG (NODE *p);                16  1   used only temporarily
void SETTHEFLAG (NODE *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros for the NODE structure`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT CLASS (NODE *p);                  0   3   node class for local multigrid
void SETCLASS (NODE *p, INT n);               only for backward compatibility

INT NCLASS (NODE *p);                 3   3   node class for node on next level
void SETNCLASS (NODE *p, INT n);              only for backward compatibility

INT MODIFIED (NODE *p);               6   1   is true when node has been created
void SETMODIFIED (NODE *p, INT n);            or modified during last refinement
                                              step. This in 2D only !

INT NPROP (NODE *p);                  7   8   node property derived from
void SETNPROP (NODE *p, INT n);               element property.
-----------------------------------------------------------------------------
\endverbatim

\sa

VERTEX, LINK, GRID.

D*/
struct node {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store                */
  INT id;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per node */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief Information if this node is on the leaf. */
  bool isLeaf;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer_;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size_;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this object */
  DDD_HEADER ddd;
#endif

  /* pointers */
  /** \brief Doubly linked list of nodes per level*/
  struct node *pred,*succ;

  /** \brief List of links
   *
   * Points to the first element of the LINK list. The LINK list
   * provides access to all neighbors of the node.
   */
  struct link *start;

  /** \brief Node or edge on coarser level (NULL if none) */
  union geom_object *father;

  /** \brief Node on finer level (NULL if none)   */
  struct node *son;

  /** \brief Corresponding vertex structure               */
  union vertex *myvertex;

  /** \brief Associated data pointer
   *
   * WARNING: The allocation of the data pointer depends on the format */
  void *data;

#ifdef ModelP
  const char* message_buffer() const
    { return message_buffer_; }

  std::size_t message_buffer_size() const
  { return message_buffer_size_; }

  void message_buffer(char* p, std::size_t size)
  {
    message_buffer_ = p;
    message_buffer_size_ = size;
  }

  void message_buffer_free()
  {
    std::free(message_buffer_);
    message_buffer(nullptr, 0);
  }
#endif
};

/** \brief Data type realizing a list of neighbors of a node

The \ref link structures form a single linked list starting in each NODE.
Two nodes are said to be neighbors if they are connected by an edge `in the mesh`.
Note that this is only the geometric neighborship, which can be different from the
algebraic neighborship derived from the non-zero structure of the global stiffness matrix.
Each LINK provides a pointer to one neighbor of the NODE where the list
starts. The neighbor relationship is symmetric, therefore if node `a` is a neighbor
of node `b` then `a` occurs in the list of `b` and vice versa. These two link structures
are then combined in an EDGE structure.

MACROS:

#NBNODE(LINK *p)
#NEXT(LINK *p)
#MYEDGE(LINK *p)
#REVERSE (LINK *p)

CONTROL WORDS:

`General macros available for all objects`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (LINK *p);                   27  5   Object type EDOBJ for first
void SETOBJT (LINK *p, INT n);                LINK of an EDGE

INT USED (LINK *p);                   23  1   used only temporarily
void SETUSED (LINK *p, INT n);

INT THEFLAG (LINK *p);                16  1   used only temporarily
void SETTHEFLAG (LINK *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros for the LINK structure`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT LOFFSET (LINK *p);                0   1   0 if first LINK of EDGE
void SETLOFFSET (LINK *p, INT n);             1 if second LINK of EDGE
-----------------------------------------------------------------------------
\endverbatim

\sa edge, node
*/
struct link {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief ptr to next link                                     */
  struct link *next;

  /** \brief ptr to neighbor node                                 */
  struct node *nbnode;

  /** \brief ptr to neighboring elem                              */
#if defined(UG_DIM_2)
  union element *elem;
#endif

};

/** \brief Undirected edge of the grid graph

The EDGE data type combines LINK structures to form an `undirected`
connection of two NODEs. An EDGE represents an edge of the mesh.

CONTROL WORDS:

`General macros available for all objects`
\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (EDGE *p);                   27  5   Object type EDOBJ for an edge
void SETOBJT (EDGE *p, INT n);

INT USED (EDGE *p);                   23  1   used only temporarily
void SETUSED (EDGE *p, INT n);

INT LEVEL (EDGE *p);                  17  5   level on which edge is
void SETLEVEL (EDGE *p, INT n);               allocated

INT THEFLAG (EDGE *p);                16  1   used only temporarily
void SETTHEFLAG (EDGE *p, INT n);
-----------------------------------------------------------------------------
\endverbatim

`Macros for the EDGE structure`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT EOFFSET (EDGE *p);                0   1   Overlay with bit from LINK
void SETEOFFSET (EDGE *p, INT n);

INT EXTRA (EDGE *p);                  1   1   True if additional edge is
void SETEXTRA (EDGE *p, INT n);               diagonal of quadrilateral. This is
                                              provided for backward compatibility

INT NO_OF_ELEM (EDGE *p);             2   7   Number of elements sharing the
void SET_NO_OF_ELEM (EDGE *p, INT n);         given edge. This is provided
void INC_NO_OF_ELEM (EDGE *p);                only in 3D version
void DEC_NO_OF_ELEM (EDGE *p);

INT EDGENEW (EDGE *p);                16  1   True if edge has been created in
void SETEDGENEW (EDGE *p, INT n);             last refinement step. 3D only!
-----------------------------------------------------------------------------
\endverbatim

\sa LINK
*/
struct edge {

  /** \brief The two links that make up this edge
   *
   * LINKs are always allocated pairwise and in consecutive memory locations.
   * This allows to switch easily from a given LINK to the LINK in reverse direction or to the EDGE.
   */
  struct link links[2];

  /* When UG is used as part of the DUNE numerics system we need
     place to store codim dim-1 indices */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

  /** \brief A unique and persistent, but not necessarily consecutive index */
  INT id;

#ifdef ModelP
  /** Bookkeeping information for DDD */
  DDD_HEADER ddd;
#endif

  /** \brief Pointer to mid node on next finer grid
   *
   * In 3D a pointer to the node created on the midpoint of an edge is
   * needed during refinement (This is because many elements share an edge in 3D).
   */
  struct node *midnode;

  /** \brief associated vector
   *
   * WARNING: the allocation of the vector pointer depends on the format */
  VECTOR *vector;
};

/** \brief A generic grid element

   No difference between inner and boundary elements
 */
struct generic_element {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief unique id used for load/store        */
  INT id;

  /** \brief additional flags for elements
   * A lot of information requiring only a small number of bits must be stored
   * for each element. Therefore a second control word, the flag, has been introduced.
   * The flag is also used bitwise.
   */
  UINT flag;

  /** \brief to store NodeOrder for hexahedrons
   *
   * Since control and flag were full, a third word had to be introduced that
   * can be used bitwise. A part of this word stores the element property, which is simple
   * a number per element. This number can be used to distinguish different materials
   * for example.
   */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this object */
  DDD_HEADER ddd;

  /** \brief Stores partition information */
  INT lb1;
#endif

  /** \brief double linked list of elements       */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to center node */
  struct node *centernode;
#endif

  /** \brief Element specific part of variable length array managed by ug  */
  void *refs[1];

};

/** \brief A triangle element in a 2d grid
    \implements generic_element
 */
struct triangle {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Additional flags for elements */
  UINT flag;

  /** \brief Even more property bits */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this object */
  DDD_HEADER ddd;

  /** \brief stores partition information         */
  INT lb1;
#endif

  /** \brief Realize a doubly linked list of elements */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to the center node */
  struct node *centernode;
#endif

  /** \brief Corners of this element */
  struct node *n[3];

  /** \brief Father element on next-coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief The neighboring elements */
  union element *nb[3];

  /** \brief Only on the boundary, NULL if interior side */
  BNDS *bnds[3];
};

/** \brief A quadrilateral element in a 2d grid
    \implements generic_element
 */
struct quadrilateral {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief unique id used for load/store                */
  INT id;

  /** \brief additional flags for elements                */
  UINT flag;

  /** \brief Even more flags */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this object */
  DDD_HEADER ddd;

  /** \brief stores partition information         */
  INT lb1;
#endif

  /** \brief doubly linked list of elements               */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief pointer to center node				*/
  struct node *centernode;
#endif

  /** \brief corners of that element */
  struct node *n[4];

  /** \brief father element on coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief The neighbor elements */
  union element *nb[4];

  /** \brief only on bnd, NULL if interior side   */
  BNDS *bnds[4];
};

/** \brief A tetrahedral element in a 3d grid
    \implements generic_element
 */
struct tetrahedron {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Additional flags */
  UINT flag;

  /** \brief Even more flags */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP

  /** \brief Information about the parallelization of this element */
  DDD_HEADER ddd;

  /** \brief Stores partition information */
  INT lb1;
#endif

  /* pointers */

  /** \brief Realize a double linked list of elements */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to the center node */
  struct node *centernode;
#endif

  /** \brief Corners of this element */
  struct node *n[4];

  /** \brief Father element on coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];                   /* element tree                                                 */
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief The neighboring elements */
  union element *nb[4];

  /** \brief Associated vector for sides */
  VECTOR *sidevector[4];

  /** \brief The boundary segments, NULL if interior side */
  BNDS *bnds[4];
};

/** \brief A pyramid element in a 3d grid
    \implements generic_element
 */
struct pyramid {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Additional flags for elements */
  UINT flag;

  /** \brief Even more flags */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this element */
  DDD_HEADER ddd;

  /** \brief Stores partition information */
  INT lb1;
#endif

  /* pointers */
  /** \brief Realize a doubly linked list of elements */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to center node */
  struct node *centernode;
#endif

  /** \brief Corners of this element */
  struct node *n[5];

  /** \brief Father element on coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief The neighbor elements */
  union element *nb[5];

  /** \brief Associated vector for sides */
  VECTOR *sidevector[5];

  /** \brief The boundary segments, NULL if interior side */
  BNDS *bnds[5];
};

/** \brief A prism element in a 3d grid
    \implements generic_element
 */
struct prism {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Additional flags for this element */
  UINT flag;

  /** \brief Even more flags */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this element */
  DDD_HEADER ddd;

  /** \brief Stores partition information */
  INT lb1;
#endif

  /* pointers */
  /** \brief Realize doubly linked list */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to center node */
  struct node *centernode;
#endif

  /** \brief Corners of this element */
  struct node *n[6];

  /** \brief Father element on next coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief Neighbor elements */
  union element *nb[5];

  /** \brief Associated vectors for sides */
  VECTOR *sidevector[5];

  /** \brief Boundary segments, NULL if interior side */
  BNDS *bnds[5];
};

/** \brief A hexahedral element in a 3d grid
    \implements generic_element
 */
struct hexahedron {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Additional flags for this element */
  UINT flag;

  /** \brief Even more flags */
  INT property;

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per element */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int levelIndex;

  /** \brief An index hat is unique and consecutive on the grid surface.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \brief Per-node message buffer used by Dune for dynamic load-balancing */
  char* message_buffer;

  /** \brief Size of the `message_buffer` */
  std::size_t message_buffer_size;
#endif

#ifdef ModelP
  /** \brief Information about the parallelization of this element */
  DDD_HEADER ddd;

  /** \brief Stores partition information */
  INT lb1;
#endif

  /** \brief Realize doubly linked list of elements on one grid level */
  union element *pred, *succ;

#ifdef __CENTERNODE__
  /** \brief Pointer to center node */
  struct node *centernode;
#endif

  /** \brief Corners of this element */
  struct node *n[8];

  /** \brief Father element on coarser grid */
  union element *father;

#ifdef ModelP
  /** \brief Element tree */
  union element *sons[2];
#else
  /** \brief Element tree */
  union element *sons[1];
#endif

  /** \brief The neighboring elements */
  union element *nb[6];

  /** \brief Associated vectors for sides */
  VECTOR *sidevector[6];

  /** \brief Boundary segments, NULL if interior side */
  BNDS *bnds[6];
} ;

/** \brief Objects that can hold an element */





/** \brief Data type representing an element in the mesh

UG provides a flexible element concept, i.e. there may be different
types of elements in a mesh (currently limited to 8). All element types are derived
from the generic_element where the refs array is allocated to the
appropriate length. Data types for the currently implemented three different
elements triangle, quadrilateral and tetrahedron are provided only
for illustration and debugging purposes. Internally only the generic_element
is used. The TAG field in the control word is used to identify the
element type at run-time, therefore the limitation to eight element types.
Memory requirements are also higher for elements having at least one
side on the boundary because additional pointers to boundary information
is stored.
In 3D, degrees of freedom can be associated also with the sides of an element.
Since there is no data type to represent sides, the degrees of freedom associated
with a side of an element are accessed via the element (a side is shared by exactly
two elements, UG ensures the consistency of the pointers).
as the element has sides.

MACROS:

#SIDES_OF_ELEM (ELEMENT *p)
#EDGES_OF_ELEM (ELEMENT *p)
#CORNERS_OF_ELEM (ELEMENT *p)
#SONS_OF_ELEM (ELEMENT *p)
#EDGES_OF_SIDE (ELEMENT *p,INT i)
#CORNERS_OF_SIDE (ELEMENT *p,INT i)

#EDGE_OF_SIDE (ELEMENT *p,INT s,INT e)
#CORNER_OF_SIDE (ELEMENT *p,INT s,INT c)
#CORNER_OF_EDGE (ELEMENT *p,INT e,INT c)
#EDGE_WITH_CORNERS (ELEMENT *p,INT c0,INT c1)
#SIDE_WITH_EDGE (ELEMENT *p,INT e,INT k)
#CORNER_OF_SIDE_INV (ELEMENT *p,INT s,INT c)
#EDGES_OF_CORNER (ELEMENT *p,INT c,INT k)
#SUCCE (ELEMENT *p)
#PREDE (ELEMENT *p)
#CORNER (ELEMENT *p,INT i)
#EFATHER (ELEMENT *p)
#SON (ELEMENT *p,INT i)
#NBELEM (ELEMENT *p,INT i)
#SVECTOR (ELEMENT *p,INT i)


CONTROL WORDS:

`General macros available for all objects`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT OBJT (ELEMENT *p);                27  5   Object type BEOBJ for an element
void SETOBJT (ELEMENT *p, INT n);             with at least one side on a
                                              boundary and IEOBJ else

INT USED (ELEMENT *p);                23  1   used only temporarily
void SETUSED (ELEMENT *p, INT n);

INT TAG (ELEMENT *p);                 24  3   identifies element type. E.g.
void SETTAG (ELEMENT *p, INT n);              3=triangle, 4=quadrilateral.
                                              should not be used anymore!

INT LEVEL (ELEMENT *p);               17  5   level on which element is
void SETLEVEL (ELEMENT *p, INT n);            allocated

INT THEFLAG (ELEMENT *p);             16  1   True if elements refinement rule
void SETTHEFLAG (ELEMENT *p, INT n);          changed during last refinement
                                              step. 3D only !
-----------------------------------------------------------------------------
\endverbatim

`Macros for the ELEMENT structure in control`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT ECLASS (ELEMENT *p);              8   2   stores element class which is
void SETECLASS (ELEMENT *p,INT n);            out of COPY_CLASS,
                                              IRREGULAR_CLASS or REGULAR_CLASS

INT NSONS (ELEMENT *p);               10  4   number of sons of an element.
void SETNSONS (ELEMENT *p,INT n);

INT NEWEL (ELEMENT *p);               14  1   Set at creation time but never
void SETNEWEL (ELEMENT *p,INT n);             reset. Do not use it!
-----------------------------------------------------------------------------
\endverbatim

`Macros for the ELEMENT structure in flag are all for internal use only`



`Macros for the ELEMENT structure in property`

\verbatim
Macro name                            Pos Len Purpose
-----------------------------------------------------------------------------
INT PROP (ELEMENT *p);                0   32  Stores element property number.
void SETPROP (ELEMENT *p,INT n);
-----------------------------------------------------------------------------
\endverbatim
*/

union element {
  struct generic_element ge;
    #ifdef UG_DIM_2
  struct triangle tr;
  struct quadrilateral qu;
        #endif
    #ifdef UG_DIM_3
  struct tetrahedron te;
  struct pyramid py;
  struct prism pr;
  struct hexahedron he;
        #endif

#ifdef ModelP
  const char* message_buffer() const
    { return ge.message_buffer; }

  std::size_t message_buffer_size() const
  { return ge.message_buffer_size; }

  void message_buffer(char* p, std::size_t size)
  {
    ge.message_buffer = p;
    ge.message_buffer_size = size;
  }

  void message_buffer_free()
  {
    std::free(ge.message_buffer);
    message_buffer(nullptr, 0);
  }
#endif
};

/** \brief Union of all geometric objects

  This data type unifies all data types that can have references to a
  VECTOR structure. This type is never used for allocation.
*/
union geom_object {
  struct node nd;
  struct edge ed;
  union element el;
};

/** \brief Objects that can have a key */
union object_with_key {
  struct node nd;
  union element el;
  struct vector ve;
  union vertex vertex;
  struct edge edge;
};

typedef struct
{
  UINT VecReserv[MAXVECTORS][MAX_NDOF_MOD_32];
  UINT MatReserv[MAXCONNECTIONS][MAX_NDOF_MOD_32];
  /** \todo Is this used anywhere? */
  UINT VecConsistentStatus[MAXMATRICES][MAX_NDOF_MOD_32];
  /** \todo Is this used anywhere? */
  UINT VecCollectStatus[MAXMATRICES][MAX_NDOF_MOD_32];
} DATA_STATUS;

/** \brief Data type giving access to all objects on a grid level

The \ref grid data type provides access to all objects defined on a grid level.
All the pointers to the first list elements are there.

CONTROL WORDS:

No entries of the control word are currently used.

*/
struct grid {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief A word storing status information. This can be used also by the
      problem class, e.g. to store whether the grid level is assembled or not. */
  INT status;

  /** \brief Level within the multigrid structure */
  INT level;

  /** \brief Number of vertices */
  INT nVert[NS_DIM_PREFIX MAX_PRIOS];

  /** \brief Number of nodes on this grid level */
  INT nNode[NS_DIM_PREFIX MAX_PRIOS];

  /** \brief Number of elements on this grid level */
  INT nElem[NS_DIM_PREFIX MAX_PRIOS];

  /** \brief Number of edges on this grid level */
  INT nEdge;

  /** \brief Number of vectors on this grid level */
  INT nVector[NS_DIM_PREFIX MAX_PRIOS];

  DATA_STATUS data_status;          /* memory management for vectors|matrix */
                                    /* status for consistent and collect    */
  /* pointers */
  union  element *elements[NS_DIM_PREFIX ELEMENT_LISTPARTS];       /* pointer to first element*/
  union  element *lastelement[NS_DIM_PREFIX ELEMENT_LISTPARTS];      /*pointer to last element*/
  union  vertex *vertices[NS_DIM_PREFIX VERTEX_LISTPARTS];            /* pointer to first vertex      */
  union  vertex *lastvertex[NS_DIM_PREFIX VERTEX_LISTPARTS];      /* pointer to last vertex   */
  struct node *firstNode[NS_DIM_PREFIX NODE_LISTPARTS];       /* pointer to first node                */
  struct node *lastNode[NS_DIM_PREFIX NODE_LISTPARTS];        /* pointer to last node                 */
  VECTOR *firstVector[NS_DIM_PREFIX VECTOR_LISTPARTS];        /* pointer to first vector              */
  VECTOR *lastVector[NS_DIM_PREFIX VECTOR_LISTPARTS];         /* pointer to last vector               */

  struct grid *coarser, *finer;         /* coarser and finer grids                              */
  struct multigrid *mg;                         /* corresponding multigrid structure    */

  const PPIF::PPIFContext& ppifContext() const;

#ifdef ModelP
  const DDD::DDDContext& dddContext() const;
  DDD::DDDContext& dddContext();
#endif
};

/** \brief Data type representing a complete multigrid structure

Data type providing access to all information about the complete
multigrid hierarchy, problem description and memory management information.
This is the root of all.
MACROS:

*/
struct multigrid {

  /** \brief The MULTIGRID is an environment item, i.e. it resides in the environment tree.
   *
   * v stores also the name of the MULTIGRID as it is declared in the new
   * and open commands.
   */
  NS_PREFIX ENVDIR v;

  /** \brief Multigrid status word */
  INT status;

  /** \brief used for identification                          */
  INT magic_cookie;

  /** \brief count objects in that multigrid              */
  INT vertIdCounter;

  /** \brief count objects in that multigrid              */
  INT nodeIdCounter;

  /** \brief count objects in that multigrid              */
  INT elemIdCounter;

  /** \brief count objects in that multigrid              */
  INT edgeIdCounter;

#ifndef ModelP
  /** \brief Count vector objects in that multigrid   */
  INT vectorIdCounter;
#endif

  /** \brief Finest grid level currently allocated in the MULTIGRID */
  INT topLevel;

  /** \brief level we are working on
   *
   * Any number between 0 and topLevel. The currentLevel is used by many commands
   * that work on a grid level as default value. It can be changed with the
   * level command from the UG shell.
   */
  INT currentLevel;

  /** \brief last level with complete surface     */
  INT fullrefineLevel;

  /** \brief pointer to BndValProblem                             */
  BVP *theBVP;

  /** \brief description of BVP-properties                */
  std::string BVP_Name;

  /** \brief associated heap structure                    */
  NS_PREFIX HEAP *theHeap;

  /** \brief max nb of properties used in elements*/
  INT nProperty;


  /** \brief memory management for vectors|matrix
   * status for consistent and collect */
  DATA_STATUS data_status;

  /* pointers */
  /** \brief pointers to the grids                                */
  struct grid *grids[MAXLEVEL];

  /** @{ Helper structures to for an O(n) InsertElement           */
  /** \brief List of pointers to face nodes,
      used as an Id of a face
  */
  struct FaceNodes : public std::array<node*,MAX_CORNERS_OF_SIDE>
  {
    using array<node*,MAX_CORNERS_OF_SIDE>::array;
  };
  /** \brief Hasher for FaceNodes */
  struct FaceHasher {
    std::hash<node*> hasher;
    std::size_t operator() (const FaceNodes& key) const {
      return std::accumulate(key.begin(), key.end(),
        144451, [&](auto a, auto b){
          return hasher(a+b);
        });
    }
  };
  /** \brief hash-map used for an O(1) search of the neighboring element
      during InsertElement
   */
  std::unordered_map<FaceNodes,
    std::pair<element *,int>,
    FaceHasher> facemap;
  /** @} */

  /* i/o handling */
  /** \brief 1 if multigrid saved                                 */
  INT saved;

  /** \brief filename if saved                                    */
  char filename[NS_PREFIX NAMESIZE];

  /** \brief coarse grid complete                                 */
  INT CoarseGridFixed;

  /** \brief coarse grid MarkKey for SIMPLE_HEAP Mark/Release     */
  INT MarkKey;

  const PPIF::PPIFContext& ppifContext() const
    { return *ppifContext_; }

  std::shared_ptr<PPIF::PPIFContext> ppifContext_;

#ifdef ModelP
  const DDD::DDDContext& dddContext() const
    { return *dddContext_; }

  DDD::DDDContext& dddContext()
    { return *dddContext_; }

  std::shared_ptr<DDD::DDDContext> dddContext_;
#endif
};

/****************************************************************************/
/*                                                                                                                                                      */
/*                                      typedef for structs                                                                     */
/*                                                                                                                                                      */
/****************************************************************************/

/* geometrical part */
typedef union  vertex VERTEX;
typedef struct node NODE;
typedef union  element ELEMENT;
typedef struct link LINK;
typedef struct edge EDGE;
typedef union  geom_object GEOM_OBJECT;
typedef struct grid GRID;
typedef struct multigrid MULTIGRID;
typedef union object_with_key KEY_OBJECT;


/****************************************************************************/
/*                                                                          */
/* dynamic management of control words                                      */
/*                                                                          */
/****************************************************************************/

        #define ControlWord(p,ce)  (((UINT *)(p))[control_entries[ce].offset_in_object])

        #define CW_READ(p,ce)      ((ControlWord(p,ce) & control_entries[ce].mask)>>control_entries[ce].offset_in_word)

        #define CW_WRITE(p,ce,n)   ControlWord(p,ce) = (ControlWord(p,ce)&control_entries[ce].xor_mask)|(((n)<<control_entries[ce].offset_in_word)&control_entries[ce].mask)

/* static control words */
        #define StaticControlWord(p,t)            (((UINT *)(p))[t ## OFFSET])
        #define StaticControlWordMask(s)          ((POW2(s ## LEN) - 1) << s ## SHIFT)

        #define CW_READ_STATIC(p,s,t)                                                \
  ((StaticControlWord(p,t) &  StaticControlWordMask(s)) >> s ## SHIFT)

        #define CW_WRITE_STATIC(p,s,t,n)                                             \
  StaticControlWord(p,t) =                                           \
    (StaticControlWord(p,t) &  (~StaticControlWordMask(s))) |          \
    (((n) << s ## SHIFT) & StaticControlWordMask(s))

/** \brief Enumeration list of all control words of gm.h */
enum GM_CW {
  VECTOR_CW,
  VERTEX_CW,
  NODE_CW,
  LINK_CW,
  EDGE_CW,
  ELEMENT_CW,
  FLAG_CW,
  PROPERTY_CW,
  GRID_CW,
  GRID_STATUS_CW,
  MULTIGRID_STATUS_CW,

  GM_N_CW
};

/** \brief Enumeration list of all control entry of gm.h */
enum GM_CE {
  VOTYPE_CE,
  VCOUNT_CE,
  VECTORSIDE_CE,
  VCLASS_CE,
  VDATATYPE_CE,
  VNCLASS_CE,
  VNEW_CE,
  VCCUT_CE,
  FINE_GRID_DOF_CE,
  OBJ_CE,
  USED_CE,
  TAG_CE,
  LEVEL_CE,
  THEFLAG_CE,
  MOVE_CE,
  MOVED_CE,
  ONEDGE_CE,
  ONSIDE_CE,
  ONNBSIDE_CE,
  NOOFNODE_CE,
  NSUBDOM_CE,
  NTYPE_CE,
  NPROP_CE,
  MODIFIED_CE,
  NCLASS_CE,
  NNCLASS_CE,
  LOFFSET_CE,
  NO_OF_ELEM_CE,
  AUXEDGE_CE,
  EDGENEW_CE,
  EDSUBDOM_CE,
  ECLASS_CE,
  NSONS_CE,
  NEWEL_CE,
  SUBDOMAIN_CE,
  NODEORD_CE,
  PROP_CE,
#ifdef ModelP
  XFERVECTOR_CE,
#endif

  GM_N_CE
};

enum LV_MODIFIERS {

  LV_VO_INFO              = (1<<1),                     /* vector object related info                   */
  LV_POS                  = (1<<2)                      /* position vector  */
};

/****************************************************************************/
/*                                                                          */
/* Macro definitions for algebra structures                                 */
/*                                                                          */
/*                                                                          */
/* Use of the control word:                                                 */
/*                                                                          */
/* macro name|bits      |V|M|use                                            */
/*                                                                          */
/* all objects:                                                             */
/*                                                                          */
/* vectors:                                                                 */
/* VOTYPE        |0 - 1 |*| | node-,edge-,side- or elemvector               */
/* VCUSED        |4     |*| | flag for general use                          */
/* VCOUNT        |5-6   |*| | The number of elements that reference         */
/*                            the vector (if it is a side vector)           */
/* VECTORSIDE|7 - 9 |*| | nb of side the side vect corr. to (in object elem)*/
/* VCLASS        |11-12 |*| | class of v. (3: if corr. to red/green elem) */
/*                                        (2: if corr. to first algebraic nb.) */
/*                                        (1: if corr. to second algebraic nb.) */
/* VDATATYPE |13-16 |*| | data type used bitwise */
/* VNCLASS       |17-18 |*| | type of elem on finer grid the v. lies geom. in:  */
/*                                                      0: no elem on finer grid                                                */
/*                                                      1: elem of 'second alg. nbhood' only                    */
/*                                                      2: elem of 'first alg. nbhood' only                     */
/*                                                      3: red or green elem                                                    */
/* VNEW          |19    |*| | 1 if vector is new                                                                */
/* VCNEW         |20    |*| | 1 if vector has a new connection                                  */
/* VCCUT         |26    |*| |                                                                                                   */
/*                                                                                                                                                      */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* general macros                                                           */
/*                                                                          */
/****************************************************************************/

/* macros to calculate from a coordinate (2D/3D) a hopefully unique ID */
#define SIGNIFICANT_DIGITS(d,exp_ptr) (ceil(frexp((d),(exp_ptr))*1e5))

/* the idea to calculate from a 2d/3D position a (hopefully) unique key:
   add the weighted significant digits of the coordinates; the weights
   may not have a common divisor to ensure uniqueness of the result;
   take from this again the sigificant digits */
#ifdef UG_DIM_2
#define COORDINATE_TO_KEY(coord,dummy_int_ptr)  ((INT)(SIGNIFICANT_DIGITS((SIGNIFICANT_DIGITS((coord)[0],(dummy_int_ptr))*1.246509423749342 + \
                                                                           SIGNIFICANT_DIGITS((coord)[1],(dummy_int_ptr))*M_PI)\
                                                                          , (dummy_int_ptr))))
#endif

#ifdef UG_DIM_3
#define COORDINATE_TO_KEY(coord,dummy_int_ptr)  ((INT)(SIGNIFICANT_DIGITS((SIGNIFICANT_DIGITS((coord)[0],(dummy_int_ptr))*1.246509423749342 + \
                                                                           SIGNIFICANT_DIGITS((coord)[1],(dummy_int_ptr))*M_PI + \
                                                                           SIGNIFICANT_DIGITS((coord)[2],(dummy_int_ptr))*0.76453456834568356936598)\
                                                                          , (dummy_int_ptr))))
#endif

/****************************************************************************/
/*                                                                          */
/* macros for VECTORs                                                       */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define VECTOR_OFFSET                           offsetof(struct NS_DIM_PREFIX vector, control)/sizeof(UINT)

/* predefined control word entries */
#define VOTYPE_SHIFT                            0
#define VOTYPE_LEN                                      2
#define VOTYPE(p)                                       CW_READ_STATIC(p,VOTYPE_,VECTOR_)
#define SETVOTYPE(p,n)                          CW_WRITE_STATIC(p,VOTYPE_,VECTOR_,n)
#if (MAXVOBJECTS > POW2(VOTYPE_LEN))
        #error  *** VOTYPE_LEN too small ***
#endif

#define VDATATYPE_SHIFT                         4
#define VDATATYPE_LEN                           4
#define VDATATYPE(p)                            CW_READ_STATIC(p,VDATATYPE_,VECTOR_)
#define SETVDATATYPE(p,n)                       CW_WRITE_STATIC(p,VDATATYPE_,VECTOR_,n)
#if (MAXVTYPES > VDATATYPE_LEN)
        #error  *** VDATATYPE_LEN too small ***
#endif

#define VCLASS_SHIFT                            8
#define VCLASS_LEN                                      2
#define VCLASS(p)                                       CW_READ_STATIC(p,VCLASS_,VECTOR_)
#define SETVCLASS(p,n)                          CW_WRITE_STATIC(p,VCLASS_,VECTOR_,n)

#define VNCLASS_SHIFT                           10
#define VNCLASS_LEN                             2
#define VNCLASS(p)                                      CW_READ_STATIC(p,VNCLASS_,VECTOR_)
#define SETVNCLASS(p,n)                         CW_WRITE_STATIC(p,VNCLASS_,VECTOR_,n)

#define VNEW_SHIFT                                      12
#define VNEW_LEN                                        1
#define VNEW(p)                                         CW_READ_STATIC(p,VNEW_,VECTOR_)
#define SETVNEW(p,n)                            CW_WRITE_STATIC(p,VNEW_,VECTOR_,n)

#define VCCUT_SHIFT                             13
#define VCCUT_LEN                                       1
#define VCCUT(p)                                        CW_READ_STATIC(p,VCCUT_,VECTOR_)
#define SETVCCUT(p,n)                           CW_WRITE_STATIC(p,VCCUT_,VECTOR_,n)

#define VCOUNT_SHIFT                            14
#define VCOUNT_LEN                                      2
#define VCOUNT(p)                                       CW_READ_STATIC(p,VCOUNT_,VECTOR_)
#define SETVCOUNT(p,n)                          CW_WRITE_STATIC(p,VCOUNT_,VECTOR_,n)

#define VECTORSIDE_SHIFT                        16
#define VECTORSIDE_LEN                          3
#define VECTORSIDE(p)                           CW_READ_STATIC(p,VECTORSIDE_,VECTOR_)
#define SETVECTORSIDE(p,n)                      CW_WRITE_STATIC(p,VECTORSIDE_,VECTOR_,n)

#define FINE_GRID_DOF_SHIFT             20
#define FINE_GRID_DOF_LEN                       1
#define FINE_GRID_DOF(p)                        CW_READ_STATIC(p,FINE_GRID_DOF_,VECTOR_)
#define SETFINE_GRID_DOF(p,n)           CW_WRITE_STATIC(p,FINE_GRID_DOF_,VECTOR_,n)

#ifdef ModelP
#define XFERVECTOR_SHIFT                        20
#define XFERVECTOR_LEN                          2
#define XFERVECTOR(p)                           CW_READ(p,XFERVECTOR_CE)
#define SETXFERVECTOR(p,n)                      CW_WRITE(p,XFERVECTOR_CE,n)
#endif /* ModelP */

#define VOBJECT(v)                                      ((v)->object)
#ifdef ModelP
#define PPREDVC(p,v)                            (((v)==PRIO_FIRSTVECTOR(p,PrioMaster)) ? \
                                                 PRIO_LASTVECTOR(p,PrioBorder) : (v)->pred)
#else
#define PPREDVC(p,v)                            ((v)->pred)
#endif
#define PREDVC(v)                                       ((v)->pred)
#define SUCCVC(v)                                       ((v)->succ)
#define VINDEX(v)                                       ((v)->index)

/****************************************************************************/
/*                                                                                                                                                      */
/* Macro definitions for geometric objects                                                                      */
/*                                                                                                                                                      */
/*                                                                                                                                                      */
/* Use of the control word:                                                                                             */
/*                                                                                                                                                      */
/* macro name|bits      |V|N|L|E|V|M|  use                                                                              */
/*                                                       C                                                                                      */
/* all objects:                                                                                                                         */
/* TAG           |18-20 | | | |*| | |general purpose tag field                                  */
/* LEVEL         |21-25 |*|*| |*| | |level of a node/element (imp. for copies)  */
/* THEFLAG       |26    |*|*|*|*| | |general purp.,  leave them as you found 'em*/
/* USED          |27    |*|*|*|*| | |object visited, leave them as you found 'em*/
/* OBJT          |28-31 |*|*|*|*| | |object type identification                                 */

/*                                                                                                                                                      */
/* vertices:                                                                                                                            */
/* MOVED         |0     |*| | | | | |boundary vertex not lying on edge midpoint */
/* MOVE          |1-2   |*| | | | | |vertex can be moved on a 0(1,2,3) dim subsp*/
/* ONEDGE        |3 - 6 |*| | | | | |no. of edge in father element                              */
/* ONSIDE        |3 - 5 |*| | | | | |no. of side in father element                              */
/* ONNBSIDE      |6 - 8 |*| | | | | |no. of side in the neighbor of the father   */
/* NOOFNODE      |9 -13 |*| | | | | |???                                                                            */
/*                                                                                                                                                      */
/* nodes:                                                                                                                                       */
/* NSUBDOM       |0-3   | |*| | | | |subdomain id                                       */
/* MODIFIED  |6         | |*| | | | |1 if node must be assembled                                */
/* N_OUTFLOW |0-7       |                                                                                                               */
/* N_INFLOW  |8-15      |                                                                                                               */
/*                                                                                                                                                      */
/* links and edges:                                                                                                             */
/* LOFFSET       |0     | | |*| | | |position of link in links array                    */
/* EDGENEW       |1     | | |*| | | |status of edge                                                             */
/* NOOFELEM  |2-8       | | |*| | | |nb. of elem. the edge is part of                   */
/* AUXEDGE       |9             |                                                                                                               */
/* EDSUBDOM  |12-17 | | | |*| | |subdomain of edge if inner edge, 0 else        */
/*                                                                                                                                                      */
/* elements:                                                                                                                            */
/* ECLASS        |8-9   | | | |*| | |element class from enumeration type                */
/* NSONS         |10-13 | | | |*| | |number of sons                                                     */
/* NEWEL         |14    | | | |*| | |element just created                                               */
/* VSIDES        |11-14 | | | |*| | |viewable sides                                                     */
/* NORDER        |15-19 | | | |*| | |view position order of the nodes                   */
/* CUTMODE       |26-27 | | | |*| | |elem intersects cutplane or...                     */
/*                                                                                                                                                      */
/****************************************************************************/

/* object identification */
enum GM_OBJECTS {

  MGOBJ,                            /*!< Multigrid object                             */
  IVOBJ,                            /*!< Inner vertex                                         */
  BVOBJ,                            /*!< Boundary vertex                                      */
  IEOBJ,                            /*!< Inner element                                        */
  BEOBJ,                            /*!< Boundary element                             */
  EDOBJ,                            /*!< Edge object                                          */
  NDOBJ,                            /*!< Node object                                          */
  GROBJ,                            /*!< Grid object                                          */

  /* object numbers for algebra */
  VEOBJ,                            /*!< Vector object                                        */

  NPREDEFOBJ,                       /*!< Number of predefined objects             */

  NOOBJ = -1                        /*!< No object */
};
#define LIOBJ           EDOBJ           /* link and edge are identified                 */

/****************************************************************************/
/*                                                                                                                                                      */
/* general macros                                                                                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset */
#define GENERAL_CW                                      NODE_CW         /* any of the geom objects      */
#define GENERAL_OFFSET                          0

#define OBJ_SHIFT                                       28
#define OBJ_LEN                                         4
#define OBJT(p)                                         (enum GM_OBJECTS)CW_READ_STATIC(p,OBJ_,GENERAL_)
#define SETOBJT(p,n)                            CW_WRITE_STATIC(p,OBJ_,GENERAL_,n)
#define OBJT_MAX                                        (POW2(OBJ_LEN)-1)

#define USED_SHIFT                                      27
#define USED_LEN                                        1
#define USED(p)                                         CW_READ_STATIC(p,USED_,GENERAL_)
#define SETUSED(p,n)                            CW_WRITE_STATIC(p,USED_,GENERAL_,n)

#define THEFLAG_SHIFT                           26
#define THEFLAG_LEN                             1
#define THEFLAG(p)                                      CW_READ_STATIC(p,THEFLAG_,GENERAL_)
#define SETTHEFLAG(p,n)                         CW_WRITE_STATIC(p,THEFLAG_,GENERAL_,n)

#define LEVEL_SHIFT                             21
#define LEVEL_LEN                                       5
#define LEVEL(p)                                        CW_READ_STATIC(p,LEVEL_,GENERAL_)
#define SETLEVEL(p,n)                           CW_WRITE_STATIC(p,LEVEL_,GENERAL_,n)

#define TAG_SHIFT                                       18
#define TAG_LEN                                         3
#define TAG(p)                                          CW_READ_STATIC(p,TAG_,GENERAL_)
#define SETTAG(p,n)                             CW_WRITE_STATIC(p,TAG_,GENERAL_,n)

#define REF2TAG(n)                                      (reference2tag[n])

#define CTRL(p)         (*((UINT *)(p)))
#define ID(p)           (((INT *)(p))[1])

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for vertices                                                                                                          */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset */
#define VERTEX_OFFSET                           offsetof(struct ivertex,control)/sizeof(UINT)

#define MOVE_SHIFT                                      1
#define MOVE_LEN                                        2
#define MOVE(p)                                         CW_READ_STATIC(p,MOVE_,VERTEX_)
#define SETMOVE(p,n)                            CW_WRITE_STATIC(p,MOVE_,VERTEX_,n)

#define MOVED_SHIFT                             0
#define MOVED_LEN                                       1
#define MOVED(p)                                        CW_READ_STATIC(p,MOVED_,VERTEX_)
#define SETMOVED(p,n)                           CW_WRITE_STATIC(p,MOVED_,VERTEX_,n)

#define ONEDGE_SHIFT                            3
#define ONEDGE_LEN                                      4
#define ONEDGE(p)                                       CW_READ_STATIC(p,ONEDGE_,VERTEX_)
#define SETONEDGE(p,n)                          CW_WRITE_STATIC(p,ONEDGE_,VERTEX_,n)

/* the following two overlap with ONEDGE */
#define ONSIDE_SHIFT                            3
#define ONSIDE_LEN                                      3
#define ONSIDE(p)                                       CW_READ_STATIC(p,ONSIDE_,VERTEX_)
#define SETONSIDE(p,n)                          CW_WRITE_STATIC(p,ONSIDE_,VERTEX_,n)

#define ONNBSIDE_SHIFT                          6
#define ONNBSIDE_LEN                            3
#define ONNBSIDE(p)                                     CW_READ_STATIC(p,ONNBSIDE_,VERTEX_)
#define SETONNBSIDE(p,n)                        CW_WRITE_STATIC(p,ONNBSIDE_,VERTEX_,n)

#define NOOFNODE_SHIFT                          9
#define NOOFNODE_LEN                            5
#define NOOFNODEMAX                                     POW2(NOOFNODE_LEN)
#if (MAXLEVEL > NOOFNODEMAX)
#error  ****  set NOOFNODEMAX/_LEN appropriate to MAXLEVEL: 2^NOOFNODE_LEN = NOOFNODEMAX >= MAXLEVEL ****
#endif
#define NOOFNODE(p)                                     CW_READ_STATIC(p,NOOFNODE_,VERTEX_)
#define SETNOOFNODE(p,n)                        CW_WRITE_STATIC(p,NOOFNODE_,VERTEX_,n)
#define INCNOOFNODE(p)                          SETNOOFNODE(p,NOOFNODE(p)+1)
#define DECNOOFNODE(p)                          SETNOOFNODE(p,NOOFNODE(p)-1)

#define PREDV(p)                ((p)->iv.pred)
#define SUCCV(p)                ((p)->iv.succ)
#define CVECT(p)                ((p)->iv.x)
#define LCVECT(p)               ((p)->iv.xi)
#define VDATA(p)                ((p)->iv.data)
#define VFATHER(p)              ((p)->iv.father)

/* for boundary vertices */
#define V_BNDP(p)               ((p)->bv.bndp)

/* parallel macros */
#ifdef ModelP
#define PARHDRV(p)              (&((p)->iv.ddd))
#endif /* ModelP */

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for nodes                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset */
#define NODE_OFFSET                 offsetof(struct node, control)/sizeof(UINT)

#define NTYPE_SHIFT                                     0
#define NTYPE_LEN                                       3
#define NTYPE(p)                                        CW_READ_STATIC(p,NTYPE_,NODE_)
#define SETNTYPE(p,n)                           CW_WRITE_STATIC(p,NTYPE_,NODE_,n)

#define NSUBDOM_SHIFT                           3
#define NSUBDOM_LEN                                     6
#define NSUBDOM(p)                                      CW_READ_STATIC(p,NSUBDOM_,NODE_)
#define SETNSUBDOM(p,n)                         CW_WRITE_STATIC(p,NSUBDOM_,NODE_,n)

#define NPROP_SHIFT                 11
#define NPROP_LEN                   4
#define NPROP(p)                    CW_READ_STATIC(p,NPROP_,NODE_)
#define SETNPROP(p,n)               CW_WRITE_STATIC(p,NPROP_,NODE_,n)

#define MODIFIED_SHIFT                          15
#define MODIFIED_LEN                            1
#define MODIFIED(p)                             CW_READ_STATIC(p,MODIFIED_,NODE_)
#define SETMODIFIED(p,n)                        CW_WRITE_STATIC(p,MODIFIED_,NODE_,n)

#define NCLASS_SHIFT                16
#define NCLASS_LEN                  2
#define NCLASS(p)                   CW_READ_STATIC(p,NCLASS_,NODE_)
#define SETNCLASS(p,n)              CW_WRITE_STATIC(p,NCLASS_,NODE_,n)

#define NNCLASS_SHIFT               18
#define NNCLASS_LEN                 2
#define NNCLASS(p)                  CW_READ_STATIC(p,NNCLASS_,NODE_)
#define SETNNCLASS(p,n)             CW_WRITE_STATIC(p,NNCLASS_,NODE_,n)

#define PREDN(p)                        ((p)->pred)
#define SUCCN(p)                        ((p)->succ)
#define START(p)                        ((p)->start)

#define NFATHER(p)                      ((NODE*)(p)->father)
#define SETNFATHER(p,n)         ((p)->father = n)
#define NFATHEREDGE(p)          ((EDGE*)(p)->father)
/*
   #define NFATHER(p)                      ((NTYPE(p) == CORNER_NODE) ? (p)->father : NULL)
   #define NFATHEREDGE(p)          ((NTYPE(p) == MID_NODE) ? (EDGE *)(p)->father : NULL)
   #define SETNFATHEREDGE(p,e)     ((p)->father = (NODE *) (e))
 */
#define CORNERTYPE(p)           (NTYPE(p) == CORNER_NODE)
#define MIDTYPE(p)                      (NTYPE(p) == MID_NODE)
#define SIDETYPE(p)                     (NTYPE(p) == SIDE_NODE)
#define CENTERTYPE(p)           (NTYPE(p) == CENTER_NODE)

#define SONNODE(p)                      ((p)->son)
#define MYVERTEX(p)             ((p)->myvertex)

#define ELEMENT_PTR(p)                  ((p)->el)

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for links                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

/* CAUTION: the controlword of LINK0 and its edge are identical (AVOID overlapping of flags) */

/* control word offset */
#define LINK_OFFSET                             offsetof(struct link, control)/sizeof(UINT)

#define LOFFSET_SHIFT                           0
#define LOFFSET_LEN                             1
#define LOFFSET(p)                                      CW_READ(p,LOFFSET_CE)
#define SETLOFFSET(p,n)                         CW_WRITE(p,LOFFSET_CE,n)

/** \brief Get the neighboring node of a link */
#define NBNODE(p)                                       ((p)->nbnode)
#define NEXT(p)                                         ((p)->next)
#define LDATA(p)                                        ((p)->matelem)
#define MATELEM(p)                                      ((p)->matelem)  /* can be used for node and link */

#define MYEDGE(p)                                       ((EDGE *)((p)-LOFFSET(p)))
/**
Macro that provides fast access to the LINK in the reverse direction.
If the given LINK connects node `a` with node `b`, then REVERSE
provides the LINK in the list of node `b` pointing to `a`.
*/
#define REVERSE(p)                                      ((p)+(1-LOFFSET(p)*2))

#if defined(UG_DIM_2)
#define LELEM(p)                                        ((p)->elem)
#define SET_LELEM(p,e)                                  ((p)->elem = (e))
#endif

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for edges                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset: Use the control word of the link object contained in the edge object */

#define EDGE_OFFSET                             offsetof(struct edge, links[0].control)/sizeof(UINT)

#define NO_OF_ELEM_SHIFT                        2
#define NO_OF_ELEM_LEN                          7
#define NO_OF_ELEM_MAX                          128
#define NO_OF_ELEM(p)                           CW_READ(p,NO_OF_ELEM_CE)
#define SET_NO_OF_ELEM(p,n)             CW_WRITE(p,NO_OF_ELEM_CE,n)
#define INC_NO_OF_ELEM(p)                       SET_NO_OF_ELEM(p,NO_OF_ELEM(p)+1)
#define DEC_NO_OF_ELEM(p)                       SET_NO_OF_ELEM(p,NO_OF_ELEM(p)-1)

#define AUXEDGE_SHIFT                           9
#define AUXEDGE_LEN                             1
#define AUXEDGE(p)                                      CW_READ(p,AUXEDGE_CE)
#define SETAUXEDGE(p,n)                         CW_WRITE(p,AUXEDGE_CE,n)

#define EDGENEW_SHIFT                           1
#define EDGENEW_LEN                             1
#define EDGENEW(p)                                      CW_READ(p,EDGENEW_CE)
#define SETEDGENEW(p,n)                         CW_WRITE(p,EDGENEW_CE,n)

/* boundary edges will be indicated by a subdomain id of 0 */
#define EDSUBDOM_SHIFT                          12
#define EDSUBDOM_LEN                            6
#define EDSUBDOM(p)                                     CW_READ(p,EDSUBDOM_CE)
#define SETEDSUBDOM(p,n)                        CW_WRITE(p,EDSUBDOM_CE,n)

#define LINK0(p)        (&((p)->links[0]))
#define LINK1(p)        (&((p)->links[1]))
#define MIDNODE(p)      ((p)->midnode)
#define EDDATA(p)       ((p)->data)
#define EDVECTOR(p) ((p)->vector)

/****************************************************************************/
/*                                                                          */
/* macros for elements                                                      */
/*                                                                          */
/****************************************************************************/

enum {TRIANGLE = 3,
      QUADRILATERAL = 4};

enum {TETRAHEDRON = 4,
      PYRAMID = 5,
      PRISM = 6,
      HEXAHEDRON = 7};

/* control word offsets */
#define ELEMENT_OFFSET                  offsetof(struct generic_element, control)/sizeof(UINT)
#define FLAG_OFFSET                     offsetof(struct generic_element, flag)/sizeof(UINT)
#define PROPERTY_OFFSET                 offsetof(struct generic_element, property)/sizeof(UINT)

/* macros for control word */
#define ECLASS_SHIFT                                    8
#define ECLASS_LEN                                              2
#define ECLASS(p)                                               CW_READ(p,ECLASS_CE)
#define SETECLASS(p,n)                                  CW_WRITE(p,ECLASS_CE,n)

#define NSONS_SHIFT                                     10
#define NSONS_LEN                                               5
#define NSONS(p)                                                CW_READ(p,NSONS_CE)
#define SETNSONS(p,n)                                   CW_WRITE(p,NSONS_CE,n)

#define NEWEL_SHIFT                                     17
#define NEWEL_LEN                                               1
#define NEWEL(p)                                                CW_READ(p,NEWEL_CE)
#define SETNEWEL(p,n)                                   CW_WRITE(p,NEWEL_CE,n)

/* macros for flag word                           */
/* are obviously all for internal use */

/* the property field */
#define SUBDOMAIN_SHIFT                 24
#define SUBDOMAIN_LEN                   6
#define SUBDOMAIN(p)                    CW_READ(p,SUBDOMAIN_CE)
#define SETSUBDOMAIN(p,n)               CW_WRITE(p,SUBDOMAIN_CE,n)

#define NODEORD_SHIFT                   0
#define NODEORD_LEN                     24
#define NODEORD(p)                      CW_READ(p,NODEORD_CE)
#define SETNODEORD(p,n)                 CW_WRITE(p,NODEORD_CE,n)

#define PROP_SHIFT                      30
#define PROP_LEN                        2
#define PROP(p)                         CW_READ(p,PROP_CE)
#define SETPROP(p,n)                    CW_WRITE(p,PROP_CE,n)

/* parallel macros */
#ifdef ModelP
#define PARTITION(p)                    ((p)->ge.lb1)
#define PARHDRE(p)                      (&((p)->ge.ddd))
#endif

/*******************************/
/* the general element concept */
/*******************************/

/** \brief This structure contains all topological properties
    of an element and more ..
 */
struct GENERAL_ELEMENT {
  INT tag;                           /**< Element type to be defined       */

  /* the following parameters determine size of refs array in element */
  INT sides_of_elem;                 /**< How many sides ?                 */
  INT corners_of_elem;               /**< How many corners ?               */

  /* local geometric description of the element */
  DOUBLE_VECTOR local_corner[MAX_CORNERS_OF_ELEM];                /**< Local coordinates of the corners of the element */

  /* more size parameters */
  INT edges_of_elem;                 /**< How many edges ?         */
  INT edges_of_side[MAX_SIDES_OF_ELEM];     /**< Number of edges for each side        */
  INT corners_of_side[MAX_SIDES_OF_ELEM];   /**< Number of corners for each side  */

  /* index computations */
  /* Within each element sides, edges, corners are numbered in some way.      */
  /* Within each side the edges and corners are numbered, within the edge the */
  /* corners are numbered. The following arrays map the local numbers within  */
  /* the side or edge to the numbering within the element.                                        */
  INT edge_of_side[MAX_SIDES_OF_ELEM][MAX_EDGES_OF_SIDE];
  INT corner_of_side[MAX_SIDES_OF_ELEM][MAX_CORNERS_OF_SIDE];
  INT corner_of_edge[MAX_EDGES_OF_ELEM][CORNERS_OF_EDGE];

  /* the following parameters are derived from data above */
  GM_OBJECTS mapped_inner_objt = NOOBJ;                               /* tag to objt mapping for free list*/
  GM_OBJECTS mapped_bnd_objt = NOOBJ;                                 /* tag to objt mapping for free list*/
  INT inner_size, bnd_size;                                 /* size in bytes used for alloc     */
  INT edge_with_corners[MAX_CORNERS_OF_ELEM][MAX_CORNERS_OF_ELEM];
  INT side_with_edge[MAX_EDGES_OF_ELEM][MAX_SIDES_OF_EDGE];
  INT corner_of_side_inv[MAX_SIDES_OF_ELEM][MAX_CORNERS_OF_ELEM];
  INT edges_of_corner[MAX_CORNERS_OF_ELEM][MAX_EDGES_OF_ELEM];
  INT corner_opp_to_side[MAX_SIDES_OF_ELEM];
  INT opposite_edge[MAX_EDGES_OF_ELEM];
  INT side_opp_to_corner[MAX_CORNERS_OF_ELEM];
  INT edge_of_corner[MAX_CORNERS_OF_ELEM][MAX_EDGES_OF_ELEM];
  INT edge_of_two_sides[MAX_SIDES_OF_ELEM][MAX_SIDES_OF_ELEM];

  /* ... the refinement rules should be placed here later */
};

END_UGDIM_NAMESPACE

/** \todo move this to include section, when other general element stuff is separated */
#include "elements.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* macros for element descriptors                                           */
/*                                                                          */
/****************************************************************************/

/** @name Macros to access element descriptors by element pointers             */
/*@{*/
/** Returns the number of sides of a given element. In 2D the number of sides and number
  of edges of an element coincide. The nb, sidevector and side
  arrays are of this size.
*/
#define SIDES_OF_ELEM(p)                (element_descriptors[TAG(p)]->sides_of_elem)
/** Returns the number of edges of a given element. In 2D the number of sides and number
  of edges of an element coincide.
*/
#define EDGES_OF_ELEM(p)                (element_descriptors[TAG(p)]->edges_of_elem)
/** Returns the number of corners of a given element. This is also the size of the n array.
*/
#define CORNERS_OF_ELEM(p)              (element_descriptors[TAG(p)]->corners_of_elem)
#define LOCAL_COORD_OF_ELEM(p,c)    (element_descriptors[TAG(p)]->local_corner[(c)])

/** \brief Returns the number of edges of side i of the given element.
 *
 * In 2D a side has always one edge since edges and sides coincide.
 */
#define EDGES_OF_SIDE(p,i)              (element_descriptors[TAG(p)]->edges_of_side[(i)])

/** \brief Returns the number of corners of side i of the given element. */
#define CORNERS_OF_SIDE(p,i)            (element_descriptors[TAG(p)]->corners_of_side[(i)])

/** \brief Returns the number of edge e of side s within the element p. */
#define EDGE_OF_SIDE(p,s,e)             (element_descriptors[TAG(p)]->edge_of_side[(s)][(e)])
#define EDGE_OF_TWO_SIDES(p,s,t)        (element_descriptors[TAG(p)]->edge_of_two_sides[(s)][(t)])
/** \brief Returns the number of corner c of side s within the element p. */
#define CORNER_OF_SIDE(p,s,c)           (element_descriptors[TAG(p)]->corner_of_side[(s)][(c)])
/** \brief Returns the number of corner c of edge e within the element p. */
#define CORNER_OF_EDGE(p,e,c)           (element_descriptors[TAG(p)]->corner_of_edge[(e)][(c)])

/** \brief Returns the number of the edge (with respect to numbering in the element p)
 * that connects corners with numbers c0 and c1 (with respect to numbering in
 * element p).
 */
#define EDGE_WITH_CORNERS(p,c0,c1)      (element_descriptors[TAG(p)]->edge_with_corners[(c0)][(c1)])

/** SIDE_WITH_EDGE(p,e,0) and SIDE_WITH_EDGE(p,e,1) deliver the numbers
 * of the two sides (with respect to numbering in p) that share edge number e
 * (also with respect to the element).
 */
#define SIDE_WITH_EDGE(p,e,k)           (element_descriptors[TAG(p)]->side_with_edge[(e)][(k)])

/** When c is the number of a corner and s is the number of a side
 * (both with respect to numbering in the element) then
 * CORNER_OF_SIDE_INV returns the number of corner c with respect
 * to the numbering in side s or -1 if s does not contain corner c.
 */
#define CORNER_OF_SIDE_INV(p,s,c)       (element_descriptors[TAG(p)]->corner_of_side_inv[(s)][(c)])

/** If c is the number of a corner with respect to numbering within
 * the element then EDGES_OF_CORNER(p,c,k) returns all numbers
 * of edges (with respect to p) that are connected to corner c.
 * Since this number is variable a value of -1 identifies an invalid entry.
 * The edges are numbered consecutively, i.e. if a -1 is encountered
 * larger values of k (less than EDGES_OF_ELEM(p)) will lead also to -1).
 */
#define EDGES_OF_CORNER(p,c,k)          (element_descriptors[TAG(p)]->edges_of_corner[(c)][(k)])
#define CORNER_OPP_TO_SIDE(p,s)         (element_descriptors[TAG(p)]->corner_opp_to_side[(s)])
#define OPPOSITE_EDGE(p,e)                  (element_descriptors[TAG(p)]->opposite_edge[(e)])
#define SIDE_OPP_TO_CORNER(p,c)         (element_descriptors[TAG(p)]->side_opp_to_corner[(c)])
#define EDGE_OF_CORNER(p,c,e)           (element_descriptors[TAG(p)]->edge_of_corner[(c)][(e)])

#define FLAG(p)                 ((p)->ge.flag)
#define SUCCE(p)                ((p)->ge.succ)
#define PREDE(p)                ((p)->ge.pred)

#ifdef __CENTERNODE__
#define CENTERNODE(p)   ((p)->ge.centernode)
#endif
/** \brief Returns a pointer to corner i of element p.
 *
 * i should be less than CORNERS_OF_ELEM(p).
 */
#define CORNER(p,i)     ((NODE *) (p)->ge.refs[n_offset[TAG(p)]+(i)])

/** \brief Returns a pointer to the father element. */
#define EFATHER(p)              ((ELEMENT *) (p)->ge.refs[father_offset[TAG(p)]])

/** \brief Returns a pointer to son i of p in the element hierarchy.
 *
 * In 3D only i=0 is allowed and the function GetSons should be used instead.
 * i should be smaller than NSONS(p).
 */
#define SON(p,i)                ((ELEMENT *) (p)->ge.refs[sons_offset[TAG(p)]+(i)])

/** \todo NbElem is declared in ugm.h, but never defined.
    We need a clean solution. */
/*
   #if defined(UG_DIM_2)
   #define NBELEM(p,i)     NbElem((p),(i))
   #else
 */
/** \brief Returns a pointer to neighboring element over side i.
 *
 * i should be less than SIDES_OF_ELEM(p).
 */
#define NBELEM(p,i)     ((ELEMENT *) (p)->ge.refs[nb_offset[TAG(p)]+(i)])
/*
   #endif
 */
#define ELEM_BNDS(p,i)  ((BNDS *) (p)->ge.refs[side_offset[TAG(p)]+(i)])
#define EVECTOR(p)              ((VECTOR *) (p)->ge.refs[0])

/** \brief Returns a pointer to the VECTOR associated with the side i of element p.
 *
 * i must be less than SIDES_OF_ELEM(p).
 */
#define SVECTOR(p,i)    ((VECTOR *) (p)->ge.refs[svector_offset[TAG(p)]+(i)])
#define SIDE_ON_BND(p,i) (ELEM_BNDS(p,i) != NULL)
#define INNER_SIDE(p,i)  (ELEM_BNDS(p,i) == NULL)
#define INNER_BOUNDARY(p,i) (InnerBoundary(p,i))
/* TODO: replace by function call */

#ifdef UG_DIM_2
#define EDGE_ON_BND(p,i) (ELEM_BNDS(p,i) != NULL)
#endif

#ifdef UG_DIM_3
#define EDGE_ON_BND(p,i) (SIDE_ON_BND(p,SIDE_WITH_EDGE(p,i,0)) || \
                          SIDE_ON_BND(p,SIDE_WITH_EDGE(p,i,1)))
#endif
/*@}*/

/* use the following macros to assign values, since definition  */
/* above is no proper lvalue. */
#ifdef __CENTERNODE__
#define SET_CENTERNODE(p,q) ((p)->ge.centernode = q)
#endif
#define SET_CORNER(p,i,q)       ((p)->ge.refs[n_offset[TAG(p)]+(i)] = q)
#define SET_EFATHER(p,q)        ((p)->ge.refs[father_offset[TAG(p)]] = q)
#define SET_SON(p,i,q)          ((p)->ge.refs[sons_offset[TAG(p)]+(i)] = q)
/** \todo Set_NbElem is declared in ugm.h, but never defined.
    We need a clean solution. */
/*
   #if defined(UG_DIM_2)
   #define SET_NBELEM(p,i,q)       Set_NbElem((p),(i),(q))
   #else
 */
#define SET_NBELEM(p,i,q)       ((p)->ge.refs[nb_offset[TAG(p)]+(i)] = q)
/*
   #endif
 */
#if defined(UG_DIM_2)
#define VOID_NBELEM(p,i)        NBELEM(p,i)
#else
#define VOID_NBELEM(p,i)        ((p)->ge.refs[nb_offset[TAG(p)]+(i)])
#endif
#define SET_BNDS(p,i,q)         ((p)->ge.refs[side_offset[TAG(p)]+(i)] = q)
#define SET_EVECTOR(p,q)        ((p)->ge.refs[evector_offset[TAG(p)]] = q)
#if defined(UG_DIM_3)
#define SET_SVECTOR(p,i,q)      ((p)->ge.refs[svector_offset[TAG(p)]+(i)] = q)
#endif

/** @name Macros to access corner pointers directly */
/*@{*/
#define CORNER_OF_EDGE_PTR(e,i,j)               (CORNER(e,CORNER_OF_EDGE(e,i,j)))
#define CORNER_OF_SIDE_PTR(e,i,j)               (CORNER(e,CORNER_OF_SIDE(e,i,j)))
/*@}*/

/** @name Macros to access element descriptors by element tags */
/*@{*/
#define INNER_SIZE_TAG(t)                       (element_descriptors[t]->inner_size)
#define BND_SIZE_TAG(t)                         (element_descriptors[t]->bnd_size)
#define MAPPED_INNER_OBJT_TAG(t)                (element_descriptors[t]->mapped_inner_objt)
#define MAPPED_BND_OBJT_TAG(t)                  (element_descriptors[t]->mapped_bnd_objt)

#define SIDES_OF_TAG(t)                         (element_descriptors[t]->sides_of_elem)
#define EDGES_OF_TAG(t)                         (element_descriptors[t]->edges_of_elem)
#define CORNERS_OF_TAG(t)                       (element_descriptors[t]->corners_of_elem)
#define LOCAL_COORD_OF_TAG(t,c)                 (element_descriptors[t]->local_corner[(c)])

#define EDGES_OF_SIDE_TAG(t,i)                  (element_descriptors[t]->edges_of_side[(i)])
#define CORNERS_OF_SIDE_TAG(t,i)                (element_descriptors[t]->corners_of_side[(i)])

#define EDGE_OF_SIDE_TAG(t,s,e)                 (element_descriptors[t]->edge_of_side[(s)][(e)])
#define EDGE_OF_TWO_SIDES_TAG(t,s,u)    (element_descriptors[t]->edge_of_two_sides[(s)][(u)])
#define CORNER_OF_SIDE_TAG(t,s,c)               (element_descriptors[t]->corner_of_side[(s)][(c)])
#define CORNER_OF_EDGE_TAG(t,e,c)               (element_descriptors[t]->corner_of_edge[(e)][(c)])

#define EDGE_WITH_CORNERS_TAG(t,c0,c1)  (element_descriptors[t]->edge_with_corners[(c0)][(c1)])
#define SIDE_WITH_EDGE_TAG(t,e,k)               (element_descriptors[t]->side_with_edge[(e)][(k)])
#define CORNER_OF_SIDE_INV_TAG(t,s,c)   (element_descriptors[t]->corner_of_side_inv[(s)][(c)])
#define EDGES_OF_CORNER_TAG(t,c,k)              (element_descriptors[t]->edges_of_corner[(c)][(k)])
#define CORNER_OPP_TO_SIDE_TAG(t,s)             (element_descriptors[t]->corner_opp_to_side[(s)])
#define OPPOSITE_EDGE_TAG(t,e)              (element_descriptors[t]->opposite_edge[(e)])
#define SIDE_OPP_TO_CORNER_TAG(t,c)             (element_descriptors[t]->side_opp_to_corner[(c)])
#define EDGE_OF_CORNER_TAG(t,c,e)               (element_descriptors[t]->edge_of_corner[(c)][(e)])
/*@}*/

/****************************************************************************/
/*                                                                          */
/* macros for grids                                                         */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define GRID_OFFSET                                     offsetof(grid,control)/sizeof(UINT)
#define GRID_STATUS_OFFSET                              offsetof(grid,status)/sizeof(UINT)

#define GLEVEL(p)                                       ((p)->level)
#define GFORMAT(p)                                      MGFORMAT(MYMG(p))
#define SETGLOBALGSTATUS(p)             ((p)->status=~0)
#define GSTATUS(p,n)                            ((p)->status&(n))
/** \brief Set or reset all bits that are 1 in the mask n. */
#define RESETGSTATUS(p,n)                       ((p)->status&=~(n))

#ifdef ModelP
#define PFIRSTELEMENT(p)                                ((LISTPART_FIRSTELEMENT(p,0)!=NULL) ?\
                                                         (LISTPART_FIRSTELEMENT(p,0)) : (FIRSTELEMENT(p)))
#define PRIO_FIRSTELEMENT(p,prio)               ((p)->elements[PRIO2LISTPART(ELEMENT_LIST,prio)])
#define LISTPART_FIRSTELEMENT(p,part)   ((p)->elements[part])
#define FIRSTELEMENT(p)                                 ((p)->elements[PRIO2LISTPART(ELEMENT_LIST,PrioMaster)])

#define PLASTELEMENT(p)                                 LASTELEMENT(p)
#define PRIO_LASTELEMENT(p,prio)                ((p)->lastelement[PRIO2LISTPART(ELEMENT_LIST,prio)])
#define LISTPART_LASTELEMENT(p,part)    ((p)->lastelement[part])
#define LASTELEMENT(p)                                  ((p)->lastelement[PRIO2LISTPART(ELEMENT_LIST,PrioMaster)])
#else
#define FIRSTELEMENT(p)         ((p)->elements[0])
#define PFIRSTELEMENT(p)        FIRSTELEMENT(p)
#define LASTELEMENT(p)          ((p)->lastelement[0])
#define PLASTELEMENT(p)         LASTELEMENT(p)
#endif

#ifdef ModelP
#define PFIRSTVERTEX(p)                                 ((LISTPART_FIRSTVERTEX(p,0)!=NULL) ?\
                                                         (LISTPART_FIRSTVERTEX(p,0)) :\
                                                         ((LISTPART_FIRSTVERTEX(p,1)!=NULL) ?\
                                                          (LISTPART_FIRSTVERTEX(p,1)) : (FIRSTVERTEX(p))))
#define PRIO_FIRSTVERTEX(p,prio)                ((p)->vertices[PRIO2LISTPART(VERTEX_LIST,prio)])
#define LISTPART_FIRSTVERTEX(p,part)    ((p)->vertices[part])
#define FIRSTVERTEX(p)                                  (((p)->vertices[PRIO2LISTPART(VERTEX_LIST,\
                                                                                      PrioBorder)]!=NULL) ?\
                                                         (p)->vertices[PRIO2LISTPART(VERTEX_LIST,PrioBorder)] :\
                                                         (p)->vertices[PRIO2LISTPART(VERTEX_LIST,PrioMaster)])
#define SFIRSTVERTEX(p)                                  (p)->vertices[PRIO2LISTPART(VERTEX_LIST,PrioMaster)]

#define PLASTVERTEX(p)                                  LASTVERTEX(p)
#define PRIO_LASTVERTEX(p,prio)                 ((p)->lastvertex[PRIO2LISTPART(VERTEX_LIST,prio)])
#define LISTPART_LASTVERTEX(p,part)     ((p)->lastvertex[part])
#define LASTVERTEX(p)                                   ((p)->lastvertex[PRIO2LISTPART(VERTEX_LIST,PrioMaster)])
#else
#define FIRSTVERTEX(p)          ((p)->vertices[0])
#define PFIRSTVERTEX(p)         FIRSTVERTEX(p)
#define SFIRSTVERTEX(p)         FIRSTVERTEX(p)
#define LASTVERTEX(p)           ((p)->lastvertex[0])
#define PLASTVERTEX(p)          LASTVERTEX(p)
#endif

#define FIRSTELEMSIDE(p)        ((p)->sides)

#ifdef ModelP
#define PFIRSTNODE(p)                                   ((LISTPART_FIRSTNODE(p,0)!=NULL) ?\
                                                         (LISTPART_FIRSTNODE(p,0)) :\
                                                         ((LISTPART_FIRSTNODE(p,1)!=NULL) ?\
                                                          (LISTPART_FIRSTNODE(p,1)) : (FIRSTNODE(p))))
#define PRIO_FIRSTNODE(p,prio)                  ((p)->firstNode[PRIO2LISTPART(NODE_LIST,prio)])
#define LISTPART_FIRSTNODE(p,part)              ((p)->firstNode[part])
#define FIRSTNODE(p)                                    (((p)->firstNode[PRIO2LISTPART(NODE_LIST,\
                                                                                       PrioBorder)]!=NULL) ?\
                                                         (p)->firstNode[PRIO2LISTPART(NODE_LIST,PrioBorder)] :\
                                                         (p)->firstNode[PRIO2LISTPART(NODE_LIST,PrioMaster)])
#define SFIRSTNODE(p)                                    (p)->firstNode[PRIO2LISTPART(NODE_LIST,PrioMaster)]

#define PLASTNODE(p)                                    LASTNODE(p)
#define PRIO_LASTNODE(p,prio)                   ((p)->lastNode[PRIO2LISTPART(NODE_LIST,prio)])
#define LISTPART_LASTNODE(p,part)               ((p)->lastNode[part])
#define LASTNODE(p)                                     ((p)->lastNode[PRIO2LISTPART(NODE_LIST,PrioMaster)])
#else
#define FIRSTNODE(p)            ((p)->firstNode[0])
#define PFIRSTNODE(p)           FIRSTNODE(p)
#define SFIRSTNODE(p)           FIRSTNODE(p)
#define LASTNODE(p)             ((p)->lastNode[0])
#define PLASTNODE(p)            LASTNODE(p)
#endif

#ifdef ModelP
#define PFIRSTVECTOR(p)                                 ((LISTPART_FIRSTVECTOR(p,0)!=NULL) ?\
                                                         (LISTPART_FIRSTVECTOR(p,0)) :\
                                                         ((LISTPART_FIRSTVECTOR(p,1)!=NULL) ?\
                                                          (LISTPART_FIRSTVECTOR(p,1)) : (FIRSTVECTOR(p))))
#define PRIO_FIRSTVECTOR(p,prio)                ((p)->firstVector[PRIO2LISTPART(VECTOR_LIST,prio)])
#define LISTPART_FIRSTVECTOR(p,part)    ((p)->firstVector[part])
#define FIRSTVECTOR(p)                                  (((p)->firstVector[PRIO2LISTPART(VECTOR_LIST,\
                                                                                         PrioBorder)]!=NULL) ?\
                                                         (p)->firstVector[PRIO2LISTPART(VECTOR_LIST,PrioBorder)] :\
                                                         (p)->firstVector[PRIO2LISTPART(VECTOR_LIST,PrioMaster)])
#define SFIRSTVECTOR(p)                                  (p)->firstVector[PRIO2LISTPART(VECTOR_LIST,PrioMaster)]

#define PLASTVECTOR(p)                                  LASTVECTOR(p)
#define PRIO_LASTVECTOR(p,prio)                 ((p)->lastVector[PRIO2LISTPART(VECTOR_LIST,prio)])
#define LISTPART_LASTVECTOR(p,part)     ((p)->lastVector[part])
#define LASTVECTOR(p)                                   ((p)->lastVector[PRIO2LISTPART(VECTOR_LIST,PrioMaster)])
#else
#define FIRSTVECTOR(p)          ((p)->firstVector[0])
#define PFIRSTVECTOR(p)         FIRSTVECTOR(p)
#define SFIRSTVECTOR(p)         FIRSTVECTOR(p)
#define LASTVECTOR(p)           ((p)->lastVector[0])
#define PLASTVECTOR(p)          LASTVECTOR(p)
#endif

#define UPGRID(p)                       ((p)->finer)
#define DOWNGRID(p)             ((p)->coarser)
#define MYMG(p)                         ((p)->mg)
#define NV(p)                           ((p)->nVert[0])
#define NN(p)                           ((p)->nNode[0])
#define NT(p)                           ((p)->nElem[0])
#define NVEC(p)                         ((p)->nVector[0])
#ifdef ModelP
#define NV_PRIO(p,prio)                         ((p)->nVert[prio])
#define NN_PRIO(p,prio)                         ((p)->nNode[prio])
#define NT_PRIO(p,prio)                         ((p)->nElem[prio])
#define NVEC_PRIO(p,prio)                       ((p)->nVector[prio])
#endif
#define NE(p)                           ((p)->nEdge)
#define NS(p)                           ((p)->nSide)
#ifdef UG_DIM_3
#define VEC_DEF_IN_OBJ_OF_GRID(p,tp)     (true)   // 3d grids have side vectors
#else
#define VEC_DEF_IN_OBJ_OF_GRID(p,tp)     (false)  // 2d grids have no vectors at all
#endif
#define NIMAT(p)                        ((p)->nIMat)

#define GRID_ATTR(g) ((unsigned char) (GLEVEL(g)+32))
#define ATTR_TO_GLEVEL(i) (i-32)

inline const PPIF::PPIFContext&
grid::ppifContext() const
{
  return mg->ppifContext();
}

#ifdef ModelP
inline const DDD::DDDContext&
grid::dddContext() const
{
  return mg->dddContext();
}

inline DDD::DDDContext&
grid::dddContext()
{
  return mg->dddContext();
}
#endif

/****************************************************************************/
/*                                                                          */
/* macros for multigrids                                                    */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define MULTIGRID_STATUS_OFFSET         offsetof(struct multigrid, status)/sizeof(UINT)

#define MGSTATUS(p)                     ((p)->status)
#define RESETMGSTATUS(p)                {(p)->status=0; (p)->magic_cookie = (int)time(NULL); (p)->saved=0;}
#define MG_MAGIC_COOKIE(p)              ((p)->magic_cookie)
#define VIDCNT(p)                       ((p)->vertIdCounter)
#define NIDCNT(p)                       ((p)->nodeIdCounter)
#define EIDCNT(p)                       ((p)->elemIdCounter)
#define TOPLEVEL(p)                     ((p)->topLevel)
#define CURRENTLEVEL(p)                 ((p)->currentLevel)
#define FULLREFINELEVEL(p)              ((p)->fullrefineLevel)
#define MG_BVP(p)                               ((p)->theBVP)
#define MG_BVPD(p)                              (&((p)->theBVPD))
#define MGHEAP(p)                               ((p)->theHeap)
#define MG_NPROPERTY(p)                 ((p)->nProperty)
#define GRID_ON_LEVEL(p,i)              ((p)->grids[i])
#define MGNAME(p)                               ((p)->v.name)
#ifdef UG_DIM_3
#define VEC_DEF_IN_OBJ_OF_MG(p,tp)     (true)   // 3d grids have side vectors
#else
#define VEC_DEF_IN_OBJ_OF_MG(p,tp)     (false)  // 2d grids have no vectors at all
#endif
#define MG_SAVED(p)                             ((p)->saved)

/****************************************************************************/
/*                                                                          */
#define MG_FILENAME(p)                  ((p)->filename)
#define MG_COARSE_FIXED(p)              ((p)->CoarseGridFixed)
#define MG_MARK_KEY(p)              ((p)->MarkKey)
/* macros for formats                                                       */
/*                                                                          */
/****************************************************************************/

/** \brief Constants for USED flags of objects */
enum {MG_ELEMUSED =    1,
      MG_NODEUSED =    2,
      MG_EDGEUSED =    4,
      MG_VERTEXUSED =   8,
      MG_VECTORUSED =  16,
      MG_MATRIXUSED =  32};


/****************************************************************************/
/*                                                                          */
/* interface functions for module grid manager                              */
/*                                                                          */
/****************************************************************************/

/** \brief Return values for functions returning an INT. The usual rule is: 0 ok, >0 error */
enum {GM_OK                       = 0,
      GM_ERROR                    = 1,
      GM_FILEOPEN_ERROR           = 2,
      GM_RULE_WITH_ORIENTATION    = 3,
      GM_RULE_WITHOUT_ORIENTATION = 4,
      GM_OUT_OF_MEM               = 5,
      GM_OUT_OF_RANGE             = 6,
      GM_NOT_FOUND                = 7,
      GM_INCONSISTENCY            = 8,
      GM_COARSE_NOT_FIXED         = 9,
      GM_FATAL                    = 999};


/** @name Some constants passed as parameters */
/*@{*/
enum {GM_KEEP_BOUNDARY_NODES,
      GM_MOVE_BOUNDARY_NODES,
      GM_REFINE_TRULY_LOCAL,
      GM_COPY_ALL,
      GM_REFINE_NOT_CLOSED};

enum {GM_REFINE_PARALLEL, GM_REFINE_SEQUENTIAL};

enum {GM_REFINE_NOHEAPTEST, GM_REFINE_HEAPTEST};

enum {GM_ALL_LEVELS = 1,
      GM_CURRENT_LEVEL = 2};

/*@}*/

/* get/set current multigrid, loop through multigrids */
MULTIGRID               *MakeMGItem                             (const char *name, std::shared_ptr<PPIF::PPIFContext> context);
MULTIGRID               *GetMultigrid                           (const char *name);
MULTIGRID               *GetFirstMultigrid                      (void);
MULTIGRID               *GetNextMultigrid                       (const MULTIGRID *theMG);

/* create, saving and disposing a multigrid structure */
MULTIGRID *CreateMultiGrid (char *MultigridName, BVP theBVP,
                            const char *format,
                            INT optimizedIE, INT insertMesh,
                            std::shared_ptr<PPIF::PPIFContext> ppifContext = nullptr);
MULTIGRID *OpenMGFromDataFile(MULTIGRID *theMG, INT number, char *type,
                              char *DataFileName, NS_PREFIX MEM heapSize);
MULTIGRID       *LoadMultiGrid  (const char *MultigridName, const char *name, const char *type,
                                 BVP *theBVP, const char *format,
                                 unsigned long heapSize,INT force,INT optimizedIE, INT autosave,
                                 std::shared_ptr<PPIF::PPIFContext> ppifContext = nullptr);
INT             SaveMultiGrid (MULTIGRID *theMG, const char *name, const char *type, const char *comment, INT autosave, INT rename);
INT         DisposeGrid             (GRID *theGrid);
INT             DisposeMultiGrid                (MULTIGRID *theMG);
INT         Collapse                (MULTIGRID *theMG);

/* coarse grid manipulations */
NODE        *InsertInnerNode            (GRID *theGrid, const DOUBLE *pos);
NODE        *InsertBoundaryNode     (GRID *theGrid, BNDP *bndp);

INT             DeleteNode                              (GRID *theGrid, NODE *theNode);
ELEMENT     *InsertElement                      (GRID *theGrid, INT n, NODE **NodeList, ELEMENT **ElemList, INT *NbgSdList, INT *bnds_flag);
INT         InsertMesh              (MULTIGRID *theMG, MESH *theMesh);
INT             DeleteElement                   (MULTIGRID *theMG, ELEMENT *theElement);

/* refinement */
/** \todo !!! should be moved to rm.h [Thimo] */
INT             EstimateHere                    (const ELEMENT *theElement);
INT         MarkForRefinement       (ELEMENT *theElement, enum RefinementRule rule, INT data);
INT             GetRefinementMark               (ELEMENT *theElement, INT *rule, void *data);
INT             GetRefinementMarkType   (ELEMENT *theElement);
INT             AdaptMultiGrid                  (MULTIGRID *theMG, INT flag, INT seq, INT mgtest);
INT         SetRefineInfo           (MULTIGRID *theMG);


/* moving nodes */
#ifdef UG_DIM_3
INT                     GetSideIDFromScratch    (ELEMENT *theElement, NODE *theNode);
#endif

/* algebraic connections */
INT             DisposeConnectionsInGrid (GRID *theGrid);

/* searching */
INT          InnerBoundary          (ELEMENT *t, INT side);

/* list */
void            ListMultiGridHeader     (const INT longformat);
void            ListMultiGrid           (const MULTIGRID *theMG, const INT isCurrent, const INT longformat);
INT         MultiGridStatus             (const MULTIGRID *theMG, INT gridflag, INT greenflag, INT lbflag, INT verbose);
void            ListGrids                               (const MULTIGRID *theMG);
void            ListNode                                (const MULTIGRID *theMG, const NODE *theNode, INT dataopt, INT bopt, INT nbopt, INT vopt);
void            ListElement                     (const MULTIGRID *theMG, const ELEMENT *theElement, INT dataopt, INT bopt, INT nbopt, INT vopt);
void            ListVector                      (const MULTIGRID *theMG, const VECTOR *theVector, INT dataopt, INT modifiers);

/* query */
LINK            *GetLink                                (const NODE *from, const NODE *to);
EDGE            *GetSonEdge                             (const EDGE *theEdge);
INT                     GetSonEdges                             (const EDGE *theEdge, EDGE *SonEdges[MAX_SON_EDGES]);
EDGE            *GetFatherEdge                  (const EDGE *theEdge);
#ifdef UG_DIM_3
EDGE            *FatherEdge                             (NODE **SideNodes, INT ncorners, NODE **Nodes, EDGE *theEdge);
#endif
EDGE            *GetEdge                                (const NODE *from, const NODE *to);
INT             GetSons                                 (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);
#ifdef ModelP
INT             GetAllSons                              (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);
#endif
INT             VectorPosition                  (const VECTOR *theVector, FieldVector<DOUBLE,DIM>& position);

/* check */
#ifndef ModelP
INT                     CheckGrid                               (GRID *theGrid, INT checkgeom, INT checkalgebra, INT checklists);
#else
INT                     CheckGrid                               (GRID *theGrid, INT checkgeom, INT checkalgebra, INT checklists, INT checkif);
#endif
INT                     CheckLists                              (GRID *theGrid);
INT             CheckSubdomains                 (MULTIGRID *theMG);

/* multigrid user data space management (using the heaps.c block heap management) */
INT             AllocateControlEntry    (INT cw_id, INT length, INT *ce_id);
INT             FreeControlEntry                (INT ce_id);
UINT ReadCW                                     (const void *obj, INT ce);
void            WriteCW                                 (void *obj, INT ce, INT n);

/* miscellaneous */
INT             RenumberMultiGrid                                       (MULTIGRID *theMG, INT *nboe, INT *nioe, INT *nbov, INT *niov, NODE ***vid_n, INT *foid, INT *non, INT MarkKey);
INT         MGSetVectorClasses                              (MULTIGRID *theMG);
INT         SetEdgeSubdomainFromElements        (GRID *theGrid);
INT         SetSubdomainIDfromBndInfo           (MULTIGRID *theMG);
INT         FixCoarseGrid                       (MULTIGRID *theMG);
INT                     ClearMultiGridUsedFlags                         (MULTIGRID *theMG, INT FromLevel, INT ToLevel, INT mask);
void            CalculateCenterOfMass                           (ELEMENT *theElement, DOUBLE_VECTOR center_of_mass);
INT             KeyForObject                                            (KEY_OBJECT *obj);

/** \todo remove the following functions after the code will never need any debugging */
char *PrintElementInfo (ELEMENT *theElement,INT full);

END_UGDIM_NAMESPACE

#endif
