// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  handler.c														*/
/*																			*/
/* Purpose:   defines the handlers used by ddd during data management.      */
/*			                                                                                                                                */
/*																			*/
/* Author:	  Stefan Lang                                                                           */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: stefan@ica3.uni-stuttgart.de							*/
/*			  phone: 0049-(0)711-685-7003									*/
/*			  fax  : 0049-(0)711-685-7000									*/
/*																			*/
/* History:   10.05.95 begin, ugp version 3.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

#ifdef ModelP

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <dune/common/unused.hh>

#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/domain/domain.h>
#include "parallel.h"
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/pargm.h>
#include <dune/uggrid/gm/rm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/shapes.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/heaps.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

/* UG namespaces: */
USING_UG_NAMESPACES

/* PPIF namespace */
using namespace PPIF;

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#ifdef Debug
#define DEBUGNSONS(e,f,m) {                                                  \
    if (f!=NULL)                                     \
    {                                                \
      if (CheckNSons(f,m))                         \
        UserWriteF(PFMT "inserted elem=" EID_FMTX\
                   "\n",me,EID_PRTX(e));                \
    }                                                \
}
#else
#define DEBUGNSONS(pe,f,m)
#endif

#define CEIL(n)          ((n)+((ALIGNMENT-((n)&(ALIGNMENT-1)))&(ALIGNMENT-1)))


/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/




/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

static void PrintSons (ELEMENT *theElement)
{
  ELEMENT *SonList[MAX_SONS];
  int i;

  if (GetAllSons(theElement,SonList)) ASSERT(0);
  for (i=0; SonList[i] != NULL; )
  {
    UserWriteF("elem=" EID_FMTX " son[%d]=" EID_FMTX "\n",
               EID_PRTX(theElement),i,EID_PRTX(SonList[i]));
    i++;
  }
}

static int CheckNSons (ELEMENT *theElement, char *buffer)
{
  ELEMENT *SonList[MAX_SONS];
  int i,nsons;

  if (GetAllSons(theElement,SonList)) ASSERT(0);
  for (i=0; SonList[i] != NULL; ) i++;

  nsons = NSONS(theElement);
  if(i != nsons)
  {
    UserWriteF("%s: elem=" EID_FMTX " ERROR nsons=%d NSONS=%d\n\n",
               buffer,EID_PRTX(theElement),i,nsons);
    if (1) PrintSons(theElement);
    fflush(stdout);
    return(1);
  }
  return (0);
}

static GRID *GetGridOnDemand (MULTIGRID *mg, int level)
{
  while (level > TOPLEVEL(mg))
  {
    PRINTDEBUG(dddif,1,(PFMT " CreateNewLevel toplevel=%d level=%d",me,TOPLEVEL(mg),level));
    if (CreateNewLevel(mg,0)==NULL) assert(0);
  }

  return GRID_ON_LEVEL(mg,level);
}




/****************************************************************************/
/*																			*/
/*      For data management during redistribution and communication             */
/*		DDD needs for each DDD (data) object several handlers.				*/
/*		These are:															*/
/*			HANDLER_LDATACONSTRUCTOR,  handler: init object's LDATA parts   */
/*			HANDLER_UPDATE,            handler: update objects internals    */
/*			HANDLER_OBJMKCONS,         handler: make obj consistent         */
/*			HANDLER_DESTRUCTOR,        handler: destruct object             */
/*			HANDLER_XFERCOPY,          handler: copy cmd during xfer        */
/*			HANDLER_XFERDELETE,        handler: delete cmd during xfer      */
/*			HANDLER_XFERGATHER,        handler: send additional data        */
/*			HANDLER_XFERSCATTER,       handler: recv additional data        */
/*																			*/
/*																			*/
/*	    For each of the ddd (data) objects the handlers needed for the      */
/*	    specific object are defined below in the order handlers for         */
/*																			*/
/*			DDD objects:													*/
/*				*dimension independent										*/
/*				 TypeVector, TypeIVertex, TypeBVertex, TypeNode				*/
/*				*dimension dependent										*/
/*				 2-Dim:														*/
/*				 TypeTrElem, TypeTrBElem,									*/
/*				 TypeQuElem, TypeQuBElem									*/
/*				 3-Dim:														*/
/*				 TypeTeElem, TypeTeBElem									*/
/*				 TypePyElem, TypePyBElem									*/
/*				 TypePrElem, TypePrBElem									*/
/*				 TypeHeElem, TypeHeBElem									*/
/*				 TypeEdge													*/
/*																			*/
/*			DDD data objects:												*/
/*				TypeMatrix,                                                                                             */
/*				2-Dim:														*/
/*				TypeEdge													*/
/*																			*/
/*		NOT all handlers are to be specified for each object!				*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typevector												*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

static void VectorUpdate (DDD::DDDContext& context, DDD_OBJ obj)
{
  VECTOR  *pv                     = (VECTOR *)obj;
  INT level           = ATTR_TO_GLEVEL(DDD_InfoAttr(PARHDR(pv)));
  GRID    *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
  INT prio            = PRIO(pv);

  PRINTDEBUG(dddif,1,(PFMT " VectorUpdate(): v=" VINDEX_FMTX
                      " VEOBJ=%d\n",me,VINDEX_PRTX(pv),OBJT(pv)))

  /* insert in vector list */
  GRID_LINK_VECTOR(theGrid,pv,prio);
}



static void VectorXferCopy(DDD::DDDContext& context, DDD_OBJ obj, DDD_PROC proc, DDD_PRIO prio)
{
  INT nmat    = 0;
  MATRIX  *mat;
  VECTOR  *pv             = (VECTOR *)obj;
  INT level           = ATTR_TO_GLEVEL(DDD_InfoAttr(PARHDR(pv)));
  /* TODO: define this static global                    */
  /* TODO: take size as maximum of possible connections */
  size_t sizeArray[256];
  INT flag;

  PRINTDEBUG(dddif,1,(PFMT " VectorXferCopy(): v=" VINDEX_FMTX " proc=%d "
                      "prio=%d vtype=%d\n",me,VINDEX_PRTX(pv),proc,prio,VTYPE(pv)))

#if defined __OVERLAP2__
  flag = 1;             /* for overlap 2 ghost-matrices are required too */
#else
  flag = (!GHOSTPRIO(prio));
    #ifndef __EXCHANGE_CONNECTIONS__
  flag = (flag && (level < 0));
        #endif
#endif

  if (flag) {
    if (DDD_XferWithAddData(context)) {
      for(mat=VSTART(pv); mat!=NULL; mat=MNEXT(mat)) {
        ASSERT(nmat<256);
        sizeArray[nmat++] = UG_MSIZE(mat);
      }
      PRINTDEBUG(dddif,2,(PFMT " VectorXferCopy(): v=" VINDEX_FMTX
                          " AddData nmat=%d\n",me,VINDEX_PRTX(pv),nmat))
      DDD_XferAddDataX(context, nmat, ddd_ctrl(context).TypeMatrix,sizeArray);
    }
  }
  /*
          else
          {
                  GRID *theGrid = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
                  MATRIX *theMatrix,*next;

                  for (theMatrix=VSTART(pv); theMatrix!=NULL; theMatrix = next) {
                          next = MNEXT(theMatrix);
                          if (DisposeConnection(theGrid,MMYCON(theMatrix)))
                              ASSERT(0);
                  }
          }
   */
}

static void VectorGatherMatX (DDD::DDDContext&, DDD_OBJ obj, int cnt, DDD_TYPE type_id, char **Data)
{
  VECTOR  *vec = (VECTOR *)obj;
  MATRIX  *mat;
  INT nmat = 0;

  PRINTDEBUG(dddif,3,(PFMT " VectorGatherMatX(): v=" VINDEX_FMTX
                      " VOBJID=%d cnt=%d type=%d veobj=%d vtype=%d\n",
                      me,VINDEX_PRTX(vec),ID(VOBJECT(vec)),cnt,type_id,
                      OBJT(vec),VTYPE(vec)))

  if (cnt<=0) return;

  for (mat=VSTART((VECTOR *) vec); mat!=NULL; mat=MNEXT(mat))
  {
    int Size;

    IFDEBUG(dddif,0)
    if (cnt<nmat+1)
    {
      PRINTDEBUG(dddif,0,(PFMT " VectorGatherMatX(): v=" VINDEX_FMTX
                          " cnt=%d nmat=%d type=%d veobj=%d\n",
                          me,VINDEX_PRTX(vec),cnt,nmat,type_id,OBJT(vec)))
      assert(0);
    }
    ENDDEBUG

      Size = UG_MSIZE(mat);
    memcpy(Data[nmat],mat,Size);

    PRINTDEBUG(dddif,3,(PFMT " VectorGatherMatX(): v=" VINDEX_FMTX
                        " mat=%x Size=%d vectoID=" VINDEX_FMTX "\n",
                        me,VINDEX_PRTX(vec),mat,Size,VINDEX_PRTX(MDEST(mat))))

    nmat++;
  }
}


static void VectorScatterConnX (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id, char **Data, int newness)
{
  VECTOR          *vec            = (VECTOR *)obj;
  CONNECTION      *first          = NULL,
  *last           = NULL;
  INT level           = ATTR_TO_GLEVEL(DDD_InfoAttr(PARHDR(vec)));
  GRID            *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
  INT prio            = PRIO(vec);
  INT i;
  INT nconn           = 0;
  INT newconn         = 0;

  PRINTDEBUG(dddif,3,(PFMT " VectorScatterConnX(): v=" VINDEX_FMTX
                      " cnt=%d type=%d veobj=%d vtype=%d\n",\
                      me,VINDEX_PRTX(vec),cnt,type_id,OBJT(vec),VTYPE(vec)))

#if ! defined __OVERLAP2__
  if (GHOSTPRIO(prio))
  {
    PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v=" VINDEX_FMTX
                        " USELESS since ghost vector\n",
                        me,VINDEX_PRTX(vec)))
    return;
  }
