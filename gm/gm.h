// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
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

#include <unordered_map>
#include <array>
#include <numeric>

#include "ugtypes.h"
#include "heaps.h"
#include "ugenv.h"
#include "misc.h"
#include "debug.h"
#include "domain.h"
#include "pargm.h"
#include "cw.h"
#include "namespace.h"
#include "dimension.h"

/****************************************************************************/
/*                                                                          */
/* consistency of commandline defines                                       */
/*                                                                          */
/****************************************************************************/

#ifdef __TWODIM__
#ifdef Sideon
#error ****   two dimensional case cannot have side data        ****
#endif
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

#ifdef ModelP
/* This ensures that for each master node-vector all matrix-neighbors in link depth 2 are
   at leat as a copy on the same processor and all connections are copied (even for ghosts) */
/*#define __OVERLAP2__*/
#endif

/* if periodic boundaries are used */
/*  #define __PERIODIC_BOUNDARY__ */

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
/** \brief  maximal space dimension                              */
#define DIM_MAX                                 3
/** \brief  maximal dimension of boundary surface*/
#define DIM_OF_BND_MAX                  2
/** \brief  maximum depth of triangulation               */
#define MAXLEVEL                                32
/** \brief  use 5 bits for object identification */
#define MAXOBJECTS                              32
/*@}*/

/** @name Some size macros for allocation purposes */
/*@{*/
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
/** \brief an edge has always two corners.. */
#define MAX_CORNERS_OF_EDGE             2
/** \brief two sides have one edge in common */
#define MAX_SIDES_OF_EDGE               2
/** \brief max number of sons of an element */
enum {MAX_SONS = 30};
/** \brief max number of nodes on elem side */
#define MAX_SIDE_NODES                  9
/** \brief max number of son edges of edge  */
#define MAX_SON_EDGES                   2
/** \brief max #fine sides touching a coarse*/
#define MAX_SIDES_TOUCHING              10

/** \todo Please doc me! */
#define MAX_ELEM_VECTORS                (MAX_CORNERS_OF_ELEM+MAX_EDGES_OF_ELEM+1+MAX_SIDES_OF_ELEM)
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
#define MAXVOBJECTS                                             4
/** \brief max number of abstract vector types                  */
#define MAXVECTORS                                              4
#if (MAXVECTORS<MAXVOBJECTS)
        #error *** MAXVECTORS must not be smaller than MAXVOBJECTS ***
#endif

/** \brief to indicate type not defined                                 */
#define NOVTYPE                                                 -1
/** \brief max number of geometric domain parts                 */
#define MAXDOMPARTS                                             4

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
enum VectorType {NODEVEC,   /**< Vector associated to a node */
                 EDGEVEC,   /**< Vector associated to an edge */
                 ELEMVEC,   /**< Vector associated to an element */
                 SIDEVEC    /**< Vector associated to an element side */
};

/** @name Some constants for abstract vector type names */
/*@{*/
/** \todo Please doc me! */
#define FROM_VTNAME                                             '0'
/** \todo Please doc me! */
#define TO_VTNAME                                               'z'
/** \todo Please doc me! */
#define MAXVTNAMES                                              (1+TO_VTNAME-FROM_VTNAME)
/*@}*/

/****************************************************************************/
/*                                                                          */
/* various defines                                                          */
/*                                                                          */
/****************************************************************************/

/**      0 = OK as usual */
/** @name result codes of user supplied functions*/
/*@{*/
/** \brief coordinate out of range                              */
#define OUT_OF_RANGE                    1
/** \brief configProblem could not init problem */
#define CANNOT_INIT_PROBLEM     1

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
#ifdef __TWODIM__
 BLUE = 3,   // For quadrilaterals
#endif
 COARSE = 4,
#ifdef __TWODIM__
 // The BISECTION* rules are all triangle rules
 BISECTION_1 = 5,
 BISECTION_2_Q = 6,
 BISECTION_2_T1 = 7,
 BISECTION_2_T2 = 8,
 BISECTION_3 = 9
#endif
#ifdef __THREEDIM__

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

/** @name Macros for the multigrid user data space management */
/*@{*/
#define OFFSET_IN_MGUD(id)              (GetMGUDBlockDescriptor(id)->offset)
#define IS_MGUDBLOCK_DEF(id)    (GetMGUDBlockDescriptor(id)!=NULL)
/*@}*/

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
typedef DOUBLE DOUBLE_VECTOR[DIM];
typedef DOUBLE DOUBLE_VECTOR_2D[2];
typedef DOUBLE DOUBLE_VECTOR_3D[3];
/*@}*/

/*----------- typedef for functions ----------------------------------------*/

/** \brief Print user data --> string
 * @param pointer to user data
 * @param Prefix for each line
 * @param resulting string
 */
typedef INT (*ConversionProcPtr)(void *, const char *, char *);

/** \brief Tagged print user data --> string
 *
 */
typedef INT (*TaggedConversionProcPtr)(INT,            /**< Tag for data identification */
                                       void *,             /**< Pointer to user data */
                                       const char *,      /**< Prefix for each line */
                                       char *              /**< Resulting string */
                                       );


/*----------- definition of structs ----------------------------------------*/

/* struct documentation is in gm.doc */
struct format {

  /** \brief fields for environment variable */
  NS_PREFIX ENVDIR d;

  /* variables of format */
  /** \brief size of vertex user data struc. in bytes*/
  INT sVertex;

  /** \brief size of mg user data structure in bytes     */
  INT sMultiGrid;

  /** \brief number of doubles in vectors                    */
  INT VectorSizes[MAXVECTORS];

  /** \brief a single char for abstract type name    */
  char VTypeNames[MAXVECTORS];

  /** \brief number of doubles in matrices                */
  INT MatrixSizes[MAXCONNECTIONS];

  /** \brief depth of connection for matrices */
  INT ConnectionDepth[MAXCONNECTIONS];

  /** \todo Please doc me! */
  INT nodeelementlist;
  /** \todo Please doc me! */
  INT nodedata;

  /** \brief print user data to string */
  ConversionProcPtr PrintVertex;
  /** \todo Please doc me! */
  ConversionProcPtr PrintGrid;
  /** \todo Please doc me! */
  ConversionProcPtr PrintMultigrid;

  /** \todo Please doc me!
   *
   * tag indicates VTYPE
   */
  TaggedConversionProcPtr PrintVector;

  /** \todo Please doc me!
   *
   * tag indicates MTP
   */
  TaggedConversionProcPtr PrintMatrix;

  /* table connecting parts, objects and types */

  /** \brief (part,obj) --> vtype, -1 if not defined */
  INT po2t[MAXDOMPARTS][MAXVOBJECTS];

  /* derived components */
  /** \brief maximal connection depth                     */
  INT MaxConnectionDepth;
  /** \brief geometrical depth corresponding                      */
  INT NeighborhoodDepth;

  /* algebraic con with depth 1                           */
  /* both derived from ConnectionDepth            */
  /** \brief type --> part, bitwise, not unique           */
  INT t2p[MAXVECTORS];
  /** \brief type --> object, bitwise, not unique         */
  INT t2o[MAXVECTORS];

  /* both derived from po2t                                       */
  /** \brief type --> type name                                           */
  char t2n[MAXVECTORS];

  /** \brief type name --> type                                           */
  INT n2t[MAXVTNAMES];

  /** \brief 0 if vector not needed for geom object       */
  INT OTypeUsed[MAXVOBJECTS];

  /** \brief largest part used                                            */
  INT MaxPart;
  /* both derived from po2t                                       */

  /** \brief largest type used */
  INT MaxType;
  /* derived from VectorSizes                                     */
};

typedef struct {

  /** \brief Abstract type is described here
   *
   * description only complete with po2t info
   */
  int tp;

  /** \brief A single char as name of abstract type */
  char name;

  /* \brief Data size in bytes */
  int size;

} VectorDescriptor ;

typedef struct {

  /** \brief This connection goes from position 'from'      */
  int from;

  /** \brief to position 'to' */
  int to;

  /** \brief 1 if diagonal, 0 if not */
  int diag;

  /** \brief Number of bytes per connection */
  int size;

  /** \brief Size of interpolation matrices */
  int isize;

  /** \brief Connect with depth in dual graph */
  int depth;
} MatrixDescriptor ;

/****************************************************************************/
/*                                                                          */
/* matrix/vector/blockvector data structure                                 */
/*                                                                          */
/****************************************************************************/

/* Struct documentation in gm.doc */
struct vector {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief associated object */
  union geom_object *object;

#ifdef ModelP
  /** \todo Please doc me! */
  DDD_HEADER ddd;
#endif

