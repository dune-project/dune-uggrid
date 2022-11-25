// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************/
/*                                                                          */
/* File:      if.h                                                          */
/*                                                                          */
/* Purpose:   include file for DDD interface module                         */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/09/21 kb  extracted from if.c                              */
/*            96/01/16 kb  added DDD_OBJ shortcut to ifHead and ifAttr      */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


#ifndef __DDD_IF_H__
#define __DDD_IF_H__

#include <vector>

#include <dune/uggrid/parallel/ddd/dddtypes_impl.hh>

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

START_UGDIM_NAMESPACE

using namespace DDD::If;

/*
   #define CtrlTimeouts
   #define CtrlTimeoutsDetailed
 */


enum CplDir {
  DirAB  =  0x01,
  DirBA  =  0x02,
  DirABA =  DirAB|DirBA
};





/* macros for easier coding of replicated source code */

/* loop over one interface (all ifHeads) */
#define ForIF(context, id, iter)                            \
  for((iter)=context.ifCreateContext().theIf[(id)].ifHead;  \
      (iter)!=NULL;                                         \
      (iter)=(iter)->next)


/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/


/* ifuse.c */
void    IFGetMem (IF_PROC *, size_t, int, int);
int     IFInitComm(DDD::DDDContext& context, DDD_IF);
void    IFExitComm(DDD::DDDContext& context, DDD_IF);
void    IFInitSend(DDD::DDDContext& context, IF_PROC *);
int     IFPollSend(DDD::DDDContext& context, DDD_IF);
char *  IFCommLoopObj (DDD::DDDContext& context, ComProcPtr2, IFObjPtr *, char *, size_t, int);
char *  IFCommLoopCpl (DDD::DDDContext& context, ComProcPtr2, COUPLING **, char *, size_t, int);
char *  IFCommLoopCplX (DDD::DDDContext& context, ComProcXPtr, COUPLING **, char *, size_t , int);
void    IFExecLoopObj (DDD::DDDContext& context, ExecProcPtr, IFObjPtr *, int);
void    IFExecLoopCplX (DDD::DDDContext& context, ExecProcXPtr, COUPLING **, int);
char *  IFCommHdrLoopCpl (DDD::DDDContext& context, ComProcHdrPtr, COUPLING **, char *, size_t, int);
char *  IFCommHdrLoopCplX (DDD::DDDContext& context, ComProcHdrXPtr, COUPLING **, char *, size_t, int);
void    IFExecHdrLoopCpl (DDD::DDDContext& context, ExecProcHdrPtr, COUPLING **, int);
void    IFExecHdrLoopCplX (DDD::DDDContext& context, ExecProcHdrXPtr, COUPLING **, int);


/* ifobjsc.c */
void IFCreateObjShortcut(DDD::DDDContext& context, DDD_IF);
void IFCheckShortcuts(DDD::DDDContext& context, DDD_IF);


/****************************************************************************/

END_UGDIM_NAMESPACE

#endif
