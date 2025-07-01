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
#include <memory>

#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/namespace.h>

#ifdef ModelP
#  include <dune/uggrid/parallel/ddd/dddtypes.hh>
#endif

#include <dune/common/fvector.hh>

#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugtypes.h>

START_UGDIM_NAMESPACE

/** \brief Status for mesh */
enum MeshStatus {MESHSTAT_NOTINIT,
                 MESHSTAT_EMPTY,
                 MESHSTAT_CNODES,
                 MESHSTAT_SURFMESH,
                 MESHSTAT_MESH};

typedef void *BNDS;                /*!< Structure handled by domain module    */
typedef void *BNDP;                /*!< Structure handled by domain module    */

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

/*********************************************/
/* Code that was formerly in std_internal.hh */
/*********************************************/

typedef DOUBLE COORD_BND_VECTOR[DIM_OF_BND];

enum PatchType {POINT_PATCH_TYPE,
#ifdef UG_DIM_3
                LINE_PATCH_TYPE,
#endif
                LINEAR_PATCH_TYPE,
                PARAMETRIC_PATCH_TYPE};

/** @name Macros for DOMAIN */
/*@{*/
#define DOMAIN_NSEGMENT(p)                              ((p)->numOfSegments)
#define DOMAIN_NCORNER(p)                               ((p)->numOfCorners)
/*@}*/

/** @name Macros for STD_BVP */
/*@{*/
#define STD_BVP_DOMAIN(p)                               ((p)->Domain)

#define STD_BVP_NSIDES(p)                               ((p)->nsides)
#define STD_BVP_SIDEOFFSET(p)                   ((p)->sideoffset)
#define STD_BVP_PATCH(p,i)                              ((p)->patches[i])

#define GetSTD_BVP(p)                           ((STD_BVP *)(p))
/*@}*/

/****************************************************************************/
/*                                                                          */
/* macros for patches                                                       */
/*                                                                          */
/****************************************************************************/

/** @name Macros for patches */
/*@{*/
#define PATCH_TYPE(p)           (p)->ge.type
#define PATCH_ID(p)             (p)->ge.id
#define POINT_PATCH_N(p)        (p)->po.npatches
#define POINT_PATCH_PID(p,i)    (p)->po.pop[i].patch_id
#define POINT_PATCH_CID(p,i)    (p)->po.pop[i].corner_id
#define LINE_PATCH_C0(p)                ((p)->li.c0)
#define LINE_PATCH_C1(p)                ((p)->li.c1)
#define LINE_PATCH_N(p)         (p)->li.npatches
#define LINE_PATCH_PID(p,i)     (p)->li.lop[i].patch_id
#define LINE_PATCH_CID0(p,i)    (p)->li.lop[i].corner_id[0]
#define LINE_PATCH_CID1(p,i)    (p)->li.lop[i].corner_id[1]
#define PARAM_PATCH_LEFT(p)     (p)->pa.left
#define PARAM_PATCH_RIGHT(p)    (p)->pa.right
#define PARAM_PATCH_POINTS(p,i) (p)->pa.points[i]
#define PARAM_PATCH_RANGE(p)    (p)->pa.range
#define PARAM_PATCH_BS(p)       (p)->pa.BndSegFunc
#define PARAM_PATCH_BSD(p)      (p)->pa.bs_data
#define LINEAR_PATCH_LEFT(p)    (p)->lp.left
#define LINEAR_PATCH_RIGHT(p)   (p)->lp.right
#define LINEAR_PATCH_N(p)       (p)->lp.corners
#define LINEAR_PATCH_POINTS(p,i) (p)->lp.points[i]
#define LINEAR_PATCH_POS(p,i)    (p)->lp.pos[i]

#define BND_PATCH_ID(p)         (((BND_PS *)p)->patch_id)
#define BND_DATA(p)             (((BND_PS *)p)->data)
#define BND_N(p)                (((BND_PS *)p)->n)
#define BND_LOCAL(p,i)          (((BND_PS *)p)->local[i])
#define BND_SIZE(p)             ((((BND_PS *)p)->n-1)*sizeof(COORD_BND_VECTOR)+sizeof(BND_PS))
#define M_BNDS_NSIZE(n)         (((n)-1)*sizeof(M_BNDP)+sizeof(M_BNDS))
#define M_BNDS_SIZE(p)          M_BNDS_NSIZE(((M_BNDS *)(p))->n)

/*@}*/

