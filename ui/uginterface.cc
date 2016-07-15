// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  uginterface.c                                                                                                 */
/*																			*/
/* Purpose:   ug interface data structure manager							*/
/*																			*/
/* Author:	  Klaus Johannsen												*/
/*			  Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen	*/
/*			  Universitaet Heidelberg										*/
/*			  Im Neuenheimer Feld 368										*/
/*			  6900 Heidelberg												*/
/*																			*/
/* History:   14.06.93 begin, ug version ug21Xmas3d                                             */
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

#include <cstdio>
#include <cstring>
#include <cassert>

#include "uginterface.h"

/* low module */
#include "architecture.h"
#include "misc.h"
#include "evm.h"
#include "ugenv.h"
#include "ugdevices.h"
#include "gm.h"
#include "wpm.h"
#include "wop.h"
#include "cmdint.h"
#include "debug.h"
#include "general.h"

#include "parallel/util/xbc.h"

USING_UG_NAMESPACES

#ifdef ModelP
using namespace PPIF;
#endif

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define INTERRUPT_CHAR          '.'

#define MAXCMDLEN                       256

#define POINTER                                 0               /* arrow tool can zoom pictures		*/
#define PAN                                             1               /* arrow tool can pan pictures		*/
#define ZOOM                                    2               /* arrow tool can zoom pictures		*/
#define ROTATE                                  3               /* arrow tool can rotate pictures	*/
#define N_ARROW_FUNCS_WO_CUT    4

#define ROTATE_CUT                              4               /* arrow tool can rotate cut (iff)	*/
#define MOVE_CUT                                5               /* arrow tool can move cut (iff)	*/
#define N_ARROW_FUNCS                   6

#define SEL_NODE                                0               /* hand tool can select nodes		*/
#define SEL_VECTOR                              1               /* hand tool can select vectors		*/

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

typedef struct {

  /* fields for enironment list variable */
  ENVVAR v;

  /* cmd key specific stuff */
  char Comment[KEY_COMMENT_SIZE];               /* comment string					*/
  INT ShowBar;                                                  /* show bar before key in list		*/
  char CommandName[MAXCMDLEN];                  /* command associated with the name */

} CMDKEY;

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

static UGWINDOW *currUgWindow;
static PICTURE *currPicture;

static INT autoRefresh;                                 /* ON or OFF						*/
static INT use_bullet = NO;             /* whether auto refresh uses bullet */
static DOUBLE offset_factor = 1.0;      /* offset factor for bullet         */

static const char *ArrowToolFuncs[N_ARROW_FUNCS]={"pointer",
                                                  "pan",
                                                  "zoom",
                                                  "rotate",
                                                  "rotate cut",
                                                  "move cut"};

static INT theCmdKeyDirID;                              /* env ID for the /Cmd Key dir		*/
static INT theCmdKeyVarID;                              /* env ID for the /Cmd Key dir		*/

static INT MousePos[2]={-1,-1};
static OUTPUTDEVICE *DefaultDevice;     /* our default ouput device             */


/****************************************************************************/
/*D
   toolbox - interactive mouse tools for graphics

   DESCRIPTION:
   The toolbox appearing in a ug graphical window at the lower right corner
   offers the possibility to switch between different mouse tools to
   interactively manipulate graphics.
   The tools are (from left to right):
   .n    arrow
   .n    x
   .n    +
   .n    circle
   .n    hand
   .n    heart
   .n    gnoedel

   The tools can be disabled or can have multiple functionality. Moving the
   mouse over the toolbox the 'infobox' (to the left of the toolbox) shows
   the current functionality, its index and the number of functions for this tool.
   In general, the functionality is associated with a plot object ('PLOTOBJ'). So
   tools are only active and chooseable (by clicking on them) if the active window
   contains the current ppicture with a valid plot object.
   The multiple functionality of a tool can be switched by multiply clicking on it.

   For a description of the tools see 'arrowtool' (which is availabble for all
   plotobejcts with the same functionality) or the different plot objects
   (EScalar, EVector, Grid, Matrix, VecMat).

   KEYWORDS:
   graphics, interactive, tools, mouse

   SEE ALSO:
   'EScalar', 'EVector', 'Grid', 'Matrix', 'VecMat', 'arrowtool'
   D*/
/****************************************************************************/

