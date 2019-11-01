// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef UG_STD_INTERNAL_H
#define UG_STD_INTERNAL_H

#ifndef UGLIB
#error internal header! Must not be used in Applications!
#endif

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
#ifdef __THREEDIM__
                LINE_PATCH_TYPE,
#endif
                LINEAR_PATCH_TYPE,
                PARAMETRIC_PATCH_TYPE};

/** @name  Macros for DOMAIN_PART_INFO */
/*@{*/
#define DPI_SD2P_PTR(p)                                 ((p)->sd2part)
#define DPI_SD2P(p,sd)                                  ((p)->sd2part[sd])
#define DPI_SG2P_PTR(p)                                 ((p)->sg2part)
#define DPI_SG2P(p,sg)                                  ((p)->sg2part[sg])
#define DPI_LN2P_PTR(p)                                 ((p)->ln2part)
#define DPI_LN2P(p,c0,c1)                               ((p)->ln2part[c0][c1])
#define DPI_PT2P_PTR(p)                                 ((p)->pt2part)
#define DPI_PT2P(p,pt)                                  ((p)->pt2part[pt])
/*@}*/

/** @name Macros for DOMAIN */
/*@{*/
#define DOMAIN_NSEGMENT(p)                              ((p)->numOfSegments)
#define DOMAIN_NCORNER(p)                               ((p)->numOfCorners)
#define DOMAIN_NPARTS(p)                                ((p)->nParts)
#define DOMAIN_PARTINFO(p)                              ((p)->dpi)
/*@}*/

/** @name Macros for STD_BVP */
/*@{*/
#define STD_BVP_DOMAIN(p)                               ((p)->Domain)

#define STD_BVP_NSIDES(p)                               ((p)->nsides)
#define STD_BVP_SIDEOFFSET(p)                   ((p)->sideoffset)
#define STD_BVP_PATCH(p,i)                              ((p)->patches[i])

#define STD_BVP_NSUBDOM(p)                              ((p)->numOfSubdomains)
#define STD_BVP_NDOMPART(p)                             ((p)->nDomainParts)
#define STD_BVP_S2P_PTR(p)                              ((p)->s2p)
#define STD_BVP_S2P(p,s)                                ((p)->s2p[s])

#define GetSTD_BVP(p)                           ((STD_BVP *)(p))
/*@}*/

/****************************************************************************/
/*                                                                          */
/* macros for patches                                                       */
/*                                                                          */
/****************************************************************************/

/** @name Macros for patches */
/*@{*/
enum {PATCH_FIXED,
      PATCH_BND_OF_FREE,
      PATCH_FREE};

#define PATCH_TYPE(p)           (p)->ge.type
#define PATCH_STATE(p)          (p)->ge.state
#define PATCH_IS_FREE(p)                ((p)->ge.state==PATCH_FREE)
#define PATCH_IS_FIXED(p)               ((p)->ge.state==PATCH_FIXED)
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
#define PARAM_PATCH_BC(p)       (p)->pa.BndCond
#define PARAM_PATCH_BCD(p)      (p)->pa.bc_data
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
/* domain definition data structures                                        */
/*                                                                          */
/****************************************************************************/


/*----------- definition of structs ----------------------------------------*/


/** \brief Data type describing a domain. */
struct domain {

  /** \brief Fields for environment directory */
  NS_PREFIX ENVDIR d;

  /** \brief Number of boundary segments */
  INT numOfSegments;

  /** \brief Number of corner points */
  INT numOfCorners;

  /** @name Description of domain parts */
  /*@{*/

  /** \brief Number of parts in the domain */
  INT nParts;

  /** \brief Domain part info */
  const DOMAIN_PART_INFO *dpi;
  /*@}*/
};

/** \brief Data structure defining part of the boundary of a domain */
struct boundary_segment {

  /** \brief Field for environment directory */
  NS_PREFIX ENVVAR v;

  /** @name Fields for boundary segment */
  /*@{*/
  /** \brief Number of left and right subdomain */
  INT left,right;

  /** \brief Unique id of that segment */
  INT id;

  /** \brief Segment type, see above
   *
   * \todo See where???*/
  INT segType;

  /** \brief Numbers of the vertices (ID) */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief Parameter interval used*/
  DOUBLE alpha[DIM_OF_BND],beta[DIM_OF_BND];

  /** \brief Pointer to definition function */
  BndSegFuncPtr BndSegFunc;

  /** \brief Can be used by application to find data */
  void *data;
  /*@}*/
};

/** \brief ???
 *
 * \todo Please doc me!
 */
struct linear_segment {

  /** \brief Field for environment directory */
  NS_PREFIX ENVVAR v;

  /* fields for boundary segment */
  /** \brief  Number of left and right subdomain */
  INT left,right;

  /** \brief  Unique id of that segment                  */
  INT id;

  /** \brief  Number of corners                  */
  INT n;

  /** \brief  Numbers of the vertices (ID)*/
  INT points[CORNERS_OF_BND_SEG];

