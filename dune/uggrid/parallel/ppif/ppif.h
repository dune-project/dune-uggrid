// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ppif.h                                                        */
/*                                                                          */
/* Purpose:   header file for parallel processor interface                  */
/*                                                                          */
/* Author:    Peter Bastian / Klaus Birken                                  */
/*                                                                          */
/* History:   17 Aug 1992 begin                                             */
/*            14 Sep 1993 pass argc, argv from main to InitPPIF             */
/*            16 Sep 1993 async send/recv return msgid now                  */
/*            951106 kb  changed parameters for InitPPIF()                  */
/*            970213 kb  added C++ class interface                          */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __PPIF__
#define __PPIF__

#include <memory>

#include <dune/uggrid/parallel/ppif/ppiftypes.hh>

/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

using VChannelPtr = PPIF::VChannelPtr;
using msgid = PPIF::msgid;

namespace PPIF {

enum directions {north,east,south,west,up,down};

#define PPIF_SUCCESS    0       /* Return value for success                     */
#define PPIF_FAILURE    1       /* Return value for failure                     */

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/* id's */
extern int me;                     /* my processor id                       */
extern int master;                 /* id of master processor                */
extern int procs;                  /* number of processors in the network   */

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/**
 * set context used by PPIF.
 *
 * This also updates the legacy global variables `me`, `master`, and `procs`.
 *
 * \warn context should be managed by the caller
 */
void ppifContext(const std::shared_ptr<PPIFContext>& context);

/**
 * reset context used by PPIF.
 */
void ppifContext(std::nullptr_t);

/**
 * get context used by PPIF
 *
 * \warn context should be managed by the called
 */
const std::shared_ptr<PPIFContext>& ppifContext();

/* initialization & shutdown */
void        InitPPIF         (PPIFContext& context);
int         InitPPIF         (int *argcp, char ***argvp);
void        ExitPPIF         (PPIFContext& context);
int         ExitPPIF         (void);

/* tree oriented functions */
int         Broadcast        (const PPIFContext& context, void* data, int size);
int         Concentrate      (const PPIFContext& context, void *data, int size);
int         GetConcentrate   (const PPIFContext& context, int slave, void *data, int size);
int         Spread           (const PPIFContext& context, int slave, void *data, int size);
int         GetSpread        (const PPIFContext& context, void *data, int size);
int         Synchronize      (const PPIFContext& context);

/* synchronous communication */
VChannelPtr ConnSync         (const PPIFContext& context, int p, int id);
int         DiscSync         (const PPIFContext& context, VChannelPtr vc);
int         SendSync         (const PPIFContext& context, VChannelPtr vc, void *data, int size);
int         RecvSync         (const PPIFContext& context, VChannelPtr vc, void *data, int size);

/* asynchronous communication */
VChannelPtr ConnASync        (const PPIFContext& context, int p, int id);
int         DiscASync        (const PPIFContext& context, VChannelPtr vc);
msgid       SendASync        (const PPIFContext& context, VChannelPtr vc, void *data, int size, int *error);
msgid       RecvASync        (const PPIFContext& context, VChannelPtr vc, void *data, int size, int *error);
int         InfoAConn        (const PPIFContext& context, VChannelPtr vc);
int         InfoADisc        (const PPIFContext& context, VChannelPtr vc);
int         InfoASend        (const PPIFContext& context, VChannelPtr vc, msgid m);
int         InfoARecv        (const PPIFContext& context, VChannelPtr vc, msgid m);

}  // end namespace PPIF


/****************************************************************************/

#endif