  /** \brief double linked list of vectors                */
  struct vector *pred,*succ;

  /** \brief ordering of unknowns                                 */
  UINT index;

#ifndef ModelP   // Dune uses ddd.gid for ids in parallel
  /** \brief A unique and persistent, but not necessarily consecutive index

      Used to implement face ids for Dune.
   */
  INT id;
#endif

  /** \brief used bitwise to skip unknowns                */
  UINT skip;

  /** \brief implements matrix                                    */
  struct matrix *start;

  /** \brief User data */
  DOUBLE value[1];
};
typedef struct vector VECTOR;


/* Struct documentation in gm.doc */
struct matrix {

  /** \brief object identification, various flags */
  UINT control;

#ifdef __XXL_MSIZE__
  /** \brief for people needing large matrices    */
  UINT xxl_msize;
#endif

  /** \brief row list */
  struct matrix *next;

  /** \brief destination vector */
  struct vector *vect;

  /** \brief User data */
  DOUBLE value[1];
};

typedef struct matrix MATRIX;
typedef struct matrix CONNECTION;

/****************************************************************************/
/*                                                                          */
/* unstructured grid data structures                                        */
/*                                                                          */
/****************************************************************************/

/*----------- typedef for functions ----------------------------------------*/


/*----------- definition of structs ----------------------------------------*/

/** \brief Inner vertex data structure */
struct ivertex {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Vertex position                                              */
  DOUBLE x[DIM];

  /** \brief Local coordinates in father element  */
  DOUBLE xi[DIM];

  /* When UG is used as part of the DUNE numerics system we need
     a few more indices per node */

  /** \brief An index hat is unique and consecutive per level.
      Controlled by DUNE */
  int leafIndex;

#ifdef ModelP
  /** \todo Please doc me! */
  DDD_HEADER ddd;
#endif

  /* pointers */
  /** \brief Doubly linked list of vertices */
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
};

/** \brief Boundary vertex data structure */
struct bvertex {

  /* variables */
  /** \brief Object identification, various flags */
  UINT control;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Vertex position */
  DOUBLE x[DIM];

  /** \brief Local coordinates in father element  */
  DOUBLE xi[DIM];

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

  /** \brief Pointer to boundary point decriptor */
  BNDP *bndp;
};

/** \brief Only used to define pointer to vertex */
union vertex {
  struct ivertex iv;
  struct bvertex bv;
};

/** \brief A simply linked list of elements */
struct elementlist {
  union element *el;
  struct elementlist *next;
};

/** \brief Level-dependent part of a vertex */
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

  /** \brief List of links                                                */
  struct link *start;

  /** \brief Node or edge on coarser level (NULL if none) */
  union geom_object *father;

  /** \brief Node on finer level (NULL if none)   */
  struct node *son;

  /** \brief Corresponding vertex structure               */
  union vertex *myvertex;

  /** \brief Associated vector
   *
   * WARNING: the allocation of the vector pointer depends on the format */
  VECTOR *vector;

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

/** \todo Please doc me! */
struct link {

  /** \brief object identification, various flags */
  UINT control;

  /** \brief ptr to next link                                     */
  struct link *next;

  /** \brief ptr to neighbor node                                 */
  struct node *nbnode;

  /** \brief ptr to neighboring elem                              */
#if defined(__TWODIM__)
  union element *elem;
#endif

};

/** \brief Undirected edge of the grid graph    */
struct edge {

  /* variables */
  /* two links */
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

  /** \brief Pointer to mid node on next finer grid */
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

  /** \brief additional flags for elements        */
  UINT flag;

  /** \brief to store NodeOrder for hexahedrons   */
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[4] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1],ptr[2]
     Use the macros to find the correct address!                              */
  VECTOR *vector;

  /** \brief Only on the boundary, NULL if interior side */
  BNDS *bnds[3];

  /* \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format        */
  void *data;
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[5] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1], ..
     Use the macros to find the correct address!
   */
  VECTOR *vector;

  /** \brief only on bnd, NULL if interior side   */
  BNDS *bnds[4];

  /** \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format        */
  void *data;
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[9] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1], ..
     Use the macros to find the correct address! */
  VECTOR *vector;

  /** \brief Associated vector for sides */
  VECTOR *sidevector[4];

  /** \brief The boundary segments, NULL if interior side */
  BNDS *bnds[4];

  /** \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format */
  void *data;
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[11] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1], ..
     Use the macros to find the correct address! */
  VECTOR *vector;

  /** \brief Associated vector for sides */
  VECTOR *sidevector[5];

  /** \brief The boundary segments, NULL if interior side */
  BNDS *bnds[5];

  /** \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format        */
  void *data;
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[11] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1], ...
     Use the macros to find the correct address!
   */
  VECTOR *vector;

  /** \brief Associated vectors for sides */
  VECTOR *sidevector[5];

  /** \brief Boundary segments, NULL if interior side */
  BNDS *bnds[5];

  /** \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format        */
  void *data;
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

  /** \brief Associated vector

     WARNING: the allocation of the vector pointer depends on the format
     void *ptr[13] would be possible too:
     if there are no element vectors, the sides will be ptr[0],ptr[1], ...
      Use the macros to find the correct address!
   */
  VECTOR *vector;

  /** \brief Associated vectors for sides */
  VECTOR *sidevector[6];

  /** \brief Boundary segments, NULL if interior side */
  BNDS *bnds[6];

  /** \brief Associated data pointer

     WARNING: the allocation of the data pointer depends on the format        */
  void *data;
} ;

/** \brief Objects that can hold an element */
union element {
  struct generic_element ge;
    #ifdef __TWODIM__
  struct triangle tr;
  struct quadrilateral qu;
        #endif
    #ifdef __THREEDIM__
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

/** \brief Objects that can hold a vector */
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

struct grid {

  /** \brief Object identification, various flags */
  UINT control;

  /** \brief level + 32; needed for control word check not detecting HEAPFAULT */
  INT attribut;

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

  /** \brief Number of connections on this grid level */
  INT nCon;

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
};

struct multigrid {

  /** \brief env item */
  NS_PREFIX ENVDIR v;

  /** \brief Multigrid status word */
  INT status;

  /** \brief used for identification                          */
  INT magic_cookie;

  /** \brief is bottom memory temp allocated?

     This field is really only necessary when DYNAMIC_MEMORY_ALLOCMODEL is set.
     However, as struct declarations should not depend on settings in config.h
     we leave in the field anyways.  It's just four bytes per multigrid.
   */
  INT bottomtmpmem;

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

  /** \brief depth of the element tree                    */
  INT topLevel;

  /** \brief level we are working on                              */
  INT currentLevel;

  /** \brief last level with complete surface     */
  INT fullrefineLevel;

  /** \brief bottom level for AMG                 */
  INT bottomLevel;

  /** \brief pointer to BndValProblem                             */
  BVP *theBVP;

  /** \brief description of BVP-properties                */
  BVP_DESC theBVPD;

  /** \brief pointer to format definitions                */
  struct format *theFormat;

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

  /* user data */
  /** \brief general user data space                              */
  void *GenData;

  /** \brief user heap                                                    */
  NS_PREFIX HEAP *UserHeap;

  /* i/o handling */
  /** \brief 1 if multigrid saved                                 */
  INT saved;

  /** \brief filename if saved                                    */
  char filename[NS_PREFIX NAMESIZE];

  /** \brief coarse grid complete                                 */
  INT CoarseGridFixed;

  /** \brief coarse grid MarkKey for SIMPLE_HEAP Mark/Release     */
  INT MarkKey;
};

/****************************************************************************/
/*                                                                                                                                                      */
/*                                      typedef for structs                                                                     */
/*                                                                                                                                                      */
/****************************************************************************/

/* geometrical part */
typedef struct format FORMAT;

typedef union  vertex VERTEX;
typedef struct elementlist ELEMENTLIST;
typedef struct node NODE;
typedef union  element ELEMENT;
typedef struct link LINK;
typedef struct edge EDGE;
typedef union  geom_object GEOM_OBJECT;
typedef struct grid GRID;
typedef struct multigrid MULTIGRID;
typedef union object_with_key KEY_OBJECT;

/****************************************************************************/
/*                                                                                                                                                      */
/*                                      structs for evaluation functions                                                */
/*                                                                                                                                                      */
/****************************************************************************/

/*----------- typedef for functions ----------------------------------------*/

typedef INT (*PreprocessingProcPtr)(const char *, MULTIGRID *);
typedef DOUBLE (*ElementEvalProcPtr)(const ELEMENT *,const DOUBLE **,DOUBLE *);
typedef void (*ElementVectorProcPtr)(const ELEMENT *,const DOUBLE **,DOUBLE *,DOUBLE *);
typedef DOUBLE (*MatrixEvalProcPtr)(const MATRIX *);

/*----------- definition of structs ----------------------------------------*/

struct elementvalues {

