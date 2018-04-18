// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      objmgr.c                                                      */
/*                                                                          */
/* Purpose:   creation and deletion of objects                              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/02/21 kb  begin                                            */
/*            95/11/03 kb  complete redesign of objmgr-interface, C++ style */
/*            95/11/15 kb  arbitrary offset for DDD_HEADER                  */
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
#include <cassert>

#include <algorithm>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include "dddi.h"

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

/*
   #define DebugCreation
   #define DebugDeletion
 */


/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/


#define MakeUnique(n)  (((n)<<MAX_PROCBITS_IN_GID)+me)
#define ProcFromId(n)  ((n)& ((1<<MAX_PROCBITS_IN_GID)-1))
#define CountFromId(n) (((n)-((n)& ((1<<MAX_PROCBITS_IN_GID)-1)))>>MAX_PROCBITS_IN_GID)




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/



static bool sort_ObjListGID (const DDD_HDR& a, const DDD_HDR& b)
{
  return OBJ_GID(a) < OBJ_GID(b);
}


std::vector<DDD_HDR> LocalObjectsList(const DDD::DDDContext& context)
{
  const int nObjs = context.nObjs();
  std::vector<DDD_HDR> locObjs(nObjs);

  const auto& objTable = context.objTable();
  std::copy(objTable.begin(), objTable.begin() + nObjs, locObjs.begin());
  std::sort(locObjs.begin(), locObjs.end(), sort_ObjListGID);

  return locObjs;
}


std::vector<DDD_HDR> LocalCoupledObjectsList(const DDD::DDDContext& context)
{
  const auto& nCpls = context.couplingContext().nCpls;
  std::vector<DDD_HDR> locObjs(nCpls);

  const auto& objTable = context.objTable();
  std::copy(objTable.begin(), objTable.begin() + nCpls, locObjs.begin());
  std::sort(locObjs.begin(), locObjs.end(), sort_ObjListGID);

  return locObjs;
}


/****************************************************************************/


void ddd_EnsureObjTabSize(DDD::DDDContext& context, int n)
{
  auto& objTable = context.objTable();

  /* if size is large enough, we are already finished. */
  if (objTable.size() >= n)
    return;

  objTable.resize(n);

  /* issue a warning in order to inform user */
  Dune::dwarn << "increased object table, now " << n << " entries\n";
}


/****************************************************************************/

/*
        Description of ObjMgr Interfaces

        Raw-Memory-Interface:    DDD_ObjNew, DDD_ObjDelete
        Constructor-Interface:   DDD_HdrConstructor, DDD_HdrDestructor
        Application-Interface:   DDD_Get, DDD_UnGet
 */


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_ObjNew                                                    */
/*                                                                          */
/****************************************************************************/

/* Purpose:   get raw memory for new DDD-object.                            */
/*                                                                          */
/* Input:     size:  memory size of object                                  */
/*            typ:   DDD_TYPE of object                                     */
/*            prio:  DDD_PRIO of object                                     */
/*            attr:  attribute of distributed object                        */
/*                                                                          */
/* Output:    pointer to raw memory                                         */
/*                                                                          */
/****************************************************************************/

/**
        Allocate raw memory for new \ddd{object}.
        This function dynamically creates raw memory for a new \ddd{object}.
        Therefore, the user-supplied memory manager function \memmgrfunk{AllocOMEM}
        is called to allocate the necessary memory. Although the caller must
        supply the object's priority and attribute, its header will not be
        initialized by \funk{ObjNew}; the parameters are used for smart
        memory allocation, only.

        The function \funk{ObjNew} and \funk{ObjDelete}, its corresponding
        deletion function, form the ObjManager's {\em raw memory interface}.

        DDD users who use the #C_FRONTEND# may prefer the more
        elaborate {\em application interface}, consisting of the functions
        \funk{ObjGet} and \funk{ObjUnGet}. DDD users who use the language
        C++ (object-oriented style) with #CPP_FRONTEND# will use the
        {\em raw memory interface} together with the
        {\em constructor/destructor interface} (\funk{HdrConstructor},
        \funk{HdrDestructor}, \funk{HdrConstructorMove})
        in order to integrate DDD into C++ style object management easily.

        For variable-sized \ddd{objects}, the parameter {\em aSize}
        may differ from the size specified during the corresponding
        \funk{TypeDefine}-call.

   @return pointer to free memory block for the \ddd{object}
   @param  aSize   memory size of the new object
   @param  aType   \ddd{type} of the new object
   @param  aPrio   \ddd{priority} of the new object
   @param  aAttr   \ddd{attribute} of the new object
 */


