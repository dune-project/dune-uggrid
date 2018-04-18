// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ddd.h                                                         */
/*                                                                          */
/* Purpose:   header file for ddd module                                    */
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
/*            94/06/27 kb  revision for usage in C++ context                */
/*            94/08/22 kb  major revision, corresp. to Ref.Man. V1.0        */
/*            94/09/13 kb  added DDD_Status                                 */
/*            95/01/13 kb  added range functionality                        */
/*            95/01/16 kb  added Statistics features                        */
/*            95/01/18 kb  moved Statistics to dddaddon.h                   */
/*            95/02/06 kb  added extended Ifs                               */
/*            95/03/08 kb  added UserData features                          */
/*            95/03/21 kb  added variable-sized Objects                     */
/*            95/05/22 kb  added var-sized AddData features                 */
/*            95/11/04 kb  complete redesign of ObjMgr and Registering      */
/*            95/11/06 kb  changed parameters of DDD_Init()                 */
/*            95/11/15 kb  ddd_hdr with arbitrary offset (started)          */
/*            96/01/08 kb  renamed range to attr                            */
/*            96/05/12 kb  xfer-unpack rewritten                            */
/*            96/06/05 kb  changed handler management                       */
/*            96/09/06 kb  xfer-module completely rewritten                 */
/*            96/11/28 kb  merged F_FRONTEND functionality from code branch */
/*            97/02/10 kb  started with CPP_FRONTEND implementation         */
/*            98/01/28 kb  new ddd-environment: Join.                       */
/*            98/05/14 kb  redesigned memory handling.                      */
/*            98/07/20 kb  new ddd-environment: Prio.                       */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


#ifndef __DDD__
#define __DDD__

/* for size_t */
#include <cstddef>
#include <cinttypes>

#include "namespace.h"

#include <dune/uggrid/parallel/ddd/dddtypes.hh>
#include <dune/uggrid/parallel/ddd/dddtypes_impl.hh>

START_UGDIM_NAMESPACE

using namespace DDD;

#define DDD_VERSION    "1.9"

/* F77SYM(lsym,usym) macro is defined in compiler.h. 961127 KB */

/****************************************************************************/
/*                                                                          */
/* compile time constants defining static data size (i.e. arrays)           */
/* other constants                                                          */
/*                                                                          */
/****************************************************************************/



/* return types for DDD functions */
enum DDD_RET {
  DDD_RET_OK            = 0,            /* function was executed ok             */
  DDD_RET_ERROR_UNKNOWN = 1,            /* unknown error condition              */
  DDD_RET_ERROR_NOMEM   = 2             /* function aborted due to mem shortage */
};


/* types of elements for StructRegister */
/* (use negative values for combination with positive DDD_TYPEs) */
enum DDD_ELEM_TYPE {
  EL_DDDHDR   =  0,                     /* element type: DDD header             */
  EL_GDATA    = -1,                     /* element type: global data            */
  EL_LDATA    = -2,                     /* element type: local data             */
  EL_GBITS    = -3,                     /* element type: bitwise, 1=global      */
  EL_DATAPTR  = -4,                     /* element type: data pointer           */
  EL_OBJPTR   = -5,                     /* element type: object pointer         */
  EL_CONTINUE = -6,                     /* continued element definition list    */
  EL_END      = -7                      /* end of element definition list       */
};






/* NOTE: changes must be also done in fddd.f */
enum OptConsts {
  OPT_OFF = 0,
  OPT_ON
};

enum OptConstIdent {
  IDMODE_LISTS = 1,         /* ordering of each identify-tupel is relevant      */
  IDMODE_SETS               /* ordering of each identify-tupel is not sensitive */
};

enum OptConstXfer {
  XFER_SHOW_NONE     = 0x0000,        /* show no statistical infos              */
  XFER_SHOW_OBSOLETE = 0x0001,        /* show #obsolete xfer-commands           */
  XFER_SHOW_MEMUSAGE = 0x0002,        /* show sizes of message buffers          */
  XFER_SHOW_MSGSALL  = 0x0004         /* show message contents by LowComm stats */
};

enum OptConstJoin {
  JOIN_SHOW_NONE     = 0x0000,        /* show no statistical infos              */
  JOIN_SHOW_OBSOLETE = 0x0001,        /* show #obsolete join-commands           */
  JOIN_SHOW_MEMUSAGE = 0x0002,        /* show sizes of message buffers          */
  JOIN_SHOW_MSGSALL  = 0x0004         /* show message contents by LowComm stats */
};



