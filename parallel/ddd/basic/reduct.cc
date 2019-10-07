// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      reduct.c                                                      */
/*                                                                          */
/* Purpose:   standard parallel routines not supported by ddd               */
/*            reduction operations (GlobalSum, GlobalMax etc)               */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   940128 kb  begin                                              */
/*            960902 kb  copied from fedemo, adapted                        */
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

#include "config.h"

#include <mpi.h>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>
#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

#include <low/namespace.h>

namespace DDD {

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/* some useful functions by Peter Bastian, from ugp/ug/ugcom.c */


int ddd_GlobalMaxInt(const DDD::DDDContext& context, int i)
{
  MPI_Allreduce(MPI_IN_PLACE, &i, 1, MPI_INT, MPI_MAX, context.ppifContext().comm());
  return i;
}

int ddd_GlobalMinInt(const DDD::DDDContext& context, int i)
{
  MPI_Allreduce(MPI_IN_PLACE, &i, 1, MPI_INT, MPI_MIN, context.ppifContext().comm());
  return i;
}

int ddd_GlobalSumInt(const DDD::DDDContext& context, int x)
{
  MPI_Allreduce(MPI_IN_PLACE, &x, 1, MPI_INT, MPI_SUM, context.ppifContext().comm());
  return x;
}

} /* namespace DDD */
