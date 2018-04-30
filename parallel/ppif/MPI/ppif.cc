// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      mpi-ppif.c                                                    */
/*                                                                          */
/* Purpose:   parallel processor interface                                  */
/*            Provides a portable interface to message passing MIMD         */
/*            architectures. PPIF is divided into three parts:              */
/*                                                                          */
/*            (1) Administration                                            */
/*            (2) Communication                                             */
/*            (3) Miscellaneous                                             */
/*                                                                          */
/*            The interface assumes that the parallel machine has           */
/*            the following properties:                                     */
/*                                                                          */
/*            (1) it is physically connected at least as a 2 or 3 dim. array*/
/*            (2) it has a fast virtual channel communication mechanism     */
/*            (3) it has an asynchronous communication mechanism            */
/*                                                                          */
/*            MPI Implementation                                            */
/*                                                                          */
/* Author:    Jens Boenisch                                                 */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 3a                                                */
/*            7000 Stuttgart 80                                             */
/*            internet: boenisch@rus.uni-stuttgart.de                       */
/*                                                                          */
/* History:   17 Aug 1992, begin                                            */
/*            18 Feb 1993, Indigo version                                   */
/*            02 Jun 1993, Paragon OSF/1 version                            */
/*            14 Sep 1995, MPI version                                      */
/*            29 Jan 2003, pV3 concentrator support                         */
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
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include <mpi.h>

#include "../ppif.h"

#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

/*#include "ugtypes.h"*/

using namespace PPIF;

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#define MAXT            15      /* maximum number of downtree nodes max     */
                                /* log2(P)                                  */

#define ID_TREE         101     /* channel id: tree                         */

#define PPIF_SUCCESS    0       /* Return value for success                 */
#define PPIF_FAILURE    1       /* Return value for failure                 */

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

namespace PPIF {

struct VChannel
{
  int p;
  int chanid;
};

struct Msg
{
  MPI_Request req;
};

} /* namespace PPIF */

/****************************************************************************/
/*                                                                          */
/* definition of static variables                                           */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/* id's */
int PPIF::me;                          /* my processor id                          */
int PPIF::master;                      /* id of master processor                   */
int PPIF::procs;                       /* number of processors in the network      */

/****************************************************************************/
/*                                                                          */
/* routines for handling virtual channels                                   */
/*                                                                          */
/****************************************************************************/

static VChannelPtr NewVChan (int p, int id)

{
  VChannelPtr myChan = new PPIF::VChannel;

  myChan->p      = p;
  myChan->chanid = id;

  return myChan;
}


static void DeleteVChan (VChannelPtr myChan)

{
  delete myChan;
}

static std::shared_ptr<PPIF::PPIFContext> ppifContext_;

void PPIF::ppifContext(const std::shared_ptr<PPIFContext>& context)
{
  ppifContext_ = context;

  me = context->me();
  master = context->master();
  procs = context->procs();
}

void PPIF::ppifContext(std::nullptr_t)
{
  ppifContext_ = nullptr;

  me = 0;
  master = 0;
  procs = 1;
}

const std::shared_ptr<PPIF::PPIFContext>& PPIF::ppifContext()
{
  return ppifContext_;
}

/****************************************************************************/
/*                                                                          */
/* Function:  InitPPIF                                                      */
/*                                                                          */
/* Purpose:   initialize parallel processor interface                       */
/*            set exported variables, allocate tree communication structure */
/*                                                                          */
/* Input:     void                                                          */
/*                                                                          */
/* Output:    int 0:  ok                                                    */
/*            int!=0: error                                                 */
/*                                                                          */
/****************************************************************************/

/*
   Factor N into two integers that are as close together as possible
 */

static void Factor (int N, int *pn, int *pm)

{
  int n = (int)ceil (sqrt ((double) N));
  int m = (int)floor (sqrt ((double) N));

  while (n*m != N)
  {
    if (n*m < N) n++;
    else m--;
  }

  *pn = n; *pm = m;
}


void PPIF::InitPPIF(PPIFContext& context)
{
  const auto me = context.me();
  const auto procs = context.procs();

  context.dims_[2] = 1;
  Factor(procs, &context.dims_[0], &context.dims_[1]);

  /* tree configuration */

  auto& degree = context.degree_;
  auto& uptree = context.uptree_;
  auto& downtree = context.downtree_;

  degree = 0;
  const int sonl = 2*me + 1;
  const int sonr = 2*me + 2;

  if (sonl<procs)
  {
    degree++;
    if (! downtree[0])  /* InitPPIF is being called for the first time */
      downtree[0] = NewVChan(sonl,ID_TREE);
  }
  else
  {
    downtree[0] = NULL;
  }

  if (sonr<procs)
  {
    degree++;
    if (! downtree[1])  /* InitPPIF is being called for the first time */
      downtree[1] = NewVChan(sonr,ID_TREE);
  }
  else
  {
    downtree[1] = NULL;
  }

  if (me>0)
  {
    if (! uptree)  /* InitPPIF is being called for the first time */
      uptree = NewVChan((me-1)/2,ID_TREE);
  }
  else
  {
    uptree = NULL;
  }

  auto& slvcnt = context.slvcnt_;

  int succ=1;
  for(int i=0; i<degree; i++)
  {
    MPI_Recv ((void *) &(slvcnt[i]), (int) sizeof(int), MPI_BYTE,
              downtree[i]->p, ID_TREE, context.comm(), MPI_STATUS_IGNORE);
    succ += slvcnt[i];
  }
  if (me>0)
  {
    MPI_Send ((void *) &succ, (int) sizeof(int), MPI_BYTE, (int)(me-1)/2, ID_TREE, context.comm());
  }
}

