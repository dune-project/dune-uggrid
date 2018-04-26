// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      pack.c                                                        */
/*                                                                          */
/* Purpose:   packs objects into messages                                   */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/01/31 kb  begin                                            */
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
#include <cinttypes>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include "dddi.h"
#include "xfer.h"

#include "ugtypes.h"
#include "namespace.h"

USING_UG_NAMESPACE

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



/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/





/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


static bool sort_SymTabEntries (const SYMTAB_ENTRY& a, const SYMTAB_ENTRY& b)
{
  return a.gid < b.gid;
}

static bool sort_MsgSize (const XFERMSG* a, const XFERMSG* b)
{
  /* sort with descending msg-size */
  return LC_GetBufferSize(a->msg_h) > LC_GetBufferSize(b->msg_h);
}


/****************************************************************************/
/*                                                                          */
/* Function:  BuildSymTab                                                   */
/*                                                                          */
/* Purpose:   compute message SymTab entries for one single ddd-object.     */
/*                                                                          */
/* Input:     desc: descriptor of object                                    */
/*            obj:  DDD_OBJ ptr to ddd-object (in local mem) or NULL        */
/*            copy: copy of ddd-object (inside message buffer)              */
/*            theSymTab: actual portion of message SymTab                   */
/*                                                                          */
/* Output:    number of new entries into SymTab                             */
/*                                                                          */
/****************************************************************************/

static int BuildSymTab (DDD::DDDContext& context,
                        TYPE_DESC *desc,
                        DDD_OBJ obj,
                        const char *copy,
                        SYMTAB_ENTRY *theSymTab)
{
  ELEM_DESC   *theElem;
  int e, actSym;

  /* reset local portion of SymTab */
  actSym = 0;

  /* prepare map of structure elements */
  theElem = desc->element;

  /* loop over all pointers inside of object obj */
  for(e=0; e<desc->nElements; e++, theElem++)
  {
    if (theElem->type==EL_OBJPTR)
    {
      TYPE_DESC *refdesc;
      int l;
      int rt_on_the_fly = (EDESC_REFTYPE(theElem)==DDD_TYPE_BY_HANDLER);

      /* determine reftype of this elem */
      if (! rt_on_the_fly)
      {
        /* we know the reftype of this element in advance */
        refdesc = &context.typeDefs()[EDESC_REFTYPE(theElem)];
      }
      /* else: determine reftype on the fly by calling handler */


      /* loop over single pointer array */
      for(l=0; l<theElem->size; l+=sizeof(void *))
      {
        /* get address of outside reference */
        DDD_OBJ *ref = (DDD_OBJ *)(copy+theElem->offset+l);


        /* create symbol table entry */
        if (*ref!=NULL)
        {
          DDD_HDR refhdr;

          if (rt_on_the_fly)
          {
            DDD_TYPE rt;

            /* determine reftype on the fly by calling handler */
            assert(obj!=NULL);                                       /* we need a real object here */

            rt = theElem->reftypeHandler(context, obj, *ref);
            if (rt>=MAX_TYPEDESC)
              DUNE_THROW(Dune::Exception,
                         "invalid referenced DDD_TYPE returned by handler");

            refdesc = &context.typeDefs()[rt];
          }

          /* get header of referenced object */
          refhdr = OBJ2HDR(*ref,refdesc);

          /* remember the GID of the referenced object */
          theSymTab[actSym].gid = OBJ_GID(refhdr);

          /* remember the address of the reference (in obj-copy) */
          theSymTab[actSym].adr.ref = ref;
          actSym++;
        }
      }
    }
  }

  /* return SymTab increment */
  return(actSym);
}



/****************************************************************************/
/*                                                                          */
/* Function:  GetDepData                                                    */
/*                                                                          */
/* Purpose:   fill object-dependent data into message. an appl. routine     */
/*            will be called to fill in the data actually. pointers are     */
/*            localized and the message SymTab is actualized.               */
/*                                                                          */
/* Input:     data: portion of message buffer reserved for dependent data   */
/*            desc: descriptor of object                                    */
/*            obj:  current ddd-object                                      */
/*            theSymTab: actual portion of message SymTab                   */
/*            xi:   single xferinfo for current ddd-object                  */
/*                                                                          */
/* Output:    number of new entries into SymTab                             */
/*                                                                          */
/****************************************************************************/

