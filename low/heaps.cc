// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      heaps.c                                                       */
/*                                                                          */
/* Purpose:   low-level memory management for ug 2.0                        */
/*                                                                          */
/* Author:      Peter Bastian/Henrik Rentz-Reichert                         */
/*              Institut fuer Computeranwendungen III                       */
/*              Universitaet Stuttgart                                      */
/*              Pfaffenwaldring 27                                          */
/*              70569 Stuttgart                                             */
/*              email: ug@ica3.uni-stuttgart.de                             */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*              02.02.95 begin, ug version 3.0                              */
/*                                                                          */
/* Revision:  04.09.95                                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files in the order                                               */
/*              system include files                                        */
/*              application include files                                   */
/*                                                                          */
/****************************************************************************/

#include <config.h>

#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <cstdio>

#include "ugtypes.h"
#include "architecture.h"
#include "heaps.h"
#include "misc.h"
#include "general.h"
#include "debug.h"
#include <dev/ugdevices.h>

#include "namespace.h"
USING_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*          compile time constants defining static data size (i.e. arrays)  */
/*          other constants                                                 */
/*          macros                                                          */
/*                                                                          */
/****************************************************************************/

#define FLOOR(n)    ((n)&ALIGNMASK)     /* lower next multiple of four */
#define CEIL(n)          ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))


/* defines and macros for the virtual heap management                        */

#define B_OFFSET(bhm,i)         ((bhm)->BlockDesc[i].offset)
#define B_SIZE(bhm,i)            ((bhm)->BlockDesc[i].size)
#define B_ID(bhm,i)             ((bhm)->BlockDesc[i].id)

#define CALC_B_OFFSET(bhm,i)    (((i)==0) ? 0 : (B_OFFSET(theVHM,(i)-1)+B_SIZE(theVHM,(i)-1)))

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

REP_ERR_FILE

/****************************************************************************/
/** \brief Install a new heap structure

   \param type - type of heap
   \param size - size of new heap in bytes
   \param buffer - 4-aligned memory for the heap

   This function installs a new heap structure.
   Valid 'type' is either 'SIMPLE_HEAP' or 'GENERAL_HEAP'.
   The allocation of memory starts at the address given by
   '*buffer' and is of 'size' bytes.

   \return <ul>
   <li>    pointer to HEAP </li>
   <li>    NULL if not enough space available </li>
   </ul>
 */
/****************************************************************************/

HEAP *NS_PREFIX NewHeap (enum HeapType type, MEM size, void *buffer)
{
  HEAP *theHeap;

  /* check size */
  if (buffer==NULL) return(NULL);
  if (size<MIN_HEAP_SIZE) return(NULL);

  /* initialize heap structure */
  theHeap = (HEAP *) buffer;
  theHeap->type = type;
  theHeap->size = size;
  theHeap->markKey = 0;

  /* No constructor is ever called for theHeap.  Consequently, no constructor
   * has been called for its member markedMemory, either.  Here we force this
   * constructor call using placement new. */
  new(theHeap->markedMemory) std::vector<void*>[MARK_STACK_SIZE+1];

  /* return heap structure */
  return(theHeap);
}

/****************************************************************************/
/** \brief Clean up and deallocate a heap structure

   \param theHeap The heap to be deallocated

   This method properly cleans up a HEAP data structure, and then frees the
   memory.
 */
/****************************************************************************/



void NS_PREFIX DisposeHeap (HEAP *theHeap)
{
  if (theHeap != NULL) {
    /* When using the system heap, the HEAP data structure contains an array of
     * std::vectors, which have all be created using placement new.  Therefore,
     * before freeing the memory of a HEAP we have to call the destructors of
     * these std::vectors explicitly.  Otherwise we get memory leaks.
     */
    using namespace std;
    for (INT i=0; i<MARK_STACK_SIZE; i++)
      theHeap->markedMemory[i].~vector<void*>();

    free(theHeap);
  }
}

/****************************************************************************/
/** \brief Allocate memory from heap

   \param theHeap - heap structure which manages memory allocation
   \param n - number of bytes to allocate

   this function now only forwards to `malloc`.
*/
void *NS_PREFIX GetMem (HEAP *theHeap, MEM n)
{
  return malloc(n);
}

/****************************************************************************/
/** \brief Allocate memory and register it for rollback

   \param theHeap - heap structure which manages memory allocation
   \param n - number of bytes to allocate
   \param key - key with which we can identify the rollback record

   this function allocates memory on the heap and tags it with the `key`.
*/
void *NS_PREFIX GetTmpMem (HEAP *theHeap, MEM n, INT key)
{
  if (theHeap->type==SIMPLE_HEAP)
  {
    ASSERT(key == theHeap->markKey);
    /* we have to keep track of allocated memory, in order to do a proper rollback */
    void* ptr = GetMem(theHeap,n);
    theHeap->markedMemory[key].push_back(ptr);
    return theHeap->markedMemory[key].back();
  }
  /* no key for GENERAL_HEAP */
  return (GetMem(theHeap,n));
}

/****************************************************************************/
/** \brief Free memory previously allocated from that heap

   \param theHeap - heap structure which manages memory allocation
   \param buffer - memory area previously allocated

   This function creates free memory previously allocated from that heap.
 */
/****************************************************************************/

void NS_PREFIX DisposeMem (HEAP *theHeap, void *buffer)
{
  free(buffer);
}

/****************************************************************************/
/** \copydoc Allocate memory from heap

   \param theHeap - heap structure which manages memory allocation
   \param size - number of bytes to allocate

   \note the allocated memory is initialized to zero

   \return pointer to an object of the requested size
 */
/****************************************************************************/

void *NS_PREFIX GetFreelistMemory (HEAP *theHeap, INT size)
{
  void* obj = malloc(size);

  if (obj != NULL)
    memset(obj,0,size);

  return(obj);
}

/****************************************************************************/
/** \brief Mark heap position for future release

   \param theHeap - heap to mark

   This function marks heap position for future release. Only valid in
   the 'SIMPLE_HEAP' type.

   \return <ul>
   <li>   0 if OK </li>
   <li>   1 if mark stack full or wrong heap type </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX MarkTmpMem (HEAP *theHeap, INT *key)
{
  assert(theHeap->type==SIMPLE_HEAP);
  if (theHeap->type!=SIMPLE_HEAP) return(1);

  if(theHeap->markKey >= MARK_STACK_SIZE)
    return 1;
  theHeap->markKey++;
  *key = theHeap->markKey;
  return 0;
}

/****************************************************************************/
/** \brief Release to next stack position

   \param theHeap - heap to release

   This function releases to the next stack position. Only valid in the
   'SIMPLE_HEAP' type.

   \return <ul>
   <li>   0 if OK </li>
   <li>   1 if mark stack empty or wrong heap type. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX ReleaseTmpMem (HEAP *theHeap, INT key)
{
  if (theHeap->type!=SIMPLE_HEAP) return 1;

  if (theHeap->markKey == 0) return 0;
  if (key > theHeap->markKey) return 1;

  /* Free all memory associated to 'key' */
  for (void* ptr : theHeap->markedMemory[key])
    free(ptr);
  theHeap->markedMemory[key].resize(0);

  if (key < theHeap->markKey) return 2;
  while (theHeap->markKey > 0 && theHeap->markedMemory[theHeap->markKey].size() == 0)
    theHeap->markKey--;

  return 0;
}