/****************************************************************************/
/*D
   arrowtool - general functionality: pan, zoom, rotate, rotate cut, move cut

   DESCRIPTION:
   The arrow tool is available for all plot objects and has the following
   functionalities:~
   .     pointer		- only to make windows and pictures the current (active) ones.
                                          (All other tools and functionalities offer this possinility too.)
   .	  pan			- click into the current picture, push mouse button and move.
                                          A frame indicating the bounds of the picture will be drawn when moving.
                                          Realising the mouse button will move the picture to the current
                                          frame position.
                                          Moving the mouse outside the current picture cancels the operation.
   .     zoom			- click into the current picture, push mouse button and move.
                                          A frame is pulled indicaating the region which will be centerd and zoomed
                                          to fit the pictures borders after releasing the button.
                                          Moving the mouse outside the current picture cancels the operation.
   .     rotate		- click into the current picture, push mouse button and move.
                                          A tripod showing the coordinate axes will indiacte the rotated position
                                          (in 3D a little cube will facilitate orientation).
                                          Moving the mouse outside the current picture cancels the operation.

   .     rotate cut	- this is possible only with 3D plot objects which offer the possibilty
                                          of a cutting plane. Everything works similar to 'rotate'
   .     move cut		- this is possible only with 3D plot objects which offer the possibilty
                                          of a cutting plane. At the lower part of the picture a ruler will
                                          aopear on mouse click into the current picture. A cross is indicating
                                          the current cut position in units of the objects bounding sphere.
                                          A tick mark can be moved by horizontal mouse movement.
                                          Moving the mouse outside the current picture cancels the operation.

   KEYWORDS:
   graphics, interactive, tools, mouse, rotate, pan, zoom

   SEE ALSO:
   'EScalar', 'EVector', 'Grid', 'Matrix', 'VecMat', 'toolbox'
   D*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* Function:  DoCmdKey														*/
/*																			*/
/* Purpose:   read out command string for command key						*/
/*																			*/
/* Input:	  char c: command key to read out								*/
/*			  char String: returned command string							*/
/*																			*/
/* Output:	  0: ok                                                                                                                 */
/*			  1: error														*/
/*																			*/
/****************************************************************************/

static INT DoCmdKey (char c, char *String)
{
  CMDKEY *theCmdKey;
  char theCmdKeyName[2],*s;

  /* find cmd key env item */
  theCmdKeyName[0] = c;
  theCmdKeyName[1] = '\0';
  theCmdKey = (CMDKEY*) SearchEnv(theCmdKeyName,"/Cmd Keys",theCmdKeyVarID,theCmdKeyDirID);
  if (theCmdKey != NULL)
  {
    strcpy(String, (const char *)theCmdKey->CommandName);
    for (s=String; *s!='\0'; s++)
      if (*s=='?')
        *s = '@';
    return (1);
  }
  return (0);
}

/****************************************************************************/
/*D
        ProcessEvent - the event handler of ug

        SYNOPSIS:
        static INT ProcessEvent (char *String, INT EventMask)

        PARAMETERS:
   .   String - to store string got from TERM_STRING - event
   .   EventMask - specifies which events will be returned by 'GetNextUGEvent'
                possible values are EVERY_EVENT, TERM_STRING and TERM_CMDKEY

        DESCRIPTION:
        ProcessEvent is the event handler of ug. The event types it can handle
        and which are returned bye the interface function 'GetNextUGEvent' are':'

   .vb
   #define EVENT_ERROR                  0

   #define NO_EVENT				2
   #define TERM_GOAWAY                  3
   #define TERM_CMDKEY                  4
   #define TERM_STRING                  5
   #define DOC_GOAWAY				6
   #define DOC_ACTIVATE			7
   #define DOC_DRAG				8
   #define DOC_GROW				9
   #define DOC_CHANGETOOL			10
   #define DOC_CONTENTCLICK		11
   #define DOC_UPDATE				12
   .ve

   .   EVENT_ERROR              - an error occured in the interface function 'GetNextUGEvent'
   .   NO_EVENT			- no ug event (but may bay something to do for the output device
   .   TERM_GOAWAY              - close the shell window (and force ug to quit)
   .   TERM_CMDKEY              - a command key was typed
   .   TERM_STRING              - a command sequence typed into the shell was completed by typing <cr>
   .   DOC_GOAWAY			- close a ug graphics window
   .   DOC_ACTIVATE		- the first mouse click into a ug window produces an activate event
                                                        (and makes the corresponding window the current window)
   .   DOC_DRAG			- drag a ug graphics window
   .   DOC_GROW			- resize a ug graphics window (if the size changes in both directions
                                                        the pictures of that window will be resized accordingly; if it was
                                                        only one direction the scale pictures will not be changed)
   .   DOC_CHANGETOOL		- change the current mouse tool on the toolbar
   .   DOC_CONTENTCLICK	- a mouse click into a ug graphics window was encountered (the
                                                        action executed depends on the current mouse tool --> 'toolbar')
   .   DOC_UPDATE			- update event for a ug graphics window was encountered (if
                                                        the refresh state is on pictures will be reploted otherwise they will
                                                        just be invalidated)

        Always the last command string encountered from the shell is stored in the special command key "!"

        RETURN VALUE:
        INT
   .n   PE_STRING: command string encounterd from the shell
   .n   PE_INTERRUPT: user interrupt encountered (command key ".")
   .n   PE_NOTHING1: actually not of further interest
   .n   PE_NOTHING2: actually not of further interest
   .n   PE_OTHER: any other event occured (which is not of further interest)
   .n   PE_ERROR: an error occured

        SEE ALSO:
        UGWINDOW, PICTURE
   D*/
