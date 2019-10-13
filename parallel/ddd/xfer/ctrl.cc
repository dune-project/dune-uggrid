// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ctrl.c                                                        */
/*                                                                          */
/* Purpose:   controls and displays messages, for debugging only            */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/02/16 kb  begin                                            */
/*            960718 kb  introduced lowcomm-layer (sets of messages)        */
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
#include <sstream>

#include <iomanip>
#include <iostream>

#include <parallel/ddd/dddi.h>
#include "xfer.h"

using namespace PPIF;

START_UGDIM_NAMESPACE

/* #define DebugAllPointers */


/****************************************************************************/
/*                                                                          */
/* definition of static variables                                           */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


#ifdef DebugAllPointers

static void XferPtr (DDD::DDDContext& context, LC_MSGHANDLE xm, const std::string& prefix, std::ostream& out)
{
  SYMTAB_ENTRY *theSymTab;
  OBJTAB_ENTRY *theObjTab;
  char         *theObjects;
  int i;
  int lenSymTab = (int) LC_GetTableLen(xm, ctx.symtab_id);
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);


  /* get table addresses inside message buffer */
  theSymTab = (SYMTAB_ENTRY *) LC_GetPtr(xm, ctx.symtab_id);
  theObjTab = (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);
  theObjects = (char *)        LC_GetPtr(xm, ctx.objmem_id);


  /* build symbol table */
  for(i=0; i<lenObjTab; i++)            /* for all objects in message */
  {
    DDD_HDR hdr   = (DDD_HDR)(theObjects+theObjTab[i].offset);
    const TYPE_DESC& desc = context.typeDefs()[OBJ_TYPE(hdr)];
    DDD_OBJ obj   = HDR2OBJ(hdr, &desc);
    const ELEM_DESC* theElem = desc.element;

    /* loop over all pointers inside of object with DDD_HEADER hdr */
    for(int e=0; e < desc.nElements; ++e, ++theElem)
    {
      if (theElem->type != EL_OBJPTR)
        continue;

      for(int l=0; l<theElem->size; l+=sizeof(void *))
      {
        /* ref points to a reference inside objmem */
        DDD_OBJ *ref = (DDD_OBJ *)(((char *)obj)+theElem->offset+l);

        /* reference had been replaced by SymTab-index */
        INT stIdx = ((int)*ref)-1;

        if (stIdx < 0)
          continue;

        /* get corresponding symtab entry */
        const SYMTAB_ENTRY *st = &(theSymTab[stIdx]);

        using std::dec;
        using std::hex;
        using std::setw;
        out << prefix << " 20       "
            << " obj=" << setw(3) << theObjTab[i].offset << " " << setw(3) << stIdx
            << hex
            << " st=" << setw(8) << st << " gid=" << setw(8) << st->gid
            << "(" << setw(x) << st->adr.hdr << "=="
            << setw(8) << st->adr.ref << ")\n"
            << dec;
      }
    }
  }
}
#endif


void XferDisplayMsg (DDD::DDDContext& context, const char *comment, LC_MSGHANDLE xm)
{
  using std::setw;

  std::ostream& out = std::cout;
  auto& ctx = context.xferContext();
  SYMTAB_ENTRY *theSymTab;
  OBJTAB_ENTRY *theObjTab;
  TENewCpl     *theNewCpl;
  TEOldCpl     *theOldCpl;
  char         *theObjects;
  int i, proc = LC_MsgGetProc(xm);
  int lenSymTab = (int) LC_GetTableLen(xm, ctx.symtab_id);
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);
  int lenNewCpl = (int) LC_GetTableLen(xm, ctx.newcpl_id);
  int lenOldCpl = (int) LC_GetTableLen(xm, ctx.oldcpl_id);

  std::ostringstream prefixStream;
  prefixStream << " " << setw(3) << context.me() << "-" << comment
               << "-" << setw(3) << proc << " ";
  const std::string& prefix = prefixStream.str();

  /* get table addresses inside message */
  theSymTab = (SYMTAB_ENTRY *)LC_GetPtr(xm, ctx.symtab_id);
  theObjTab = (OBJTAB_ENTRY *)LC_GetPtr(xm, ctx.objtab_id);
  theNewCpl = (TENewCpl *)    LC_GetPtr(xm, ctx.newcpl_id);
  theOldCpl = (TEOldCpl *)    LC_GetPtr(xm, ctx.oldcpl_id);
  theObjects= (char *)LC_GetPtr(xm, ctx.objmem_id);


  /* because of LC layer, this data can't be accessed anymore. KB 960718
          sprintf(cBuffer, "%s 00 MsgBuf=%08x\n", buf, xmdata);
          DDD_PrintDebug(cBuffer);
          sprintf(cBuffer, "%s 00 SymTab %04d\n", buf, theHeader->beginSymTab);
          DDD_PrintDebug(cBuffer);
          sprintf(cBuffer, "%s 01 ObjTab %04d\n", buf, theHeader->beginObjTab);
          DDD_PrintDebug(cBuffer);
          sprintf(cBuffer, "%s 02 CplTab %04d\n", buf, theHeader->beginCplTab);
          DDD_PrintDebug(cBuffer);
          sprintf(cBuffer, "%s 03 DelTab %04d\n", buf, theHeader->beginDelTab);
          DDD_PrintDebug(cBuffer);
          sprintf(cBuffer, "%s 04 ObjMem %04d\n", buf, theHeader->beginObjMem);
          DDD_PrintDebug(cBuffer);
   */


  out << prefix << " 05 ObjTab.size=" << setw(5) << lenObjTab << "\n";
  out << prefix << " 06 SymTab.size=" << setw(5) << lenSymTab << "\n";
  out << prefix << " 07 NewCpl.size=" << setw(5) << lenNewCpl << "\n";
  out << prefix << " 08 OldCpl.size=" << setw(5) << lenOldCpl << "\n";

  for(i=0; i<lenObjTab; i++)
  {
    DDD_OBJ obj = OTE_OBJ(context, theObjects, &(theObjTab[i]));

    out << prefix << " 10 objtab    " << setw(6) << (((char *)obj)-theObjects)
        << " typ=" << OTE_TYPE(theObjects,&(theObjTab[i]))
        << " gid=" << OTE_GID(theObjects,&(theObjTab[i]))
        << " hdr=" << theObjTab[i].hdr << " size=" << setw(5) << theObjTab[i].size
        << " add=" << setw(5) << theObjTab[i].addLen << "\n";
  }

  for(i=0; i<lenSymTab; i++)
    out << prefix << " 11 symtab " << setw(4) << i << " - " << theSymTab[i].gid
        << " (" << setw(8) << theSymTab[i].adr.hdr << "=="
        << theSymTab[i].adr.ref << ")\n";

  for(i=0; i<lenNewCpl; i++)
    out << prefix << "  12 newcpl " << setw(4) << i << " - "
        << NewCpl_GetGid(theNewCpl[i])
        << " " << setw(4) << NewCpl_GetDest(theNewCpl[i])
        << " " << setw(4) << NewCpl_GetPrio(theNewCpl[i]) << "\n";

  for(i=0; i<lenOldCpl; i++)
    out << prefix << " 13 oldcpl " << setw(4) << i << " - "
        << theOldCpl[i].gid
        << " " << setw(4) << theOldCpl[i].proc
        << " " << setw(4) << theOldCpl[i].prio << "\n";

#       ifdef DebugAllPointers
  XferPtr(context, xm, prefix, out);
#       endif
}

END_UGDIM_NAMESPACE
