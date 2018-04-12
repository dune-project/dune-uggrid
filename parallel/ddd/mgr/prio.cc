// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      prio.c                                                        */
/*                                                                          */
/* Purpose:   priority management for ddd                                   */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/04/25 kb  begin                                            */
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
#include <cstring>

#include "dddi.h"

USING_UG_NAMESPACE

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


/* get index for one-dimensional storage of twodimensional symm. matrix,
   which is stored as lower triangle and diagonal

   col must be <= row !
 */
#define PM_ENTRY(pm,row,col)   (pm[((((row)+1)*(row))/2)+(col)])
#define PM_SIZE  ((MAX_PRIO*(MAX_PRIO+1))/2)


/* get priority-merge value for given default mode */
#define PM_GETDEFAULT(mm,p1,p2,pres)                         \
  switch (mm) {                                            \
  case PRIOMERGE_MAXIMUM :   pres = MAX(p1,p2); break;  \
  case PRIOMERGE_MINIMUM :   pres = MIN(p1,p2); break;  \
  default :                  pres = 0; break;           \
  }


/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* definition of static variables                                           */
/*                                                                          */
/****************************************************************************/





/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/



void DDD_PrioritySet(DDD::DDDContext& context, DDD_HDR hdr, DDD_PRIO prio)
{
/* check input parameters */
if (prio>=MAX_PRIO)
{
  sprintf(cBuffer,
          "priority must be less than %d in DDD_PrioritySet", MAX_PRIO);
  DDD_PrintError('E', 2305, cBuffer);
  HARD_EXIT;
}


#       ifdef LogObjects
sprintf(cBuffer, "%4d: LOG DDD_PrioritySet %08x old=%d new=%d\n",
        me, OBJ_GID(hdr), OBJ_PRIO(hdr), prio);
DDD_PrintDebug(cBuffer);
#       endif

if (ddd_XferActive())
{
  /* we are during Xfer, therefore initiate PrioChange operation */
  DDD_XferPrioChange(hdr, prio);
}
else if(ddd_PrioActive(context))
{
  /* we are in a Prio-environment, therefore initiate consistent PrioChange operation */
  DDD_PrioChange(context, hdr, prio);
}
else
{
  if (! ObjHasCpl(hdr))
  {
    /* just one local object, we can simply change its priority */
    OBJ_PRIO(hdr) = prio;
  }
  else
  {
    /* distributed object will get inconsistent here. issue warning. */
    if (DDD_GetOption(OPT_WARNING_PRIOCHANGE)==OPT_ON)
    {
      sprintf(cBuffer,
              "creating inconsistency for gid=" OBJ_GID_FMT " in DDD_PrioritySet",
              OBJ_GID(hdr));
      DDD_PrintError('W', 2300, cBuffer);
    }

    /* change priority, nevertheless */
    OBJ_PRIO(hdr) = prio;
  }
}
}





/****************************************************************************/

/*
        compute the result of merging two priorities.

        this function merges two priorities p1 and p2 for
        object of DDD_TYPE desc. the default merge operation
        is used (PRIOMERGE_DEFAULT). if a merge-matrix has been
        specified, this matrix will be used.

        on return, *pres will contain the resulting priority.
        the return value is:

                PRIO_UNKNOWN   can't decide which priority wins.
                PRIO_FIRST     first priority wins.
                PRIO_SECOND    second priority wins.
                PRIO_ERROR     an error has occurred.
 */

enum PrioMergeVals PriorityMerge(const TYPE_DESC* desc, DDD_PRIO p1, DDD_PRIO p2, DDD_PRIO *pres)
{
  /* check if matrix is available */
  if (desc->prioMatrix == nullptr)
  {
    PM_GETDEFAULT(desc->prioDefault, p1, p2, *pres);
    if (*pres==MAX_PRIO) return PRIO_ERROR;
  }
  else
  {
    if (p2<=p1)
    {
      *pres = PM_ENTRY(desc->prioMatrix,p1,p2);
    }
    else
    {
      *pres = PM_ENTRY(desc->prioMatrix,p2,p1);
    }

  }

  if (*pres==p1 || *pres!=p2) return(PRIO_FIRST);
  if (*pres==p2 || *pres!=p1) return(PRIO_SECOND);
  return(PRIO_UNKNOWN);
}


/****************************************************************************/

/*
        allocate prioMatrix and set default entries
 */