/****************************************************************************/
/*                                                                          */
/* BoundaryValueProblem data structure                                      */
/*                                                                          */
/****************************************************************************/
/** \brief ???
 *
 * \todo Please doc me!
 */
struct std_BoundaryValueProblem
{
  ~std_BoundaryValueProblem();

  /** \brief Domain pointer                      */
  std::unique_ptr<domain> Domain;

  /** \brief File name for boundary infos        */
  char bnd_file[NS_PREFIX NAMESIZE];

  /** \brief File name for meshinfos             */
  char mesh_file[NS_PREFIX NAMESIZE];

  /** @name Boundary description */
  /*@{*/
  INT ncorners;
  INT nsides;
  INT sideoffset;

  /** \brief list of patches */
  union patch **patches;
  /*@}*/
};

/****************************************************************************/
/*                                                                          */
/* Patch data structure                                                     */
/*                                                                          */
/****************************************************************************/
/** \brief ???
 *
 * \todo Please doc me!
 */
struct generic_patch {


  /** \brief Patch type */
  enum PatchType type;

  /** \brief Unique id used for load/store */
  INT id;
};

/** \brief ???
 *
 * \todo Please doc me!
 */
struct point_on_patch {

  INT patch_id;
  INT corner_id;
};
/** \brief ???
 *
 * \todo Please doc me!
 */
struct point_patch {

  /** \brief Patch type */
  enum PatchType type;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Number of patches */
  INT npatches;

  /** \brief Reference to surface */
  struct point_on_patch pop[1];
};

#ifdef UG_DIM_3
/** \brief ???
 *
 * \todo Please doc me!
 */
struct line_on_patch {

  INT patch_id;
  INT corner_id[2];
};
/** \brief ???
 *
 * \todo Please doc me!
 */
struct line_patch {

  /** \brief Patch type */
  enum PatchType type;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Number of patches */
  INT npatches;

  /** \brief Corner 0 of line */
  INT c0;

  /** \brief Corner 1 of line */
  INT c1;

  /** \brief Reference to surface */
  struct line_on_patch lop[1];
};
#endif
/** \brief ???
 *
 * \todo Please doc me!
 */
struct linear_patch {

  /** \brief Patch type */
  enum PatchType type;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Id of left and right subdomain */
  INT left,right;

  /** \brief Number of corners */
  INT corners;

  /** \brief Ids of points */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief Position */
  DOUBLE pos[CORNERS_OF_BND_SEG][DIM];
};
/** \brief ???
 *
 * \todo Please doc me!
 */
struct parameter_patch {

  /** \brief Patch type */
  enum PatchType type;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Id of left and right subdomain */
  INT left,right;

  /** \brief Ids of points */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief Parameter range
   *
   * The range is an axis-aligned rectangle, described by two points
   * in DIM_OF_BND-dimensional space.
   */
  DOUBLE range[2][DIM_OF_BND];

  /** \brief Pointer to definition function */
  BndSegFuncPtr BndSegFunc;

  /** \brief Can be used by applic to find data */
  void *bs_data;

  /*@}*/
};

/** \brief ???
 *
 * \todo Please doc me!
 */
union patch {
  struct generic_patch ge;
  struct point_patch po;
  struct linear_patch lp;
  struct parameter_patch pa;
    #ifdef UG_DIM_3
  struct line_patch li;
        #endif
} ;

/** \brief ???
 *
 * \todo Please doc me!
 */
struct bnd_ps {

  /** \brief Associated patch                     */
  INT patch_id;

  /** \brief E.g. global coordinates, pointers... */
  void *data;

  /** \brief Number of arguments                  */
  INT n;

  /** \brief Parameter range                      */
  COORD_BND_VECTOR local[1];
};

/*----------- typedef for structs ------------------------------------------*/

/* typedefs */
#undef DOMAIN
typedef struct domain DOMAIN;
typedef struct linear_segment LINEAR_SEGMENT;
typedef struct boundary_segment BOUNDARY_SEGMENT;

typedef struct std_BoundaryValueProblem STD_BVP;
typedef union patch PATCH;
typedef struct point_patch POINT_PATCH;
typedef struct line_patch LINE_PATCH;
typedef struct linear_patch LINEAR_PATCH;
typedef struct parameter_patch PARAMETER_PATCH;
typedef struct bnd_ps BND_PS;


/* --- public functions --- */

/** \brief Access the id of the segment (used by DUNE) */
UINT GetBoundarySegmentId(BNDS* boundarySegment);

