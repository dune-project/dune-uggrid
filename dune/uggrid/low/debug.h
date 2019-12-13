// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  debug.h														*/
/*																			*/
/* Purpose:   header file for ug internal debugger							*/
/*																			*/
/* Author:	  Stefan Lang                                                                                   */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: stefan@ica3.uni-stuttgart.de							*/
/*			  phone: 0049-(0)711-685-7003									*/
/*			  fax  : 0049-(0)711-685-7000									*/
/*																			*/
/* History:   10.07.95 begin                                                                            */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __DEBUG__
#define __DEBUG__

#include "general.h"
#include "misc.h"
#include "ugtypes.h"

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define REP_ERR_MAX             10
#define DEBUG_TIME_MAX  100

/* if HEAPCHECK is true p is a pointer to a zombie object */
#define HEAPCHECK(ptr) (((int *)ptr)[1] == -1)

#ifdef Debug

#include <cassert>

#define IFDEBUG(m,n)    if (Debug ## m >=(n)) {
#define PRINTDEBUG(m,n,s) IFDEBUG(m,n) PrintDebug s; ENDDEBUG
#define PRINTDEBUG_EXT(m,n,s) IFDEBUG(m,n) PrintDebug("-" STR(m) "-"); PrintDebug s; ENDDEBUG
#define ENDDEBUG  }
#define RETURN(rcode)   {INT rc; rc = rcode; assert(!rc); return (rc);}
#define SETHEAPFAULT(p,v)       {int *hf_pr; hf_pr=(int*)(((char*)(p))+sizeof(void*)); hf_pr[0]=(int)(v);}
#define HEAPFAULT(p)  assert(((int*)(((char*)(p))+sizeof(void*)))[0]!=-1);
#define ASSERT(exp)             assert(exp)
#define REP_ERR_INC             {rep_err_line[rep_err_count] = __LINE__;  \
                                 rep_err_file[rep_err_count] = this_file; \
                                 rep_err_count = (rep_err_count+1)%REP_ERR_MAX;}
#ifdef ModelP
#define REP_ERR_RETURN(err)             { assert(((err)==0));return (err);}
#define REP_ERR_RETURN_PTR(p)   { assert(((p)!=NULL));return (p);}
#define REP_ERR_RETURN_VOID             { assert(false);return;}
#define REP_ERR_GOTO(st,lbl)    { st; assert(false); goto lbl;}
#else
#define REP_ERR_RETURN(err)             { if (err) REP_ERR_INC  return (err);}
#define REP_ERR_RETURN_PTR(p)   { if (p == NULL) REP_ERR_INC  return (p);}
#define REP_ERR_RETURN_VOID             { REP_ERR_INC  return;}
#define REP_ERR_GOTO(st,lbl)    { REP_ERR_INC st; goto lbl;}
#endif
#define REP_ERR_ENCOUNTERED             (rep_err_count)

#define REP_ERR_RESET                   rep_err_count = 0;
#define REP_ERR_FILE                    static char *this_file=__FILE__;

#else /* Debug */

#define IFDEBUG(m,n)    if (1==0) {
#define ENDDEBUG        }
#define PRINTDEBUG(m,n,s) /* no debugging */
#define PRINTDEBUG_EXT(m,n,s) /* no debugging */
#define RETURN(rcode)   return (rcode)
#define HEAPFAULT(ptr)
#define ASSERT(exp)

#define REP_ERR_RETURN(err)             {return (err);}
#define REP_ERR_RETURN_PTR(p)   {return (p);}
#define REP_ERR_RETURN_VOID             {return;}
#define REP_ERR_GOTO(st,lbl)    {st; goto lbl;}
#define REP_ERR_ENCOUNTERED             (false)
#define REP_ERR_INC
#define REP_ERR_RESET
#define REP_ERR_FILE

#define PrintDebug(...)

#endif  /* Debug */

START_UG_NAMESPACE

/****************************************************************************/
/*																			*/
/* data structures exported by the corresponding source file				*/
/*																			*/
/****************************************************************************/

typedef int (*PrintfProcPtr)(const char *, ...);

/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/

#if (defined Debug && !defined compile_debug)

extern int Debuginit;
extern int Debugdddif;
extern int Debugdev;
extern int Debuggm;
extern int Debuggraph;
extern int Debuggui;
extern int Debuglow;
extern int Debugdom;
extern int Debugmachines;
extern int Debugnp;
extern int Debugui;
extern int Debugappl;
extern int Debugpclib;

/* for reporting of erros (using the REP_ERR_RETURN-macro) */
extern int rep_err_count;
extern int rep_err_line[REP_ERR_MAX];
extern const char  *rep_err_file[REP_ERR_MAX];

#endif

/****************************************************************************/
/*																			*/
/* function declarations													*/
/*																			*/
/****************************************************************************/

#ifdef Debug
void SetPrintDebugProc          (PrintfProcPtr print);
int  PrintDebug                         (const char *format, ...);
int  PrintDebugToFile           (const char *format, ...);
int  SetPrintDebugToFile        (const char *fname);
int  PostprocessDebugFile       (const char *newname);
INT  PrintRepErrStack           (PrintfProcPtr print);
#endif

END_UG_NAMESPACE

#endif