/****************************************************************************/

#define PE_STRING               0
#define PE_OTHER                1
#define PE_NOTHING1     2               /* Interface event: yes */
#define PE_NOTHING2     3               /* Interface event: no	*/
#define PE_INTERRUPT    4
#define PE_ERROR                5

static INT ProcessEvent (char *String, INT EventMask)
{
  EVENT theEvent;
  DOUBLE qw, qh, scaling;
  UGWINDOW *theUgW;
  PICTURE *thePic;
  WINDOWID WinID;
  INT UGW_LLL_old[2], UGW_LUR_old[2], Offset[2], nfct, r;
  static INT MousePosition[2];

#ifdef ModelP
  if (me==master)
#endif
  r=GetNextUGEvent(&theEvent,EventMask);
#ifdef ModelP
  XBroadcast(2, &theEvent, sizeof(theEvent), &r, sizeof(r));
#endif
  if (r) return PE_ERROR;

  switch (EVENT_TYPE(theEvent))
  {
  case NO_EVENT :
    if (!(EventMask&PE_INTERRUPT))
    {
      /* update infobox */
#ifdef ModelP
      if (me == master)
#endif
      /* do current work (not if UserInterrupt is calling) */
      for (theUgW=GetFirstUgWindow(); theUgW!=NULL; theUgW=GetNextUgWindow(theUgW))
      {
        if (UGW_VALID(theUgW)==NO) if (UpdateUgWindow(theUgW,currPicture)) return (PE_OTHER);
      }
    }

    if (theEvent.NoEvent.InterfaceEvent) return (PE_NOTHING1);
    return (PE_NOTHING2);
    break;
  case TERM_GOAWAY :
    /* tell interpreter to execute quit command */
    theEvent.Type = TERM_STRING;
    strcpy(String,"quit");
    break;
  case TERM_CMDKEY :
    if (DoCmdKey(theEvent.TermCmdKey.CmdKey, String))
    {
      theEvent.Type = TERM_STRING;
      strcpy(theEvent.TermString.String,(const char *)String);
      UserWrite(String);
      UserWrite("\n");
    }
    else if (theEvent.TermCmdKey.CmdKey==INTERRUPT_CHAR)
      return (PE_INTERRUPT);
  }

  /* return */
  if (EVENT_TYPE(theEvent) == TERM_STRING)
    return (PE_STRING);

  return (PE_OTHER);
}

/****************************************************************************/
/*D
        UserInterrupt - check whether a user interrupt event was encounterd

        SYNOPSIS:
        INT UserInterrupt (const char *text)

        PARAMETERS:
   .   text - if an interrupt event was found and if 'text==NULL' 'YES' will be returned
                        otherwise a promt "### user-interrupt in <text>?" will be prompted and 'YES'
                        will be returned only if a 'y' was entered into the shell

        DESCRIPTION:
        Check whether a user interrupt event was encounterd and return 'YES' or 'NO' correspondingly.
        If yes the mutelevel is set to 0 if it was < 0.

        RETURN VALUE:
        INT
   .n   YES: a user interrupt was encountered
   .n   NO:  no interrupt
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX UserInterrupt (const char *text)
{
  INT Code,EventMask,mutelevel;
  char buffer[128];

#ifndef ModelP
  EventMask = TERM_CMDKEY;

  Code = ProcessEvent(buffer,EventMask);

  if (Code==PE_INTERRUPT)
  {
    if (text==NULL)
    {
      return (YES);
    }
    else
    {
      mutelevel = GetMuteLevel();
      if (GetMuteLevel()<0)
        SetMuteLevel(0);
      UserWriteF("### user-interrupt in '%s'?",text);
      UserRead(buffer);
      if (buffer[0]=='y')
      {
        return (YES);
      }
      else
      {
        SetMuteLevel(mutelevel);
        return (NO);
      }
    }
  }
#endif
  return (NO);
}

/****************************************************************************/
/*                                                                          */
/* Function:  ParExecCommand                                                */
/*                                                                          */
/* Purpose:   Broadcast a command line string to all processors, execute    */
/*            command on each processor and collect global status after     */
/*            termination.                                                  */
/*                                                                          */
/* Input:     pointer to the command line string                            */
/*                                                                          */
/* Output:    maximum of all return values on the different processors      */
/*                                                                          */
/****************************************************************************/

#ifdef ModelP

