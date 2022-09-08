// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:

/****************************************************************************/

#ifdef ModelP

#include <config.h>
#include <cstdio>

#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/low/namespace.h>

#include <dune/uggrid/parallel/ddd/include/memmgr.h>
#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

#include "debugger.h"
#include "parallel.h"

USING_UG_NAMESPACES
using namespace PPIF;



/****************************************************************************/


/****************************************************************************/
/*
   dddif_DisplayMemoryUsage -

   SYNOPSIS:
   static void dddif_DisplayMemoryUsage (void);

   PARAMETERS:
   .  void

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void dddif_DisplayMemoryUsage (const DDD::DDDContext& context)
{
  UserWriteF("mem for interfaces:  %8ld bytes\n",
             (unsigned long) DDD_IFInfoMemoryAll(context)
             );

  UserWriteF("mem for couplings:   %8ld bytes\n",
             (unsigned long) DDD_InfoCplMemory(context)
             );
}


/****************************************************************************/
/*
   ddd_pstat -

   SYNOPSIS:
   void ddd_pstat (const DDD::DDDContext& context, char *arg);

   PARAMETERS:
   .  context
   .  arg

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/


void NS_DIM_PREFIX ddd_pstat(DDD::DDDContext& context, char *arg)
{
  int cmd;

  if (arg==NULL)
    return;

  cmd = arg[0];

  switch (cmd)
  {
  case 'X' :
    dddif_PrintGridRelations(ddd_ctrl(context).currMG);
    break;

  case 'm' :
    dddif_DisplayMemoryUsage(context);
    break;

  case 'c' :
    DDD_ConsCheck(context);
    UserWrite("\n");
    break;

  case 's' :
    DDD_Status(context);
    UserWrite("\n");
    break;

  case 't' :
    if (context.isMaster())
    {
      const auto& dddctrl = ddd_ctrl(context);
      /* display ddd types */
      DDD_TypeDisplay(context, dddctrl.TypeVector);
      DDD_TypeDisplay(context, dddctrl.TypeIVertex);
      DDD_TypeDisplay(context, dddctrl.TypeBVertex);
      DDD_TypeDisplay(context, dddctrl.TypeNode);
                                #ifdef UG_DIM_3
      DDD_TypeDisplay(context, dddctrl.TypeEdge);
                                #endif

                                #ifdef UG_DIM_2
      DDD_TypeDisplay(context, dddctrl.TypeTrElem);
      DDD_TypeDisplay(context, dddctrl.TypeTrBElem);
      DDD_TypeDisplay(context, dddctrl.TypeQuElem);
      DDD_TypeDisplay(context, dddctrl.TypeQuBElem);
                                #endif

                                #ifdef UG_DIM_3
      DDD_TypeDisplay(context, dddctrl.TypeTeElem);
      DDD_TypeDisplay(context, dddctrl.TypeTeBElem);
      DDD_TypeDisplay(context, dddctrl.TypePyElem);
      DDD_TypeDisplay(context, dddctrl.TypePyBElem);
      DDD_TypeDisplay(context, dddctrl.TypePrElem);
      DDD_TypeDisplay(context, dddctrl.TypePrBElem);
      DDD_TypeDisplay(context, dddctrl.TypeHeElem);
      DDD_TypeDisplay(context, dddctrl.TypeHeBElem);
                                #endif

      /* display dependent types */
                                #ifdef UG_DIM_2
      DDD_TypeDisplay(context, dddctrl.TypeEdge);
                                #endif
    }
    break;


  case 'i' :
  {
    DDD_IF ifId = atoi(arg+1);

    if (ifId<=0)
      DDD_IFDisplayAll(context);
    else
      DDD_IFDisplay(context, ifId);

    UserWrite("\n");
  }
  break;

  case 'l' :
    DDD_ListLocalObjects(context);
    UserWrite("\n");
    break;

  case 'b' :
    buggy(ddd_ctrl(context).currMG);
    UserWrite("BUGGY: returning control to caller\n");
    break;
  }
}


/****************************************************************************/

/*
        buggy - interactive debugging tool for distributed grids ug/ddd

        960603 kb  copied from pceq, modified, integrated
 */


/****************************************************************************/


/****************************************************************************/
/*
   buggy_ShowCopies -

   SYNOPSIS:
   static void buggy_ShowCopies (DDD_HDR hdr);

   PARAMETERS:
   .  hdr -

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void buggy_ShowCopies (DDD::DDDContext& context, DDD_HDR hdr)
{
  for (auto&& [proc, prio] : DDD_InfoProcListRange(context, hdr))
  {
    printf("%4d:    copy on %3d with prio %d\n",
           context.me(), proc, prio);
  }
}



/****************************************************************************/


