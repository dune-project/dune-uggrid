// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  enrol.c														*/
/*																			*/
/* Purpose:   contains functions to enrol user defineable structures to         */
/*			  ug's environment.                                             */
/*																			*/
/* Author:	  Peter Bastian                                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de						        */
/*																			*/
/* History:   12.11.94 begin, ug version 3.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/*		defines to exclude functions										*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>

/* low modules */
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugtypes.h>

/* devices module */
#include <dune/uggrid/ugdevices.h>

/* grid manager module */
#include "gm.h"
#include "algebra.h"
#include "enrol.h"

USING_UG_NAMESPACE
USING_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
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

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static INT theSymbolVarID;                      /* env type for Format vars                     */

std::unique_ptr<FORMAT> NS_DIM_PREFIX CreateFormat ()
{
  INT i, j, type, type2, part, obj, MaxDepth, NeighborhoodDepth, MaxType;

  std::string name = "DuneFormat" + std::to_string(DIM) + "d";

  /* allocate new format structure */
  auto fmt = std::make_unique<FORMAT>();
  if (fmt==NULL) REP_ERR_RETURN_PTR(NULL);

  /* initialize with zero */
  for (i=0; i<MAXVECTORS; i++)
  {
    FMT_S_VEC_TP(fmt,i) = 0;
  }
  for (i=0; i<MAXCONNECTIONS; i++)
  {
    FMT_S_MAT_TP(fmt,i) = 0;
    FMT_CONN_DEPTH_TP(fmt,i) = 0;
  }
  for (i=FROM_VTNAME; i<=TO_VTNAME; i++)
    FMT_SET_N2T(fmt,i,NOVTYPE);
  MaxDepth = NeighborhoodDepth = 0;

  /* init po2t */
  INT po2t[MAXDOMPARTS][MAXVOBJECTS];
  for (INT i=0; i<MAXDOMPARTS; i++)
    for (INT j=0; j<MAXVOBJECTS; j++)
      po2t[i][j] = NOVTYPE;

#ifdef __THREEDIM__
  po2t[0][3] = SIDEVEC;
#endif

  SHORT MatStorageNeeded[MAXCONNECTIONS];
  for (type=0; type<MAXCONNECTIONS; type++)
    MatStorageNeeded[type] = 0;

  /* fill connections needed */
  MatrixDescriptor mDesc[MAXMATRICES*MAXVECTORS];
  INT nmDesc = 0;
  for (type=0; type<MAXCONNECTIONS; type++)
  {
    INT rtype = (((type)<MAXMATRICES) ? (type)/MAXVECTORS : (type)%MAXVECTORS);
    INT ctype = ((type)%MAXVECTORS);

    INT size = MatStorageNeeded[type];

    if (ctype==rtype)
    {
      /* ensure diag/matrix coexistence (might not be necessary) */
#define MTP(rt,ct)          ((rt)*MAXVECTORS+(ct))
#define DMTP(rt)            (MAXMATRICES+rt)
      type2=(type<MAXMATRICES) ? DMTP(rtype) : MTP(rtype,rtype);
      if ((size<=0) && (MatStorageNeeded[type2]<=0)) continue;
    }
    else
    {
      /* ensure symmetry of the matrix graph */
      type2=MTP(ctype,rtype);
      if ((size<=0) && (MatStorageNeeded[type2]<=0)) continue;
    }

    mDesc[nmDesc].from  = rtype;
    mDesc[nmDesc].to    = ctype;
    mDesc[nmDesc].diag  = (type>=MAXMATRICES);
    mDesc[nmDesc].size  = size*sizeof(DOUBLE);
    mDesc[nmDesc].depth = 0;
    nmDesc++;
  }


  /* set vector stuff */
#ifdef __THREEDIM__
  FMT_S_VEC_TP(fmt,SIDEVEC) = sizeof(DOUBLE);
  FMT_VTYPE_NAME(fmt,SIDEVEC) = 's';
  FMT_SET_N2T(fmt,'S',SIDEVEC);
  FMT_T2N(fmt,SIDEVEC) = 's';
#endif

  /* copy part,obj to type table and derive t2p, t2o lists */
  for (type=0; type<MAXVECTORS; type++)
    FMT_T2P(fmt,type) = FMT_T2O(fmt,type) = 0;
  for (part=0; part<MAXDOMPARTS; part++)
    for (obj=0; obj<MAXVOBJECTS; obj++)
    {
      type = FMT_PO2T(fmt,part,obj) = po2t[part][obj];
      FMT_T2P(fmt,type) |= (1<<part);
      FMT_T2O(fmt,type) |= (1<<obj);
    }

  /* set connection stuff */
  for (i=0; i<nmDesc; i++)
  {
    if ((mDesc[i].from<0)||(mDesc[i].from>=MAXVECTORS)) REP_ERR_RETURN_PTR(NULL);
    if ((mDesc[i].to<0)  ||(mDesc[i].to>=MAXVECTORS)) REP_ERR_RETURN_PTR(NULL);
    if (mDesc[i].diag<0) REP_ERR_RETURN_PTR(NULL);
    if ((mDesc[i].size<0)||(mDesc[i].depth<0)) REP_ERR_RETURN_PTR(NULL);

    if (FMT_S_VEC_TP(fmt,mDesc[i].from)<=0) REP_ERR_RETURN_PTR(NULL);
    if (FMT_S_VEC_TP(fmt,mDesc[i].to)<=0) REP_ERR_RETURN_PTR(NULL);

    if (mDesc[i].size>0 && mDesc[i].depth>=0)
    {
      if (mDesc[i].from==mDesc[i].to)
      {
        /* set data (ensuring that size(diag) >= size(off-diag) */
        if (mDesc[i].diag)
        {
          type=DIAGMATRIXTYPE(mDesc[i].from);
          type2=MATRIXTYPE(mDesc[i].from,mDesc[i].from);
          if (mDesc[i].size>=FMT_S_MAT_TP(fmt,type2))
            FMT_S_MAT_TP(fmt,type) = mDesc[i].size;
          else
            FMT_S_MAT_TP(fmt,type) = FMT_S_MAT_TP(fmt,type2);
        }
        else
        {
          type=MATRIXTYPE(mDesc[i].from,mDesc[i].from);
          FMT_S_MAT_TP(fmt,type) = mDesc[i].size;
          type2=DIAGMATRIXTYPE(mDesc[i].from);
          if (mDesc[i].size>=FMT_S_MAT_TP(fmt,type2))
            FMT_S_MAT_TP(fmt,type2) = mDesc[i].size;
        }
      }
      else
      {
        /* set data (ensuring size symmetry, which is needed at the moment) */
        type=MATRIXTYPE(mDesc[i].from,mDesc[i].to);
        FMT_S_MAT_TP(fmt,type) = mDesc[i].size;
        type2 = MATRIXTYPE(mDesc[i].to,mDesc[i].from);
        if (mDesc[i].size>FMT_S_MAT_TP(fmt,type2))
          FMT_S_MAT_TP(fmt,type2) = mDesc[i].size;
      }
    }
    /* set connection depth information */
    FMT_CONN_DEPTH_TP(fmt,type) = mDesc[i].depth;
    MaxDepth = MAX(MaxDepth,mDesc[i].depth);
    if ((FMT_TYPE_USES_OBJ(fmt,mDesc[i].from,ELEMVEC))&&(FMT_TYPE_USES_OBJ(fmt,mDesc[i].to,ELEMVEC)))
      NeighborhoodDepth = MAX(NeighborhoodDepth,mDesc[i].depth);
    else
      NeighborhoodDepth = MAX(NeighborhoodDepth,mDesc[i].depth+1);

  }
  FMT_CONN_DEPTH_MAX(fmt) = MaxDepth;
  FMT_NB_DEPTH(fmt)           = NeighborhoodDepth;

  /* derive additional information */
  for (i=0; i<MAXVOBJECTS; i++) FMT_USES_OBJ(fmt,i) = false;
  FMT_MAX_PART(fmt) = 0;
  MaxType = 0;
  for (i=0; i<MAXDOMPARTS; i++)
    for (j=0; j<MAXVOBJECTS; j++)
      if (po2t[i][j]!=NOVTYPE)
      {
        FMT_USES_OBJ(fmt,j) = true;
        FMT_MAX_PART(fmt) = MAX(FMT_MAX_PART(fmt),i);
        MaxType = MAX(MaxType,po2t[i][j]);
      }
  FMT_MAX_TYPE(fmt) = MaxType;

  return fmt;
}


/****************************************************************************/
/*D
   InitEnrol - Create and initialize the environment

   SYNOPSIS:
   INT InitEnrol ();

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function creates the environment

   RETURN VALUE:
   INT
   .n   0 if ok
   .n   1 when error occured.
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX InitEnrol ()
{
  /* install the /Formats directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitEnrol","could not changedir to root");
    return(__LINE__);
  }
  theSymbolVarID = GetNewEnvVarID();

  return (GM_OK);
}
