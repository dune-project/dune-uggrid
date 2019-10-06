// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      udm.h                                                         */
/*                                                                          */
/* Purpose:   user data manager (header file)                               */
/*                                                                          */
/* Author:    Peter Bastian                                                 */
/*                                                                          */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/*                                                                          */
/* History:   02.12.96 begin, ug version 3.4                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __UDM__
#define __UDM__

#include "ugtypes.h"
#include <gm/gm.h>
#include "sm.h"

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* macros concerned with data descriptors                                   */
/*                                                                          */
/****************************************************************************/

#define NVECTYPES                       MAXVECTORS
#define NMATTYPES                       MAXCONNECTIONS
#define NMATTYPES_NORMAL        MAXMATRICES

#define MTP(rt,ct)          ((rt)*NVECTYPES+(ct))
#define DMTP(rt)            (NMATTYPES_NORMAL+rt)
#define MTYPE_RT(mtp)       (((mtp)<NMATTYPES_NORMAL) ? (mtp)/NVECTYPES : (mtp)%NVECTYPES)
#define MTYPE_CT(mtp)           ((mtp)%NVECTYPES)

/** \brief Max nb of vec comps in one TYPE      */
#define MAX_SINGLE_VEC_COMP             40
/** \brief Max nb of mat comps in one TYPE              */
#define MAX_SINGLE_MAT_COMP       1600
/** \brief Max nb of comps in one VECDATA_DESC  */
#define MAX_VEC_COMP                40
/** \brief Max nb of comps in one MATDATA_DESC  */
#define MAX_MAT_COMP              7000
/** \brief Max #(comp) in one MATDATA_DESC       */
#define MAX_MAT_COMP_TOTAL        7000

#define NVECOFFSETS                             (NVECTYPES+1)

/** \brief For offset component in VECDATA_DESC */
#define NMATOFFSETS                             (NMATTYPES+1)

#define DEFAULT_NAMES "uvwzpabcdefghijklmnoPQRSTUVWXYZ123456789" /* of size MAX_VEC_COMP */

/** \brief No identification of components */
#define NO_IDENT                        -1

#define GENERATED_NAMES_SEPERATOR               "_"

/** @name Defines for VECDATA_DESC */
/*@{*/
#define VD_MG(vd)                                                       ((vd)->mg)
#define VD_ISDEF_IN_TYPE(vd,tp)             (VD_NCMPS_IN_TYPE(vd,tp)>0)
#define VD_NCMPPTR(vd)                              ((vd)->NCmpInType)
#define VD_NCMPS_IN_TYPE(vd,tp)             (VD_NCMPPTR(vd)[tp])
#define VD_CMP_OF_TYPE(vd,tp,i)             ((vd)->CmpsInType[tp][i])
#define VD_CMPPTR_OF_TYPE(vd,tp)            ((vd)->CmpsInType[tp])

#define VD_NID(vd)                                                      ((vd)->nId)
#define VD_IDENT_PTR(vd)                                        ((vd)->Ident)
#define VD_IDENT(vd,i)                                          ((vd)->Ident[i])

#define VD_DATA_TYPES(vd)                                       ((vd)->datatypes)
#define VD_OBJ_USED(vd)                                         ((vd)->objused)

#define VD_IS_SCALAR(vd)                    ((vd)->IsScalar)
#define VD_SCALCMP(vd)                                          ((vd)->ScalComp)
#define VD_SCALTYPEMASK(vd)                                     ((vd)->ScalTypeMask)
#define VD_OFFSETPTR(vd)                    ((vd)->offset)
#define VD_OFFSET(vd,tp)                    (VD_OFFSETPTR(vd)[tp])
#define VD_NCOMP(vd)                        (VD_OFFSETPTR(vd)[NVECTYPES])
#define VD_MINTYPE(vd)                      ((vd)->mintype)
#define VD_MAXTYPE(vd)                      ((vd)->maxtype)
#define VD_SUCC_COMP(vd)                    ((vd)->SuccComp)
/*@}*/


/** @name Macros for MATDATA_DESC */
/*@{*/
#define MCMP(row,col,ncol)                  ((row)*(ncol)+col)