/** \todo Please doc me! */
struct mesh
{
  /** \brief Status */
  enum MeshStatus mesh_status;

  /** \brief Number of boundary points              */
  INT nBndP;

  /** \brief List of boundary points                 */
  BNDP **theBndPs;

  /** \brief Number of inner nodes                  */
  INT nInnP;

  /** \brief Positions of inner nodes            */
  DOUBLE **Position;

  /** \brief Number of boundary sides per subdomain */
  INT *nSides;

  /** \brief Number of side corners                 */
  INT **Side_corners;

  /** \brief Triangle_id for prism                */
  INT **xy_Side;

  /** \brief Corner ids                          */
  INT ***Side_corner_ids;

  /** \brief Number of elements per subdomain       */
  INT *nElements;

  /** \brief Number of element corners              */
  INT **Element_corners;

  /** \brief Number of side corners                 */
  INT ***Element_corner_ids;

  /** \brief Number of side corners                 */
  INT ***nbElements;

  /** \brief Used bitwise: sides on bnd for elem */
  INT **ElemSideOnBnd;

  /** @name Parallel part */
  /*@}*/
  /** \brief Level of vertex NULL if all vertex on level 0 */
  unsigned char *VertexLevel;

  /** \brief Priority of vertex   NULL if all vertex are master               */
  char *VertexPrio;

  /** \brief Level of element in subdomain   NULL if all elements on level 0             */
  char **ElementLevel;

  /** \brief Priority of element in subdomain  NULL if all elements are master             */
  char **ElementPrio;
  /*@}*/
};
typedef struct mesh MESH;


void Set_Current_BVP(STD_BVP *theBVP);

/****************************************************************************/
/** \brief Initialize a BVP and return a mesh
 *
 * @param theHeap - heap
 * @param MarkKey - use key for temporary memory allocation (do not Mark/Release)

   Function initialize a BVP and returns a mesh.
 */
/****************************************************************************/
void BVP_Init (STD_BVP *theBVP, NS_PREFIX HEAP *theHeap, MESH *Mesh, INT MarkKey);

/****************************************************************************/
/** \brief Write command to insert this BNDP
 *
 * @param theBndP - BNDP structure
 * @param data - string to store command
 * @param max_data_size - maximal datasize to use

   This function writes a command to string which inserts the BNDP.

 * @return <ul>
 *   <li> 0 if ok
 *   <li> 1 if error.
 * </ul> */
/****************************************************************************/
INT         BNDP_SaveInsertedBndP (BNDP *theBndP, char *data, INT max_data_size);

/****************************************************************************/
/** \brief Return global coordinates of BNDP
 *
 * @param theBndP - BNDP structure
 * @param global - global coordinates

   This function returns global coordinates of BNDP

 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error. </li>
 * </ul> */
/****************************************************************************/
INT         BNDP_Global           (const BNDP *theBndP, Dune::FieldVector<DOUBLE,DIM>& global);

/****************************************************************************/
/** \brief Sets descriptor for BNDP
 *
 * @param theBndP - BNDP structure
 * @param move  - movable flag (0: no, 1:yes)

   This function sets the descriptor for a BNDP.

 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error. </li>
 * </ul> */
/****************************************************************************/
INT         BNDP_BndPDesc         (BNDP *theBndP, INT *move);

/****************************************************************************/
/** \brief Sets descriptor for BNDE (boundary edge)
 *
 * @param theBndP0 - first BNDP
 * @param theBndP1 - second BNDP

   This function sets the descriptor for a BNDE.

 * @return <ul>
 *   <li> 0 if ok
 *   <li> 1 if error.
 * </ul> */
/****************************************************************************/
INT         BNDP_BndEDesc         (BNDP *theBndP0, BNDP *theBndP1);

/****************************************************************************/
/** \brief Creates a BNDS from a nb of BNDPs
 *
 * @param Heap  - heap to allocate from
 * @param theBndP - Pointer to list of BNDP structures
 * @param n     - Number of BNDPs

   This function creates a BNDS from n BNDPs

 * @return <ul>
 *   <li> pointer </li>
 *   <li> NULL if the points describe an inner side </li>
 * </ul> */
/****************************************************************************/
BNDS* BNDP_CreateBndS (NS_PREFIX HEAP *Heap, BNDP **theBndP, INT n);