  /** \brief Fields for environment list variable */
  NS_PREFIX ENVVAR v;

  /** \brief Prepare eval values */
  PreprocessingProcPtr PreprocessProc;

  /** \brief Pointer to corresponding function */
  ElementEvalProcPtr EvalProc;
};

struct elementvector {

  /** \brief Fields for environment list variable */
  NS_PREFIX ENVVAR v;

  /** \brief Prepare eval values */
  PreprocessingProcPtr PreprocessProc;

  /** \brief Pointer to corresponding function */
  ElementVectorProcPtr EvalProc;

  /** \brief Dimension of result vector */
  int dimension;

};

typedef struct elementvalues EVALUES ;
typedef struct elementvector EVECTOR ;

/****************************************************************************/
/*                                                                                                                                                      */
/* algebraic dependency for vector ordering                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

typedef INT (*DependencyProcPtr)(GRID *, const char *);

struct AlgebraicDependency {

  /* fields for enironment list variable */
  NS_PREFIX ENVVAR v;

  DependencyProcPtr DependencyProc;             /* pointer to dependency function                       */
};

typedef struct AlgebraicDependency ALG_DEP;

/****************************************************************************/
/*                                                                          */
/* periodic boundary info                                                   */
/*                                                                          */
/****************************************************************************/

#ifdef __PERIODIC_BOUNDARY__

/* node counter for periodic vector */
#define PVCOUNT(p)              VINDEX(p)
#define SETPVCOUNT(p,n) (VINDEX(p)=(n))

/* maximal count of periodic objects */
#define MAX_PERIODIC_OBJ        DIM+1

typedef INT (* PeriodicBoundaryInfoProcPtr)(
  VERTEX *vtx,                                                                  /* vertex, for which to examine boundary   */
  INT *n,                                                                               /* number of periodic boundaries of vertex */
  INT *periodic_ids,                                                            /* n ids of periodic boundaries            */
  DOUBLE_VECTOR own_coord,                                              /* own coord from vertex                   */
  DOUBLE_VECTOR *periodic_coords                                /* n coords of periodic boundaries         */
  );

INT SetPeriodicBoundaryInfoProcPtr (PeriodicBoundaryInfoProcPtr PBI);
INT GetPeriodicBoundaryInfoProcPtr (PeriodicBoundaryInfoProcPtr *PBI);

#endif


/****************************************************************************/
/*                                                                          */
/* find cut for vector ordering                                             */
/*                                                                          */
/****************************************************************************/

typedef VECTOR *(*FindCutProcPtr)(GRID *, VECTOR *, INT *);

typedef struct {

  /* fields for enironment list variable */
  NS_PREFIX ENVVAR v;

  FindCutProcPtr FindCutProc;           /* pointer to find cut function                         */

} FIND_CUT;

/****************************************************************************/
/*                                                                          */
/* dynamic management of control words                                      */
/*                                                                          */
/****************************************************************************/

/** @name status of control word */
/*@{*/
#define CW_FREE                                         0
#define CW_USED                                         1
/*@}*/


/** @name Status of control entry */
/*@{*/
#define CE_FREE                                         0
#define CE_USED                                         1
#define CE_LOCKED                                       1
/*@}*/

/** @name Initializer macros for control entry and word predefines */
/*@{*/
#define CW_INIT(used,cw,objs)                           {used, STR(cw), cw ## CW, cw ## OFFSET,objs}
#define CW_INIT_UNUSED                                          {CW_FREE,0,0,0}
#define CE_INIT(mode,cw,ce,objs)                        {mode, STR(ce), cw ## CW, ce ## CE, ce ## SHIFT, ce ## LEN, objs}
#define CE_INIT_UNUSED                                          {CE_FREE, 0, 0, 0, 0, 0, 0}
/*@}*/

/* general query macros */

/* dynamic control words */
/*#define _DEBUG_CW_*/
#if (defined _DEBUG_CW_) && \
  !(defined __COMPILE_CW__)                             /* to avoid infinite recursion during ReadCW */

/* map cw read/write to functions */
        #define CW_READ(p,ce)                                   ReadCW(p,ce)
        #define CW_READ_STATIC(p,s,t)                   ReadCW(p,s ## CE)
        #define CW_WRITE(p,ce,n)                                WriteCW(p,ce,n)
        #define CW_WRITE_STATIC(p,s,t,n)                WriteCW(p,s ## CE,n)

#else   /* _DEBUG_CW_ */

        #define ControlWord(p,ce)  (((UINT *)(p))[control_entries[ce].offset_in_object])

        #ifndef __T3E__
        #define CW_READ(p,ce)      ((ControlWord(p,ce) & control_entries[ce].mask)>>control_entries[ce].offset_in_word)
        #endif

/* very special hack */
        #ifdef __T3E__
        #define CW_READ(p,ce)      ((int)((ControlWord(p,ce) & control_entries[ce].mask)>>control_entries[ce].offset_in_word) )
        #endif

        #define CW_WRITE(p,ce,n)   ControlWord(p,ce) = (ControlWord(p,ce)&control_entries[ce].xor_mask)|(((n)<<control_entries[ce].offset_in_word)&control_entries[ce].mask)

/* static control words */
        #define StaticControlWord(p,t)            (((UINT *)(p))[t ## OFFSET])
        #define StaticControlWordMask(s)          ((POW2(s ## LEN) - 1) << s ## SHIFT)

        #ifndef __T3E__
        #define CW_READ_STATIC(p,s,t)                                                \
  ((StaticControlWord(p,t) &  StaticControlWordMask(s)) >> s ## SHIFT)
        #endif

/* very special hack */
        #ifdef __T3E__
        #define CW_READ_STATIC(p,s,t)                                                \
  ((int)     ((StaticControlWord(p,t) &  StaticControlWordMask(s)) >> s ## SHIFT))
        #endif

        #define CW_WRITE_STATIC(p,s,t,n)                                             \
  StaticControlWord(p,t) =                                           \
    (StaticControlWord(p,t) &  (~StaticControlWordMask(s))) |          \
    (((n) << s ## SHIFT) & StaticControlWordMask(s))

#endif  /* _DEBUG_CW_ */

/** \brief Enumeration list of all control words of gm.h */
enum GM_CW {
  VECTOR_CW,
  MATRIX_CW,
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
  VTYPE_CE,
  VOTYPE_CE,
  VPART_CE,
  VCOUNT_CE,
  VECTORSIDE_CE,
  VCLASS_CE,
  VDATATYPE_CE,
  VNCLASS_CE,
  VNEW_CE,
  VCCUT_CE,
  VCCOARSE_CE,
  NEW_DEFECT_CE,
  VACTIVE_CE,
  FINE_GRID_DOF_CE,
  MOFFSET_CE,
  MROOTTYPE_CE,
  MDESTTYPE_CE,
  MDIAG_CE,
  MSIZE_CE,
  MNEW_CE,
  CEXTRA_CE,
  MDOWN_CE,
  MUP_CE,
  MLOWER_CE,
  MUPPER_CE,
  MACTIVE_CE,
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
  XFERMATX_CE,
#endif

  GM_N_CE
};

enum LV_MODIFIERS {

  LV_SKIP                 = (1<<0),                     /* print skip flags in vector                   */
  LV_VO_INFO              = (1<<1),                     /* vector object related info                   */
  LV_POS                  = (1<<2)                      /* position vector  */
};

enum LV_ID_TYPES {
  LV_ID,
  LV_GID,
  LV_KEY
};

#define LV_MOD_DEFAULT          (LV_POS | LV_VO_INFO)

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
/* VCFLAG        |3     |*| | flag for general use                          */
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
/* VACTIVE   |24        |*| | 1 if vector is active inside a smoother                   */
/* VCCUT         |26    |*| |                                                                                                   */
/* VTYPE         |27-28 |*| | abstract vector type                                                              */
/* VPART         |29-30 |*| | domain part                                                                               */
/* VCCOARSE  |31    |*| | indicate algebraic part of VECTOR-MATRIX graph        */
/*                                                                                                                                                      */
/* matrices:                                                                                                                            */
/* MOFFSET       |0     | |*| 0 if first matrix in connection else 1                    */
/* MROOTTYPE |1 - 2 | |*| VTYPE of root vector                                                          */
/* MDESTTYPE |3 - 4 | |*| VTYPE of destination vector                                           */
/* MDIAG         |5     | |*| 1 if diagonal matrix element                                              */
/* MNEW          |6     | |*| ???                                                       */
/* CEXTRA        |7     | |*| 1 if is extra connection                                                  */
/* MDOWN         |8     | |*| ???                                                       */
/* MUP           |9     | |*| ???                                                       */
/* MLOWER        |10    | |*| 1 if matrix belongs to lower triangular part      */
/* MUPPER        |11    | |*| 1 if matrix belongs to upper triangular part      */
/* UG_MSIZE      |12-25 | |*| size of the matrix in bytes                                               */
/* MUSED         |12    | |*| general purpose flag                                                              */
/* MNEW          |28    | |*| 1 if matrix/connection is new                                     */
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
#ifdef __TWODIM__
#define COORDINATE_TO_KEY(coord,dummy_int_ptr)  ((INT)(SIGNIFICANT_DIGITS((SIGNIFICANT_DIGITS((coord)[0],(dummy_int_ptr))*1.246509423749342 + \
                                                                           SIGNIFICANT_DIGITS((coord)[1],(dummy_int_ptr))*PI)\
                                                                          , (dummy_int_ptr))))
#endif

#ifdef __THREEDIM__
#define COORDINATE_TO_KEY(coord,dummy_int_ptr)  ((INT)(SIGNIFICANT_DIGITS((SIGNIFICANT_DIGITS((coord)[0],(dummy_int_ptr))*1.246509423749342 + \
                                                                           SIGNIFICANT_DIGITS((coord)[1],(dummy_int_ptr))*PI + \
                                                                           SIGNIFICANT_DIGITS((coord)[2],(dummy_int_ptr))*0.76453456834568356936598)\
                                                                          , (dummy_int_ptr))))
