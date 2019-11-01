// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      misc.h                                                        */
/*                                                                          */
/* Purpose:   header for misc.c                                             */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen                             */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            internet: ug@ica3.uni-stuttgart.de                            */
/*                                                                          */
/* History:   23.02.95 ug3-version                                          */
/*                                                                          */
/* Revision:  07.09.95                                                      */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __MISC__
#define __MISC__


#include "ugtypes.h"
#include <cstring>
#include "heaps.h"

#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*          compile time constants defining static data size (i.e. arrays)  */
/*          other constants                                                 */
/*          macros                                                          */
/*                                                                          */
/****************************************************************************/

#ifndef PI
#define PI                       3.141592653589793238462643383279
#endif

#define KBYTE                                   1024
#define MBYTE                                   (KBYTE*KBYTE)
#define GBYTE                                   (KBYTE*KBYTE*KBYTE)


/* cleanup old definitions of macros */
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif

#define MIN(x,y)                 (((x)<(y)) ? (x) : (y))
#define MAX(x,y)                 (((x)>(y)) ? (x) : (y))
#define POW2(i)                  (1<<(i))

#define SET_FLAG(flag,bitpattern)               (flag |=  (bitpattern))
#define CLEAR_FLAG(flag,bitpattern)     (flag &= ~(bitpattern))
#define READ_FLAG(flag,bitpattern)              ((flag & (bitpattern))>0)

#define HiWrd(aLong)             (((aLong) >> 16) & 0xFFFF)
#define LoWrd(aLong)             ((aLong) & 0xFFFF)

#define SetHiWrd(aLong,n)        aLong = (((n)&0xFFFF)<<16)|((aLong)&0xFFFF)
#define SetLoWrd(aLong,n)        aLong = ((n)&0xFFFF)|((aLong)&0xFFFF0000)

/* concatenation macros for preprocessor */
#define XCAT(a,b)                       a ## b
#define XCAT3(a,b,c)            a ## b ## c
#define CAT(a,b)                        XCAT(a,b)
#define CAT3(a,b,c)                     XCAT3(a,b,c)

/* expand macro and transfer expanded to string */
#define XSTR(s) # s
#define STR(s) XSTR(s)

#ifndef YES
    #define YES         1
#endif
#define ON              1

#ifndef NO
    #define NO          0
#endif
#define OFF             0

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

#ifndef ModelP

END_UG_NAMESPACE
namespace PPIF {

extern int me;          /* to have in the serial case this variable as a dummy */
extern int master;  /* to have in the serial case this variable as a dummy */
extern int procs;       /* to have in the serial case this variable as a dummy */
}  /* end namespace PPIF */
START_UG_NAMESPACE

extern int _proclist_; /* to have in the serial case this variable as a dummy*/
extern int _partition_; /* to have in the serial case this variable as a dummy*/
#endif

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* general routines */
void            INT_2_bitpattern        (INT n, char text[33]);
INT                     CenterInPattern         (char *str, INT PatLen, const char *text, char p, const char *end);
char       *expandfmt           (const char *fmt);
char       *ExpandCShellVars    (char *string);
const char *strntok             (const char *str, const char *sep, int n, char *token);

INT             ReadMemSizeFromString   (const char *s, MEM *mem_size);
INT                     WriteMemSizeToString    (MEM mem_size, char *s);

END_UG_NAMESPACE

#endif