/****************************************************************************/
/** \brief Sets BNDP from a two of BNDPs
 *
 * @param theBndP0 - first BNDP
 * @param theBndP1 - second BNDP
 * @param lcoord - local coordinate between P0 and P1 where the BNDP will be created

   This function sets a BNDP from two BNDPs

 * @return <ul>
 *   <li> pointer </li>
 *   <li> NULL if the points describe an inner point </li>
 * </ul> */
/****************************************************************************/
BNDP*       BNDP_CreateBndP (NS_PREFIX HEAP *Heap, BNDP *theBndP0, BNDP *theBndP1, DOUBLE lcoord);

/****************************************************************************/
/** \brief Dispose a BNDP
 *
 * @param Heap - heap
 * @param theBndP - BNDP

   This function disposes a BNDP

 * @return <ul>
 *   <li>    0 if ok  </li>
 *   <li>    1 if error.    </li>
 * </ul> */
/****************************************************************************/
INT         BNDP_Dispose          (NS_PREFIX HEAP *Heap, BNDP *theBndP);

/****************************************************************************/
/** \brief Save a BNDP
 *
 * @param theBndP - BNDP
 * @param stream - file

   This function saves a BNDP on a file.

 * @return <ul>
 *   <li>    0 if ok   </li>
 *   <li>    1 if error.     </li>
 * </ul> */
/****************************************************************************/
INT         BNDP_SaveBndP         (BNDP *theBndP);
INT         BNDP_SaveBndP_Ext     (BNDP *theBndP);

/****************************************************************************/
/** \brief Load a BNDP
 *
 * @param theBVP - BVP structure
 * @param Heap   - heap to allocate from
 * @param stream - file

   This function loads a BNDP with the format given by BVP_SaveBndP and
   allocates it from the given heap.

 * @return <ul>
 *   <li>    0 if ok  </li>
 *   <li>    1 if error.      </li>
 * </ul> */
/****************************************************************************/
BNDP       *BNDP_LoadBndP         (STD_BVP *theBVP, NS_PREFIX HEAP *Heap);
BNDP       *BNDP_LoadBndP_Ext     (void);

/****************************************************************************/
/** \brief Sets descriptor for BNDS
 *
 * @param theBndS - BNDS structure
 * @param id  - subdomain ID of the element with aBndS
 * @param nbid  - subdomain ID of the neighbour element (across the boundary)

   This function sets the descriptor for a BNDS.

 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error. </li>
 * </ul>
 */
/****************************************************************************/
INT         BNDS_BndSDesc         (BNDS *theBndS, INT *id, INT *nbid);

/****************************************************************************/
/** \brief Create BNDP on BNDS
 *
 * @param Heap  - heap to allocate from
 * @param theBndS - BNDS structure
 * @param local - local coordinate on BNDS

   This function creates a boundary point (BNDP) on a BNDS

 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error. </li>
 * </ul> */
/****************************************************************************/
BNDP*       BNDS_CreateBndP       (NS_PREFIX HEAP *Heap, BNDS *theBndS, const FieldVector<DOUBLE,DIM_OF_BND>& local);

/****************************************************************************/
/** \brief Dispose BNDS
 *
 * @param Heap - heap
 * @param theBndS - BNDS struct

   This function disposes a BNDS

 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error. </li>
 * </ul> */
/****************************************************************************/
INT         BNDS_Dispose          (NS_PREFIX HEAP *Heap, BNDS *theBndS);

/****************************************************************************/
/** \brief Gets surface ids for a BNDP
 *
 * @param aBndP - BNDP structure
 * @param i     - evaluate on patch i
 * @param n     - number of BNDS

   This function returns surface ids of the n surfaces on which
   the BNDP resides

 * @return  surface id
 */
/****************************************************************************/
INT         BNDP_SurfaceId        (BNDP *aBndP, INT *n, INT i);


#ifdef ModelP

void DomInitParallel     (INT TypeBndP, INT TypeBndS);
void DomHandlerInit      (INT handlerSet);

void BElementXferBndS    (DDD::DDDContext& context, BNDS **bnds, int n, int proc, int prio);
void BElementGatherBndS  (BNDS **bnds, int n, int cnt, char *data);
void BElementScatterBndS (const DDD::DDDContext& context, BNDS **bnds, int n, int cnt, char *data);

void BVertexXferBndP     (DDD::DDDContext& context, BNDP *bndp, int proc, int prio);
void BVertexGatherBndP   (BNDP *bndp, int cnt, char *data);
void BVertexScatterBndP  (const DDD::DDDContext& context, BNDP **bndp, int cnt, char *data);

#endif

END_UGDIM_NAMESPACE

/** @} */

#endif
