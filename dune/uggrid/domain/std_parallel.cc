// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  std_parallel.c				                                                                */
/*																			*/
/* Purpose:   parallel part of standard ug domain description                           */
/*																			*/
/* Author:	  Klaus Birken / Christian Wieners								*/
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   Sep 12 1996 ug version 3.4                                                                */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

#ifdef ModelP

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cassert>

/* low modules */
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

/* parallel modules */
#include <dune/uggrid/parallel/ddd/include/memmgr.h>
#include <dune/uggrid/parallel/dddif/parallel.h>

/* domain module */
#include "domain.h"
#include "std_domain.h"
#include "std_internal.h"

USING_UGDIM_NAMESPACE
  USING_UG_NAMESPACE

#ifdef ModelP
  using namespace PPIF;
#endif

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define CEIL(n)          ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

void NS_DIM_PREFIX DomInitParallel (INT TypeBndP, INT TypeBndS)
{}

void NS_DIM_PREFIX DomHandlerInit (INT handlerSet)
{}

void NS_DIM_PREFIX BElementXferBndS(DDD::DDDContext& context, BNDS **bnds, int n, int proc, int prio)
{
  INT size,i,size0;

  size = CEIL(sizeof(INT));
  for (i=0; i<n; i++)
    if (bnds[i] != NULL)
    {
      size0 = BND_SIZE(bnds[i]);
      size += CEIL(size0) + CEIL(sizeof(INT));

      PRINTDEBUG(dom,1,("BElementXferBndS(): Xfer "
                        "%x pid %d n %d size %d\n",
                        bnds[i],BND_PATCH_ID(bnds[i]),
                        BND_N(bnds[i]),BND_SIZE(bnds[i])));

    }

  DDD_XferAddData(context, size, DDD_DOMAIN_DATA);
}

void NS_DIM_PREFIX BElementGatherBndS (BNDS **bnds, int n, int cnt, char *data)
{
  INT size,i;

  for (i=0; i<n; i++)
    if (bnds[i] != NULL)
    {

      PRINTDEBUG(dom,1,("BElementGatherBndS(): %d  "
                        "%x pid %d n %d size %d\n",i,
                        bnds[i],BND_PATCH_ID(bnds[i]),
                        BND_N(bnds[i]),BND_SIZE(bnds[i])));


      size = BND_SIZE(bnds[i]);
      memcpy(data,&i,sizeof(INT));
      data += CEIL(sizeof(INT));
      memcpy(data,bnds[i],size);
      data += CEIL(size);
    }
  i = -1;
  memcpy(data,&i,sizeof(INT));
}

void NS_DIM_PREFIX BElementScatterBndS (const DDD::DDDContext& context, BNDS **bnds, int n, int cnt, char *data)
{
  INT size,i;
  BNDS *bs;

  memcpy(&i,data,sizeof(INT));
  while (i != -1)
  {
    data += CEIL(sizeof(INT));
    bs = (BNDS *) data;
    size = BND_SIZE(bs);

    PRINTDEBUG(dom,1,("BElementScatterBndS(): %d me %d\n",i,size));

    if (bnds[i] == NULL)
    {
      bs = (BNDS *) memmgr_AllocOMEM((size_t)size,ddd_ctrl(context).TypeBndS,0,0);
      memcpy(bs,data,size);
      bnds[i] = bs;
    }
    data += CEIL(size);
    memcpy(&i,data,sizeof(INT));
  }
}

void NS_DIM_PREFIX BVertexXferBndP (DDD::DDDContext& context, BNDP *bndp, int proc, int prio)
{
  INT size;

  size = BND_SIZE(bndp);

  PRINTDEBUG(dom,1,("BVertexXferBndP():  %x pid %d n %d size %d\n",
                    bndp,BND_PATCH_ID(bndp),BND_N(bndp),BND_SIZE(bndp)));

  DDD_XferAddData(context, size, DDD_DOMAIN_DATA);
}

void NS_DIM_PREFIX BVertexGatherBndP (BNDP *bndp, int cnt, char *data)
{
  PRINTDEBUG(dom,1,("BVertexGatherBnd():  pid %d "
                    "n %d size %d cnt %d\n",
                    BND_PATCH_ID(bndp),
                    BND_N(bndp),BND_SIZE(bndp),cnt));

  ASSERT(cnt == BND_SIZE(bndp));

  memcpy(data,bndp,cnt);
}


void NS_DIM_PREFIX BVertexScatterBndP (const DDD::DDDContext& context, BNDP **bndp, int cnt, char *data)
{
  if (*bndp == NULL)
  {
    *bndp = (BNDS *) memmgr_AllocOMEM((size_t)cnt,ddd_ctrl(context).TypeBndP,0,0);
    memcpy(*bndp,data,cnt);
    PRINTDEBUG(dom,1,("BVertexScatterBndP():  pid "
                      "%d n %d size %d cnt %d\n",
                      BND_PATCH_ID(*bndp),
                      BND_N(*bndp),BND_SIZE(*bndp),cnt));
  }
}
#endif /* ModelP */