  /** \brief  Coordinates  */
  DOUBLE x[CORNERS_OF_BND_SEG][DIM];
};

/****************************************************************************/
/*                                                                          */
/* problem data structure                                                   */
/*                                                                          */
/****************************************************************************/

/*----------- definition of structs ----------------------------------------*/

/** \brief Data type describing a problem. */
struct problem {

  /** \brief Field for environment directory
   *
   * The problem is an environment directory. This directory is a subdirectory
   * of the domain where this problem corresponds to. d also contains the
   * name of the problem.
   */
  NS_PREFIX ENVDIR d;

  /* fields for problem */
  /** \brief Used to identify problem type
   *
   * Problem class identification number. This number is used to determine
   * that the problem description coincides with the pde solved by the
   * problem class library.
   */
  INT problemID;

  /** \brief Procedure to reinitialize problem
   *
   * Pointer to a user definable function that is executed when the reinit
   * command is given in the UG shell.
   */
  ConfigProcPtr ConfigProblem;

  /** \brief Number of coefficient functions
   *
   *  User definable coefficient functions come in two flavours.
   * They are either of type CoeffProcPtr or of type UserProcPtr.
   * numOfCoeffFct and numOfUserFct give the number of functions of each type that
   * make up the problem description.
   */
  INT numOfCoeffFct;

  /** \brief Number of User functions
   *
   * User definable coefficient functions come in two flavours.
   * They are either of type CoeffProcPtr or of type UserProcPtr.
   * numOfCoeffFct and numOfUserFct give the number of functions of each type that
   * make up the problem description.
   */
  INT numOfUserFct;

  /** \brief Coefficient functions
   *
   *  Array that stores the pointers to coefficient and user functions.
   * Since access to this array is provided through macros (see below) the layout
   * is not important. Note that this array is allocated dynamically to the desired length.
   */
  void *CU_ProcPtr[1];
};

/** \brief ???
 *
 * \todo Please doc me!
 */
struct bndcond {

  /** \brief Field for environment variable */
  NS_PREFIX ENVVAR v;

  /* fields for boundary condition */
  /** \brief Corresponds to boundary segment id ! */
  INT id;

  /** \brief Function defining boundary condition */
  BndCondProcPtr BndCond;

  /** \brief Additional data for bnd cond */
  void *data;
};

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
  struct domain *Domain;

  /** \brief Problem pointer                     */
  struct problem *Problem;

  /** \brief File name for boundary infos        */
  char bnd_file[NS_PREFIX NAMESIZE];

  /** \brief File name for meshinfos             */
  char mesh_file[NS_PREFIX NAMESIZE];

  /** @name Domain part */
  /*@{*/
  /** \brief Number of subdomains, exterior not counted                */
  INT numOfSubdomains;

  /** \brief Number of parts in the domain               */
  INT nDomainParts;

  /** \brief Pointer to table subbdom --> part   */
  INT *s2p;
  /*@}*/

  /** @name Boundary decription */
  /*@{*/
  INT ncorners;
  INT nsides;
  INT sideoffset;

  /** \brief list of patches */
  union patch **patches;
  /*@}*/

  /** @name Problem part */
  /*@{*/
  /** \brief Configuration function              */
  ConfigProcPtr ConfigProc;

  /** \brief Number of coefficient functions        */
  INT numOfCoeffFct;

  /** \brief Number of user functions               */
  INT numOfUserFct;

  /** \brief General bnd. cond. (if exists)      */
  BndCondProcPtr GeneralBndCond;

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

  /** \brief Fixed/bnd of free/free */
  INT state;

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

  /** \brief Fixed/bnd of free/free */
  INT state;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Number of patches */
  INT npatches;

  /** \brief Reference to surface */
  struct point_on_patch pop[1];
};

#ifdef __THREEDIM__
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

  /** \brief Fixed/bnd of free/free */
  INT state;

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

  /** \brief Fixed/bnd of free/free */
  INT state;

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

  /** \brief Fixed/bnd of free/free */
  INT state;

  /** \brief Unique id used for load/store */
  INT id;

  /** \brief Id of left and right subdomain */
  INT left,right;

  /** \brief Ids of points */
  INT points[CORNERS_OF_BND_SEG];

  /** \brief Parameter range */
  DOUBLE range[2][DIM_OF_BND];

  /** \brief Pointer to definition function */
  BndSegFuncPtr BndSegFunc;

  /** \brief Can be used by applic to find data */
  void *bs_data;

  /** @name Fields for boundary condition */
  /*@{*/
  /** \brief Function defining boundary condition */
  BndCondProcPtr BndCond;

  /** \brief Additional data for bnd cond */
  void *bc_data;
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
    #ifdef __THREEDIM__
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
typedef struct problem PROBLEM;
typedef struct bndcond BOUNDARY_CONDITION;

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

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

DOMAIN *GetDomain                           (const char *name);

END_UGDIM_NAMESPACE

#endif
