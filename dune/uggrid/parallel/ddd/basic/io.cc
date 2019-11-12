// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      io.c                                                              */
/*                                                                          */
/* Purpose:   routines for I/O used by DDD                                      */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*                                                                          */
/* History:   92/01/29 PrintErrorMessage by Peter Bastian                   */
/*            95/03/21 kb  added PrintString()                              */
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

#include <config.h>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/low/ugtypes.h>
#include <dune/uggrid/parallel/ppif/ppif.h>
#include <dune/uggrid/parallel/ddd/include/dddio.h>
#include <dune/uggrid/parallel/ddd/dddi.h>

/* PPIF namespace: */
using namespace PPIF;

namespace DDD {

void (*DDD_UserLineOutFunction)(const char *s);



/****************************************************************************/
/*                                                                          */
/* definition of static variables                                           */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrintLine                                                 */
/*                                                                          */
/* Purpose:   print interface, all output lines will be routed to here      */
/*                                                                          */
/* Input:     char *s: string to be printed                                 */
/*                                                                          */
/* Output:    none                                                              */
/*                                                                          */
/****************************************************************************/

void DDD_PrintLine (const char *s)
{
  /* newline character will be included in s */

  if (DDD_UserLineOutFunction!=NULL)
  {
    DDD_UserLineOutFunction(s);
  }
  else
  {
    printf("%s", s);
  }
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_Flush                                                     */
/*                                                                          */
/* Purpose:   flush output device                                           */
/*                                                                          */
/* Input:     none                                                          */
/*                                                                          */
/* Output:    none                                                              */
/*                                                                          */
/****************************************************************************/

void DDD_Flush (void)
{
  fflush(stdout);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_SyncAll                                                   */
/*                                                                          */
/* Purpose:   flush output devices and synchronize                          */
/*                                                                          */
/* Input:     none                                                          */
/*                                                                          */
/* Output:    none                                                              */
/*                                                                          */
/****************************************************************************/

void DDD_SyncAll(const DDD::DDDContext& context)
{
  DDD_Flush();
  Synchronize(context.ppifContext());
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrintDebug                                                */
/*                                                                          */
/* Purpose:   print interface for debug output                              */
/*                                                                          */
/* Input:     char *s: string to be printed                                 */
/*                                                                          */
/* Output:    none                                                              */
/*                                                                          */
/****************************************************************************/

void DDD_PrintDebug (const char *s)
{
  /* newline character will be included in s */

  DDD_PrintLine(s);

  DDD_Flush();
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_PrintError                                                */
/*                                                                          */
/* Purpose:   print formatted error message on user screen                  */
/*                                                                          */
/* Input:     char class: 'W' Warning, 'E' Error, 'F' Fatal                 */
/*            int errno: error number                                       */
/*            char *text: error message text                                */
/*                                                                          */
/* Output:    none                                                              */
/*                                                                          */
/****************************************************************************/

void DDD_PrintError (char error_class, int error_no, const char *text)
{
  char buffer[256];
  const char* classText;

  switch (error_class)
  {
  case 'W' :
    classText = "WARNING";
    break;

  case 'E' :
    classText = "ERROR";
    break;

  case 'F' :
    classText = "FATAL";
    break;

  default :
    classText = "USER";
    break;
  }
  sprintf(buffer,"DDD %s %05d: %s\n",classText,error_no,text);
  DDD_PrintLine(buffer);
}

} /* namespace DDD */