static int GetDepData (DDD::DDDContext& context,
                       char *data,
                       TYPE_DESC *desc,
                       DDD_OBJ obj,
                       SYMTAB_ENTRY *theSymTab,
                       XICopyObj *xi)
{
  XFERADDDATA  *xa;
  char         *chunk, *adr, **table1, *next_chunk;
  int chunks, i, actSym, *table2;


  if (xi->addLen==0) return(0);

  chunks = 0;
  actSym = 0;


  /* first entry will be number of dependency chunks */
  chunk = data + CEIL(sizeof(int));


  /* loop through whole dependency data descriptor */
  for(xa=xi->add; xa!=NULL; xa=xa->next)
  {
    /* first entries of chunk are addCnt and addTyp */
    ((int *)chunk)[0]      = xa->addCnt;
    ((DDD_TYPE *)chunk)[1] = xa->addTyp;

    if (xa->sizes==NULL)
    {
      chunk += CEIL(sizeof(int)+sizeof(DDD_TYPE));

      /* then all records should be gathered via handler */
      if (desc->handlerXFERGATHER)
      {
        desc->handlerXFERGATHER( context, obj,
                                 xa->addCnt, xa->addTyp, (void *)chunk);
      }

      if (xa->addTyp<DDD_USER_DATA || xa->addTyp>DDD_USER_DATA_MAX)
      {
        /* insert pointers into symtab */
        TYPE_DESC* descDep = &context.typeDefs()[xa->addTyp];
        for(i=0; i<xa->addCnt; i++)
        {
          actSym += BuildSymTab(context, descDep, NULL,
                                chunk, &(theSymTab[actSym]));
          chunk += CEIL(descDep->size);
        }
      }
      else
      {
        /* no regular type -> send byte stream with length addCnt */
        chunk += CEIL(xa->addCnt);
      }
    }
    else
    {
      /* var-sized AddData items */
      ((int *)chunk)[0] *= -1;
      chunk += CEIL(sizeof(int)+sizeof(DDD_TYPE));

      /* create pointer array inside message */
      table1 = (char **)chunk;
      chunk += CEIL(sizeof(int)*xa->addCnt);
      for(i=0, adr=chunk; i<xa->addCnt; i++)
      {
        table1[i] = adr;
        adr += CEIL(xa->sizes[i]);
      }
      next_chunk = adr;

      /* then all records should be gathered via handler */
      if (desc->handlerXFERGATHERX)
      {
        desc->handlerXFERGATHERX( context, obj,
                                  xa->addCnt, xa->addTyp, table1);
      }

      /* convert pointer table into offset table */
      table2 = (int *)table1;
      TYPE_DESC* descDep = &context.typeDefs()[xa->addTyp];
      adr = chunk;
      for(i=0; i<xa->addCnt; i++)
      {
        /* insert pointers into symtab */
        if (xa->addTyp<DDD_USER_DATA || xa->addTyp>DDD_USER_DATA_MAX)
        {
          actSym += BuildSymTab(context, descDep, NULL,
                                table1[i], &(theSymTab[actSym]));
        }

        table2[i] = (int)(table1[i]-adr);
      }

      chunk = next_chunk;
    }

    /* count chunks */
    chunks++;
  }


  /* remember number of chunks at the beginning of the deplist */
  ((int *)data)[0] = chunks;


  return(actSym);
}