#endif

/****************************************************************************/
/*                                                                          */
/* macros for VECTORs                                                       */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define VECTOR_OFFSET                           0

/* predefined control word entries */
#define VOTYPE_SHIFT                            0
#define VOTYPE_LEN                                      2
#define VOTYPE(p)                                       CW_READ_STATIC(p,VOTYPE_,VECTOR_)
#define SETVOTYPE(p,n)                          CW_WRITE_STATIC(p,VOTYPE_,VECTOR_,n)
#if (MAXVOBJECTS > POW2(VOTYPE_LEN))
        #error  *** VOTYPE_LEN too small ***
#endif

#define VTYPE_SHIFT                             2
#define VTYPE_LEN                                       2
#define VTYPE(p)                                (enum VectorType)CW_READ_STATIC(p,VTYPE_,VECTOR_)
#define SETVTYPE(p,n)                           CW_WRITE_STATIC(p,VTYPE_,VECTOR_,n)
#if (MAXVTYPES > POW2(VTYPE_LEN))
        #error  *** VTYPE_LEN too small ***
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

#define VCCOARSE_SHIFT                          19
#define VCCOARSE_LEN                            1
#define VCCOARSE(p)                                     CW_READ_STATIC(p,VCCOARSE_,VECTOR_)
#define SETVCCOARSE(p,n)                        CW_WRITE_STATIC(p,VCCOARSE_,VECTOR_,n)

#define FINE_GRID_DOF_SHIFT             20
#define FINE_GRID_DOF_LEN                       1
#define FINE_GRID_DOF(p)                        CW_READ_STATIC(p,FINE_GRID_DOF_,VECTOR_)
#define SETFINE_GRID_DOF(p,n)           CW_WRITE_STATIC(p,FINE_GRID_DOF_,VECTOR_,n)

#define NEW_DEFECT_SHIFT                        21
#define NEW_DEFECT_LEN                          1
#define NEW_DEFECT(p)                           CW_READ_STATIC(p,NEW_DEFECT_,VECTOR_)
#define SETNEW_DEFECT(p,n)                      CW_WRITE_STATIC(p,NEW_DEFECT_,VECTOR_,n)

#ifdef ModelP
#define XFERVECTOR_SHIFT                        20
#define XFERVECTOR_LEN                          2
#define XFERVECTOR(p)                           CW_READ(p,XFERVECTOR_CE)
#define SETXFERVECTOR(p,n)                      CW_WRITE(p,XFERVECTOR_CE,n)
#endif /* ModelP */

#define VPART_SHIFT                             22
#define VPART_LEN                                       2
#define VPART(p)                                        CW_READ_STATIC(p,VPART_,VECTOR_)
#define SETVPART(p,n)                           CW_WRITE_STATIC(p,VPART_,VECTOR_,n)
#if (MAXDOMPARTS > POW2(VPART_LEN))
        #error  *** VPART_LEN too small ***
#endif

#define VACTIVE_SHIFT                       24
#define VACTIVE_LEN                                 1
#define VACTIVE(p)                                  CW_READ_STATIC(p,VACTIVE_,VECTOR_)
#define SETVACTIVE(p,n)                     CW_WRITE_STATIC(p,VACTIVE_,VECTOR_,n)

#define VCFLAG(p)                                       THEFLAG(p)
#define SETVCFLAG(p,n)                          SETTHEFLAG(p,n)

#define VCUSED(p)                                       USED(p)
#define SETVCUSED(p,n)                          SETUSED(p,n)

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
#define V_IN_DATATYPE(v,dt)                     (VDATATYPE(v) & (dt))
#define VSKIPME(v,n)                            ((((v)->skip)>>n) & 1)
#define VVECSKIP(v,n)                           ((((v)->skip)>>n) & 15)
#define VFULLSKIP(v,n)                          (VVECSKIP(v,n)==15)
#define SETVSKIPME(v,n)                         (((v)->skip=n))
#define VECSKIP(v)                                      ((v)->skip)
#define VECSKIPBIT(v,n)                         (((v)->skip) & (1<<n))
#define SETVECSKIPBIT(v,n)                      (v)->skip = ((v)->skip & (~(1<<n))) | (1<<n)
#define VSTART(v)                                       ((v)->start)
#define VVALUE(v,n)                             ((v)->value[n])
#define VVALUEPTR(v,n)                          (&((v)->value[n]))
#define VMYNODE(v)                                      ((NODE*)((v)->object))
#define VMYEDGE(v)                                      ((EDGE*)((v)->object))
#define VMYELEMENT(v)                           ((ELEMENT*)((v)->object))
#define VUP(p)                                          LoWrd(VINDEX(p))
#define SETVUP(p,n)                             SetLoWrd(VINDEX(p),n)
#define VDOWN(p)                                        HiWrd(VINDEX(p))
#define SETVDOWN(p,n)                           SetHiWrd(VINDEX(p),n)

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for MATRIXs                                                                                                           */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset */
#define MATRIX_OFFSET                           0

#define MOFFSET_SHIFT                           0
#define MOFFSET_LEN                             1
#define MOFFSET(p)                                      CW_READ_STATIC(p,MOFFSET_,MATRIX_)
#define SETMOFFSET(p,n)                         CW_WRITE_STATIC(p,MOFFSET_,MATRIX_,n)

#define MROOTTYPE_SHIFT                         1
#define MROOTTYPE_LEN                           2
#define MROOTTYPE(p)                            CW_READ_STATIC(p,MROOTTYPE_,MATRIX_)
#define SETMROOTTYPE(p,n)                       CW_WRITE_STATIC(p,MROOTTYPE_,MATRIX_,n)
#if (MAXVTYPES > POW2(MROOTTYPE_LEN))
#error  *** MROOTTYPE_LEN too small ***
#endif

#define MDESTTYPE_SHIFT                         3
#define MDESTTYPE_LEN                           2
#define MDESTTYPE(p)                            CW_READ_STATIC(p,MDESTTYPE_,MATRIX_)
#define SETMDESTTYPE(p,n)                       CW_WRITE_STATIC(p,MDESTTYPE_,MATRIX_,n)
#if (MAXVTYPES > POW2(MDESTTYPE_LEN))
        #error  *** MDESTTYPE_LEN too small ***
#endif

#define MDIAG_SHIFT                             5
#define MDIAG_LEN                                       1
#define MDIAG(p)                                        CW_READ_STATIC(p,MDIAG_,MATRIX_)
#define SETMDIAG(p,n)                           CW_WRITE_STATIC(p,MDIAG_,MATRIX_,n)

#define MNEW_SHIFT                                      6
#define MNEW_LEN                                        1
#define MNEW(p)                                         CW_READ_STATIC(p,MNEW_,MATRIX_)
#define SETMNEW(p,n)                            CW_WRITE_STATIC(p,MNEW_,MATRIX_,n)

