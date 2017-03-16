// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:          np.h                                                                                                                  */
/*                                                                                                                                                      */
/* Purpose:   numerics subsystem header file                                                            */
/*                                                                                                                                                      */
/* Author:        Klaus Johannsen/Henrik Rentz-Reichert                                                 */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70569 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                               */
/*                                                                                                                                                      */
/* History:   25.03.95 begin, ug version 3.0                                                            */
/*                        09.12.95 transition to new descriptor formats (HRR)                   */
/*                        December 2, 1996 redesign of numerics                                                 */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __NP__
#define __NP__

#include "ugtypes.h"
#include "gm.h"
#include "algebra.h"
#include "ugenv.h"
#include "udm.h"
#include "numproc.h"
#include "npscan.h"

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*     compile time constants defining static data size (i.e. arrays)       */
/*     other constants                                                      */
/*     macros                                                               */
/*                                                                          */
/****************************************************************************/

/* if FF_PARALLEL_SIMULATION is defined, special functions from fe/ff are linked */
/*#define FF_PARALLEL_SIMULATION*/
/*#define FF_ModelP*/           /* um temp. Verweise von np nach fe/ff fuer die Allgemeinheit auszublenden */

/* return codes of the numerics routines                                                                        */
#define NUM_OK                                  0       /* everything ok                                                */
#define NUM_OUT_OF_MEM                  1       /* could not allocate mem (for connect.)*/
/*#define NUM_DECOMP_FAILED        -n      any neg value: diag block singular   */
#define NUM_DESC_MISMATCH               3       /* descriptors passed are inconsistent  */
#define NUM_BLOCK_TOO_LARGE             4       /* block too large
                                                                                        (increase MAX_SINGLE_VEC_COMP)  */
#define NUM_FORMAT_MISMATCH             5       /* user data size exceeded                              */
#define NUM_SMALL_DIAG                  6       /* diag entry too small to invert               */
#define NUM_NO_COARSER_GRID             7       /* restrict called on grid level 0              */
#define NUM_TYPE_MISSING                8       /* indicates one float for VEC_SCALAR   */
#define NUM_ERROR                               9       /* other error                                                  */

/* modes for l_iluspdecomp */
#define SP_LOCAL                                0       /* modify locally                                               */
#define SP_GLOBAL                               1       /* modify globally                                              */

/* matrix consitency modes */
#define MAT_DIAG_CONS         0
#define MAT_CONS              1
#define MAT_MASTER_CONS       2
#define MAT_GHOST_DIAG_CONS   3
#define MAT_DIAG_VEC_CONS     4

/* special REP_ERR_RETURN macro */
#define NP_RETURN(err,intvar)           {intvar = __LINE__; REP_ERR_RETURN(err);}

/****************************************************************************/
/*                                                                                                                                                      */
/* macros concerned with solving                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

/* formats for display routines */
#define DISPLAY_WIDTH                                   50
#define DISPLAY_NP_BAR                                  "--------------------------------------------------\n"
#define DISPLAY_NP_LI_FORMAT_SSSSS              "%-2s %-15.12s %-15.12s %-15.12s %-15.12s\n"
#define DISPLAY_NP_LI_FORMAT_SSSSSI             "%-2s %-15.12s %-15.12s %-15.12s %-15.12s %-2d\n"
#define DISPLAY_NP_LI_FORMAT_SSSSSS             "%-2s %-15.12s %-15.12s %-15.12s %-15.12s %-15.12s\n"
#define DISPLAY_NP_FORMAT_S                     "%-16.13s = "
#define DISPLAY_NP_FORMAT_SS                    "%-16.13s = %-35.32s\n"
#define DISPLAY_NP_FORMAT_SSS                   "%-16.13s = %-15.12s %-15.12s\n"
#define DISPLAY_NP_FORMAT_SF                    "%-16.13s = %-7.4g\n"
#define DISPLAY_NP_FORMAT_SFF                   "%-16.13s = %-7.4g  %-7.4g\n"
#define DISPLAY_NP_FORMAT_SFFF                  "%-16.13s = %-7.4g  %-7.4g  %-7.4g\n"
#define DISPLAY_NP_FORMAT_SI                    "%-16.13s = %-2d\n"
#define DISPLAY_NP_FORMAT_SII                   "%-16.13s = %-2d  %-2d\n"
#define DISPLAY_NP_FORMAT_SIII                  "%-16.13s = %-2d  %-2d  %-2d\n"
#define DISPLAY_NP_FORMAT_FF                    "%-7.4g  %-7.4g\n"