#endif

  if (cnt<=0) return;

  for (i=0; i<cnt; i++)
  {
    MATRIX *mcopy = (MATRIX *)Data[i];

    /* reset MNEXT */
    MNEXT(mcopy) = NULL;

    if (MDEST(mcopy)==NULL)
    {
      /* destination vector is not on this processor  */
      /* -> matrix entry is useless, throw away       */
      PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v=" VINDEX_FMTX
                          " mat=%x Size=%d, USELESS no dest vector \n",
                          me,VINDEX_PRTX(vec),mcopy,UG_MSIZE(mcopy)))
      continue;
    }

#if ! defined __OVERLAP2__
    if (GHOST(MDEST(mcopy)))
    {
      /* destination vector has only prio Ghost on this processor */
      /* -> matrix entry is useless, throw away                     */
      PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v=" VINDEX_FMTX
                          " mat=%x Size=%d, USELESS dest vect is ghost\n",
                          me,VINDEX_PRTX(vec),mcopy,UG_MSIZE(mcopy)))
      continue;
    }
#endif

    {
      MATRIX *m,*mat=NULL;

      /* does matrix entry already exist? */
      /* TODO not nice, linear search, change this! */
      for (m=VSTART((VECTOR *)vec);
           m!=NULL && (mat==NULL);
           m=MNEXT(m))
      {
        if (MDEST(m)==MDEST(mcopy)) mat=m;
      }

      /* matrix entry is really new */
      if (mat == NULL)
      {
        /* handle diagonal entry */
        if (MDIAG(mcopy))
        {
          /* matrix diagonal entry, no other vector is involved */
          CONNECTION *conn = (CONNECTION *)
                             GetMemoryForObject(ddd_ctrl(context).currMG,UG_MSIZE(mcopy),MAOBJ);

          nconn++; newconn++;

          if (conn==NULL)
          {
            UserWriteF("%2d:  VectorScatterConnX(): can't get mem "
                       "for conn=%x\n",conn);
            return;
          }

          PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v="
                              VINDEX_FMTX " conn=%x Size=%d, DIAG\n",
                              me,VINDEX_PRTX(vec),conn,UG_MSIZE(mcopy)))


          /* TODO: define clearly
             memcpy(conn,mcopy,UG_MSIZE(mcopy)); */
          memset(conn,0,UG_MSIZE(mcopy));
          memcpy(conn,mcopy,sizeof(MATRIX)-sizeof(DOUBLE));

          if (first==NULL) first = conn;
          else MNEXT(last) = conn;
          last = conn;
        }
        /* handle off-diagonal entry */
        else
        {
          /* matrix off-diagonal entry, another vector is involved */
          VECTOR *other = MDEST(mcopy);
          MATRIX *m, *back=NULL, *newm;

          /* does connection already exist for other vec? */
          /* TODO not nice, linear search, change this! */

          for (m=VSTART((VECTOR *)other);
               m!=NULL && back==NULL;
               m=MNEXT(m))
          {
            if (MDEST(m)==vec) back=m;
          }

          /* no backward entry, create connection */
          if (back==NULL)
          {
            MATRIX *otherm;
            CONNECTION *conn = (CONNECTION *)
                               GetMemoryForObject(ddd_ctrl(context).currMG,2*UG_MSIZE(mcopy),MAOBJ);

            nconn++; newconn++;

            if (conn==NULL)
            {
              UserWriteF("%2d:  VectorScatterConnX(): can't get "
                         "mem for mat=%x\n",mcopy);
              return;
            }


            if (MOFFSET(mcopy))
            {
              newm =         (MATRIX *) ((char *)conn+UG_MSIZE(mcopy));
              otherm = (MATRIX *) conn;

              PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v="
                                  VINDEX_FMTX " conn=%x newm=%x Size=%d vectoID="
                                  VINDEX_FMTX " GETMEM\n",
                                  me,VINDEX_PRTX(vec),conn,newm, UG_MSIZE(mcopy),
                                  VINDEX_PRTX(MDEST(mcopy))))
            }
            else
            {
              newm = (MATRIX *) conn;
              otherm = (MATRIX *) ((char *)conn+UG_MSIZE(mcopy));

              PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v="
                                  VINDEX_FMTX " conn=%x newm=%x Size=%d vectoID="
                                  VINDEX_FMTX " GETMEM\n",
                                  me,VINDEX_PRTX(vec),conn,newm, UG_MSIZE(mcopy),
                                  VINDEX_PRTX(MDEST(mcopy))))
            }

            MDEST(otherm) = NULL;
          }
          /* backward entry found, use existing connection */
          else
          {
            nconn++;
            /*
               newm = MADJ(back);
             */
            if (MOFFSET(back))
            {
              newm = (MATRIX *) ((char *)back-UG_MSIZE(mcopy));
              SETMOFFSET(mcopy,0);
            }
            else
            {
              newm = (MATRIX *) ((char *)back+UG_MSIZE(mcopy));
              SETMOFFSET(mcopy,1);
            }

            PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v="
                                VINDEX_FMTX " back=%x newm=%x Size=%d vectoID="
                                VINDEX_FMTX " REUSE\n",
                                me,VINDEX_PRTX(vec),back,newm,UG_MSIZE(mcopy),
                                VINDEX_PRTX(MDEST(mcopy))))
          }

          /* TODO: define clearly
             memcpy(newm,mcopy,UG_MSIZE(mcopy)); */
          memset(newm,0,UG_MSIZE(mcopy));
          memcpy(newm,mcopy,sizeof(MATRIX)-sizeof(DOUBLE));

          if (first==NULL) first = newm;
          else MNEXT(last) = newm;
          last = newm;
        }
      }
      /* matrix entry does already exist */
      else
      {
        PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX(): v="
                            VINDEX_FMTX " mat=%x Size=%d vectoID=" VINDEX_FMTX
                            " FOUND\n",me,VINDEX_PRTX(vec),mat,UG_MSIZE(mcopy),
                            VINDEX_PRTX(MDEST(mcopy))))
      }
    }
  }

  /* enter matrix list at the beginning of existing list for this vector */
  /* ensure diagonal entry being at first position */
  if (nconn > 0)
  {
    if (VSTART(vec)!=NULL)
    {
      MNEXT(last) = MNEXT(VSTART(vec));
      MNEXT(VSTART(vec)) = first;
    }
    else
    {
      MNEXT(last) = NULL;
      VSTART(vec) = first;
    }

                #ifdef Debug
    /*
                    {
                    MATRIX *mat;
            PRINTDEBUG(dddif,4,(PFMT " VectorScatterConnX():  v="
                    VINDEX_FMTX "new matrices:\n",me,VINDEX_PRTX(vec)));
                    for (mat=first; mat!=NULL; mat=MNEXT(mat))
                    {
                    PRINTDEBUG(dddif,4,(PFMT "     mat=%x dest=" EID_FMTX "\n",me,mat,VINDEX_PRTX(MDEST(mat))));
                    }
                    }
     */
                #endif
  }

#ifdef Debug
  {
    MATRIX *mat=NULL;

    for (mat=VSTART(vec); mat!=NULL; mat=MNEXT(mat))
    {
      if (MDEST(mat) == MDEST(MADJ(mat)))
      {
        if (MDIAG(mat)) continue;

        if (!MDIAG(mat))
          PRINTDEBUG(dddif,1,(PFMT " VectorScatterConnX(): NOT DIAGONAL v="
                              VINDEX_FMTX " conn=%x mat=%x Size=%d vectoID=" VINDEX_FMTX
                              "\n",me,VINDEX_PRTX(vec),MMYCON(mat),mat,UG_MSIZE(mat),
                              VINDEX_PRTX(MDEST(mat))));
      }
    }
  }
#endif

  /* count new connections */
  NC(theGrid) += newconn;
}



static void VectorObjMkCons (DDD::DDDContext& context, DDD_OBJ obj, int newness)
{
  VECTOR          *vec            = (VECTOR *) obj;
  MATRIX          *theMatrix,*Prev,*Next;
  INT level           = ATTR_TO_GLEVEL(DDD_InfoAttr(PARHDR(vec)));
  GRID        *theGrid    = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);


  PRINTDEBUG(dddif,2,(PFMT " VectorObjMkCons(): v=" VINDEX_FMTX
                      " VEOBJ=%d\n",
                      me,VINDEX_PRTX(vec),OBJT(vec)))

  /*
          NOTE (TODO): this might be too less. for n2n transfer, connections
          might be set up consisting of two matrix structures transfered from
          different procs. this code will NOT handle that case, the connection
          will be created with the first matrix and destroyed here. When the
          second message arrives, the second matrix will lead to construction
          of a second connection, which will also be deleted here. we would
          need a mkcons after all messages to handle that case. (NIY in ddd 1.6.5)

          THIS case is handeled in newer DDD version (s.l. 970127)!!
   */

  /* kill inconsistent connections */
  for (theMatrix = VSTART((VECTOR *)vec);
       theMatrix!=NULL; theMatrix=Next)
  {
    MATRIX *theAdjoint = MADJ(theMatrix);

    Next = MNEXT(theMatrix);

    if (MDEST(theAdjoint) == NULL)
    {
      PRINTDEBUG(dddif,4,(PFMT " VectorObjMkCons(): v=" VINDEX_FMTX
                          " mat=%x vectoID=%d, KILLING incomplete connection\n",
                          me,VINDEX_PRTX(vec),theMatrix,ID(MDEST(theMatrix))))

      ASSERT(!MDIAG(theMatrix));

      {
        INT size = ((MDIAG(theMatrix)) ? UG_MSIZE(theMatrix) : 2*UG_MSIZE(theMatrix));
        PutFreeObject(ddd_ctrl(context).currMG,MMYCON(theMatrix),size,MAOBJ);
      }


      MNEXT(Prev) = Next;

      NC(theGrid)--;
      continue;
    }
    Prev = theMatrix;
  }
}

