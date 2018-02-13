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
  INT i;

  /* check size */
  if (buffer==NULL) return(NULL);
  if (size<MIN_HEAP_SIZE) return(NULL);

  /* initialize heap structure */
  theHeap = (HEAP *) buffer;
  theHeap->type = type;
  theHeap->size = size;
  theHeap->topStackPtr = theHeap->bottomStackPtr = 0;
  theHeap->heapptr = (BLOCK *) CEIL(((MEM)theHeap)+sizeof(HEAP));

  /* initialize first block */
  theHeap->heapptr->size = ((MEM)theHeap)+size-((MEM)theHeap->heapptr);
  theHeap->heapptr->next = theHeap->heapptr;
  theHeap->heapptr->previous = theHeap->heapptr;

  /* No constructor is ever called for theHeap.  Consequently, no constructor
   * has been called for its member markedMemory, either.  Here we force this
   * constructor call using placement new. */
  new(theHeap->markedMemory) std::vector<void*>[MARK_STACK_SIZE];

  /* initialize data variables needed for bottom tmp memory management */
  theHeap->usefreelistmemory = 1;
  theHeap->freelist_end_mark = 0;


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
  }

  free(theHeap);
}

/****************************************************************************/
/** \brief Allocate memory from heap, depending on heap type

   \param theHeap - heap to allocate from
   \param n - number of bytes to allocate
   \param mode - allocation position for mark/release heap

   This function allocates memory from 'HEAP', depending on heap type.

   If the heap type (theHeap->type) is 'SIMPLE_HEAP' new blocks in the
   allocated memory can be taken either from the top (mode = 'FROM_TOP')
   or from the bottom (mode = 'FROM_BOTTOM') of this block. In other words
   the total memory block to be provided for ug is used from both sides by
   introducing recursively new, smaller blocks.

   \verbatim
      --------------------------------
 | 1 | 3 |  4  |           |  2 |   total allocated memory for ug
      --------------------------------
      used blocks are #1,#2,#3 and #4

      -----
 ||5|6|   block #4 is separated also in block #5 and #6
      -----
   \endverbatim

   If the heap type (theHeap->type) is 'GENERAL_HEAP' new blocks to be
   introduced in the total allocated memory for ug are laid at the position
   where enough memory is free. The search for this position is done by
   running round the memory and looking for the equivalent space.

   \verbatim
      --------------------------------
 |   |  1  |               |  2 |   total allocated memory for ug
      --------------------------------
      used blocks are #1 and #2

      block to be introduced #3:  |  3  |
      start position at the beginning

      --------------------------------
 |   |  1  |  3  |         |  2 |   total allocated memory for ug
      --------------------------------
   \endverbatim

   \return <ul>
   <li>      NULL pointer                   if error occurs </li>
   <li>      'theBlock'                     if OK by type of 'SIMPLE_HEAP' </li>
   <li>      '((char *)newBlock)+ALIGNMENT' or </li>
   <li>      '((char *)theBlock)+ALIGNMENT' if OK by type of 'GENERAL_HEAP' </li>
   </ul>
 */
/****************************************************************************/

void *NS_PREFIX GetMem (HEAP *theHeap, MEM n, enum HeapAllocMode mode)
{
  return malloc(n);
}

void *NS_PREFIX GetMemUsingKey (HEAP *theHeap, MEM n, enum HeapAllocMode mode, INT key)
{
  if (theHeap->type==SIMPLE_HEAP)
  {
    /* and first some error checks */
    ASSERT(
      /* when allocating, we require, that the corresponding TOP or BOTTOM mark is set */
      (mode==FROM_TOP && theHeap->topStackPtr>0)
      ||
      (mode==FROM_BOTTOM && theHeap->bottomStackPtr>0)
      );
    /* next we require that in the corresponding mode the key matches the stack-ptr.
       otherwise one of the folloing errors has happened:
         a) TOP:
            key > topStackPtr: Mark/Release calls not balanced
            key < topStackPtr: stack pos already released
         b) BOTTOM:
            key > bottomStackPtr: Mark/Release calls not balanced
            key < bottomStackPtr: stack pos already released
      */
    ASSERT(
      (mode==FROM_TOP && key == theHeap->topStackPtr)
      ||
      (mode==FROM_BOTTOM && key == theHeap->bottomStackPtr)
      );
    /* we have to keep track of allocated memory, in order to do a proper rollback */
    void* ptr = GetMem(theHeap,n,mode);
    theHeap->markedMemory[key].push_back(ptr);
    return theHeap->markedMemory[key].back();
  }
  /* no key for GENERAL_HEAP */
  return (GetMem(theHeap,n,mode));
}

/****************************************************************************/
/** \brief Free memory previously allocated from that heap

   \param theHeap - heap from which memory has been allocated
   \param buffer - memory area previously allocated

   This function creates free memory previously allocated from that heap.
   This function is only valid for a heap of type GENERAL_HEAP.

 */