static int SetPrioMatrix (TYPE_DESC *desc, int priomerge_mode)
{
  int r, c;

  if (desc->prioMatrix==nullptr)
  {
    /* prioMatrix has not been allocated before */
    desc->prioMatrix = std::make_unique<DDD_PRIO[]>(PM_SIZE);
  }

  for(r=0; r<MAX_PRIO; r++)
  {
    for(c=0; c<=r; c++)
    {
      DDD_PRIO pres;

      PM_GETDEFAULT(priomerge_mode, r, c, pres);
      if (pres==MAX_PRIO) return false;

      PM_ENTRY(desc->prioMatrix, r, c) = pres;
    }
  }

  /* remember default setting */
  desc->prioDefault = priomerge_mode;

  return true;
}


/****************************************************************************/

/*
        check prioMatrix
 */

static int CheckPrioMatrix (TYPE_DESC *desc)
{
  int r, c;

  if (desc->prioMatrix==nullptr)
    /* no prioMatrix defined, ok */
    return true;

  /* check: entries i must be 0<=i<MAX_PRIO */
  for(r=0; r<MAX_PRIO; r++)
  {
    for(c=0; c<=r; c++)
    {
      DDD_PRIO p = PM_ENTRY(desc->prioMatrix,r,c);

      if (p>=MAX_PRIO)
      {
        sprintf(cBuffer, "PriorityMerge(%d,%d) yields %d larger than %d!",
                r, c, p, MAX_PRIO-1);
        DDD_PrintError('E', 2340, cBuffer);
        HARD_EXIT;
      }
    }
  }


  /* TODO: check for associativity */


  return true;
}



/****************************************************************************/


void DDD_PrioMergeDefault (DDD::DDDContext& context, DDD_TYPE type_id, int priomerge_mode)
{
  if (! SetPrioMatrix(&context.typeDefs()[type_id], priomerge_mode))
  {
    DDD_PrintError('E', 2330,
                   "unknown default prio-mergemode in DDD_TYPE "
                   "in DDD_PrioMergeDefault()");
    HARD_EXIT;
  }
}



#define FUNCNAME "DDD_PrioMergeDefine()"

void DDD_PrioMergeDefine (DDD::DDDContext& context, DDD_TYPE type_id,
                          DDD_PRIO p1, DDD_PRIO p2, DDD_PRIO pres)
{
  TYPE_DESC *desc = &context.typeDefs()[type_id];

  /* check for correct type */
  if (! ddd_TypeDefined(desc))
  {
    DDD_PrintError('E', 2331, "undefined DDD_TYPE in " FUNCNAME);
    HARD_EXIT;
  }


  /* create prioMatrix on demand */
  if (desc->prioMatrix == nullptr)
  {
    if (! SetPrioMatrix(desc, PRIOMERGE_DEFAULT))
    {
      sprintf(cBuffer,
              "error for DDD_TYPE %d during " FUNCNAME,
              type_id);
      DDD_PrintError('E', 2332, cBuffer);
      HARD_EXIT;
    }
  }


  /* check input priorities */

  if (p1>=MAX_PRIO)
  {
    sprintf(cBuffer, "invalid priority %d in " FUNCNAME, p1);
    DDD_PrintError('E', 2333, cBuffer);
    HARD_EXIT;
  }
  if (p2>=MAX_PRIO)
  {
    sprintf(cBuffer, "invalid priority %d in " FUNCNAME, p2);
    DDD_PrintError('E', 2333, cBuffer);
    HARD_EXIT;
  }
  if (pres>=MAX_PRIO)
  {
    sprintf(cBuffer, "invalid priority %d in " FUNCNAME, pres);
    DDD_PrintError('E', 2333, cBuffer);
    HARD_EXIT;
  }


  /* set prioMatrix entry */
  if (p2<=p1)
  {
    PM_ENTRY(desc->prioMatrix,p1,p2) = pres;
  }
  else
  {
    PM_ENTRY(desc->prioMatrix,p2,p1) = pres;
  }


  /* finally always check prioMatrix, just to be sure */
  if (!CheckPrioMatrix(desc))
  {
    sprintf(cBuffer,
            "error(s) in merge-check for DDD_TYPE %d during " FUNCNAME,
            type_id);
    DDD_PrintError('E', 2334, cBuffer);
    HARD_EXIT;
  }
}
#undef FUNCNAME


/****************************************************************************/