static void VectorPriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO isnew)
{
  VECTOR  *pv                     = (VECTOR *)obj;
  INT level           = ATTR_TO_GLEVEL(DDD_InfoAttr(PARHDR(pv)));
  GRID    *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
  INT old                     = PRIO(pv);

  PRINTDEBUG(dddif,2,(PFMT " VectorPriorityUpdate(): v=" VINDEX_FMTX
                      " old=%d new=%d level=%d attr=%d\n",
                      me,VINDEX_PRTX(pv),old,isnew,level,
                      (int)DDD_InfoAttr(PARHDR(pv))));

  if (pv == NULL) return;
  if (old == isnew) return;

  if (old == PrioNone) {
    /* only valid for masters */
    ASSERT(isnew == PrioMaster);
    return;
  }

  if (isnew == PrioNone) {
    /* only valid when prio undefined */
    printf("prio=%d\n",old);
    fflush(stdout);
    ASSERT(old <= 0);
    return;
  }

  /* dispose connections for geom levels not for amg levels */
#if ! defined __OVERLAP2__
  if (level>=0)
    if (GHOSTPRIO(isnew))
    {
      MATRIX *theMatrix,*next;

      for (theMatrix=VSTART(pv); theMatrix!=NULL;
           theMatrix = next)
      {
        next = MNEXT(theMatrix);

        PRINTDEBUG(dddif,2,(PFMT " VectorPriorityUpdate(): v="
                            VINDEX_FMTX " old=%d new=%d dispose conn=%x\n",
                            me,VINDEX_PRTX(pv),old,isnew,MMYCON(theMatrix)))

        if (DisposeConnection(theGrid,MMYCON(theMatrix)))
          ASSERT(0);
      }
    }
#endif

        #ifdef __EXCHANGE_CONNECTIONS__
  IFDEBUG(dddif,1)
  if (isnew == PrioMaster)
  {
    if (VSTART(pv) == NULL)
    {
      PRINTDEBUG(dddif,0,(PFMT " VectorPriorityUpdate(): ERROR v="
                          VINDEX_FMTX " old=%d new=%d matrix list empty\n",
                          me,VINDEX_PRTX(pv),old,isnew))
    }
  }
  ENDDEBUG
        #endif

  GRID_UNLINK_VECTOR(theGrid,pv);

  GRID_LINK_VECTOR(theGrid,pv,isnew);

  return;
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeivertex											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typebvertex											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/


static void BVertexLDataConstructor (DDD::DDDContext&, DDD_OBJ obj)
{
  VERTEX  *theVertex                      = (VERTEX *) obj;

  PRINTDEBUG(dddif,1,(PFMT " BVertexLDataConstructor(): v=" VID_FMTX
                      " I/BVOBJ=%d\n",me,VID_PRTX(theVertex),OBJT(theVertex)))

  V_BNDP(theVertex) = NULL;
}


static void VertexUpdate (DDD::DDDContext& context, DDD_OBJ obj)
{
  VERTEX  *theVertex      = (VERTEX *) obj;
  INT level           = LEVEL(theVertex);
  GRID    *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
  INT prio            = VXPRIO(theVertex);

  PRINTDEBUG(dddif,1,(PFMT " VertexUpdate(): v=" VID_FMTX " I/BVOBJ=%d\n",
                      me,VID_PRTX(theVertex),OBJT(theVertex)))

  GRID_LINK_VERTEX(theGrid,theVertex,prio);


  /* this assertion is not correct, since there may be an arbitrary number of */
  /* calls to NodeUpdate(), which increments NOOFNODE							*/
  /*	ASSERT(NOOFNODE(theVertex) == 0); */

  /* update ID of vertex */
  /* TODO: change to global id */
  /* TODO: delete
     ID(theVertex) = (theGrid->mg->vertIdCounter)++;
   */
}

static void VertexObjMkCons (DDD::DDDContext&, DDD_OBJ obj, int newness)
{
  DUNE_UNUSED VERTEX  *theVertex      = (VERTEX *) obj;

  PRINTDEBUG(dddif,1,(PFMT " VertexObjMkCons(): v=" VID_FMTX
                      " I/BVOBJ=%d newness=%d\n",
                      me,VID_PRTX(theVertex),OBJT(theVertex),newness))
}


static void BVertexXferCopy (DDD::DDDContext& context, DDD_OBJ obj, DDD_PROC proc, DDD_PRIO prio)
{
  VERTEX  *theVertex                      = (VERTEX *) obj;

  PRINTDEBUG(dddif,1,(PFMT " BVertexXferCopy(): v=" VID_FMTX
                      " I/BVOBJ=%d proc=%d prio=%d \n",
                      me,VID_PRTX(theVertex),OBJT(theVertex),proc,prio))

  BVertexXferBndP(context, V_BNDP(theVertex),proc,prio);
}


static void BVertexGather (DDD::DDDContext&, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data)
{
  BVertexGatherBndP (V_BNDP((VERTEX *)obj),cnt,(char*)Data);
}

/* Handlers used by Dune to implement dynamic load balancing:
 * Dune writes the data into the objects' 'message_buffer'
 * variable, then we take it from there.
 * The first sizeof(int) bytes of the message_buffer is the length of
 * the message (without the size of the int).
 */
template<typename Entity>
static void DuneEntityGather (DDD::DDDContext&, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data)
{
  char* data = static_cast<char*>(Data);
  const Entity* entity = reinterpret_cast<Entity*>(obj);
  const auto size = entity->message_buffer_size();

  std::memcpy(data, &size, sizeof size);
  data += sizeof size;

  std::memcpy(data, entity->message_buffer(), size);
}

template<typename Entity>
static void DuneEntityScatter (DDD::DDDContext&, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data, int newness)
{
  const char* data = static_cast<const char*>(Data);
  Entity* entity = reinterpret_cast<Entity*>(obj);

  std::size_t size;
  std::memcpy(&size, data, sizeof size);
  data += sizeof size;

  char* buffer = static_cast<char*>(std::malloc(size));
  std::memcpy(buffer, data, size);
  entity->message_buffer(buffer, size);
}

static void BVertexScatter (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data, int newness)
{
  BVertexScatterBndP(context, &(V_BNDP((VERTEX *)obj)),cnt,(char*)Data);
}


static void VertexPriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO new_)
{
  VERTEX      *theVertex                      = (VERTEX *)obj;
  INT level           = LEVEL(theVertex);
  GRID        *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);
  INT old                     = VXPRIO(theVertex);

  PRINTDEBUG(dddif,2,(PFMT " VertexPriorityUpdate(): v=" VID_FMTX
                      " old=%d new=%d level=%d\n",me,VID_PRTX(theVertex),old,new_,level))

  if (theVertex == NULL) return;
  if (old == new_) return;

  if (old == PrioNone) {
    /* only valid for masters */
    ASSERT(new_ == PrioMaster);
    return;
  }

  if (new_ == PrioNone) {
    /* only valid when prio undefined */
    printf("prio=%d\n",old);
    fflush(stdout);
    ASSERT(old <= 0);
    return;
  }

  GRID_UNLINK_VERTEX(theGrid,theVertex);

  GRID_LINK_VERTEX(theGrid,theVertex,new_);

  return;
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typenode												*/
/*																			*/
/****************************************************************************/
/****************************************************************************/


static void NodeDestructor(DDD::DDDContext&, DDD_OBJ obj)
{
  NODE *node      = (NODE *) obj;

  node->message_buffer_free();

  PRINTDEBUG(dddif,2,(PFMT " NodeDestructor(): n=" ID_FMTX " NDOBJ=%d\n",
                      me,ID_PRTX(node),OBJT(node)))
}

static void NodeObjInit(DDD::DDDContext&, DDD_OBJ obj)
{
  NODE *node      = (NODE *) obj;

  node->message_buffer(nullptr, 0);

  PRINTDEBUG(dddif,2,(PFMT " NodeObjInit(): n=" ID_FMTX " NDOBJ=%d\n",
                      me,ID_PRTX(node),OBJT(node)))
}


static void NodeObjMkCons (DDD::DDDContext& context, DDD_OBJ obj, int newness)
{
  NODE *theNode   = (NODE *) obj;

  PRINTDEBUG(dddif,2,(PFMT " NodeObjMkCons(): n=" ID_FMTX " NDOBJ=%d\n",
                      me,ID_PRTX(theNode),OBJT(theNode)))

        #ifdef TOPNODE
  /* set topnode pointer of vertex */
  if (TOPNODE(MYVERTEX(theNode)) == NULL)
    TOPNODE(MYVERTEX(theNode)) = theNode;
  else
  {
    NODE *TopNode   = TOPNODE(MYVERTEX(theNode));
    INT level              = LEVEL(TopNode);

    if (level < LEVEL(theNode))
      TOPNODE(MYVERTEX(theNode)) = theNode;
  }
        #endif

  /* TODO: this needs to be done here not in NodeUpdate() for 2D,       */
  /* since father would be overwritten by ElemScatterEdge()                     */
        #ifdef __TWODIM__
  if (NFATHER(theNode) != NULL)
  {
    switch (NTYPE(theNode))
    {
    case (CORNER_NODE) :
      ASSERT(OBJT(NFATHER(theNode)) == NDOBJ);
      SONNODE((NODE *)NFATHER(theNode)) = theNode;
      break;

    case (MID_NODE) :
      ASSERT(OBJT(NFATHER(theNode)) == EDOBJ);
      MIDNODE((EDGE *)NFATHER(theNode)) = theNode;
      break;

    default :
      ASSERT(0);
      break;
    }
  }
        #endif

  /* set pointer of vector to its node */
  if (ddd_ctrl(context).nodeData && NVECTOR(theNode))
#ifdef __PERIODIC_BOUNDARY__
    if (VOBJECT(NVECTOR(theNode))==NULL || PRIO((NODE *)VOBJECT(NVECTOR(theNode)))<PRIO(theNode))
#endif
    VOBJECT(NVECTOR(theNode)) = (GEOM_OBJECT*)theNode;

}


