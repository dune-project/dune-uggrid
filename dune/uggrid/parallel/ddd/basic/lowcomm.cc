// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      lowcomm.c                                                     */
/*                                                                          */
/* Purpose:   lowlevel communication layer                                  */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            internet: birken@ica3.uni-stuttgart.de                        */
/*                                                                          */
/* History:   960715 kb  begin                                              */
/*            971007 kb  reworked                                           */
/*                                                                          */
/* Remarks:                                                                 */
/*            This module provides two basic abstractions:                  */
/*            - sending of messages without explicit receive calls          */
/*            - message types consisting of a set of components, where      */
/*              components are tables (with entries of equal sizes) and     */
/*              raw data chunks.                                            */
/*                                                                          */
/*            The LowComm subsystem uses the Notify-subsystem in order to   */
/*            tell receiving processors that corresponding send-calls had   */
/*            been issued.                                                  */
/*                                                                          */
/*            The structure of each message is:                             */
/*                                                                          */
/*               description                               |  type          */
/*              -------------------------------------------+---------       */
/*               magic number                              |  ULONG         */
/*               #components                               |  ULONG         */
/*               offset component1 (from beginning of Msg) |  ULONG         */
/*               length component1 (in bytes)              |  ULONG         */
/*               nItems component1                         |  ULONG         */
/*                 ...                                     |  ...           */
/*               offset componentN                         |  ULONG         */
/*               length componentN                         |  ULONG         */
/*               nItems componentN                         |  ULONG         */
/*               component1                                                 */
/*                ...                                                       */
/*               componentN                                                 */
/*                                                                          */
/*            The LowComm subsystem is able to handle low-memory situations,*/
/*            where the available memory is not enough for all send- and    */
/*            receive-buffers. See LC_MsgAlloc for details.                 */
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
#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <iomanip>
#include <iostream>
#include <new>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "lowcomm.h"
#include "notify.h"

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

#define DebugLowComm  10  /* 0 is all, 10 is off */

namespace DDD {
namespace Basic {

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/


/* maximum number of components in a message */
#define MAX_COMPONENTS 8


/* dummy magic number for messages */
#define MAGIC_DUMMY 0x1234


/* number of entries per chunk in message header */
#define HDR_ENTRIES_PER_CHUNK   3

enum CompType {
  CT_NONE,
  CT_TABLE,
  CT_CHUNK
};


/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

struct COMP_DESC
{
  const char *name;               /* textual description of component */
  int type;                    /* type of this component */
  size_t entry_size;           /* size per entry (for tables) */
};


struct MSG_TYPE
{
  const char *name;                      /* textual description of msgtype */
  int nComps;                            /* number of components */
  std::array<COMP_DESC, MAX_COMPONENTS> comp;        /* component array */

  MSG_TYPE *next;         /* linked list of all message types */
};


struct CHUNK_DESC
{
  size_t size;           /* size of chunk (in bytes) */
  ULONG entries;         /* number of valid entries (for tables) */
  ULONG offset;          /* offset of chunk in MSG */
};


enum MsgState { MSTATE_NEW, MSTATE_FREEZED, MSTATE_ALLOCATED, MSTATE_COMM, MSTATE_READY };

struct MSG_DESC
{
  int msgState;                /* message state of this message (one of MsgState) */
  MSG_TYPE   *msgType;         /* message type of this message */

  ULONG magic;                 /* magic number */
  CHUNK_DESC *chunks;          /* array of chunks */


  size_t bufferSize;           /* size of message buffer (in bytes) */
  char    *buffer;             /* address of message buffer */


  MSG_DESC *next;              /* linked list inside Send/Recv-queue */
  DDD_PROC proc;
  msgid msgId;

};


struct TABLE_DESC
{
  size_t entry_size;             /* size of one table entry */
  int nMax;                      /* number of entries in table */
  int nValid;                    /* number of valid entries */
};

} /* namespace Basic */
} /* namespace DDD */

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