#define CLEAR_VECSKIP_OF_GRID(g)                                \
  { VECTOR *theVector;                                 \
    for (theVector=FIRSTVECTOR((g)); theVector!= NULL; \
         theVector=SUCCVC(theVector))                  \
      VECSKIP(theVector) = 0;}

/****************************************************************************/
/*                                                                          */
/* structures concerned with symbolic user data management                  */
/*                                                                          */
/****************************************************************************/

typedef INT (*SetFuncProcPtr)(const DOUBLE_VECTOR, INT, DOUBLE *);

/****************************************************************************/
/*                                                                          */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

/****************************************************************************/
/*                                                                                                                                                      */
/* blas routines and iterative methods                                                                          */
/*                                                                                                                                                      */
/****************************************************************************/

#ifdef ModelP
INT l_vector_consistent (GRID *g, const VECDATA_DESC *x);
INT a_vector_consistent (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
INT l_ghostvector_consistent (GRID *g, const VECDATA_DESC *x);
INT a_outervector_consistent (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
INT l_ghostvector_project (GRID *g, const VECDATA_DESC *x);
INT l_vector_collect (GRID *g, const VECDATA_DESC *x);
INT a_vector_collect (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
INT a_vector_collect_noskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
INT l_ghostvector_collect (GRID *g, const VECDATA_DESC *x);
INT a_vector_vecskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
int DDD_InfoPrioCopies (DDD_HDR hdr);
INT a_elementdata_consistent (MULTIGRID *mg, INT fl, INT tl);
INT a_nodedata_consistent (MULTIGRID *mg, INT fl, INT tl);
INT l_vector_consistent_noskip (GRID *g, const VECDATA_DESC *x);
INT a_vector_consistent_noskip (MULTIGRID *mg, INT fl, INT tl, const VECDATA_DESC *x);
INT l_vector_minimum_noskip (GRID *g, const VECDATA_DESC *x);
INT l_vector_maximum_noskip (GRID *g, const VECDATA_DESC *x);

#ifdef __BLOCK_VECTOR_DESC__
INT l_vector_consistentBS (GRID *g, const BV_DESC *bvd, const BV_DESC_FORMAT *bvdf, INT x);
#endif

#endif


/* modus for blas routines                                                  */
#define ON_SURFACE      -1      /* class on surface                                     */
#define ALL_VECTORS      0      /* all vectors                                          */

/* iterative methods */
INT l_setindex          (GRID *g);

/****************************************************************************/
/*                                                                                                                                                      */
/* symbols and numprocs                                                                                                         */
/*                                                                                                                                                      */
/****************************************************************************/

/* create */
NP_BASE *       GetNumProcFromName                      (char *name);

/* step */
NP_BASE    *GetFirstNumProcType                 (void);
NP_BASE    *GetNextNumProcType                  (NP_BASE *);
NP_BASE   *GetFirstNumProc                              (void);
NP_BASE   *GetNextNumProc                               (NP_BASE *);

/* miscellaneous */
INT             ExecuteNumProc                          (NP_BASE *theNumProc, MULTIGRID *theMG, INT argc, char **argv);
INT             DisplayNumProc                          (NP_BASE *theNumProc);
INT             ListNumProc                             (NP_BASE *currNumProc);
INT                     SetNumProc                                      (NP_BASE *, INT, char **);
INT             InitNum                                         (void);
INT             GetVectorCompNames                      (VECDATA_DESC *theVDT, char *compNames, INT *nComp);
INT             WriteVEC_SCALAR                         (const VECDATA_DESC *theVDT, const VEC_SCALAR Scalar, const char *structdir);

END_UGDIM_NAMESPACE

#endif
