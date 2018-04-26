// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      supp.c                                                        */
/*                                                                          */
/* Purpose:   support routines for Transfer Module                          */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin (xfer.c)                                   */
/*            95/03/21 kb  added variable sized objects (XferCopyObjX)      */
/*            95/04/05 kb  V1.3: extracted from xfer.c                      */
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
#include <cassert>

#include <new>

#include "dddi.h"

USING_UG_NAMESPACE

/*
        NOTE: all container-classes from ooppcc.h are implemented in this
              source file by setting the following define.
 */
#define ContainerImplementation
/* this is the hardliner version, for debugging only
   #define _CHECKALLOC(ptr)   assert(ptr!=NULL)
 */
#define _CHECKALLOC(ptr)   if (ptr==NULL) return (NULL)

START_UGDIM_NAMESPACE

static int TmpMem_kind = TMEM_ANY;

void *xfer_AllocTmp (size_t size)
{
  void *buffer = AllocTmpReq(size, TmpMem_kind);
  return(buffer);
}

void xfer_FreeTmp (void *buffer)
{
  FreeTmpReq(buffer, 0, TmpMem_kind);
}

void xfer_SetTmpMem (int kind)
{
  TmpMem_kind = kind;
}

END_UGDIM_NAMESPACE

#include "xfer.h"

START_UGDIM_NAMESPACE

void *xfer_AllocSend (size_t size)
{
  void *buffer = AllocTmpReq(size, TMEM_ANY);
  return(buffer);
}

void xfer_FreeSend (void *buffer)
{
  FreeTmpReq(buffer, 0, TMEM_ANY);
}



/****************************************************************************/
/*                                                                          */
/* definition of constants, macros                                          */
/*                                                                          */
/****************************************************************************/

#define ADDDATASEGM_SIZE 256
#define SIZESSEGM_SIZE 2048




/****************************************************************************/
/*                                                                          */
/* data types                                                               */
/*                                                                          */
/****************************************************************************/

END_UGDIM_NAMESPACE
namespace DDD {
namespace Xfer {

/* segment of AddDatas */
struct AddDataSegm
{
  AddDataSegm *next;
  int nItems;

  XFERADDDATA item[ADDDATASEGM_SIZE];
};


/* segment of AddData-Sizes */
struct SizesSegm
{
  SizesSegm   *next;
  int current;

  int data[SIZESSEGM_SIZE];
};

} /* namespace Xfer */
} /* namespace DDD */
START_UGDIM_NAMESPACE

using namespace DDD::Xfer;

/****************************************************************************/
/*                                                                          */
/* class member function implementations                                    */
/*                                                                          */
/****************************************************************************/

#define ClassName XICopyObj

/*
        compare-method in order to eliminate double XICopyObj-items.
        merge priorities from similar XICopyObj-items.

        the items are sorted according to key (dest,gid),
        all in ascending order. if dest and gid are equal,
        we merge priorities and get a new priority together with
        the information whether first item wins over second.
        in both cases, we use the new priority for next comparison.

        this implements rule XFER-C1.
 */
int Method(Compare) (ClassPtr item1, ClassPtr item2, const DDD::DDDContext* context)
{
  DDD_PRIO newprio;
  int ret;

  if (item1->dest < item2->dest) return(-1);
  if (item1->dest > item2->dest) return(1);

  if (item1->gid < item2->gid) return(-1);
  if (item1->gid > item2->gid) return(1);


  /* items have equal gid and dest, so they are considered as equal. */
  /* however, we must check priority, and patch both items with
     the new priority after merge. */
  ret = PriorityMerge(&context->typeDefs()[OBJ_TYPE(item1->hdr)],
                      item1->prio, item2->prio, &newprio);

  item1->prio = newprio;

  if (ret==PRIO_FIRST || ret==PRIO_UNKNOWN)
  {
    /* tell XferInitCopyInfo() that item is rejected */
    item2->prio = PRIO_INVALID;
  }
  else
  {
    /* communicate new priority to XferInitCopyInfo() */
    item2->prio = newprio;
  }

  return(0);
}


void Method(Print) (ParamThis _PRINTPARAMS)
{
  fprintf(fp, "XICopyObj dest=%d gid=" DDD_GID_FMT " prio=%d\n",
          This->dest, This->gid, This->prio);
}

#undef ClassName



/****************************************************************************/

#define ClassName XISetPrio

/*
        compare-method in order to eliminate double XISetPrio-items.
        merge priorities from similar XISetPrio-items.

        the items are sorted according to key (gid),
        all in ascending order. if both gids are equal,
        we merge priorities and get a new priority together with
        the information whether first item wins over second.
        in both cases, we use the new priority for next comparison.

        this implements rule XFER-P1.
 */
