// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      formats.c                                                     */
/*                                                                          */
/* Purpose:   definition of user data and symbols                           */
/*                                                                          */
/* Author:	  Henrik Rentz-Reichert                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de	                                                */
/*																			*/
/* History:   27.03.95 begin, ug version 3.0								*/
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "devices.h"
#include "enrol.h"
#include "compiler.h"
#include "misc.h"
#include "gm.h"
#include "ugenv.h"
#include "ugm.h"
#include "algebra.h"
#include "helpmsg.h"
#include "general.h"

#include "np.h"

#include "formats.h"

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

/* limits for XDATA_DESC handling */
#define MAX_SUB                         5
#define SYMNAMESIZE                     16

#define MAX_PRINT_SYM           5

/* format for PrintVectorData and PrintMatrixData */
#define VFORMAT                         " %c=%11.4lE"
#define MFORMAT                         " %c%c=%11.4lE"

/* seperators */
#define NAMESEP                         ':'
#define BLANKS                          " \t"

/* macros for SUBVEC */
#define SUBV_NAME(s)            ((s)->Name)
#define SUBV_NCOMPS(s)          ((s)->Comp)
#define SUBV_NCOMP(s,tp)        ((s)->Comp[tp])
#define SUBV_COMP(s,tp,i)       ((s)->Comps[tp][i])

/* macros for SUBMAT */
#define SUBM_NAME(s)            ((s)->Name)
#define SUBM_RCOMPS(s)          ((s)->RComp)
#define SUBM_CCOMPS(s)          ((s)->CComp)
#define SUBM_RCOMP(s,tp)        ((s)->RComp[tp])
#define SUBM_CCOMP(s,tp)        ((s)->CComp[tp])
#define SUBM_COMP(s,tp,i)       ((s)->Comps[tp][i])

/* macros for VEC_FORMAT */
#define VF_COMPS(vf)            ((vf)->Comp)
#define VF_COMP(vf,tp)          ((vf)->Comp[tp])
#define VF_COMPNAMES(vf)        ((vf)->CompNames)
#define VF_COMPNAME(vf,i)       ((vf)->CompNames[i])
#define VF_SUB(vf,i)            ((vf)->SubVec[i])
#define VF_NSUB(vf)                     ((vf)->nsub)

/* macros for MAT_FORMAT */
#define MF_RCOMPS(mf)           ((mf)->RComp)
#define MF_RCOMP(mf,tp)         ((mf)->RComp[tp])
#define MF_CCOMPS(mf)           ((mf)->CComp)
#define MF_CCOMP(mf,tp)         ((mf)->CComp[tp])
#define MF_COMPNAMES(mf)        ((mf)->CompNames)
#define MF_COMPNAME(mf,i)       ((mf)->CompNames[i])
#define MF_SUB(mf,i)            ((mf)->SubMat[i])
#define MF_NSUB(mf)                     ((mf)->nsub)

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

typedef struct {

  char Name[SYMNAMESIZE];
  SHORT Comp[NVECTYPES];
  SHORT Comps[NVECTYPES][MAX_VEC_COMP];

} SUBVEC;

typedef struct {

  char Name[SYMNAMESIZE];
  SHORT RComp[NMATTYPES];
  SHORT CComp[NMATTYPES];
  SHORT Comps[NMATTYPES][MAX_MAT_COMP];

} SUBMAT;

typedef struct {

  ENVITEM v;

  SHORT Comp[NVECTYPES];
  char CompNames[MAX_VEC_COMP];

  SHORT nsub;
  SUBVEC  *SubVec[MAX_SUB];

} VEC_FORMAT;

typedef struct {

  ENVITEM v;

  SHORT RComp[NMATTYPES];
  SHORT CComp[NMATTYPES];
  char CompNames[2*MAX_MAT_COMP];

  SHORT nsub;
  SUBMAT  *SubMat[MAX_SUB];

} MAT_FORMAT;

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/* printing routine ptrs */
static ConversionProcPtr PrintVectorDataPtr[NVECTYPES];
static ConversionProcPtr PrintMatrixDataPtr[NMATTYPES];

/* print symbol counters and lists */
static INT NPrintVectors=0;
static INT NPrintMatrixs=0;
static VECDATA_DESC *PrintVector[MAX_PRINT_SYM];
static MATDATA_DESC *PrintMatrix[MAX_PRINT_SYM];

/* environment dir and var ids */
static INT theNewFormatDirID;                   /* env type for NewFormat dir           */
static INT theVecVarID;                                 /* env type for VEC_FORMAT vars         */
static INT theMatVarID;                                 /* env type for MAT_FORMAT vars         */

REP_ERR_FILE;

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);


/****************************************************************************/
/*																			*/
/* functions to set, display and change the printing format			                */
/*																			*/
/****************************************************************************/

INT DisplayPrintingFormat ()
{
  INT i;

  if (NPrintVectors==0)
    UserWrite("no vector symbols printed\n");
  else
  {
    UserWrite("printed vector symbols\n");
    for (i=0; i<NPrintVectors; i++)
      UserWriteF("   '%s'\n",ENVITEM_NAME(PrintVector[i]));
  }

  if (NPrintMatrixs==0)
    UserWrite("\nno matrix symbols printed\n");
  else
  {
    UserWrite("\nprinted matrix symbols\n");
    for (i=0; i<NPrintMatrixs; i++)
      UserWriteF("   '%s'\n",ENVITEM_NAME(PrintMatrix[i]));
  }

  return (NUM_OK);
}

/********************************************************/
/* for the following function							*/
/* please keep help comment in commands.c up to date	*/
/********************************************************/

