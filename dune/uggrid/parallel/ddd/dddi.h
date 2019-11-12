// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      dddi.h                                                        */
/*                                                                          */
/* Purpose:   internal header file for ddd module                           */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/*                                                                          */
/* History:   93/11/22 kb  begin                                            */
/*            95/10/05 kb  added casts to mem-management macros             */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/*
   #define CheckIFMEM
   #define CheckPMEM
   #define CheckCplMEM
   #define CheckMsgMEM
   #define CheckTmpMEM
   #define CheckHeapMem
 */


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DDDI_H__
#define __DDDI_H__

#include <climits>
#include <memory>
#include <vector>

#include <cassert>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/parallel/ppif/ppif.h>
#include <dune/uggrid/parallel/ddd/ctrl/stat.h>
#include <dune/uggrid/parallel/ddd/include/ddd.h>
#include <dune/uggrid/parallel/ddd/include/dddio.h>
#include <dune/uggrid/parallel/ddd/include/memmgr.h>

START_UGDIM_NAMESPACE

using namespace DDD;

/*
        macro for exiting program in case of a severe error condition
 */
#define HARD_EXIT  assert(0)
/* #define HARD_EXIT  exit(1) */


/*
        macros for correct return or premature abort of a procedure
 */
#define RET_ON_OK      return (true)
#define RET_ON_ERROR   return (false)
#define IS_OK(p)       ((p)==true)


/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

/*** DDD internal parameters ***/

#define MAX_PRIO       32     /* max. number of DDD_PRIO                    */
#define MAX_CPL_START  65536  /* max. number of local objects with coupling */

#ifdef WithFullObjectTable
#define MAX_OBJ_START  262144 /* max. number of locally registered objects  */
#else
#define MAX_OBJ_START  MAX_CPL_START
#endif



#define MAX_TRIES  50000000  /* max. number of tries until timeout in IF-comm */

#ifdef DDD_MAX_PROCBITS_IN_GID
#define MAX_PROCBITS_IN_GID DDD_MAX_PROCBITS_IN_GID
#else
#define MAX_PROCBITS_IN_GID 24  /* this allows 2^24 procs and 2^40 objects  */
#endif

/* use maximum as default, if no Priomerge-matrix is available */
#define PRIOMERGE_DEFAULT PRIOMERGE_MAXIMUM



/*** DDD internal constants ***/

/* maximum number of procs allowed (limited by GID construction) */
#define MAX_PROCS   (1<<MAX_PROCBITS_IN_GID)


#define GID_INVALID  -1            /* invalid global id                     */
#define PRIO_INVALID (MAX_PRIO+1)  /* invalid priority                      */
#define PROC_INVALID (MAX_PROCS+1) /* invalid processor number              */
#define ERROR        -1            /* standard error indicator              */


/* types of virtual channels (for ppif interface) */
enum VChanType {
  VC_IDENT   = 15,               /* channels used for identification module     */
  VC_IFCOMM  = 16,               /* channels used for interface module          */
  VC_TOPO    = 17                /* channels used for xfer module (topology)    */
};


/* results of an prio-merge operation. see mgr/prio.c for more details. */
enum PrioMergeVals {
  PRIO_ERROR = -1,
  PRIO_UNKNOWN,
  PRIO_FIRST,
  PRIO_SECOND
};



/****************************************************************************/

/* string constants */
#define STR_NOMEM  "out of memory"


/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

/* macros for accessing COUPLING */
#define CPL_NEXT(cpl)   ((cpl)->_next)
#define CPL_PROC(cpl)   ((cpl)->_proc)

/* macros for accessing ELEM_DESC */
#define EDESC_REFTYPE(ed)          ((ed)->_reftype)
#define EDESC_SET_REFTYPE(ed,rt)   (ed)->_reftype=(rt)

/****************************************************************************/
/*                                                                          */
/* definitions previously hold in misc.h                                    */
/*                                                                          */
/****************************************************************************/

#ifndef MIN
#define MIN(x,y) (((x)<(y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) (((x)>(y)) ? (x) : (y))
#endif


/* round up to next alignment border */
#ifndef CEIL
#define CEIL(n) ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))
#endif

