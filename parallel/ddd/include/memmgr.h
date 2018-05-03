// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      memmgr.h                                                      */
/*                                                                          */
/* Purpose:   header file for ddd memory management interface               */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/*                                                                          */
/* History:   94/04/27 kb  begin                                            */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __MEMMGR__
#define __MEMMGR__

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

void *memmgr_AllocOMEM (size_t size, int Typeid, int prio, int attr);
void  memmgr_FreeOMEM (void *mem, size_t size, int Typeid);

void *memmgr_AllocPMEM (long unsigned int size);
void  memmgr_FreePMEM (void *mem);

void *memmgr_AllocAMEM (long unsigned int size);
void  memmgr_FreeAMEM (void *mem);

void *memmgr_AllocTMEM (long unsigned int size, int kind);
void  memmgr_FreeTMEM (void *mem, int kind);

END_UGDIM_NAMESPACE

#endif
