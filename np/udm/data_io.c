// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  dio.c															*/
/*																			*/
/* Purpose:   input/output of data			                                                                */
/*																			*/
/* Author:	  Klaus Johannsen                                                                                               */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   16.12.96 begin,												*/
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

#include <stdio.h>

#include "compiler.h"
#include "heaps.h"
#include "defaults.h"
#include "general.h"
#include "debug.h"
#include "devices.h"
#include "switch.h"
#include "gm.h"
#include "algebra.h"
#include "misc.h"
#include "bio.h"
#include "dio.h"
#include "num.h"
#include "ugm.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
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
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/* RCS string */
RCSID("$Header$",UG_RCS_STRING)

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*D
   LoadData - reads data

   SYNOPSIS:
   INT LoadData (MULTIGRID *theMG, char *FileName, INT n, VECDATA_DESC **theVDList);

   PARAMETERS:
   .  theMG - multigrid
   .  FileName - name of file
   .  n - nb of vecdata desc
   .  theVDList - list of vecdata

   DESCRIPTION:
   loads data from file

   RETURN VALUE:
   int
   .n    0 if ok
   .n    1 when error occured.

   SEE ALSO:
   D*/
/****************************************************************************/

INT LoadData (MULTIGRID *theMG, char *FileName, INT n, VECDATA_DESC **theVDList)
{
  INT i,j,ncomp,s,*entry,copied_until,copy_until,still_to_read,read,zip;
  unsigned long m;
  DIO_GENERAL dio_general;
  HEAP *theHeap;
  char sysCom[NAMESIZE];
  char *p;
  double *data;
  VECTOR *theV;
  GRID *theGrid;
  NODE *theNode;

  if (theMG==NULL) return (1);
  theHeap = MGHEAP(theMG);
  if (n<1) return (1);

  /* check if only NODEVECTORs */
  for (i=0; i<n; i++)
  {
    if (VD_NCMPS_IN_TYPE(theVDList[i],EDGEVECTOR)) return (1);
    if (VD_NCMPS_IN_TYPE(theVDList[i],ELEMVECTOR)) return (1);
#ifdef __THREEDIM__
    if (VD_NCMPS_IN_TYPE(theVDList[i],SIDEVECTOR)) return (1);
#endif
  }

#ifndef __MWCW__
  /* unzip if */
  zip = 0;
  p = FileName + strlen(FileName) - 3;
  if (strcmp(p,".gz")==0)
  {
    zip = 1;
    strcpy(sysCom,"gzip -d -f ");
    strcat(sysCom,FileName);
    if (system(sysCom)==-1) return (1);
    p[0] = '\0';
  }
#endif

  /* open file */
  if (Read_OpenDTFile (FileName)) return (1);

  /* read general information */
  if (Read_DT_General (&dio_general))                                     {CloseDTFile(); return (1);}

  if (strcmp(dio_general.version,DIO_VERSION)!=0)                 {CloseDTFile(); UserWrite("ERROR: wrong version\n"); return (1);}
  if (dio_general.magic_cookie != MG_MAGIC_COOKIE(theMG)) {CloseDTFile(); UserWrite("m-c-error"); return (1);}
  if (dio_general.nVD != n)                                                               {CloseDTFile(); UserWrite("ERROR: wrong version\n"); return (1);}
  ncomp = 0;
  for (i=0; i<n; i++)
  {
    if (strcmp(dio_general.VDname[i],"---"))                        {CloseDTFile(); UserWrite("vd-names do not match\n"); return (1);}
    if (dio_general.VDncomp[i] != VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR)) {CloseDTFile(); UserWrite("vd-comp do not match\n"); return (1);}
    ncomp += VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR);
  }

  /* load data */
  MarkTmpMem(theHeap);
  entry = (INT *)GetTmpMem(theHeap,ncomp*sizeof(INT));
  if (entry==NULL)                                                                                {CloseDTFile(); return (1);}
  s=0;
  for (i=0; i<n; i++)
    for (j=0; j<VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR); j++)
      entry[s++] = VD_CMP_OF_TYPE(theVDList[i],NODEVECTOR,j);
  if (s!=ncomp)                                                                                   {CloseDTFile(); return (1);}
  m = theHeap->size - theHeap->used;
  m = m - m%sizeof(double); m /= sizeof(double);
  while(m>=ncomp)
  {
    data = (double *)GetTmpMem(theHeap,m*sizeof(double));
    if (data!=NULL) break;
    m *= 0.5;
  }
  if (data==NULL)                                                                                 {CloseDTFile(); UserWrite("ERROR: cannot allocate tmp mem\n"); return (1);}
  if (m<ncomp)                                                                                    {CloseDTFile(); UserWrite("ERROR: tmp mem too small\n"); return (1);}

  copied_until = copy_until=0; still_to_read=dio_general.ndata;
  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theNode=FIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
    {
      if (copied_until>=copy_until)
      {
        if (still_to_read<=m) read=still_to_read;
        else read=m;
        read -= read%ncomp;
        if (Bio_Read_mdouble(read,data))                        {CloseDTFile(); UserWrite("ERROR: tmp mem too small\n"); return (1);}
        still_to_read -= read;
        copy_until += read;
        s=0;
      }
      theV = NVECTOR(theNode);
      for (j=0; j<ncomp; j++)
        VVALUE(theV,entry[j]) = data[s++];
      copied_until += ncomp;
    }
  }
  ReleaseTmpMem(theHeap);

  /* close file */
  if (CloseDTFile()) return (1);