/* round down to next alignment border */
#ifndef FLOOR
#define FLOOR(n) ((n)&ALIGNMASK)
#endif

#define YES     1
#define ON      1

#define NO      0
#define OFF     0



/****************************************************************************/
/*                                                                          */
/* macros                                                                   */
/*                                                                          */
/****************************************************************************/


/* internal access of DDD_HEADER members */
#define ACCESS_HDR(o,c)   ((o)->c)

/* type of object */
#define OBJ_TYPE(o)     ACCESS_HDR(o,typ)

/* priority of object */
#define OBJ_PRIO(o)     ACCESS_HDR(o,prio)

/* attr of object */
#define OBJ_ATTR(o)     ACCESS_HDR(o,attr)

/* global id of object */
#define OBJ_GID(o)      ACCESS_HDR(o,gid)
#define OBJ_GID_FMT     DDD_GID_FMT

/* get index into global object table */
#define OBJ_INDEX(o)    ACCESS_HDR(o,myIndex)

/* internal flags of object */
#define OBJ_FLAGS(o)    ACCESS_HDR(o,flags)


/****************************************************************************/

/* usage of flags in DDD_HEADER */
#define MASK_OBJ_PRUNED 0x00000001
#define OBJ_PRUNED(c) (((int)(OBJ_FLAGS(c)))&MASK_OBJ_PRUNED)
#define SET_OBJ_PRUNED(c,d) (OBJ_FLAGS(c)) = ((OBJ_FLAGS(c))&(~MASK_OBJ_PRUNED))|((d)&MASK_OBJ_PRUNED)

#define MASK_OBJ_RESENT  0x00000002
#define SHIFT_OBJ_RESENT 1
#define OBJ_RESENT(c) ((((int)(OBJ_FLAGS(c)))&MASK_OBJ_RESENT)>>SHIFT_OBJ_RESENT)
#define SET_OBJ_RESENT(c,d) (OBJ_FLAGS(c)) = ((OBJ_FLAGS(c))&(~MASK_OBJ_RESENT))|(((d)<<SHIFT_OBJ_RESENT)&MASK_OBJ_RESENT)


/* usage of flags in COUPLING */
/* usage of 0x03 while interface-building, temporarily */
#define MASKCPLDIR 0x00000003
#define CPLDIR(c) (((int)((c)->_flags))&MASKCPLDIR)
#define SETCPLDIR(c,d) ((c)->_flags) = (((c)->_flags)&(~MASKCPLDIR))|((d)&MASKCPLDIR)

/* usage of 0x10 for remembering the memory origin for the COUPLING struct */
#define MASKCPLMEM 0x00000010
#define CPLMEM_EXTERNAL  0x00
#define CPLMEM_FREELIST  0x10
#define CPLMEM(c) (((int)((c)->_flags))&MASKCPLMEM)
#define SETCPLMEM_EXTERNAL(c) ((c)->_flags) = (((c)->_flags)&(~MASKCPLMEM))|(CPLMEM_EXTERNAL)
#define SETCPLMEM_FREELIST(c) ((c)->_flags) = (((c)->_flags)&(~MASKCPLMEM))|(CPLMEM_FREELIST)



/* convert DDD_OBJ to DDD_HDR and vice versa */

#define OBJ2HDR(obj,desc)  ((DDD_HDR)(((char *)obj)+((desc)->offsetHeader)))
#define HDR2OBJ(hdr,desc)  ((DDD_OBJ)(((char *)hdr)-((desc)->offsetHeader)))
#define OBJ_OBJ(context, hdr) ((DDD_OBJ)(((char *)hdr)-            \
                                      (context.typeDefs()[OBJ_TYPE(hdr)].offsetHeader)))


/****************************************************************************/

/*
        macros for access of coupling tables
 */

/* get boolean: does object have couplings? */
#define ObjHasCpl(context, o)      (OBJ_INDEX(o) < context.couplingContext().nCpls)

/* get #couplings per object */
#define ObjNCpl(context, o)        (ObjHasCpl(context, o) ? context.couplingContext().nCplTable[OBJ_INDEX(o)] : 0)
#define IdxNCpl(context, i)        (context.couplingContext().nCplTable[i])

