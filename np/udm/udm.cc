// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      udm.c                                                         */
/*                                                                          */
/* Purpose:   user data manager                                             */
/*                                                                          */
/*                                                                          */
/* Author:    Christian Wieners                                             */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   December 9, 1996                                              */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>

#include <cstring>

#include <dev/ugdevices.h>
#include <gm/gm.h>
#include <gm/rm.h>
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/ugenv.h>
#include <np/np.h>

#include "udm.h"

USING_UG_NAMESPACES

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MAX_NAMES 99

#define READ_DR_VEC_FLAG(p,vt,i)        READ_FLAG((p)->data_status.VecReserv[vt][(i)/32],1<<((i)%32))
#define READ_DR_MAT_FLAG(p,mt,i)        READ_FLAG((p)->data_status.MatReserv[mt][(i)/32],1<<((i)%32))
#define SET_DR_VEC_FLAG(p,vt,i)         SET_FLAG((p)->data_status.VecReserv[vt][(i)/32],1<<((i)%32))
#define SET_DR_MAT_FLAG(p,mt,i)         SET_FLAG((p)->data_status.MatReserv[mt][(i)/32],1<<((i)%32))
#define CLEAR_DR_VEC_FLAG(p,vt,i)       CLEAR_FLAG((p)->data_status.VecReserv[vt][(i)/32],1<<((i)%32))
#define CLEAR_DR_MAT_FLAG(p,mt,i)       CLEAR_FLAG((p)->data_status.MatReserv[mt][(i)/32],1<<((i)%32))

#define A_REASONABLE_NUMBER                     100

/* vm decriptor lock status */
#define VM_LOCKED(p)               ((p)->locked)
#define VM_IS_UNLOCKED                          0
#define VM_IS_LOCKED                            1

#define CEIL(n)          ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))

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

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static INT VectorDirID;
static INT MatrixDirID;
static INT VectorVarID;
static INT MatrixVarID;

REP_ERR_FILE


/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/*			here follows vector stuff										*/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*D
   ConstructVecOffsets - Calculate offsets for VEC_SCALARs

   SYNOPSIS:
   INT ConstructVecOffsets (const SHORT *NCmpInType, SHORT *offset);

   PARAMETERS:
   .  NCmpInType - the numbers of components is used for the calculation of offsets
   .  offset - array of length NVECOFFSETS (!)

   DESCRIPTION:
   This function calculates offsets in DOUBLE vector called 'VEC_SCALAR'.
   It describes the number of components of each abstract type.

   .n      offset[0] = 0
   .n      offset[1] - offset[0] number of components in first type
   .n      offset[2] - offset[1] number of components in second type
   .n      etc.

   All components of a vector data descriptor are mapped uniquely to
   one of the DOUBLE values in the 'VEC_SCALAR'.

   'VD_CMP_OF_TYPE(vd,type,n)'

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX ConstructVecOffsets (const SHORT *NCmpInType, SHORT *offset)
{
  INT type;

  offset[0] = 0;
  for (type=0; type<NVECTYPES; type++)
    offset[type+1] = offset[type] + NCmpInType[type];

  return (NUM_OK);
}

/****************************************************************************/
/*D
   SetScalVecSettings - fill the scalar settings components of a VECDATA_DESC

   SYNOPSIS:
   INT SetScalVecSettings (VECDATA_DESC *vd)

   PARAMETERS:
   .  vd - fill this scalar settings components

   DESCRIPTION:
   This function fills the scalar settings components of a VECDATA_DESC.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

static INT SetScalVecSettings (VECDATA_DESC *vd)
{
  INT tp;

  VD_IS_SCALAR(vd) = false;

  /* check number of components per type */
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
    {
      if (VD_NCMPS_IN_TYPE(vd,tp)!=1)
        return (NUM_OK);                                                        /* no scalar */
      else
        VD_SCALCMP(vd) = VD_CMP_OF_TYPE(vd,tp,0);
    }

  /* check location of components per type */
  VD_SCALTYPEMASK(vd) = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
    {
      VD_SCALTYPEMASK(vd) |= 1<<tp;
      if (VD_SCALCMP(vd)!=VD_CMP_OF_TYPE(vd,tp,0))
        return (NUM_OK);                                                        /* no scalar */
    }

  VD_IS_SCALAR(vd) = true;
  return (NUM_OK);
}

