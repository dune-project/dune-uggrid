// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file bio.h
 * \ingroup low
 */

/** \addtogroup low
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      bio.h                                                         */
/*                                                                          */
/* Purpose:   header file for bio.c                                         */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70550 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   09.12.96 begin                                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __BIO__
#define __BIO__

#include <cstdio>

#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* configuration of interface                                               */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)        */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

/* mode of file */
#define BIO_XDR                                 0
#define BIO_ASCII                                       1
#define BIO_BIN                                         2

/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/** \ingroup low \brief Hallo Welt! */
int Bio_Initialize                      (FILE *file, int mode, char rw);
int Bio_Read_mint                       (int n, int *intList);
int Bio_Write_mint                      (int n, int *intList);
int Bio_Read_mdouble            (int n, double *doubleList);
int Bio_Write_mdouble           (int n, double *doubleList);
int Bio_Read_string             (char *string);
int Bio_Write_string            (const char *string);
int Bio_Jump_From                       (void);
int Bio_Jump_To                         (void);
int Bio_Jump                            (int dojump);


END_UG_NAMESPACE

/** @} */
#endif