INT SetPrintingFormatCmd (const MULTIGRID *mg, INT argc, char **argv)
{
  VECDATA_DESC *vd;
  MATDATA_DESC *md;
  INT i,j,add,vec;
  char *token,buffer[64];

  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'V' :
    case 'M' :
      if (strchr("0+-",argv[i][1])==NULL)
      {
        PrintErrorMessage('E',"setpf","specify 0,+ or - after V or M option");
        REP_ERR_RETURN (1);
      }
      vec = (argv[i][0]=='V');
      if (argv[i][1]=='0')
      {
        if (vec)
          NPrintVectors = 0;
        else
          NPrintMatrixs = 0;
        break;
      }
      add = (argv[i][1]=='+');
      token = strtok(argv[i]+1,BLANKS);                                 /* forget about first token (0,+ or -) */
      while ((token=strtok(NULL,BLANKS))!=NULL)
      {
        if (add)
        {
          if (vec)
          {
            if (NPrintVectors>=MAX_PRINT_SYM)
            {
              PrintErrorMessage('E',"setpf","max number of print vetor symbols exceeded");
              REP_ERR_RETURN (1);
            }
            for (j=0; j<NPrintVectors; j++)
              if (strcmp(token,ENVITEM_NAME(PrintVector[j]))==0)
                break;
            if (j<NPrintVectors) continue;                                                      /* already in list */
            if ((vd=GetVecDataDescByName(mg,token))==NULL)
            {
              PrintErrorMessage('E',"setpf","vector symbol not found");
              REP_ERR_RETURN (1);
            }
            PrintVector[NPrintVectors++] = vd;
          }
          else
          {
            if (NPrintMatrixs>=MAX_PRINT_SYM)
            {
              PrintErrorMessage('E',"setpf","max number of print vetor symbols exceeded");
              REP_ERR_RETURN (1);
            }
            for (j=0; j<NPrintMatrixs; j++)
              if (strcmp(token,ENVITEM_NAME(PrintMatrix[j]))==0)
                break;
            if (j<NPrintMatrixs) continue;                                                      /* already in list */
            if ((md=GetMatDataDescByName(mg,token))==NULL)
            {
              PrintErrorMessage('E',"setpf","matrix symbol not found");
              REP_ERR_RETURN (1);
            }
            PrintMatrix[NPrintMatrixs++] = md;
          }
        }
        else
        {
          if (vec)
          {
            for (j=0; j<NPrintVectors; j++)
              if (strcmp(token,ENVITEM_NAME(PrintVector[j]))==0)
                break;
            if (j==NPrintVectors)
            {
              PrintErrorMessage('W',"setpf","vector symbol not in list");
              continue;
            }
            for (j++; j<NPrintVectors; j++)
              PrintVector[j-1] = PrintVector[j];
            NPrintVectors--;
          }
          else
          {
            for (j=0; j<NPrintMatrixs; j++)
              if (strcmp(token,ENVITEM_NAME(PrintMatrix[j]))==0)
                break;
            if (j==NPrintMatrixs)
            {
              PrintErrorMessage('W',"setpf","matrix symbol not in list");
              continue;
            }
            for (j++; j<NPrintMatrixs; j++)
              PrintMatrix[j-1] = PrintMatrix[j];
            NPrintMatrixs--;
          }
        }
      }
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintErrorMessage('E',"setpf",buffer);
      REP_ERR_RETURN (1);
    }

  DisplayPrintingFormat();

  return (NUM_OK);
}

static char *DisplayVecDD (const VECDATA_DESC *vd, INT type, const DOUBLE *data, const char *indent, char *s)
{
  INT i,n,off;

  n = VD_NCMPS_IN_TYPE(vd,type);
  if (n==0) return (s);

  off = VD_OFFSET(vd,type);

  s += sprintf(s,"%s%s:",indent,ENVITEM_NAME(vd));
  for (i=0; i<n; i++)
    s += sprintf(s,VFORMAT,VM_COMP_NAME(vd,off+i),data[VD_CMP_OF_TYPE(vd,type,i)]);

  *(s++) = '\n';

  return (s);
}

/****************************************************************************/
/*D
        PrintTypeVectorData - print selected vector user data for the 'nsr' format

        SYNOPSIS:
        static INT PrintTypeVectorData (INT type, void *data, const char *indent, char *s)

    PARAMETERS:
   .   type - consider only this type
   .   data - user data
   .   indent - is printed at the beginning of lines
   .   s - output string

        DESCRIPTION:
        Print selected vector user data for the 'nsr' format.

        RETURN VALUE:
        INT
   .n   0: ok

        SEE ALSO:
        setformat, showformat
   D*/
/****************************************************************************/

static INT PrintTypeVectorData (INT type, void *data, const char *indent, char *s)
{
  INT i;

  for (i=0; i<NPrintVectors; i++)
    s = DisplayVecDD(PrintVector[i],type,data,indent,s);

  /* remove last \n */
  *s = '\0';

  return(0);
}

static INT PrintNodeVectorData (void *data, const char *indent, char *s)
{
  return (PrintTypeVectorData(NODEVECTOR,data,indent,s));
}

static INT PrintElemVectorData (void *data, const char *indent, char *s)
{
  return (PrintTypeVectorData(ELEMVECTOR,data,indent,s));
}

static INT PrintEdgeVectorData (void *data, const char *indent, char *s)
{
  return (PrintTypeVectorData(EDGEVECTOR,data,indent,s));
}

#ifdef __THREEDIM__
static INT PrintSideVectorData (void *data, const char *indent, char *s)
{
  return (PrintTypeVectorData(SIDEVECTOR,data,indent,s));
}
#endif

static char *DisplayMatDD (const MATDATA_DESC *md, INT type, const DOUBLE *data, const char *indent, char *s)
{
  INT i,j,nr,nc,off;

  nr = MD_ROWS_IN_MTYPE(md,type);
  nc = MD_COLS_IN_MTYPE(md,type);
  if (nr==0) return (s);

  off = MD_MTYPE_OFFSET(md,type);

  for (i=0; i<nr; i++)
  {
    s += sprintf(s,"%s%s:",indent,ENVITEM_NAME(md));
    for (j=0; j<nc; j++)
      s += sprintf(s,MFORMAT,VM_COMP_NAME(md,2*(off+i*nc+j)),VM_COMP_NAME(md,2*(off+i*nc+j)+1),data[MD_IJ_CMP_OF_MTYPE(md,type,i,j)]);
    *(s++) = '\n';
  }

  return (s);
}

