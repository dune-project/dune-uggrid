// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      pcmds.c                                                       */
/*                                                                          */
/* Purpose:   DDD-commands for Prio Environment                             */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: birken@ica3.uni-stuttgart.de                           */
/*            phone: 0049-(0)711-685-7007                                   */
/*            fax  : 0049-(0)711-685-7000                                   */
/*                                                                          */
/* History:   980720 kb  begin                                              */
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
#include <cstring>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>

#define DebugPrio     10   /* 0 is all, 10 is off */


/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/


namespace DDD {
namespace Prio {

/* overall mode of prio-environment */
enum class PrioMode : unsigned char {
  PMODE_IDLE = 0,            /* waiting for next DDD_PrioBegin() */
  PMODE_CMDS,                /* after DDD_PrioBegin(), before DDD_PrioEnd() */
  PMODE_BUSY                 /* during DDD_PrioEnd() */
};

} /* namespace Prio */
} /* namespace DDD */

START_UGDIM_NAMESPACE

using DDD::Prio::PrioMode;

/*
        management functions for PrioMode.

        these functions control the mode the prio-module is
        currently in. this is used for error detection, but
        also for correct detection of coupling inconsistencies
        and recovery.
 */

static const char* PrioModeName (PrioMode mode)
{
  switch(mode)
  {
  case PrioMode::PMODE_IDLE : return "idle-mode";
  case PrioMode::PMODE_CMDS : return "commands-mode";
  case PrioMode::PMODE_BUSY : return "busy-mode";
  }
  return "unknown-mode";
}


static void PrioSetMode (DDD::DDDContext& context, PrioMode mode)
{
  auto& ctx = context.prioContext();
  ctx.prioMode = mode;

#       if DebugPrio<=8
  Dune::dinfo << "PrioMode=" << PrioModeName(ctx.prioMode) << "\n";
#       endif
}


static PrioMode PrioSuccMode (PrioMode mode)
{
  switch(mode)
  {
  case PrioMode::PMODE_IDLE: return PrioMode::PMODE_CMDS;
  case PrioMode::PMODE_CMDS: return PrioMode::PMODE_BUSY;
  case PrioMode::PMODE_BUSY: return PrioMode::PMODE_IDLE;
  default:                   std::abort();
  }
}


bool ddd_PrioActive (const DDD::DDDContext& context)
{
  return context.prioContext().prioMode != PrioMode::PMODE_IDLE;
}


static bool PrioStepMode(DDD::DDDContext& context, PrioMode old)
{
  auto& ctx = context.prioContext();
  if (ctx.prioMode!=old)
  {
    Dune::dwarn
      << "wrong prio-mode (currently in " << PrioModeName(ctx.prioMode)
      << ", expected " << PrioModeName(old) << ")\n";
    return false;
  }

  PrioSetMode(context, PrioSuccMode(ctx.prioMode));
  return true;
}


/****************************************************************************/


void ddd_PrioInit(DDD::DDDContext& context)
{
  PrioSetMode(context, PrioMode::PMODE_IDLE);
}


void ddd_PrioExit(DDD::DDDContext&)
{}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrioChange                                                */
/*                                                                          */
/****************************************************************************/

/**
        Consistent change of a local object's priority during DDD Prio Environment.
        Local objects which are part of a distributed object must notify
        other copies about local priority changes.
        DDD will send appropriate messages to the owner processors of
        the other copies.

        This function is regarded as a {\bf Prio}-operation due
        to its influence on DDD management information on neighbouring
        processors. Therefore the function has to be issued between
        a starting \funk{PrioBegin} and a final \funk{PrioEnd} call.

   @param hdr  DDD local object whose priority should be changed.
   @param prio new priority for this local object.
 */

