// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef UG_STD_INTERNAL_H
#define UG_STD_INTERNAL_H

#include <memory>

#include <dune/uggrid/low/dimension.h>
#include <dune/uggrid/low/namespace.h>

#include "domain.h"
#include "std_domain.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)        */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

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
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

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
  /** \brief Fields for environment directory */
  NS_PREFIX ENVDIR d;

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

  /** @name Problem part */
  /*@{*/
  /** \brief Number of coefficient functions        */
  INT numOfCoeffFct;

  /** \brief Number of user functions               */
  INT numOfUserFct;

  /** \brief Coefficient functions                           */
  void *CU_ProcPtr[1];
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

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

END_UGDIM_NAMESPACE

#endif