#define CEXTRA_SHIFT                            7
#define CEXTRA_LEN                                      1
#define CEXTRA(p)                                       CW_READ_STATIC(p,CEXTRA_,MATRIX_)
#define SETCEXTRA(p,n)                          CW_WRITE_STATIC(p,CEXTRA_,MATRIX_,n)

#define MDOWN_SHIFT                             8
#define MDOWN_LEN                                       1
#define MDOWN(p)                                        CW_READ_STATIC(p,MDOWN_,MATRIX_)
#define SETMDOWN(p,n)                           CW_WRITE_STATIC(p,MDOWN_,MATRIX_,n)

#define MUP_SHIFT                                       9
#define MUP_LEN                                         1
#define MUP(p)                                          CW_READ_STATIC(p,MUP_,MATRIX_)
#define SETMUP(p,n)                             CW_WRITE_STATIC(p,MUP_,MATRIX_,n)

#define MLOWER_SHIFT                            10
#define MLOWER_LEN                                      1
#define MLOWER(p)                                       CW_READ_STATIC(p,MLOWER_,MATRIX_)
#define SETMLOWER(p,n)                          CW_WRITE_STATIC(p,MLOWER_,MATRIX_,n)

#define MUPPER_SHIFT                            11
#define MUPPER_LEN                                      1
#define MUPPER(p)                                       CW_READ_STATIC(p,MUPPER_,MATRIX_)
#define SETMUPPER(p,n)                          CW_WRITE_STATIC(p,MUPPER_,MATRIX_,n)

#define MACTIVE_SHIFT                           12
#define MACTIVE_LEN                             1
#define MACTIVE(p)                                      CW_READ_STATIC(p,MACTIVE_,MATRIX_)
#define SETMACTIVE(p,n)                         CW_WRITE_STATIC(p,MACTIVE_,MATRIX_,n)

#define MSIZE_SHIFT                             13
#define MSIZE_LEN                                       12
#ifndef __XXL_MSIZE__
#define MSIZEMAX                                        (POW2(MSIZE_LEN)-1)
#define UG_MSIZE(p)                                        (CW_READ(p,MSIZE_CE)+sizeof(MATRIX)-sizeof(DOUBLE))
#define SETMSIZE(p,n)                           CW_WRITE(p,MSIZE_CE,(n-sizeof(MATRIX)+sizeof(DOUBLE)))
#else
#define MSIZEMAX                                        10000000
#define UG_MSIZE(p)                                        ((p)->xxl_msize)
#define SETMSIZE(p,n)                           (p)->xxl_msize = (n)
#endif

#define MTYPE(p)                                        (MDIAG(p) ? (MAXMATRICES+MROOTTYPE(p)) : (MROOTTYPE(p)*MAXVECTORS+MDESTTYPE(p)))

#define MUSED(p)                                        USED(p)
#define SETMUSED(p,n)               SETUSED(p,n)

#ifdef ModelP
#define XFERMATX_SHIFT                          25
#define XFERMATX_LEN                            2
#define XFERMATX(p)                             CW_READ(p,XFERMATX_CE)
#define SETXFERMATX(p,n)                        CW_WRITE(p,XFERMATX_CE,n)
#endif

#define MINC(m)                                         ((MATRIX*)(((char *)(m))+UG_MSIZE(m)))
#define MDEC(m)                                         ((MATRIX*)(((char *)(m))-UG_MSIZE(m)))
#define MNEXT(m)                                        ((m)->next)
#define MDEST(m)                                        ((m)->vect)
#define MADJ(m)                                         ((MDIAG(m)) ? (m) : ((MOFFSET(m)) ? (MDEC(m)) : (MINC(m))))
#define MROOT(m)                                        MDEST(MADJ(m))
#define MMYCON(m)                                       ((MOFFSET(m)) ? (MDEC(m)) : (m))
#define MVALUE(m,n)                             ((m)->value[n])
#define MVALUEPTR(m,n)                          (&((m)->value[n]))
#define MDESTINDEX(m)                           ((m)->vect->index)
#define MSTRONG(p)                                      (MDOWN(p) && MUP(p))

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for CONNECTIONs                                                                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#define CMATRIX0(m)                             (m)
#define CMATRIX1(m)                             ((MDIAG(m)) ? (NULL) : (MINC(m)))
#define SETCUSED(c,n)                           {SETMUSED(CMATRIX0(c),n); SETMUSED(MADJ(CMATRIX0(c)),n);}

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
/* ONNBSIDE      |6 - 8 |*| | | | | |no. of side in the neigbor of the father   */
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
  MAOBJ,                            /*!< Matrix object                                        */

  NPREDEFOBJ,                       /*!< Number of predefined objects             */

  NOOBJ = -1                        /*!< No object */
};
#define LIOBJ           EDOBJ           /* link and edge are identified                 */
#define COOBJ           MAOBJ           /* connection and matrix are identified         */

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
#define VERTEX_OFFSET                           0

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
#define XC(p)                   ((p)->iv.x[0])
#define YC(p)                   ((p)->iv.x[1])
#define ZC(p)                   ((p)->iv.x[2])
#define LCVECT(p)               ((p)->iv.xi)
#define XI(p)                   ((p)->iv.xi[0])
#define ETA(p)                  ((p)->iv.xi[1])
#define NU(p)                   ((p)->iv.xi[2])
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
#define NODE_OFFSET                                     0

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

#if defined ModelP && defined __OVERLAP2__
#define NO_DELETE_OVERLAP2_LEN                 1
#define NO_DELETE_OVERLAP2(p)                  CW_READ(p,ce_NO_DELETE_OVERLAP2)
#define SETNO_DELETE_OVERLAP2(p,n)             CW_WRITE(p,ce_NO_DELETE_OVERLAP2,n)
#endif

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
#define NDATA(p)                        ((p)->data)
#define NVECTOR(p)                      ((p)->vector)

#define NODE_ELEMENT_LIST(p)    ((ELEMENTLIST *)(p)->data)
#define ELEMENT_PTR(p)                  ((p)->el)

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for links                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

/* CAUTION: the controlword of LINK0 and its edge are identical (AVOID overlapping of flags) */

/* control word offset */
#define LINK_OFFSET                             0

#define LOFFSET_SHIFT                           0
#define LOFFSET_LEN                             1
#define LOFFSET(p)                                      CW_READ(p,LOFFSET_CE)
#define SETLOFFSET(p,n)                         CW_WRITE(p,LOFFSET_CE,n)

#define NBNODE(p)                                       ((p)->nbnode)
#define NEXT(p)                                         ((p)->next)
#define LDATA(p)                                        ((p)->matelem)
#define MATELEM(p)                                      ((p)->matelem)  /* can be used for node and link */

#define MYEDGE(p)                                       ((EDGE *)((p)-LOFFSET(p)))
#define REVERSE(p)                                      ((p)+(1-LOFFSET(p)*2))

#if defined(__TWODIM__)
#define LELEM(p)                                        ((p)->elem)
#define SET_LELEM(p,e)                                  ((p)->elem = (e))
#endif