/****************************************************************************/
/*D
        PrintTypeMatrixData - print selected matrix user data for the 'nsr' format

        SYNOPSIS:
        static INT PrintTypeMatrixData (INT type, void *data, const char *indent, char *s)

    PARAMETERS:
   .   type - consider this mat type
   .   data - user data
   .   indent - is printed at the beginning of lines
   .   s - output string

        DESCRIPTION:
        Print selected matrix user data for the 'nsr' format.

        RETURN VALUE:
        INT
   .n   0: ok

        SEE ALSO:
        setformat, showformat
   D*/
/****************************************************************************/

static INT PrintTypeMatrixData (INT type, void *data, const char *indent, char *s)
{
  INT i;

  for (i=0; i<NPrintMatrixs; i++)
    s = DisplayMatDD(PrintMatrix[i],type,data,indent,s);

  /* remove last \n */
  *s = '\0';

  return(0);
}

static INT PrintNodeNodeMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(NODEVECTOR,NODEVECTOR),data,indent,s));
}

static INT PrintNodeElemMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(NODEVECTOR,ELEMVECTOR),data,indent,s));
}

static INT PrintNodeEdgeMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(NODEVECTOR,EDGEVECTOR),data,indent,s));
}

#ifdef __THREEDIM__
static INT PrintNodeSideMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(NODEVECTOR,SIDEVECTOR),data,indent,s));
}
#endif

static INT PrintElemElemMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(ELEMVECTOR,ELEMVECTOR),data,indent,s));
}

static INT PrintElemEdgeMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(ELEMVECTOR,EDGEVECTOR),data,indent,s));
}

#ifdef __THREEDIM__
static INT PrintElemSideMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(ELEMVECTOR,SIDEVECTOR),data,indent,s));
}
#endif

static INT PrintEdgeEdgeMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(EDGEVECTOR,EDGEVECTOR),data,indent,s));
}

#ifdef __THREEDIM__
static INT PrintEdgeSideMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(EDGEVECTOR,SIDEVECTOR),data,indent,s));
}

static INT PrintSideSideMatrixData (void *data, const char *indent, char *s)
{
  return (PrintTypeMatrixData(MTP(SIDEVECTOR,SIDEVECTOR),data,indent,s));
}
#endif


static VEC_FORMAT *GetVectorTemplate (MULTIGRID *theMG, char *template)
{
  ENVITEM *item,*dir;

  if (ChangeEnvDir("/Formats") == NULL) REP_ERR_RETURN (NULL);
  dir = (ENVITEM *)ChangeEnvDir(ENVITEM_NAME(MGFORMAT(theMG)));
  if (dir == NULL) REP_ERR_RETURN (NULL);
  if (template != NULL)
    for (item=ENVITEM_DOWN(dir); item != NULL; item = NEXT_ENVITEM(item))
      if (ENVITEM_TYPE(item) == theVecVarID)
        if (strcmp(ENVITEM_NAME(item),template)==0)
          return ((VEC_FORMAT *)item);
  for (item=ENVITEM_DOWN(dir); item != NULL; item = NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == theVecVarID)
      return ((VEC_FORMAT *)item);

  REP_ERR_RETURN (NULL);
}

VECDATA_DESC *CreateVecDescOfTemplate (MULTIGRID *theMG,
                                       char *name, char *template)
{
  VECDATA_DESC *vd,*svd;
  VEC_FORMAT *vf;
  SUBVEC *subv;
  SHORT *Comp,SubComp[MAX_VEC_COMP],*offset;
  char SubName[MAX_VEC_COMP];
  INT i,j,k,nc,cmp,type;
  char buffer[NAMESIZE];

  if (template != NULL)
    vf = GetVectorTemplate(theMG,template);
  else
    vf = GetVectorTemplate(theMG,name);
  if (vf == NULL) {
    PrintErrorMessage('E',"CreateVecDescOfTemplate",
                      "no vector template");
    REP_ERR_RETURN(NULL);
  }
  vd = CreateVecDesc(theMG,name,VF_COMPNAMES(vf),VF_COMPS(vf));
  if (vd == NULL) {
    PrintErrorMessage('E',"CreateVecDescOfTemplate",
                      "cannot create vector descriptor");
    REP_ERR_RETURN(NULL);
  }
  VM_LOCKED(vd) = 1;

  /* now create sub vec descs */
  offset = VD_OFFSETPTR(vd);
  Comp = VM_COMPPTR(vd);
  for (i=0; i<VF_NSUB(vf); i++) {
    subv = VF_SUB(vf,i);
    strcpy(buffer,SUBV_NAME(subv));
    strcat(buffer,name);

    /* compute sub components */
    k = 0;
    for (type=0; type<NVECTYPES; type++)
    {
      nc = SUBV_NCOMP(subv,type);
      for (j=0; j<nc; j++)
      {
        cmp = offset[type]+SUBV_COMP(subv,type,j);
        SubComp[k] = Comp[cmp];
        SubName[k] = VF_COMPNAME(vf,cmp);
        k++;
      }
    }
    svd = CreateSubVecDesc(theMG,vd,buffer,SUBV_NCOMPS(subv),SubComp,SubName);
    if (svd == NULL) {
      PrintErrorMessage('E',"CreateVecDescOfTemplate",
                        "cannot create subvector descriptor");
      REP_ERR_RETURN(NULL);
    }
    VM_LOCKED(svd) = 1;
  }

  return (vd);
}

INT CreateVecDescCmd (MULTIGRID *theMG, INT argc, char **argv)
{
  char *token,*template,buffer[NAMESIZE];

  if (ReadArgvChar("t",buffer,argc,argv))
    template = NULL;
  else
    template = buffer;
  token = strtok(argv[0],BLANKS);
  token = strtok(NULL,BLANKS);
  while (token!=NULL) {
    if (CreateVecDescOfTemplate(theMG,token,template) == NULL) {
      PrintErrorMessage('E'," CreateVecDescCmd",
                        "cannot create vector descriptor");
      REP_ERR_RETURN(1);
    }
    token = strtok(NULL,BLANKS);
  }

  return (NUM_OK);
}