/****************************************************************************/
/*																			*/
/* Function:  NodeUpdate                                                                                        */
/*																			*/
/* Purpose:   update information related to a node.                                             */
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the node to handle							*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void NodeUpdate (DDD::DDDContext& context, DDD_OBJ obj)
{
  NODE    *theNode        = (NODE *)obj;
  VERTEX  *theVertex      = MYVERTEX(theNode);
  INT level           = LEVEL(theNode);
  GRID    *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);
  INT prio            = PRIO(theNode);

  PRINTDEBUG(dddif,1,(PFMT " NodeUpdate(): n=" ID_FMTX " NDOBJ=%d\n",
                      me,ID_PRTX(theNode),OBJT(theNode)))

  /* insert in listpart */
  GRID_LINK_NODE(theGrid,theNode,prio);

  /* TODO: can this be done in NodeObjMkCons() also	*/
  /* to unify 2 and 3D case							*/
        #ifdef __THREEDIM__
  if (NFATHER(theNode) != NULL)
  {
    switch (NTYPE(theNode))
    {
    case (CORNER_NODE) :
      ASSERT(OBJT(NFATHER(theNode)) == NDOBJ);
      SONNODE((NODE *)NFATHER(theNode)) = theNode;
      break;

    case (MID_NODE) :
      ASSERT(OBJT((EDGE *)NFATHER(theNode)) == EDOBJ);
      MIDNODE((EDGE *)NFATHER(theNode)) = theNode;
      break;

    default :
      ASSERT(0);
      break;
    }
  }
        #endif

  if (NOOFNODE(theVertex)<NOOFNODEMAX-1)
    INCNOOFNODE(theVertex);
  else
    ASSERT(0);

  /* TODO: change to global id */
  /* TODO: delete
     ID(node) = (theGrid->mg->nodeIdCounter)++;
   */
}

/****************************************************************************/
/*																			*/
/* Function:  NodeXferCopy											        */
/*																			*/
/* Purpose:   initiate dependent copy of data related to a node.	                */
/*																			*/
/* Input:	  DDD_OBJ	obj:	the object which is transfered to proc		*/
/*			  int		proc:	destination processor for that object		*/
/*			  int		prio:	priority of object new local object			*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void NodeXferCopy (DDD::DDDContext& context, DDD_OBJ obj, DDD_PROC proc, DDD_PRIO prio)
{
  NODE    *theNode        = (NODE *)obj;
  VECTOR  *vec            = NULL;

  PRINTDEBUG(dddif,1,(PFMT " NodeXferCopy(): n=" ID_FMTX
                      " proc=%d prio=%d\n",
                      me,ID_PRTX(theNode),proc,prio));

  /* copy vertex */
  PRINTDEBUG(dddif,2,(PFMT " NodeXferCopy(): n=" ID_FMTX " Xfer v="
                      VID_FMTX "\n",me,ID_PRTX(theNode),VID_PRTX(MYVERTEX(theNode))))

        #ifdef Debug
  if (NFATHER(theNode) != NULL)
  {
    PRINTDEBUG(dddif,1,(PFMT " NodeXferCopy(): n=" ID_FMTX
                        " NTYPE=%d OBJT=%d\n",
                        me,ID_PRTX(theNode),NTYPE(theNode),
                        OBJT(NFATHER(theNode))));

    switch (NTYPE(theNode))
    {
    case (CORNER_NODE) :
      ASSERT(OBJT(NFATHER(theNode)) == NDOBJ);
      break;

    case (MID_NODE) :
      ASSERT(OBJT(NFATHER(theNode)) == EDOBJ);
      break;

    default :
      ASSERT(0);
      break;
    }
  }
        #endif

  if (DDD_XferWithAddData(context)) {
    /* Extra data for Dune */
    DDD_XferAddData(context, sizeof(theNode->message_buffer_size()) + theNode->message_buffer_size(), DDD_USER_DATA);
  }

  DDD_XferCopyObj(context, PARHDRV(MYVERTEX(theNode)), proc, prio);

  /* copy vector if defined */
  if (ddd_ctrl(context).nodeData)
  {
    vec = NVECTOR(theNode);
    if (vec != NULL) {
      INT Size = sizeof(VECTOR)-sizeof(DOUBLE)
             +FMT_S_VEC_TP(MGFORMAT(ddd_ctrl(context).currMG),VTYPE(vec));

      PRINTDEBUG(dddif,2,(PFMT " NodeXferCopy(): n=" ID_FMTX
                          " Xfer NODEVEC=" VINDEX_FMTX " size=%d\n",
                          me,ID_PRTX(theNode),VINDEX_PRTX(vec),Size))

      DDD_XferCopyObjX(context, PARHDR(vec), proc, prio, Size);
    }
  }
}


static void NodePriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO new_)
{
  NODE    *pn                     = (NODE *)obj;
  INT level           = LEVEL(pn);
  GRID    *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);
  INT old                     = PRIO(pn);

  PRINTDEBUG(dddif,2,(PFMT " NodePriorityUpdate(): n=" ID_FMTX " old=%d new=%d "
                      "level=%d\n",me,ID_PRTX(pn),old,new_,level))

  if (pn == NULL) return;
  if (old == new_) return;

  if (old == PrioNone)
  {
    /* only valid for masters */
    ASSERT(new_ == PrioMaster);
    return;
  }

  if (new_ == PrioNone)
  {
    /* only valid when prio undefined */
    printf("prio=%d\n",old);
    fflush(stdout);
    ASSERT(old <= 0);
    return;
  }

  /* insert in new list part */
  GRID_UNLINK_NODE(theGrid,pn);
  GRID_LINK_NODE(theGrid,pn,new_);

  return;
}

