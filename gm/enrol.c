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

#ifdef __MPW32__
#pragma segment ugm
#endif

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
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* low modules */
#include "compiler.h"
#include "heaps.h"
#include "ugenv.h"
#include "general.h"

/* devices module */
#include "devices.h"

/* grid manager module */
#include "switch.h"
#include "gm.h"
#include "algebra.h"
#include "misc.h"
#include "enrol.h"

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

static INT theFormatDirID;                      /* env type for Format dir				*/
static INT theSymbolVarID;                      /* env type for Format vars                     */

/* RCS string */
static char RCS_ID("$Header$",UG_RCS_STRING);

/****************************************************************************/
/*D
   CreateFormat	- Create a new FORMAT structure in the environment

   SYNOPSIS:
   FORMAT *CreateFormat (char *name, INT sVertex, INT sMultiGrid,
                ConversionProcPtr PrintVertex, ConversionProcPtr PrintGrid,
                ConversionProcPtr PrintMultigrid, INT nvDesc, VectorDescriptor *vDesc,
                INT nmDesc, MatrixDescriptor *mDesc);

   PARAMETERS:
   .  name - name of new format structure
   .  sVertex - size of user data space in VERTEX counted in bytes
   .  sMultiGrid -  size of user data space in MULTIGRID counted in bytes
   .  PrintVertex - pointer to conversion procedure
   .  PrintGrid - pointer to conversion procedure (though there are no user data associated directly with a grid,
                                the user may wish to print grid associated data from his multigrid user data space)
   .  PrintMultigrid - pointer to conversion procedure
   .  nvDesc - number of vector descriptors
   .  vDesc - pointer to vector descriptor
   .  nmDesc - number of matrix desciptors
   .  mDesc - pointer to matrix descriptor

   DESCRIPTION:
   This function allocates and initializes a new FORMAT structure in the environment.
   The parameters vDesc and mDesc are pointers to structures which describe the
   VECTOR or MATRIX types used.
   VectorDescriptor is defined as
   .vb
          typedef struct {
          int pos;
          int size;
          ConversionProcPtr print;
          } VectorDescriptor ;
   .ve
        The components have the following meaning

   .   pos - this is the position for which this description is valid, one of
                        NODEVECTOR, EDGEVECTOR, ELEMVECTOR, SIDEVECTOR
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

   .   from - this connection goes from position
   .   to - to this position, position is one of NODEVECTOR, EDGEVECTOR, ELEMVECTOR, SIDEVECTOR
   .   size - this defines the size in bytes per connection
   .   depth - this connection has the depth defined here
   .   print - function to print the data.

   EXAMPLES:
   A small example to create a format looks like the following. In this format only
   vectors in nodes are used and therfore all connections connect two nodevectors.
   .vb
      // we need dofs only in nodes
      vd[0].pos   = NODEVECTOR;
      vd[0].size  = 3*sizeof(DOUBLE);
      vd[0].print = Print_3_NodeVectorData;

      // and the following connection: node-node
      md[0].from  = NODEVECTOR;
      md[0].to    = NODEVECTOR;
      md[0].size  = sizeof(DOUBLE);
      md[0].depth = 0;
      md[0].print = Print_1_NodeNodeMatrixData;

      newFormat = CreateFormat("full scalar",0,0,
                  (ConversionProcPtr)NULL,(ConversionProcPtr)NULL,
                  (ConversionProcPtr)NULL,1,vd,1,md);
   .ve

   RETURN VALUE:
   FORMAT *
   .n     pointer to FORMAT
   .n     NULL if out of memory.
   D*/
/****************************************************************************/

