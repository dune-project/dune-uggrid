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

#include "dddi.h"
#include "basic/notify.h"
#include "basic/lowcomm.h"

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
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

int        *iBuffer;        /* general bufferspace, integer */
char       *cBuffer;        /* general bufferspace, integer */


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
  int buffsize;

  dddUsers += 1;

  /* init lineout-interface to stdout */
  DDD_UserLineOutFunction = NULL;

  /* check max. number of procs (limited by GID construction) */
  if (context.procs() > MAX_PROCS) {
    DDD_PrintError('E', 1010,
                   "too many processors, cannot construct global IDs in DDD_Init");
    HARD_EXIT;
  }

  /* compute size for general buffer */
  buffsize = (context.procs()+1)*(sizeof(int)*BUFFER_SIZE_FACTOR);
  if (buffsize<MIN_BUFFER_SIZE)
  {
    buffsize = MIN_BUFFER_SIZE;
  }

  /* get bufferspace */
  iBuffer = (int *)AllocFix(buffsize);
  if (iBuffer==NULL) {
    DDD_PrintError('E', 1000, "not enough memory in DDD_Init");
    HARD_EXIT;
  }
  /* overlay with other buffers */
  cBuffer = (char *)iBuffer;

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
  ddd_IdentInit();
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

  /* free bufferspace */
  FreeFix(iBuffer);

  /* close up all DDD components */
  ddd_ConsExit(context);
  ddd_JoinExit(context);
  ddd_PrioExit(context);
  ddd_XferExit(context);
  ddd_IFExit(context);
  ddd_IdentExit();
  ddd_TopoExit(context);
  ddd_CplMgrExit(context);
  ddd_ObjMgrExit(context);
  ddd_TypeMgrExit(context);
  ddd_StatExit();
  LC_Exit(context);
  NotifyExit(context);

  /* exit PPIF */
  ExitPPIF(context.ppifContext());
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
  sprintf(cBuffer, "| DDD_Status for proc=%03d, DDD-Version %s\n", me,
          DDD_VERSION);
  DDD_PrintLine(cBuffer);
  sprintf(cBuffer, "|\n|     MAX_ELEMDESC = %4d\n", TYPE_DESC::MAX_ELEMDESC);
  sprintf(cBuffer, "|     MAX_TYPEDESC = %4d\n", MAX_TYPEDESC);
  sprintf(cBuffer, "|     MAX_PROCS    = %4d\n", MAX_PROCS);
  sprintf(cBuffer, "|     MAX_PRIO     = %4d\n", MAX_PRIO);
  DDD_PrintLine(cBuffer);
#ifdef WithFullObjectTable
  sprintf(cBuffer, "|\n|     MAX_OBJ = %8d  MAX_CPL = %8zd\n",
          (int) context.objTable().size(), context.couplingContext().cplTable.size());
#else
  sprintf(cBuffer, "|\n|     MAX_CPL = %8zd\n", context.couplingContext().cplTable.size());
#endif
  DDD_PrintLine(cBuffer);

  sprintf(cBuffer, "|     nObjs   = %8d  nCpls   = %8d  nCplItems = %8d\n",
          context.nObjs(), context.couplingContext().nCpls, context.couplingContext().nCplItems);
  DDD_PrintLine(cBuffer);
  DDD_PrintLine("|\n|     Timeouts:\n");
  sprintf(cBuffer, "|        IFComm:  %12ld\n", (unsigned long)MAX_TRIES);
  DDD_PrintLine(cBuffer);

  sprintf(cBuffer, "|\n|     Compile-Time Options: ");

#       ifdef Statistics
  strcat(cBuffer, "Statistics ");
#       endif

  strcat(cBuffer, "\n");
  DDD_PrintLine(cBuffer);
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
  DDD_PrintError('E', 1090, "invalid DDD_OPTION in DDD_SetOption()");
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
    DDD_PrintError('E', 1091, "invalid DDD_OPTION in DDD_GetOption()");
    return 0;
  }

  return context.options()[option];
}

END_UGDIM_NAMESPACE
