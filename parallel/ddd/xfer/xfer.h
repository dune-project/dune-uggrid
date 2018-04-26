// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      xfer.h                                                        */
/*                                                                          */
/* Purpose:   header file for xfer module                                   */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/*                                                                          */
/* History:   931122 kb  begin                                              */
/*            950404 kb  copied from dddi.h                                 */
/*            960718 kb  introduced lowcomm-layer (sets of messages)        */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __XFER_H__
#define __XFER_H__

#include <vector>

#define DebugXfer     10   /* 0 is all, 10 is off */
#define DebugPack     6    /* off: 6 */
#define DebugUnpack   5    /* off: 5 */
#define DebugCmdMsg   10   /* off: 10 */
#define DebugCplMsg   10   /* off: 10 */




#include "basic/lowcomm.h"
#include "basic/oopp.h"    /* for object-orientated style via preprocessor */
#include "sll.h"           /* TODO: remove this in future versions */

START_UGDIM_NAMESPACE

#define SUPPORT_RESENT_FLAG


/* define this to use as small types as possible. for debugging, try it
   without this define */
#define SMALL_TYPES_BY_CASTS


/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

/* some macros for customizing oopp */
#define _NEWPARAMS
#define _NEWPARAMS_OR_VOID    void

#define __INDENT(n)   { int i; for(i=0; i<n; i++) fputs("   ",fp);}
#define _PRINTPARAMS  , int indent, FILE *fp
#define _PRINTPARAMS_DEFAULT  ,0,stdout
#define _INDENT       __INDENT(indent)
#define _INDENT1      __INDENT(indent+1)
#define _INDENT2      __INDENT(indent+2)
#define _PRINTNEXT    , indent+1, fp
#define _PRINTSAME    , indent, fp

/* map memory allocation calls */
#define OO_Allocate  std::malloc
#define OO_Free      std::free


/* extra prefix for all xfer-related data structures and/or typedefs */
/*#define ClassPrefix*/


/* types of 'newness' of incoming object (for xferunpack.c) */
enum XferNewType {
  NOTNEW     = 0x00,             /* object is not new                           */
  PARTNEW    = 0x01,             /* object has been updated partially           */
  PRUNEDNEW  = 0x02,             /* object is new due to PruneDel (see cmdmsg.c)*/
  TOTALNEW   = 0x04,             /* object has been updated completely          */
  OTHERMSG   = 0x08,             /* object is taken from another message        */
  THISMSG    = 0x10              /* object is taken from this msg, temp setting */
};

END_UGDIM_NAMESPACE
namespace DDD {
namespace Xfer {

/* overall mode of transfer */
enum class XferMode : unsigned char
{
  XMODE_IDLE = 0,                /* waiting for next DDD_XferBegin() */
  XMODE_CMDS,                    /* after DDD_XferBegin(), before DDD_XferEnd() */
  XMODE_BUSY                     /* during DDD_XferEnd() */
};

} /* namespace Xfer */
} /* namespace DDD */
START_UGDIM_NAMESPACE

using DDD::Xfer::XferMode;

END_UGDIM_NAMESPACE

/****************************************************************************/

#ifdef SMALL_TYPES_BY_CASTS
#define DDD_PRIO_SMALL unsigned char
#define DDD_TYPE_SMALL unsigned char
#else
#define DDD_PRIO_SMALL DDD_PRIO
#define DDD_TYPE_SMALL DDD_TYPE
#endif


/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

namespace DDD {
namespace Xfer {

/****************************************************************************/
/* XFERADDDATA: description of additional data on sender side               */
/****************************************************************************/

struct XFERADDDATA {
  int addCnt;
  DDD_TYPE addTyp;
  int addLen;                    /* length of additional data                   */
  int addNPointers;
  int      *sizes;

  XFERADDDATA* next;
};


/****************************************************************************/
/* XICopyObj:                                                               */
/****************************************************************************/

struct XICopyObj
{
  DDD_HDR hdr;                    /* local obj for which a copy should be created */
  DDD_GID gid;                    /* gid of local object                          */
  DDD_PROC dest;                  /* proc involved with operation, me -> proc     */
  DDD_PRIO prio;                  /* priority for new object copy                 */
  size_t size;                    /* V1.2: needed for variable-sized objects      */