/****************************************************************************/
/*
   buggy_ElemShow -

   SYNOPSIS:
   static void buggy_ElemShow (ELEMENT *e);

   PARAMETERS:
   .  e -

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void buggy_ElemShow (ELEMENT *e)
{
  ELEMENT *SonList[MAX_SONS];
  int i;

  printf("    ID=%06d LEVEL=%02d corners=%03d\n",
         ID(e), LEVEL(e), CORNERS_OF_ELEM(e));

  if (EFATHER(e))
    printf("    father=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDRE(EFATHER(e))));

  if (PREDE(e))
    printf("    pred=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDRE(PREDE(e))));

  if (SUCCE(e))
    printf("    succ=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDRE(SUCCE(e))));

  for(i=0; i<SIDES_OF_ELEM(e); i++)
  {
    if (NBELEM(e,i)!=NULL)
    {
      printf("    nb[%d]=" DDD_GID_FMT "\n",
             i, DDD_InfoGlobalId(PARHDRE(NBELEM(e,i))));
    }
  }

  if (GetAllSons(e,SonList)==0)
  {
    for(i=0; SonList[i]!=NULL; i++)
    {
      printf("    son[%d]=" DDD_GID_FMT " prio=%d\n",
             i,
             DDD_InfoGlobalId(PARHDRE(SonList[i])),
             DDD_InfoPriority(PARHDRE(SonList[i]))
             );
    }
  }
}


/****************************************************************************/
/*
   buggy_NodeShow -

   SYNOPSIS:
   static void buggy_NodeShow (NODE *n);

   PARAMETERS:
   .  n -

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void buggy_NodeShow (NODE *n)
{
  int i;

  printf("    ID=%06d LEVEL=%02d\n",
         ID(n), LEVEL(n));

  /* print coordinates of that node */
  printf("    VERTEXID=%06d LEVEL=%02d",
         ID(MYVERTEX(n)), LEVEL(MYVERTEX(n)));
  for(i=0; i<DIM; i++)
  {
    printf(" x%1d=%11.4E",i, (float)(CVECT(MYVERTEX(n))[i]) );
  }
  printf("\n");


  if (NFATHER(n))
    printf("    father=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDR((NODE *)NFATHER(n))));

  if (PREDN(n))
    printf("    pred=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDR(PREDN(n))));

  if (SUCCN(n))
    printf("    succ=" DDD_GID_FMT "\n",
           DDD_InfoGlobalId(PARHDR(SUCCN(n))));
}


/****************************************************************************/
/*
   buggy_Search -

   SYNOPSIS:
   static void buggy_Search (MULTIGRID *theMG, DDD_GID gid);

   PARAMETERS:
   .  the MG -
   .  gid -

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void buggy_Search (MULTIGRID *theMG, DDD_GID gid)
{
  auto& context = theMG->dddContext();

  int level, found;

  found = false;
  for(level=0; level<=TOPLEVEL(theMG); level++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,level);
    ELEMENT *e;
    NODE    *n;

    /* search for element */
    for(e=PFIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
    {
      if (DDD_InfoGlobalId(PARHDRE(e))==gid)
      {
        printf("ELEMENT gid=" DDD_GID_FMT ", adr=%p, level=%d\n",
               gid, (void*) e, level);
        buggy_ShowCopies(context, PARHDRE(e));
        buggy_ElemShow(e);
        found = true;
      }
    }


    /* search for node */
    for(n=PFIRSTNODE(theGrid); n!=NULL; n=SUCCN(n))
    {
      if (DDD_InfoGlobalId(PARHDR(n))==gid)
      {
        printf("NODE gid=" DDD_GID_FMT ", adr=%p, level=%d\n",
               gid, (void*) n, level);
        buggy_ShowCopies(context, PARHDR(n));
        buggy_NodeShow(n);
        found = true;
      }
    }
  }

  if (!found)
  {
    DDD_HDR hdr = DDD_SearchHdr(theMG->dddContext(), gid);

    if (hdr!=NULL)
    {
      printf("DDDOBJ gid=" DDD_GID_FMT ", typ=%d, level=%d\n",
             gid, DDD_InfoType(hdr), DDD_InfoAttr(hdr));
      buggy_ShowCopies(context, hdr);
    }
    else
    {
      printf("unknown gid=" DDD_GID_FMT "\n", gid);
    }
  }
}



/****************************************************************************/