#define MD_MG(md)                                                       ((md)->mg)
#define MD_ISDEF_IN_MTYPE(md,mtp)           (MD_ROWS_IN_MTYPE(md,mtp)>0)
#define MD_ISDEF_IN_RT_CT(md,rt,ct)         MD_ISDEF_IN_MTYPE(md,MTP(rt,ct))
#define MD_ROWPTR(md)                               ((md)->RowsInType)
#define MD_ROWS_IN_MTYPE(md,mtp)            (MD_ROWPTR(md)[mtp])
#define MD_ROWS_IN_RT_CT(md,rt,ct)          MD_ROWS_IN_MTYPE(md,MTP(rt,ct))
#define MD_COLPTR(md)                               ((md)->ColsInType)
#define MD_COLS_IN_MTYPE(md,mtp)            (MD_COLPTR(md)[mtp])
#define MD_COLS_IN_RT_CT(md,rt,ct)          MD_COLS_IN_MTYPE(md,MTP(rt,ct))
#define MD_NCMPS_IN_MTYPE(md,mtp)           (MD_ROWS_IN_MTYPE(md,mtp)*MD_COLS_IN_MTYPE(md,mtp))
#define MD_NCMPS_IN_RT_CT(md,rt,ct)         MD_NCMPS_IN_MTYPE(md,MTP(rt,ct))
#define MD_MCMPPTR(md)                      ((md)->CmpsInType)
#define MD_MCMPPTR_OF_MTYPE(md,mtp)         ((md)->CmpsInType[mtp])
#define MD_MCMP_OF_MTYPE(md,mtp,i)          ((md)->CmpsInType[mtp][i])
#define MD_MCMPPTR_OF_RT_CT(md,rt,ct)       ((md)->CmpsInType[MTP(rt,ct)])
#define MD_MCMP_OF_RT_CT(md,rt,ct,i)        MD_MCMP_OF_MTYPE(md,MTP(rt,ct),i)
#define MD_IJ_CMP_OF_MTYPE(md,mtp,i,j)      MD_MCMP_OF_MTYPE(md,mtp,MCMP(i,j,MD_COLS_IN_MTYPE(md,mtp)))
#define MD_IJ_CMP_OF_RT_CT(md,rt,ct,i,j)    MD_IJ_CMP_OF_MTYPE(md,MTP(rt,ct),i,j)

#define MD_ROW_DATA_TYPES(md)                           ((md)->rowdatatypes)
#define MD_COL_DATA_TYPES(md)                           ((md)->coldatatypes)
#define MD_ROW_OBJ_USED(md)                                     ((md)->rowobjused)
#define MD_COL_OBJ_USED(md)                                     ((md)->colobjused)

#define MD_IS_SPARSE(md)                    ((md)->IsSparse)
#define MD_IS_SCALAR(md)                    ((md)->IsScalar)
#define MD_SCALCMP(md)                                          ((md)->ScalComp)
#define MD_SCAL_RTYPEMASK(md)                           ((md)->ScalRowTypeMask)
#define MD_SCAL_CTYPEMASK(md)                           ((md)->ScalColTypeMask)
#define MD_OFFSETPTR(md)                        ((md)->offset)
#define MD_MTYPE_OFFSET(md,mtp)             (MD_OFFSETPTR(md)[mtp])
#define MD_RT_CT_OFFSET(md,mtp)             (MD_MTYPE_OFFSET(md,MTP(rt,ct)))
#define MD_SUCC_COMP(md)                    ((md)->SuccComp)
/*@}*/

/** @name Macros for VEC_SCALAR */
/*@{*/
#define VM_COMP_NAMEPTR(p)                 ((p)->compNames)
#define VM_COMP_NAME(p,i)                  (VM_COMP_NAMEPTR(p)[i])
#define VM_COMPPTR(p)                      ((p)->Components)
/*@}*/

/* we remove this for security reasons: please use function calls
   #define VM_LOCKED(p)                       ((p)->locked)
 */
#define EVM_LOCKED(p)                      ((p)->locked)

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

/** \brief A set of degrees of freedom associated to a geometrical object
 */