static INT SetCompactTypesOfVec (VECDATA_DESC *vd)
{
  FORMAT *fmt;
  INT tp;

  /* fill bitwise fields */
  fmt = MGFORMAT(VD_MG(vd));
  VD_DATA_TYPES(vd) = VD_OBJ_USED(vd) = 0;
  VD_MAXTYPE(vd) = 0;
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
    {
      VD_MAXTYPE(vd) = tp;
      VD_DATA_TYPES(vd) |= BITWISE_TYPE(tp);
      VD_OBJ_USED(vd)   |= FMT_T2O(fmt,tp);
    }
  for (tp=0; tp<NVECTYPES; tp++)
    if (VD_ISDEF_IN_TYPE(vd,tp))
      break;
  VD_MINTYPE(vd) = tp;

  return (0);
}

static INT VDCompsSubsequent (const VECDATA_DESC *vd)
{
  INT tp,i;

  for (tp=0; tp<NVECTYPES; tp++)
    for (i=0; i<VD_NCMPS_IN_TYPE(vd,tp); i++)
      if (VD_CMP_OF_TYPE(vd,tp,i)!=VD_CMP_OF_TYPE(vd,tp,0)+i)
        return (NO);
  return (YES);
}

/****************************************************************************/
/*D
   FillRedundantComponentsOfVD - fill the redundant components of a VECDATA_DESC

   SYNOPSIS:
   INT FillRedundantComponentsOfVD (VECDATA_DESC *vd)

   PARAMETERS:
   .  vd - VECDATA_DESC

   DESCRIPTION:
   This function fills the redundant components of a VECDATA_DESC.
   The functions 'ConstructVecOffsets' and 'SetScalVecSettings' are called.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX FillRedundantComponentsOfVD (VECDATA_DESC *vd)
{
  ConstructVecOffsets(VD_NCMPPTR(vd),VD_OFFSETPTR(vd));
  SetCompactTypesOfVec(vd);
  SetScalVecSettings(vd);
  VD_SUCC_COMP(vd) = VDCompsSubsequent(vd);

  return (NUM_OK);
}

/****************************************************************************/
/*D
   LockVD - protect vector against removal or deallocation

   SYNOPSIS:
   INT LockVD (MULTIGRID *theMG, VECDATA_DESC *vd);

   PARAMETERS:
   .  theMG - multigrid pointer
   .  vd - vector descriptor

   DESCRIPTION:
   This function locks a vector against removal or deallocation.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT NS_DIM_PREFIX LockVD (MULTIGRID *theMG, VECDATA_DESC *vd)
{
  INT tp,j;

  VM_LOCKED(vd) = VM_IS_LOCKED;

  for (tp=0; tp<NVECTYPES; tp++)
    for (j=0; j<VD_NCMPS_IN_TYPE(vd,tp); j++)
      SET_DR_VEC_FLAG(theMG,tp,VD_CMP_OF_TYPE(vd,tp,j));

  return (0);
}

/****************************************************************************/
/*D
   TransmitLockStatusVD - ...

   SYNOPSIS:
   INT TransmitLockStatusVD (const VECDATA_DESC *vd, VECDATA_DESC *svd)

   PARAMETERS:
   .  vd  - vector descriptor
   .  svd - sub vector descriptor

   DESCRIPTION:
   This function ...

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT NS_DIM_PREFIX TransmitLockStatusVD (const VECDATA_DESC *vd, VECDATA_DESC *svd)
{
  if (!VM_LOCKED(vd) && VM_LOCKED(svd))
    REP_ERR_RETURN(1);
  VM_LOCKED(svd) = VM_LOCKED(vd);

  return (0);
}

/****************************************************************************/
/*D
   GetVecDataDescByName - find vector data desciptor

   SYNOPSIS:
   VECDATA_DESC *GetVecDataDescByName (MULTIGRID *theMG, char *name);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - name of a vector

   DESCRIPTION:
   This function finds a vector by name.

   RETURN VALUE:
   VECDATA_DESC *
   .n      pointer to the vector
   .n      NULL if there is no vector of this name in the multigrid
   D*/