/* get pointer to object's coupling list */
#define ObjCplList(context, o)     (ObjHasCpl(context, o) ? context.couplingContext().cplTable[OBJ_INDEX(o)] : nullptr)
#define IdxCplList(context, i)     (context.couplingContext().cplTable[i])


/* DDD_HDR may be invalid */
#define MarkHdrInvalid(hdr)    OBJ_INDEX(hdr)=(INT_MAX-1)
#define IsHdrInvalid(hdr)      OBJ_INDEX(hdr)==(INT_MAX-1)

#ifndef WithFullObjectTable
#define MarkHdrLocal(hdr)      OBJ_INDEX(hdr)=(INT_MAX)
#define IsHdrLocal(hdr)        OBJ_INDEX(hdr)==(INT_MAX)
#endif

/****************************************************************************/

/** get default VChan to processor p */
inline
const VChannelPtr VCHAN_TO(const DDD::DDDContext& context, DDD_PROC p)
{
  return context.topoContext().theTopology[p];
}


/****************************************************************************/

/* types for StdIf-communication functions (see if/ifstd.ct) */
typedef int (*ExecProcHdrPtr)(DDD::DDDContext& context, DDD_HDR);
typedef int (*ExecProcHdrXPtr)(DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_PRIO);
typedef int (*ComProcHdrPtr)(DDD::DDDContext& context, DDD_HDR, void *);
typedef int (*ComProcHdrXPtr)(DDD::DDDContext& context, DDD_HDR, void *, DDD_PROC, DDD_PRIO);


/****************************************************************************/
/*                                                                          */
/* memory management                                                        */
/*                                                                          */
/****************************************************************************/


#if defined(CheckPMEM) || defined(CheckIFMEM) || defined(CheckCplMEM) || defined(CheckMsgMEM) || defined(CheckTmpMEM) || defined(CheckHeapMem)

static void *dummy_ptr;
static char *mem_ptr;

#ifndef SST
#define SST (CEIL(sizeof(size_t)))
#define GET_SSTVAL(adr)   *(size_t *)(((char *)adr)-SST)
#endif

#endif


/*** mapping memory allocation calls to memmgr_ calls ***/

#define AllocObj(s,t,p,a) memmgr_AllocOMEM((size_t)s,(int)t,(int)p,(int)a)


#ifdef CheckPMEM
#define AllocFix(s)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocPMEM(SST+(size_t)s)) != NULL ? \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL PFix adr=%08x size=%ld file=%s line=%d\n",             \
         dummy_ptr,s,__FILE__,__LINE__)
#else
#define AllocFix(s)       memmgr_AllocPMEM((size_t)s)
#endif


#ifdef CheckMsgMEM
#define AllocMsg(s)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocTMEM(SST+(size_t)s, TMEM_MSG)) \
               != NULL ?                                                            \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL TMsg adr=%08x size=%ld file=%s line=%d\n",             \
         dummy_ptr,s,__FILE__,__LINE__)
#else
#define AllocMsg(s)       memmgr_AllocTMEM((size_t)s, TMEM_MSG)
#endif


#ifdef CheckTmpMEM
#define AllocTmp(s)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocTMEM(SST+(size_t)s, TMEM_ANY)) \
               != NULL ?                                                            \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL TTmp adr=%08x size=%ld file=%s line=%d\n",             \
         dummy_ptr,s,__FILE__,__LINE__)

#define AllocTmpReq(s,r)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocTMEM(SST+(size_t)s, r))        \
               != NULL ?                                                            \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL TTmp adr=%08x size=%ld kind=%d file=%s line=%d\n",     \
         dummy_ptr,s,r,__FILE__,__LINE__)
#else
#define AllocTmp(s)       memmgr_AllocTMEM((size_t)s, TMEM_ANY)
#define AllocTmpReq(s,r)  memmgr_AllocTMEM((size_t)s, r)
#endif


#ifdef CheckCplMEM
#define AllocCpl(s)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocAMEM(SST+(size_t)s)) != NULL ? \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL ACpl adr=%08x size=%ld file=%s line=%d\n",             \
         dummy_ptr,s,__FILE__,__LINE__)
