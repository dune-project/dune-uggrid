// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      formats.c                                                     */
/*                                                                          */
/* Purpose:   definition of user data and symbols                           */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   27.03.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>

#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/enrol.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/scan.h> // for ReadArgvChar
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugstruct.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/numerics/formats.h>


USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static char default_type_names[MAXVECTORS];

/** @name Environment dir and var ids */
/*@{*/
static INT theNewFormatDirID;                   /* env type for NewFormat dir           */
static INT theVecVarID;                                 /* env type for VEC_TEMPLATE vars       */
static INT theMatVarID;                                 /* env type for MAT_TEMPLATE vars       */
/*@}*/

REP_ERR_FILE


static INT RemoveTemplateSubs (FORMAT *fmt)
{
  ENVITEM *item;
  VEC_TEMPLATE *vt;
  MAT_TEMPLATE *mt;
  INT i;

  for (item=ENVITEM_DOWN(fmt); item != NULL; item = NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == theVecVarID)
    {
      vt = (VEC_TEMPLATE*) item;

      for (i=0; i<VT_NSUB(vt); i++)
        if (VT_SUB(vt,i)!=NULL)
          FreeEnvMemory(VT_SUB(vt,i));
      VT_NSUB(vt) = 0;
    }
    else if (ENVITEM_TYPE(item) == theMatVarID)
    {
      mt = (MAT_TEMPLATE*) item;

      for (i=0; i<MT_NSUB(mt); i++)
        if (MT_SUB(mt,i)!=NULL)
          FreeEnvMemory(MT_SUB(mt,i));
      MT_NSUB(mt) = 0;
    }
  return (0);
}


static INT CleanupTempDir (void)
{
  ENVDIR *dir;

  dir = ChangeEnvDir("/newformat");
  if (dir == NULL) {
    PrintErrorMessage('E',"CleanupTempDir","/newformat does not exist");
    REP_ERR_RETURN (1);
  }

  if (RemoveTemplateSubs((FORMAT *) dir))
    REP_ERR_RETURN (1);

  ChangeEnvDir("/");
  ENVITEM_LOCKED(dir) = 0;
  if (RemoveEnvDir((ENVITEM *) dir))
    REP_ERR_RETURN (1);

  return (0);
}