#ifndef __MWCW__
  /* zip if */
  if (zip)
  {
    strcpy(sysCom,"gzip -f ");
    strcat(sysCom,FileName);
    if (system(sysCom)==-1) return (1);
  }
#endif

  return (0);
}

/****************************************************************************/
/*D
   SaveData - writes data

   SYNOPSIS:
   INT SaveData (MULTIGRID *theMG, char *FileName, INT n, VECDATA_DESC **theVDList);

   PARAMETERS:
   .  theMG - multigrid
   .  FileName - name of file
   .  n - nb of vecdata desc
   .  theVDList - list of vecdata

   DESCRIPTION:
   saves data on file

   RETURN VALUE:
   int
   .n    0 if ok
   .n    1 when error occured.

   SEE ALSO:
   D*/
/****************************************************************************/

INT SaveData (MULTIGRID *theMG, char *FileName, INT n, VECDATA_DESC **theVDList)
{
  INT i,j,ncomp,s,*entry,nDouble,zip;
  unsigned long m;
  DIO_GENERAL dio_general;
  HEAP *theHeap;
  char *p;
  char sysCom[NAMESIZE];
  double *data;
  VECTOR *theV;
  GRID *theGrid;
  NODE *theNode;

  /* init */
  if (theMG==NULL) return (1);
  if (RenumberNodeElem (theMG)) return (1);
  theHeap = MGHEAP(theMG);
  if (n<1) return (1);

  /* check if only NODEVECTORs */
  for (i=0; i<n; i++)
  {
    if (VD_NCMPS_IN_TYPE(theVDList[i],EDGEVECTOR)) return (1);
    if (VD_NCMPS_IN_TYPE(theVDList[i],ELEMVECTOR)) return (1);
#ifdef __THREEDIM__
    if (VD_NCMPS_IN_TYPE(theVDList[i],SIDEVECTOR)) return (1);
#endif
  }

#ifndef __MWCW__
  /* zip if */
  zip = 0;
  p = FileName + strlen(FileName) - 3;
  if (strcmp(p,".gz")==0)
  {
    zip = 1;
    p[0] = '\0';
  }
#endif

  /* open file */
  if (Write_OpenDTFile (FileName)) return (1);

  /* write general information */
  p = FileName + strlen(FileName) - 4;
  if (strcmp(p,".dbg")==0) dio_general.mode = BIO_DEBUG;
  else if (strcmp(p,".asc")==0) dio_general.mode = BIO_ASCII;
  else if (strcmp(p,".bin")==0) dio_general.mode = BIO_BIN;
  else                                                                                                    {CloseDTFile(); return (1);}
  strcpy(dio_general.version,DIO_VERSION);
  dio_general.magic_cookie = MG_MAGIC_COOKIE(theMG);
  dio_general.nVD = n;
  ncomp = 0;
  for (i=0; i<n; i++)
  {
    strcpy(dio_general.VDname[i],"---");
    dio_general.VDncomp[i] = VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR);
    ncomp += VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR);
  }
  nDouble=0;
  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    nDouble += NN(theGrid);
  }
  dio_general.ndata = nDouble*ncomp;
  if (Write_DT_General (&dio_general))                                    {CloseDTFile(); return (1);}

  /* save data */
  MarkTmpMem(theHeap);
  entry = (INT *)GetTmpMem(theHeap,ncomp*sizeof(INT));
  if (entry==NULL)                                                                                {CloseDTFile(); return (1);}
  s=0;
  for (i=0; i<n; i++)
    for (j=0; j<VD_NCMPS_IN_TYPE(theVDList[i],NODEVECTOR); j++)
      entry[s++] = VD_CMP_OF_TYPE(theVDList[i],NODEVECTOR,j);
  if (s!=ncomp)                                                                                   {CloseDTFile(); return (1);}
  m = theHeap->size - theHeap->used;
  m = m - m%sizeof(double); m /= sizeof(double);
  while(m>=ncomp)
  {
    data = (double *)GetTmpMem(theHeap,m*sizeof(double));
    if (data!=NULL) break;
    m *= 0.5;
  }
  if (data==NULL)                                                                                 {CloseDTFile(); UserWrite("ERROR: cannot allocate tmp mem\n"); return (1);}
  if (m<ncomp)                                                                                    {CloseDTFile(); UserWrite("ERROR: tmp mem too small\n"); return (1);}
  s=0;
  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theNode=FIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
    {
      theV = NVECTOR(theNode);
      for (j=0; j<ncomp; j++)
        data[s++] = VVALUE(theV,entry[j]);
      if (s>m-ncomp)
      {
        if (Bio_Write_mdouble(s,data))                          {CloseDTFile(); return (1);}
        s=0;
      }
    }
  }
  if (s>0) if (Bio_Write_mdouble(s,data))                                 {CloseDTFile(); return (1);}
  ReleaseTmpMem(theHeap);

  /* close file */
  if (CloseDTFile()) return (1);

#ifndef __MWCW__
  /* zip if */
  if (zip)
  {
    strcpy(sysCom,"gzip -f ");
    strcat(sysCom,FileName);
    if (system(sysCom)==-1) return (1);
  }
#endif

  return (0);
}
