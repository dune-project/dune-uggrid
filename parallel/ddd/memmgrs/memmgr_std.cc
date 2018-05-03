// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      memmgr_std.c                                                  */
/*                                                                          */
/* Purpose:   basic memory management module                                */
/*            (with standard malloc() calls)                                */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/04/27 kb  begin                                            */
/*            96/01/20 kb  updated to DDD V1.5                              */
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

#include "ppif.h"


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

START_UGDIM_NAMESPACE

void *memmgr_AllocPMEM (size_t size)
{
  return std::malloc(size);
}


void memmgr_FreePMEM (void *buffer)
{
  std::free(buffer);
}




void *memmgr_AllocOMEM (size_t size, int ddd_typ, int proc, int attr)
{
  return std::malloc(size);
}


void memmgr_FreeOMEM (void *buffer, size_t size, int ddd_typ)
{
  std::free(buffer);
}




void *memmgr_AllocAMEM (size_t size)
{
  return std::malloc(size);
}


void memmgr_FreeAMEM (void *buffer)
{
  std::free(buffer);
}


void *memmgr_AllocTMEM (size_t size)
{
  return std::malloc(size);
}


void memmgr_FreeTMEM (void *buffer)
{
  std::free(buffer);
}


END_UGDIM_NAMESPACE
