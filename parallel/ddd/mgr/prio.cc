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

#include <iomanip>
#include <iostream>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

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
    DUNE_THROW(Dune::Exception, "priority must be less than " << MAX_PRIO);

#       ifdef LogObjects
  Dune::dinfo << "LOG DDD_PrioritySet " << OBJ_GID(hdr)
              << " old=" << OBJ_PRIO(hdr) << " new=" << prio << "\n";
#       endif

if (ddd_XferActive(context))
{
  /* we are during Xfer, therefore initiate PrioChange operation */
  DDD_XferPrioChange(context, hdr, prio);
}
else if(ddd_PrioActive(context))
{
  /* we are in a Prio-environment, therefore initiate consistent PrioChange operation */
  DDD_PrioChange(context, hdr, prio);
}
else
{
  if (! ObjHasCpl(context, hdr))
  {
    /* just one local object, we can simply change its priority */
    OBJ_PRIO(hdr) = prio;
  }
  else
  {
    /* distributed object will get inconsistent here. issue warning. */
    if (DDD_GetOption(context, OPT_WARNING_PRIOCHANGE)==OPT_ON)
      Dune::dwarn << "DDD_PrioritySet: creating inconsistency for gid="
                  << OBJ_GID(hdr) << "\n";

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
        DUNE_THROW(Dune::Exception,
                   "PriorityMerge(" << r << "," << c << ") yields"
                   << p << " larger than " << (MAX_PRIO-1));
    }
  }


  /* TODO: check for associativity */


  return true;
}



/****************************************************************************/


void DDD_PrioMergeDefault (DDD::DDDContext& context, DDD_TYPE type_id, int priomerge_mode)
{
  if (! SetPrioMatrix(&context.typeDefs()[type_id], priomerge_mode))
    DUNE_THROW(Dune::Exception, "unknown defaultprio-mergemode in DDD_TYPE " << type_id);
}



void DDD_PrioMergeDefine (DDD::DDDContext& context, DDD_TYPE type_id,
                          DDD_PRIO p1, DDD_PRIO p2, DDD_PRIO pres)
{
  TYPE_DESC *desc = &context.typeDefs()[type_id];

  /* check for correct type */
  if (! ddd_TypeDefined(desc))
    DUNE_THROW(Dune::Exception, "undefined DDD_TYPE");


  /* create prioMatrix on demand */
  if (desc->prioMatrix == nullptr)
  {
    if (! SetPrioMatrix(desc, PRIOMERGE_DEFAULT))
      DUNE_THROW(Dune::Exception, "error for DDD_TYPE " << type_id);
  }


  /* check input priorities */

  if (p1>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "invalid priority p1=" << p1);
  if (p2>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "invalid priority p2=" << p2);
  if (pres>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "invalid priority pres=" << pres);


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
    DUNE_THROW(Dune::Exception,
               "error(s) in merge-check for DDD_TYPE " << type_id);
}


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
    DUNE_THROW(Dune::Exception, "undefined DDD_TYPE");

  if (p1>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "invalid priority p1=" << p1);
  if (p2>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "invalid priority p2=" << p2);

  if (PriorityMerge(desc, p1, p2, &newprio) == PRIO_ERROR)
    DUNE_THROW(Dune::Exception, "cannot merge priorities");

  return newprio;
}


/****************************************************************************/

static const char* prioMergeDefaultName(int prioDefault)
{
  switch (prioDefault) {
  case PRIOMERGE_MAXIMUM:
    return "MAX";
  case PRIOMERGE_MINIMUM:
    return "MIN";
  default:
    return "(ERROR)";
  }
}

void DDD_PrioMergeDisplay (DDD::DDDContext& context, DDD_TYPE type_id)
{
  std::ostream& out = std::cout;
  TYPE_DESC* desc = &context.typeDefs()[type_id];
  int r, c, changed_rows[MAX_PRIO];

  if (me!=0)
    return;

  /* check for correct type */
  if (! ddd_TypeDefined(desc))
    DUNE_THROW(Dune::Exception, "undefined DDD_TYPE");

  out << "/ PrioMergeDisplay for '" << desc->name << "', default mode "
      << prioMergeDefaultName(desc->prioDefault) << "\n";

  if (desc->prioMatrix == nullptr)
  {
    out << "\\ \t(no special cases defined)\n";
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
  using std::setw;
  out << "|\t     ";
  for(c=0; c<MAX_PRIO; c++)
  {
    if (! changed_rows[c])
      continue;

    out << " " << setw(3) << c << "  ";
  }
  out << "\n";

  for(r=0; r<MAX_PRIO; r++)
  {
    if (! changed_rows[r])
      continue;

    out << "|\t" << setw(2) << r << " :  ";
    for(c=0; c<MAX_PRIO; c++)
    {
      DDD_PRIO p_dflt, p_actual;

      if (! changed_rows[c])
        continue;

      PM_GETDEFAULT(desc->prioDefault, r, c, p_dflt);
      PriorityMerge(desc, r, c, &p_actual);

      if (p_dflt != p_actual)
        out << " " << setw(3) << p_actual << "  ";
      else
        out << "(" << setw(3) << p_actual << ") ";
    }

    out << "\n";
  }

  out << "\\\n";
}

/****************************************************************************/

END_UGDIM_NAMESPACE
