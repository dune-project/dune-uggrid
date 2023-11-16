// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ddd.c                                                         */
/*                                                                          */
/* Purpose:   distributed dynamic data module                               */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/22 kb  begin                                            */
/*            94/09/13 kb  added DDD_Status                                 */
/*            95/11/03 kb  complete rewrite of StructRegister code          */
/*            95/11/16 kb  moved DDD_TYPE-definition code to typemgr.c      */
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

/* standard C library  */
#include <config.h>
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>
#include <cstring>

#include <iomanip>
#include <iostream>
#include <new>

#include "dddi.h"
#include <dune/uggrid/parallel/ddd/basic/lowcomm.h>
#include <dune/uggrid/parallel/ddd/basic/notify.h>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

USING_UG_NAMESPACE
using namespace PPIF;

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#define  BUFFER_SIZE_FACTOR   3
#define  MIN_BUFFER_SIZE     256


/****************************************************************************/
/*                                                                          */
/* definition of static variables                                           */
/*                                                                          */
/****************************************************************************/

/**
 * number of users of DDD.
 * Managed by calls to `DDD_Init` and `DDD_Exit`.
 * Resources are only freed by `DDD_Exit` when `dddUsers` is zero.
 *
 * This variable will be removed once no global state for DDD remains.
 */
static unsigned int dddUsers = 0;


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

static void *LowComm_DefaultAlloc (size_t s)
{
  return memmgr_AllocTMEM(s, TMEM_LOWCOMM);
}

