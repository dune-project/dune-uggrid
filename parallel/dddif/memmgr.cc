// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      memmgr.c                                                      */
/*                                                                          */
/* Purpose:   memory management module                                      */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/04/27 kb  begin                                            */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

#ifdef ModelP

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

#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>
#include <dev/ugdevices.h>

#include "parallel.h"

#include <gm/ugm.h>
#include "memmgr.h"

/* UG namespaces */
USING_UG_NAMESPACES

/* PPIF namespace */
using namespace PPIF;

  START_UGDIM_NAMESPACE

#define HARD_EXIT abort()
/*#define HARD_EXIT exit(1)*/


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*
   memmgr_AllocOMEM -

   SYNOPSIS:
   void *memmgr_AllocOMEM (size_t size, int ddd_type, int prio, int attr);

   PARAMETERS:
   .  size
   .  ddd_type
   .  prio
   .  attr

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void * memmgr_AllocOMEM (size_t size, int ddd_type, int prio, int attr)
{
  void* p = std::malloc(size);
  std::memset(p, 0, size);
  return p;
}


/****************************************************************************/
/*
   memmgr_FreeOMEM -

   SYNOPSIS:
   void memmgr_FreeOMEM (void *buffer, size_t size, int ddd_type);

   PARAMETERS:
   .  buffer
   .  size
   .  ddd_type

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_FreeOMEM (void *buffer, size_t size, int ddd_type)
{
  std::free(buffer);
}


/****************************************************************************/
/*
   memmgr_AllocPMEM -

   SYNOPSIS:
   void *memmgr_AllocPMEM (unsigned long size);

   PARAMETERS:
   .  size

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void * memmgr_AllocPMEM (unsigned long size)
{
  return std::malloc(size);
}


/****************************************************************************/
/*
   memmgr_FreePMEM -

   SYNOPSIS:
   void memmgr_FreePMEM (void *buffer);

   PARAMETERS:
   .  buffer

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_FreePMEM (void *buffer)
{
  std::free(buffer);
}



/****************************************************************************/
/*
   memmgr_AllocAMEM -

   SYNOPSIS:
   void *memmgr_AllocAMEM (unsigned long size);

   PARAMETERS:
   .  size

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void * memmgr_AllocAMEM (unsigned long size)
{
  return std::malloc(size);
}


/****************************************************************************/
/*
   memmgr_FreeAMEM -

   SYNOPSIS:
   void memmgr_FreeAMEM (void *buffer);

   PARAMETERS:
   .  buffer

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_FreeAMEM (void *buffer)
{
  std::free(buffer);
}


/****************************************************************************/
/*
   memmgr_AllocTMEM -

   SYNOPSIS:
   void *memmgr_AllocTMEM (unsigned long size);

   PARAMETERS:
   .  size

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void * memmgr_AllocTMEM (unsigned long size, int kind)
{
  void* p = std::malloc(size);
  std::memset(p, 0, size);
  return p;
}


/****************************************************************************/
/*
   memmgr_FreeTMEM -

   SYNOPSIS:
   void memmgr_FreeTMEM (void *buffer);

   PARAMETERS:
   .  buffer

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_FreeTMEM (void *buffer, int kind)
{
  std::free(buffer);
}


END_UGDIM_NAMESPACE

#endif /* ModelP */