/****************************************************************************/

void NS_PREFIX DisposeMem (HEAP *theHeap, void *buffer)
{
  free(buffer);
}

/****************************************************************************/
/** \brief Get an object from free list if possible

   \param theHeap - pointer to Heap
   \param size - size of the object

   This function gets an object of type `type` from free list if possible,
   otherwise it allocates memory from the heap using 'GetMem'.

   \return <ul>
   <li> pointer to an object of the requested type </li>
   <li> NULL if object of requested type is not available </li>
   </ul>
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
/** \brief Put an object in the free list

   \param theHeap - pointer to Heap
   \param object - object to insert in free list
   \param size - size of the object

   This function puts an object in the free list.

   \return <ul>
   <li> 0 if ok </li>
   <li> 1 when error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX PutFreelistMemory (HEAP *theHeap, void *object, INT size)
{
  free(object);
  return 0;
}

/****************************************************************************/
/** \brief Mark heap position for future release

   \param theHeap - heap to mark
   \param mode - 'FROM_TOP' or 'FROM_BOTTOM' of the block

   This function marks heap position for future release. Only valid in
   the 'SIMPLE_HEAP' type.

   \return <ul>
   <li>   0 if OK </li>
   <li>   1 if mark stack full or wrong heap type </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX Mark (HEAP *theHeap, INT mode, INT *key)
{
  if (theHeap->type!=SIMPLE_HEAP) return(1);

  if (mode==FROM_TOP)
  {
    if (theHeap->topStackPtr<MARK_STACK_SIZE)
    {
      theHeap->topStack[theHeap->topStackPtr++] =
        ((MEM)theHeap->heapptr) + ((MEM)theHeap->heapptr->size);
      *key = theHeap->topStackPtr;
      return(0);
    }
  }
  if (mode==FROM_BOTTOM)
  {
    if (theHeap->bottomStackPtr<MARK_STACK_SIZE)
    {
      theHeap->bottomStack[theHeap->bottomStackPtr++] =
        ((MEM)theHeap->heapptr);
      *key = theHeap->bottomStackPtr;
      return(0);
    }
  }
  return(1);
}

/****************************************************************************/
/** \brief Release to next stack position

   \param theHeap - heap to release
   \param mode - 'FROM_TOP' or 'FROM_BOTTOM' of the block

   This function releases to the next stack position. Only valid in the
   'SIMPLE_HEAP' type.

   \return <ul>
   <li>   0 if OK </li>
   <li>   1 if mark stack empty or wrong heap type. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX Release (HEAP *theHeap, INT mode, INT key)
{
  MEM oldsize;
  MEM newsize;

  if (theHeap->type!=SIMPLE_HEAP) return(1);

  /* Free all memory associated to 'key' */
  for (size_t i=0; i<theHeap->markedMemory[key].size(); i++)
    free(theHeap->markedMemory[key][i]);
  theHeap->markedMemory[key].resize(0);

#warning do we need all this TOP/BOTTOM magick, when using system heap? I do not think so...
  if (mode==FROM_TOP)
  {
    if (theHeap->topStackPtr>0)
    {
      if (key>theHeap->topStackPtr)
      {
        /* Mark/Release calls not balanced */
        ASSERT(false);
        return(1);
      }
      if (key<theHeap->topStackPtr)
      {
        /* stack pos already released */
        ASSERT(false);
        return(2);
      }
      oldsize = theHeap->heapptr->size;
      newsize = theHeap->topStack[--theHeap->topStackPtr]-((MEM)theHeap->heapptr);
      theHeap->heapptr->size = newsize;
      return(0);
    }
    if (theHeap->topStackPtr==0)
      /* no memory in this heap ever allocated */
      return(0);
  }
  if (mode==FROM_BOTTOM)
  {
    if (theHeap->bottomStackPtr>0)
    {
      if (key>theHeap->bottomStackPtr)
      {
        /* Mark/Release calls not balanced */
        ASSERT(false);
        return(3);
      }
      if (key<theHeap->bottomStackPtr)
      {
        /* stack pos already released */
        ASSERT(false);
        return(4);
      }
      oldsize = theHeap->heapptr->size;
      newsize = (((MEM)theHeap->heapptr)+((MEM)theHeap->heapptr->size))
                -theHeap->bottomStack[--theHeap->bottomStackPtr];
      theHeap->heapptr = (BLOCK *) theHeap->bottomStack[theHeap->bottomStackPtr];
      theHeap->heapptr->size = newsize;
      return(0);
    }
    if (theHeap->bottomStackPtr==0)
      /* no memory in this heap ever allocated */
      return(0);
  }
  return(5);
}

