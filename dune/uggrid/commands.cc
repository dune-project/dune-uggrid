// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file commands.c
 * \ingroup ui
 */

/** \addtogroup ui
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      commands.c                                                    */
/*                                                                          */
/* Purpose:   definition of all dimension independent commands of ug        */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*            02.02.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:   for dimension dependent commands see commands2d/3d.c          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/* system include files                                                     */
/* application include files                                                */
/*                                                                          */
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cassert>

#include <dune/uggrid/initug.h>

/* low module */
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/defaults.h>
#include <dune/uggrid/low/fileopen.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/heaps.h>              /* for MEM declaration */
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugstruct.h>
#include <dune/uggrid/low/ugtime.h>
#include <dune/uggrid/low/ugtypes.h>

/* devices module */
#include <dune/uggrid/ugdevices.h>

/* grid manager module */
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/cw.h>
#include <dune/uggrid/gm/elements.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/mgio.h>
#include <dune/uggrid/gm/pargm.h>
#include <dune/uggrid/gm/rm.h>
#include <dune/uggrid/gm/shapes.h>
#include <dune/uggrid/gm/ugm.h>

/* numerics module */
#include <dune/uggrid/numerics/np.h>
#include <dune/uggrid/numerics/disctools.h>
#include <dune/uggrid/numerics/udm.h>

#ifdef ModelP
#include <dune/uggrid/parallel/dddif/parallel.h>
#endif

/* own header */
#include "commands.h"

USING_UG_NAMESPACES
using namespace PPIF;

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static MULTIGRID *currMG=NULL;                  /*!< The current multigrid			*/

static INT untitledCounter=0;   /*!< Counter for untitled multigrids	*/

REP_ERR_FILE

/****************************************************************************/
/** \brief Return a pointer to the current multigrid
 *
 * This function returns a pionter to the current multigrid.
 *
 * @return <ul>
 *    <li> pointer to multigrid </li>
 *    <li> NULL if there is no current multigrid. </li>
 * </ul>
 */
/****************************************************************************/

MULTIGRID * NS_DIM_PREFIX GetCurrentMultigrid (void)
{
  return (currMG);
}

/** \brief Implementation of \ref configure. */
INT NS_DIM_PREFIX ConfigureCommand (INT argc, char **argv)
{
  BVP *theBVP;
  BVP_DESC theBVPDesc;
  char BVPName[NAMESIZE];

  /* get BVP name */
  if (sscanf(argv[0],expandfmt(" configure %" NAMELENSTR "[ -~]"),BVPName) != 1 || strlen(BVPName) == 0)
  {
    PrintErrorMessage('E', "ConfigureCommand", "cannot read BndValProblem specification");
    return 1;
  }

  theBVP = BVP_GetByName(BVPName);
  if (theBVP == NULL)
  {
    PrintErrorMessage('E', "ConfigureCommand", "cannot read BndValProblem specification");
    return 1;
  }

  if (BVP_SetBVPDesc(theBVP,&theBVPDesc))
    return 1;

  if (BVPD_CONFIG(&theBVPDesc)!=NULL)
    if ((*BVPD_CONFIG(&theBVPDesc))(argc,argv))
    {
      PrintErrorMessage('E',"configure"," (could not configure BVP)");
      return 1;
    }

  return 0;
}


/** \brief Implementation of \ref close. */
static INT CloseCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i;
  bool closeonlyfirst;

  if (ResetPrintingFormat())
    REP_ERR_RETURN(1);

  closeonlyfirst = true;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      closeonlyfirst = false;
      break;

    default :
      PrintErrorMessageF('E', "CloseCommand", "Unknown option '%s'", argv[i]);
      return 1;
    }

  i = 0;
  do
  {
    /* get multigrid */
    theMG = currMG;
    if (theMG==NULL)
    {
      if (i==0)
      {
        PrintErrorMessage('W',"close","no open multigrid");
        return 0;
      }
      closeonlyfirst = false;
      break;
    }

    if (DisposeMultiGrid(theMG)!=0)
    {
      PrintErrorMessage('E',"close","closing the mg failed");
      return 1;
    }
    i++;

    currMG = GetFirstMultigrid();
  }
  while (!closeonlyfirst);

  return 0;
}


/** \brief Implementation of \ref new. */
INT NS_DIM_PREFIX NewCommand (INT argc, char **argv, std::shared_ptr<PPIF::PPIFContext> ppifContext)
{
  MULTIGRID *theMG;
  char Multigrid[NAMESIZE],BVPName[NAMESIZE],Format[NAMESIZE];
  INT i;
  bool bopt, fopt, hopt, IEopt, emptyGrid;

  /* get multigrid name */
  if (sscanf(argv[0],expandfmt(" new %" NAMELENSTR "[ -~]"),Multigrid) != 1 || strlen(Multigrid)==0)
    sprintf(Multigrid,"untitled-%d",(int)untitledCounter++);

  theMG = GetMultigrid(Multigrid);
  if ((theMG != NULL) && (theMG == currMG)) CloseCommand(0,NULL);

  /* get problem, domain and format */
  bopt = fopt = hopt = false;
  IEopt = true;
  emptyGrid = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      if (sscanf(argv[i],expandfmt("b %" NAMELENSTR "[ -~]"),BVPName) != 1)
      {
        PrintErrorMessage('E', "NewCommand", "cannot read BndValProblem specification");
        return 1;
      }
      bopt = true;
      break;

    case 'f' :
      if (sscanf(argv[i],expandfmt("f %" NAMELENSTR "[ -~]"),Format) != 1)
      {
        PrintErrorMessage('E', "NewCommand", "cannot read format specification");
        return 1;
      }
      fopt = true;
      break;

    case 'n' :
      IEopt = false;
      break;

    case 'e' :
      emptyGrid = true;
      break;

    default :
      PrintErrorMessageF('E', "NewCommand", "Unknown option '%s'", argv[i]);
      return 1;
    }

  if (!(bopt && fopt))
  {
    PrintErrorMessage('E', "NewCommand", "the d, p, and f arguments are mandatory");
    return 1;
  }

  /* allocate the multigrid structure */
  theMG = CreateMultiGrid(Multigrid,BVPName,Format,IEopt,!emptyGrid, ppifContext);
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"new","could not create multigrid");
    return 1;
  }

  currMG = theMG;

  return 0;
}

/** @} */
