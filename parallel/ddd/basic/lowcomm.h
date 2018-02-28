// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      lowcomm.h                                                     */
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
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DDD_LOWCOMM_H__
#define __DDD_LOWCOMM_H__

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

#define EXCEPTION_LOWCOMM_CONNECT  -10

/* lowcomm users should use exceptions EXCEPTION_LOWCOMM_USER or lower */
#define EXCEPTION_LOWCOMM_USER     -100


/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/


typedef unsigned long ULONG;

/**
 * opaque data type for messages
 */
struct MSG_DESC;

/**
 * handle for messages
 */
using LC_MSGHANDLE = MSG_DESC*;

/**
 * opaque data type for message types
 */
struct MSG_TYPE;

/**
 * handle for message types (on send AND recv side)
 */
using LC_MSGTYPE = MSG_TYPE*;

/**
 * component of message (on send AND recv side)
 */
typedef int LC_MSGCOMP;


/* function pointer types for alloc and free */
typedef void * (*AllocFunc)(size_t);
typedef void (*FreeFunc)(void *);


/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/


/* lowcomm.c */
void  LC_Init(DDD::DDDContext& context, AllocFunc,FreeFunc);
void  LC_Exit(DDD::DDDContext& context);

void  LC_SetMemMgrSend(DDD::DDDContext&, AllocFunc,FreeFunc);
void  LC_SetMemMgrRecv(DDD::DDDContext&, AllocFunc,FreeFunc);
void  LC_SetMemMgrDefault(DDD::DDDContext&);


LC_MSGTYPE LC_NewMsgType (DDD::DDDContext& context, const char *);
LC_MSGCOMP LC_NewMsgTable (const char *, LC_MSGTYPE, size_t);
LC_MSGCOMP LC_NewMsgChunk (const char *, LC_MSGTYPE);

void       LC_MsgSend (const DDD::DDDContext& context, LC_MSGHANDLE);

int           LC_Connect(DDD::DDDContext& context, LC_MSGTYPE);
int           LC_Abort(DDD::DDDContext& context, int);
LC_MSGHANDLE *LC_Communicate(const DDD::DDDContext& context);
void          LC_Cleanup(DDD::DDDContext& context);



LC_MSGHANDLE LC_NewSendMsg(DDD::DDDContext& context, LC_MSGTYPE, DDD_PROC);
ULONG    LC_GetTableLen (LC_MSGHANDLE, LC_MSGCOMP);
void *   LC_GetPtr (LC_MSGHANDLE, LC_MSGCOMP);
DDD_PROC LC_MsgGetProc (LC_MSGHANDLE);

size_t   LC_MsgPrepareSend(DDD::DDDContext& context, LC_MSGHANDLE);
size_t   LC_MsgFreeze (LC_MSGHANDLE);
int      LC_MsgAlloc(DDD::DDDContext& context, LC_MSGHANDLE);

void     LC_SetTableLen (LC_MSGHANDLE, LC_MSGCOMP, ULONG);
void     LC_SetTableSize (LC_MSGHANDLE, LC_MSGCOMP, ULONG);
void     LC_SetChunkSize (LC_MSGHANDLE, LC_MSGCOMP, size_t);

size_t   LC_GetBufferSize (LC_MSGHANDLE);


void LC_PrintSendMsgs(const DDD::DDDContext& context);
void LC_PrintRecvMsgs(const DDD::DDDContext& context);

END_UGDIM_NAMESPACE

#endif
