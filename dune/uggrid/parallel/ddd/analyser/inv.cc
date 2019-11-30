// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      inventor.c                                                    */
/*                                                                          */
/* Purpose:   visualization in IRIS Inventor format                         */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   95/03/28 kb  begin                                            */
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
#include <cstdio>
#include <cmath>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>


USING_UG_NAMESPACE
using namespace PPIF;

  START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

/* graph of DDD_TYPEs, with directed edges according to references */

struct TYPE_EDGE
{
  DDD_TYPE reftype;        /* referenced type */
  int n;                   /* number of references */

  TYPE_EDGE *next;         /* linked list */
};

struct TYPE_NODE
{
  const TYPE_DESC *def;          /* pointer to DDD_TYPE */
  TYPE_EDGE *refs;         /* linked list of referenced types */
};

/****************************************************************************/
/*                                                                          */
/* subroutines                                                              */
/*                                                                          */
/****************************************************************************/


static TYPE_EDGE *GetTypeEdge (TYPE_NODE *tn, DDD_TYPE reftype)
{
  TYPE_EDGE *te;

  for(te=tn->refs; te!=NULL && te->reftype!=reftype; te=te->next)
    ;

  if (te==NULL)
  {
    te = (TYPE_EDGE *)AllocTmp(sizeof(TYPE_EDGE));
    te->reftype = reftype;
    te->n = 0;

    te->next = tn->refs;
    tn->refs = te;
  }

  return(te);
}


static void AnalyseTypes(const DDD::DDDContext& context)
{
  int i;

  /* create graph of DDD_TYPE ref-relations */
  for(i=0; i<DDD_InfoTypes(context); i++)
  {
    const TYPE_DESC *td = &context.typeDefs()[i];
    TYPE_NODE tn;
    int e;

    tn.def = td;
    tn.refs = NULL;

    for(e=0; e<td->nElements; e++)
    {
      const ELEM_DESC *el = &(td->element[e]);

      if (el->type==EL_OBJPTR)
      {
        TYPE_EDGE *te = GetTypeEdge(&tn, EDESC_REFTYPE(el));
        te->n += (el->size / sizeof(void *));
      }
    }

    printf("%4d: type %s (%03d) refs:\n", context.me(), td->name, i);
    {
      TYPE_EDGE *te;
      for(te=tn.refs; te!=NULL; te=te->next)
      {
        printf("         %s (%03d), n=%d\n",
               context.typeDefs()[te->reftype].name, te->reftype, te->n);
      }
    }
  }
}



/****************************************************************************/


void DDD_GraphicalAnalyser (DDD::DDDContext& context, char *filename)
{
  FILE *fp;

  fp = fopen(filename, "w");

  if (context.isMaster())
  {
    AnalyseTypes(context);
  }

  fclose(fp);
}

END_UGDIM_NAMESPACE