DDD_TYPE NS_DIM_PREFIX NFatherObjType(DDD::DDDContext& context, DDD_OBJ obj, DDD_OBJ ref)
{
  const auto& dddctrl = ddd_ctrl(context);
  NODE *theNode = (NODE *)obj;

  switch (NTYPE(theNode))
  {
  case (CORNER_NODE) :
                        #ifdef Debug
    if (OBJT((NODE *)ref) != NDOBJ)
      UserWriteF("NFatherObjType(): wrong OBJT=%d\n",OBJT((NODE *)ref));
                        #endif
    HEAPFAULT(ref);
    ASSERT(OBJT((NODE *)ref) == NDOBJ);
    return(dddctrl.TypeNode);

  case (MID_NODE) :
                        #ifdef Debug
    if (OBJT((EDGE *)ref) != EDOBJ)
      UserWriteF("NFatherObjType(): wrong OBJT=%d\n",OBJT((EDGE *)ref));
                        #endif
    HEAPFAULT(ref);
    ASSERT(OBJT((EDGE *)ref) == EDOBJ);
    return(dddctrl.TypeEdge);

  default :
    abort();
  }
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeelement											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* Function:  ElementLDataConstructor										*/
/*																			*/
/* Purpose:   update information related to an element.						*/
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the element to handle						*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void ElementLDataConstructor (DDD::DDDContext& context, DDD_OBJ obj)
{
  INT i;
  ELEMENT *pe                     = (ELEMENT *)obj;
  DUNE_UNUSED INT level           = LEVEL(pe);
  /* TODO: delete
  GRID    *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);
  INT prio            = EPRIO(pe); */

  pe->message_buffer(nullptr, 0);

  PRINTDEBUG(dddif,2,(PFMT " ElementLDataConsX(): pe=" EID_FMTX
                      " EOBJ=%d l=%d\n",me,EID_PRTX(pe),OBJT(pe),level))

  /*	TODO: delete
          GRID_LINK_ELEMENT(theGrid,pe,prio); */

  if (OBJT(pe)==BEOBJ)
  {
    for (i=0; i<SIDES_OF_ELEM(pe); i++)
      SET_BNDS(pe,i,NULL);
  }

  /* TODO: in global id umrechnen */
  /* TODO: delete
     ID(pe) = (theGrid->mg->elemIdCounter)++;
   */
}

/****************************************************************************/
/*																			*/
/* Function:  ElementUpdate                                                                                     */
/*																			*/
/* Purpose:   update information related to an element.						*/
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the element to handle						*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void ElementUpdate (DDD::DDDContext&, DDD_OBJ obj)
{
  DUNE_UNUSED ELEMENT *pe                     = (ELEMENT *)obj;

  PRINTDEBUG(dddif,1,(PFMT " ElementUpdate(): pe=" EID_FMTX
                      " EOBJ=%d\n",me,EID_PRTX(pe),OBJT(pe)))

  /* TODO: delete this
          SETNSONS(pe,0); */

  /* TODO: this is not true any more, since elements pe, which are new and
                   have local non-new sons, have already gotten their NSONS through
                   ElementrPriorityUpdate() of the sons!!!
          ASSERT(NSONS(pe) == 0);
   */
}

/****************************************************************************/
/*																			*/
/* Function:  ElementDelete													*/
/*																			*/
/* Purpose:   remove element from UG data structure.						*/
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the element to handle						*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void ElementDelete (DDD::DDDContext& context, DDD_OBJ obj)
{
  ELEMENT *pe                     = (ELEMENT *)obj;
  INT level           = LEVEL(pe);
  GRID    *theGrid        = GRID_ON_LEVEL(ddd_ctrl(context).currMG,level);

  PRINTDEBUG(dddif,1,(PFMT " ElementDelete(): e=" EID_FMTX " EOBJ=%d l=%d "
                      "ncon=%d\n",
                      me,EID_PRTX(pe),OBJT(pe),level,NC(theGrid)))

  /* dispose element without connections (false) */
  if (DisposeElement(theGrid, pe, false)) ASSERT(0);
}


/****************************************************************************/
/*																			*/
/* Function:  ElementXferCopy	                                                                                        */
/*																			*/
/* Purpose:   initiate dependent copy of data related to an element.            */
/*			  this handler is implemented for an arbitrary element type.	*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the object which is transfered to proc		*/
/*			  int		proc:	destination processor for that object		*/
/*			  int		prio:	priority of object new local object			*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

static void ElementXferCopy (DDD::DDDContext& context, DDD_OBJ obj, DDD_PROC proc, DDD_PRIO prio)
{
  auto& dddctrl = ddd_ctrl(context);

  INT i,nsides;
  INT Size;
  ELEMENT *pe     =       (ELEMENT *)obj;
  VECTOR  *vec;
  NODE    *node;
  BNDS    *bnds[MAX_SIDES_OF_ELEM];

  PRINTDEBUG(dddif,1,(PFMT " ElementXferCopy(): "
                      "pe=" EID_FMTX " proc=%d prio=%d EOBJT=%d\n",
                      me,EID_PRTX(pe),proc,prio,OBJT(pe)))

  /* add element sides */
  /* must be done before any XferCopyObj-call! herein */
  /* or directly after XferCopyObj-call */

  if (OBJT(pe)==BEOBJ) {
    nsides = SIDES_OF_ELEM(pe);
    for (i=0; i<nsides; i++)
      bnds[i] = ELEM_BNDS(pe,i);

    PRINTDEBUG(dddif,3,(PFMT " ElementXferCopy(): "
                        "pe=" EID_FMTX " BElementXferBndS nsides=%d\n",
                        me,EID_PRTX(pe),nsides))

    BElementXferBndS(context, bnds,nsides,proc,prio);
  }

  if (DDD_XferWithAddData(context)) {
    DDD_XferAddData(context, sizeof(pe->message_buffer_size()) + pe->message_buffer_size(), DDD_USER_DATA);

    /* add edges of element */
    /* must be done before any XferCopyObj-call! herein    */
    /* or directly after XferCopyObj-call for this element */
            #ifdef __TWODIM__
    DDD_XferAddData(context, EDGES_OF_ELEM(pe), dddctrl.TypeEdge);
            #endif
  }

  /* copy corner nodes */
  for(i=0; i<CORNERS_OF_ELEM(pe); i++)
  {
    node = CORNER(pe,i);

    PRINTDEBUG(dddif,2,(PFMT " ElementXferCopy():  e=" EID_FMTX
                        " Xfer n=" ID_FMTX " i=%d\n",
                        me,EID_PRTX(pe),ID_PRTX(node),i))

    DDD_XferCopyObj(context, PARHDR(node), proc, prio);
  }

  /* send edge and edge vectors */
  for (i=0; i<EDGES_OF_ELEM(pe); i++)
  {
    EDGE* edge = GetEdge(CORNER(pe,CORNER_OF_EDGE(pe,i,0)),
                   CORNER(pe,CORNER_OF_EDGE(pe,i,1)));
    ASSERT(edge != NULL);
    ASSERT(OBJT(edge) == EDOBJ);

    PRINTDEBUG(dddif,2,(PFMT " ElementXferCopy():  e=" EID_FMTX
                        " EDGE=%x/%08x proc=%d prio=%d\n",
                        me,EID_PRTX(pe),edge,DDD_InfoGlobalId(PARHDR(edge)),
                        proc,prio))

    DDD_XferCopyObj(context, PARHDR(edge), proc, prio);

    if (dddctrl.edgeData) {
      VECTOR *vec = EDVECTOR(edge);

      if (vec != NULL) {
        int Size = sizeof(VECTOR)-sizeof(DOUBLE)
               +FMT_S_VEC_TP(MGFORMAT(dddctrl.currMG),VTYPE(vec));
        PRINTDEBUG(dddif,3,(PFMT " ElementXferCopy():  e=" EID_FMTX
                            " EDGEVEC=" VINDEX_FMTX " size=%d\n",
                            me,EID_PRTX(pe),VINDEX_PRTX(vec),Size))
        DDD_XferCopyObjX(context, PARHDR(vec), proc, prio, Size);
      }
    }
  }



  /* copy element vector */
  if (ddd_ctrl(context).elemData)
  {
    vec = EVECTOR(pe);

    if (vec != NULL) {
      Size = sizeof(VECTOR)-sizeof(DOUBLE)
             +FMT_S_VEC_TP(MGFORMAT(dddctrl.currMG),VTYPE(vec));

      PRINTDEBUG(dddif,2,(PFMT " ElementXferCopy(): e=" EID_FMTX
                          " ELEMVEC=" VINDEX_FMTX " size=%d\n",
                          me,EID_PRTX(pe),VINDEX_PRTX(vec),Size))

      DDD_XferCopyObjX(context, PARHDR(vec), proc, prio, Size);
    }
  }

  /* copy sidevectors */
  if (dddctrl.sideData)
  {
    for (i=0; i<SIDES_OF_ELEM(pe); i++)
    {
      vec = SVECTOR(pe,i);

      if (vec != NULL) {
        Size = sizeof(VECTOR)-sizeof(DOUBLE)
               +FMT_S_VEC_TP(MGFORMAT(dddctrl.currMG),VTYPE(vec));

        PRINTDEBUG(dddif,2,(PFMT " ElementXferCopy(): e=" EID_FMTX
                            " SIDEVEC=" VINDEX_FMTX " size=%d\n",
                            me,EID_PRTX(pe),VINDEX_PRTX(vec),Size))
        DDD_XferCopyObjX(context, PARHDR(vec), proc, prio, Size);
      }
    }
  }
}


/****************************************************************************/

#ifdef __TWODIM__
static void ElemGatherEdge (DDD::DDDContext& context, ELEMENT *pe, int cnt, char *data)
{
  INT i;
  INT size = sizeof(EDGE) - ((ddd_ctrl(context).edgeData) ? 0 : sizeof(VECTOR*));

  PRINTDEBUG(dddif,3,(PFMT " ElemGatherEdge(): pe=" EID_FMTX " cnt=%d size=%d\n",
                      me,EID_PRTX(pe),cnt,size))

  /* copy edges into message */
  for (i=0; i<EDGES_OF_ELEM(pe); i++)
  {
    EDGE *edge = GetEdge(CORNER(pe,CORNER_OF_EDGE(pe,i,0)),
                         CORNER(pe,CORNER_OF_EDGE(pe,i,1)));
    ASSERT(edge!=NULL);
    memcpy(data, (char *)edge, size);
    data += CEIL(size);


    PRINTDEBUG(dddif,2,(PFMT " ElemGatherEdge(): pe=" EID_FMTX " i=%d n1="
                        ID_FMTX " n2=" ID_FMTX " nmid=%08x\n",me,EID_PRTX(pe),i,
                        ID_PRTX(NBNODE(LINK0(edge))),ID_PRTX(NBNODE(LINK1(edge))),
                        MIDNODE(edge)))
  }
}