void DDD_PrioChange (const DDD::DDDContext& context, DDD_HDR hdr, DDD_PRIO prio)
{
#if DebugPrio<=2
  DDD_PRIO old_prio = OBJ_PRIO(hdr);
#endif

  if (!ddd_PrioActive(context))
    DUNE_THROW(Dune::Exception, "Missing DDD_PrioBegin()");


  /* change priority of object directly, for local objects this
     is all what we need. */
  {
    /*
                    DDD_PRIO newprio;
                    PriorityMerge(&context.typeDefs()[OBJ_TYPE(hdr)],
                            OBJ_PRIO(hdr), prio, &newprio);
                    OBJ_PRIO(hdr) = newprio;
     */
    OBJ_PRIO(hdr) = prio;
  }

  /* handle distributed objects
     if (ObjHasCpl(context, hdr))
     {
          nothing to do here:
          for distributed objects, we will communicate the prio
          via standard interface later.
     }
   */


#       if DebugPrio<=2
  Dune::dvverb
    << "DDD_PrioChange " << OBJ_GID(hdr)
    << ", old_prio=" << old_prio << ", new_prio=" << OBJ_PRIO(hdr) << "\n";
#       endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrioEnd                                                   */
/*                                                                          */
/****************************************************************************/

static int GatherPrio (DDD::DDDContext&, DDD_HDR obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
#       if DebugPrio<=1
  Dune::dvverb
    << "DDD_PrioEnd/GatherPrio " << OBJ_GID(obj) << ", prio=" << OBJ_PRIO(obj)
    << ". Send to copy on proc " << proc << "/p" << prio << "\n";
#       endif

  *((DDD_PRIO *)data) = OBJ_PRIO(obj);
  return(0);
}

static int ScatterPrio (DDD::DDDContext& context, DDD_HDR obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  DDD_PRIO real_prio = *((DDD_PRIO *)data);

  /* if prio on other processor has been changed, we adapt it here. */
  if (real_prio!=prio)
  {
#               if DebugPrio<=1
    Dune::dvverb
      << "DDD_PrioEnd/ScatterPrio " << OBJ_GID(obj) << "/" << OBJ_PRIO(obj)
      << ", copy on proc " << proc << "/p" << prio
      << " changed prio " << prio << " -> " << real_prio << "\n";
#               endif

    ModCoupling(context, obj, proc, real_prio);
  }
#       if DebugPrio<=1
  else
  {
    Dune::dvverb
      << "DDD_PrioEnd/ScatterPrio " << OBJ_GID(obj) << "/" << OBJ_PRIO(obj)
      << ", copy on proc " << proc << "/p" << prio
      << " keeps prio " << prio << "\n";
  }
#       endif

  return(0);
}


/**
        End of PrioEnvironment.
        This function starts the actual process of changing priorities.
        After a call to this function (on all processors) all
        \funk{PrioChange}-commands since the last call to \funk{PrioBegin}
        are executed. This involves a set of interface communications
        between the processors.
 */

DDD_RET DDD_PrioEnd(DDD::DDDContext& context)
{
  /* step mode and check whether call to PrioEnd is valid */
  if (!PrioStepMode(context, PrioMode::PMODE_CMDS))
    DUNE_THROW(Dune::Exception, "DDD_PrioEnd() aborted");


  ddd_StdIFExchangeX(context, sizeof(DDD_PRIO), GatherPrio, ScatterPrio);

  /*
          free temporary storage
   */
  STAT_RESET;
  IFAllFromScratch(context);
  STAT_TIMER(T_PRIO_BUILD_IF);


  PrioStepMode(context, PrioMode::PMODE_BUSY);

  return(DDD_RET_OK);
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrioBegin                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Starts a PrioEnvironment.
        A call to this function establishes a global operation of changing
        priorities.  It must be issued on all processors. After this call an
        arbitrary series of \funk{PrioChange}-commands may be issued. The
        global transfer operation is carried out via a \funk{PrioEnd} call on
        each processor.
 */

void DDD_PrioBegin(DDD::DDDContext& context)
{
  /* step mode and check whether call to JoinBegin is valid */
  if (!PrioStepMode(context, PrioMode::PMODE_IDLE))
    DUNE_THROW(Dune::Exception, "DDD_PrioBegin() aborted");
}



END_UGDIM_NAMESPACE