#else
#define AllocCpl(s)       memmgr_AllocAMEM((size_t)s)
#endif

#ifdef CheckIFMEM
#define AllocIF(s)  \
  (dummy_ptr = (mem_ptr=(char *)memmgr_AllocAMEM(SST+(size_t)s)) != NULL ? \
               mem_ptr+SST : NULL);                                     \
  if (mem_ptr!=NULL) GET_SSTVAL(dummy_ptr) = s;                            \
  printf("MALL AIF  adr=%08x size=%ld file=%s line=%d\n",             \
         dummy_ptr,s,__FILE__,__LINE__)
#else
#define AllocIF(s)        memmgr_AllocAMEM((size_t)s)
#endif



/*** mapping memory free calls to memmgr calls ***/

#define FreeObj(mem,s,t)  memmgr_FreeOMEM(mem,(size_t)s,(int)t)



#ifdef CheckPMEM
#define FreeFix(mem)      {               \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreePMEM(((char *)mem)-SST);  \
    printf("FREE PFix adr=%08x size=%ld file=%s line=%d\n",\
           mem,s,__FILE__,__LINE__); }
#else
#define FreeFix(mem)      memmgr_FreePMEM(mem)
#endif

#ifdef CheckMsgMEM
#define FreeMsg(mem,size)      {   \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreeTMEM(((char *)mem)-SST, TMEM_MSG);    \
    printf("FREE TMsg adr=%08x size=%ld file=%s line=%d\n",\
           mem,s,__FILE__,__LINE__); }
#else
#define FreeMsg(mem,size)      memmgr_FreeTMEM(mem, TMEM_MSG)
#endif


#ifdef CheckTmpMEM
#define FreeTmp(mem,size)      {                  \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreeTMEM(((char *)mem)-SST,TMEM_ANY);       \
    printf("FREE TTmp adr=%08x size=%ld file=%s line=%d\n",\
           mem,s,__FILE__,__LINE__); }
#define FreeTmpReq(mem,size,r)      {                  \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreeTMEM(((char *)mem)-SST,r);       \
    printf("FREE TTmp adr=%08x size=%ld kind=%d file=%s line=%d\n",\
           mem,s,r,__FILE__,__LINE__); }
#else
#define FreeTmp(mem,size)      memmgr_FreeTMEM(mem,TMEM_ANY)
#define FreeTmpReq(mem,size,r) memmgr_FreeTMEM(mem,r)
#endif


#ifdef CheckCplMEM
#define FreeCpl(mem)      {                   \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreeAMEM(((char *)mem)-SST);     \
    printf("FREE ACpl adr=%08x size=%ld file=%s line=%d\n",\
           mem,s,__FILE__,__LINE__); }
#else
#define FreeCpl(mem)      memmgr_FreeAMEM(mem)
#endif

#ifdef CheckIFMEM
#define FreeIF(mem)       { \
    size_t s=GET_SSTVAL(mem); \
    memmgr_FreeAMEM(((char *)mem)-SST);    \
    printf("FREE AIF  adr=%08x size=%ld file=%s line=%d\n",\
           mem,s,__FILE__,__LINE__); }
#else
#define FreeIF(mem)       memmgr_FreeAMEM(mem)
#endif



/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* ddd.c */
int DDD_GetOption(const DDD::DDDContext& context, DDD_OPTION);


/* typemgr.c */
void      ddd_TypeMgrInit(DDD::DDDContext& context);
void      ddd_TypeMgrExit(DDD::DDDContext& context);
int       ddd_TypeDefined (TYPE_DESC *);


/* objmgr.c */
void      ddd_ObjMgrInit(DDD::DDDContext& context);
void      ddd_ObjMgrExit(DDD::DDDContext& context);
void      ddd_EnsureObjTabSize(DDD::DDDContext& context, int);


/* cplmgr.c */
void      ddd_CplMgrInit(DDD::DDDContext& context);
void      ddd_CplMgrExit(DDD::DDDContext& context);
COUPLING *AddCoupling(DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_PRIO);
COUPLING *ModCoupling(DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_PRIO);
void      DelCoupling(DDD::DDDContext& context, DDD_HDR, DDD_PROC);
void      DisposeCouplingList(DDD::DDDContext& context, COUPLING *);
void      DDD_InfoCoupling(const DDD::DDDContext& context, DDD_HDR);


