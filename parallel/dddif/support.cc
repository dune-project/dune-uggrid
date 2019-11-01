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

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstdio>
#include <cstddef>

#include <type_traits>

#ifdef ModelP
#  include <mpi.h>
#  include <dune/uggrid/parallel/ppif/ppifcontext.hh>
#endif

#include <parallel/ppif/ppif.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>
#include <parallel/ddd/include/memmgr.h>

#include <gm/pargm.h>


/* UG namespaces: */
USING_UG_NAMESPACES

/* PPIF namespaces: */
using namespace PPIF;

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/* some useful functions by Peter Bastian, from ugp/ug/ugcom.c */

#ifdef ModelP

static_assert(
  std::is_same<int, INT>::value,
  "The implementation assumes that `int` and `INT` are the same type.");
static_assert(
  std::is_same<double, DOUBLE>::value,
  "The implementation assumes that `double` and `DOUBLE` are the same type.");

/****************************************************************************/
/*D
   UG_GlobalMaxINT - get maximum for INT value

   SYNOPSIS:
   INT UG_GlobalMaxINT (INT i)

   PARAMETERS:
   .  i - calculate maximum for this value

   DESCRIPTION:
   This function calculates the maximum of i over all processors.

   RETURN VALUE:
   The maximum value of i over all processors.

   D*/
/****************************************************************************/
INT UG_GlobalMaxINT(const PPIF::PPIFContext& context, INT i)
{
  MPI_Allreduce(MPI_IN_PLACE, &i, 1, MPI_INT, MPI_MAX, context.comm());
  return i;
}

/****************************************************************************/
/*D
   UG_GlobalMinINT - get global minimum for INT value

   SYNOPSIS:
   INT UG_GlobalMaxINT (INT i)

   PARAMETERS:
   .  i - calculate minimum for this value

   DESCRIPTION:
   This function calculates the minimum of i over all processors.

   RETURN VALUE:
   The minimum value of i over all processors

   D*/
/****************************************************************************/

INT UG_GlobalMinINT (const PPIF::PPIFContext& context, INT i)
{
  MPI_Allreduce(MPI_IN_PLACE, &i, 1, MPI_INT, MPI_MIN, context.comm());
  return i;
}

/****************************************************************************/
/*D
   UG_GlobalSumINT - get global sum for INT value

   SYNOPSIS:
   INT UG_GlobalSumINT (INT i)

   PARAMETERS:
   .  i - calculate sum for this variable

   DESCRIPTION:
   This function calculates the sum of i over all processors.

   RETURN VALUE:
   The sum of i over all processors

   D*/
/****************************************************************************/

INT UG_GlobalSumINT (const PPIF::PPIFContext& context, INT x)
{
  MPI_Allreduce(MPI_IN_PLACE, &x, 1, MPI_INT, MPI_SUM, context.comm());
  return x;
}

/****************************************************************************/
/*D
   UG_GlobalMaxNINT - get maximum for n integer values

   SYNOPSIS:
   void UG_GlobalMaxNINT (INT n, INT *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the maximum of x[i] over all processors for all
   i from 0 to n-1. x is overwritten with the maximum value.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalMaxNINT(const PPIF::PPIFContext& context, INT n, INT *x)
{
  MPI_Allreduce(MPI_IN_PLACE, x, n, MPI_INT, MPI_MAX, context.comm());
}

/****************************************************************************/
/*D
   UG_GlobalMinNINT - get minimum for n integer values

   SYNOPSIS:
   void UG_GlobalMinNINT (INT n, INT *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the minimum of x[i] over all processors for all
   i from 0 to n-1. x is overwritten with the minimum value.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalMinNINT(const PPIF::PPIFContext& context, INT n, INT *x)
{
  MPI_Allreduce(MPI_IN_PLACE, x, n, MPI_INT, MPI_MIN, context.comm());
}

/****************************************************************************/
/*D
   UG_GlobalSumNINT - calculate global sum for n integer values

   SYNOPSIS:
   void UG_GlobalSumNINT (INT n, INT *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the sum of x[i] over all processors for each
   i from 0 to n-1. x is overwritten with the result.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalSumNINT (const PPIF::PPIFContext& context, INT n, INT *xs)
{
  MPI_Allreduce(MPI_IN_PLACE, xs, n, MPI_INT, MPI_SUM, context.comm());
}

/****************************************************************************/
/*D
   UG_GlobalMaxDOUBLE - get global maximum for DOUBLE value

   SYNOPSIS:
   DOUBLE UG_GlobalMaxDOUBLE (DOUBLE x)

   PARAMETERS:
   .  x - calculate maximum for this value

   DESCRIPTION:
   This function calculates the maximum of x over all processors.

   RETURN VALUE:
   The maximum value of x over all processors.

   D*/