/****************************************************************************/
/*                                                                          */
/* Function:  XferPackSingleMsgs                                            */
/*                                                                          */
/* Purpose:   build up one outgoing message completely, fill data into      */
/*            message buffer. objects and couplings will be packed, and     */
/*            several message tables will be constructed. pointers are      */
/*            localized inside the message. several applications handlers   */
/*            are called to fill in dependent data.                         */
/*                                                                          */
/* Input:     msg:  single message-send-info structure                      */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static void XferPackSingleMsg (DDD::DDDContext& context, XFERMSG *msg)
{
  SYMTAB_ENTRY *theSymTab;
  OBJTAB_ENTRY *theObjTab;
  TENewCpl     *theNewCpl;
  TEOldCpl     *theOldCpl;
  char         *theObjects, *currObj;
  int i, actSym, actNewCpl, actOldCpl, actObj;

  /* get table addresses inside message */
  theSymTab = (SYMTAB_ENTRY *)LC_GetPtr(msg->msg_h, xferGlobals.symtab_id);
  theObjTab = (OBJTAB_ENTRY *)LC_GetPtr(msg->msg_h, xferGlobals.objtab_id);
  theNewCpl = (TENewCpl *)    LC_GetPtr(msg->msg_h, xferGlobals.newcpl_id);
  theOldCpl = (TEOldCpl *)    LC_GetPtr(msg->msg_h, xferGlobals.oldcpl_id);
  theObjects= (char *)LC_GetPtr(msg->msg_h, xferGlobals.objmem_id);


  /* build several tables inside message */
  actSym = actNewCpl = actOldCpl = actObj = 0;
  currObj = theObjects;
  for(i=0; i<msg->nObjItems; i++)          /* for all XICopyObj-items */
  {
    XICopyObj *xi = msg->xferObjArray[i];
    DDD_HDR hdr   = xi->hdr;
    TYPE_DESC *desc = &context.typeDefs()[OBJ_TYPE(hdr)];
    DDD_OBJ obj   = HDR2OBJ(hdr,desc);
    /*COUPLING  *cpl;*/
    DDD_HDR copyhdr;

    /* build coupling table */
    /* skip cpl which describes object itself (receive proc) */
    /*
                    for(cpl=THECOUPLING(hdr); cpl!=NULL; cpl=cpl->next)
                    {
                            if (cpl->proc!=msg->proc)
                            {
                                    theNewCpl[actNewCpl].gid  = OBJ_GID(hdr);
                                    theNewCpl[actNewCpl].proc = cpl->proc;
                                    theNewCpl[actNewCpl].prio = cpl->prio;
                                    actNewCpl++;
                            }
                    }
     */

    /* one coupling for object itself (send proc) */
    /*
                    theNewCpl[actNewCpl].gid  = OBJ_GID(hdr);
                    theNewCpl[actNewCpl].proc = me;
                    theNewCpl[actNewCpl].prio = OBJ_PRIO(hdr);
                    actNewCpl++;
     */


    copyhdr = OBJ2HDR(currObj,desc);


    /* update object table */
    theObjTab[actObj].h_offset = (int)(((char *)copyhdr)-theObjects);
    theObjTab[actObj].hdr      = NULL;
    theObjTab[actObj].addLen   = xi->addLen;
    theObjTab[actObj].size     = xi->size;              /* needed for variable-sized objects */
    actObj++;



    /*
            copy object into message. in the following xi->size
            equals desc->len for fixed-size objects.
     */
    /*STAT_RESET3;*/
    /* NOTE: object memory is copied _completely_, i.e., also LDATA-
       components are copied into message and sent to destination.
       then, on the receiving processor the data is sorted out... */
    memcpy(currObj, obj, xi->size);

    /* insert priority into copy */
    OBJ_PRIO(copyhdr) = xi->prio;


    /* call application handler for direct manipulation */
    /* KB 941110:  moved from objmgr.c                  */
    /*
            Caution: this is a very, very dirty situation.
            HANDLER_XFERCOPYMANIP is able to manipulate the
            obj-copy inside the message. this handler should
            be removed in future DDD versions.
     */
    if (desc->handlerXFERCOPYMANIP)
    {
      /*
              NOTE: OBJ_TYPE could change during the
              execution of HANDLER_XFERCOPYMANIP. however,
              the position of DDD_HEADER inside the object
              should not change. therefore, we can remember
              the offsetHeader here and use it afterwards
              to adjust the desc.
       */
      int offset = desc->offsetHeader;

      /* now call handler */
      desc->handlerXFERCOPYMANIP(context, currObj);

      /* adjust new description according to new type */
      desc = &context.typeDefs()[OBJ_TYPE((DDD_HDR)(currObj+offset))];
    }

    /* build symbol table portion from object copy */
    actSym += BuildSymTab(context, desc, obj, (char *)currObj, &(theSymTab[actSym]));


    /* advance to next free object slot in message, c.f. alignment */
    currObj += CEIL(xi->size);


    /* gather additional data */
    if (xi->addLen>0)
    {
      actSym += GetDepData(context, currObj,
                           desc, obj, &(theSymTab[actSym]), xi);
      currObj += xi->addLen;
    }
  }


  /* for all XINewCpl items in this message */
  for(i=0; i<msg->nNewCpl; i++)
  {
    theNewCpl[actNewCpl]  = msg->xferNewCpl[i]->te;
    actNewCpl++;
  }

  /* for all XIOldCpl items in this message */
  for(i=0; i<msg->nOldCpl; i++)
  {
    theOldCpl[actOldCpl]  = msg->xferOldCpl[i]->te;
    actOldCpl++;
  }


  /* sort SymTab, ObjTab and CplTab */
  /* sort SymTab according to the global ids stored there */
  std::sort(theSymTab, theSymTab + actSym, sort_SymTabEntries);

  /* sort ObjTab according to their global ids */
  /* sorting of objtab is necessary!! (see AcceptObjFromMsg) KB 960812 */
  const auto sort_ObjTabEntries = [=](const OBJTAB_ENTRY& a, const OBJTAB_ENTRY& b) {
    /* sort with ascending gid */
    return OTE_GID(theObjects, &a) < OTE_GID(theObjects, &b);
  };
  std::sort(theObjTab, theObjTab + msg->nObjects, sort_ObjTabEntries);


  /* substitute all pointers by index into SymTab */
  for(std::uintptr_t mi=0; mi<actSym; mi++)
  {
    /* patch SymTab index into reference location inside message */
    *(theSymTab[mi].adr.ref) = (DDD_OBJ)(mi+1);
  }


  /* TODO: theSymtab[].ref wird ab hier nicht mehr verwendet und muss nicht uebertragen werden! */


  /* set valid table entries */
  LC_SetTableLen(msg->msg_h, xferGlobals.symtab_id, actSym);
  LC_SetTableLen(msg->msg_h, xferGlobals.objtab_id, msg->nObjects);
  LC_SetTableLen(msg->msg_h, xferGlobals.newcpl_id, actNewCpl);
  LC_SetTableLen(msg->msg_h, xferGlobals.oldcpl_id, actOldCpl);


#if DebugXfer>1
  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
#endif
    XferDisplayMsg(context, "OS", msg->msg_h);
}




