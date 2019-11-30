// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      oopp.h                                                        */
/*                                                                          */
/* Purpose:   macros for implementing some rudimentary object-oriented      */
/*            techniques via the C preprocessor.                            */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: birken@ica3.uni-stuttgart.de                           */
/*            phone: 0049-(0)711-685-7007                                   */
/*            fax  : 0049-(0)711-685-7000                                   */
/*                                                                          */
/* History:   970720 kb  begin                                              */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



#ifndef __OOPP_H__
#define __OOPP_H__

/****************************************************************************/
/*                                                                          */
/* basic text manipulation macros                                           */
/*                                                                          */
/****************************************************************************/

#define _CCAT(a,b)   a ## b
#define CCAT(a,b)    _CCAT(a,b)

#define CALL(a,b)    CCAT(CCAT(a,_),b)



/****************************************************************************/
/*                                                                          */
/* class declaration                                                        */
/*                                                                          */
/****************************************************************************/

/* from now on, we assume a previous '#define ClassPrefix XXX' */
/* construction of class-name */
#ifdef ClassPrefix
#define CN(C)            CCAT(ClassPrefix,C)
#else
#define CN(C)            C
#endif


/* from now on, we suppose a '#define ClassName XXX' for a given class */

#define Class            CN(ClassName)           /* internal class-name */
#define ClassPtr         Class*
#define ClassRef         ClassPtr
/* #define VoidRef          void * */
#define Class_Data_Begin struct Class {
#define Class_Data_End   };

#define Method(M)        CCAT(CCAT(ClassName,_),M)

/* support for easy 'This' */
#define This             _oopp_this
/*
   #define DefThis          ClassRef
   #define ParamThis        ClassRef This
 */
#define DefThis          Class *
#define ParamThis        Class * This


/****************************************************************************/
/*                                                                          */
/* standard class methods                                                   */
/*                                                                          */
/****************************************************************************/


/*** constructor (NewClass) ***/
/* macros for prototype */
#define Method_New_      ClassPtr CCAT(New_,ClassName)
#define Method_New(O)    ClassPtr CCAT(CCAT(New_,ClassName),O)   /* overload */

/* macros for implementation */
#define Construct(item,check)      \
  ClassPtr item=(ClassPtr) OO_Allocate (sizeof(Class));  \
  { check; }

#define Destruct(item)   OO_Free(item)


/****************************************************************************/
/*                                                                          */
/* inheritance                                                              */
/*                                                                          */
/****************************************************************************/

#define BaseClass(BC)         typedef CN (BC) Class


/****************************************************************************/

#endif