static void ElemScatterEdge (DDD::DDDContext& context, ELEMENT *pe, int cnt, char *data, int newness)
{
  INT i;
  INT size    = sizeof(EDGE) - ((ddd_ctrl(context).edgeData) ? 0 : sizeof(VECTOR*));
  INT level   = LEVEL(pe);
  GRID    *theGrid = GetGridOnDemand(ddd_ctrl(context).currMG,level);

  PRINTDEBUG(dddif,3,(PFMT " ElemScatterEdge(): pe=" EID_FMTX
                      " cnt=%d newness=%d\n",
                      me,EID_PRTX(pe),cnt,newness))

  /* XFER_REJECT:   only case where edges must not be unpacked */
  /* this is not correct, since newness XFER_REJECT might have */
  /* non NULL midnode pointers in the edges (971007 s.l.)      */
  /* XFER_REJECT:  new MIDNODE pointers might be non NULL     */
  /* XFER_NEW:      there are still no edges -> unpack         */
  /* XFER_UPGRADE:  new MIDNODE pointers might be non NULL     */
  /* if (newness == XFER_REJECT) return; */

  /* retrieve edges from message */
  for (i=0; i<cnt; i++)
  {
    EDGE *enew, *ecopy = (EDGE *)data;
    data += CEIL(size);

    PRINTDEBUG(dddif,2,(PFMT " ElemScatterEdge(): elem=" EID_FMTX
                        " i=%d n1=" ID_FMTX " n2=" ID_FMTX " midnode=%x\n",
                        me,pe,EID_PRTX(pe),i,
                        ID_PRTX(NBNODE(LINK0(ecopy))),
                        ID_PRTX(NBNODE(LINK1(ecopy))),
                        ecopy->midnode))

    /* this is the 2D case for edge creation:              */
    /*    CreateEdge increments the NO_OF_ELEM count       */
    /*    NO_OF_ELEM counter gets wrong if an element      */
    /*    is unpacked several times.                       */
    ASSERT(NBNODE(LINK0(ecopy)) != NULL);
    ASSERT(NBNODE(LINK1(ecopy)) != NULL);
    if (newness == XFER_NEW) {
      enew = CreateEdge(theGrid,pe,i, false);
      SETEDSUBDOM(enew,EDSUBDOM(ecopy));
    }
    else {
      enew = GetEdge(NBNODE(LINK0(ecopy)),
                     NBNODE(LINK1(ecopy)));
      if (enew == NULL) {
        enew = CreateEdge(theGrid,pe,i,false);
        SETEDSUBDOM(enew,EDSUBDOM(ecopy));
        /* TODO: remove this check if the first call is with XFER_NEW */
        ASSERT(0);
        DEC_NO_OF_ELEM(enew);
      }
    }
    PRINTDEBUG(dddif,2,(PFMT " ElemScatterEdge(): elem=" EID_FMTX
                        " create edge=%x for n0=" EID_FMTX " n1=" ID_FMTX "\n",
                        me,EID_PRTX(pe),enew,
                        ID_PRTX(NBNODE(LINK0(ecopy))),
                        ID_PRTX(NBNODE(LINK1(ecopy)))));

    if (enew == NULL)
    {
      PRINTDEBUG(dddif,1,(PFMT "  ElemScatterEdge(): ERROR pe=" EID_FMTX
                          " i=%d %s returned NULL\n",
                          me,EID_PRTX(pe),i,(newness==XFER_NEW) ? "CreateEdge()" : "GetEdge()"));
      ASSERT(0);
    }
#ifdef Debug
    {
      EDGE *edge0,*edge1;

      edge0 = GetEdge(NBNODE(LINK0(ecopy)),NBNODE(LINK1(ecopy)));
      edge1 = GetEdge(NBNODE(LINK1(ecopy)),NBNODE(LINK0(ecopy)));
      if (edge0 != edge1)
      {
        PRINTDEBUG(dddif,1,(PFMT " ElemScatterEdge(): n0=" ID_FMTX
                            " n1=" ID_FMTX " edge0=%08x BUT edge1=%08x\n",me,
                            ID_PRTX(NBNODE(LINK0(ecopy))),
                            ID_PRTX(NBNODE(LINK1(ecopy))),
                            edge0,edge1));
        ASSERT(0);
      }
    }
#endif

    /* copy midnode pointer */
    if (MIDNODE(ecopy) != NULL)
    {
      VERTEX                  *theVertex;
      INT co0,co1;

      MIDNODE(enew)   = MIDNODE(ecopy);
      theVertex               = MYVERTEX(MIDNODE(enew));

      /* reconstruct local coordinates of vertex */
      co0 = CORNER_OF_EDGE(pe,i,0);
      co1 = CORNER_OF_EDGE(pe,i,1);

      ASSERT(theVertex != NULL);

      /* local coordinates have to be local towards pe */
      if (EMASTER(pe)) {
        if (MOVED(theVertex))
        {
          DOUBLE *x[MAX_CORNERS_OF_ELEM];
          INT n;

          CORNER_COORDINATES(pe,n,x);
          UG_GlobalToLocal(n,(const DOUBLE **)x,
                           CVECT(theVertex),LCVECT(theVertex));
        }
        else
          V_DIM_LINCOMB(0.5, LOCAL_COORD_OF_ELEM(pe,co0),
                        0.5, LOCAL_COORD_OF_ELEM(pe,co1),
                        LCVECT(theVertex));
        /* make vertex information consistent */
        VFATHER(theVertex) = pe;
        SETONEDGE(theVertex,i);
      }

      /* set nfather pointer of midnode */
      ASSERT(enew != NULL);
      ASSERT(ID(MIDNODE(enew)) != -1);
      SETNFATHER(MIDNODE(enew),(GEOM_OBJECT *)enew);

      PRINTDEBUG(dddif,1,(PFMT " ElemScatterEdge(): n=" ID_FMTX
                          " NTYPE=%d OBJT=%d father " ID_FMTX " \n",
                          me,ID_PRTX(MIDNODE(enew)),NTYPE(MIDNODE(enew)),
                          OBJT(NFATHER(MIDNODE(enew))),
                          NFATHER(MIDNODE(enew))));
    }

    /* copy edge vector pointer */
    if (newness == XFER_NEW)
      if (ddd_ctrl(context).edgeData)
        if (EDVECTOR(ecopy) != NULL) {
          EDVECTOR(enew) = EDVECTOR(ecopy);
          VOBJECT(EDVECTOR(enew)) = (GEOM_OBJECT *)enew;
        }
  }
}
#endif /* end __TWODIM__ */


/****************************************************************************/


static void ElemGatherI (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *data)
{
  if (type_id == DDD_USER_DATA)
  {
    DuneEntityGather<union element>(context, obj, cnt, type_id, data);
    return;
  }

    #ifdef __TWODIM__
  /* now: type_id is always TypeEdge */
  ElemGatherEdge(context, (ELEMENT *)obj, cnt, (char *)data);
        #endif
}


static void ElemScatterI (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id,
                          void *data, int newness)
{
  if (type_id == DDD_USER_DATA)
  {
    DuneEntityScatter<union element>(context, obj, cnt, type_id, data, newness);
    return;
  }

    #ifdef __TWODIM__
  /* type_id is always TypeEdge */
  ElemScatterEdge(context, (ELEMENT *)obj, cnt, (char *)data, newness);
        #endif
}

static void ElemGatherB (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *data)
{
  INT i,nsides;
  BNDS    *bnds[MAX_SIDES_OF_ELEM];
  ELEMENT *pe = (ELEMENT *)obj;

  if (type_id == DDD_DOMAIN_DATA)
  {
    nsides = SIDES_OF_ELEM(pe);
    for (i=0; i<nsides; i++)
      bnds[i] = ELEM_BNDS(pe,i);
    BElementGatherBndS(bnds, nsides, cnt, (char *)data);
    return;
  }
  if (type_id == DDD_USER_DATA)
  {
    DuneEntityGather<union element>(context, obj, cnt, type_id, data);
    return;
  }

  /* now: type_id is TypeEdge or other */
        #ifdef __TWODIM__
  if (type_id==ddd_ctrl(context).TypeEdge)
  {
    ElemGatherEdge(context, pe, cnt, (char *)data);
  }
        #endif
}


static void ElemScatterB (DDD::DDDContext& context, DDD_OBJ obj, int cnt, DDD_TYPE type_id,
                          void *data, int newness)
{
  INT i,nsides;
  BNDS    *bnds[MAX_SIDES_OF_ELEM];
  ELEMENT *pe = (ELEMENT *)obj;

  if (type_id == DDD_DOMAIN_DATA)
  {
    nsides = SIDES_OF_ELEM(pe);
    for (i=0; i<nsides; i++)
      bnds[i] = ELEM_BNDS(pe,i);
    BElementScatterBndS(context, bnds, nsides, cnt, (char *)data);
    for (i=0; i<nsides; i++)
      SET_BNDS(pe,i,bnds[i]);
    return;
  }
  if (type_id == DDD_USER_DATA)
  {
    DuneEntityScatter<union element>(context, obj, cnt, type_id, data, newness);
    return;
  }

  /* now: type_id is TypeEdge or other */
        #ifdef __TWODIM__
  if (type_id==ddd_ctrl(context).TypeEdge)
  {
    ElemScatterEdge(context, pe, cnt, (char *)data, newness);
  }
        #endif
}


/****************************************************************************/