  int addLen;
  XFERADDDATA *add;               /* additional data items                   */

  int flags;
};

} /* namespace Xfer */
} /* namespace DDD */

START_UGDIM_NAMESPACE

using DDD::Xfer::XFERADDDATA;
using DDD::Xfer::XICopyObj;

#define ClassName XICopyObj
void Method(Print)   (DefThis _PRINTPARAMS);
int  Method(Compare) (ClassPtr, ClassPtr, const DDD::DDDContext* context);
#undef ClassName


/* define container class */
#ifndef SetOf  /* necessary for inline documentation only */
#define SetOf          XICopyObj
#define Set_SegmSize   256
#define Set_BTreeOrder 32
#endif
#include "basic/ooppcc.h"


/* usage of flags in XICopyObj */
/* usage of 0x01 while PruneXIDelCpl, temporarily */
#define MASK_CO_SELF 0x00000001
#define CO_SELF(c) (((int)((c)->flags))&MASK_CO_SELF)
#define SET_CO_SELF(c,d)     ((c)->flags) =   \
  ((((c)->flags)&(~MASK_CO_SELF)) | ((d)&MASK_CO_SELF))

/* usage of 0x02 for new_owner:
        did destination proc know the object's gid before? */
#define MASK_CO_NEWOWNER 0x00000002
#define CO_NEWOWNER(c) (((int)((c)->flags))&MASK_CO_NEWOWNER)
#define SET_CO_NEWOWNER(c)     ((c)->flags) = (((c)->flags) | MASK_CO_NEWOWNER)
#define CLEAR_CO_NEWOWNER(c)   ((c)->flags) = (((c)->flags)&(~MASK_CO_NEWOWNER))



/****************************************************************************/
/* XIDelObj:                                                                */
/****************************************************************************/

/* XIDelCmd represents a XferDeleteObj-cmd by the application program */
struct XIDelCmd
{
  SLL_INFO_WITH_COUNTER(XIDelCmd);
  DDD_HDR hdr;
};

/* include template */
#define T XIDelCmd
#define SLL_WithOrigOrder
#include "sll.ht"
#undef T


struct XIDelCpl;

/* XIDelObj represents an object-delete-action (cf. XferRegisterDelete()) */
struct XIDelObj
{
  SLL_INFO(XIDelObj);
  DDD_GID gid;                  /* gid of local object                          */

  /* hdr is explicitly not stored here, because object may be deleted
     when this XIDelObj-item is evaluated. */

  XIDelCpl *delcpls;        /* couplings of deleted object              */

};

/* include template */
#define T XIDelObj
#include "sll.ht"
#undef T



/****************************************************************************/
/* XISetPrio:                                                               */
/****************************************************************************/

#define ClassName XISetPrio
Class_Data_Begin
DDD_HDR hdr;                    /* local obj for which prio should be set       */
DDD_GID gid;                    /* gid of local object                          */
DDD_PRIO prio;                  /* new priority                                 */

int is_valid;                   /* invalid iff there's a DelObj for same gid    */
Class_Data_End
void Method(Print)   (DefThis _PRINTPARAMS);
int  Method(Compare) (ClassPtr, ClassPtr, const DDD::DDDContext*);

#undef ClassName


/* define container class */
#ifndef SetOf  /* necessary for inline documentation only */
#define SetOf          XISetPrio
#define Set_SegmSize   256
#define Set_BTreeOrder 32
#endif
#include "basic/ooppcc.h"


/****************************************************************************/
/* XINewCpl:                                                                */
/****************************************************************************/

struct TENewCpl
{
  DDD_GID _gid;                 /* obj-gid for which new copy will be created   */
  DDD_PROC _dest;               /* destination of new object copy               */

  DDD_PRIO_SMALL _prio;         /* priority of new object copy                  */
  DDD_TYPE_SMALL _type;         /* ddd-type of gid for PriorityMerge on receiver*/
};

#define NewCpl_GetGid(i)     ((i)._gid)
#define NewCpl_SetGid(i,d)   ((i)._gid = (d))
#define NewCpl_GetDest(i)    ((i)._dest)
#define NewCpl_SetDest(i,d)  ((i)._dest = (d))

