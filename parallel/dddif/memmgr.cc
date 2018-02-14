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

#include "ugtypes.h"
#include "heaps.h"
#include "misc.h"
#include <dev/ugdevices.h>

#include "parallel.h"
#include "general.h"

#include "ugm.h"
#include "memmgr.h"

#include "namespace.h"

/* UG namespaces */
USING_UG_NAMESPACES

/* PPIF namespace */
using namespace PPIF;

  START_UGDIM_NAMESPACE

#define HARD_EXIT abort()
/*#define HARD_EXIT exit(1)*/


/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/


static INT allocated=0;
static size_t pmem=0;
static size_t amem=0;
static size_t tmem=0;

static size_t mem_from_ug_freelists=0;


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*
   memmgr_Report -

   SYNOPSIS:
   void memmgr_Report (void);

   PARAMETERS:
   .  void

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_Report (void)
{
  UserWriteF("%04d memmgr_Report.  Memory from UG's freelists: %9ld\n",
             me, mem_from_ug_freelists);

  fflush(stdout);
}

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
  void   *buffer;

  buffer = GetMemoryForObject(dddctrl.currMG,size,MAOBJ);

  /*
     printf("%4d: memmgr_AllocOMem: size=%05d ddd_type=%02d prio=%d attr=%d\n",
     me,size,ddd_type,prio,attr);
   */

  return(buffer);
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
  /*
     printf("%d: memmgr_FreeOMEM(): buffer=%x, ddd_type=%d\n", me, buffer, ddd_type);
   */

  PutFreeObject(dddctrl.currMG,buffer,size,MAOBJ);
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
  void   *buffer;

  buffer = malloc(size);
  allocated += size;
  pmem      +=size;

  return(buffer);
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
  free(buffer);
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
  void   *buffer;

  buffer = malloc(size);

  allocated += size;
  amem      += size;

  return(buffer);
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
  free(buffer);
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
  void   *buffer;


  if (kind==TMEM_XFER || kind==TMEM_CPL ||
      kind==TMEM_LOWCOMM || kind==TMEM_CONS || kind==TMEM_IDENT)
  {
    size_t real_size = size+sizeof(size_t);

    buffer = GetMemoryForObject(dddctrl.currMG,real_size,MAOBJ);
    if (buffer!=NULL)
    {
      /* store size at the beginning of memory chunk */
      *(size_t *)buffer = real_size;

      /* hide this information */
      buffer = (void *)(((char *)buffer) + sizeof(size_t));

      mem_from_ug_freelists += real_size;

      /*
         printf("%4d:    X MEMM adr=%08x kind=%d size=%ld\n", me,
                      buffer, kind, size);
       */
    }
  }
  else
  {
    buffer = malloc(size);

    allocated += size;
    tmem      += size;

    /*
       printf("%4d:    O MEMM adr=%08x kind=%d size=%ld\n", me,
                    buffer, kind, size);
     */
  }

  return(buffer);
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
  if (kind==TMEM_XFER || kind==TMEM_CPL ||
      kind==TMEM_LOWCOMM || kind==TMEM_CONS || kind==TMEM_IDENT)
  {
    size_t real_size;

    /*
       printf("%4d:    X MEMF adr=%08x kind=%d\n", me, buffer, kind);
     */

    /* get real_size from beginning of buffer */
    buffer = (void *)(((char *)buffer) - sizeof(size_t));
    real_size = *(size_t *)buffer;

    PutFreeObject(dddctrl.currMG,buffer,real_size,MAOBJ);

    /*
       mem_from_ug_freelists -= real_size;
     */
  }
  else
  {
    free(buffer);
  }
}


/****************************************************************************/

void memmgr_MarkHMEM (long *theMarkKey)
{
#ifdef UG_USE_SYSTEM_HEAP
  // actually we would like to use the system heap here, but this all goes via the Key magick...
  // return;
#endif
  INT myMarkKey;
  MarkTmpMem(MGHEAP(dddctrl.currMG), &myMarkKey);
  *theMarkKey = (long)myMarkKey;
}

void* memmgr_AllocHMEM (size_t size, long theMarkKey)
{
  void *buffer;
#ifdef UG_USE_SYSTEM_HEAP
  // actually we would like to use the system heap here, but this all goes via the Key magick...
  // return malloc(size);
#endif
  buffer = GetTmpMem(MGHEAP(dddctrl.currMG), size, (INT)theMarkKey);

  /*
     printf("%4d:    H MEMM adr=%08x           size=%ld\n", me, buffer, size);
   */

  return(buffer);
}

void memmgr_ReleaseHMEM (long theMarkKey)
{
#ifdef UG_USE_SYSTEM_HEAP
  // actually we would like to use the system heap here, but this all goes via the Key magick...
  // free(ptr);
#endif
  ReleaseTmpMem(MGHEAP(dddctrl.currMG), (INT)theMarkKey);
}


/****************************************************************************/
/*
   memmgr_Init -

   SYNOPSIS:
   void memmgr_Init (void);

   PARAMETERS:
   .  void

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void memmgr_Init (void)
{}


/****************************************************************************/

END_UGDIM_NAMESPACE

#endif /* ModelP */