static MAT_FORMAT *GetMatrixTemplate (MULTIGRID *theMG, char *template)
{
  ENVITEM *item,*dir;

  if (ChangeEnvDir("/Formats") == NULL) REP_ERR_RETURN (NULL);
  dir = (ENVITEM *)ChangeEnvDir(ENVITEM_NAME(MGFORMAT(theMG)));
  if (dir == NULL) REP_ERR_RETURN (NULL);
  if (template != NULL)
    for (item=ENVITEM_DOWN(dir); item != NULL; item = NEXT_ENVITEM(item))
      if (ENVITEM_TYPE(item) == theMatVarID)
        if (strcmp(ENVITEM_NAME(item),template)==0)
          return ((MAT_FORMAT *)item);
  for (item=ENVITEM_DOWN(dir); item != NULL; item = NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == theMatVarID)
      return ((MAT_FORMAT *)item);

  REP_ERR_RETURN (NULL);
}

MATDATA_DESC *CreateMatDescOfTemplate (MULTIGRID *theMG,
                                       char *name, char *template)
{
  MATDATA_DESC *md,*smd;
  MAT_FORMAT *mf;
  SUBMAT *subm;
  SHORT *Comp,SubComp[MAX_MAT_COMP],*offset;
  INT i,j,k,nc,cmp,type;
  char SubName[2*MAX_MAT_COMP];
  char buffer[NAMESIZE];

  if (template != NULL)
    mf = GetMatrixTemplate(theMG,template);
  else
    mf = GetMatrixTemplate(theMG,name);
  if (mf == NULL) {
    PrintErrorMessage('E',"CreateMatDescOfTemplate",
                      "no matrix template");
    REP_ERR_RETURN(NULL);
  }
  md = CreateMatDesc(theMG,name,MF_COMPNAMES(mf),
                     MF_RCOMPS(mf),MF_CCOMPS(mf));
  if (md == NULL) {
    PrintErrorMessage('E',"CreateMatDescOfTemplate",
                      "cannot create matrix descriptor");
    REP_ERR_RETURN(NULL);
  }
  VM_LOCKED(md) = 1;

  /* now create sub mat descs */
  offset = MD_OFFSETPTR(md);
  Comp = VM_COMPPTR(md);
  for (i=0; i<MF_NSUB(mf); i++) {
    subm = MF_SUB(mf,i);
    strcpy(buffer,SUBM_NAME(subm));
    strcat(buffer,name);

    /* compute sub components */
    k = 0;
    for (type=0; type<NMATTYPES; type++)
    {
      nc = SUBM_RCOMP(subm,type)*SUBM_CCOMP(subm,type);
      for (j=0; j<nc; j++)
      {
        cmp = offset[type]+SUBM_COMP(subm,type,j);
        SubComp[k] = Comp[cmp];
        SubName[2*k]   = MF_COMPNAME(mf,2*cmp);
        SubName[2*k+1] = MF_COMPNAME(mf,2*cmp+1);
        k++;
      }
    }
    smd = CreateSubMatDesc(theMG,md,buffer,SUBM_RCOMPS(subm),SUBM_CCOMPS(subm),SubComp,SubName);
    if (smd == NULL) {
      PrintErrorMessage('E',"CreateMatDescOfTemplate",
                        "cannot create submatrix descriptor");
      REP_ERR_RETURN(NULL);
    }
    VM_LOCKED(smd) = 1;
  }

  return (md);
}

INT CreateMatDescCmd (MULTIGRID *theMG, INT argc, char **argv)
{
  char *token,*template,buffer[NAMESIZE];

  if (ReadArgvChar("t",buffer,argc,argv))
    template = NULL;
  else
    template = buffer;
  token = strtok(argv[0],BLANKS);
  token = strtok(NULL,BLANKS);
  while (token!=NULL) {
    if (CreateMatDescOfTemplate(theMG,token,template) == NULL) {
      PrintErrorMessage('E'," CreateMatDescCmd",
                        "cannot create matrix descriptor");
      REP_ERR_RETURN(1);
    }
    token = strtok(NULL,BLANKS);
  }

  return (NUM_OK);
}

static VEC_FORMAT *CreateVecTemplate (char *name, INT n)
{
  VEC_FORMAT *vf;
  char buffer[NAMESIZE],*token;
  INT j;

  if (ChangeEnvDir("/newformat")==NULL)
    REP_ERR_RETURN(NULL);

  if (name == NULL) sprintf(buffer,"vt%02d",n);
  else strcpy(buffer,name);
  vf = (VEC_FORMAT *) MakeEnvItem (buffer,theVecVarID,sizeof(VEC_FORMAT));
  if (vf==NULL) REP_ERR_RETURN (NULL);
  VF_NSUB(vf) = 0;
  token = DEFAULT_NAMES;
  for (j=0; j<MAX(MAX_VEC_COMP,strlen(DEFAULT_NAMES)); j++)
    VF_COMPNAME(vf,j) = token[j];

  return (vf);
}

static MAT_FORMAT *CreateMatTemplate (char *name, INT n)
{
  MAT_FORMAT *mf;
  char buffer[NAMESIZE];
  INT j;

  if (ChangeEnvDir("/newformat")==NULL)
    REP_ERR_RETURN(NULL);
  if (name == NULL) sprintf(buffer,"mt%02d",n);
  else strcpy(buffer,name);
  mf = (MAT_FORMAT *) MakeEnvItem (buffer,theMatVarID,sizeof(MAT_FORMAT));
  if (mf==NULL) REP_ERR_RETURN (NULL);
  MF_NSUB(mf) = 0;
  for (j=0; j<2*MAX_MAT_COMP; j++)
    MF_COMPNAME(mf,j) = ' ';

  return (mf);
}