/****************************************************************************/
/*                                                                          */
/* Function:  XferPackMsgs                                                  */
/*                                                                          */
/* Purpose:   allocate one message buffer for each outgoing message,        */
/*            fill buffer with message contents and initiate asynchronous   */
/*            send for each message.                                        */
/*                                                                          */
/* Input:     theMsgs: list of message-send-infos                           */
/*                                                                          */
/* Output:    RET_ON_ERROR if error occured, RET_ON_OK otherwise            */
/*                                                                          */
/****************************************************************************/

RETCODE XferPackMsgs (DDD::DDDContext& context, XFERMSG *theMsgs)
{
  XFERMSG      *xm;

#if     DebugPack<=3
  Dune::dverb << "XferPackMsgs" << std::endl;
#endif

  /* allocate buffer, pack messages and send away */
  for(xm=theMsgs; xm!=NULL; xm=xm->next)
  {
    if (! LC_MsgAlloc(context, xm->msg_h))
    {
      Dune::dwarn << STR_NOMEM " in XferPackMsgs (size="
                  << LC_GetBufferSize(xm->msg_h) << ")\n";
      RET_ON_ERROR;
    }
    XferPackSingleMsg(context, xm);
    LC_MsgSend(context, xm->msg_h);
  }

  RET_ON_OK;
}




END_UGDIM_NAMESPACE