/****************************************************************************/

DOUBLE UG_GlobalMaxDOUBLE (const PPIF::PPIFContext& context, DOUBLE x)
{
  MPI_Allreduce(MPI_IN_PLACE, &x, 1, MPI_DOUBLE, MPI_MAX, context.comm());
  return x;
}

/****************************************************************************/
/*D
   UG_GlobalMinDOUBLE - get global minimum for DOUBLE value

   SYNOPSIS:
   DOUBLE UG_GlobalMinDOUBLE (DOUBLE x)

   PARAMETERS:
   .  x - calculate minimum for this value

   DESCRIPTION:
   This function calculates the minimum of x over all processors.

   RETURN VALUE:
   The minimum value of x over all processors.

   D*/
/****************************************************************************/

DOUBLE UG_GlobalMinDOUBLE (const PPIF::PPIFContext& context, DOUBLE x)
{
  MPI_Allreduce(MPI_IN_PLACE, &x, 1, MPI_DOUBLE, MPI_MIN, context.comm());
  return x;
}

/****************************************************************************/
/*D
   UG_GlobalSumDOUBLE - get global sum for DOUBLE value

   SYNOPSIS:
   DOUBLE UG_GlobalSumDOUBLE (DOUBLE i)

   PARAMETERS:
   .  i - calculate sum for this variable

   DESCRIPTION:
   This function calculates the sum of i over all processors.

   RETURN VALUE:
   The sum of i over all processors

   D*/
/****************************************************************************/

DOUBLE UG_GlobalSumDOUBLE (const PPIF::PPIFContext& context, DOUBLE x)
{
  MPI_Allreduce(MPI_IN_PLACE, &x, 1, MPI_DOUBLE, MPI_SUM, context.comm());
  return x;
}

/****************************************************************************/
/*D
   UG_GlobalMaxNDOUBLE - get maximum over n integer values

   SYNOPSIS:
   void UG_GlobalMaxNDOUBLE (INT n, DOUBLE *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the maximum of x[i] over all processors for all
   i from 0 to n-1. x is overwritten with the maximum value.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalMaxNDOUBLE (const PPIF::PPIFContext& context, INT n, DOUBLE *x)
{
  MPI_Allreduce(MPI_IN_PLACE, x, n, MPI_DOUBLE, MPI_MAX, context.comm());
}

/****************************************************************************/
/*D
   UG_GlobalMinNDOUBLE - get minimum over n integer values

   SYNOPSIS:
   void UG_GlobalMinNDOUBLE (INT n, DOUBLE *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the minimum of x[i] over all processors for all
   i from 0 to n-1. x is overwritten with the minimum value.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalMinNDOUBLE (const PPIF::PPIFContext& context, INT n, DOUBLE *x)
{
  MPI_Allreduce(MPI_IN_PLACE, x, n, MPI_DOUBLE, MPI_MIN, context.comm());
}

/****************************************************************************/
/*D
   UG_GlobalSumNDOUBLE - calculate global sum for n DOUBLE values

   SYNOPSIS:
   void UG_GlobalSumNDOUBLE (INT n, DOUBLE *x)

   PARAMETERS:
   .  n - number of elements in array x to be used
   .  x - array of size n

   DESCRIPTION:
   This function calculates the sum of x[i] over all processors for each
   i from 0 to n-1. x is overwritten with the result.

   RETURN VALUE:
   none

   D*/
/****************************************************************************/

void UG_GlobalSumNDOUBLE (const PPIF::PPIFContext& context, INT n, DOUBLE *x)
{
  MPI_Allreduce(MPI_IN_PLACE, x, n, MPI_DOUBLE, MPI_SUM, context.comm());
}

#endif  /* ModelP */

END_UGDIM_NAMESPACE