/****************************************************************************/
/*D
        newformat - init a format and allocate symbols

        DESCRIPTION:
        The 'newformat' command enrols a format for multigrid user data.
        It also creates templates for vector and matrix descriptors.

   .vb
        newformat <format_name> [$V <vec_size>: {<n_vec>|<template>*}]
                              [$comp <comp_names> {$sub <sub_name> <comps>}*]]
                            [$M <mat_size>: {<n_mat>|<template>*}
                              [$d <mtype> <depth>]]
                            [$I <mat_size>] [$N] [$e <size>] [$n <size>]
   .ve

   .   V - vector
   .   M - matrix
   .   I - interpolation matrix
   .   N - node element list
   .   e - element data
   .   n - node data

        EXAMPLE:
   .vb
    newformat scalar  $V n1: 4
                      $M n1xn1: 1;

        createvector sol rhs;
        creatematrix MAT;
   .ve
    SEE ALSO:
    'createvector', 'creatematrix'
   D*/
/****************************************************************************/

INT CreateFormatCmd (INT argc, char **argv)
{
  FORMAT *newFormat;
  VectorDescriptor vd[MAXVECTORS];
  MatrixDescriptor md[MAXMATRICES];
  ENVDIR *dir;
  VEC_FORMAT *vf,*vv;
  MAT_FORMAT *mf,*mm;
  SUBVEC *subv;
  SUBMAT *subm;
  INT i,j,size,type,currtype,rtype,ctype,nvec,nmat,nsc[NMATTYPES],nvd,nmd;
  INT edata,ndata,nodeelementlist;
  SHORT offset[NMATOFFSETS],ConnDepth[NMATTYPES],ImatTypes[NVECTYPES];
  SHORT VecStorageNeeded[NVECTYPES],MatStorageNeeded[NMATTYPES];
  char formatname[NAMESIZE],*names,*token,tp,rt,ct,*p;
  char buffer[NAMESIZE];
  int n,nr,nc,depth;

  /* scan name of format */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" newformat %",NAMELENSTR,"[ -~]")),formatname)!=1) || (strlen(formatname)==0)) {
    PrintErrorMessage('E',"newformat","no format name specified");
    REP_ERR_RETURN (1);
  }
  if (GetFormat(formatname) != NULL) {
    PrintErrorMessage('W',"newformat","format already exists");
    return (NUM_OK);
  }
  for (type=0; type<NVECTYPES; type++)
    ImatTypes[type] = VecStorageNeeded[type] = 0;
  for (type=0; type<NMATTYPES; type++)
    MatStorageNeeded[type] = ConnDepth[type] = 0;
  for (type=0; type<NVECTYPES; type++) ImatTypes[type] = 0;
  for (type=0; type<NMATTYPES; type++) ConnDepth[type] = 0;
  nvec = nmat = 0;
  edata = ndata = nodeelementlist = 0;
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
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'V' :
      /* create a vector template */
      vf = CreateVecTemplate(NULL,nvec++);
      if (vf == NULL) {
        PrintErrorMessage('E',"newformat",
                          "could not allocate environment storage");
        REP_ERR_RETURN (2);
      }

      /* find name seperator */
      if ((names=strchr(argv[i],NAMESEP))==NULL)
      {
        PrintErrorMessage('E',"newformat","seperate names by a colon ':' from the description");
        REP_ERR_RETURN (1);
      }
      *(names++) = '\0';

      /* read types and sizes */
      token = strtok(argv[i]+1,BLANKS);
      for (type=0; type<NVECTYPES; type++) VF_COMP(vf,type) = 0;
      while (token!=NULL) {
        if (sscanf(token,"%c%d",&tp,&n)!=2) {
          PrintErrorMessage('E',"newformat",
                            "could not scan type and size");
          REP_ERR_RETURN (1);
        }
        switch (tp) {
        case 'n' : type = NODEVECTOR; break;
        case 'k' : type = EDGEVECTOR; break;
        case 'e' : type = ELEMVECTOR; break;
        case 's' : type = SIDEVECTOR; break;
        default :
          PrintErrorMessage('E',"newformat","specify n,k,e,s for the type (or change config to include type)");
          REP_ERR_RETURN (1);
        }
        if (VF_COMP(vf,type) !=0 ) {
          PrintErrorMessage('E',"newformat",
                            "double vector type specification");
          REP_ERR_RETURN (1);
        }
        VF_COMP(vf,type) = n;
        token = strtok(NULL,BLANKS);
      }

      /* check next arg for compnames */
      if (i+1 < argc)
        if (strncmp(argv[i+1],"comp",4)==0) {
          i++;
          if (sscanf(argv[i],"comp %s",VF_COMPNAMES(vf))!=1) {
            PrintErrorMessage('E',"newformat",
                              "no vector comp names specified with comp option");
            REP_ERR_RETURN (1);
          }
          ConstructVecOffsets(VF_COMPS(vf),offset);
          if (strlen(VF_COMPNAMES(vf))!=offset[NVECTYPES]) {
            PrintErrorMessage('E',"newformat",
                              "number of vector comp names != number of comps");
            REP_ERR_RETURN (1);
          }
          /* check next args for subv */
          while ((i+1<argc) && (strncmp(argv[i+1],"sub",3)==0)) {
            i++;
            if (VF_NSUB(vf)>=MAX_SUB) {
              PrintErrorMessage('E',"newformat",
                                "max number of vector subs exceeded");
              REP_ERR_RETURN (1);
            }
            subv = AllocEnvMemory(sizeof(SUBVEC));
            if (subv == NULL) {
              PrintErrorMessage('E',"newformat",
                                "could not allocate environment storage");
              REP_ERR_RETURN (2);
            }
            memset(subv,0,sizeof(SUBVEC));
            VF_SUB(vf,VF_NSUB(vf)) = subv;
            VF_NSUB(vf)++;
            /* subv name */
            token = strtok(argv[i]+3,BLANKS);
            if (token==NULL) {
              PrintErrorMessage('E',"newformat",
                                "specify name of subv");
              REP_ERR_RETURN (1);
            }
            strcpy(SUBV_NAME(subv),token);

            /* subv comps */
            for (type=0; type<NVECTYPES; type++) nsc[type] = 0;
            while ((token=strtok(NULL,BLANKS))!=NULL) {
              if (strlen(token)!=1) {
                PrintErrorMessage('E',"newformat",
                                  "specify one char per subv comp");
                REP_ERR_RETURN (1);
              }
              if (strchr(VF_COMPNAMES(vf),*token)==NULL) {
                PrintErrorMessage('E',"newformat",
                                  "wrong subv comp");
                REP_ERR_RETURN (1);
              }
              n = strchr(VF_COMPNAMES(vf),*token)
                  - VF_COMPNAMES(vf);
              for (type=0; type<NVECTYPES; type++)
                if (n<offset[type+1]) break;
              if (nsc[type]>=MAX_VEC_COMP) {
                PrintErrorMessage('E',"newformat",
                                  "max number of subv comps exceeded");
                REP_ERR_RETURN (1);
              }
              SUBV_COMP(subv,type,nsc[type]++) = n-offset[type];
            }
            for (type=0; type<NVECTYPES; type++)
              SUBV_NCOMP(subv,type) = nsc[type];
          }
        }

      /* read names of templates */
      if (sscanf(names,"%d",&n) == 1)
      {
        /* compute storage needed */
        for (type=0; type<NVECTYPES; type++)
          VecStorageNeeded[type] += n * VF_COMP(vf,type);
      }
      else
      {
        /* no storage reservation for special (named) templates */
        token = strtok(names,BLANKS);
        while (token!=NULL) {
          vv = CreateVecTemplate(token,nvec++);
          if (vv == NULL) {
            PrintErrorMessage('E',"newformat",
                              "could not allocate environment storage");
            REP_ERR_RETURN (2);
          }
          for (type=0; type<NVECTYPES; type++)
            VF_COMP(vv,type) = VF_COMP(vf,type);
          for (j=0; j<MAX_VEC_COMP; j++)
            VF_COMPNAME(vv,j) = VF_COMPNAME(vf,j);
          VF_NSUB(vv) = VF_NSUB(vf);
          for (j=0; j<VF_NSUB(vf); j++)
            VF_SUB(vv,j) = VF_SUB(vf,j);
          token = strtok(NULL,BLANKS);
        }
      }
      break;

    case 'M' :
      /* create a matrix template */
      mf = CreateMatTemplate(NULL,nmat++);
      if (mf == NULL) {
        PrintErrorMessage('E',"newformat",
                          "could not allocate environment storage");
        REP_ERR_RETURN (2);
      }
      /* find name seperator */
      if ((names=strchr(argv[i],NAMESEP))==NULL) {
        PrintErrorMessage('E',"newformat",
                          "seperate names by a colon ':' from the description");
        REP_ERR_RETURN (1);
      }
      *(names++) = '\0';

      /* read types and sizes */
      token = strtok(argv[i]+1,BLANKS);
      for (type=0; type<NMATTYPES; type++)
        MF_RCOMP(mf,type) = MF_CCOMP(mf,type) = 0;
      while (token!=NULL) {
        if (sscanf(token,"%c%dx%c%d",&rt,&nr,&ct,&nc)!=4) {
          PrintErrorMessage('E',"newformat",
                            "could not scan type and size");
          REP_ERR_RETURN (1);
        }
        switch (rt)
        {
        case 'n' : rtype = NODEVECTOR; break;
        case 'k' : rtype = EDGEVECTOR; break;
        case 'e' : rtype = ELEMVECTOR; break;
        case 's' : rtype = SIDEVECTOR; break;
        default :
          PrintErrorMessage('E',"newformat","specify n,k,e,s for the row type");
          REP_ERR_RETURN (1);
        }
        switch (ct)
        {
        case 'n' : ctype = NODEVECTOR; break;
        case 'k' : ctype = EDGEVECTOR; break;
        case 'e' : ctype = ELEMVECTOR; break;
        case 's' : ctype = SIDEVECTOR; break;
        default :
          PrintErrorMessage('E',"newformat","specify n,k,e,s for the col type");
          REP_ERR_RETURN (1);
        }
        type = MTP(rtype,ctype);
        if (MF_RCOMP(mf,type) !=0 ) {
          PrintErrorMessage('E',"newformat",
                            "double matrix type specification");
          REP_ERR_RETURN (1);
        }
        MF_RCOMP(mf,type) = nr;
        MF_CCOMP(mf,type) = nc;
        token = strtok(NULL,BLANKS);
      }
      /* check next arg for compnames */
      if (i+1 < argc)
        if (strncmp(argv[i+1],"comp",4) == 0) {
          i++;
          if (sscanf(argv[i],"comp %s",MF_COMPNAMES(mf))!=1) {
            PrintErrorMessage('E',"newformat",
                              "no matrix comp names specified with comp option");
            REP_ERR_RETURN (1);
          }
          ConstructMatOffsets(MF_RCOMPS(mf),
                              MF_CCOMPS(mf),offset);
          if (strlen(MF_COMPNAMES(mf))!=2*offset[NMATTYPES]) {
            PrintErrorMessage('E',"newformat",
                              "number of matrix comp names != number of comps");
            REP_ERR_RETURN (1);
          }
          /* check next args for subm */
          while ((i+1<argc) && (strncmp(argv[i+1],"sub",3)==0)) {
            i++;
            if (MF_NSUB(mf)>=MAX_SUB) {
              PrintErrorMessage('E',"newformat",
                                "max number of matrix subs exceeded");
              REP_ERR_RETURN (1);
            }
            subm = AllocEnvMemory(sizeof(SUBMAT));
            if (subm == NULL) {
              PrintErrorMessage('E',"newformat",
                                "could not allocate environment storage");
              REP_ERR_RETURN (2);
            }
            memset(subm,0,sizeof(SUBMAT));
            MF_SUB(mf,MF_NSUB(mf)) = subm;
            MF_NSUB(mf)++;

            /* subm name */
            token = strtok(argv[i]+3,BLANKS);
            if (token==NULL) {
              PrintErrorMessage('E',"newformat",
                                "specify name of subm");
              REP_ERR_RETURN (1);
            }
            strcpy(SUBM_NAME(subm),token);

            /* subm comps */
            for (type=0; type<NMATTYPES; type++) nsc[type] = 0;
            while ((token=strtok(NULL,BLANKS))!=NULL) {
              /* scan size */
              if (sscanf(token,"%dx%d",&nr,&nc)!=2) {
                PrintErrorMessage('E',"newformat",
                                  "specify size of subm");
                REP_ERR_RETURN (1);
              }
              while ((token=strtok(NULL,BLANKS))!=NULL) {
                if (strlen(token)!=2) {
                  PrintErrorMessage('E',"newformat",
                                    "specify two chars per subm comp");
                  REP_ERR_RETURN (1);
                }
                for (p=MF_COMPNAMES(mf); *p!='\0'; p+=2)
                  if ((p[0]==token[0])&&(p[1]==token[1]))
                    break;
                if (*p=='\0') {
                  PrintErrorMessage('E',"newformat",
                                    "wrong subm comp");
                  REP_ERR_RETURN (1);
                }
                n = (p - MF_COMPNAMES(mf))/2;
                for (type=0; type<NMATTYPES; type++)
                  if (n<offset[type+1]) break;

                if (nsc[type]>=MAX_MAT_COMP) {
                  PrintErrorMessage('E',"newformat",
                                    "max number of subm comps exceeded");
                  REP_ERR_RETURN (1);
                }
                if (nsc[type]==0)
                  currtype = type;
                else if (type!=currtype) {
                  PrintErrorMessage('E',"newformat",
                                    "wrong comp type for subm");
                  REP_ERR_RETURN (1);
                }
                SUBM_COMP(subm,type,nsc[type]++) = n-offset[type];
                if (nsc[type]==nr*nc) break;
              }
              SUBM_RCOMP(subm,type) = nr;
              SUBM_CCOMP(subm,type) = nc;
            }
          }
        }

      /* read names of templates */
      if (sscanf(names,"%d",&n) == 1)
      {
        /* compute storage needed */
        for (type=0; type<NMATTYPES; type++)
          MatStorageNeeded[type] += n*MF_RCOMP(mf,type)*MF_CCOMP(mf,type);
      }
      else
      {
        /* no storage reservation for special (named) templates */
        token = strtok(names,BLANKS);
        while (token!=NULL) {
          mm = CreateMatTemplate(token,nmat++);
          if (mm == NULL) {
            PrintErrorMessage('E',"newformat",
                              "could not allocate environment storage");
            REP_ERR_RETURN (2);
          }
          for (type=0; type<NMATTYPES; type++) {
            MF_RCOMP(mm,type) = MF_RCOMP(mf,type);
            MF_CCOMP(mm,type) = MF_CCOMP(mf,type);
          }
          for (j=0; j<2*MAX_MAT_COMP; j++)
            MF_COMPNAME(mm,j) = MF_COMPNAME(mf,j);
          MF_NSUB(mm) = MF_NSUB(mf);
          for (j=0; j<MF_NSUB(mf); j++)
            MF_SUB(mm,j) = MF_SUB(mf,j);
          token = strtok(NULL,BLANKS);
        }
      }
      break;

    case 'd' :
      if (sscanf(argv[i],"d %cx%c%d",&rt,&ct,&depth)!=3)
      {
        PrintErrorMessage('E',"newformat","could not read connection depth");
        REP_ERR_RETURN (1);
      }
      switch (rt)
      {
      case 'n' : rtype = NODEVECTOR; break;
      case 'k' : rtype = EDGEVECTOR; break;
      case 'e' : rtype = ELEMVECTOR; break;
      case 's' : rtype = SIDEVECTOR; break;
      default :
        PrintErrorMessage('E',"newformat","specify n,k,e,s for the row type");
        REP_ERR_RETURN (1);
      }
      switch (ct)
      {
      case 'n' : ctype = NODEVECTOR; break;
      case 'k' : ctype = EDGEVECTOR; break;
      case 'e' : ctype = ELEMVECTOR; break;
      case 's' : ctype = SIDEVECTOR; break;
      default :
        PrintErrorMessage('E',"newformat","specify n,k,e,s for the col type");
        REP_ERR_RETURN (1);
      }
      ConnDepth[MTP(rtype,ctype)] = depth;
      break;

    case 'I' :
      /* read types and sizes of Interpolation matrix */
      token = strtok(argv[i]+1,BLANKS);
      while (token!=NULL)
      {
        if (sscanf(token,"%c%d",&tp,&n)!=2)
        {
          PrintErrorMessage('E',"newformat","could not scan type and size");
          REP_ERR_RETURN (1);
        }
        switch (tp)
        {
        case 'n' : type = NODEVECTOR; break;
        case 'k' : type = EDGEVECTOR; break;
        case 'e' : type = ELEMVECTOR; break;
        case 's' : type = SIDEVECTOR; break;
        default :
          PrintErrorMessage('E',"newformat","specify n,k,e,s for the type (or change config to include type)");
          REP_ERR_RETURN (1);
        }
        ImatTypes[type] = n;
        token = strtok(NULL,BLANKS);
      }
      break;

    case 'e' :
      if (sscanf(argv[i],"e %d",&n) == 1)
        edata = n;
      break;

    case 'n' :
      if (sscanf(argv[i],"e %d",&n) == 1)
        ndata = n;
      break;

    case 'N' :
      if (argv[i][1] == 'E')
        nodeelementlist = TRUE;
      break;

    default :
      sprintf(buffer,"(invalid option '%s')",argv[i]);
      PrintErrorMessage('E',"newformat",buffer);
      REP_ERR_RETURN (1);
    }

  if ((ndata == TRUE) && (nodeelementlist == TRUE)) {
    PrintErrorMessage('E',"newformat","specify $n or $NE");
    REP_ERR_RETURN (5);
  }

  /* now we are ready to create the format */

  /* fill degrees of freedom needed */
  nvd = 0;
  for (type=0; type<NVECTYPES; type++)
    if (VecStorageNeeded[type]>0)
    {
      vd[nvd].pos   = type;
      vd[nvd].size  = VecStorageNeeded[type]*sizeof(DOUBLE);
      vd[nvd].print = PrintVectorDataPtr[type];
      nvd++;
    }

  if (nodeelementlist || ndata) {
    for (i=0; i<nvd; i++)
      if (vd[i].pos == NODEVECTOR)
        break;
    if (i == nvd) {
      PrintErrorMessage('E',"newformat","node data requires node vector");
      REP_ERR_RETURN (5);
    }
  }

  /* fill connections needed */
  nmd = 0;
  for (rtype=0; rtype<NVECTYPES; rtype++)
    for (ctype=rtype; ctype<NVECTYPES; ctype++)
    {
      type = MTP(rtype,ctype);
      size = MAX(MatStorageNeeded[MTP(rtype,ctype)],MatStorageNeeded[MTP(ctype,rtype)]);

      if (size<= 0) continue;

      depth = MAX(ConnDepth[MTP(rtype,ctype)],ConnDepth[MTP(ctype,rtype)]);

      md[nmd].from  = rtype;
      md[nmd].to    = ctype;
      md[nmd].size  = size*sizeof(DOUBLE);
      md[nmd].depth = depth;
      md[nmd].print = PrintMatrixDataPtr[type];
      nmd++;
    }

  /* create format */
  newFormat = CreateFormat(formatname,0,0,
                           (ConversionProcPtr)NULL,(ConversionProcPtr)NULL,(ConversionProcPtr)NULL,
                           nvd,vd,nmd,md);
  if (newFormat==NULL)
  {
    PrintErrorMessage('E',"newformat","failed creating the format");
    REP_ERR_RETURN (3);
  }

