// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ugdevices.c                                                   */
/*                                                                          */
/* Purpose:   Initialization and hardware independent part of devices       */
/*                                                                          */
/* Author:    Peter Bastian/Klaus Johannsen                                 */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/* email:     ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*                                                                          */
/* Remarks:   was "devices.c" in earlier versions of UG                     */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*			  system include files											*/
/*			  application include files                                                                     */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdarg.h>
#include <cassert>

/* low module */
#include <dune/uggrid/low/fileopen.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/ugtypes.h>

/* dev module */
#include <dune/uggrid/ugdevices.h>

/* dddif module */
#ifdef ModelP
#include <dune/uggrid/parallel/ppif/ppif.h>
using namespace PPIF;
#endif

USING_UG_NAMESPACE

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/* the mute level is set by the MuteCommand and used for output control.
   convention: 0 is default, <0 produces less output, >0 produces more output.
   The default is -1001, which equals total silence. */
static INT mutelevel=-1001;

static FILE *logFile=NULL;                                              /* log file pointer             */

/****************************************************************************/
/*D
   OpenLogFile - open a log file

   SYNOPSIS:
   INT OpenLogFile (const char *name, int rename);

   PARAMETERS:
   .  name -
   .  rename -

   DESCRIPTION:
   This function opens a log file where all output to 'UserWrite', 'UserWriteF'
   and 'PrintErrorMessage' is protocoled.

   RETURN VALUE:
   INT
   .n    0 if operation ok
   .n    1 if a file is already open
   .n    2 if file open failed.
   D*/
/****************************************************************************/

INT NS_PREFIX OpenLogFile (const char *name, int rename)
{
  char logpath[256];

  if (logFile!=NULL) return(1);

  /* get path to logfile directory */
  logFile = FileOpenUsingSearchPath_r(name,"w",logpath,rename);

  if (logFile==NULL) return(2);

  return(0);
}

/****************************************************************************/
/*D
   CloseLogFile	- Close log file

   SYNOPSIS:
   INT CloseLogFile (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function closes the log file.

   RETURN VALUE:
   INT
   .n    0 if operation ok
   .n    1 if an error occurred.
   D*/
/****************************************************************************/

INT NS_PREFIX CloseLogFile (void)
{
  if (logFile==NULL) return(1);

  fclose(logFile);
  logFile = NULL;
  return(0);
}

/****************************************************************************/
/*D
   SetLogFile - set log file ptr

   SYNOPSIS:
   INT SetLogFile (FILE *file);

   PARAMETERS:
   .  file -

   DESCRIPTION:
   This function et log file ptr.

   RETURN VALUE:
   INT
   .n    0 if operation ok
   .n    1 if an error occurred.
   D*/
/****************************************************************************/

INT NS_PREFIX SetLogFile (FILE *file)
{
  logFile = file;
  return(0);
}

/****************************************************************************/
/*D
   WriteLogFile	- Write to (open) log file

   SYNOPSIS:
   INT WriteLogFile (const char *text);

   PARAMETERS:
   .  text -

   DESCRIPTION:
   This function writes to (open) log file.

   RETURN VALUE:
   INT
   .n    0 if operation ok
   .n    1 if error occurred.
   D*/
/****************************************************************************/

INT NS_PREFIX WriteLogFile (const char *text)
{
  if (logFile==NULL) return(1);

  if ( fputs(text,logFile) < 0 )
  {
    UserWrite( "ERROR in writing logfile\n" );
                #ifdef Debug
    printf( "ERROR in writing logfile\n" );
    fflush(logFile);
                #endif
    return 1;
  }
        #ifdef Debug
  fflush(logFile);
        #endif

  return(0);
}

/****************************************************************************/
/*D
   UserWrite - Write a string to shell window

   SYNOPSIS:
   void UserWrite (const char *s);

   PARAMETERS:
   .  s -

   DESCRIPTION:
   This function writes a string to shell window with log file mechanism.

   RETURN VALUE:
   void
   D*/
/****************************************************************************/

