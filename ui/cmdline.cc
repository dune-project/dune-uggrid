// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  cmdline.c                                                                                                     */
/*																			*/
/* Purpose:   command structure and execution								*/
/*																			*/
/* Author:	  Peter Bastian                                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   17.12.94 begin, ug version 3.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>

#include "ugenv.h"
#include "cmdline.h"
#include <dev/ugdevices.h>

USING_UG_NAMESPACES

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static INT theMenuDirID;                        /* env type for Menu dir				*/
static INT theCommandVarID;             /* env type for Command vars			*/

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/** \brief Register the commands exported by this module in the environ

   \param name - Name of the command
   \param cmdProc - Pointer to a function of type 'CommandProcPtr'

   This function registers a new command that can be executed from the UG
   shell. This process is described in detail on the page 'commands'.

   \sa
   'commands'.

   \return <ul>
   <li> pointer to new 'COMMAND' structure if o.k </li>
   <li> NULL pointer in case of an error </li>
   </ul>
 */
/****************************************************************************/

COMMAND * NS_DIM_PREFIX CreateCommand (const char *name, CommandProcPtr cmdProc)
{
  COMMAND *newCommand;

  /* change to Menu directory */
  if (ChangeEnvDir("/Menu")==NULL)
    return (NULL);

  /* allocate structure */
  newCommand = (COMMAND *) MakeEnvItem(name,theCommandVarID,sizeof(COMMAND));
  if (newCommand==NULL) return(NULL);

  /* fill data */
  newCommand->cmdProc = cmdProc;

  return(newCommand);
}

INT NS_DIM_PREFIX InitCmdline ()
{
  /* install the /Menu directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitCmdline","could not changedir to root");
    return(__LINE__);
  }
  theMenuDirID = GetNewEnvDirID();
  if (MakeEnvItem("Menu",theMenuDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitCmdline","could not install '/Menu' dir");
    return(__LINE__);
  }
  theCommandVarID = GetNewEnvVarID();

  return (0);
}