int PPIF::InitPPIF (int *, char ***)
{
  auto context = ppifContext();
  if (not context)
    context = std::make_shared<PPIFContext>();
  ppifContext(context);

  return PPIF_SUCCESS;
}

void PPIF::ExitPPIF(PPIF::PPIFContext& context)
{
  /* Deallocate tree structure */
  DeleteVChan(context.uptree_);
  context.uptree_ = nullptr;
  /* I currently think that only the first two entries of downtree can contain
   * valied entries, but I am not entirely sure. */
  DeleteVChan(context.downtree_[0]);
  DeleteVChan(context.downtree_[1]);
  context.downtree_[0] = context.downtree_[1] = nullptr;
}

int PPIF::ExitPPIF ()
{
  if (ppifContext()) {
    ppifContext(nullptr);
  }

  return PPIF_SUCCESS;
}

/****************************************************************************/
/*                                                                          */
/* Tree oriented functions                                                  */
/*                                                                          */
/****************************************************************************/

int PPIF::Broadcast (const PPIFContext& context, void *data, int size)
{
  if (MPI_SUCCESS != MPI_Bcast (data, size, MPI_BYTE, context.master(), context.comm()) )
    return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

int PPIF::Concentrate (const PPIFContext& context, void *data, int size)
{
  if (not context.isMaster())
    if (SendSync (context, context.uptree(), data, size) < 0) return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

int PPIF::GetConcentrate(const PPIFContext& context, int slave, void *data, int size)
{
  if (slave < context.degree())
    if (RecvSync (context, context.downtree()[slave], data, size) < 0) return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

int PPIF::Spread(const PPIFContext& context, int slave, void *data, int size)
{
  if (slave < context.degree())
    if (SendSync(context, context.downtree()[slave], data, size) < 0) return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

int PPIF::GetSpread(const PPIFContext& context, void *data, int size)
{
  if (not context.isMaster())
    if (RecvSync(context, context.uptree(), data, size) < 0) return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

int PPIF::Synchronize(const PPIFContext& context)
{
  if (MPI_SUCCESS != MPI_Barrier (context.comm()) ) return (PPIF_FAILURE);

  return (PPIF_SUCCESS);
}

/****************************************************************************/
/*                                                                          */
/* Synchronous communication                                                */
/*                                                                          */
/****************************************************************************/

VChannelPtr PPIF::ConnSync(const PPIFContext&, int p, int id)
{
  return NewVChan(p, id);
}

int PPIF::DiscSync(const PPIFContext&, VChannelPtr v)
{
  DeleteVChan(v);

  return (0);
}

int PPIF::SendSync(const PPIFContext& context, VChannelPtr v, void *data, int size)
{
  if (MPI_SUCCESS == MPI_Ssend (data, size, MPI_BYTE,
                                v->p, v->chanid, context.comm()) )
    return (size);
  else
    return (-1);
}

int PPIF::RecvSync(const PPIFContext& context, VChannelPtr v, void *data, int size)
{
  int count = -1;
  MPI_Status status;

  if (MPI_SUCCESS == MPI_Recv (data, size, MPI_BYTE,
                               v->p, v->chanid, context.comm(), &status) )
    MPI_Get_count (&status, MPI_BYTE, &count);

  return (count);
}

/****************************************************************************/
/*                                                                          */
/* Asynchronous communication                                               */
/*                                                                          */
/****************************************************************************/

VChannelPtr PPIF::ConnASync(const PPIFContext&, int p, int id)
{
  return NewVChan(p, id);
}

int PPIF::InfoAConn(const PPIFContext&, VChannelPtr v)
{
  return (v ? 1 : -1);
}

int PPIF::DiscASync(const PPIFContext&, VChannelPtr v)
{
  DeleteVChan (v);
  return (PPIF_SUCCESS);
}

int PPIF::InfoADisc(const PPIFContext&, VChannelPtr v)
{
  return (true);
}

msgid PPIF::SendASync(const PPIFContext& context, VChannelPtr v, void *data, int size, int *error)
{
  msgid m = new PPIF::Msg;

  if (m)
  {
    if (MPI_SUCCESS == MPI_Isend (data, size, MPI_BYTE,
                                  v->p, v->chanid, context.comm(), &m->req) )
    {
      *error = false;
      return m;
    }
  }

  *error = true;
  return NULL;
}

msgid PPIF::RecvASync(const PPIFContext& context, VChannelPtr v, void *data, int size, int *error)
{
  msgid m = new PPIF::Msg;

  if (m)
  {
    if (MPI_SUCCESS == MPI_Irecv (data, size, MPI_BYTE,
                                  v->p, v->chanid, context.comm(), &m->req) )
    {
      *error = false;
      return m;
    }
  }

  *error = true;
  return (NULL);
}

int PPIF::InfoASend(const PPIFContext&, VChannelPtr v, msgid m)
{
  int complete;

  if (m)
  {
    if (MPI_SUCCESS == MPI_Test (&m->req, &complete, MPI_STATUS_IGNORE) )
    {
      if (complete)
        delete m;

      return (complete);        /* complete is true for completed send, false otherwise */
    }
  }

  return (-1);          /* return -1 for FAILURE */
}

int PPIF::InfoARecv(const PPIFContext&, VChannelPtr v, msgid m)
{
  int complete;

  if (m)
  {
    if (MPI_SUCCESS == MPI_Test (&m->req, &complete, MPI_STATUS_IGNORE) )
    {
      if (complete)
        delete m;

      return (complete);        /* complete is true for completed receive, false otherwise */
    }
  }

  return (-1);          /* return -1 for FAILURE */
}