DDD_OBJ DDD_ObjNew (size_t aSize, DDD_TYPE aType,
                    DDD_PRIO aPrio, DDD_ATTR aAttr)
{
  DDD_OBJ obj;

  /* check input parameters */
  if (aPrio>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "priority must be less than " << MAX_PRIO);
  if (aType>=MAX_TYPEDESC)
    DUNE_THROW(Dune::Exception, "DDD-type must be less than " << MAX_TYPEDESC);

  /* get object memory */
  obj = (DDD_OBJ) AllocObj(aSize, aType, aPrio, aAttr);
  if (obj==NULL)
    throw std::bad_alloc();

#       ifdef DebugCreation
  Dune::dinfo
    << "DDD_ObjNew(aSize=" << aSize << ", type=" << aType << ", prio=" << aPrio
    << ", attr=" << aAttr << ") ADR=" << obj << "\n";
#       endif

  return(obj);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_ObjDelete                                                 */
/*                                                                          */
/* Purpose:   free raw memory from DDD-object                               */
/*                                                                          */
/* Input:     obj:   object header address                                  */
/*            size:  memory size of object                                  */
/*            typ:   DDD_TYPE of object                                     */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/


void DDD_ObjDelete (DDD_OBJ obj, size_t size, DDD_TYPE typ)
{
  FreeObj((void *)obj, size, typ);
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_HdrConstructor                                            */
/*                                                                          */
/****************************************************************************/

/**
        Initiate object's \ddd{header}.
        This function registers a \ddd{object} via constructing
        its DDD-header. Each \ddd{object} is given a unique {\em global ID},
        which is stored in the DDD-header together with the object's
        properties (type\_id, prio, attr) and additional data used by DDD.

        The function \funk{HdrConstructor} and its corresponding deletion
        function \funk{HdrDestructor} form the ObjManager's
        {\em constructor/destructor interface}. This interface will be
        useful for C++ users, together with the {\em raw memory interface}
        functions \funk{ObjNew} and \funk{ObjDelete}.
        DDD users who use the language C may prefer the more
        elaborate {\em application interface}, consisting of the functions
        \funk{ObjGet} and \funk{ObjUnGet}.

        Due to the construction of global IDs, an overflow error may occur
        after many calls to \funk{HdrConstructor}
        (\MaxUniqueGids\ times in \Version).

   @param aHdr   pointer to the \ddd{header} which should be constructed
   @param aType  \ddd{type} of \ddd{object}
   @param aPrio  \ddd{priority} of \ddd{object}
   @param aAttr  \ddd{attribute} of \ddd{object}
 */

void DDD_HdrConstructor (DDD::DDDContext& context,
                         DDD_HDR aHdr, DDD_TYPE aType,
                         DDD_PRIO aPrio, DDD_ATTR aAttr)
{
  auto& objTable = context.objTable();
  auto& ctx = context.objmgrContext();

  /* check input parameters */
  if (aPrio>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "priority must be less than " << MAX_PRIO);

        #ifdef WithFullObjectTable
/* in case of FullObjectTable, we register each header in the
   global ddd_ObjTable. */

/* check whether there are available objects */
if (context.nObjs() == objTable.size())
{
  /* TODO update docu */
  /* this is a fatal case. we cant register more objects here */
  /* TODO one could try to expand the global tables here. */
  DUNE_THROW(Dune::Exception, "no more objects in DDD_HdrConstructor");
}

/* insert into theObj array */
objTable[context.nObjs()] = aHdr;
OBJ_INDEX(aHdr) = context.nObjs();
context.nObjs(context.nObjs() + 1);
        #else
/* if we dont have WithFullObjectTable, pure local objects without
   copies on other processors aren't registered by DDD. Therefore,
   they don't have a valid OBJ_INDEX field. */
MarkHdrLocal(aHdr);
        #endif


/* init object header with defaults */
OBJ_TYPE(aHdr)  = aType;
OBJ_PRIO(aHdr)  = aPrio;
OBJ_ATTR(aHdr)  = aAttr;
OBJ_FLAGS(aHdr) = 0;

/* create unique GID */
OBJ_GID(aHdr)   = MakeUnique(ctx.theIdCount++);

/* check overflow of global id numbering */
if (MakeUnique(ctx.theIdCount) <= MakeUnique(ctx.theIdCount-1))
{
  /* TODO update docu */
  /* TODO one could try to renumber all objects here. */
  DUNE_THROW(Dune::Exception, "global ID overflow DDD_HdrConstructor");
}

#       ifdef DebugCreation
  Dune::dinfo
    << "DDD_HdrConstructor(adr=" << aHdr << ", type=" << aType
    << ", prio=" << aPrio << ", attr=" << aAttr << "), GID=" << OBJ_GID(aHdr)
    << "  INDEX=" << OBJ_INDEX(aHdr) << "\n";
#       endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_HdrDestructor                                             */
/*                                                                          */
/****************************************************************************/

/**
        Remove object's header from DDD management.
        This function removes an object from DDD-management
        via destructing its \ddd{header}.
        {\em Note:} The \ddd{object} will be destroyed, but its copies
        on remote processors will not be informed by \funk{HdrDestructor}.
        There are two consistent possibilities to delete \ddd{objects} which
        have copies on remote processors:

        \begin{itemize}
        \item In order to delete only this local object copy, use
        \funk{XferDelete} during a DDD Transfer-operation. This will
        inform the remote copies of the deletion.
        \item In order to delete a distributed object (\ie, all its
        object copies), use function \funk{ObjUnGet} (when using the
        {\em application interface})
        or a combination of functions
        \funk{HdrDestructor} / \funk{ObjDelete} (when not using
        the {\em application interface}) for all copies.
        \end{itemize}

        The function \funk{HdrDestructor} and its corresponding creation
        function \funk{HdrConstructor} form the ObjManager's
        {\em constructor/destructor interface}. This interface will be
        useful for C++ users, together with the {\em raw memory interface}
        functions \funk{ObjNew} and \funk{ObjDelete}.
        DDD users who use the language C may prefer the more
        elaborate {\em application interface}, consisting of the functions
        \funk{ObjGet} and \funk{ObjUnGet}.

   @param hdr  the object's DDD Header
 */

void DDD_HdrDestructor(DDD::DDDContext& context, DDD_HDR hdr)
{
  auto& objTable = context.objTable();
  auto& nCpls = context.couplingContext().nCpls;
COUPLING   *cpl;
int objIndex, xfer_active = ddd_XferActive();

#       ifdef DebugDeletion
  Dune::dinfo
    << "DDD_HdrDestructor(adr=" << hdr << ", typ=" << OBJ_TYPE(hdr)
    << ", prio=" << OBJ_PRIO(hdr) << ", attr=" << OBJ_ATTR(hdr)
    << "), GID=" << OBJ_GID(hdr) << "  INDEX=" << OBJ_INDEX(hdr) << "\n";
#       endif


if (IsHdrInvalid(hdr))
{
  /* DDD_HDR is invalid, so destructor is useless */
  return;
}

/* formally, the object's GID should be returned here */


/* if currently in xfer, register deletion for other processors */
if (xfer_active)
  ddd_XferRegisterDelete(context, hdr);


objIndex = OBJ_INDEX(hdr);

if (objIndex < nCpls)
{
  /* this is an object with couplings */
  cpl = IdxCplList(context, objIndex);

  /* if not during xfer, deletion may be inconsistent */
  if (!xfer_active)
  {
    /* deletion is dangerous, distributed object might get
       inconsistent. */
    if (DDD_GetOption(context, OPT_WARNING_DESTRUCT_HDR)==OPT_ON)
      Dune::dwarn << "DDD_HdrDestructor: inconsistency by deleting gid="
                  << OBJ_GID(hdr) << "\n";
  }

  nCpls -= 1;
  context.nObjs(context.nObjs() - 1);

  /* fill slot of deleted obj with last cpl-obj */
  objTable[objIndex] = objTable[nCpls];
  IdxCplList(context, objIndex) = IdxCplList(context, nCpls);
  IdxNCpl(context, objIndex) = IdxNCpl(context, nCpls);
  OBJ_INDEX(objTable[objIndex]) = objIndex;

                #ifdef WithFullObjectTable
  /* fill slot of last cpl-obj with last obj */
  if (nCpls < context.nObjs())
  {
    objTable[nCpls] = objTable[context.nObjs()];
    OBJ_INDEX(objTable[nCpls]) = nCpls;
  }
                #else
  assert(nCpls == context.nObjs());
                #endif

  /* dispose all couplings */
  DisposeCouplingList(context, cpl);
}
else
{
                #ifdef WithFullObjectTable
  /* this is an object without couplings */
  /* deletion is not dangerous (no consistency problem) */
  context.nObjs(context.nObjs() - 1);

  /* fill slot of deleted obj with last obj */
  objTable[objIndex] = objTable[context.nObjs()];
  OBJ_INDEX(objTable[objIndex]) = objIndex;
                #endif
}

/* invalidate this DDD_HDR */
MarkHdrInvalid(hdr);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_ObjGet                                                    */
/*                                                                          */
/* Purpose:   get new DDD object for a given DDD_TYPE                       */
/*                                                                          */
/*            DDD_ObjGet allows different size for each object instance,    */
/*            differing from the size computed during DDD_TypeDefine        */
/*                                                                          */
/* Input:     typ: DDD_TYPE of object                                       */
/*                                                                          */
/* Output:    pointer to memory, which is raw except from the               */
/*              constructed DDD_HDR.                                        */
/*                                                                          */
/****************************************************************************/

DDD_OBJ DDD_ObjGet (DDD::DDDContext& context, size_t size, DDD_TYPE typ, DDD_PRIO prio, DDD_ATTR attr)
{
  DDD_OBJ obj;
  const TYPE_DESC& desc = context.typeDefs()[typ];

  /* check input parameters */
  if (prio<0 || prio>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "priority must be less than " << MAX_PRIO);

  /* get raw memory */
  obj = (DDD_OBJ) DDD_ObjNew(size, typ, prio, attr);
  if (obj==NULL)
    throw std::bad_alloc();

  if ((desc.size != size) && (DDD_GetOption(context, OPT_WARNING_VARSIZE_OBJ)==OPT_ON))
  {
    DDD_PrintError('W', 2200,
                   "object size differs from declared size in DDD_ObjGet");
  }

  if ((desc.size > size) && (DDD_GetOption(context, OPT_WARNING_SMALLSIZE)==OPT_ON))
  {
    DDD_PrintError('W', 2201,
                   "object size smaller than declared size in DDD_ObjGet");
  }


  /* call DDD_HdrConstructor */
  DDD_HdrConstructor(context, OBJ2HDR(obj, &desc), typ, prio, attr);

  return(obj);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_ObjUnGet                                                  */
/*                                                                          */
/* Purpose:   remove object from DDD management and free its memory         */
/*                                                                          */
/* Input:     obj: object header address                                    */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_ObjUnGet (DDD::DDDContext& context, DDD_HDR hdr, size_t size)

{
  DDD_TYPE typ = OBJ_TYPE(hdr);
  const TYPE_DESC& desc = context.typeDefs()[typ];
  DDD_OBJ obj = HDR2OBJ(hdr, &desc);

  if ((desc.size != size) && (DDD_GetOption(context, OPT_WARNING_VARSIZE_OBJ)==OPT_ON))
  {
    DDD_PrintError('W', 2299,
                   "object size differs from declared size in DDD_ObjUnGet");
  }

  /* call DDD_HDR-destructor */
  DDD_HdrDestructor(context, hdr);

  /* free raw memory */
  DDD_ObjDelete(obj, size, typ);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_HdrConstructorCopy                                        */
/*                                                                          */
/* Purpose:   create DDD_HDR copy from message original                     */
/*                                                                          */
/* Input:     newhdr: new DDD_HDR                                           */
/*            prio:   DDD_PRIO of new copy                                  */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_HdrConstructorCopy (DDD::DDDContext& context, DDD_HDR newhdr, DDD_PRIO prio)
{
  auto& objTable = context.objTable();

  /* check input parameters */
  if (prio>=MAX_PRIO)
    DUNE_THROW(Dune::Exception, "priority must be less than " << MAX_PRIO);

        #ifdef WithFullObjectTable
  /* check whether there are available objects */
  if (context.nObjs() == context.objTable().size())
  {
    /* TODO update docu */
    /* this is a fatal case. we cant register more objects here */
    DDD_PrintError('F', 2220, "no more objects in DDD_HdrConstructorCopy");
    /* TODO one could try to expand the global tables here. */
  }

  /* insert into theObj array */
  objTable[context.nObjs()] = newhdr;
  OBJ_INDEX(newhdr) = context.nObjs();
  context.nObjs(context.nObjs() + 1);
        #else
  MarkHdrLocal(newhdr);
  assert(context.nObjs() == context.couplingContext().nCpls);
        #endif

  /* init LDATA components. GDATA components will be copied elsewhere */
  OBJ_PRIO(newhdr)  = prio;


#       ifdef DebugCreation
  Dune::dinfo
    << "DDD_HdrConstructorCopy(adr=" << newhdr << ", prio=" << prio
    << "), GID= " << OBJ_GID(newhdr) << "  INDEX=" << OBJ_INDEX(newhdr)
    << "  ATTR=" << OBJ_ATTR(newhdr) << "\n";
#       endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_HdrConstructorMove                                        */
/*                                                                          */
/* Purpose:   create DDD_HDR copy inside local memory,                      */
/*            simultaneously destruct original DDD_HDR                      */
/*                                                                          */
/* Input:     newhdr: new DDD_HDR                                           */
/*            oldhdr: old DDD_HDR                                           */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_HdrConstructorMove (DDD::DDDContext& context, DDD_HDR newhdr, DDD_HDR oldhdr)
{
  int objIndex = OBJ_INDEX(oldhdr);
  const auto& nCpls = context.couplingContext().nCpls;


  /* copy all components */
  OBJ_INDEX(newhdr) = OBJ_INDEX(oldhdr);
  OBJ_TYPE(newhdr)  = OBJ_TYPE(oldhdr);
  OBJ_PRIO(newhdr)  = OBJ_PRIO(oldhdr);
  OBJ_ATTR(newhdr)  = OBJ_ATTR(oldhdr);
  OBJ_FLAGS(newhdr) = OBJ_FLAGS(oldhdr);
  OBJ_GID(newhdr)   = OBJ_GID(oldhdr);


  /* change all references from DDD to oldhdr */

  /* change entry of theObj array */
  auto& objTable = context.objTable();
        #ifdef WithFullObjectTable
  objTable[objIndex] = newhdr;
        #else
  if (objIndex < nCpls)
    objTable[objIndex] = newhdr;
        #endif

  /* change pointers from couplings to object */
  if (objIndex < nCpls)
  {
    COUPLING *cpl = IdxCplList(context, objIndex);

    for(; cpl!=NULL; cpl=CPL_NEXT(cpl)) {
      cpl->obj = newhdr;
    }

    /* invalidate update obj-shortcut tables from IF module */
    IFInvalidateShortcuts(context, OBJ_TYPE(newhdr));
  }

  /* invalidate old DDD_HDR */
  MarkHdrInvalid(oldhdr);
}



/****************************************************************************/
/*                                                                          */
/* Function:  ObjCopyGlobalData                                             */
/*                                                                          */
/* Purpose:   copy DDD-object from message to memory. elements marked       */
/*            EL_LDATA (local data) will not be copied. this is done        */
/*            in an efficient way by using the DDD_TYPEs copy-mask (which   */
/*            has been set up during StructRegister()).                     */
/*                                                                          */
/*            CopyByMask() is a support function doing the actual work.     */
/*            This function could be more efficient by copying more than    */
/*            one byte at a time.                                           */
/*                                                                          */
/* Input:     target: DDD_OBJ address of target memory                      */
/*            source: DDD_OBJ address of source object                      */
/*            size:   size of object in bytes (might be != desc->size)      */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/


static void CopyByMask (TYPE_DESC *desc, DDD_OBJ target, DDD_OBJ source)
{
  unsigned char *s=(unsigned char *)source, *t=(unsigned char *)target;
  int i;

#       ifdef DebugCreation
  Dune::dinfo << "CopyByMask(" << desc->name << ", size=" << desc->size
              << ", to=" << target << ", from=" << source << ")\n";
#       endif

  unsigned char* maskp = desc->cmask.get();

  /* copy all bits set in cmask from source to target */
  for(i=0; i<desc->size; i++)
  {
    unsigned char negmask = *maskp^0xff;
    *t = (*s & *maskp) | (*t & negmask);

    t++; s++; maskp++;              /* Paragon didn't manage postfix increment */
  }
}


void ObjCopyGlobalData (TYPE_DESC *desc,
                        DDD_OBJ target, DDD_OBJ source, size_t size)
{
  /*
          normally size will be equal to desc->size (for fixed-sized objects).
          for variable-sized objects size depends on what the sender put into
          the message
   */

  CopyByMask(desc, target, source);

  /* copy remainder as EL_GDATA */
  if (size > desc->size)
  {
    memcpy(((char *)target)+desc->size,
           ((char *)source)+desc->size, size-desc->size);
  }

#       ifdef DebugCreation
  const auto& hdr = OBJ2HDR(target, desc);
  Dune::dinfo
    << "ObjCopyGlobalData(" << hdr << " <- " << source << ", size=" << size
    << "), TYP=" << OBJ_TYPE(hdr) << "  GID=" << OBJ_GID(hdr)
    << "  INDEX=" << OBJ_INDEX(hdr) << "\n";
#       endif
}



/****************************************************************************/

DDD_HDR DDD_SearchHdr(DDD::DDDContext& context, DDD_GID gid)
{
  auto& objTable = context.objTable();
  const int nObjs = context.nObjs();
int i;

i=0;
while (i < nObjs && OBJ_GID(objTable[i])!=gid)
  i++;

if (i < nObjs)
{
  return(objTable[i]);
}
else
  return(NULL);
}


/****************************************************************************/


void ddd_ObjMgrInit(DDD::DDDContext& context)
{
  auto& ctx = context.objmgrContext();

  /* sanity check: does the DDD_PROC type have enough bits? */
  if (sizeof(DDD_PROC)*8 < MAX_PROCBITS_IN_GID)
    DDD_PrintError('F', 666, "DDD_PROC isn't large enough for MAX_PROCBITS_IN_GID bits");

  ctx.theIdCount = 1;        /* start with 1, for debugging reasons */

  /* allocate first (smallest) object table */
  context.objTable().resize(MAX_OBJ_START);
}


void ddd_ObjMgrExit(DDD::DDDContext& context)
{
  context.objTable().clear();
}


/****************************************************************************/

END_UGDIM_NAMESPACE