/****************************************************************************/
/** \brief Initialize the VIRT_HEAP_MGMT data structure

   \param theVHM - pointer to the storage to initialize
   \param TotalSize - the total size of the heap to manage

   This function initializes the VIRT_HEAP_MGMT data structure that provides
   additional memory independently of the 'SIMPLE_HEAP' and 'GENERAL_HEAP' for
   further use by handling virtual heaps.

   \return <ul>
   <li>  'BHM_OK' if OK </li>
   <li>        99 if error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX InitVirtualHeapManagement (VIRT_HEAP_MGMT *theVHM, MEM TotalSize)
{
  if (theVHM==NULL)
    return (99);

  /* first clear everything */
  memset(theVHM,0,sizeof(VIRT_HEAP_MGMT));

  /* now init what is necessary */
  if (TotalSize==SIZE_UNKNOWN)
    theVHM->locked    = false;
  else
    theVHM->locked    = true;

  theVHM->TotalSize    = TotalSize;
  theVHM->TotalUsed    = 0;
  theVHM->UsedBlocks   = 0;
  theVHM->LargestGap   = 0;
  theVHM->nGaps        = 0;

  return (BHM_OK);
}

/****************************************************************************/
/** \brief Sum up the sizes of the blocks, set 'TotalSize' and
   lock it

   \param theVHM - pointer to the storage to init

   This function should be called while initializing virtual heaps. It
   sums up the sizes of all the heaps and returns the 'TotalSize' of all.

   \return <ul>
   <li>  'theVHM->TotalSize' if OK </li>
   <li>                   99 if error occurred. </li>
   </ul>
 */
/****************************************************************************/

MEM NS_PREFIX CalcAndFixTotalSize (VIRT_HEAP_MGMT *theVHM)
{
  if (theVHM==NULL)
    return (99);

  assert(theVHM->locked!=true);

  theVHM->locked        = true;
  theVHM->TotalSize    = theVHM->TotalUsed;
  theVHM->LargestGap    = 0;
  theVHM->nGaps        = 0;

  return (theVHM->TotalSize);
}

/****************************************************************************/
/** \brief Return a unique block ID starting with FIRST_BLOCK_ID

   This function returns a unique block ID starting with the FIRST_BLOCK_ID.

   \return
   'newID'
 */
/****************************************************************************/

BLOCK_ID NS_PREFIX GetNewBlockID ()
{
  static BLOCK_ID newID = 0;

  return (++newID);
}

/****************************************************************************/
/** \brief Return a pointer to the block descriptor with 'id'

   \param theVHM - pointer to the virtual heap management
   \param id - id of the desired block

   As the location of the block descriptors is not fixed in the heap
   management 'GetBlockDesc' returns the address of the block descriptor.

   \return <ul>
   <li>  NULL if not defined in theVHM </li>
   <li>  'theVHM->BlockDesc' pointer to block descriptor  </li>
   </ul>
 */
/****************************************************************************/

BLOCK_DESC *NS_PREFIX GetBlockDesc (VIRT_HEAP_MGMT *theVHM, BLOCK_ID id)
{
  INT i;

  if (theVHM==NULL)
    return (NULL);

  for (i=0; i<theVHM->UsedBlocks; i++)
    if (B_ID(theVHM,i)==id)
      break;

  if (i<theVHM->UsedBlocks)
    return (&(theVHM->BlockDesc[i]));
  else
    return (NULL);
}