static void ElementObjMkCons (DDD::DDDContext& context, DDD_OBJ obj, int newness)
{
  INT i,j;
  INT lostson         = 0;
  ELEMENT *pe                     = (ELEMENT *)obj;
  ELEMENT *succe          = SUCCE(pe);
  ELEMENT *Next           = NULL;
  INT prio            = EPRIO(pe);
  ELEMENT *theFather      = EFATHER(pe);
  ELEMENT *NbElement;
  INT level           = LEVEL(pe);
  GRID    *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);


  PRINTDEBUG(dddif,1,(PFMT " ElementObjMkCons(): pe=" EID_FMTX
                      " newness=%d\n",
                      me,EID_PRTX(pe),newness))

  DEBUGNSONS(pe,theFather,"ElementObjMkCons begin:");

  /* correct nb relationships between ghostelements */
  if (EGHOST(pe))
  {
    for (i=0; i<SIDES_OF_ELEM(pe); i++)
    {
      NbElement = NBELEM(pe,i);
      if (NbElement!=NULL && EGHOST(NbElement))
      {
        for (j=0; j<SIDES_OF_ELEM(NbElement); j++)
          if (NBELEM(NbElement,j) == pe) break;
        /* no backptr reset nb pointer */
        if (j >= SIDES_OF_ELEM(NbElement)) SET_NBELEM(pe,i,NULL);
      }
    }
  }

  /* reconstruct pointer from vectors */
  if (ddd_ctrl(context).elemData) VOBJECT(EVECTOR(pe)) = (GEOM_OBJECT*)pe;

  if (ddd_ctrl(context).sideData)
    for (i=0; i<SIDES_OF_ELEM(pe); i++) {
      VOBJECT(SVECTOR(pe,i)) = (GEOM_OBJECT*)pe;
      SETVECTORSIDE(SVECTOR(pe,i), i);
    }

  /*  if called with prio old=ghost and new=ghost,
          then you have eventually to unlink and link
          an element again to avoid
          decoupling of element and its father.
          Sample scenario:
                  father=a  son=x are on proc p.
                  father is deleted and removes his reference in son,
                  but father and son are sent again to p. Son x gets
                  his father pointer again. Son needs to be
                  rearranged in element list to be surely a son of father.
                  This applies only for ghost sons, since master sons
                  avoid deleting of their fathers?!
   */
  if (newness != XFER_NEW)
  {
    if (prio == PrioMaster)
      return;
    else if (theFather != NULL)
    {
      ELEMENT *SonList[MAX_SONS];
      int i;

      /* check whether NSONS of father must be incremented */
      if (GetAllSons(theFather,SonList)) ASSERT(0);
      i = 0;
      while (SonList[i] != NULL)
      {
        if (SonList[i] == pe) return;
        i++;
      }
      PRINTDEBUG(dddif,1,(PFMT "  ElementObjMkCons(): father: f="
                          EID_FMTX " lost son=" EID_FMTX " nsons=%d\n",me,
                          EID_PRTX(theFather),EID_PRTX(pe),NSONS(theFather)));

      lostson = 1;
      GRID_UNLINK_ELEMENT(theGrid,pe);

      if (SON(theFather,PRIO2INDEX(prio)) == pe)
      {
        if (succe != NULL)
        {
          if (EFATHER(succe)==theFather)
                    #ifdef ModelP
            if (PRIO2INDEX(EPRIO(succe)) == PRIO2INDEX(prio))
                    #endif
          {
            Next = succe;
          }
        }
        SET_SON(theFather,PRIO2INDEX(prio),Next);
      }
    }
  }

  if (newness == XFER_NEW || lostson)
  {
    /* link element into list according to prio */
    INT where   = PRIO2INDEX(prio);
    ELEMENT *after;

    if (theFather != NULL)
    {
      /* link element with father */
      after = SON(theFather,where);

      PRINTDEBUG(dddif,1,(PFMT " ElementObjMkCons(): GRID_LINKX_ELEMENT "
                          "pe=" EID_FMTX " prio=%d where=%d after=%x father= " EID_FMTX
                          "\n", me,EID_PRTX(pe),prio,where,after,EID_PRTX(theFather)))

      GRID_LINKX_ELEMENT(theGrid,pe,prio,after);

      /* construct son information */
      if (after == NULL)
      {
        ELEMENT *next;

        SET_SON(theFather,where,pe);

        /* every successor of pe was decoupled before */
        /* -> correct NSONS                          */
        next = SUCCE(pe);
        while (next!=NULL && PRIO2INDEX(EPRIO(next))==PRIO2INDEX(prio)
               && theFather==EFATHER(next))
        {
          SETNSONS(theFather,NSONS(theFather)+1);
          next = SUCCE(next);
        }
      }
      SETNSONS(theFather,NSONS(theFather)+1);
    }
    else
    {
      /* link coarse grid element or ghost element */
      GRID_LINK_ELEMENT(theGrid,pe,prio);

                        #ifdef Debug
      if (level > 0)
        /* only ghost elements may have no father */
        assert(EGHOST(pe));
                        #endif
    }
  }

        #ifdef __THREEDIM__
  /* update element count of edges for new created elements */
  if (newness == XFER_NEW)
    /* increment elem counter in edges */
    for (i=0; i<EDGES_OF_ELEM(pe); i++)
    {
      EDGE *theEdge;
      NODE *theNode0 = CORNER(pe,CORNER_OF_EDGE(pe,i,0));
      NODE *theNode1 = CORNER(pe,CORNER_OF_EDGE(pe,i,1));

      ASSERT(theNode0!=NULL && theNode1!=NULL);

      PRINTDEBUG(dddif,4,(PFMT " ElementObjMkCons(): pe=" EID_FMTX
                          " INC_NO_OF_ELEM for n0=" ID_FMTX " n1=" ID_FMTX "\n",
                          me,EID_PRTX(pe),ID_PRTX(theNode0),ID_PRTX(theNode1)))

      theEdge = GetEdge(theNode0,theNode1);
      if (theEdge == NULL)
      {
        PRINTDEBUG(dddif,0,(PFMT " ElementObjMkCons(): pe=" EID_FMTX
                            "/%d/%d INC_NO_OF_ELEM for n0=" ID_FMTX " n1=" ID_FMTX " FAILED\n",
                            me,EID_PRTX(pe),ECLASS(pe),REFINE(EFATHER(pe)),
                            ID_PRTX(theNode0),ID_PRTX(theNode1)))
        assert(0);
      }

      INC_NO_OF_ELEM(theEdge);
    }
        #endif

  DEBUGNSONS(pe,theFather,"end ElementObjMkCons");
}


static void ElementPriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO new_)
{
  ELEMENT *pe                     = (ELEMENT *)obj;
  ELEMENT *theFather      = EFATHER(pe);
  ELEMENT *succe          = SUCCE(pe);
  INT level           = LEVEL(pe);
  GRID    *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);
  INT old                     = EPRIO(pe);
  INT lostson         = 1;

  PRINTDEBUG(dddif,1,(PFMT "  ElementPriorityUpdate(): e=" EID_FMTX
                      " old=%d new=%d level=%d\n",me,EID_PRTX(pe),old,new_,level))

  if (pe == NULL) return;

  /*  if called with prio old=ghost and new=ghost,
          then you have to unlink and link again to avoid
          decoupling of son and father.
          Sample cenario:
                  father=a  son=x are on proc p.
                  father is deleted and removes his reference in son,
                  but father and son are sent again to p. Son x gets
                  his father pointer again. Son needs to be
                  rearranged in element to list be surely a son of father.
                  This applies only for ghost sons, since master sons
                  avoid deleting of their fathers?!
   */

  if (old == PrioNone) {
    /* only valid for masters */
    ASSERT(new_ == PrioMaster);
    return;
  }

  if (new_ == PrioNone) {
    /* only valid when prio undefined */
    printf("prio=%d\n",old);
    fflush(stdout);
    ASSERT(old <= 0);
    return;
  }

  /* check whether element and father are decoupled */
  if (theFather != NULL)
  {
    ELEMENT *SonList[MAX_SONS];
    int i;

    if (GetAllSons(theFather,SonList)) ASSERT(0);
    i = 0;
    while (SonList[i] != NULL)
    {
      if (SonList[i] == pe) lostson = 0;
      i++;
    }

    PRINTDEBUG(dddif,1,(PFMT "  ElementPriorityUpdate(): father: f="
                        EID_FMTX " lost son=" EID_FMTX " nsons=%d\n",me,
                        EID_PRTX(theFather),EID_PRTX(pe),NSONS(theFather)));

    if (lostson == 1)
      SETNSONS(theFather,NSONS(theFather)+1);
    else if (old == new_)
      return;
  }

  GRID_UNLINK_ELEMENT(theGrid,pe);

  /* link element into list according to prio */
  {
    INT where   = PRIO2INDEX(new_);
    ELEMENT *after;

    if (theFather != NULL)
    {
      ELEMENT *Next           = NULL;
      INT oldwhere        = PRIO2INDEX(old);

      /* update son information for old prio */
      if (pe == SON(theFather,oldwhere))
      {
        if (succe != NULL)
          if (EFATHER(succe)==theFather && PRIO2INDEX(EPRIO(succe))==PRIO2INDEX(old))
            Next = succe;

        SET_SON(theFather,oldwhere,Next);
      }

      /* link elements with father */
      after = SON(theFather,where);

      PRINTDEBUG(dddif,2,(PFMT " ElementPriorityUpdate(): GRID_LINKX_ELEMENT "
                          "pe=" EID_FMTX " prio=%d after=%x\n",me,EID_PRTX(pe),new_,after))

      GRID_LINKX_ELEMENT(theGrid,pe,new_,after);

      /* update son information for new prio */
      if (after == NULL)
      {
        ELEMENT *next;

        SET_SON(theFather,where,pe);

        /* add successor elements which were decoupled before */
        next = SUCCE(pe);
        while (next!=NULL && PRIO2INDEX(EPRIO(next))==PRIO2INDEX(new_)
               && theFather==EFATHER(next))
        {
          SETNSONS(theFather,NSONS(theFather)+1);
          next = SUCCE(next);
        }
      }
    }
    else
    {
      PRINTDEBUG(dddif,2,(PFMT " ElementPriorityUpdate(): GRID_LINK_ELEMENT "
                          "pe=" EID_FMTX " prio=%d",me,EID_PRTX(pe),new_))

      /* link coarse grid element */
      GRID_LINK_ELEMENT(theGrid,pe,new_);

      /* only GhostElements may have no father */
      /*
                              if (level > 0)
                              {
                                      assert(EGHOSTPRIO(new_));
                                      assert(EGHOST(pe));
                              }
       */
    }
  }

  return;
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeedge                                                                                           */
/*																			*/
/****************************************************************************/
/****************************************************************************/

static void EdgeUpdate (DDD::DDDContext& context, DDD_OBJ obj)
{
  EDGE    *pe                     = (EDGE *)obj;
  INT level           = LEVEL(NBNODE(LINK0(pe)));
  GRID    *theGrid        = GetGridOnDemand(ddd_ctrl(context).currMG,level);

  PRINTDEBUG(dddif,1,(PFMT " EdgeUpdate(): edge=%x/%08x EDOBJT=%d "
                      " NO_OF_ELEM=%d\n",
                      me,pe,DDD_InfoGlobalId(PARHDR(pe)),OBJT(pe),NO_OF_ELEM(pe)))

  {
    /* insert in link lists of nodes */
    LINK* link0 = LINK0(pe);
    LINK* link1 = LINK1(pe);

    PRINTDEBUG(dddif,2,(PFMT " EdgeUpdate(): edge=%x/%08x node0="
                        ID_FMTX " node1=" ID_FMTX "\n",
                        me,pe,DDD_InfoGlobalId(PARHDR(pe)),
                        ID_PRTX(NBNODE(link1)),ID_PRTX(NBNODE(link0))))

    NODE* node0 = NBNODE(link1);
    NODE* node1 = NBNODE(link0);

    NEXT(link0) = START(node0);
    START(node0) = link0;
    NEXT(link1) = START(node1);
    START(node1) = link1;

    /* reset element counter
       SET_NO_OF_ELEM(pe,0); */
  }

  /* set nfather pointer of midnode */
  if (MIDNODE(pe) != NULL)
  {
    ASSERT(ID(MIDNODE(pe)) != -1);
    ASSERT(NTYPE((MIDNODE(pe))) == MID_NODE);
    SETNFATHER(MIDNODE(pe),(GEOM_OBJECT *)pe);
  }

  ASSERT(OBJT(pe) == EDOBJ);

  /* increment counter */
  NE(theGrid)++;
}

