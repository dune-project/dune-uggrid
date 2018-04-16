// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ifcheck.c                                                     */
/*                                                                          */
/* Purpose:   routines concerning interfaces between processors             */
/*            checking routines                                             */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70550 Stuttgart                                               */
/*            email: birken@ica3.uni-stuttgart.de                           */
/*                                                                          */
/* History:   960926 kb  begin                                              */
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

#include "dddi.h"
#include "if.h"
#include "basic/notify.h"

using namespace PPIF;

/* general error string */
#define ERRSTR "    DDD-IFC Warning: "

START_UGDIM_NAMESPACE

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



static int DDD_CheckInterface(DDD::DDDContext& context, DDD_IF ifId)
{
  auto& theIF = context.ifCreateContext().theIf;

  int errors=0;
  IF_PROC *h;
  NOTIFY_DESC *msgs = DDD_NotifyBegin(context, theIF[ifId].nIfHeads);
  int nRecvs, k;

  /* fill NOTIFY_DESCS */
  k=0;
  ForIF(context, ifId, h)
  {
    msgs[k].proc = h->proc;
    msgs[k].size = h->nItems;
    k++;
  }

  nRecvs = DDD_Notify(context);
  if (nRecvs==ERROR)
  {
    sprintf(cBuffer, "Notify failed on proc %d\n", me);
    DDD_PrintLine(cBuffer);
    errors++;
  }
  else
  {
    if (nRecvs!=theIF[ifId].nIfHeads)
    {
      sprintf(cBuffer, ERRSTR "IF %02d not symmetric on proc %d (%d!=%d)\n",
              ifId, me, nRecvs, theIF[ifId].nIfHeads);
      DDD_PrintLine(cBuffer);
      errors++;
    }

    ForIF(context, ifId, h)
    {
      for(k=0; k<nRecvs; k++)
      {
        if (msgs[k].proc==h->proc)
        {
          if (msgs[k].size!=h->nItems)
          {
            sprintf(cBuffer, ERRSTR
                    "IF %02d proc %d->%d has non-symmetric items (%d!=%d)\n",
                    ifId, me, msgs[k].proc, h->nItems, msgs[k].size);
            DDD_PrintLine(cBuffer);
            errors++;
          }
        }
      }
    }
  }

  DDD_NotifyEnd(context);
  return(errors);
}


/****************************************************************************/


int DDD_CheckInterfaces(DDD::DDDContext& context)
{
  const auto& nIFs = context.ifCreateContext().nIfs;

  int errors = 0;
  for(int i = 0; i < nIFs; ++i)
  {
    errors += DDD_CheckInterface(context, i);
  }

  return(errors);
}

/****************************************************************************/

END_UGDIM_NAMESPACE