void NS_PREFIX UserWrite (const char *s)
{
  if (mutelevel>-1000)
    printf("%s", s);
  if (logFile!=NULL) {
    if ( fputs(s,logFile) < 0 )
    {
      UserWrite( "ERROR in writing logfile\n" );
                        #ifdef Debug
      printf( "ERROR in writing logfile\n" );
      fflush(logFile);
                        #endif
    }
                #ifdef Debug
    fflush(logFile);
                #endif
  }
}

/****************************************************************************/
/*D
   UserWriteF - write a formatted string to shell window

   SYNOPSIS:
   int UserWriteF (const char *format, ...);

   PARAMETERS:
   .  format -
   .  ... - list of arguments for format string

   DESCRIPTION:
   This function writes a formatted string to shell
   window with log file  mechanism.

   RETURN VALUE:
   int
   .n    0 if ok
   .n    1 if error occurred.
   D*/
/****************************************************************************/

#define VAR_ARG_BUFLEN 512

int NS_PREFIX UserWriteF (const char *format, ...)
{
  char buffer[VAR_ARG_BUFLEN];
  va_list args;

  /* initialize args */
  va_start(args,format);
                #ifndef NDEBUG
  int count = vsnprintf(buffer,VAR_ARG_BUFLEN,format,args);
  assert(count<VAR_ARG_BUFLEN-1);
                #endif
  if (mutelevel>-1000)
    printf("%s", buffer);

  if (logFile!=NULL) {
    if ( fputs(buffer,logFile) < 0 )
    {
      UserWrite( "ERROR in writing logfile\n" );
                        #ifdef Debug
      printf( "ERROR in writing logfile\n" );
      fflush(logFile);
                        #endif
      va_end(args);
      return 1;
    }
                #ifdef Debug
    fflush(logFile);
                #endif
  }

  /* garbage collection */
  va_end(args);

  return (0);
}

/****************************************************************************/
/** \brief Formatted error output (also to log file)
 *
 * @param   type - 'W','E','F'
 * @param   procName - name  of procedure where error occurred
 * @param   text -  additional explanation
 *
 * This function formats error output (also to log file).
 *
 * \sa PrintErrorMessageF
 */
/****************************************************************************/

void NS_PREFIX PrintErrorMessage (char type, const char *procName, const char *text)
{
  char classText[32];
  INT oldmutelevel;

  oldmutelevel = mutelevel;
  switch (type)
  {
  case 'W' :
    strcpy(classText,"WARNING");
    break;

  case 'E' :
    strcpy(classText,"ERROR");
    mutelevel = 0;
    break;

  case 'F' :
    strcpy(classText,"FATAL");
    mutelevel = 0;
    break;

  default :
    strcpy(classText,"USERERROR");
    break;
  }
  UserWriteF("%s in %.20s: %.200s\n",classText,procName,text);
  mutelevel = oldmutelevel;
}

/****************************************************************************/
/*D
   PrintErrorMessageF - Formatted error output with fotmatted message (also to log file)

   SYNOPSIS:
   void PrintErrorMessageF (char type, const char *procName, const char *format, ...);

   PARAMETERS:
 * @param   type - 'W','E','F'
 * @param   procName - name  of procedure where error occurred
 * @param   format -  additional formatted explanation (like printf)

   DESCRIPTION:
   This function formats error output (also to log file).
   After expanding message 'PrintErrorMessage' is called.

   RETURN VALUE:
   void

 * \sa PrintErrorMessage
   D*/
/****************************************************************************/

void NS_PREFIX PrintErrorMessageF (char type, const char *procName, const char *format, ...)
{
  char buffer[256];
  va_list args;

  /* initialize args */
  va_start(args,format);

  vsnprintf(buffer,256,format,args);

  PrintErrorMessage(type,procName,buffer);

  /* garbage collection */
  va_end(args);
}

/****************************************************************************/
/*D
   InitDevices - Initialize all devices at startup

   SYNOPSIS:
   INT InitDevices();

   DESCRIPTION:
   This function initializes all devices at startup.
   It must be extended when an output device is added.

   RETURN VALUE:
   INT
   .n    0 if ok
   .n    1 if some error occurred.
   D*/
/****************************************************************************/


INT NS_PREFIX InitDevices()
{
  return 0;
}



INT NS_PREFIX ExitDevices (void)
{
  return(0);         /* no error */
}