/****************************************************************************/
/*                                                                                                                                                      */
/* macros for edges                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/

/* control word offset */
#define EDGE_OFFSET                             0

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
#define ELEMENT_OFFSET                                  0
#define FLAG_OFFSET                                             2
#define PROPERTY_OFFSET                                 3

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
typedef struct {
  INT tag;                           /**< Element type to be defined       */

  /* the following parameters determine size of refs array in element */
  INT max_sons_of_elem;              /**< Max number of sons for this type */
  INT sides_of_elem;                 /**< How many sides ?                 */
  INT corners_of_elem;               /**< How many corners ?               */

  /* local geometric description of the element */
  DOUBLE_VECTOR local_corner[MAX_CORNERS_OF_ELEM];                /**< Local coordinates of the corners of the element */

  /* more size parameters */
  INT edges_of_elem;                 /**< How many edges ?         */
  INT edges_of_side[MAX_SIDES_OF_ELEM];     /**< Number of edges for each side        */
  INT corners_of_side[MAX_SIDES_OF_ELEM];   /**< Number of corners for each side  */
  INT corners_of_edge;                                      /**< Is always 2 !         */

  /* index computations */
  /* Within each element sides, edges, corners are numbered in some way.      */
  /* Within each side the edges and corners are numbered, within the edge the */
  /* corners are numbered. The following arrays map the local numbers within  */
  /* the side or edge to the numbering within the element.                                        */
  INT edge_of_side[MAX_SIDES_OF_ELEM][MAX_EDGES_OF_SIDE];
  INT corner_of_side[MAX_SIDES_OF_ELEM][MAX_CORNERS_OF_SIDE];
  INT corner_of_edge[MAX_EDGES_OF_ELEM][MAX_CORNERS_OF_EDGE];

  /* the following parameters are derived from data above */
  INT mapped_inner_objt;                                    /* tag to objt mapping for free list*/
  INT mapped_bnd_objt;                                      /* tag to objt mapping for free list*/
  INT inner_size, bnd_size;                                 /* size in bytes used for alloc     */
  INT edge_with_corners[MAX_CORNERS_OF_ELEM][MAX_CORNERS_OF_ELEM];
  INT side_with_edge[MAX_EDGES_OF_ELEM][MAX_SIDES_OF_EDGE];
  INT corner_of_side_inv[MAX_SIDES_OF_ELEM][MAX_CORNERS_OF_ELEM];
  INT edges_of_corner[MAX_CORNERS_OF_ELEM][MAX_EDGES_OF_ELEM];
  INT corner_of_oppedge[MAX_EDGES_OF_ELEM][MAX_CORNERS_OF_EDGE];
  INT corner_opp_to_side[MAX_SIDES_OF_ELEM];
  INT opposite_edge[MAX_EDGES_OF_ELEM];
  INT side_opp_to_corner[MAX_CORNERS_OF_ELEM];
  INT edge_of_corner[MAX_CORNERS_OF_ELEM][MAX_EDGES_OF_ELEM];
  INT edge_of_two_sides[MAX_SIDES_OF_ELEM][MAX_SIDES_OF_ELEM];

  /* ... the refinement rules should be placed here later */
} GENERAL_ELEMENT;

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
#define SIDES_OF_ELEM(p)                (element_descriptors[TAG(p)]->sides_of_elem)
#define EDGES_OF_ELEM(p)                (element_descriptors[TAG(p)]->edges_of_elem)
#define CORNERS_OF_ELEM(p)              (element_descriptors[TAG(p)]->corners_of_elem)
#define LOCAL_COORD_OF_ELEM(p,c)    (element_descriptors[TAG(p)]->local_corner[(c)])


#define SONS_OF_ELEM(p)                         (element_descriptors[TAG(p)]->max_sons_of_elem) /* this is the number of pointers ! */

#define EDGES_OF_SIDE(p,i)              (element_descriptors[TAG(p)]->edges_of_side[(i)])
#define CORNERS_OF_SIDE(p,i)            (element_descriptors[TAG(p)]->corners_of_side[(i)])

#define CORNERS_OF_EDGE                         2

#define EDGE_OF_SIDE(p,s,e)             (element_descriptors[TAG(p)]->edge_of_side[(s)][(e)])
#define EDGE_OF_TWO_SIDES(p,s,t)        (element_descriptors[TAG(p)]->edge_of_two_sides[(s)][(t)])
#define CORNER_OF_SIDE(p,s,c)           (element_descriptors[TAG(p)]->corner_of_side[(s)][(c)])
#define CORNER_OF_EDGE(p,e,c)           (element_descriptors[TAG(p)]->corner_of_edge[(e)][(c)])

#define EDGE_WITH_CORNERS(p,c0,c1)      (element_descriptors[TAG(p)]->edge_with_corners[(c0)][(c1)])
#define SIDE_WITH_EDGE(p,e,k)           (element_descriptors[TAG(p)]->side_with_edge[(e)][(k)])
#define CORNER_OF_SIDE_INV(p,s,c)       (element_descriptors[TAG(p)]->corner_of_side_inv[(s)][(c)])
#define EDGES_OF_CORNER(p,c,k)          (element_descriptors[TAG(p)]->edges_of_corner[(c)][(k)])
#define CORNER_OF_OPPEDGE(p,e,c)        (element_descriptors[TAG(p)]->corner_of_oppedge[(e)][(c)])
#define CORNER_OPP_TO_SIDE(p,s)         (element_descriptors[TAG(p)]->corner_opp_to_side[(s)])
#define OPPOSITE_EDGE(p,e)                  (element_descriptors[TAG(p)]->opposite_edge[(e)])
#define SIDE_OPP_TO_CORNER(p,c)         (element_descriptors[TAG(p)]->side_opp_to_corner[(c)])
#define EDGE_OF_CORNER(p,c,e)           (element_descriptors[TAG(p)]->edge_of_corner[(c)][(e)])

#define CTRL2(p)        ((p)->ge.flag)
#define FLAG(p)                 ((p)->ge.flag)
#define SUCCE(p)                ((p)->ge.succ)
#define PREDE(p)                ((p)->ge.pred)

#ifdef __CENTERNODE__
#define CENTERNODE(p)   ((p)->ge.centernode)
#endif
#define CORNER(p,i)     ((NODE *) (p)->ge.refs[n_offset[TAG(p)]+(i)])
#define EFATHER(p)              ((ELEMENT *) (p)->ge.refs[father_offset[TAG(p)]])
#define SON(p,i)                ((ELEMENT *) (p)->ge.refs[sons_offset[TAG(p)]+(i)])
/** \todo NbElem is declared in ugm.h, but never defined.
    We need a clean solution. */
/*
   #if defined(__TWODIM__)
   #define NBELEM(p,i)     NbElem((p),(i))
   #else
 */
#define NBELEM(p,i)     ((ELEMENT *) (p)->ge.refs[nb_offset[TAG(p)]+(i)])
/*
   #endif
 */
#define ELEM_BNDS(p,i)  ((BNDS *) (p)->ge.refs[side_offset[TAG(p)]+(i)])
#define EVECTOR(p)              ((VECTOR *) (p)->ge.refs[evector_offset[TAG(p)]])
#define SVECTOR(p,i)    ((VECTOR *) (p)->ge.refs[svector_offset[TAG(p)]+(i)])
#define SIDE_ON_BND(p,i) (ELEM_BNDS(p,i) != NULL)
#define INNER_SIDE(p,i)  (ELEM_BNDS(p,i) == NULL)
#define INNER_BOUNDARY(p,i) (InnerBoundary(p,i))
/* TODO: replace by function call */

#ifdef __TWODIM__
#define EDGE_ON_BND(p,i) (ELEM_BNDS(p,i) != NULL)
#endif

#ifdef __THREEDIM__
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
   #if defined(__TWODIM__)
   #define SET_NBELEM(p,i,q)       Set_NbElem((p),(i),(q))
   #else
 */
#define SET_NBELEM(p,i,q)       ((p)->ge.refs[nb_offset[TAG(p)]+(i)] = q)
/*
   #endif
 */
#if defined(__TWODIM__)
#define VOID_NBELEM(p,i)        NBELEM(p,i)
#else
#define VOID_NBELEM(p,i)        ((p)->ge.refs[nb_offset[TAG(p)]+(i)])
#endif
#define SET_BNDS(p,i,q)         ((p)->ge.refs[side_offset[TAG(p)]+(i)] = q)
#define SET_EVECTOR(p,q)        ((p)->ge.refs[evector_offset[TAG(p)]] = q)
#define SET_SVECTOR(p,i,q)      ((p)->ge.refs[svector_offset[TAG(p)]+(i)] = q)

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

#define SONS_OF_TAG(t)                                  (element_descriptors[t]->max_sons_of_elem) /* this is the number of pointers ! */

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
#define CORNER_OF_OPPEDGE_TAG(t,e,c)    (element_descriptors[t]->corner_of_oppedge[(e)][(c)])
#define CORNER_OPP_TO_SIDE_TAG(t,s)             (element_descriptors[t]->corner_opp_to_side[(s)])
#define OPPOSITE_EDGE_TAG(t,e)              (element_descriptors[t]->opposite_edge[(e)])
#define SIDE_OPP_TO_CORNER_TAG(t,c)             (element_descriptors[t]->side_opp_to_corner[(c)])
#define EDGE_OF_CORNER_TAG(t,c,e)               (element_descriptors[t]->edge_of_corner[(c)][(e)])
/*@}*/