FORMAT *CreateFormat (char *name, INT sVertex, INT sMultiGrid,
                      ConversionProcPtr PrintVertex, ConversionProcPtr PrintGrid, ConversionProcPtr PrintMultigrid,
                      INT nvDesc, VectorDescriptor *vDesc, INT nmDesc, MatrixDescriptor *mDesc)
{
  FORMAT *newFormat;
  INT i, j, MaxDepth, NeighborhoodDepth;


  /* change to /Formats directory */
  if (ChangeEnvDir("/Formats")==NULL)
    return (NULL);

  /* allocate new format structure */
  newFormat = (FORMAT *) MakeEnvItem (name,theFormatDirID,sizeof(FORMAT));
  if (newFormat==NULL) return(NULL);

  /* fill in data */
  newFormat->sVertex                      = sVertex;
  newFormat->sMultiGrid           = sMultiGrid;
  newFormat->PrintVertex          = PrintVertex;
  newFormat->PrintGrid            = PrintGrid;
  newFormat->PrintMultigrid       = PrintMultigrid;

  /* initialize with zero */
  for (i=0; i<MAXVECTORS; i++)
  {
    newFormat->VectorSizes[i] = 0;
    newFormat->PrintVector[i] = (ConversionProcPtr) NULL;
    for (j=0; j<MAXVECTORS; j++) newFormat->PrintMatrix[i][j] = (ConversionProcPtr) NULL;
  }
  for (i=0; i<MAXMATRICES; i++)
  {
    newFormat->MatrixSizes[i] = 0;
    newFormat->ConnectionDepth[i] = 0;
  }
  MaxDepth = NeighborhoodDepth = 0;

  /* set vector stuff */
  for (i=0; i<nvDesc; i++)
  {
    if ((vDesc[i].pos<0)||(vDesc[i].pos>=MAXVECTORS)||(vDesc[i].size<0)) return(NULL);
    newFormat->VectorSizes[vDesc[i].pos] = vDesc[i].size;
    newFormat->PrintVector[vDesc[i].pos] = vDesc[i].print;
  }

#ifdef __INTERPOLATION_MATRIX__
  for (i=0; i<MAXMATRICES; i++)
    newFormat->IMatrixSizes[i] = 0;
#endif

  /* set connection stuff */
  for (i=0; i<nmDesc; i++)
  {
    if ((mDesc[i].from<0)||(mDesc[i].from>=MAXVECTORS)) return(NULL);
    if ((mDesc[i].to<0)  ||(mDesc[i].to>=MAXVECTORS)) return(NULL);
    if ((mDesc[i].size<0)||(mDesc[i].depth<0)) return(NULL);
    if (newFormat->VectorSizes[mDesc[i].from]>0 &&
        newFormat->VectorSizes[mDesc[i].to]>0 &&
        mDesc[i].size>0 && mDesc[i].depth>=0)
    {
      newFormat->MatrixSizes[MatrixType[mDesc[i].from][mDesc[i].to]] = mDesc[i].size;
#ifdef __INTERPOLATION_MATRIX__
      newFormat->IMatrixSizes[MatrixType[mDesc[i].from][mDesc[i].to]] = mDesc[i].isize;
#endif
      newFormat->ConnectionDepth[MatrixType[mDesc[i].from][mDesc[i].to]] = mDesc[i].depth;
      MaxDepth = MAX(MaxDepth,mDesc[i].depth);
      if ((mDesc[i].from==ELEMVECTOR)&&(mDesc[i].to==ELEMVECTOR))
        NeighborhoodDepth = MAX(NeighborhoodDepth,mDesc[i].depth);
      else
        NeighborhoodDepth = MAX(NeighborhoodDepth,mDesc[i].depth+1);
      newFormat->PrintMatrix[mDesc[i].from][mDesc[i].to] = mDesc[i].print;
      newFormat->PrintMatrix[mDesc[i].to][mDesc[i].from] = mDesc[i].print;
    }
  }
  newFormat->MaxConnectionDepth = MaxDepth;
  newFormat->NeighborhoodDepth  = NeighborhoodDepth;

  if (ChangeEnvDir(name)==NULL) return(NULL);
  UserWrite("format "); UserWrite(name); UserWrite(" installed\n");

  return(newFormat);
}

/****************************************************/
/*D
   GetFormat - Get a format pointer from the environment

   SYNOPSIS:
   FORMAT *GetFormat (char *name)

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

FORMAT *GetFormat (const char *name)
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

FORMAT *GetFirstFormat (void)
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

FORMAT *GetNextFormat (FORMAT *fmt)
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

INT ChangeToFormatDir (const char *name)
{
  if (ChangeEnvDir("/Formats")==NULL)
    return (1);
  if (ChangeEnvDir(name)==NULL)
    return (2);

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

INT InitEnrol ()
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