/****************************************************************************/

VECDATA_DESC * NS_DIM_PREFIX GetVecDataDescByName (const MULTIGRID *theMG, char *name)
{
  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  return((VECDATA_DESC *) SearchEnv(name,"Vectors",
                                    VectorVarID,VectorDirID));
}

/****************************************************************************/
/****************************************************************************/
/*			here follows matrix stuff										*/
/****************************************************************************/
/****************************************************************************/



/****************************************************************************/
/*D
   ConstructMatOffsets - Calculate offsets for MAT_SCALARs

   SYNOPSIS:
   INT ConstructMatOffsets (const SHORT *RowsInType, const SHORT *ColsInType, SHORT *offset)

   PARAMETERS:
   .  RowsInType - the numbers of row components is used for the calculation of offsets
   .  ColsInType - the numbers of col components is used for the calculation of offsets
   .  offset - array of length NMATOFFSETS (!)

   DESCRIPTION:
   This function calculates offsets for MAT_SCALARs.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX ConstructMatOffsets (const SHORT *RowsInType, const SHORT *ColsInType, SHORT *offset)
{
  INT type;

  offset[0] = 0;
  for (type=0; type<NMATTYPES; type++)
    offset[type+1] = offset[type] + RowsInType[type]*ColsInType[type];

  return (NUM_OK);
}

/****************************************************************************/
/*D
   SetScalMatSettings - fill the scalar settings components of a MATDATA_DESC

   SYNOPSIS:
   INT SetScalMatSettings (MATDATA_DESC *md)

   PARAMETERS:
   .  md - fill this scalar settings components

   DESCRIPTION:
   This function fills the scalar settings components of a MATDATA_DESC.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

static INT SetScalMatSettings (MATDATA_DESC *md)
{
  INT mtp;

  MD_IS_SCALAR(md) = false;

  /* check number of components per type */
  for (mtp=0; mtp<NMATTYPES; mtp++)
    if (MD_ISDEF_IN_MTYPE(md,mtp))
    {
      if ((MD_ROWS_IN_MTYPE(md,mtp)!=1) || (MD_COLS_IN_MTYPE(md,mtp)!=1))
        return (NUM_OK);                                                        /* no scalar */
      else
        MD_SCALCMP(md) = MD_MCMP_OF_MTYPE(md,mtp,0);
    }

  /* check location of components per type */
  MD_SCAL_RTYPEMASK(md) = MD_SCAL_CTYPEMASK(md) = 0;
  for (mtp=0; mtp<NMATTYPES; mtp++)
    if (MD_ISDEF_IN_MTYPE(md,mtp))
    {
      MD_SCAL_RTYPEMASK(md) |= 1<<MTYPE_RT(mtp);
      MD_SCAL_CTYPEMASK(md) |= 1<<MTYPE_CT(mtp);
      if (MD_SCALCMP(md)!=MD_MCMP_OF_MTYPE(md,mtp,0))
        return (NUM_OK);                                                        /* no scalar */
    }

  MD_IS_SCALAR(md) = true;

  return (NUM_OK);
}

static INT SetCompactTypesOfMat (MATDATA_DESC *md)
{
  FORMAT *fmt;
  INT rt,ct;

  /* fill bitwise fields */
  fmt = MGFORMAT(MD_MG(md));
  MD_ROW_DATA_TYPES(md) = MD_COL_DATA_TYPES(md) =
                            MD_ROW_OBJ_USED(md) = MD_COL_OBJ_USED(md) = 0;
  for (rt=0; rt<NVECTYPES; rt++)
    for (ct=0; ct<NVECTYPES; ct++)
      if (MD_ISDEF_IN_RT_CT(md,rt,ct))
      {
        MD_ROW_DATA_TYPES(md) |= BITWISE_TYPE(rt);
        MD_COL_DATA_TYPES(md) |= BITWISE_TYPE(ct);
        MD_ROW_OBJ_USED(md)   |= FMT_T2O(fmt,rt);
        MD_COL_OBJ_USED(md)   |= FMT_T2O(fmt,ct);
      }
  return (0);
}

