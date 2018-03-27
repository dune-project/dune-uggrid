// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
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

/*
   #define CtrlTimeouts
   #define CtrlTimeoutsDetailed
 */


/* maximum number of interfaces */
#define MAX_IF                  32


/* maximum length of interface description string */
#define IF_NAMELEN      80



enum CplDir {
  DirAB  =  0x01,
  DirBA  =  0x02,
  DirABA =  DirAB|DirBA
};





/* macros for easier coding of replicated source code */

/* loop over one interface (all ifHeads) */
#define ForIF(id,iter)  for((iter)=NS_DIM_PREFIX theIF[(id)].ifHead;  \
                            (iter)!=NULL;  \
                            (iter)=(iter)->next)


typedef DDD_OBJ IFObjPtr;



/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* IF_ATTR: single part of interface, all couplings have same attr          */
/****************************************************************************/

struct IF_ATTR
{
  IF_ATTR* next;

  /* note: the cplXX resp. objXX arrays are NOT contiguous in memory */
  COUPLING   **cplAB, **cplBA, **cplABA;
  IFObjPtr   *objAB,  *objBA,  *objABA;       /* object shortcut */
  int nItems, nAB, nBA, nABA;
  DDD_ATTR attr;
};



/****************************************************************************/
/* IF_PROC: descriptor of message and its contents/buffers for IF-communic. */
/****************************************************************************/

struct IF_PROC
{
  IF_PROC* next;
  IF_ATTR    *ifAttr;
  int nAttrs;

  /* note: the cplXX resp. objXX arrays ARE contiguous in memory */
  COUPLING   **cpl, **cplAB, **cplBA, **cplABA;
  IFObjPtr   *obj,  *objAB,  *objBA,  *objABA;       /* object shortcut */
  int nItems, nAB, nBA, nABA;
  DDD_PROC proc;

  VChannelPtr vc;
  msgid msgIn;
  msgid msgOut;
  std::vector<char> bufIn;
  std::vector<char> bufOut;
};



/****************************************************************************/
/* IF_DEF: descriptor for one single interface                              */
/****************************************************************************/

struct IF_DEF
{
  IF_PROC   *ifHead;
  COUPLING  **cpl;              /* list of couplings belonging to interface     */
  int nItems;                   /* overall number of items in this interface    */

  IFObjPtr  *obj;              /* shortcut: list of object addresses in interf */
  int objValid;                 /* flag: is obj-table valid?                    */

  int nIfHeads;

  int nObjStruct;
  int nPrioA;
  int nPrioB;
  DDD_TYPE O[16];
  DDD_PRIO A[16];
  DDD_PRIO B[16];

  /* data for efficiency tuning */
  int maskO;

  /* data for nice user interaction */
  char name[IF_NAMELEN+1];            /* string for interface identification */
};



/****************************************************************************/
/*                                                                          */
/* definition of variables for corresponding module                         */
/*                                                                          */
/****************************************************************************/

extern IF_DEF theIF[MAX_IF];
extern int nIFs;



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
void IFCreateObjShortcut (DDD_IF);
void IFCheckShortcuts (DDD_IF);


/****************************************************************************/

END_UGDIM_NAMESPACE

#endif
