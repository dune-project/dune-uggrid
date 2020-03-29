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
#include <dune/uggrid/numerics/udm.h>

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

static INT theFormatDirID;                      /* env type for Format dir				*/
static INT theSymbolVarID;                      /* env type for Format vars                     */

REP_ERR_FILE

/****************************************************************************/
/*D
   CreateFormat	- Create a new FORMAT structure in the environment

   SYNOPSIS:
   FORMAT *CreateFormat (const char *name, INT nvDesc, VectorDescriptor *vDesc,
                INT nmDesc, MatrixDescriptor *mDesc, INT po2t[MAXDOMPARTS][MAXVOBJECTS]);

   PARAMETERS:
   .  nvDesc - number of vector descriptors
   .  vDesc - pointer to vector descriptor
   .  nmDesc - number of matrix desciptors
   .  mDesc - pointer to matrix descriptor
   .  ImatTypes - size of interpolation matrices
   .  po2t  - table (part,obj) --> vtype, NOVTYPE if not defined
   .  edata - size of edge data

   DESCRIPTION:
   This function allocates and initializes a new FORMAT structure in the environment.
   The parameters vDesc and mDesc are pointers to structures which describe the
   VECTOR or MATRIX types used.
   VectorDescriptor is defined as
   .vb
          typedef struct {
          int tp;
          int size;
          ConversionProcPtr print;
          } VectorDescriptor ;
   .ve
        The components have the following meaning

   .   tp - this is just an abstract vector type
   .   size - the data size of a VECTOR structure on this position in bytes
   .   print - pointer to a function which is called for printing the contents of the data
                        fields.

        MatrixDescriptor has the definition
   .vb
           typedef struct {
           int from;
           int to;
           int size;
           int depth;
           ConversionProcPtr print;
           } MatrixDescriptor ;
   .ve
        The meaning of the components is

   .   from - this connection goes from vtype
   .   to - to vtype
   .   size - this defines the size in bytes per connection
   .   depth - this connection has the depth defined here
   .   print - function to print the data.

   EXAMPLES:
   A small example to create a format looks like the following. In this format only
   vectors in nodes are used and therfore all connections connect two nodevectors.
   .vb
   HRR_TODO: man page for CreateFormat:
      // we need dofs only in nodes
      vd[0].tp    = NODEVEC;
      vd[0].size  = 3*sizeof(DOUBLE);
      vd[0].print = Print_3_NodeVectorData;

      // and the following connection: node-node
      md[0].from  = NODEVEC;
      md[0].to    = NODEVEC;
      md[0].size  = sizeof(DOUBLE);
      md[0].depth = 0;
      md[0].print = Print_1_NodeNodeMatrixData;

      newFormat = CreateFormat("full scalar",0,0,
                  (ConversionProcPtr)NULL,(ConversionProcPtr)NULL,
                  (ConversionProcPtr)NULL,1,vd,1,md,po2t);
   .ve

   RETURN VALUE:
   FORMAT *
   .n     pointer to FORMAT
   .n     NULL if out of memory.
   D*/
/****************************************************************************/