INT NS_DIM_PREFIX CreateFormatCmd (INT argc, char **argv)
{
  ENVDIR *dir;
  VectorDescriptor vd[MAXVECTORS];
  INT type;
  SHORT ImatTypes[NVECTYPES];
  SHORT VecStorageNeeded[NVECTYPES];
  char TypeNames[NVECTYPES];

  std::string formatname = "DuneFormat" + std::to_string(DIM) + "d";

  if (GetFormat(formatname.c_str()) != nullptr) {
    PrintErrorMessage('W',"newformat","format already exists");
    return 0;
  }

  /* install the /newformat directory */
  if (ChangeEnvDir("/")==NULL) {
    PrintErrorMessage('F',"InitFormats","could not changedir to root");
    REP_ERR_RETURN(__LINE__);
  }
  if (MakeEnvItem("newformat",theNewFormatDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitFormats",
                      "could not install '/newformat' dir");
    REP_ERR_RETURN(__LINE__);
  }

  /* init */
  for (type=0; type<NVECTYPES; type++)
    ImatTypes[type] = VecStorageNeeded[type] = 0;
  for (INT j=0; j<MAXVOBJECTS; j++)
    TypeNames[j] = default_type_names[j];

  /* scan other options */
#ifdef __THREEDIM__
  if (ChangeEnvDir("/newformat")==nullptr)
    abort();

  VEC_TEMPLATE* vt = (VEC_TEMPLATE *) MakeEnvItem ("vt",theVecVarID,sizeof(VEC_TEMPLATE));
  VT_NSUB(vt) = 0;
  VT_NID(vt) = NO_IDENT;
  const char* token = DEFAULT_NAMES;
  for (size_t j=0; j<MAX(MAX_VEC_COMP,strlen(DEFAULT_NAMES)); j++)
    VT_COMPNAME(vt,j) = token[j];

  if (vt == NULL) {
    PrintErrorMessageF('E',"newformat",
                       "could not allocate environment storage");
    REP_ERR_RETURN (1);
  }

  /* read types and sizes */
  for (INT type=0; type<NVECTYPES; type++)
    VT_COMP(vt,type) = 0;

  char tp='s';
  for (type=0; type<MAXVOBJECTS; type++)
    if (tp==TypeNames[type])
      break;
  if (type>=MAXVOBJECTS)
  {
    PrintErrorMessageF('E',"newformat","no valid type name '%c'",tp);
    REP_ERR_RETURN (1);
  }
  if (VT_COMP(vt,type) !=0 ) {
    PrintErrorMessageF('E',"newformat",
                       "double vector type specification");
    REP_ERR_RETURN (1);
  }
  VT_COMP(vt,type) = 1;

  /* compute storage needed */
  for (INT type=0; type<NVECTYPES; type++)
    VecStorageNeeded[type] += 1 * VT_COMP(vt,type);
#endif

  /* fill degrees of freedom needed */
  INT nvd = 0;
  for (type=0; type<NVECTYPES; type++)
    if (VecStorageNeeded[type]>0)
    {
      vd[nvd].tp    = type;
      vd[nvd].size  = VecStorageNeeded[type]*sizeof(DOUBLE);
      vd[nvd].name  = TypeNames[type];
      nvd++;
    }

  /* create format */
  FORMAT* newFormat = CreateFormat(nvd,vd,ImatTypes);
  if (newFormat==NULL)
  {
    PrintErrorMessage('E',"newformat","failed creating the format");
    REP_ERR_RETURN (CleanupTempDir());
  }

  /* move templates into the new directory */
  dir = ChangeEnvDir("/newformat");
  if (dir == NULL) {
    PrintErrorMessage('E',"newformat","failed moving template");
    REP_ERR_RETURN (4);
  }
  if (ENVITEM_DOWN((ENVDIR *)newFormat) != NULL) {
    PrintErrorMessage('E',"newformat","failed moving template");
    REP_ERR_RETURN (4);
  }
  ENVITEM_DOWN((ENVDIR *)newFormat) = ENVITEM_DOWN(dir);
  ENVITEM_DOWN(dir) = NULL;
  ENVITEM_LOCKED(dir) = 0;
  ChangeEnvDir("/");
  if (RemoveEnvDir((ENVITEM *)dir))
    PrintErrorMessage('W',"InitFormats","could not remove newformat dir");

  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function:  InitFormats	                                                */
/*                                                                          */
/* Purpose:   calls all inits of format definitions                         */
/*                                                                          */
/* Input:     none                                                          */
/*                                                                          */
/* Output:    INT 0: everything ok                                          */
/*            INT 1: fatal error (not enough env. space, file not found...  */
/*                                                                          */
/****************************************************************************/

INT NS_DIM_PREFIX InitFormats ()
{
  INT tp;

  theNewFormatDirID = GetNewEnvDirID();
  theVecVarID = GetNewEnvVarID();
  theMatVarID = GetNewEnvVarID();

  if (MakeStruct(":SparseFormats")!=0) return(__LINE__);

  /* init default type names */
  for (tp=0; tp<MAXVECTORS; tp++)
    switch (tp) {
    case NODEVEC : default_type_names[tp] = 'n'; break;
    case EDGEVEC : default_type_names[tp] = 'k'; break;
    case ELEMVEC : default_type_names[tp] = 'e'; break;
    case SIDEVEC : default_type_names[tp] = 's'; break;
    default :
      PrintErrorMessage('E',"newformat","Huh");
      return (__LINE__);
    }

  return (0);
}
