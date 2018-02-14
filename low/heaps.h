// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file heaps.h
 * \ingroup low
 */

/** \addtogroup low
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      heaps.h                                                       */
/*                                                                          */
/* Purpose:   low-level memory management for ug                            */
/*                                                                          */
/* Author:      Peter Bastian                                               */
/*              Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen */
/*              Universitaet Heidelberg                                     */
/*              Im Neuenheimer Feld 368                                     */
/*              6900 Heidelberg                                             */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*                                                                          */
/* Revision:  04.09.95                                                      */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __HEAPS__
#define __HEAPS__

#include <vector>

#include "ugtypes.h"
#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*          compile time constants defining static data size (i.e. arrays)  */
/*          other constants                                                 */
/*          macros                                                          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* defines for the simple and general heap management                       */
/****************************************************************************/

/** \brief Smallest heap to allocate       */
#define MIN_HEAP_SIZE   256
/** \brief Max depth of mark/release calls */
#define MARK_STACK_SIZE 128

enum HeapType {GENERAL_HEAP,                  /**< Heap with alloc/free mechanism  */
               SIMPLE_HEAP         /**< Heap with mark/release mechanism*/
};

enum HeapAllocMode
{FROM_TOP=1,                       /**< Allocate from top of stack      */
 FROM_BOTTOM=2                       /**< Allocate from bottom of stack   */
};

/* by convention, tempory memory on a simple heap should allocated FROM_TOP */
/* the Freelist memory is allocated FROM_BOTTOM                             */

#define MarkTmpMem(p,kp)     Mark(p,FROM_TOP,kp)
#define GetTmpMem(p,n,k)         GetMemUsingKey(p,n,FROM_TOP,k)
#define ReleaseTmpMem(p,k)       Release(p,FROM_TOP,k)

/****************************************************************************/
/****************************************************************************/
/** @name Defines and macros for the virtual heap management                 */

/** \brief That many blocks can be allocated   */
#define MAXNBLOCKS         50

/** \brief Pass to init routine if no heap yet */
#define SIZE_UNKNOWN        0

/** \brief Ok return code for virtual heap mgmt*/
#define BHM_OK              0

/** \brief Return codes of DefineBlock */
enum {HEAP_FULL =           1,           /**< Return code if storage exhausted    */
      BLOCK_DEFINED =       2,           /**< Return code if block already defined*/
      NO_FREE_BLOCK =       3           /**< Return code if no free block found  */
};

/* return codes of FreeBlock */
/** \brief Return code if the block is not defined */
#define BLOCK_NOT_DEFINED    1

/* @} */
/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

typedef unsigned long MEM;

/****************************************************************************/
/* structs and typedefs for the simple and general heap management          */
/****************************************************************************/

typedef struct {
  enum HeapType type;
  MEM size;
  INT markKey;

  /* This is used only if UG_USE_SYSTEM_HEAP is set, but I don't want the
   * #ifdef in an installed header, hence the data member is there all the time. */
  std::vector<void*> markedMemory[MARK_STACK_SIZE+1];

  /* These were global variables needed for DYNAMIC_MEMORY_ALLOCMODEL

     we try to avoid global state, as it does not play well with multiple UG instances.
   */
  INT usefreelistmemory;
  INT freelist_end_mark;

} HEAP;

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/** @name Functions for the simple and general heap management */
/* @{ */
HEAP        *NewHeap                (enum HeapType type, MEM size, void *buffer);
void         DisposeHeap            (HEAP *theHeap);
void        *GetMem                 (HEAP *theHeap, MEM n, enum HeapAllocMode mode);
void        *GetMemUsingKey         (HEAP *theHeap, MEM n, enum HeapAllocMode mode, INT key);
void         DisposeMem             (HEAP *theHeap, void *buffer);

void        *GetFreelistMemory      (HEAP *theHeap, INT size);
INT          PutFreelistMemory      (HEAP *theHeap, void *object, INT size);

INT          Mark                   (HEAP *theHeap, INT mode, INT *key);
INT          Release                (HEAP *theHeap, INT mode, INT key);
/* @} */

END_UG_NAMESPACE

/** @} */

#endif