FORMAT * NS_DIM_PREFIX CreateFormat (INT nvDesc, VectorDescriptor *vDesc, /*INT nmDesc, MatrixDescriptor *mDesc,*/
                                     SHORT ImatTypes[])
{
  FORMAT *fmt;
  INT i, j, type, type2, part, obj, MaxDepth, NeighborhoodDepth, MaxType;

  std::string name = "DuneFormat" + std::to_string(DIM) + "d";

  /* change to /Formats directory */
  if (ChangeEnvDir("/Formats")==NULL)
    REP_ERR_RETURN_PTR (NULL);

  /* allocate new format structure */
  fmt = (FORMAT *) MakeEnvItem (name.c_str(),theFormatDirID,sizeof(FORMAT));
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

  SHORT MatStorageNeeded[NMATTYPES];
  for (type=0; type<NMATTYPES; type++)
    MatStorageNeeded[type] = 0;

  /* fill connections needed */
  MatrixDescriptor mDesc[MAXMATRICES*MAXVECTORS];
  INT nmDesc = 0;
  for (type=0; type<NMATTYPES; type++)
  {
    INT rtype = MTYPE_RT(type);
    INT ctype = MTYPE_CT(type);

    INT size = MatStorageNeeded[type];

    if (ctype==rtype)
    {
      /* ensure diag/matrix coexistence (might not be necessary) */
      type2=(type<NMATTYPES_NORMAL) ? DMTP(rtype) : MTP(rtype,rtype);
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
    mDesc[nmDesc].diag  = (type>=NMATTYPES_NORMAL);
    mDesc[nmDesc].size  = size*sizeof(DOUBLE);
    mDesc[nmDesc].depth = 0;
    nmDesc++;
  }


  /* set vector stuff */
  for (i=0; i<nvDesc; i++)
  {
    if ((vDesc[i].tp<0)||(vDesc[i].tp>=MAXVECTORS)||(vDesc[i].size<0)) REP_ERR_RETURN_PTR(NULL);
    FMT_S_VEC_TP(fmt,vDesc[i].tp) = vDesc[i].size;
    if ((vDesc[i].name<FROM_VTNAME) || (TO_VTNAME<vDesc[i].name))
    {
      PrintErrorMessageF('E',"CreateFormat","type name '%c' out of range (%c-%c)",vDesc[i].name,FROM_VTNAME,TO_VTNAME);
      REP_ERR_RETURN_PTR (NULL);
    }
    FMT_VTYPE_NAME(fmt,vDesc[i].tp) = vDesc[i].name;
    FMT_SET_N2T(fmt,vDesc[i].name,vDesc[i].tp);
    FMT_T2N(fmt,vDesc[i].tp) = vDesc[i].name;
  }

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

  if (ChangeEnvDir(name.c_str())==NULL) REP_ERR_RETURN_PTR(NULL);
  UserWrite("format "); UserWrite(name.c_str()); UserWrite(" installed\n");

  return(fmt);
}

/****************************************************/
/*D
   DeleteFormat - remove previously enroled format

   SYNOPSIS:
   INT DeleteFormat (const char *name)

   PARAMETERS:
   .  name - name of the format

   DESCRIPTION:
   This function removes the specified format.

   RETURN VALUE:
   INT
   .n   GM_OK if removed or non existent
   .n   GM_ERROR if an error occured
   D*/
/****************************************************/

INT NS_DIM_PREFIX DeleteFormat (const char *name)
{
  FORMAT *fmt;

  fmt = GetFormat(name);
  if (fmt==NULL)
  {
    PrintErrorMessageF('W',"DeleteFormat","format '%s' doesn't exist",name);
    return (GM_OK);
  }

  if (ChangeEnvDir("/Formats")==NULL)
    REP_ERR_RETURN (GM_ERROR);
  ENVITEM_LOCKED(fmt) = 0;
  if (RemoveEnvDir((ENVITEM *)fmt))
    REP_ERR_RETURN (GM_ERROR);

  return (GM_OK);
}

/****************************************************/
/*D
   GetFormat - Get a format pointer from the environment

   PARAMETERS:
   .  name - name of the format

   DESCRIPTION:
   This function searches the directory /Formats for a format

   RETURN VALUE:
   FORMAT *
   .n   pointer to FORMAT
   .n   NULL  if not found or error.
   D*/
/****************************************************/

FORMAT* NS_DIM_PREFIX GetFormat (const char *name)
{
  return((FORMAT *) SearchEnv(name,"/Formats",theFormatDirID,theFormatDirID));
}

/****************************************************************************/
/*D
   GetFirstFormat - Get first format definition

   SYNOPSIS:
   FORMAT *GetFirstFormat (void);

   PARAMETERS:
   .  void - none

   DESCRIPTION:
   This function returns the first format definition.

   RETURN VALUE:
   FORMAT *
   .n     pointer to a FORMAT
   .n     NULL if not found or error.
   D*/
/****************************************************************************/

FORMAT * NS_DIM_PREFIX GetFirstFormat (void)
{
  ENVITEM *fmt;

  if ((fmt=(ENVITEM*)ChangeEnvDir("/Formats")) == NULL) return (NULL);

  for (fmt=ENVITEM_DOWN(fmt); fmt!=NULL; fmt=NEXT_ENVITEM(fmt))
    if (ENVITEM_TYPE(fmt) == theFormatDirID)
      return ((FORMAT*)fmt);
  return (NULL);
}

/****************************************************************************/
/*D
   GetNextFormat - Get next format definition

   SYNOPSIS:
   FORMAT *GetNextFormat (void);

   PARAMETERS:
   .  fmt - predecessor format

   DESCRIPTION:
   This function returns the next format definition following the specified one.

   RETURN VALUE:
   FORMAT *
   .n     pointer to a FORMAT
   .n     NULL if not found or error.
   D*/
/****************************************************************************/

FORMAT * NS_DIM_PREFIX GetNextFormat (FORMAT *fmt)
{
  ENVITEM *nextfmt;

  if (fmt == NULL) return (NULL);

  for (nextfmt=NEXT_ENVITEM(fmt); nextfmt!=NULL; nextfmt=NEXT_ENVITEM(nextfmt))
    if (ENVITEM_TYPE(nextfmt) == theFormatDirID)
      return ((FORMAT*)nextfmt);
  return (NULL);
}

/****************************************************/
/*D
   ChangeToFormatDir - change to format directory with name

   SYNOPSIS:
   INT ChangeToFormatDir (const char *name)

   PARAMETERS:
   .  name - name of the format

   DESCRIPTION:
   This function changes to the format directory with name.

   RETURN VALUE:
   INT
   .n   0: ok
   .n   1: could not change to /Formats/<name> dir
   D*/
/****************************************************/

INT NS_DIM_PREFIX ChangeToFormatDir (const char *name)
{
  if (ChangeEnvDir("/Formats")==NULL)
    REP_ERR_RETURN (1);
  if (ChangeEnvDir(name)==NULL)
    REP_ERR_RETURN (2);

  return (0);
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
  theFormatDirID = GetNewEnvDirID();
  if (MakeEnvItem("Formats",theFormatDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitEnrol","could not install '/Formats' dir");
    return(__LINE__);
  }
  theSymbolVarID = GetNewEnvVarID();

  return (GM_OK);
}