#ifdef __INTERPOLATION_MATRIX__
  for (i=0; i<MAXVECTORS; i++)
    for (j=0; j<MAXVECTORS; j++)
      newFormat->IMatrixSizes[MatrixType[i][j]]
        = ImatTypes[i] * ImatTypes[j] * sizeof(DOUBLE);
#endif

  newFormat->nodeelementlist = nodeelementlist;
  newFormat->elementdata = edata;
  newFormat->nodedata = ndata;

  /* move tempaltes into the new directory */
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

  return (NUM_OK);
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

INT InitFormats ()
{
  /* init printing routine ptrs */
        #ifdef __TWODIM__
  PrintVectorDataPtr[NODEVECTOR] = PrintNodeVectorData;
  PrintVectorDataPtr[ELEMVECTOR] = PrintElemVectorData;
  PrintVectorDataPtr[EDGEVECTOR] = PrintEdgeVectorData;

  PrintMatrixDataPtr[MTP(NODEVECTOR,NODEVECTOR)] = PrintNodeNodeMatrixData;
  PrintMatrixDataPtr[MTP(NODEVECTOR,ELEMVECTOR)] =
    PrintMatrixDataPtr[MTP(ELEMVECTOR,NODEVECTOR)] = PrintNodeElemMatrixData;
  PrintMatrixDataPtr[MTP(NODEVECTOR,EDGEVECTOR)] =
    PrintMatrixDataPtr[MTP(EDGEVECTOR,NODEVECTOR)] = PrintNodeEdgeMatrixData;
  PrintMatrixDataPtr[MTP(ELEMVECTOR,ELEMVECTOR)] = PrintElemElemMatrixData;
  PrintMatrixDataPtr[MTP(ELEMVECTOR,EDGEVECTOR)] =
    PrintMatrixDataPtr[MTP(EDGEVECTOR,ELEMVECTOR)] = PrintElemEdgeMatrixData;
  PrintMatrixDataPtr[MTP(EDGEVECTOR,EDGEVECTOR)] = PrintEdgeEdgeMatrixData;
        #endif
        #ifdef __THREEDIM__
  PrintVectorDataPtr[NODEVECTOR] = PrintNodeVectorData;
  PrintVectorDataPtr[ELEMVECTOR] = PrintElemVectorData;
  PrintVectorDataPtr[EDGEVECTOR] = PrintEdgeVectorData;
  PrintVectorDataPtr[SIDEVECTOR] = PrintSideVectorData;

  PrintMatrixDataPtr[MTP(NODEVECTOR,NODEVECTOR)] = PrintNodeNodeMatrixData;
  PrintMatrixDataPtr[MTP(NODEVECTOR,ELEMVECTOR)] =
    PrintMatrixDataPtr[MTP(ELEMVECTOR,NODEVECTOR)] = PrintNodeElemMatrixData;
  PrintMatrixDataPtr[MTP(NODEVECTOR,EDGEVECTOR)] =
    PrintMatrixDataPtr[MTP(EDGEVECTOR,NODEVECTOR)] = PrintNodeEdgeMatrixData;
  PrintMatrixDataPtr[MTP(NODEVECTOR,SIDEVECTOR)] =
    PrintMatrixDataPtr[MTP(SIDEVECTOR,NODEVECTOR)] = PrintNodeSideMatrixData;
  PrintMatrixDataPtr[MTP(ELEMVECTOR,ELEMVECTOR)] = PrintElemElemMatrixData;
  PrintMatrixDataPtr[MTP(ELEMVECTOR,EDGEVECTOR)] =
    PrintMatrixDataPtr[MTP(EDGEVECTOR,ELEMVECTOR)] = PrintElemEdgeMatrixData;
  PrintMatrixDataPtr[MTP(ELEMVECTOR,SIDEVECTOR)] =
    PrintMatrixDataPtr[MTP(SIDEVECTOR,ELEMVECTOR)] = PrintElemSideMatrixData;
  PrintMatrixDataPtr[MTP(EDGEVECTOR,EDGEVECTOR)] = PrintEdgeEdgeMatrixData;
  PrintMatrixDataPtr[MTP(EDGEVECTOR,SIDEVECTOR)] =
    PrintMatrixDataPtr[MTP(SIDEVECTOR,EDGEVECTOR)] = PrintEdgeSideMatrixData;
  PrintMatrixDataPtr[MTP(SIDEVECTOR,SIDEVECTOR)] = PrintSideSideMatrixData;
        #endif

  theNewFormatDirID = GetNewEnvDirID();
  theVecVarID = GetNewEnvVarID();
  theMatVarID = GetNewEnvVarID();

  return (0);
}