/** @name  Macros to access reference descriptors by number of element corners  */
/*@{*/
#define SIDES_OF_REF(n)                   (reference_descriptors[n]->sides_of_elem)
#define EDGES_OF_REF(n)                   (reference_descriptors[n]->edges_of_elem)
#define CORNERS_OF_REF(n)                 (reference_descriptors[n]->corners_of_elem)
#define LOCAL_COORD_OF_REF(n,c)           (reference_descriptors[n]->local_corner[(c)])
#define EDGES_OF_SIDE_REF(n,i)            (reference_descriptors[n]->edges_of_side[(i)])
#define CORNERS_OF_SIDE_REF(n,i)          (reference_descriptors[n]->corners_of_side[(i)])
#define EDGE_OF_SIDE_REF(n,s,e)           (reference_descriptors[n]->edge_of_side[(s)][(e)])
#define EDGE_OF_TWO_SIDES_REF(n,s,t)  (reference_descriptors[n]->edge_of_two_sides[(s)][(t)])
#define CORNER_OF_SIDE_REF(n,s,c)         (reference_descriptors[n]->corner_of_side[(s)][(c)])
#define CORNER_OF_EDGE_REF(n,e,c)         (reference_descriptors[n]->corner_of_edge[(e)][(c)])
#define EDGE_WITH_CORNERS_REF(n,c0,c1) (reference_descriptors[n]->edge_with_corners[(c0)][(c1)])
#define SIDE_WITH_EDGE_REF(n,e,k)         (reference_descriptors[n]->side_with_edge[(e)][(k)])
#define CORNER_OF_SIDE_INV_REF(n,s,c) (reference_descriptors[n]->corner_of_side_inv[(s)][(c)])
#define EDGES_OF_CORNER_REF(n,c,k)        (reference_descriptors[n]->edges_of_corner[(c)][(k)])
#define CORNER_OF_OPPEDGE_REF(n,e,c)  (reference_descriptors[n]->corner_of_oppedge[(e)][(c)])
#define CORNER_OPP_TO_SIDE_REF(n,s)       (reference_descriptors[n]->corner_opp_to_side[(s)])
#define OPPOSITE_EDGE_REF(n,e)            (reference_descriptors[n]->opposite_edge[(e)])
#define SIDE_OPP_TO_CORNER_REF(n,c)       (reference_descriptors[n]->side_opp_to_corner[(c)])
#define EDGE_OF_CORNER_REF(n,c,e)         (reference_descriptors[n]->edge_of_corner[(c)][(e)])
/*@}*/

/****************************************************************************/
/*                                                                          */
/* macros for grids                                                         */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define GRID_OFFSET                                             0
#define GRID_STATUS_OFFSET                              1

#define GLEVEL(p)                                       ((p)->level)
#define GATTR(p)                                        ((p)->attribut)
#define GFORMAT(p)                                      MGFORMAT(MYMG(p))
#define SETGLOBALGSTATUS(p)             ((p)->status=~0)
#define GSTATUS(p,n)                            ((p)->status&(n))
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
#define NC(p)                           ((p)->nCon)
#define VEC_DEF_IN_OBJ_OF_GRID(p,tp)     (GFORMAT(p)->OTypeUsed[(tp)]>0)
#define NIMAT(p)                        ((p)->nIMat)
#define NELIST_DEF_IN_GRID(p)  (GFORMAT(p)->nodeelementlist)
#define NDATA_DEF_IN_GRID(p)   (GFORMAT(p)->nodedata)

#define GRID_ATTR(g) ((unsigned char) (GLEVEL(g)+32))
#define ATTR_TO_GLEVEL(i) (i-32)

/****************************************************************************/
/*                                                                          */
/* macros for multigrids                                                    */
/*                                                                          */
/****************************************************************************/

/* control word offset */
#define MULTIGRID_STATUS_OFFSET           ((sizeof(ENVDIR))/sizeof(UINT))

#define MGSTATUS(p)                     ((p)->status)
#define RESETMGSTATUS(p)                {(p)->status=0; (p)->magic_cookie = (int)time(NULL); (p)->saved=0;}
#define MG_MAGIC_COOKIE(p)              ((p)->magic_cookie)
#define VIDCNT(p)                       ((p)->vertIdCounter)
#define NIDCNT(p)                       ((p)->nodeIdCounter)
#define EIDCNT(p)                       ((p)->elemIdCounter)
#define TOPLEVEL(p)                     ((p)->topLevel)
#define BOTTOMLEVEL(p)                  ((p)->bottomLevel)
#define CURRENTLEVEL(p)                 ((p)->currentLevel)
#define FULLREFINELEVEL(p)              ((p)->fullrefineLevel)
#define MGFORMAT(p)                     ((p)->theFormat)
#define DATAFORMAT(p)                   MGFORMAT(p)
#define MG_BVP(p)                               ((p)->theBVP)
#define MG_BVPD(p)                              (&((p)->theBVPD))
#define MGBNDSEGDESC(p,i)               (&((p)->segments[i]))
#define MGVERTEX(p,k)                   ((p)->corners[k])
#define MGNOOFCORNERS(p)                ((p)->numOfCorners)
#define MGHEAP(p)                               ((p)->theHeap)
#define MG_NPROPERTY(p)                 ((p)->nProperty)
#define GRID_ON_LEVEL(p,i)              ((p)->grids[i])
#define MGNAME(p)                               ((p)->v.name)
#define MG_USER_HEAP(p)                 ((p)->UserHeap)
#define GEN_MGUD(p)                     ((p)->GenData)
#define GEN_MGUD_ADR(p,o)               ((void *)(((char *)((p)->GenData))+(o)))
#define VEC_DEF_IN_OBJ_OF_MG(p,tp)       (MGFORMAT(p)->OTypeUsed[(tp)]>0)
#define NELIST_DEF_IN_MG(p)     (MGFORMAT(p)->nodeelementlist)
#define NDATA_DEF_IN_MG(p)      (MGFORMAT(p)->nodedata)
#define MG_SAVED(p)                             ((p)->saved)
#define MG_FILENAME(p)                  ((p)->filename)
#define MG_COARSE_FIXED(p)              ((p)->CoarseGridFixed)
#define MG_MARK_KEY(p)              ((p)->MarkKey)

/****************************************************************************/
/*                                                                          */
/* macros for formats                                                       */
/*                                                                          */
/****************************************************************************/

#define FMT_NODE_DATA(f)                                ((f)->nodedata)
#define FMT_NODE_ELEM_LIST(f)                   ((f)->nodeelementlist)
#define FMT_S_VERTEX(f)                                 ((f)->sVertex)
#define FMT_S_MG(f)                                             ((f)->sMultiGrid)
#define FMT_S_VEC_TP(f,t)                               ((f)->VectorSizes[t])
#define FMT_VTYPE_NAME(f,t)                             ((f)->VTypeNames[t])
#define FMT_S_MAT_TP(f,t)                               ((f)->MatrixSizes[t])
#define FMT_S_MATPTR(f)                                 ((f)->MatrixSizes)
#define FMT_S_IMAT_TP(f,t)                              ((f)->IMatrixSizes[t])
#define FMT_CONN_DEPTH_TP(f,t)                  ((f)->ConnectionDepth[t])
#define FMT_CONN_DEPTH_PTR(f)                   ((f)->ConnectionDepth)
#define FMT_CONN_DEPTH_MAX(f)                   ((f)->MaxConnectionDepth)
#define FMT_NB_DEPTH(f)                                 ((f)->NeighborhoodDepth)
#define FMT_PR_VERTEX(f)                                ((f)->PrintVertex)
#define FMT_PR_GRID(f)                                  ((f)->PrintGrid)
#define FMT_PR_MG(f)                                    ((f)->PrintMultigrid)
#define FMT_PR_VEC(f)                                   ((f)->PrintVector)
#define FMT_PR_MAT(f)                                   ((f)->PrintMatrix)
#define FMT_PO2T(f,p,o)                                 ((f)->po2t[p][o])
#define FMT_T2P(f,t)                                    ((f)->t2p[t])
#define FMT_TYPE_IN_PART(f,t,o)                 ((f)->t2p[o] & (1<<o))
#define FMT_T2O(f,o)                                    ((f)->t2o[o])
#define FMT_TYPE_USES_OBJ(f,t,o)                ((f)->t2o[o] & (1<<o))
#define FMT_USES_OBJ(f,o)                               ((f)->OTypeUsed[o])
#define FMT_MAX_PART(f)                                 ((f)->MaxPart)
#define FMT_MAX_TYPE(f)                                 ((f)->MaxType)

#define FMT_N2T(f,c)                                    (((c)<FROM_VTNAME) ? NOVTYPE : ((c)>TO_VTNAME) ? NOVTYPE : (f)->n2t[(c)-FROM_VTNAME])
#define FMT_SET_N2T(f,c,t)                              ((f)->n2t[(c)-FROM_VTNAME] = t)
#define FMT_T2N(f,t)                                    (((f)->t2n[t]))

/** \brief Constants for USED flags of objects */
enum {MG_ELEMUSED =    1,
      MG_NODEUSED =    2,
      MG_EDGEUSED =    4,
      MG_VERTEXUSED =   8,
      MG_VECTORUSED =  16,
      MG_MATRIXUSED =  32};