typedef struct {

  /** \brief Fields for environment list variable */
  NS_PREFIX ENVVAR v;

  /** \brief Locked for dynamic allocation */
  SHORT locked;

  /** \brief Associated multigrid */
  MULTIGRID *mg;

  /** \brief Names for symbol components */
  char compNames[MAX_VEC_COMP];

  /** \brief Number of components of a vector per type */
  SHORT NCmpInType[NVECTYPES];

  /** \brief Pointer to SHORT vector containing the components */
  SHORT *CmpsInType[NVECTYPES];

  /* redundant (but frequently used) information */
  /** \brief true if desc is scalar: same settings in all types */
  SHORT IsScalar;

  /** \brief Successive components */
  SHORT SuccComp;

  /** \brief Location of scalar component */
  SHORT ScalComp;

  /** \brief Mask for used vectypes */
  SHORT ScalTypeMask;

  /** \brief Offsets for VEC_SCALARs */
  SHORT offset[NVECOFFSETS];

  /** \brief Compact form of vtypes (bitwise) */
  SHORT datatypes;

  /** \brief Compact form of otypes (bitwise) */
  SHORT objused;

  /** \brief Minimal used type */
  SHORT mintype;

  /** \brief Maximal used type */
  SHORT maxtype;

  /** \brief Number of comps after ident */
  SHORT nId;

  /** \brief Identification table */
  SHORT *Ident;

  /** \brief Memory for component mapping */
  SHORT Components[1];

} VECDATA_DESC;

typedef struct {

  /** \brief Inheritance from environment variable class */
  NS_PREFIX ENVVAR v;

  /** \brief Locked for dynamic allocation        */
  SHORT locked;

  /** \brief Associated multigrid */
  MULTIGRID *mg;

  /** \brief Names for symbol components          */
  char compNames[2*MAX_MAT_COMP];

  /** \brief Number of rows of a matrix per type  */
  SHORT RowsInType[NMATTYPES];

  /** \brief Number of columns of a matrix per type */
  SHORT ColsInType[NMATTYPES];

  /** \brief Pointer to SHORT vector containing the components */
  SHORT *CmpsInType[NMATTYPES];

  /* redundant (but frequently used) information                          */
  /** \brief true if sparse form should be used   */
  SHORT IsSparse;

  /** \brief true if desc is scalar: same settings in all types             */
  SHORT IsScalar;

  /** \brief Successive components                */
  SHORT SuccComp;

  /** \brief Location of scalar component         */
  SHORT ScalComp;

  /** \brief Mask for used vectypes in rows       */
  SHORT ScalRowTypeMask;

  /** \brief Mask for used vectypes in cols       */
  SHORT ScalColTypeMask;

  /** \brief Offsets for what ever you need it    */
  SHORT offset[NMATOFFSETS];

  /** \brief Compact form of row vtypes (bitwise)     */
  SHORT rowdatatypes;

  /** \brief Compact form of col vtypes (bitwise)     */
  SHORT coldatatypes;

  /** \brief Compact form of row otypes (bitwise) */
  SHORT rowobjused;

  /** \brief Compact form of col otypes (bitwise) */
  SHORT colobjused;

  /** \brief Memory for component mapping         */
  SHORT Components[1];

} MATDATA_DESC;


typedef DOUBLE VEC_SCALAR[MAX_VEC_COMP];

/****************************************************************************/
/*                                                                          */
/* definition of exported data structures                                   */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported functions                                         */
/*                                                                          */
/****************************************************************************/

INT FillRedundantComponentsOfVD (VECDATA_DESC *vd);
INT FillRedundantComponentsOfMD (MATDATA_DESC *md);

VECDATA_DESC *GetVecDataDescByName (const MULTIGRID *theMG, char *name);
MATDATA_DESC *GetMatDataDescByName (const MULTIGRID *theMG, char *name);

/** @name Locking of vector and matrix descriptors */
/*@{*/
INT LockVD (MULTIGRID *theMG, VECDATA_DESC *vd);
INT LockMD (MATDATA_DESC *md);
INT UnlockMD (MATDATA_DESC *md);
/*@}*/

/** @name ??? */
/*@{*/
INT TransmitLockStatusVD (const VECDATA_DESC *vd, VECDATA_DESC *svd);
INT TransmitLockStatusMD (const MATDATA_DESC *md, MATDATA_DESC *smd);
/*@}*/

INT ConstructVecOffsets         (const SHORT *NCmpInType, SHORT *offset);
INT ConstructMatOffsets         (const SHORT *RowsInType, const SHORT *ColsInType, SHORT *offset);

/** \brief Init user data manager */
INT InitUserDataManager (void);

END_UGDIM_NAMESPACE

#endif
