// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      debug.c                                                       */
/*                                                                          */
/* Purpose:   produces lists for debugging DDD data structures              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/07/04 kb  begin                                            */
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

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstdio>

#include "dddi.h"

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

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
/* data structures                                                          */
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




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* Function:  ListLocalObjects                                              */
/*                                                                          */
/* Purpose:   display list of all local objects                             */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static int sort_LocalObjs (const void *e1, const void *e2)
{
  DDD_HDR o1, o2;

  o1 = *((DDD_HDR *)e1);
  o2 = *((DDD_HDR *)e2);

  if (OBJ_TYPE(o1) < OBJ_TYPE(o2)) return(-1);
  if (OBJ_TYPE(o1) > OBJ_TYPE(o2)) return(1);

  if (OBJ_GID(o1) < OBJ_GID(o2)) return(-1);
  if (OBJ_GID(o1) == OBJ_GID(o2)) return(0);
  return(1);
}


void DDD_ListLocalObjects (void)
{
  DDD_HDR o, *locObjs;
  int i;

  if ((locObjs=LocalObjectsList()) ==NULL)
    return;

  qsort(locObjs, ddd_nObjs, sizeof(DDD_HDR), sort_LocalObjs);

  for(i=0; i<ddd_nObjs; i++)
  {
    o = locObjs[i];
    sprintf(cBuffer, "%4d: #%04d  adr=%p gid=0x" OBJ_GID_FMT " type=0x%02x"
            " prio=%04d attr=%04d\n",
            me, i, o, OBJ_GID(o), OBJ_TYPE(o), OBJ_PRIO(o), OBJ_ATTR(o));
    DDD_PrintLine(cBuffer);
  }

  FreeLocalObjectsList(locObjs);
}

END_UGDIM_NAMESPACE
