// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      disctools.c                                                   */
/*                                                                          */
/* Purpose:   tools for assembling                                          */
/*                                                                          */
/* Author:    Christian Wieners                                             */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/* email:     ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   Nov 27 95 begin                                               */
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

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <gm/evm.h>      /* for data structure               */
#include <gm/gm.h>       /* for data structure               */
#include "general.h"
#include <dev/ugdevices.h>

#include "disctools.h"
#ifdef ModelP
#include "parallel.h"   /* for PRIO */
#endif

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
/* data structures used in this source file (exported data structures are   */
/*        in the corresponding include file!)                               */
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

  REP_ERR_FILE

static void PrintSingleVectorX (const VECTOR *v, const VECDATA_DESC *X, INT vclass, INT vnclass, PrintfProcPtr Printf, INT *info)
{
  char buffer[256];
  DOUBLE_VECTOR pos;
  INT comp,ncomp,i,j;

  if (VCLASS(v) > vclass) return;
  if (VNCLASS(v) > vnclass) return;
  ncomp = VD_NCMPS_IN_TYPE(X,VTYPE(v));
  if (ncomp == 0) return;
  /* Check if there is an object associated with the vector. */
  i = 0;
  if (VOBJECT(v) != NULL) {
    VectorPosition(v,pos);
    i += sprintf(buffer,"x=%5.2f y=%5.2f ",pos[0],pos[1]);
    if (DIM == 3)
      i += sprintf(buffer+i,"z=%5.2f ",pos[2]);
  }
  else {
    *info = true;
    i += sprintf(buffer,"                ");
    if (DIM == 3)
      i += sprintf(buffer+i,"        ");
  }
  for (j=0; j<ncomp; j++)
  {
    comp = VD_CMP_OF_TYPE(X,VTYPE(v),j);
    i += sprintf(buffer+i,"u[%d]=%15.8f ",j,VVALUE(v,comp));
  }
  i += sprintf(buffer+i,"   cl %d %d sk ",VCLASS(v),VNCLASS(v));
  for (j=0; j<ncomp; j++)
    i += sprintf(buffer+i,"%d ",((VECSKIP(v) & (1<<j))!=0));
  i += sprintf(buffer+i,"n %d t %d o %d\n",VNEW(v),VTYPE(v),VOTYPE(v));
  Printf(buffer);

        #ifdef Debug
  if (Printf!=PrintDebug)
    PRINTDEBUG(np,1,("%s",buffer));
        #endif

  return;
}

INT NS_DIM_PREFIX PrintVectorX (const GRID *g, const VECDATA_DESC *X, INT vclass, INT vnclass, PrintfProcPtr Printf)
{
  const VECTOR *v;
  INT info=false;

  for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
    PrintSingleVectorX(v,X,vclass,vnclass,Printf,&info);

  if (info) Printf("NOTE: Geometrical information not available for some vectors.\n");

  return(NUM_OK);
}

/****************************************************************************/
/** \brief Print a vector list

   \param g - pointer to a grid
   \param X - pointer to vector descriptor
   \param vclass - class number
   \param vnclass - class number

   This function prints the values of X for all vectors with class smaller
   or equal to vclass and next class smaller or equal to vnclass.

   RETURN VALUE:
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured
 */
/****************************************************************************/

INT NS_DIM_PREFIX PrintVector (GRID *g, const VECDATA_DESC *X, INT vclass, INT vnclass)
{
  return (PrintVectorX(g,X,vclass,vnclass,UserWriteF));
}

/****************************************************************************/
/** \brief Print a matrix list

   \param g - pointer to a grid
   \param M - pointer to matrix descriptor
   \param vclass - class number
   \param vnclass - class number

   This function prints the values of M of the matrix list
   for all vectors with class smaller
   or equal to vclass and next class smaller or equal to vnclass.

   RETURN VALUE:
   .n    NUM_OK if ok
   .n    NUM_ERROR if error occured
 */
/****************************************************************************/

INT NS_DIM_PREFIX PrintMatrix (GRID *g, MATDATA_DESC *Mat, INT vclass, INT vnclass)
{
  VECTOR *v;
  MATRIX *m;
  INT Mcomp,rcomp,ccomp,i,j,rtype,ctype;

  for (v=FIRSTVECTOR(g); v!= NULL; v=SUCCVC(v))
  {
    if (VCLASS(v) > vclass) continue;
    if (VNCLASS(v) > vnclass) continue;
    rtype = VTYPE(v);
    rcomp = MD_ROWS_IN_RT_CT(Mat,rtype,rtype);
    for (i=0; i<rcomp; i++)
    {
      for (m=VSTART(v); m!=NULL; m = MNEXT(m))
      {
        ctype = MDESTTYPE(m);
        ccomp = MD_COLS_IN_RT_CT(Mat,rtype,ctype);
        if (ccomp == 0) continue;
        if (rcomp != MD_ROWS_IN_RT_CT(Mat,rtype,ctype))
          UserWrite("wrong type\n");
        Mcomp = MD_MCMP_OF_RT_CT(Mat,rtype,ctype,i*ccomp);
        for (j=0; j<ccomp; j++)
          UserWriteF("%16.8e ",MVALUE(m,Mcomp+j));
      }
      UserWrite("\n");
    }
  }

  return(NUM_OK);
}