/* direction of interface communication (DDD_IFOneway) */
/* NOTE: changes must be also done in fddd.f */
enum DDD_IF_DIR {
  IF_FORWARD  = 1,                     /* communicate from A to B               */
  IF_BACKWARD = 2                      /* communicate from B to A               */
};


/* ID of (predefined) standard interface */
enum IFConstants {
  STD_INTERFACE = 0
};


/* DDD_TYPE DDD_USER_DATA: send stream of bytes with XferAddData */
enum XferConstants {
  /* DDD_TYPE DDD_USER_DATA: send stream of bytes with XferAddData */
  /* application may add small integers in order to get more
     stream-of-byte channels, up to DDD_USER_DATA_MAX */
  DDD_USER_DATA     = 0x4000,
  DDD_USER_DATA_MAX = 0x4fff,


  /* additional parameter for user defined handlers in xfer */

  /* object has been rejected due to RULE C3 */
  XFER_REJECT  = 0x9000,

  /* object has been upgraded due to RULE C3 */
  XFER_UPGRADE,

  /* object has been downgraded due to PruneDel */
  XFER_DOWNGRADE,

  /* object is totally new */
  XFER_NEW,


  /* return value for DDD_XferIsPrunedDelete */
  XFER_PRUNED_TRUE = 0x9100,
  XFER_PRUNED_FALSE,
  XFER_PRUNED_ERROR,


  /* return value for DDD_XferObjIsResent */
  XFER_RESENT_TRUE = 0x9200,
  XFER_RESENT_FALSE,
  XFER_RESENT_ERROR
};


/* several default modes for priority handling */
enum PrioMatrixDefaults {
  PRIOMERGE_MAXIMUM = 0,
  PRIOMERGE_MINIMUM
};



/* constants for management of temporary memory allocation/deletion */
enum TMemRequests {
  TMEM_ANY     = 0x0000,
  TMEM_MSG,
  TMEM_OBJLIST,
  TMEM_CPL,

  TMEM_XFER    = 0x1000,
  TMEM_LOWCOMM,

  TMEM_JOIN    = 0x2000,

  TMEM_CONS    = 0x3000,

  TMEM_IDENT   = 0x4000
};



/****************************************************************************/
/*                                                                          */
/* data structures and new types                                            */
/*                                                                          */
/****************************************************************************/


/*
        new DDD types, used during access of DDD functional interface
 */
using DDD_GID = DDD::DDD_GID;
static_assert(
  std::is_same<DDD::DDD_GID, std::uint_least64_t>::value,
  "printf conversion specifier below expects DDD_GID to be uint_least64_t");
#define DDD_GID_FMT "%08" PRIxLEAST64
#define DDD_GID_TO_INT(A) (A)

using DDD_TYPE = DDD::DDD_TYPE;
using DDD_IF = DDD::DDD_IF;
using DDD_PROC = DDD::DDD_PROC;
using DDD_PRIO = DDD::DDD_PRIO;
using DDD_ATTR = DDD::DDD_ATTR;

using DDD_OBJ = DDD::DDD_OBJ;
using DDD_HEADER = DDD::DDD_HEADER;
using DDD_HDR = DDD::DDD_HDR;

/* NULL values for DDD types */
#define DDD_TYPE_NULL  0
#define DDD_PROC_NULL  0
#define DDD_PRIO_NULL  0
#define DDD_ATTR_NULL  0


/* special feature: hybrid reftype at TypeDefine-time */
#define DDD_TYPE_BY_HANDLER   127   /* must be > MAX_TYPEDESC */


/****************************************************************************/
/*                                                                          */
/* macros                                                                   */
/*                                                                          */
/****************************************************************************/


/*
        external access of elements in DDD_HEADER
 */
#define DDD_InfoPriority(ddd_hdr)    ((ddd_hdr)->prio)
#define DDD_InfoGlobalId(ddd_hdr)    ((ddd_hdr)->gid)
#define DDD_InfoAttr(ddd_hdr)       ((ddd_hdr)->attr)
#define DDD_InfoType(ddd_hdr)       ((ddd_hdr)->typ)


/****************************************************************************/
/*                                                                          */
/* declaration of DDD functional interface                                  */
/*                                                                          */
/****************************************************************************/

/*
        General DDD Module
 */
void     DDD_Init(DDD::DDDContext& context);
void     DDD_Exit(DDD::DDDContext& context);
void     DDD_Status(const DDD::DDDContext& context);
void     DDD_SetOption(DDD::DDDContext& context, DDD_OPTION, int);

/*
        Redirect line-oriented output, new in V1.2
 */
void     DDD_LineOutRegister (void (*func)(const char *s));


/*
        Type Manager Module
 */

