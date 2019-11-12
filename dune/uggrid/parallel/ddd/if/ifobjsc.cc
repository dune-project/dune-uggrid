// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ifobjsc.c                                                     */
/*                                                                          */
/* Purpose:   routines concerning interfaces between processors             */
/*            part 4: routines for creation and management of               */
/*                    object shortcut tables                                */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   96/01/24 kb  copied from ifcreate.c                           */
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

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "if.h"

USING_UG_NAMESPACE

START_UGDIM_NAMESPACE

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


/*
        convert cpl-IF-table into obj-IF-table
 */
static void IFComputeShortcutTable(DDD::DDDContext& context, DDD_IF ifId)
{
  auto& theIF = context.ifCreateContext().theIf;

  int nItems = theIF[ifId].nItems;
  COUPLING  **cpls = theIF[ifId].cpl;
  IFObjPtr   *objs = theIF[ifId].obj;

  /* mark obj-shortcut-table as valid */
  theIF[ifId].objValid = true;

  if (nItems==0)
    return;

  /* fill in object pointers, this is the 4-fold indirection step */
  for(int i = 0; i < nItems; ++i)
  {
    objs[i] = OBJ_OBJ(context, cpls[i]->obj);
  }
}


/****************************************************************************/


/*
        create direct link from ifHead and ifAttr to objects,
        avoid one indirect addressing step across couplings.
        each cpl-entry in an interface has one corresponding obj-entry
 */
void IFCreateObjShortcut(DDD::DDDContext& context, DDD_IF ifId)
{
  auto& theIF = context.ifCreateContext().theIf;

  COUPLING    **cplarray = theIF[ifId].cpl;
  IFObjPtr     *objarray;
  IF_PROC     *ifHead;

  /* dont create shortcuts for STD_INTERFACE */
  if (ifId==STD_INTERFACE)
    return;

  /* are there any items? */
  if (theIF[ifId].nItems == 0)
    return;

  /* get memory for addresses of objects inside IF */
  objarray = (IFObjPtr *) AllocIF(sizeof(IFObjPtr)*theIF[ifId].nItems);
  if (objarray==NULL)
    throw std::bad_alloc();

  theIF[ifId].obj = objarray;

  IFComputeShortcutTable(context, ifId);


  ForIF(context, ifId, ifHead)
  {
    IF_ATTR  *ifAttr;

    /* compute pointers to subarrays */
    ifHead->obj    = objarray + (ifHead->cpl    - cplarray);
    ifHead->objAB  = objarray + (ifHead->cplAB  - cplarray);
    ifHead->objBA  = objarray + (ifHead->cplBA  - cplarray);
    ifHead->objABA = objarray + (ifHead->cplABA - cplarray);


    /* compute pointers from ifAttrs to subarrays */
    for(ifAttr=ifHead->ifAttr; ifAttr!=0; ifAttr=ifAttr->next)
    {
      ifAttr->objAB  = objarray + (ifAttr->cplAB  - cplarray);
      ifAttr->objBA  = objarray + (ifAttr->cplBA  - cplarray);
      ifAttr->objABA = objarray + (ifAttr->cplABA - cplarray);
    }
  }
}


/****************************************************************************/


/*
        if object addresses in memory are changed, then the shortcut-tables
        will get invalid. this routine does the invalidation.
 */
void IFInvalidateShortcuts(DDD::DDDContext& context, DDD_TYPE invalid_type)
{
  auto& theIF = context.ifCreateContext().theIf;
  const auto& nIFs = context.ifCreateContext().nIfs;

  /* test all interfaces */
  for(int i = 0; i < nIFs; ++i)
  {
    if (i==STD_INTERFACE)
      continue;

    if (theIF[i].objValid)
    {
      /* determine whether object belongs to IF */
      if ((1<<invalid_type) & theIF[i].maskO)
      {
        /* yes, invalidate interface */
        theIF[i].objValid = false;
      }
    }
  }
}



/****************************************************************************/


/*
        check if shortcut-table is valid and recompute, if necessary
 */
void IFCheckShortcuts (DDD::DDDContext& context, DDD_IF ifId)
{
  if (ifId==STD_INTERFACE)
    return;

  auto& theIF = context.ifCreateContext().theIf;
  if (! theIF[ifId].objValid)
  {
    /*
       sprintf(cBuffer, "%04d: IFComputeShortcutTable IF=%d\n", context.me(), ifId);
       DDD_PrintDebug(cBuffer);
     */

    IFComputeShortcutTable(context, ifId);
  }
}

/****************************************************************************/

END_UGDIM_NAMESPACE