#ifdef SMALL_TYPES_BY_CASTS
#define NewCpl_GetPrio(i)    ((DDD_PRIO)(i)._prio)
#define NewCpl_SetPrio(i,d)  ((i)._prio = (DDD_PRIO_SMALL)(d))
#define NewCpl_GetType(i)    ((DDD_TYPE)(i)._type)
#define NewCpl_SetType(i,d)  ((i)._type = (DDD_TYPE_SMALL)(d))
#else
#define NewCpl_GetPrio(i)    ((i)._prio)
#define NewCpl_SetPrio(i,d)  ((i)._prio = (d))
#define NewCpl_GetType(i)    ((i)._type)
#define NewCpl_SetType(i,d)  ((i)._type = (d))
#endif


struct XINewCpl
{
  SLL_INFO(XINewCpl);
  DDD_PROC to;                  /* receiver of this item                        */
  TENewCpl te;                  /* table entry (for message)                    */

};

/* include template */
#define T XINewCpl
#include "sll.ht"
#undef T



/****************************************************************************/
/* XIOldCpl:                                                                */
/****************************************************************************/

struct TEOldCpl
{
  DDD_GID gid;                  /* obj-gid of local copy                        */
  DDD_PROC proc;                /* owner of that local object                   */
  DDD_PRIO prio;                /* priority of that local object                */
};


struct XIOldCpl
{
  SLL_INFO(XIOldCpl);
  DDD_PROC to;                  /* receiver of this item                        */
  TEOldCpl te;                  /* table entry (for message)                    */
};

/* include template */
#define T XIOldCpl
#include "sll.ht"
#undef T


/****************************************************************************/
/* XIAddCpl:                                                                */
/****************************************************************************/

struct TEAddCpl
{
  DDD_GID gid;                  /* obj-gid of new local object                  */
  DDD_PROC proc;                /* owner of new object copy                     */
  DDD_PRIO prio;                /* priority of new local object                 */
};


struct XIAddCpl
{
  SLL_INFO(XIAddCpl);
  DDD_PROC to;                  /* receiver of this item                        */
  TEAddCpl te;                  /* table entry (for message)                    */

};

/* include template */
#define T XIAddCpl
#include "sll.ht"
#undef T



/****************************************************************************/
/* XIDelCpl:                                                                */
/****************************************************************************/

struct TEDelCpl
{
  DDD_GID gid;                  /* obj-gid of deleted local object              */
};


struct XIDelCpl
{
  SLL_INFO(XIDelCpl);
  DDD_PROC to;                  /* receiver of this item                        */
  TEDelCpl te;                  /* table entry (for message)                    */

  DDD_PRIO prio;                /* prio of deleted coupling                     */
  XIDelCpl* next;               /* linked list of XIDelCpls                     */
};

/* include template */
#define T XIDelCpl
#include "sll.ht"
#undef T




/****************************************************************************/
/* XIModCpl:                                                                */
/****************************************************************************/

struct TEModCpl
{
  DDD_GID gid;                  /* obj-gid of corresponding object              */
  DDD_PRIO prio;                /* new priority for this obj on sending proc    */
};


struct XIModCpl
{
  SLL_INFO(XIModCpl);
  DDD_PROC to;                  /* receiver of this item                        */
  TEModCpl te;                  /* table entry (for message)                    */

  DDD_TYPE typ;                 /* type of corresponding object                 */
};

/* include template */
#define T XIModCpl
#include "sll.ht"
#undef T


/****************************************************************************/
/* XFERMSG: complete description about message on sender side               */
/****************************************************************************/

struct XFERMSG
{
  DDD_PROC proc;                 /* receiver of message                         */
  size_t size;                   /* size of message data                        */

  XFERMSG* next;


  XICopyObjPtr *xferObjArray;
  int nObjItems;

  XINewCpl     **xferNewCpl;
  int nNewCpl;

  XIOldCpl     **xferOldCpl;
  int nOldCpl;

  int nPointers;
  int nObjects;


  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;
};




/****************************************************************************/
/* SYMTAB_ENTRY: single entry of symbol table inside message                */
/****************************************************************************/


struct SYMTAB_ENTRY
{
  DDD_GID gid;

  union {
    DDD_HDR hdr;                      /* used on receiver side only */
    DDD_OBJ    *ref;                  /* used on sender side only   */
  } adr;                          /* undefined during transfer  */
};