/****************************************************************************/
/*                                                                          */
/* declaration of exported global variables                                 */
/*                                                                          */
/****************************************************************************/

#if defined ModelP && defined __OVERLAP2__
extern INT ce_NO_DELETE_OVERLAP2;
#endif

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

enum {GM_FCFCLL = 1,
      GM_FFCCLL = 2,
      GM_FFLLCC = 3,
      GM_FFLCLC = 4,
      GM_CCFFLL = 5};

enum {GM_LOV_BEGIN = 1,
      GM_LOV_END = 2};

enum {GM_GEN_FIRST, GM_GEN_LAST, GM_GEN_CUT};

enum {GM_ALL_LEVELS = 1,
      GM_CURRENT_LEVEL = 2};

enum {GM_ORDER_IN_COLS, GM_ORDER_IN_ROWS};

enum {GM_PUT_AT_BEGIN = 1,               /*!< put skip vectors at begin of the list */
      GM_PUT_AT_END = 2                  /*!< put skip vectors at end of the list */
};
#define GM_TAKE_SKIP                            (1<<0)
#define GM_TAKE_NONSKIP                         (1<<1)
/*@}*/

/* get/set current multigrid, loop through multigrids */
MULTIGRID               *MakeMGItem                             (const char *name);
MULTIGRID               *GetMultigrid                           (const char *name);
MULTIGRID               *GetFirstMultigrid                      (void);
MULTIGRID               *GetNextMultigrid                       (const MULTIGRID *theMG);

/* format definition */
FORMAT                   *GetFormat                             (const char *name);
FORMAT                   *GetFirstFormat                        (void);
FORMAT                   *GetNextFormat                         (FORMAT * fmt);
INT                               ChangeToFormatDir                     (const char *name);
INT                               DeleteFormat                          (const char *name);
FORMAT                   *CreateFormat (char *name, INT sVertex, INT sMultiGrid,
                                        ConversionProcPtr PrintVertex,
                                        ConversionProcPtr PrintGrid,
                                        ConversionProcPtr PrintMultigrid,
                                        TaggedConversionProcPtr PrintVector,
                                        TaggedConversionProcPtr PrintMatrix,
                                        INT nvDesc, VectorDescriptor *vDesc,
                                        INT nmDesc, MatrixDescriptor *mDesc,
                                        SHORT ImatTypes[],
                                        INT po2t[MAXDOMPARTS][MAXVOBJECTS],
                                        INT nodeelementlist, INT ndata);

/* create, saving and disposing a multigrid structure */
MULTIGRID *CreateMultiGrid (char *MultigridName, char *BndValProblem,
                            const char *format, NS_PREFIX MEM heapSize,
                            INT optimizedIE, INT insertMesh);
MULTIGRID *OpenMGFromDataFile(MULTIGRID *theMG, INT number, char *type,
                              char *DataFileName, NS_PREFIX MEM heapSize);
MULTIGRID       *LoadMultiGrid  (const char *MultigridName, const char *name, const char *type,
                                 const char *BndValProblem, const char *format,
                                 unsigned long heapSize,INT force,INT optimizedIE, INT autosave);
INT             SaveMultiGrid (MULTIGRID *theMG, const char *name, const char *type, const char *comment, INT autosave, INT rename);
INT         DisposeGrid             (GRID *theGrid);
INT             DisposeMultiGrid                (MULTIGRID *theMG);
INT         DisposeAMGLevels        (MULTIGRID *theMG);
INT         Collapse                (MULTIGRID *theMG);
#ifdef __TWODIM__
INT                     SaveCnomGridAndValues (MULTIGRID *theMG, char *FileName, char *plotprocName, char *tagName);
#endif

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
INT         TestRefineInfo          (MULTIGRID *theMG);
INT         SetRefineInfo           (MULTIGRID *theMG);


NODE            *GetFineNodeOnEdge              (const ELEMENT *theElement, INT side);

/* moving nodes */
#ifdef __THREEDIM__
INT                     GetSideIDFromScratch    (ELEMENT *theElement, NODE *theNode);
#endif

/* algebraic connections */
CONNECTION      *CreateExtraConnection  (GRID *theGrid, VECTOR *from, VECTOR *to);
INT             DisposeExtraConnections (GRID *theGrid);
INT             DisposeConnectionsInGrid (GRID *theGrid);
MATRIX          *GetMatrix                              (const VECTOR *FromVector, const VECTOR *ToVector);
CONNECTION      *GetConnection                  (const VECTOR *FromVector, const VECTOR *ToVector);
INT         GetAllVectorsOfElement  (GRID *theGrid, ELEMENT *theElement,
                                     VECTOR **vec);

/* searching */
NODE            *FindNodeFromId                 (const GRID *theGrid, INT id);
VECTOR      *FindVectorFromIndex    (GRID *theGrid, INT index);
ELEMENT         *FindElementFromId              (GRID *theGrid, INT id);
ELEMENT     *FindElementOnSurface   (MULTIGRID *theMG, DOUBLE *global);
ELEMENT     *FindElementOnSurfaceCached (MULTIGRID *theMG, DOUBLE *global);
ELEMENT     *NeighbourElement       (ELEMENT *t, INT side);
INT          InnerBoundary          (ELEMENT *t, INT side);

/* list */
void            ListMultiGridHeader     (const INT longformat);
void            ListMultiGrid           (const MULTIGRID *theMG, const INT isCurrent, const INT longformat);
INT         MultiGridStatus             (const MULTIGRID *theMG, INT gridflag, INT greenflag, INT lbflag, INT verbose);
void            ListGrids                               (const MULTIGRID *theMG);
void            ListNode                                (const MULTIGRID *theMG, const NODE *theNode, INT dataopt, INT bopt, INT nbopt, INT vopt);
void            ListElement                     (const MULTIGRID *theMG, const ELEMENT *theElement, INT dataopt, INT bopt, INT nbopt, INT vopt);
void            ListVector                      (const MULTIGRID *theMG, const VECTOR *theVector, INT matrixopt, INT dataopt, INT modifiers);

/* query */
LINK            *GetLink                                (const NODE *from, const NODE *to);
EDGE            *GetSonEdge                             (const EDGE *theEdge);
INT                     GetSonEdges                             (const EDGE *theEdge, EDGE *SonEdges[MAX_SON_EDGES]);
EDGE            *GetFatherEdge                  (const EDGE *theEdge);
#ifdef __THREEDIM__
EDGE            *FatherEdge                             (NODE **SideNodes, INT ncorners, NODE **Nodes, EDGE *theEdge);
#endif
EDGE            *GetEdge                                (const NODE *from, const NODE *to);
INT             GetSons                                 (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);
#ifdef ModelP
INT             GetAllSons                              (const ELEMENT *theElement, ELEMENT *SonList[MAX_SONS]);
#endif
INT             VectorPosition                  (const VECTOR *theVector, DOUBLE *position);
INT             VectorInElement                 (ELEMENT *theElement, VECTOR *theVector);

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
void            ListCWofObject                  (const void *obj, INT offset);
void            ListAllCWsOfObject              (const void *obj);
void            ListAllCWsOfAllObjectTypes (PrintfProcPtr myprintf);
UINT ReadCW                                     (const void *obj, INT ce);
void            WriteCW                                 (void *obj, INT ce, INT n);
void            ResetCEstatistics               (void);
void            PrintCEstatistics               (void);
INT             DefineMGUDBlock                 (NS_PREFIX BLOCK_ID id, NS_PREFIX MEM size);
INT             FreeMGUDBlock                   (NS_PREFIX BLOCK_ID id);
NS_PREFIX BLOCK_DESC      *GetMGUDBlockDescriptor (NS_PREFIX BLOCK_ID id);

/* ordering of degrees of freedom */
ALG_DEP         *CreateAlgebraicDependency              (const char *name, DependencyProcPtr DependencyProc);
FIND_CUT        *CreateFindCutProc                              (const char *name, FindCutProcPtr FindCutProc);

/* functions for evaluation-fct management */
INT              InitEvalProc                                                           (void);
EVALUES         *GetElementValueEvalProc                                        (const char *name);
EVECTOR         *GetElementVectorEvalProc                                       (const char *name);

/* miscellaneous */
INT             RenumberMultiGrid                                       (MULTIGRID *theMG, INT *nboe, INT *nioe, INT *nbov, INT *niov, NODE ***vid_n, INT *foid, INT *non, INT MarkKey);
INT                     OrderNodesInGrid                                        (GRID *theGrid, const INT *order, const INT *sign, INT AlsoOrderLinks);
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
