// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      misc.c                                                        */
/*                                                                          */
/* Purpose:   miscellaneous routines                                        */
/*                                                                          */
/* Author:      Klaus Johannsen                                             */
/*              Institut fuer Computeranwendungen                           */
/*              Universitaet Stuttgart                                      */
/*              Pfaffenwaldring 27                                          */
/*              70569 Stuttgart                                             */
/*            internet: ug@ica3.uni-stuttgart.de                            */
/*                                                                          */
/* History:   08.12.94 begin, ug3-version                                   */
/*                                                                          */
/* Revision:  07.09.95                                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*              system include files                                        */
/*              application include files                                   */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cstddef>
#include <cstdio>
#include <climits>

#include "ugtypes.h"
#include "architecture.h"
#include "misc.h"
#include "heaps.h"

USING_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*          compile time constants defining static data size (i.e. arrays)  */
/*          other constants                                                 */
/*          macros                                                          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/*          in the corresponding include file!)                             */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

#ifndef ModelP
int PPIF::me = 0;              /* to have in the serial case this variable as a dummy */
int PPIF::master = 0;          /* to have in the serial case this variable as a dummy */
int PPIF::procs = 1;           /* to have in the serial case this variable as a dummy */
int NS_PREFIX _proclist_ = -1; /* to have in the serial case this variable as a dummy */
int NS_PREFIX _partition_ = 0; /* to have in the serial case this variable as a dummy */
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****                                                                    ****/
/****        general routines                                            ****/
/****                                                                    ****/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#define FMTBUFFSIZE         1031

static char newfmt[FMTBUFFSIZE];

/****************************************************************************/
/** \brief Expand (make explicit) charset-ranges in scanf

   This function expands (make explicit) charset-ranges in scanf.
   For example '%<number>[...a-d...]' --> '%<number>[...abcd...]'.

   \return
   .n        new pointer to char
 */
/****************************************************************************/
char * NS_PREFIX expandfmt (const char *fmt)
{
  const char *pos;
  char *newpos;
  char leftchar,rightchar;
  int newlen;

  /* calculate min size of newfmt */
  newlen = strlen(fmt);
  assert (newlen<FMTBUFFSIZE-1);

  pos    = fmt;
  newpos = newfmt;

  /* scan fmt for %<number>[ */
  while (*pos!='\0')
  {
    /* copy til '%' */
    while (*pos!='%' && *pos!='\0')
      *(newpos++) = *(pos++);

    if (*pos=='\0')
      break;

    *(newpos++) = *(pos++);

    /* copy til !isdigit */
    while (isdigit(*pos) && *pos!='\0')
      *(newpos++) = *(pos++);

    if (*pos=='\0')
      break;

    if (*pos!='[')
      continue;

    *(newpos++) = *(pos++);

    /* ']' following '[' is included in the charset */
    if ((*pos)==']')
      *(newpos++) = *(pos++);

    /* '^]' following '[' is included in the charset */
    else if ((*pos)=='^' && (*(pos+1))==']')
    {
      *(newpos++) = *(pos++);
      *(newpos++) = *(pos++);
    }

    while (*pos!=']' && *pos!='\0')
    {
      /* now we are inside the charset '[...]': */

      /* treat character ranges indicated by '-' */
      while (*pos!='-' && *pos!=']' && *pos!='\0')
        *(newpos++) = *(pos++);

      if (*pos=='\0')
        break;

      if ((*pos)==']')
        continue;

      /* gotya: is left char < right char? */
      leftchar  = *(pos-1);
      rightchar = *(pos+1);

      if (leftchar=='[' || rightchar==']')
      {
        *(newpos++) = *(pos++);
        continue;
      }

      if (leftchar>=rightchar)
      {
        *(newpos++) = *(pos++);
        continue;
      }

      if (leftchar+1==rightchar)
      {
        /* for example '...b-c...' */
        pos++;
        continue;
      }

      /* calc new size and expand range */
      newlen += rightchar-leftchar-2;
      assert (newlen<FMTBUFFSIZE-1);

      leftchar++;
      pos++;

      while (leftchar<rightchar)
      {
        if (leftchar=='^' || leftchar==']')
        {
          leftchar++;
          continue;
        }
        *(newpos++) = leftchar++;
      }
    }
  }

  *newpos = '\0';

  return (newfmt);
}

/****************************************************************************/
/** \brief Split a string into tokens each of maximal length 'n+1'

   \param str -   pointer to char (const)
   \param sep -   pointer to char (const)
   \param n -     integer, number of chars in token
   \param token - pointer to char

   This function splits a string into tokens each of maximal length 'n+1'.
   A pointer to the next char following the token (its a sep) is returned.
   NB: possibly check whether the returned char is a sep.
   If not: the token was to long and only the first n chars where copied!

   \return
   .n     pointer to token
   .n     NULL if token larger than n.
 */
/****************************************************************************/

const char * NS_PREFIX strntok (const char *str, const char *sep, int n, char *token)
{
  int i;

  /* scan while current char is a separator */
  while ((*str!='\0') && (strchr(sep,*str)!=NULL)) str++;

  /* copy into token */
  for (i=0; i<n; i++,str++)
    if ((*str!='\0') && (strchr(sep,*str)==NULL))
      token[i] = *str;
    else
      break;

  if (strchr(sep,*str)==NULL)
    return (NULL);                    /* ERROR: token too long! */

  /* 0-terminate string */
  token[i] = '\0';

  return (str);
}