/****************************************************************************/
/* OBJTAB_ENTRY: single entry of object table inside message                */
/****************************************************************************/

struct OBJTAB_ENTRY
{

  int h_offset;                   /* header offset from beginObjMem */

  int addLen;                     /* length of additional data */
  size_t size;                    /* size of object, ==desc->len for
                                      fixed-sized objects */

  DDD_HDR hdr;              /* TODO this is probably not used on sender side */

  /* TODO: the following data is only used on receiver side */
  char is_new;
  char oldprio;
};



/* NOTE: the following macros require the DDD_HEADER being copied
         directly into the message! (with its LDATA!) */

#define OTE_HDR(objmem,ote)    ((DDD_HDR)(((char *)(objmem))+((ote)->h_offset)))
#define OTE_OBJ(context, objmem,ote)    OBJ_OBJ(context, OTE_HDR(objmem,ote))
#define OTE_GID(objmem,ote)    OBJ_GID(OTE_HDR(objmem,ote))
#define OTE_PRIO(objmem,ote)   OBJ_PRIO(OTE_HDR(objmem,ote))
#define OTE_TYPE(objmem,ote)   OBJ_TYPE(OTE_HDR(objmem,ote))
#define OTE_ATTR(objmem,ote)   OBJ_ATTR(OTE_HDR(objmem,ote))

#define OTE_GID_FMT OBJ_GID_FMT



/****************************************************************************/
/* DELTAB_ENTRY: single entry of deletion table inside message              */
/****************************************************************************/

struct DELTAB_ENTRY {
  DDD_GID gid;
  DDD_PROC proc;
};


/****************************************************************************/
/* XFER_PER_PROC: data needed on a per-proc basis during xfer               */
/****************************************************************************/

struct XFER_PER_PROC
{
  int dummy;        /* not used yet */
};


/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/


/* supp.c */
XFERADDDATA *NewXIAddData(DDD::DDDContext& context);
void FreeAllXIAddData(DDD::DDDContext& context);
int *AddDataAllocSizes(DDD::DDDContext& context, int);
/* and others, via template mechanism */



/* cplmsg.c */
void CommunicateCplMsgs (DDD::DDDContext& context,
                         XIDelCpl **, int,
                         XIModCpl **, int, XIAddCpl **, int, DDD_HDR *, int);
void CplMsgInit(DDD::DDDContext& context);
void CplMsgExit(DDD::DDDContext& context);


/* cmdmsg.c */
int  PruneXIDelCmd (DDD::DDDContext& context, XIDelCmd **, int, std::vector<XICopyObj*>&);
void CmdMsgInit(DDD::DDDContext& context);
void CmdMsgExit(DDD::DDDContext& context);


/* xfer.c, used only by cmds.c */
XICopyObj **CplClosureEstimate(DDD::DDDContext& context, const std::vector<XICopyObj*>&, int *);
int  PrepareObjMsgs(DDD::DDDContext& context,
                    std::vector<XICopyObj*>&, XINewCpl **, int,
                    XIOldCpl **, int, XFERMSG **, size_t *);
void ExecLocalXIDelCmd(DDD::DDDContext& context, XIDelCmd  **, int);
void ExecLocalXISetPrio(DDD::DDDContext& context, const std::vector<XISetPrio*>&, XIDelObj  **,int, XICopyObj **,int);
void ExecLocalXIDelObj(XIDelObj  **, int, XICopyObj **,int);
void PropagateCplInfos(XISetPrio **, int, XIDelObj  **, int,
                       TENewCpl *, int);
enum XferMode XferMode (const DDD::DDDContext& context);
int XferStepMode(DDD::DDDContext& context, enum XferMode);


/* pack.c,   used only by cmds.c */
RETCODE XferPackMsgs (DDD::DDDContext& context, XFERMSG *);


/* unpack.c, used only by cmds.c */
void XferUnpack (DDD::DDDContext& context, LC_MSGHANDLE *, int, const DDD_HDR *, int,
                 std::vector<XISetPrio*>&, XIDelObj  **, int,
                 const std::vector<XICopyObj*>&, XICopyObj **, int);


/* ctrl.c */
void XferDisplayMsg(DDD::DDDContext& context, const char *comment, LC_MSGHANDLE);

END_UGDIM_NAMESPACE

#endif