int NS_DIM_PREFIX ParExecCommand (char *s)
{
  int error;
  int l,n;

  PRINTDEBUG(ui,4,("%d: ParExecCommand(%.30s)...\n",me,s))

  /* broadcast command line to all processors */
  PRINTDEBUG(ui,4,("%d:         Broadcast(%.30s)...\n",me,s))
  s[cmdintbufsize-1] = (char) 0;
  if (me == 0) n = strlen(s);
  Broadcast(&n,sizeof(int));
  PRINTDEBUG(ui,4,("%d: strlen s %d\n",me,n));
  Broadcast(s,n+1);

  /* execute command on each processor */
  PRINTDEBUG(ui,4,("%d:         ExecCommand(%.30s)...\n",me,s))
  error = ExecCommand(s);

  /* collect result code */
  PRINTDEBUG(ui,4,("%d:         (Get)Concentrate(%.30s)...\n",me,s))
  for (l=degree-1; l>=0; l--)
  {
    GetConcentrate(l,&n,sizeof(int));
    error = MAX(error,n);
  }
  Concentrate(&error,sizeof(int));

  /* fanout error code */
  PRINTDEBUG(ui,4,("%d:         Broadcast(%d)...\n",me,error))
  Broadcast(&error,sizeof(int));

  PRINTDEBUG(ui,4,("%d: ...end ParExecCommand(%.30s)...\n",me,s))

  /* return global status */
  return(error);
}

#endif

/****************************************************************************/
/*D
        UserIn - call process event until a string is enterd into the shell and return
                                the string (`all` events will be handled in the meantime)

        SYNOPSIS:
        INT UserIn (char *String)

        PARAMETERS:
   .   String - store shell string to be entered here

        DESCRIPTION:
        Call process event until a string is enterd into the shell and return
        the string (`all` events will be handled in the meantime, i.e it is possible
        for example to resize a graphics window etc.). It is called by the --> 'CommandLoop'.

        RETURN VALUE:
        INT
   .n   1: a process event error occured
   .n   0: ok
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX UserIn (char *String)
{
  INT Code,EventMask;

  EventMask = EVERY_EVENT;

  /* loop till string is entered */
  while (true)
  {
    Code = ProcessEvent(String,EventMask);
#ifdef ModelP
    Broadcast(&Code, sizeof(Code));
#endif
    if (Code == PE_ERROR) return (1);
    if (Code == PE_STRING) {
#ifdef ModelP
      if (me==master)
#endif
      WriteLogFile(String);
      return 0;
    }
  }
}

/****************************************************************************/
/*D
        UserRead - call process event until a string is enterd into the shell and return
                                the string (only 'TERM_STRING' events will be handled in the meantime)

        SYNOPSIS:
        INT UserRead (char *String)

        PARAMETERS:
   .   String - store shell string to be entered here

        DESCRIPTION:
        Call process event until a string is enterd into the shell and return
        the string (only 'TERM_STRING' events will be handled in the meantime, i.e it is possible
        for example to resize a graphics window etc.). It is called by --> 'InterpretString'
        and by 'UserInterrupt'.

        RETURN VALUE:
        INT
   .n   1: a process event error occured
   .n   0: ok
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX UserRead (char *String)
{
  INT Code,EventMask;

  EventMask = TERM_STRING;

  /* loop till string is entered */
  while (true)
  {
    Code = ProcessEvent(String,EventMask);
#ifdef ModelP
    Broadcast(&Code, sizeof(Code));
#endif
    ASSERT(Code!=PE_ERROR);
    if (Code == PE_ERROR) return (1);
    if (Code == PE_STRING) {
#ifdef ModelP
      if (me==master)
#endif
      WriteLogFile(String);
      return (0);
    }
  }
}

/****************************************************************************/
/*D
        InitUgInterface - initialize 'uginterface.c'

        SYNOPSIS:
        INT InitUgInterface ()

        PARAMETERS:
        --

        DESCRIPTION:
        The command key environment directory '/Cmd Keys' is created and the default
        output device is set locally.

        RETURN VALUE:
        INT
   .n   '__LINE__': failed to create the '/Cmd Keys' dir or the DefaultDevice is 'NULL'
   .n   0: ok

   D*/
/****************************************************************************/

INT NS_DIM_PREFIX InitUgInterface ()
{
  /* install the /Cmd Keys directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitUgInterface","could not changedir to root");
    return(__LINE__);
  }
  theCmdKeyDirID = GetNewEnvDirID();
  if (MakeEnvItem("Cmd Keys",theCmdKeyDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitUgInterface","could not install '/Cmd Keys' dir");
    return(__LINE__);
  }
  theCmdKeyVarID = GetNewEnvVarID();

  DefaultDevice = GetDefaultOutputDevice();

  return (0);
}