/****************************************************************************/
/** \brief Set size and offset of a new block

   \param theVHM - pointer to the virtual heap management
   \param id - id of the block to define
   \param size - size to be allocated

   This function sets size and offset of a new block. It tries to fill gaps.

   \return <ul>
   <li>  'BHM_OK'        if OK </li>
   <li>  'HEAP_FULL'     if heap is full </li>
   <li>  'BLOCK_DEFINED' if block is already defined </li>
   <li>  'NO_FREE_BLOCK' if number of 'MAXNBLOCKS' reached </li>
   <li>   99             if error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX DefineBlock (VIRT_HEAP_MGMT *theVHM, BLOCK_ID id, MEM size)
{
  BLOCK_DESC *theBlock;
  MEM Gap,BestFitGap,LargestGap;
  INT i,BestFitGapPos;

  if (theVHM==NULL)
    return (99);

  size = CEIL(size);

  /* check size */
  if (theVHM->TotalSize!=SIZE_UNKNOWN)
    if (size>theVHM->TotalSize-theVHM->TotalUsed)
      return (HEAP_FULL);

  theBlock = GetBlockDesc(theVHM,id);

  if (theBlock!=NULL)
    /* block already is defined */
    return (BLOCK_DEFINED);

  if (theVHM->UsedBlocks>=MAXNBLOCKS)
    return (NO_FREE_BLOCK);

  if (theVHM->TotalSize==SIZE_UNKNOWN)
  {
    /* the size is unbounded: take first new block */
    i = theVHM->UsedBlocks;

    theVHM->TotalUsed    += size;
    theVHM->UsedBlocks    ++;

    B_ID(theVHM,i)        = id;
    B_SIZE(theVHM,i)    = size;
    B_OFFSET(theVHM,i)    = CALC_B_OFFSET(theVHM,i);

    return (BHM_OK);
  }

  /* the TotalSize is fixed */

  /* is there a gap large enough */
  if (theVHM->nGaps>0)
    if (size<theVHM->LargestGap)
    {
      /*    find the minimal gap large enough */
      BestFitGap = theVHM->LargestGap;
      Gap = B_OFFSET(theVHM,0);
      if ((Gap>=size) && (Gap<BestFitGap))
      {
        BestFitGap      = Gap;
        BestFitGapPos = 0;
      }
      for (i=1; i<theVHM->UsedBlocks; i++)
      {
        Gap = B_OFFSET(theVHM,i) - (B_OFFSET(theVHM,i-1) + B_SIZE(theVHM,i-1));
        if ((Gap>=size) && (Gap<BestFitGap))
        {
          BestFitGap      = Gap;
          BestFitGapPos = i;
        }
      }

      /* shift the descriptors one up */
      for (i=theVHM->UsedBlocks-1; i>BestFitGapPos; i--)
        theVHM->BlockDesc[i] = theVHM->BlockDesc[i-1];

      theVHM->nGaps--;

      theVHM->TotalUsed    += size;
      theVHM->UsedBlocks    ++;

      B_ID(theVHM,BestFitGapPos)        = id;
      B_SIZE(theVHM,BestFitGapPos)    = size;
      B_OFFSET(theVHM,BestFitGapPos)    = CALC_B_OFFSET(theVHM,BestFitGapPos);

      /* recalculate LargestGap? */
      if (BestFitGap==theVHM->LargestGap)
      {
        LargestGap = 0;
        for (i=0; i<theVHM->TotalUsed; i++)
          if (LargestGap<B_SIZE(theVHM,i))
            LargestGap = theVHM->BlockDesc[i].size;

        theVHM->LargestGap = LargestGap;
      }

      return (BHM_OK);
    }

  /* there is no gap large enough: take the next new block */
  i = theVHM->UsedBlocks;

  theVHM->TotalUsed    += size;
  theVHM->UsedBlocks    ++;

  B_ID(theVHM,i)        = id;
  B_SIZE(theVHM,i)    = size;
  B_OFFSET(theVHM,i)    = CALC_B_OFFSET(theVHM,i);

  return (BHM_OK);
}

/****************************************************************************/
/** \brief Free a block in the bhm defined before

   \param theVHM - pointer to the virtual heap management
   \param id - id of the block to free

   This function frees a block in the bhm defined before.

   \return <ul>
   <li>  'BHM_OK'            if OK </li>
   <li>  'BLOCK_NOT_DEFINED' if block is not defined, nothing to free </li>
   <li>   99                 if error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX FreeBlock (VIRT_HEAP_MGMT *theVHM, BLOCK_ID id)
{
  MEM NewGap;
  INT i,i_free;

  if (theVHM==NULL)
    return (99);

  for (i_free=0; i_free<theVHM->UsedBlocks; i_free++)
    if (B_ID(theVHM,i_free)==id)
      break;

  if (i_free>=theVHM->UsedBlocks)
    /* block is not defined, nothing to free */
    return (BLOCK_NOT_DEFINED);

  assert(theVHM->TotalUsed > B_SIZE(theVHM,i_free));

  theVHM->UsedBlocks--;
  theVHM->TotalUsed -= B_SIZE(theVHM,i_free);

  if (theVHM->TotalSize==SIZE_UNKNOWN)
  {
    /* shift the blocks one down and recalculate the offset */
    for (i=i_free; i<theVHM->UsedBlocks; i++)
    {
      theVHM->BlockDesc[i] = theVHM->BlockDesc[i+1];
      B_OFFSET(theVHM,i)     = CALC_B_OFFSET(theVHM,i);
    }

    return (BHM_OK);
  }

  /* the TotalSize is fixed */

  /* shift the blocks one down (don't change the offset!) */
  for (i=i_free; i<theVHM->UsedBlocks; i++)
    theVHM->BlockDesc[i] = theVHM->BlockDesc[i+1];

  /* new gap? */
  if (i_free<theVHM->UsedBlocks)
  {
    theVHM->nGaps++;

    NewGap = B_OFFSET(theVHM,i_free) - (B_OFFSET(theVHM,i_free-1) + B_SIZE(theVHM,i_free-1));
    if (theVHM->LargestGap<NewGap)
      theVHM->LargestGap = NewGap;
  }

  return (BHM_OK);
}