static void EdgePriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO new_)
{
  EDGE    *theEdge        = (EDGE *)obj;
  INT level           = LEVEL(theEdge);
  DUNE_UNUSED INT old                     = PRIO(theEdge);

  GetGridOnDemand(ddd_ctrl(context).currMG,level);

  PRINTDEBUG(dddif,2,(PFMT " EdgePriorityUpdate(): n=" ID_FMTX " old=%d new_=%d "
                      "level=%d\n",me,ID_PRTX(theEdge),old,new_,level))
}

static void EdgeObjMkCons (DDD::DDDContext& context, DDD_OBJ obj, int newness)
{
  EDGE *theEdge   = (EDGE *) obj;

  PRINTDEBUG(dddif,2,(PFMT " EdgeObjMkCons(): n=" ID_FMTX " EDOBJ=%d\n",
                      me,ID_PRTX(theEdge),OBJT(theEdge)))

  /* set pointer of vector to its edge */
  if (ddd_ctrl(context).edgeData && EDVECTOR(theEdge))
    VOBJECT(EDVECTOR(theEdge)) = (GEOM_OBJECT*)theEdge;

  ASSERT(OBJT(theEdge) == EDOBJ);
}

static void EdgeXferCopy (DDD::DDDContext& context, DDD_OBJ obj, DDD_PROC proc, DDD_PRIO prio)
{
  DUNE_UNUSED EDGE *pe        =       (EDGE *)obj;

  PRINTDEBUG(dddif,1,(PFMT " EdgeXferCopy(): edge=%x/%08x proc=%d prio=%d\n",
                      me,pe,DDD_InfoGlobalId(PARHDR(pe)),proc,prio));

  ASSERT(OBJT(pe) == EDOBJ);
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		general handlers                                                                                                        */
/*																			*/
/****************************************************************************/
/****************************************************************************/

void NS_DIM_PREFIX ObjectPriorityUpdate (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO new_)
{
  char type = OBJT(obj);

  switch(type)
  {
  case VEOBJ :
    VectorPriorityUpdate(context, obj,new_);
    break;
  case IVOBJ :
  case BVOBJ :
    VertexPriorityUpdate(context, obj,new_);
    break;
  case NDOBJ :
    NodePriorityUpdate(context, obj,new_);
    break;
  case IEOBJ :
  case BEOBJ :
    ElementPriorityUpdate(context, obj,new_);
    break;
  case EDOBJ :
    EdgePriorityUpdate(context, obj,new_);
    break;
  default : std::abort();
  }
  return;
}

/****************************************************************************/


/* init handlers for all element */
static void ElemHandlerInit (DDD::DDDContext& context, DDD_TYPE etype, INT handlerSet)
{
  DDD_SetHandlerLDATACONSTRUCTOR(context, etype, ElementLDataConstructor);
  DDD_SetHandlerDELETE          (context, etype, ElementDelete);
  DDD_SetHandlerXFERCOPY        (context, etype, ElementXferCopy);
  DDD_SetHandlerSETPRIORITY     (context, etype, ElementPriorityUpdate);

        #ifdef __THREEDIM__
  DDD_SetHandlerUPDATE          (context, etype, ElementUpdate);
        #endif


  switch (handlerSet)
  {
  /* TODO: not needed any more ??
     case HSET_XFER:
          DDD_SetHandlerOBJMKCONS(context, etype, ElementObjMkCons_Xfer);
          break;

     case HSET_REFINE:
          DDD_SetHandlerOBJMKCONS(context, etype, ElementObjMkCons_Refine);
          break;
   */
  default :
    DDD_SetHandlerOBJMKCONS(context, etype, ElementObjMkCons);
    break;
  }
}


/* init handlers for inner element */
static void IElemHandlerInit (DDD::DDDContext& context, DDD_TYPE etype, INT handlerSet)
{
  /* init standard elem handlers */
  ElemHandlerInit(context, etype, handlerSet);

  /* init additional handlers, necessary for inside management */
  DDD_SetHandlerXFERGATHER (context, etype, ElemGatherI);
  DDD_SetHandlerXFERSCATTER(context, etype, ElemScatterI);
}


/* init handlers for boundary element */
static void BElemHandlerInit (DDD::DDDContext& context, DDD_TYPE etype, INT handlerSet)
{
  /* init standard elem handlers */
  ElemHandlerInit(context, etype, handlerSet);

  /* init additional handlers, necessary for boundary management */
  DDD_SetHandlerXFERGATHER (context, etype, ElemGatherB);
  DDD_SetHandlerXFERSCATTER(context, etype, ElemScatterB);
}


/****************************************************************************/

/* init all handlers necessary for grid xfer */
void NS_DIM_PREFIX ddd_HandlerInit (DDD::DDDContext& context, INT handlerSet)
{
  auto& dddctrl = ddd_ctrl(context);

  DDD_SetHandlerUPDATE           (context, dddctrl.TypeVector, VectorUpdate);
  DDD_SetHandlerXFERCOPY         (context, dddctrl.TypeVector, VectorXferCopy);
  DDD_SetHandlerXFERGATHERX      (context, dddctrl.TypeVector, VectorGatherMatX);
  DDD_SetHandlerXFERSCATTERX     (context, dddctrl.TypeVector, VectorScatterConnX);
  DDD_SetHandlerOBJMKCONS        (context, dddctrl.TypeVector, VectorObjMkCons);
  DDD_SetHandlerSETPRIORITY      (context, dddctrl.TypeVector, VectorPriorityUpdate);
  /* TODO: not used
          DDD_SetHandlerDELETE           (context, dddctrl.TypeVector, VectorDelete);
   */

  DDD_SetHandlerUPDATE           (context, dddctrl.TypeIVertex, VertexUpdate);
  DDD_SetHandlerSETPRIORITY      (context, dddctrl.TypeIVertex, VertexPriorityUpdate);

  DDD_SetHandlerLDATACONSTRUCTOR (context, dddctrl.TypeBVertex, BVertexLDataConstructor);
  DDD_SetHandlerUPDATE           (context, dddctrl.TypeBVertex, VertexUpdate);
  DDD_SetHandlerXFERCOPY         (context, dddctrl.TypeBVertex, BVertexXferCopy);
  DDD_SetHandlerXFERGATHER       (context, dddctrl.TypeBVertex, BVertexGather);
  DDD_SetHandlerXFERSCATTER      (context, dddctrl.TypeBVertex, BVertexScatter);
  DDD_SetHandlerSETPRIORITY      (context, dddctrl.TypeBVertex, VertexPriorityUpdate);

  DDD_SetHandlerXFERGATHER       (context, dddctrl.TypeNode, DuneEntityGather<NODE>);
  DDD_SetHandlerXFERSCATTER      (context, dddctrl.TypeNode, DuneEntityScatter<NODE>);

  DDD_SetHandlerLDATACONSTRUCTOR (context, dddctrl.TypeNode, NodeObjInit);
  DDD_SetHandlerDESTRUCTOR       (context, dddctrl.TypeNode, NodeDestructor);
  DDD_SetHandlerOBJMKCONS        (context, dddctrl.TypeNode, NodeObjMkCons);
  DDD_SetHandlerUPDATE           (context, dddctrl.TypeNode, NodeUpdate);
  DDD_SetHandlerXFERCOPY         (context, dddctrl.TypeNode, NodeXferCopy);
  DDD_SetHandlerSETPRIORITY      (context, dddctrl.TypeNode, NodePriorityUpdate);



        #ifdef __TWODIM__
  IElemHandlerInit(context, dddctrl.TypeTrElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypeTrBElem, handlerSet);

  IElemHandlerInit(context, dddctrl.TypeQuElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypeQuBElem, handlerSet);
        #endif

        #ifdef __THREEDIM__
  IElemHandlerInit(context, dddctrl.TypeTeElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypeTeBElem, handlerSet);

  IElemHandlerInit(context, dddctrl.TypePyElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypePyBElem, handlerSet);

  IElemHandlerInit(context, dddctrl.TypePrElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypePrBElem, handlerSet);

  IElemHandlerInit(context, dddctrl.TypeHeElem, handlerSet);
  BElemHandlerInit(context, dddctrl.TypeHeBElem, handlerSet);
        #endif

  DDD_SetHandlerUPDATE      (context, dddctrl.TypeEdge, EdgeUpdate);
  DDD_SetHandlerOBJMKCONS   (context, dddctrl.TypeEdge, EdgeObjMkCons);
  DDD_SetHandlerXFERCOPY    (context, dddctrl.TypeEdge, EdgeXferCopy);
  DDD_SetHandlerSETPRIORITY (context, dddctrl.TypeEdge, EdgePriorityUpdate);

  DomHandlerInit(handlerSet);
}

#endif /* ModelP */