/*
   static void bug_eg_item (Elem *item)
   {
        char  fname[30];
        FILE  *f;

        sprintf(fname,"buggy%08x.%03d.gnu",item->InfoGlobalId(),me);
        f = fopen(fname,"w");
        if (f==0)
        {
                Cout<<me<<": can't open "<<fname<<cr;
                return;
        }

        {
                ConnIterator(Side) siter(item->conn_dn());
                Side *side;
                for(; siter; ++siter)
                {
                        side = siter.rel()->obj();
                        Cout << me<<" side "<< hex << side->InfoGlobalId() <<dec<< cr;
                        {
                                ConnIterator(Node) niter(side->conn_dn());
                                Node *node;
                                for(; niter; ++niter)
                                {
                                        node = niter.rel()->obj();
                                        Cout << me <<"    node " << hex
                                                 << node->InfoGlobalId()
                                                 << dec << cr;
                                        fprintf(f,"%f %f\n",node->x[0],node->x[1]);
                                }
                                fprintf(f,"\n");
                        }
                }
        }

        fclose(f);
   }


   static void bug_elem_graphics (GridSegment &target, unsigned int gid)
   {
        Elem  *item=0;

        {
                Iterator(Elem) iter(target.elem_list());

                for (; iter && item==0; ++iter)
                {
                        if (iter.obj()->InfoGlobalId()==gid)
                        {
                                item = iter.obj();
                        }
                }
        }

        if (item==0)
        {
                Iterator(Elem) iter(target.ghostlist);

                for (; iter && item==0; ++iter)
                {
                        if (iter.obj()->InfoGlobalId()==gid)
                        {
                                item = iter.obj();
                        }
                }

        }

        if (item==0)
        {
                Cout<<me<<": no current elem/ghost"<<cr;
                return;
        }

        bug_eg_item(item);
   }
 */


/****************************************************************************/
/*
   buggy_help -

   SYNOPSIS:
   static void buggy_help (void);

   PARAMETERS:
   .  void

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

/****************************************************************************/

static void buggy_help (void)
{
  printf(
    " *\n"
    " * BUGGY ug debugger\n"
    " *\n"
    " *   x or q   quit\n"
    " *   p<no>    change current processor\n"
    " *   l        list DDD objects on current proc\n"
    " *   <gid>    change to object with gid\n"
    " *   ? or h   this help message\n"
    " *\n");
}



/****************************************************************************/


/****************************************************************************/
/*
   buggy -

   SYNOPSIS:
   void buggy (MULTIGRID *theMG);

   PARAMETERS:
   .  the MG

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

void NS_DIM_PREFIX buggy (MULTIGRID *theMG)
{
  char buff[100];
  DDD_GID gid;
  int proc, cmd, received;

  Synchronize(theMG->ppifContext());

  const int me = theMG->ppifContext().me();

  if (me==0)
  {
    printf("%04d: started buggy.\n", me);
    fflush(stdout);
  }

  proc = 0;
  gid = 0x0;
  do
  {
    if (me==0)
    {
      do {
        printf("%04d: buggy> ", proc);
        fflush(stdout);
        received = scanf("%s", buff);
      } while (received > 0 && buff[0] == 0);

      switch (buff[0])
      {
      case 'x' :
      case 'q' :
        proc = -1;
        cmd = 0;
        break;

      case 'p' :
        proc = (int) strtol(buff+1, 0, 0);
        cmd = 1;
        break;

      case 'l' :
        cmd = 2;
        break;

      case '?' :
      case 'h' :
        cmd=99;
        break;

      default :
        cmd = 3;
        gid = /*(DDD_GID)*/ strtol(buff, 0, 0);
      }
    }

    Broadcast(theMG->ppifContext(), &cmd, sizeof(int));
    Broadcast(theMG->ppifContext(), &proc, sizeof(int));
    Broadcast(theMG->ppifContext(), &gid, sizeof(unsigned int));

    if (me==proc)
    {
      switch (cmd)
      {
      case 99 :
        buggy_help();
        break;

      case 2 :
        DDD_ListLocalObjects(theMG->dddContext());
        break;

      default :
        buggy_Search(theMG, gid);
        /*
                                                bug_ghost(target, gid);
                                                bug_side(target, gid);
         */
        break;
      }
    }

    fflush(stdout);
    Synchronize(theMG->ppifContext());
  }
  while (proc>=0);
}


/****************************************************************************/
/*                                                                          */
/* Function:  PrintGridRelations                                            */
/*                                                                          */
/* Purpose:   print certain information about a grid in order to test       */
/*            the formal approach to parallelisation.                       */
/*                                                                          */
/****************************************************************************/

#define PREFIX "__"

void NS_DIM_PREFIX dddif_PrintGridRelations (MULTIGRID *theMG)
{
  ELEMENT *e, *enb;
  GRID    *theGrid = GRID_ON_LEVEL(theMG,TOPLEVEL(theMG));
  INT j;

  const auto& me = theMG->dddContext().me();

  for(e=FIRSTELEMENT(theGrid); e!=NULL; e=SUCCE(e))
  {
    printf(PREFIX "master(e" EGID_FMT ", p%02d).\n", EGID(e), me);

    for(j=0; j<SIDES_OF_ELEM(e); j++)
    {
      enb = NBELEM(e,j);
      if (enb!=NULL)
      {
        printf(PREFIX "nb(e" EGID_FMT ", e" EGID_FMT ").\n", EGID(e), EGID(enb));
      }
    }
  }

}

#undef PREFIX


/****************************************************************************/



#endif /* ModelP */