namespace DDD {

using namespace DDD::Basic;

/**
        Initiates LowComm subsystem.
        This function has to be called exactly once in order
        to initialize the LowComm subsystem. After a call to
        this function, the functionality of the LowComm can
        be used.

   @param aAllocFunc memory allocation function used as the default
   @param aFreeFunc  memory free function used as the default
 */

void LC_Init(DDD::DDDContext& context, AllocFunc aAllocFunc, FreeFunc aFreeFunc)
{
  auto& lcContext = context.lowCommContext();
  lcContext.DefaultAlloc = aAllocFunc;
  lcContext.DefaultFree  = aFreeFunc;
  LC_SetMemMgrDefault(context);
}



/**
        Exits LowComm subsystem.
        This function frees memory allocated by the LowComm subsystem
        and shuts down its communication structures.
 */

void LC_Exit(DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();

  {
    auto md = lcContext.FreeMsgDescs;
    while (md != nullptr) {
      const auto next = md->next;
      delete md;
      md = next;
    }
    lcContext.FreeMsgDescs = nullptr;
  }

  {
    auto mt = lcContext.MsgTypes;
    while (mt != nullptr) {
      const auto next = mt->next;
      delete mt;
      mt = next;
    }
    lcContext.MsgTypes = nullptr;
  }
}




/**
        Customizing memory management for LowComm subsystem.
        Currently this function supports only alloc/free of
        buffers for messages to be sent.
 */
void LC_SetMemMgrSend(DDD::DDDContext& context, AllocFunc aAllocFunc, FreeFunc aFreeFunc)
{
  auto& lcContext = context.lowCommContext();
  lcContext.SendAlloc = aAllocFunc;
  lcContext.SendFree  = aFreeFunc;
}


/**
        Customizing memory management for LowComm subsystem.
        Currently this function supports only alloc/free of
        buffers for messages to be received.
 */
void LC_SetMemMgrRecv(DDD::DDDContext& context, AllocFunc aAllocFunc, FreeFunc aFreeFunc)
{
  auto& lcContext = context.lowCommContext();
  lcContext.RecvAlloc = aAllocFunc;
  lcContext.RecvFree  = aFreeFunc;
}


/**
        Set memory management for LowComm subsystem to its default state.
        Set alloc/free of message buffers to the functions provided to
        \lcfunk{Init}.
 */
void LC_SetMemMgrDefault(DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();
  lcContext.SendAlloc = lcContext.DefaultAlloc;
  lcContext.SendFree  = lcContext.DefaultFree;
  lcContext.RecvAlloc = lcContext.DefaultAlloc;
  lcContext.RecvFree  = lcContext.DefaultFree;
}


/****************************************************************************/

/*
        auxiliary functions
 */

static MSG_DESC *NewMsgDesc (DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();
  MSG_DESC *md;

  if (lcContext.FreeMsgDescs != nullptr)
  {
    /* get item from freelist */
    md = lcContext.FreeMsgDescs;
    lcContext.FreeMsgDescs = lcContext.FreeMsgDescs->next;
  }
  else
  {
    /* freelist is empty */
    md = new MSG_DESC;
  }

  return(md);
}


static void FreeMsgDesc (DDD::DDDContext& context, MSG_DESC *md)
{
  auto& lcContext = context.lowCommContext();

  /* sort into freelist */
  md->next = lcContext.FreeMsgDescs;
  lcContext.FreeMsgDescs = md;
}


/****************************************************************************/



/*
        this function has internal access only because LowComm initiates
        asynchronous receive calls itself.
 */

static LC_MSGHANDLE LC_NewRecvMsg (DDD::DDDContext& context, LC_MSGTYPE mtyp, DDD_PROC source, size_t size)
{
  auto& lcContext = context.lowCommContext();
  MSG_DESC *msg = NewMsgDesc(context);

#       if DebugLowComm<=6
  Dune::dverb << "LC_NewRecvMsg(" << mtyp->name << ") source=" << source << "\n";
#       endif

  msg->msgState = MSTATE_NEW;
  msg->msgType = mtyp;
  msg->proc = source;
  msg->bufferSize = size;

  /* allocate chunks array */
  msg->chunks = new CHUNK_DESC[mtyp->nComps];

  /* enter message into recv queue */
  msg->next = lcContext.RecvQueue;
  lcContext.RecvQueue = msg;

  return msg;
}




static void LC_DeleteMsg (DDD::DDDContext& context, LC_MSGHANDLE md)
{
  delete[] md->chunks;
  FreeMsgDesc(context, md);
}


static void LC_DeleteMsgBuffer (const DDD::DDDContext& context, LC_MSGHANDLE md)
{
  const auto& lcContext = context.lowCommContext();

  if (lcContext.SendFree != nullptr)
    (*lcContext.SendFree)(md->buffer);
}




static void LC_MsgRecv (MSG_DESC *md)
{
  int i, j;

  /* get message address */
  ULONG    *hdr = (ULONG *)md->buffer;

  /* get number of chunks */
  int n = (int)(hdr[1]);

  /* magic number is hdr[0] */
  if (hdr[0] != MAGIC_DUMMY)
    DUNE_THROW(Dune::Exception, "invalid magic number for message from " << md->proc);

  /* number of chunks must be consistent with message type */
  if (n!=md->msgType->nComps)
    DUNE_THROW(Dune::Exception,
               "wrong number of chunks (got " << n << ", expected "
               << md->msgType->nComps << ") in message from " << md->proc);

  /* get chunk descriptions from message header */
  for(j=2, i=0; i<n; i++)
  {
    md->chunks[i].offset  =          hdr[j++];
    md->chunks[i].size    = (size_t)(hdr[j++]);
    md->chunks[i].entries =          hdr[j++];
  }

        #if     DebugLowComm<=2
  Dune:dvverb << "LC_MsgRecv() from=" << md->proc << " ready\n";
        #endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_PollSend                                                   */
/*                                                                          */
/* Purpose:   polls all message-sends one time and returns remaining        */
/*            outstanding messages. whenever a message-send has been        */
/*            completed, its message buffer is freed.                       */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    remaining outstanding messages                                */
/*                                                                          */
/****************************************************************************/

static int LC_PollSend(const DDD::DDDContext& context)
{
  const auto& lcContext = context.lowCommContext();

  MSG_DESC *md;
  int remaining, error;

  remaining = 0;
  for(md=lcContext.SendQueue; md != nullptr; md=md->next)
  {
    if (md->msgState==MSTATE_COMM)
    {
      error = InfoASend(context.ppifContext(), VCHAN_TO(context, md->proc), md->msgId);
      if (error==-1)
        DUNE_THROW(Dune::Exception, "InfoASend() failed for message to proc=" << md->proc);

      if (error==1)
      {
        /* free message buffer */
        LC_DeleteMsgBuffer(context, (LC_MSGHANDLE)md);

        md->msgState=MSTATE_READY;
      }
      else
      {
        /* we keep this message in SendQueue */
        remaining++;
      }
    }
  }

        #if     DebugLowComm<=3
  Dune::dvverb << "LC_PollSend, " << remaining << " msgs remaining\n";
        #endif

  return(remaining);
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_PollRecv                                                   */
/*                                                                          */
/* Purpose:   polls all message-recvs one time and returns remaining        */
/*            outstanding messages. this function doesn't free the message  */
/*            buffers.                                                      */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    remaining outstanding messages                                */
/*                                                                          */
/****************************************************************************/

static int LC_PollRecv(const DDD::DDDContext& context)
{
  const auto& lcContext = context.lowCommContext();

  MSG_DESC *md;
  int remaining, error;

  remaining = 0;
  for(md=lcContext.RecvQueue; md != nullptr; md=md->next)
  {
    if (md->msgState==MSTATE_COMM)
    {
      error = InfoARecv(context.ppifContext(), VCHAN_TO(context, md->proc), md->msgId);
      if (error==-1)
        DUNE_THROW(Dune::Exception,
                   "InfoARecv() failed for recv from proc=" << md->proc);

      if (error==1)
      {
        LC_MsgRecv(md);

        md->msgState=MSTATE_READY;
      }
      else
      {
        remaining++;
      }
    }
  }

        #if     DebugLowComm<=3
  Dune::dvverb << "LC_PollRecv, " << remaining << " msgs remaining\n";
        #endif

  return(remaining);
}


/****************************************************************************/
/*                                                                          */
/* Function:  LC_FreeSendQueue                                              */
/*                                                                          */
/****************************************************************************/

static void LC_FreeSendQueue (DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();
  MSG_DESC *md, *next=NULL;

  for(md=lcContext.SendQueue; md != nullptr; md=next)
  {
    /* the following assertion is too picky. Freeing of
       message queues should be possible in all msgStates. */
    /*assert(md->msgState==MSTATE_READY);*/

    next = md->next;
    LC_DeleteMsg(context, (LC_MSGHANDLE)md);
  }


  lcContext.SendQueue = nullptr;
  lcContext.nSends = 0;
}


/****************************************************************************/
/*                                                                          */
/* Function:  LC_FreeRecvQueue                                              */
/*                                                                          */
/****************************************************************************/

static void LC_FreeRecvQueue (DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();
  MSG_DESC *md, *next=NULL;

  for(md=lcContext.RecvQueue; md != nullptr; md=next)
  {
    /* the following assertion is too picky. Freeing of
       message queues should be possible in all msgStates. */
    /*assert(md->msgState==MSTATE_READY);*/

    next = md->next;
    LC_DeleteMsg(context, (LC_MSGHANDLE)md);
  }


  lcContext.RecvQueue = nullptr;
  lcContext.nRecvs = 0;
}



/****************************************************************************/


/* LC_MsgFreeze and LC_MsgAlloc are the two parts of LC_MsgPrepareSend(). */

/* returns size of message buffer */

size_t LC_MsgFreeze (LC_MSGHANDLE md)
{
  int i, n = md->msgType->nComps;

  assert(md->msgState==MSTATE_NEW);

  /* compute size of header */
  md->bufferSize  = 2 * sizeof(ULONG);
  md->bufferSize += (n * HDR_ENTRIES_PER_CHUNK * sizeof(ULONG));

  /* compute size and offset for each chunk */
  for(i=0; i<n; i++)
  {
    md->chunks[i].offset = md->bufferSize;
    md->bufferSize += md->chunks[i].size;
  }

  md->msgState=MSTATE_FREEZED;

  return(md->bufferSize);
}



int LC_MsgAlloc(DDD::DDDContext& context, LC_MSGHANDLE md)
{
  auto& lcContext = context.lowCommContext();
  ULONG      *hdr;
  int i, j, n = md->msgType->nComps;
  int remaining=1, give_up = false;

  assert(md->msgState==MSTATE_FREEZED);

  /* the following code tries to allocate the message buffer.
     if this fails, the previously started asynchronous sends are
     polled, in order to free their message buffers. if there are
     no remaining async-sends, we give up. */
  do {
    /* allocate buffer for messages */
    md->buffer = (char *) (*lcContext.SendAlloc)(md->bufferSize);
    if (md->buffer==NULL)
    {
      if (remaining==0)
        give_up = true;
      else
      {
#                               if DebugLowComm<=7
        Dune::dinfo << "LC_MsgAlloc(" << md->msgType->name
                    << ") detected low memory.\n";
#                               endif

        /* couldn't get msg-buffer. try to poll previous messages. */
        /* first, poll receives to avoid communication deadlock. */
        LC_PollRecv(context);

        /* now, try to poll sends and free their message buffers */
        remaining  = LC_PollSend(context);

#                               if DebugLowComm<=6
        Dune::dverb << "LC_MsgAlloc(" << md->msgType->name
                    << ") preliminary poll, sends_left=" << remaining << "\n";
#                               endif
      }
    }
  } while (md->buffer==NULL && !give_up);

  if (give_up)
  {
#               if DebugLowComm<=7
    Dune::dinfo << "LC_MsgAlloc(" << md->msgType->name << ") giving up, no memory.\n";
#               endif
    return(false);
  }


  /* enter control data into message header */
  hdr = (ULONG *)md->buffer;
  j=0;
  hdr[j++] = MAGIC_DUMMY;         /* magic number */
  hdr[j++] = n;

  /* enter chunk descriptions into message header */
  for(i=0; i<n; i++)
  {
    hdr[j++] = md->chunks[i].offset;
    hdr[j++] = md->chunks[i].size;
    hdr[j++] = md->chunks[i].entries;
  }

  md->msgState=MSTATE_ALLOCATED;

  return(true);
}




/*
        allocation of receive message buffers.
        NOTE: one big memory block is allocated and used for all
              message buffers.
 */
static RETCODE LC_PrepareRecv(DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();
  MSG_DESC *md;
  size_t sumSize;
  char     *buffer;
  int error;

  /* compute sum of message buffer sizes */
  for(sumSize=0, md=lcContext.RecvQueue; md != nullptr; md=md->next)
  {
    assert(md->msgState==MSTATE_NEW);
    sumSize += md->bufferSize;
  }


  /* allocate buffer for messages */
  lcContext.theRecvBuffer = (char *) (*lcContext.RecvAlloc)(sumSize);
  if (lcContext.theRecvBuffer == nullptr)
  {
    Dune::dwarn << "Out of memory in LC_PrepareRecv "
                << "(size of message buffer: " << sumSize << ")";
    RET_ON_ERROR;
  }


  /* initiate receive calls */
  buffer = lcContext.theRecvBuffer;
  for(md=lcContext.RecvQueue; md != nullptr; md=md->next)
  {
    md->buffer = buffer;
    buffer += md->bufferSize;

    md->msgId = RecvASync(context.ppifContext(), VCHAN_TO(context, md->proc),
                          md->buffer, md->bufferSize, &error);

    md->msgState=MSTATE_COMM;
  }

  RET_ON_OK;
}


/****************************************************************************/

/*
        MSG_TYPE definition functions
 */


/****************************************************************************/
/*                                                                          */
/* Function:  LC_NewMsgType                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Declares new message-type.
        Before messages may be sent and received with the LowComm
        subsystem, at least one {\em message-type} must be defined by
        a global call to this function. Subsequently, calls to
        \lcfunk{NewMsgTable} and \lcfunk{NewMsgChunk} can be used
        in order to define the structure of the new message-type.

        Each message-type in the LowComm subsystem consists of a set
        of {\em message-components}. Possible message-components are:
        {\em tables} (with entries of equal size) and raw {\em data chunks}.
        The set of message-components has the same structure for
        all messages of the same type, but the number of table entries
        and the size of the data chunks differ from message to message.

   @return identifier of new message-type
   @param aName  name of message-type. This string is used for debugging
        and logging output.
 */

LC_MSGTYPE LC_NewMsgType(DDD::DDDContext& context, const char *aName)
{
  auto& lcContext = context.lowCommContext();
  MSG_TYPE *mt;

  mt = new MSG_TYPE;
  mt->name   = aName;
  mt->nComps = 0;

  /* insert into linked list of message types */
  mt->next = lcContext.MsgTypes;
  lcContext.MsgTypes = mt;

  return mt;
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_NewMsgChunk                                                */
/*                                                                          */
/****************************************************************************/

/**
        Add data chunk to current set of a message-type's message-components.
        This function is called after a previous call to \lcfunk{NewMsgType}
        in order to add a new message-component to the message-type.
        The component added by this function is a chunk of raw data.
        The size of the chunk is not specified here, use \lcfunk{SetChunkSize}
        for specifying the data chunk size for a given (concrete) message.

        See \lcfunk{NewMsgTable} for adding message-tables, which are
        a different kind of message-component.

   @return           identifier of new message-component
   @param  aName     name of new message component
   @param  mtyp      previously declared message-type
 */

LC_MSGCOMP LC_NewMsgChunk (const char *aName, LC_MSGTYPE mtyp)
{
  LC_MSGCOMP id = mtyp->nComps++;

  if (id>=MAX_COMPONENTS)
    DUNE_THROW(Dune::Exception,
               "too many message components (max. " << MAX_COMPONENTS << ")");

  mtyp->comp[id].type = CT_CHUNK;
  mtyp->comp[id].name = aName;

  return(id);
}




/****************************************************************************/
/*                                                                          */
/* Function:  LC_NewMsgTable                                                */
/*                                                                          */
/****************************************************************************/

/**
        Add table to current set of a message-type's message-components.
        This function is called after a previous call to \lcfunk{NewMsgType}
        in order to add a new message-component to the message-type.
        The component added by this function is a table of data, where
        each table entry has the same size.
        The overall size of the whole table is not specified here, but only
        the size for one table entry. Use \lcfunk{SetTableSize} for setting
        the number of reserved table entries in a given (concrete) message;
        use \lcfunk{SetTableLen} in order to specify the number of valid
        entries in a given message.

        See \lcfunk{NewMsgChunk} for adding data chunks, which are
        a different kind of message-component.

   @return           identifier of new message-component
   @param  aName     name of new message component
   @param  mtyp      previously declared message-type
   @param  aSize     size of each table entry (in byte)
 */

LC_MSGCOMP LC_NewMsgTable (const char *aName, LC_MSGTYPE mtyp, size_t aSize)
{
  LC_MSGCOMP id = mtyp->nComps++;

  if (id>=MAX_COMPONENTS)
    DUNE_THROW(Dune::Exception,
               "too many message components (max. " << MAX_COMPONENTS << ")");

  mtyp->comp[id].type = CT_TABLE;
  mtyp->comp[id].entry_size = aSize;
  mtyp->comp[id].name = aName;

  return(id);
}



/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* Function:  LC_NewSendMsg                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Create new message on sending processor.
        This function creates a new message handle on the sending processor and
        links it into the LowComm send-queue. The message has a given message-type
        and a given destination processor. Before the message is actually sent
        (by calling \lcfunk{MsgSend}), the sizes of the message's components
        must be set (\lcfunk{SetTableSize}, \lcfunk{SetChunkSize}) and the message
        buffer must be prepared (via \lcfunk{MsgPrepareSend}). After that,
        the message's tables and chunks can be filled with data and the message
        sending process can be initiated by \lcfunk{MsgSend}.

   @return          identifier of new message
   @param mtyp      message-type for new message
   @param aDest     destination processor of new message
 */

LC_MSGHANDLE LC_NewSendMsg(DDD::DDDContext& context, LC_MSGTYPE mtyp, DDD_PROC aDest)
{
  auto& lcContext = context.lowCommContext();

  MSG_DESC *msg = NewMsgDesc(context);

#       if DebugLowComm<=6
  Dune::dverb << "LC_NewSendMsg(" << mtyp->name << ") dest="
              << aDest << " nSends=" << (lcContext.nSends+1) << "\n";
#       endif


  msg->msgState = MSTATE_NEW;
  msg->msgType = mtyp;
  msg->proc = aDest;
  msg->bufferSize = 0;

  /* allocate chunks array */
  msg->chunks = new CHUNK_DESC[mtyp->nComps];

  /* enter message into send queue */
  msg->next = lcContext.SendQueue;
  lcContext.SendQueue = msg;
  lcContext.nSends++;

  return msg;
}




void LC_SetChunkSize (LC_MSGHANDLE md, LC_MSGCOMP id, size_t size)
{
  assert(md->msgState==MSTATE_NEW);
  assert(id < md->msgType->nComps);

  md->chunks[id].size = size;
  md->chunks[id].entries = 1;
}


void LC_SetTableSize (LC_MSGHANDLE md, LC_MSGCOMP id, ULONG entries)
{
  assert(md->msgState==MSTATE_NEW);
  assert(id < md->msgType->nComps);

  md->chunks[id].size = ((int)entries) * md->msgType->comp[id].entry_size;
  md->chunks[id].entries = entries;
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_MsgPrepareSend                                             */
/*                                                                          */
/****************************************************************************/


/* returns size of message buffer */

size_t LC_MsgPrepareSend (DDD::DDDContext& context, LC_MSGHANDLE msg)
{
  size_t size = LC_MsgFreeze(msg);
  if (! LC_MsgAlloc(context, msg))
    throw std::bad_alloc();

  return(size);
}




DDD_PROC LC_MsgGetProc (const LC_MSGHANDLE md)
{
  return md->proc;
}


void *LC_GetPtr (LC_MSGHANDLE md, LC_MSGCOMP id)
{
  return ((void *)(((char *)md->buffer) + md->chunks[id].offset));
}


void LC_SetTableLen (LC_MSGHANDLE md, LC_MSGCOMP id, ULONG n)
{
  ULONG *hdr = (ULONG *)md->buffer;

  hdr[HDR_ENTRIES_PER_CHUNK*id+4] = n;
  md->chunks[id].entries = n;
}


ULONG LC_GetTableLen (LC_MSGHANDLE md, LC_MSGCOMP id)
{
  return((ULONG)md->chunks[id].entries);
}


void LC_MsgSend(const DDD::DDDContext& context, LC_MSGHANDLE md)
{
  int error;

  assert(md->msgState==MSTATE_ALLOCATED);

  /* initiate asynchronous send */
  md->msgId = SendASync(context.ppifContext(), VCHAN_TO(context, md->proc),
                        md->buffer, md->bufferSize, &error);

  md->msgState=MSTATE_COMM;
}


size_t LC_GetBufferSize (const LC_MSGHANDLE md)
{
  return(md->bufferSize);
}


/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* Function:  LC_Connect                                                    */
/*                                                                          */
/****************************************************************************/

int LC_Connect(DDD::DDDContext& context, LC_MSGTYPE mtyp)
{
  auto& lcContext = context.lowCommContext();

  DDD_PROC    *partners = DDD_ProcArray(context);
  NOTIFY_DESC *msgs = DDD_NotifyBegin(context, lcContext.nSends);
  MSG_DESC *md;
  int i, p;

  const auto procs = context.procs();

  if (lcContext.nSends<0 || lcContext.nSends>procs-1)
    DUNE_THROW(Dune::Exception,
               "cannot send " << lcContext.nSends << "messages "
               "(must be less than " << (procs-1) << ")");

#       if DebugLowComm<=9
  Dune::dinfo << "LC_Connect(" << mtyp->name
              << ") nSends=" << lcContext.nSends << " ...\n";
#       endif



  /* fill notify array */
  for(i=0, p=0, md=lcContext.SendQueue; md != nullptr; i++, md=md->next)
  {
    msgs[i].proc = md->proc;
    msgs[i].size = md->bufferSize;

    /* enhance list of communication partners (destinations) */
    partners[p++] = md->proc;

  }


  /* inform message receivers */
  lcContext.nRecvs = DDD_Notify(context);
  if (lcContext.nRecvs<0)
  {
    /* some processor raised an exception */
    Dune::dwarn << "Notify() raised exception #"
                << (-lcContext.nRecvs) << " in LC_Connect()\n";

    /* automatically call LC_Cleanup() */
    DDD_NotifyEnd(context);
    LC_Cleanup(context);

    return lcContext.nRecvs;
  }


  if (lcContext.nRecvs>procs-1)
  {
    Dune::dwarn << "cannot receive " << lcContext.nRecvs
                << " messages (must be less than " << (procs-1) << ")\n";
    DDD_NotifyEnd(context);
    return(EXCEPTION_LOWCOMM_CONNECT);
  }



#       if DebugLowComm<=7
  Dune::dinfo << "LC_Connect() nSends=" << lcContext.nSends
              << " nRecvs=" << lcContext.nRecvs << "\n";
#       endif


  /* create array of receive message handles */
  if (lcContext.nRecvs>0)
    lcContext.theRecvArray = new LC_MSGHANDLE[lcContext.nRecvs];


  /* create recv messages from notify array */
  for(i=0; i < lcContext.nRecvs; i++)
  {
    /* create recv message handle and store it in MSGHANDLE array */
    lcContext.theRecvArray[i] = LC_NewRecvMsg(context, mtyp, msgs[i].proc, msgs[i].size);

    /* enhance list of communication partners (sources) */
    partners[p++] = msgs[i].proc;

  }


  DDD_NotifyEnd(context);


  /* get necessary connections to comm-partners */
  if (p>0)
  {
    if (! IS_OK(DDD_GetChannels(context, lcContext.nRecvs+lcContext.nSends)))
    {
      DDD_PrintError('E', 6620, "couldn't get channels in LC_Connect()");
      return(EXCEPTION_LOWCOMM_CONNECT);
    }
  }


#       if DebugLowComm<=5
  DDD_DisplayTopo(context);
#       endif


  if (lcContext.nRecvs>0)
  {
    if (! IS_OK(LC_PrepareRecv(context)))
      return(EXCEPTION_LOWCOMM_CONNECT);
  }


#       if DebugLowComm<=9
  Dune::dinfo << "LC_Connect() ready\n";
#       endif

  return lcContext.nRecvs;
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_Abort                                                      */
/*                                                                          */
/****************************************************************************/

int LC_Abort(DDD::DDDContext& context, int exception)
{
  int retException;

  if (exception>EXCEPTION_LOWCOMM_USER)
    DUNE_THROW(Dune::Exception,
               "exception must be <= EXCEPTION_LOWCOMM_USER");

  DDD_NotifyBegin(context, exception);

#       if DebugLowComm<=9
  Dune::dwarn << "LC_Abort() exception=" << exception << " ...\n";
#       endif


  /* inform message receivers */
  retException = DDD_Notify(context);

  DDD_NotifyEnd(context);


#       if DebugLowComm<=9
  Dune::dwarn << "LC_Abort() ready, exception=" << retException << "\n";
#       endif


  /* automatically call LC_Cleanup() */
  LC_Cleanup(context);

  return(retException);
}



/****************************************************************************/
/*                                                                          */
/* Function:  LC_Communicate                                                */
/*                                                                          */
/****************************************************************************/

LC_MSGHANDLE *LC_Communicate(const DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();

#       if DebugLowComm<=9
  Dune::dinfo << "LC_Communicate() ...\n";
#       endif


  /* poll asynchronous send and receives */
  int leftSend = lcContext.nSends;
  int leftRecv = lcContext.nRecvs;
  do {
    if (leftRecv>0) leftRecv = LC_PollRecv(context);
    if (leftSend>0) leftSend = LC_PollSend(context);
  } while (leftRecv>0 || leftSend>0);


#       if DebugLowComm<=9
  Dune::dinfo << "LC_Communicate() ready\n";
#       endif

  return lcContext.theRecvArray;
}


/****************************************************************************/
/*                                                                          */
/* Function:  LC_Cleanup                                                    */
/*                                                                          */
/****************************************************************************/

void LC_Cleanup(DDD::DDDContext& context)
{
  auto& lcContext = context.lowCommContext();

#       if DebugLowComm<=9
  Dune::dinfo << "LC_Cleanup() ...\n";
#       endif

  if (lcContext.nRecvs>0)
  {
    if (lcContext.RecvFree != nullptr)
      (lcContext.RecvFree)(lcContext.theRecvBuffer);

    lcContext.theRecvBuffer = nullptr;
  }

  if (lcContext.theRecvArray != nullptr)
  {
    delete[] lcContext.theRecvArray;
    lcContext.theRecvArray = nullptr;
  }

  /* free recv queue */
  LC_FreeRecvQueue(context);

  /* free send queue */
  LC_FreeSendQueue(context);


#       if DebugLowComm<=9
  Dune::dinfo << "LC_Cleanup() ready\n";
#       endif
}


/****************************************************************************/

#define LC_COLWIDTH   10

static const char* LC_DefaultName = "<?>";

/* construct name or default name */
static const char* LC_Name(const char* name)
{
  return name ? name : LC_DefaultName;
}

static void LC_PrintMsgList (MSG_DESC *list)
{
  using std::setw;

  std::ostream& out = std::cout;
  MSG_DESC *md;
  MSG_TYPE *last_mt=NULL;
  size_t sum, comp_size[MAX_COMPONENTS];
  int i;

  for(md=list; md != nullptr; md=md->next)
  {
    MSG_TYPE *mt = md->msgType;

    if (mt!=last_mt)
    {
      /* msg-type changes, print new header */

      /* first, close part of msg-list with summary */
      if (last_mt!=NULL)
      {
        out << "        = |";
        sum = 0;
        for(i=0; i<last_mt->nComps; i++)
        {
          out << setw(9) << comp_size[i];
          sum += comp_size[i];                                 /* horizontal sum */
        }
        out << setw(9) << sum << "\n";
      }

      /* then, construct header */
      {
        std::string name = LC_Name(mt->name);
        out << setw(9) << name.substr(0, 9) << " |";
      }
      for(i=0; i<mt->nComps; i++)
      {
        if (mt->comp[i].name!=NULL) {
          std::string name = LC_Name(mt->comp[i].name);
          out << setw(9) << name.substr(0, 9);
        }
        else
          out << setw(9) << i;

        comp_size[i] = 0;
      }
      out << "        =\n";
      last_mt = mt;
    }

    /* construct info about message components */
    out << setw(9) << md->proc << " |";
    sum = 0;
    for(i=0; i<mt->nComps; i++)
    {
      size_t s = md->chunks[i].size;

      out << setw(9) << s;

      sum          += s;                     /* horizontal sum */
      comp_size[i] += s;                     /* vertical sum */
    }
    out << setw(9) << sum << "\n";
  }

  /* close last part of msg-list with summary */
  if (last_mt!=NULL)
  {
    out << "        = |";
    sum = 0;
    for(i=0; i<last_mt->nComps; i++)
    {
      out << setw(9) << comp_size[i];
      sum += comp_size[i];                     /* horizontal sum */
    }
    out << setw(9) << sum << "\n";
  }

}


void LC_PrintSendMsgs(const DDD::DDDContext& context)
{
  LC_PrintMsgList(context.lowCommContext().SendQueue);
}

void LC_PrintRecvMsgs(const DDD::DDDContext& context)
{
  LC_PrintMsgList(context.lowCommContext().RecvQueue);
}


/****************************************************************************/

} /* namespace DDD */