static INT MDCompsSubsequent (const MATDATA_DESC *md)
{
  INT tp,i;

  for (tp=0; tp<NMATTYPES; tp++)
    for (i=0; i<MD_NCMPS_IN_MTYPE(md,tp); i++)
      if (MD_MCMP_OF_MTYPE(md,tp,i)!=MD_MCMP_OF_MTYPE(md,tp,0)+i)
        return (NO);
  return (YES);
}

/****************************************************************************/
/*D
   FillRedundantComponentsOfMD - fill the redundant components of a MATDATA_DESC

   SYNOPSIS:
   INT FillRedundantComponentsOfMD (MATDATA_DESC *md)

   PARAMETERS:
   .  md - MATDATA_DESC

   DESCRIPTION:
   This function fills the redundant components of a MATDATA_DESC.
   The functions 'ConstructMatOffsets' and 'SetScalMatSettings' are called.

   RETURN VALUE:
   INT
   .n    NUM_OK
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX FillRedundantComponentsOfMD (MATDATA_DESC *md)
{
  ConstructMatOffsets(MD_ROWPTR(md),MD_COLPTR(md),MD_OFFSETPTR(md));
  SetCompactTypesOfMat(md);
  SetScalMatSettings(md);
  MD_SUCC_COMP(md) = MDCompsSubsequent(md);

  return (NUM_OK);
}

/****************************************************************************/
/*D
   GetMatDataDescByName - find matrix data descriptor

   SYNOPSIS:
   MATDATA_DESC *GetMatDataDescByName (MULTIGRID *theMG, char *name);

   PARAMETERS:
   .  theMG - create vector for this multigrid
   .  name - name of a matrix

   DESCRIPTION:
   This function finds a matrix by name.

   RETURN VALUE:
   MATDATA_DESC *
   .n      pointer to the matrix
   .n      NULL if there is no matrix of this name in the multigrid
   D*/
/****************************************************************************/

MATDATA_DESC * NS_DIM_PREFIX GetMatDataDescByName (const MULTIGRID *theMG, char *name)
{
  if (ChangeEnvDir("/Multigrids") == NULL) return (NULL);
  if (ChangeEnvDir(ENVITEM_NAME(theMG)) == NULL) return (NULL);
  return((MATDATA_DESC *) SearchEnv(name,"Matrices",
                                    MatrixVarID,MatrixDirID));
}

/****************************************************************************/
/*D
   LockMD - protect matrix against removal or deallocation

   SYNOPSIS:
   INT LockMD (MATDATA_DESC *md)

   PARAMETERS:
   .  md - matrix descriptor

   DESCRIPTION:
   This function protects a matrix against removal or deallocation.

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT NS_DIM_PREFIX LockMD (MATDATA_DESC *md)
{
  VM_LOCKED(md) = VM_IS_LOCKED;
  return (0);
}

INT NS_DIM_PREFIX UnlockMD (MATDATA_DESC *md)
{
  VM_LOCKED(md) = VM_IS_UNLOCKED;
  return (0);
}

/****************************************************************************/
/*D
   TransmitLockStatusMD - ...

   SYNOPSIS:
   INT TransmitLockStatusMD (const MATDATA_DESC *md, MATDATA_DESC *smd)

   PARAMETERS:
   .  md  - matrix descriptor
   .  smd - sub matrix descriptor

   DESCRIPTION:
   This function ...

   RETURN VALUE:
   INT
   .n      0 if ok
   .n      1 if error occurred
 */
/****************************************************************************/

INT NS_DIM_PREFIX TransmitLockStatusMD (const MATDATA_DESC *md, MATDATA_DESC *smd)
{
  if (!VM_LOCKED(md) && VM_LOCKED(smd))
    REP_ERR_RETURN(1);
  VM_LOCKED(smd) = VM_LOCKED(md);

  return (0);
}

/****************************************************************************/
/*
   InitUserDataManager - Init this file

   SYNOPSIS:
   INT InitUserDataManager (void);

   PARAMETERS:
   .  void -

   DESCRIPTION:
   This function inits this file.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    __LINE__ if error occured.
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitUserDataManager ()
{
  MatrixDirID = GetNewEnvDirID();
  VectorDirID = GetNewEnvDirID();
  MatrixVarID = GetNewEnvVarID();
  VectorVarID = GetNewEnvVarID();

  return (NUM_OK);
}