/*
        call merge operation from application program
 */
DDD_PRIO DDD_PrioMerge (DDD::DDDContext& context, DDD_TYPE type_id, DDD_PRIO p1, DDD_PRIO p2)
{
  TYPE_DESC *desc = &context.typeDefs()[type_id];
  DDD_PRIO newprio;

  /* check for correct type */
  if (! ddd_TypeDefined(desc))
  {
    DDD_PrintError('E', 2350, "undefined DDD_TYPE in DDD_PrioMerge()");
    HARD_EXIT;
  }


  if (p1>=MAX_PRIO)
  {
    sprintf(cBuffer, "invalid priority %d in DDD_PrioMerge()", p1);
    DDD_PrintError('E', 2351, cBuffer);
    HARD_EXIT;
  }
  if (p2>=MAX_PRIO)
  {
    sprintf(cBuffer, "invalid priority %d in DDD_PrioMerge()", p2);
    DDD_PrintError('E', 2351, cBuffer);
    HARD_EXIT;
  }


  if (PriorityMerge(desc, p1, p2, &newprio) == PRIO_ERROR)
  {
    DDD_PrintError('E', 2352, "cannot merge priorities in DDD_PrioMerge()");
    HARD_EXIT;
  }


  return newprio;
}


/****************************************************************************/

#define FUNCNAME "DDD_PrioMergeDisplay()"

void DDD_PrioMergeDisplay (DDD::DDDContext& context, DDD_TYPE type_id)
{
  TYPE_DESC* desc = &context.typeDefs()[type_id];
  int r, c, changed_rows[MAX_PRIO];
  char buf[20];

  if (me!=0)
    return;

  /* check for correct type */
  if (! ddd_TypeDefined(desc))
  {
    DDD_PrintError('E', 2360, "undefined DDD_TYPE in " FUNCNAME);
    HARD_EXIT;
  }

  sprintf(cBuffer, "/ PrioMergeDisplay for '%s', default mode ",
          desc->name);

  switch (desc->prioDefault)
  {
  case PRIOMERGE_MAXIMUM :  strcat(cBuffer, "MAX");  break;
  case PRIOMERGE_MINIMUM :  strcat(cBuffer, "MIN");  break;
  default :                 strcat(cBuffer, "ERROR!"); break;
  }

  strcat(cBuffer, "\n");
  DDD_PrintLine(cBuffer);

  if (desc->prioMatrix == nullptr)
  {
    sprintf(cBuffer, "\\ \t(no special cases defined)\n");
    DDD_PrintLine(cBuffer);
    return;
  }


  /* find out which rows/columns we will have to print */
  for(r=0; r<MAX_PRIO; r++)
  {
    changed_rows[r] = false;

    for(c=0; c<MAX_PRIO; c++)
    {
      DDD_PRIO p_dflt, p_actual;

      PM_GETDEFAULT(desc->prioDefault, r, c, p_dflt);
      PriorityMerge(desc, r, c, &p_actual);

      if (p_dflt != p_actual)
        changed_rows[r] = true;
    }
  }

  /* print */
  sprintf(cBuffer, "|\t     ");
  for(c=0; c<MAX_PRIO; c++)
  {
    if (! changed_rows[c])
      continue;

    sprintf(buf, " %3d  ", c);
    strcat(cBuffer, buf);
  }
  strcat(cBuffer, "\n");
  DDD_PrintLine(cBuffer);

  for(r=0; r<MAX_PRIO; r++)
  {
    if (! changed_rows[r])
      continue;

    sprintf(cBuffer, "|\t%2d :  ", r);
    for(c=0; c<MAX_PRIO; c++)
    {
      DDD_PRIO p_dflt, p_actual;

      if (! changed_rows[c])
        continue;

      PM_GETDEFAULT(desc->prioDefault, r, c, p_dflt);
      PriorityMerge(desc, r, c, &p_actual);

      if (p_dflt != p_actual)
      {
        sprintf(buf, " %3d  ", p_actual);
        strcat(cBuffer, buf);
      }
      else
      {
        sprintf(buf, "(%3d) ", p_actual);
        strcat(cBuffer, buf);
      }
    }

    strcat(cBuffer, "\n");
    DDD_PrintLine(cBuffer);
  }

  DDD_PrintLine("\\\n");
}
#undef FUNCNAME

/****************************************************************************/

END_UGDIM_NAMESPACE