static void LowComm_DefaultFree (void *buffer)
{
  memmgr_FreeTMEM(buffer, TMEM_LOWCOMM);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_Init                                                      */
/*                                                                          */
/****************************************************************************/

/**
        Initialisation of the DDD library.
        This function has to be called before any other function
        of the DDD library is called. It initializes the underlying
        PPIF-library, sets all DDD options to their default values
        and initiates all DDD subsystems.

        As some of the memory handler calls will be initiated during
        the execution of this function, the memory manager has to be
        initialized before calling \funk{Init}.
 */

void DDD_Init(DDD::DDDContext& context)
{
  dddUsers += 1;

  /* init lineout-interface to stdout */
  DDD_UserLineOutFunction = NULL;

  /* check max. number of procs (limited by GID construction) */
  if (context.procs() > MAX_PROCS)
    DUNE_THROW(Dune::Exception,
               "too many processors, cannot construct global IDs");

  /* reset all global counters */
  context.nObjs(0);
  context.couplingContext().nCpls = 0;
  context.couplingContext().nCplItems = 0;

  /* init all DDD components */
  NotifyInit(context);
  LC_Init(context, LowComm_DefaultAlloc, LowComm_DefaultFree);
  ddd_StatInit();
  ddd_TypeMgrInit(context);
  ddd_ObjMgrInit(context);
  ddd_CplMgrInit(context);
  ddd_TopoInit(context);
  ddd_IdentInit(context);
  ddd_IFInit(context);
  ddd_XferInit(context);
  ddd_PrioInit(context);
  ddd_JoinInit(context);
  ddd_ConsInit(context);

  /* set options on default values */
  DDD_SetOption(context, OPT_WARNING_VARSIZE_OBJ,   OPT_ON);
  DDD_SetOption(context, OPT_WARNING_SMALLSIZE,     OPT_ON);
  DDD_SetOption(context, OPT_WARNING_PRIOCHANGE,    OPT_ON);
  DDD_SetOption(context, OPT_WARNING_DESTRUCT_HDR,  OPT_ON);
  DDD_SetOption(context, OPT_DEBUG_XFERMESGS,       OPT_OFF);
  DDD_SetOption(context, OPT_QUIET_CONSCHECK,       OPT_OFF);
  DDD_SetOption(context, OPT_IDENTIFY_MODE,         IDMODE_LISTS);
  DDD_SetOption(context, OPT_WARNING_REF_COLLISION, OPT_ON);
  DDD_SetOption(context, OPT_INFO_XFER,             XFER_SHOW_NONE);
  DDD_SetOption(context, OPT_INFO_JOIN,             JOIN_SHOW_NONE);
  DDD_SetOption(context, OPT_WARNING_OLDSTYLE,      OPT_ON);
  DDD_SetOption(context, OPT_INFO_IF_WITH_ATTR,     OPT_OFF);
  DDD_SetOption(context, OPT_XFER_PRUNE_DELETE,     OPT_OFF);
  DDD_SetOption(context, OPT_IF_REUSE_BUFFERS,      OPT_OFF);
  DDD_SetOption(context, OPT_IF_CREATE_EXPLICIT,    OPT_OFF);
  DDD_SetOption(context, OPT_CPLMGR_USE_FREELIST,   OPT_ON);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_Exit                                                      */
/*                                                                          */
/****************************************************************************/

/**
        Clean-up of the DDD library.
        This function frees memory previously allocated by DDD and finally
        finishes up the PPIF library. After the call to \funk{Exit}
        further usage of the DDD library is no longer possible during this program
        run.

        The clean-up of the memory manager should happen afterwards and is left
        to the DDD application programmer.
 */

void DDD_Exit(DDD::DDDContext& context)
{
  dddUsers -= 1;
  if (dddUsers > 0)
    return;

  /* close up all DDD components */
  ddd_ConsExit(context);
  ddd_JoinExit(context);
  ddd_PrioExit(context);
  ddd_XferExit(context);
  ddd_IFExit(context);
  ddd_IdentExit(context);
  ddd_TopoExit(context);
  ddd_CplMgrExit(context);
  ddd_ObjMgrExit(context);
  ddd_TypeMgrExit(context);
  ddd_StatExit();
  LC_Exit(context);
  NotifyExit(context);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_Status                                                    */
/*                                                                          */
/****************************************************************************/

/**
        Show global status information.
        This function displays information concerning both
        the compile-time parameters of the DDD-library and some important
        runtime-variables. Overview of compile time parameters that will
        be displayed:

        \begin{tabular}{l|l}
        Parameter       & Description\\ \hline
        DDD-Version     & current library version number\\ ##
   #MAX_TYPEDESC#  & maximum number of #DDD_TYPE# IDs\\ ##
   #MAX_OBJ#       & maximum number of DDD-objects on one processor\\ ##
   #MAX_CPL#       & maximum number of couplings on one processor\\ ##
        \end{tabular}
 */

void DDD_Status(const DDD::DDDContext& context)
{
  using std::setw;
  std::ostream& out = std::cout;

  out << "| DDD_Status for proc=" << setw(3) << context.me()
      << ", DDD-Version " << DDD_VERSION << "\n"
      << "|\n"
      << "|     MAX_ELEMDESC = " << setw(4) << TYPE_DESC::MAX_ELEMDESC << "\n"
      << "|     MAX_TYPEDESC = " << setw(4) << MAX_TYPEDESC << "\n"
      << "|     MAX_PROCS    = " << setw(4) << MAX_PROCS << "\n"
      << "|     MAX_PRIO     = " << setw(4) << MAX_PRIO << "\n"
      << "|\n";
#ifdef WithFullObjectTable
  out << "|     MAX_OBJ = " << setw(8) << context.objTable().size()
      << "  MAX_CPL = " << context.couplingContext().cplTable.size() << "\n";
#else
  out << "|     MAX_CPL = " << context.couplingContext().cplTable.size() << "\n";
#endif

  out << "|     nObjs   = " << setw(8) << context.nObjs()
      << "  nCpls   = " << setw(8) << context.couplingContext().nCpls
      << "  nCplItems = " << setw(8) << context.couplingContext().nCplItems << "\n"
      << "|\n"
      << "|     Timeouts:\n"
      << "|        IFComm:  " << setw(12) << MAX_TRIES << "\n"
      << "|\n"
      << "|     Compile-Time Options: ";

#       ifdef Statistics
  out << "Statistics ";
#       endif

  out << "\n";
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_LineOutRegister                                           */
/*                                                                          */
/****************************************************************************/

/**
        Redirect text output.
        This function sets the DDD-textport to a given handler function.
        The handler should be declared as follows:

   #void func(char *line_of_text)#

        Instead of printing text for error, debugging and info messages
        directly to {\em standard output}, DDD will redirect all output
        one line at a time and send it to the handler {\em func}.
        This can be used to send each processor's output into a separate file.

   @param  func  handler function which should be used for text redirection
 */

void DDD_LineOutRegister (void (*func)(const char *s))
{
  DDD_UserLineOutFunction = func;
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_SetOption                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Set a DDD-option to a given value.
        The current behaviour of the DDD library can be configured
        at runtime by setting a variety of options to given values.
        For each option, there is a default setting and a set of
        possible values. See \Ref{DDD Options} for a description
        of all possible options with their default settings and
        meaning.

   @param option   DDD option specifier
   @param value    option value, possible values depend on option specifier
 */

void DDD_SetOption (DDD::DDDContext& context, DDD_OPTION option, int value)
{
  if (option>=OPT_END)
  {
    Dune::dwarn << "DDD_SetOption: invalid DDD_OPTION\n";
    return;
  }

  context.options()[option] = value;
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_GetOption (not exported)                                  */
/*                                                                          */
/* Purpose:   get DDD runtime options                                       */
/*                                                                          */
/* Input:     option:  OptionType of option to get                          */
/*                                                                          */
/* Output:    value of option                                               */
/*                                                                          */
/****************************************************************************/

int DDD_GetOption(const DDD::DDDContext& context, DDD_OPTION option)
{
  if (option>=OPT_END)
  {
    Dune::dwarn << "DDD_GetOption: invalid DDD_OPTION\n";
    return 0;
  }

  return context.options()[option];
}

END_UGDIM_NAMESPACE