DDD_TYPE DDD_TypeDeclare(DDD::DDDContext& context, const char *name);
int      DDD_InfoHdrOffset(const DDD::DDDContext& context, DDD_TYPE);
void     DDD_TypeDefine(DDD::DDDContext& context, DDD_TYPE, ...);
void     DDD_TypeDisplay(const DDD::DDDContext& context, DDD_TYPE);

int      DDD_InfoTypes(const DDD::DDDContext& context);


/* newstyle, type-secure setting of handlers */
void     DDD_SetHandlerLDATACONSTRUCTOR(DDD::DDDContext& context, DDD_TYPE, HandlerLDATACONSTRUCTOR);
void     DDD_SetHandlerDESTRUCTOR      (DDD::DDDContext& context, DDD_TYPE, HandlerDESTRUCTOR);
void     DDD_SetHandlerDELETE          (DDD::DDDContext& context, DDD_TYPE, HandlerDELETE);
void     DDD_SetHandlerUPDATE          (DDD::DDDContext& context, DDD_TYPE, HandlerUPDATE);
void     DDD_SetHandlerOBJMKCONS       (DDD::DDDContext& context, DDD_TYPE, HandlerOBJMKCONS);
void     DDD_SetHandlerSETPRIORITY     (DDD::DDDContext& context, DDD_TYPE, HandlerSETPRIORITY);
void     DDD_SetHandlerXFERCOPY        (DDD::DDDContext& context, DDD_TYPE, HandlerXFERCOPY);
void     DDD_SetHandlerXFERDELETE      (DDD::DDDContext& context, DDD_TYPE, HandlerXFERDELETE);
void     DDD_SetHandlerXFERGATHER      (DDD::DDDContext& context, DDD_TYPE, HandlerXFERGATHER);
void     DDD_SetHandlerXFERSCATTER     (DDD::DDDContext& context, DDD_TYPE, HandlerXFERSCATTER);
void     DDD_SetHandlerXFERGATHERX     (DDD::DDDContext& context, DDD_TYPE, HandlerXFERGATHERX);
void     DDD_SetHandlerXFERSCATTERX    (DDD::DDDContext& context, DDD_TYPE, HandlerXFERSCATTERX);
void     DDD_SetHandlerXFERCOPYMANIP   (DDD::DDDContext& context, DDD_TYPE, HandlerXFERCOPYMANIP);


void     DDD_PrioMergeDefault (DDD::DDDContext& context, DDD_TYPE, int);
void     DDD_PrioMergeDefine (DDD::DDDContext& context, DDD_TYPE, DDD_PRIO, DDD_PRIO, DDD_PRIO);
DDD_PRIO DDD_PrioMerge (DDD::DDDContext& context, DDD_TYPE, DDD_PRIO, DDD_PRIO);
void     DDD_PrioMergeDisplay (DDD::DDDContext& context, DDD_TYPE);



/*
        Object Properties
 */
void     DDD_PrioritySet(DDD::DDDContext& context, DDD_HDR, DDD_PRIO);
void     DDD_AttrSet (DDD_HDR, DDD_ATTR); /* this shouldn't be allowed */
int  *   DDD_InfoProcList (DDD::DDDContext& context, DDD_HDR);
DDD_PROC DDD_InfoProcPrio(const DDD::DDDContext& context, DDD_HDR, DDD_PRIO);
bool     DDD_InfoIsLocal(const DDD::DDDContext& context, DDD_HDR);
int      DDD_InfoNCopies(const DDD::DDDContext& context, DDD_HDR);
size_t   DDD_InfoCplMemory(const DDD::DDDContext& context);



/*
        Identification Environment Module
 */

void     DDD_IdentifyBegin(DDD::DDDContext& context);
DDD_RET  DDD_IdentifyEnd(DDD::DDDContext& context);
void     DDD_IdentifyNumber(DDD::DDDContext& context, DDD_HDR, DDD_PROC, int);
void     DDD_IdentifyString(DDD::DDDContext& context, DDD_HDR, DDD_PROC, char *);
void     DDD_IdentifyObject(DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_HDR);


/*
        Interface Module
 */

DDD_IF   DDD_IFDefine (DDD::DDDContext& context, int, DDD_TYPE O[], int, DDD_PRIO A[], int, DDD_PRIO B[]);
void     DDD_IFSetName (DDD::DDDContext& context, DDD_IF, const char *);

void     DDD_IFDisplayAll(const DDD::DDDContext& context);
void     DDD_IFDisplay(const DDD::DDDContext& context, DDD_IF);
size_t   DDD_IFInfoMemoryAll(const DDD::DDDContext& context);
size_t   DDD_IFInfoMemory(const DDD::DDDContext& context, DDD_IF);
void     DDD_IFRefreshAll(DDD::DDDContext& context);

