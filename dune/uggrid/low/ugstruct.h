// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          ugstruct.h                                                                                                    */
/*                                                                                                                                                      */
/* Purpose:   implements hierachically structured string variables                      */
/*                                                                                                                                                      */
/* Author:        Nikolas Neuss                                                                                                 */
/*                        Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*                        Universitaet Heidelberg                                                                               */
/*                        Im Neuenheimer Feld 368                                                                               */
/*                        6900 Heidelberg                                                                                               */
/*                        internet: ug@ica3.uni-stuttgart.de                                    */
/*                                                                                                                                                      */
/*                                                                                                                                                      */
/* History:   18.02.92 begin, ug version 2.0                                                            */
/*                        05 Sep 1992, split cmd.c into cmdint.c and commands.c                 */
/*                        17.12.94 ug 3.0                                                                                               */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __UGSTRUCT__
#define __UGSTRUCT__

#include "ugtypes.h"
#include "ugenv.h"

#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

#define STRUCTSEP ":"
#define STRUCTSEPC ':'

enum SV_NOTIFY
{
  SV_ERROR,
  SV_CREATED,
  SV_CHANGED,
  SV_NOT_CHANGED
};

/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/

typedef struct {                                /* string variable                                                      */
  ENVVAR v;                                             /* this is an environment variable                      */
  INT length;                                   /* bytes allocated for the string                       */
  char s[1];                                            /* allocated as needed                                          */
} STRVAR ;

/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

/* hierarchical string directory */
INT      MakeStruct                             (const char *name);
INT      DeleteStruct                           (const char *name);
INT      DeleteVariable                         (const char *name);
ENVDIR  *ChangeStructDir                        (const char *s);
ENVDIR  *FindStructDir                          (const char *name, const char **lastnameHnd);
STRVAR  *FindStringVar                          (const ENVDIR *where, const char *name);
ENVDIR  *FindStructure                          (const ENVDIR *where, const char *name);
INT              RemoveStringVar                        (ENVDIR *homeDir, STRVAR *theVar);
INT      SetStringVar                           (const char *name, const char *sval);
INT              SetStringVarNotify                     (const char *name, const char *sval);
INT      SetnStringVar                          (const char *name, const char *sval, int n);
char    *GetStringVar                           (const char *name);
INT              GetStringValue                         (const char *name, double *value);
INT      GetStringValueDouble           (const char *name, double *value);
INT      GetStringValueInt                      (const char *name, int *value);
INT      SetStringValue                         (const char *name, double value);
ENVDIR  *GetCurrentStructDir            (void);
INT      GetStructPathName                      (char *s, int n);
ENVITEM *MakeStructItem                         (ENVDIR *where, const char *name, INT type, INT size);
INT      CheckStructTree                        (const ENVDIR *theDir);
INT      CheckIfInStructPath            (const ENVDIR *theDir);
INT      RemoveStructTree                       (ENVDIR *homeDir, ENVDIR *theDir);
INT      PrintStructContents            (const char *name, char *buffer, int bufLen, int ropt);
INT              PrintCurrentStructContents (int flag, char *buffer, int bufLen, int ropt);

/* initialization of this module */
INT     InitUgStruct                            (void);

END_UG_NAMESPACE

#endif