int Method(Compare) (ClassPtr item1, ClassPtr item2, const DDD::DDDContext* context)
{
  DDD_PRIO newprio;
  int ret;

  /* ascending GID is needed for ExecLocalXI___ */
  if (item1->gid < item2->gid) return(-1);
  if (item1->gid > item2->gid) return(1);


  /* items have equal gid and dest, so they are considered as equal. */
  /* however, we must check priority, and patch both items with
     the new priority after merge. */
  ret = PriorityMerge(&context->typeDefs()[OBJ_TYPE(item1->hdr)],
                      item1->prio, item2->prio, &newprio);

  item1->prio = item2->prio = newprio;


  if (ret==PRIO_FIRST || ret==PRIO_UNKNOWN)
  {
    /* tell XferInitCopyInfo() that item is rejected */
    item2->prio = PRIO_INVALID;
  }
  else
  {
    /* communicate new priority to XferInitCopyInfo() */
    item2->prio = newprio;
  }

  return(0);
}


void Method(Print) (ParamThis _PRINTPARAMS)
{
  fprintf(fp, "XISetPrio gid=" DDD_GID_FMT " prio=%d\n", This->gid, This->prio);
}

#undef ClassName


/****************************************************************************/


/*
    include templates
 */
#define T XIDelCmd
#define SLL_WithOrigOrder
#include "sll.ct"
#undef T

#define T XIDelObj
#include "sll.ct"
#undef T

#define T XINewCpl
#include "sll.ct"
#undef T

#define T XIOldCpl
#include "sll.ct"
#undef T



#define T XIAddCpl
#include "sll.ct"
#undef T

#define T XIDelCpl
#include "sll.ct"
#undef T

#define T XIModCpl
#include "sll.ct"
#undef T



/****************************************************************************/


static AddDataSegm *NewAddDataSegm(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  AddDataSegm *segm;

  segm = (AddDataSegm *) OO_Allocate(sizeof(AddDataSegm));
  if (segm==NULL)
    throw std::bad_alloc();

  segm->next      = ctx.segmAddData;
  ctx.segmAddData = segm;
  segm->nItems    = 0;

  return(segm);
}


static void FreeAddDataSegms(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  AddDataSegm *segm = ctx.segmAddData;
  AddDataSegm *next = NULL;

  while (segm!=NULL)
  {
    next = segm->next;
    OO_Free (segm /*,sizeof(AddDataSegm)*/);

    segm = next;
  }

  ctx.segmAddData = nullptr;
}


/****************************************************************************/


static SizesSegm *NewSizesSegm(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  SizesSegm *segm;

  segm = (SizesSegm *) OO_Allocate (sizeof(SizesSegm));
  if (segm==NULL)
    throw std::bad_alloc();

  segm->next    = ctx.segmSizes;
  ctx.segmSizes = segm;
  segm->current = 0;

  return(segm);
}


static void FreeSizesSegms(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  SizesSegm *segm = ctx.segmSizes;
  SizesSegm *next = NULL;

  while (segm!=NULL)
  {
    next = segm->next;
    OO_Free (segm /*,sizeof(SizesSegm)*/);

    segm = next;
  }

  ctx.segmSizes = nullptr;
}


/****************************************************************************/


XFERADDDATA *NewXIAddData(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  AddDataSegm *segm = ctx.segmAddData;
  XFERADDDATA *xa;

  if (segm==NULL || segm->nItems==ADDDATASEGM_SIZE)
  {
    segm = NewAddDataSegm(context);
  }

  xa = &(segm->item[segm->nItems++]);
  xa->next = ctx.theXIAddData->add;
  ctx.theXIAddData->add = xa;

  return(xa);
}



void FreeAllXIAddData(DDD::DDDContext& context)
{
  FreeAddDataSegms(context);
  FreeSizesSegms(context);
}


/****************************************************************************/

int *AddDataAllocSizes (DDD::DDDContext& context, int cnt)
{
  auto& ctx = context.xferContext();
  SizesSegm *segm = ctx.segmSizes;
  int *pos;

  if (segm==NULL || segm->current+cnt>=SIZESSEGM_SIZE)
  {
    segm = NewSizesSegm(context);
  }

  pos = segm->data + segm->current;
  segm->current += cnt;

  return(pos);
}


/****************************************************************************/


/*
        get quantitative resource usage
 */
void GetSizesXIAddData(const DDD::DDDContext& context, int *nSegms, int *nItems, size_t *alloc_mem, size_t *used_mem)
{
  const auto& ctx = context.xferContext();
  size_t allocated=0, used=0;
  int ns=0, ni=0;

  {
    for (const AddDataSegm* segm=ctx.segmAddData; segm!=NULL; segm=segm->next)
    {
      /* count number of segments and number of items */
      ns++;
      ni+=segm->nItems;

      /* compute memory usage */
      allocated += sizeof(AddDataSegm);
      used += (sizeof(AddDataSegm) -
               (sizeof(XFERADDDATA)*(ADDDATASEGM_SIZE-segm->nItems)));
    }
  }

  /* TODO: add resources for SizesSegm */


  *nSegms    = ns;
  *nItems    = ni;
  *alloc_mem = allocated;
  *used_mem  = used;
}


/****************************************************************************/

END_UGDIM_NAMESPACE