void     DDD_IFExchange   (DDD::DDDContext& context, DDD_IF,                    size_t, ComProcPtr2,ComProcPtr2);
void     DDD_IFOneway     (DDD::DDDContext& context, DDD_IF,         DDD_IF_DIR,size_t, ComProcPtr2,ComProcPtr2);
void     DDD_IFExecLocal  (DDD::DDDContext& context, DDD_IF,                            ExecProcPtr);
void     DDD_IFAExchange  (DDD::DDDContext& context, DDD_IF,DDD_ATTR,           size_t, ComProcPtr2,ComProcPtr2);
void     DDD_IFAOneway    (DDD::DDDContext& context, DDD_IF,DDD_ATTR,DDD_IF_DIR,size_t, ComProcPtr2,ComProcPtr2);
void     DDD_IFAExecLocal (DDD::DDDContext& context, DDD_IF,DDD_ATTR,                   ExecProcPtr);
void     DDD_IFExchangeX  (DDD::DDDContext& context, DDD_IF,                    size_t, ComProcXPtr,ComProcXPtr);
void     DDD_IFOnewayX    (DDD::DDDContext& context, DDD_IF,         DDD_IF_DIR,size_t, ComProcXPtr,ComProcXPtr);
void     DDD_IFExecLocalX (DDD::DDDContext& context, DDD_IF,                            ExecProcXPtr);
void     DDD_IFAExchangeX (DDD::DDDContext& context, DDD_IF,DDD_ATTR,           size_t, ComProcXPtr,ComProcXPtr);
void     DDD_IFAOnewayX   (DDD::DDDContext& context, DDD_IF,DDD_ATTR,DDD_IF_DIR,size_t, ComProcXPtr,ComProcXPtr);
void     DDD_IFAExecLocalX(DDD::DDDContext& context, DDD_IF,DDD_ATTR,                   ExecProcXPtr);

/*
        Transfer Environment Module
 */
bool     DDD_XferWithAddData();
void     DDD_XferAddData(DDD::DDDContext& context, int, DDD_TYPE);
void     DDD_XferAddDataX(DDD::DDDContext& context, int, DDD_TYPE, size_t sizes[]);
int      DDD_XferIsPrunedDelete (DDD_HDR);
int      DDD_XferObjIsResent(const DDD::DDDContext& context, DDD_HDR);
void     DDD_XferBegin(DDD::DDDContext& context);
DDD_RET  DDD_XferEnd(DDD::DDDContext& context);
void     DDD_XferCopyObj (DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_PRIO);
void     DDD_XferCopyObjX (DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_PRIO, size_t);
void     DDD_XferDeleteObj (DDD::DDDContext& context, DDD_HDR);
void     DDD_XferPrioChange (DDD_HDR, DDD_PRIO);


/*
        Prio Environment Module
 */
void     DDD_PrioBegin(DDD::DDDContext& context);
DDD_RET  DDD_PrioEnd(DDD::DDDContext& context);
void     DDD_PrioChange(const DDD::DDDContext& context, DDD_HDR, DDD_PRIO);



/*
        Join Environment Module
 */
void     DDD_JoinBegin(DDD::DDDContext& context);
DDD_RET  DDD_JoinEnd(DDD::DDDContext& context);
void     DDD_JoinObj(DDD::DDDContext& context, DDD_HDR, DDD_PROC, DDD_GID);


/*
        Object Manager
 */

DDD_OBJ  DDD_ObjNew (size_t, DDD_TYPE, DDD_PRIO, DDD_ATTR);
void     DDD_ObjDelete (DDD_OBJ, size_t, DDD_TYPE);
void     DDD_HdrConstructor(DDD::DDDContext& context, DDD_HDR, DDD_TYPE, DDD_PRIO, DDD_ATTR);
void     DDD_HdrConstructorMove(DDD::DDDContext& context, DDD_HDR, DDD_HDR);
void     DDD_HdrDestructor(DDD::DDDContext& context, DDD_HDR);
DDD_OBJ  DDD_ObjGet (DDD::DDDContext& context, size_t, DDD_TYPE, DDD_PRIO, DDD_ATTR);
void     DDD_ObjUnGet (DDD::DDDContext& context, DDD_HDR, size_t);



/*
        Maintainance & Debugging
 */

int      DDD_ConsCheck(DDD::DDDContext& context); /* returns total #errors since V1.6.6 */
void     DDD_ListLocalObjects(const DDD::DDDContext& context);
DDD_HDR  DDD_SearchHdr(DDD::DDDContext&, DDD_GID);


/****************************************************************************/

END_UGDIM_NAMESPACE

#endif