/* mgr/prio.c */
enum PrioMergeVals PriorityMerge (const TYPE_DESC*, DDD_PRIO, DDD_PRIO, DDD_PRIO *);


/* if/if.c */
void      ddd_IFInit(DDD::DDDContext& context);
void      ddd_IFExit(DDD::DDDContext& context);
void      IFAllFromScratch(DDD::DDDContext&);
void      DDD_InfoIFImpl(DDD::DDDContext& context, DDD_IF);
void      IFInvalidateShortcuts(DDD::DDDContext& context, DDD_TYPE);
int       DDD_CheckInterfaces(DDD::DDDContext& context);

/* if/ifcmds.c */
void   ddd_StdIFExchange   (DDD::DDDContext& context, size_t, ComProcHdrPtr,ComProcHdrPtr);
void   ddd_StdIFExecLocal  (DDD::DDDContext& context,         ExecProcHdrPtr);
void   ddd_StdIFExchangeX  (DDD::DDDContext& context, size_t, ComProcHdrXPtr,ComProcHdrXPtr);
void   ddd_StdIFExecLocalX (DDD::DDDContext& context,         ExecProcHdrXPtr);



/* xfer/xfer.c */
void      ddd_XferInit(DDD::DDDContext& context);
void      ddd_XferExit(DDD::DDDContext& context);
int       ddd_XferActive(const DDD::DDDContext& context);
void      ddd_XferRegisterDelete(DDD::DDDContext& context, DDD_HDR);


/* xfer/cmds.c */
void      DDD_XferPrioChange(DDD::DDDContext& context, DDD_HDR, DDD_PRIO);


/* prio/pcmds.c */
void      ddd_PrioInit(DDD::DDDContext& context);
void      ddd_PrioExit(DDD::DDDContext& context);
bool      ddd_PrioActive(const DDD::DDDContext& context);


/* join/join.c */
void      ddd_JoinInit(DDD::DDDContext& context);
void      ddd_JoinExit(DDD::DDDContext& context);
bool      ddd_JoinActive(const DDD::DDDContext& context);


/* ident/ident.c */
void      ddd_IdentInit(DDD::DDDContext& context);
void      ddd_IdentExit(DDD::DDDContext& context);


/* basic/cons.c */
void      ddd_ConsInit(DDD::DDDContext& context);
void      ddd_ConsExit(DDD::DDDContext& context);

END_UGDIM_NAMESPACE
namespace DDD {
/* basic/topo.c */
void      ddd_TopoInit(DDD::DDDContext& context);
void      ddd_TopoExit(DDD::DDDContext& context);
DDD_PROC* DDD_ProcArray(DDD::DDDContext& context);
RETCODE   DDD_GetChannels(DDD::DDDContext& context, int);
void      DDD_DisplayTopo(const DDD::DDDContext& context);
} /* namespace DDD */
START_UGDIM_NAMESPACE


/* mgr/objmgr.c */
void      DDD_HdrConstructorCopy(DDD::DDDContext& context, DDD_HDR, DDD_PRIO);
void      ObjCopyGlobalData (TYPE_DESC *, DDD_OBJ, DDD_OBJ, size_t);
std::vector<DDD_HDR> LocalObjectsList(const DDD::DDDContext& context);
std::vector<DDD_HDR> LocalCoupledObjectsList(const DDD::DDDContext& context);

END_UGDIM_NAMESPACE
namespace DDD {
/* basic/reduct.c */
int       ddd_GlobalSumInt(const DDD::DDDContext& context, int);
int       ddd_GlobalMaxInt(const DDD::DDDContext& context, int);
int       ddd_GlobalMinInt(const DDD::DDDContext& context, int);
} /* namespace DDD */
START_UGDIM_NAMESPACE


/* ctrl/stat.c */
void      ddd_StatInit (void);
void      ddd_StatExit (void);


END_UGDIM_NAMESPACE

#endif
