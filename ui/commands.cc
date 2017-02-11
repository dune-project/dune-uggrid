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

#ifdef __MPW32__
#pragma segment cmd
#endif

/****************************************************************************/
/*                                                                          */
/* defines to exclude functions                                             */
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

/* low module */
#include "ugtypes.h"
#include "architecture.h"
#include "ugtime.h"
#include "initug.h"
#include "defaults.h"
#include "misc.h"
#include "ugstruct.h"
#include "fileopen.h"
#include "ugenv.h"
#include "debug.h"
#include "heaps.h"              /* for MEM declaration */
#include "general.h"

/* devices module */
#include <dev/ugdevices.h>

/* grid manager module */
#include "gm.h"
#include "elements.h"
#include "cw.h"
#include "pargm.h"
#include "rm.h"
#include "evm.h"
#include "ugm.h"
#include "algebra.h"
#include "shapes.h"
#include "mgio.h"

/* numerics module */
#include "np.h"
#include "ugblas.h"
#include "disctools.h"
#include "npcheck.h"
#include "udm.h"

/* user interface module */
#include "ugstruct.h"
#include "cmdint.h"
#include "cmdline.h"

#ifdef ModelP
#include "parallel.h"
#endif


/* own header */
#include "commands.h"


USING_UG_NAMESPACES
using namespace PPIF;

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*    compile time constants defining static data size (i.e. arrays)        */
/*    other constants                                                       */
/*    macros                                                                */
/*                                                                          */
/****************************************************************************/

/** \brief Size of the general purpose text buffer*/
#define BUFFERSIZE                              512

#define WHITESPACE                              " \t"

/** \brief Size of some strings */
#define LONGSTRSIZE                     256
/** \brief Length of some strings                                       */
#define LONGSTRLEN                              255
/** \brief LONGSTRLEN as string                                 */
#define LONGSTRLENSTR                   "255"

/** @name For ProtoOnCommand */
/*@{*/
#define NORENAME_PROTO                  0
#define APPEND_PROTO                    1
#define RENAME_PROTO                    2
#define TRYRENAME_PROTO                 3
#define MAXPATHLENGTH                   255
#define MAXRENAMECHAR                   'z'
/*@}*/

/** @name For the .list commands */
/*@{*/
#define DO_ID                                   1
#define DO_SELECTION                    2
#define DO_ALL                                  3
/*@}*/

/** @name For MarkCommand */
/*@{*/
#define MARK_ALL                                1
#define AI_MARK_ALL             256
#define MARK_COARSEN                    2
#define MARK_ID                                 3
#define MARK_SELECTION                  4
#define NO_SIDE_SPECIFIED               -1
#define NO_RULE_SPECIFIED               -1
#define NO_OF_RULES                     64
/*@}*/

/* for anisotropic refinement */

/* for save command */
#define NO_COMMENT                               "no comment"

/** @name For array commands */
/*@{*/
#define AR_NVAR_MAX                     10
#define AR_NVAR(p)                      ((p)->nVar)
#define AR_VARDIM(p,i)          ((p)->VarDim[i])
#define AR_DATA(p,i)            ((p)->data[i])
/*@}*/

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

struct MarkRule
{
  const char *RuleName;         /*!< what you type in the mark cmdline	*/
  INT RuleId;           /*!< corresponding rule ID for refine   */
};

typedef struct MarkRule MARKRULE;

typedef struct
{
  /** \brief Fields for environment directory */
  ENVVAR v;

  /* data */
  INT nVar;
  INT VarDim[AR_NVAR_MAX];
  DOUBLE data[1];

} ARRAY;

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static MULTIGRID *currMG=NULL;                  /*!< The current multigrid			*/

static char buffer[BUFFERSIZE];         /*!< General purpose text buffer		*/

static FILE     *protocolFile=NULL;     /*!< For protocol commands			*/

/** \brief Name and ID of available rules	*/
static MARKRULE myMR[NO_OF_RULES]=     {{"red",        RED},
                                        {"no",         NO_REFINEMENT},
#ifdef __TWODIM__
                                        {"blue",       BLUE},
#endif
                                        {"copy",       COPY},
                                         #ifdef __TWODIM__
                                        {"bi_1",       BISECTION_1},
                                        {"bi_2q",      BISECTION_2_Q},
                                        {"bi_2t1", BISECTION_2_T1},
                                        {"bi_2t2", BISECTION_2_T2},
                                        {"bi_3",       BISECTION_3},
                                         #endif
                                         #ifdef __THREEDIM__
                                        /* rules for tetrahedra */
                                         #ifndef TET_RULESET
                                        {"tet2hex",TETRA_RED_HEX},
                                        {"pri2hex",PRISM_RED_HEX},
                                         #endif
                                        /* rules for prisms */
                                        {"pri_quadsect",PRISM_QUADSECT},
                                        {"pri_bisect_hex0",PRISM_BISECT_HEX0},
                                        {"pri_bisect_hex1",PRISM_BISECT_HEX1},
                                        {"pri_bisect_hex2",PRISM_BISECT_HEX2},
                                        {"pri_rot_l",PRISM_ROTATE_LEFT},
                                        {"pri_rot_r",PRISM_ROTATE_RGHT},
                                        {"pri_quadsect_eins",PRISM_QUADSECT_HEXPRI0},
                                        /* rules for tetrahedra */
                                        {"hex_bisect_eins",HEX_BISECT_0_1},
                                        {"hex_bisect_zwei",HEX_BISECT_0_2},
                                        {"hex_bisect_drei",HEX_BISECT_0_3},
                                        {"hex_trisect_eins",HEX_TRISECT_0},
                                        {"hex_trisect_fuenf",HEX_TRISECT_5},
                                        {"hex_quadsect_null",HEX_QUADSECT_0},
                                        {"hex_quadsect_eins",HEX_QUADSECT_1},
                                        {"hex_quadsect_zwei",HEX_QUADSECT_2},
                                        {"hex_bisect_vier",HEX_BISECT_HEXPRI0},
                                        {"hex_bisect_fuenf",HEX_BISECT_HEXPRI1},
                                         #endif
                                        {"coarse", COARSE}};

static char userPath[1024];             /*!< Environment path for ls,cd		*/

static INT untitledCounter=0;   /*!< Counter for untitled multigrids	*/

/** @name Some variables to transfer info between QualityCommand and QualityElement*/
/*@{*/
static DOUBLE min,max,themin,themax,minangle,maxangle;
static INT lessopt,greateropt,selectopt;
static char mintext[32],maxtext[32],minmaxtext[32];
/*@}*/

/** @name Stuff for the array commands */
/*@{*/
static INT theArrayDirID;
static INT theArrayVarID;
static bool arraypathes_set = false;
/*@}*/

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

/****************************************************************************/
/** \brief Set the current multigrid if it is valid
 *
 * @param theMG pointer to multigrid
 *
 * This function sets the current multigrid if it is valid, i. e.
 * the function checks whether 'theMG' acually points to a multigrid.
 * It can be NULL only if no multigrid is open.
 *
 * @result <ul>
 *    <li> 0 if ok </li>
 *    <li> 1 if theMG is not in the multigrid list </li>
 * </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SetCurrentMultigrid (MULTIGRID *theMG)
{
  MULTIGRID *mg;

  if (ResetPrintingFormat())
    REP_ERR_RETURN(CMDERRORCODE);

  mg = GetFirstMultigrid();
  if (mg==theMG)
  {
    /* possibly NULL */
    currMG = theMG;
    return (0);
  }

  for (; mg!=NULL; mg=GetNextMultigrid(mg))
    if (mg==theMG)
    {
      /* never NULL */
      currMG = theMG;
      return (0);
    }

  return (1);
}


/** \brief Implementation of \ref exitug. */
static INT ExitUgCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  ExitUg();

  exit(0);

  return 0;
}


/** \brief Implementation of \ref ls. */
static INT ListEnvCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;
  ENVITEM *theItem;
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid -> change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  /* strip ' '*ls' '* */
  s = strchr(argv[0],'l');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* pathname is now in s: change dir to path */
  if (strlen(s)>0)
    currentDir = ChangeEnvDir(s);

  if (currentDir==NULL)
  {
    PrintErrorMessage('E',"ls","invalid path as argument");
    return (CMDERRORCODE);
  }

  theItem = currentDir->down;
  while (theItem!=NULL)
  {
    UserWrite(theItem->v.name);
    if (theItem->v.type%2==0)
      UserWrite("\n");
    else
      UserWrite("*\n");
    theItem = theItem->v.next;
  }

  return(OKCODE);
}


/** \brief Implementation of \ref cd. */
static INT ChangeEnvCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid -> change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  /* strip ' '*cd' '* */
  s = strchr(argv[0],'c');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* pathname is now in buffer */
  if (strlen(buffer)==0)
  {
    /* empty path: change to root directory */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
    else
      return (OKCODE);
  }
  currentDir = ChangeEnvDir(s);
  if (currentDir==NULL)
  {
    PrintErrorMessage('E',"cd","invalid path as argument");
    return (CMDERRORCODE);
  }
  GetPathName(userPath);
  UserWrite(userPath);
  UserWrite("\n");

  return (OKCODE);
}



/** \brief Implementation of \ref pwd. */
static INT PrintEnvDirCommand (INT argc, char **argv)
{
  ENVDIR *currentDir;

  NO_OPTION_CHECK(argc,argv);

  /* load current directory */
  currentDir = ChangeEnvDir(userPath);
  if (currentDir==NULL)
  {
    /* directory is invalid */
    strcpy(userPath,DIRSEP);
    currentDir = ChangeEnvDir(userPath);
    if (currentDir == NULL)
      return (CMDERRORCODE);
  }

  GetPathName(userPath);
  UserWrite(userPath);
  UserWrite("\n");

  return(OKCODE);
}


/** \brief Implementation of \ref envinfo. */
static INT EnvInfoCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  EnvHeapInfo(buffer);

  UserWrite(buffer);

  return (OKCODE);
}


/** \brief Implementation of \ref set. */
static INT SetCommand (INT argc, char **argv)
{
  char name[LONGSTRSIZE], *namePtr;
  INT flag,i,res,rv;
  bool ropt;
        #ifdef ModelP
  /* allocate buffer for definition of long string variables */
  char *buffer;

  /* alloc command buffer */
  if ((buffer=(char *)malloc(cmdintbufsize))==NULL)
  {
    PrintErrorMessage('F',"SetCommand","could not allocate buffer");
    return(__LINE__);
  }

  /* if there are problems with expandfmt print format string !
          printf("SetCommand(): formatstring=%s\n",
                  expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[]\t\n -~]")));
   */
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[]\t\n -~]")),name,buffer);
        #ifdef __CC__
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %16384[^]\t\n -~]")),name,buffer);
        #endif
        #else
  res = sscanf(argv[0],expandfmt(CONCAT3(" set %",LONGSTRLENSTR,"[0-9:.a-zA-Z_] %255[ -~]")),name,buffer);
        #endif /* ModelP */

  /* check options */
  ropt = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'r' :
      if (res>1)
      {
        PrintErrorMessage('E', "SetCommand", "The 'r' option applies not with setting a value");
        return (PARAMERRORCODE);
      }
      ropt = true;
      break;

    default :
      PrintErrorMessageF('E', "SetCommand", "Invalid option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  /* print or set struct */
  switch (res)
  {
  case 1 :
    namePtr=&(name[0]);
    do
    {
      rv=PrintStructContents(namePtr,buffer,BUFFERSIZE,ropt);
      if ((rv!=0)&&(rv!=4))
      {
        PrintErrorMessage('E',"set","structure not found or bad structure");
        return (CMDERRORCODE);
      }
      UserWrite(buffer);
      namePtr=NULL;
    }
    while (rv==4);
    break;

  case 2 :
    rv =  SetStringVar(name,buffer);
    if (rv!=0)
    {
      PrintErrorMessage('E',"set","could not allocate variable");
      return (CMDERRORCODE);
    }

    break;

  default :
    flag=1;
    do
    {
      rv=PrintCurrentStructContents(flag,buffer,BUFFERSIZE,ropt);
      if ((rv!=0)&&(rv!=4))
      {
        PrintErrorMessage('E',"set","structure not found or bad structure");
        return (CMDERRORCODE);
      }
      UserWrite(buffer);
      flag=0;
    }
    while (rv==4);
    break;
  }

        #ifdef ModelP
  free(buffer);
        #endif

  if (rv==0)
    return (OKCODE);
  else
    return (CMDERRORCODE);
}

/** \brief Implementation of \ref dv. */
static INT DeleteVariableCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" dv %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintErrorMessage('E', "DeleteVariableCommand", "Could not read name of variable");
    return(PARAMERRORCODE);
  }

  if (argc!=1)
  {
    PrintErrorMessage('E', "DeleteVariableCommand", "Wrong number of arguments");
    return(PARAMERRORCODE);
  }

  if (DeleteVariable(name)!=0)
  {
    PrintErrorMessage('E',"dv","could not delete variable");
    return (CMDERRORCODE);
  }
  else
    return(DONE);
}


/** \brief Implementation of \ref ms. */
static INT MakeStructCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" ms %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintErrorMessage('E', "MakeStructCommand", "Could not read name of struct");
    return(PARAMERRORCODE);
  }

  if (MakeStruct(name)!=0)
    return (CMDERRORCODE);
  else
    return (OKCODE);
}


/** \brief Implementation of \ref cs. */
static INT ChangeStructCommand (INT argc, char **argv)
{
  char *s;
  INT i;

  NO_OPTION_CHECK(argc,argv);

  /* strip ' '*cs' '* */
  s = strchr(argv[0],'c');
  strcpy(buffer,s);
  i = 2;
  while ((buffer[i]!='\0') && (strchr(WHITESPACE,buffer[i])!=NULL)) i++;
  s = buffer+i;

  /* s is now the pathname */
  if (ChangeStructDir(s)==NULL)
  {
    PrintErrorMessage('E',"cs","invalid path as argument");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref pws. */
static INT PrintWorkStructCommand (INT argc, char **argv)
{
  char structPath[1024];

  NO_OPTION_CHECK(argc,argv);

  GetStructPathName(structPath, 1024);
  UserWrite(structPath);
  UserWrite("\n");

  return(OKCODE);
}


/** \brief Implementation of \ref ds. */
static INT DeleteStructCommand (INT argc, char **argv)
{
  INT res;
  char name[LONGSTRSIZE];

  NO_OPTION_CHECK(argc,argv);

  res = sscanf(argv[0],expandfmt(CONCAT3(" ds %",LONGSTRLENSTR,"[0-9:.a-zA-Z_]")),name);

  if (res!=1)
  {
    PrintErrorMessage('E', "DeleteStructCommand", "Could not read name of struct");
    return(PARAMERRORCODE);
  }

  if (argc!=1)
  {
    PrintErrorMessage('E', "DeleteStructCommand", "Wrong number of arguments");
    return(PARAMERRORCODE);
  }

  if (DeleteStruct(name)!=0)
  {
    PrintErrorMessage('E',"ds","could not delete structure");
    return (CMDERRORCODE);
  }
  else
    return(DONE);
}


#define PROTOCOL_SEP '%'
/** \brief Implementation of \ref protocol. */
static INT ProtocolCommand (INT argc, char **argv)
{
  INT i,from;

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  if (protocolFile==NULL)
  {
    PrintErrorMessage('E',"protocol","no protocol file open!");
    return (CMDERRORCODE);
  }

  for (i=1; i<argc; i++)
  {
    if (argv[i][0]!=PROTOCOL_SEP)
    {
      PrintErrorMessage('E',"protocol","protocol options have to begin with %");
      return (PARAMERRORCODE);
    }
    from = (argv[i][2]==' ') ? 3 : 2;
    switch (argv[i][1])
    {
    case 'i' :
      fprintf(protocolFile,"%s",(argv[i])+from);
      break;

    case 't' :
      fprintf(protocolFile,"\t%s",(argv[i])+from);
      break;

    case 'n' :
      fprintf(protocolFile,"\n%s",(argv[i])+from);
      break;

    case 'f' :
      fflush(protocolFile);
      continue;

    default :
      PrintErrorMessageF('E', "ProtocolCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }
    /* write options not followed by a \ */
    while ((i+1<argc) && (argv[i+1][0]!=PROTOCOL_SEP))
      fprintf(protocolFile," $%s",(argv[++i]));
  }

  return (OKCODE);
}

/****************************************************************************/
/** \brief Open protocol file where specially formatted output is saved
 *
 * @param name - name of the protocol file
 * @param mode - APPEND_PROTO, RENAME_PROTO, TRYRENAME_PROTO, or NORENAME_PROTO
 *
 * This function opens protocol file where specially formatted output is saved.
 * It opens a protocol file for specially formatted output to file.
 *
 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error occured. </li>
 * </ul>
 */
/****************************************************************************/

static INT OpenProto (char *name, INT mode)
{
  char realname[MAXPATHLENGTH],fullname[MAXPATHLENGTH],*pos;
  INT pathlen;
  char c;

  pathlen = 0;
  if (GetDefaultValue(DEFAULTSFILENAME,"protocoldir",fullname)==0)
  {
    pathlen = strlen(fullname);
    strcat(fullname,name);
  }
  else
    strcpy(fullname,name);

  if (protocolFile!=NULL)
  {
    fclose(protocolFile);
    protocolFile = NULL;
    PrintErrorMessage('W',"OpenProto","open protocol file closed!!\n");
  }

  if (mode==APPEND_PROTO)
  {
    protocolFile = fileopen(fullname,"a");
    if (protocolFile==NULL)
      return (1);
    else
      return (0);
  }

  strcpy(realname,fullname);

  if ((mode==RENAME_PROTO)||(mode==TRYRENAME_PROTO))
  {
    /* while file with realname exists */
    c = 'a';
    while ((protocolFile=fileopen(realname,"r"))!=NULL)
    {
      fclose(protocolFile);
      protocolFile = NULL;

      if (c<=MAXRENAMECHAR)
      {
        /* try new name */
        strcpy(realname,fullname);
        if (strchr(name,'.')!=NULL)
        {
          if ((pos=strrchr(realname,'.'))!=NULL)
          {
            /* place a char before .ext in name.ext */
            *pos++ = c++;
            *pos = '\0';
            pos = strrchr(fullname,'.');
            strcat(realname,pos);
          }
        }
        else
        {
          /* place a char after name */
          pos = realname + strlen(realname);
          *pos++ = c++;
          *pos = '\0';
        }
      }
      else if (mode==RENAME_PROTO)
      {
        PrintErrorMessageF('E',"OpenProto","could not find a new name for '%s'",fullname);
        return (1);
      }
      else
        break;
    }
  }

  protocolFile = fileopen(realname,"w");
  if (protocolFile==NULL)
    return (1);

  SetStringVar(":protofilename",realname+pathlen);

  if (strcmp(realname+pathlen,name)!=0)
  {
    PrintErrorMessageF('W',"OpenProto","opened protocol file '%s' (instead of '%s')",realname+pathlen,name);
  }

  return(0);
}


/** \brief Implementation of \ref protoOn. */
static INT ProtoOnCommand (INT argc, char **argv)
{
  static char protoFileName[NAMESIZE];
  INT res,i,RenameMode;

        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  /* get document name */
  protoFileName[0] = '\0';
  res = sscanf(argv[0],expandfmt(CONCAT3(" protoOn %",NAMELENSTR,"[ -~]")),protoFileName);
  if (res!=1)
  {
    PrintErrorMessage('E', "ProtoOnCommand", "Filename not found");
    return (PARAMERRORCODE);
  }

  /* check options */
  RenameMode = NORENAME_PROTO;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (RenameMode!=NORENAME_PROTO)
      {
        PrintErrorMessage('E',"protoOn","specify either $r or $a");
        return (PARAMERRORCODE);
      }
      RenameMode = APPEND_PROTO;
      break;

    case 'r' :
      if (RenameMode!=NORENAME_PROTO)
      {
        PrintErrorMessage('E',"protoOn","specify either $r or $a");
        return (PARAMERRORCODE);
      }
      if (argv[i][1]=='!')
        RenameMode = RENAME_PROTO;
      else
        RenameMode = TRYRENAME_PROTO;
      break;

    default :
      PrintErrorMessageF('E', "ProtoOnCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (OpenProto(protoFileName,RenameMode)>0)
  {
    PrintErrorMessage('E',"protoOn","could not open protocol file");
    return(CMDERRORCODE);
  }

  return(OKCODE);
}


/** \brief Implementation of \ref protoOff. */
static INT ProtoOffCommand (INT argc, char **argv)
{
        #ifdef ModelP
  if (me != master) return(OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  if (protocolFile==NULL)
  {
    PrintErrorMessage('E',"protoOff","no protocol file open");
    return(PARAMERRORCODE);
  }

  fclose(protocolFile);

  protocolFile = NULL;

  return(OKCODE);
}

/****************************************************************************/
/** \brief Return pointer to current protocol file
 *
 * This function returns a pointer to the current protocol file (NULL if not open).
 *
 * @return <ul>
 *    <li> file ptr if ok </li>
 *    <li> NULL if no protocol file open </li>
 * </ul>
 */
/****************************************************************************/

FILE* NS_DIM_PREFIX GetProtocolFile ()
{
  return (protocolFile);
}


/** \brief Implementation of \ref logon. */
static INT LogOnCommand (INT argc, char **argv)
{
  char logfile[NAMESIZE];
  INT i, rv, res;
  int ropt;
  bool popt, pext, meext, rename;

  /* check options */
  popt = pext = meext = rename = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (protocolFile==NULL)
      {
        PrintErrorMessage('E',"logon","no protocol file open");
        return (PARAMERRORCODE);
      }
      popt = true;
      break;

    case 'e' :
                                #ifdef ModelP
      pext = true;
                                #endif
      break;

    case 'a' :
                                #ifdef ModelP
      meext = true;
                                #endif
      break;

    case 'f' :
      CloseLogFile();
      break;

    case 'r' :
      res = sscanf(argv[i]," r %d",&ropt);
      rename = (res==0 || (res==1 && ropt==1));
      break;

    default :
      PrintErrorMessageF('E', "LogOnCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (popt)
  {
    SetLogFile(protocolFile);
    WriteLogFile("\nbeginlog\n");
    return (OKCODE);
  }

  /* get logfile name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" logon %",NAMELENSTR,"[ -~]")),logfile)!=1)
  {
    PrintErrorMessage('E',"logon","could not read name of logfile");
    return(PARAMERRORCODE);
  }
        #ifdef ModelP
  if (pext)
  {
    sprintf(logfile,"%s.p%04d",logfile,procs);
  }
  if (meext)
  {
    sprintf(logfile,"%s.%04d",logfile,me);
  }
  else if (me != master)
    return (OKCODE);
        #endif

  rv = OpenLogFile(logfile,rename);
  switch (rv)
  {
  case 0 :
    return (OKCODE);

  case 1 :
    PrintErrorMessage('E',"logon","logfile already open");
    break;

  case 2 :
    PrintErrorMessage('E',"logon","could not open logfile");
    break;

  default :
    PrintErrorMessage('E',"logon","(unknown)");
  }

  return(CMDERRORCODE);
}


/** \brief Implementation of \ref logoff. */
static INT LogOffCommand (INT argc, char **argv)
{
  INT i;
  bool popt = false;

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (protocolFile==NULL)
      {
        PrintErrorMessage('E',"logoff","no protocol file open");
        return (PARAMERRORCODE);
      }
      popt = true;
      break;

    default :
      PrintErrorMessageF('E', "LogOffCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (popt)
  {
    WriteLogFile("\nendlog\n");
    SetLogFile(NULL);
    return (OKCODE);
  }

  if (CloseLogFile()!=0)
    PrintErrorMessage('W',"logoff","no logfile open");

  return(OKCODE);
}

#ifdef __TWODIM__


/** \brief Implementation of \ref cnom. */
static INT CnomCommand (INT argc, char **argv)
{
  char docName[32],plotprocName[NAMESIZE],tagName[NAMESIZE];
  int i,flag;

  if (currMG==NULL)
  {
    PrintErrorMessage('E',"cnom","no multigrid active");
    return(CMDERRORCODE);
  }

  /* get document name */
  docName[0] = (char) 0;
  sscanf(argv[0]," cnom %31[ -~]",docName);
  if (strlen(docName)==0)
  {
    PrintErrorMessage('E',"cnom","no document name");
    return(PARAMERRORCODE);
  }
  if (argc!=2)
  {
    PrintErrorMessage('E', "CnomCommand", "specify only one argument with cnom");
    return(PARAMERRORCODE);
  }

  flag=0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'p' :
      if (sscanf(argv[i],expandfmt(CONCAT3("p %",NAMELENSTR,"[ -~]")),plotprocName)!=1)
      {
        PrintErrorMessage('E',"cnom","can't read plotprocName");
        return(PARAMERRORCODE);
      }
      flag=flag|1;
      break;
    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),tagName)!=1)
      {
        PrintErrorMessage('E',"cnom","can't read tagName");
        return(PARAMERRORCODE);
      }
      flag=flag|2;
      break;
    default :
      flag=flag|4;
      break;
    }

  if (flag!=3)
  {
    PrintErrorMessage('E', "CnomCommand", "Wrong flag value");
    return (PARAMERRORCODE);
  }


  return(SaveCnomGridAndValues(currMG,docName,plotprocName,tagName));
}
#endif


/** \brief Implementation of \ref configure. */
INT NS_DIM_PREFIX ConfigureCommand (INT argc, char **argv)
{
  BVP *theBVP;
  BVP_DESC theBVPDesc;
  char BVPName[NAMESIZE];

  /* get BVP name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" configure %",NAMELENSTR,"[ -~]")),BVPName)!=1) || (strlen(BVPName)==0))
  {
    PrintErrorMessage('E', "ConfigureCommand", "cannot read BndValProblem specification");
    return(PARAMERRORCODE);
  }

  theBVP = BVP_GetByName(BVPName);
  if (theBVP == NULL)
  {
    PrintErrorMessage('E', "ConfigureCommand", "cannot read BndValProblem specification");
    return(PARAMERRORCODE);
  }

  if (BVP_SetBVPDesc(theBVP,&theBVPDesc))
    return (CMDERRORCODE);

  if (BVPD_CONFIG(&theBVPDesc)!=NULL)
    if ((*BVPD_CONFIG(&theBVPDesc))(argc,argv))
    {
      PrintErrorMessage('E',"configure"," (could not configure BVP)");
      return(CMDERRORCODE);
    }

  return(OKCODE);
}


/** \brief Implementation of \ref close. */
static INT CloseCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i;
  bool closeonlyfirst;

  if (ResetPrintingFormat())
    REP_ERR_RETURN(CMDERRORCODE);

  closeonlyfirst = true;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      closeonlyfirst = false;
      break;

    default :
      PrintErrorMessageF('E', "CloseCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
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
        return (OKCODE);
      }
      closeonlyfirst = false;
      break;
    }

    if (DisposeMultiGrid(theMG)!=0)
    {
      PrintErrorMessage('E',"close","closing the mg failed");
      return (CMDERRORCODE);
    }
    i++;

    currMG = GetFirstMultigrid();
  }
  while (!closeonlyfirst);

  return(OKCODE);
}


/** \brief Implementation of \ref new. */
INT NS_DIM_PREFIX NewCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Multigrid[NAMESIZE],BVPName[NAMESIZE],Format[NAMESIZE];
  MEM heapSize;
  INT i;
  bool bopt, fopt, hopt, IEopt, emptyGrid;

  /* get multigrid name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" new %",NAMELENSTR,"[ -~]")),Multigrid)!=1) || (strlen(Multigrid)==0))
    sprintf(Multigrid,"untitled-%d",(int)untitledCounter++);

  theMG = GetMultigrid(Multigrid);
  if ((theMG != NULL) && (theMG == currMG)) CloseCommand(0,NULL);

  /* get problem, domain and format */
  heapSize = 0;
  bopt = fopt = hopt = false;
  IEopt = true;
  emptyGrid = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[ -~]")),BVPName)!=1)
      {
        PrintErrorMessage('E', "NewCommand", "cannot read BndValProblem specification");
        return(PARAMERRORCODE);
      }
      bopt = true;
      break;

    case 'f' :
      if (sscanf(argv[i],expandfmt(CONCAT3("f %",NAMELENSTR,"[ -~]")),Format)!=1)
      {
        PrintErrorMessage('E', "NewCommand", "cannot read format specification");
        return(PARAMERRORCODE);
      }
      fopt = true;
      break;

    case 'n' :
      IEopt = false;
      break;

    case 'e' :
      emptyGrid = true;
      break;

    case 'h' :
      if (ReadMemSizeFromString(argv[i]+1,&heapSize)!=0)           /* skip leading 'h' in argv */
      {
        PrintErrorMessage('E', "NewCommand", "cannot read heapsize specification");
        return(PARAMERRORCODE);
      }
      hopt = true;
      break;

    default :
      PrintErrorMessageF('E', "NewCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (!(bopt && fopt && hopt))
  {
    PrintErrorMessage('E', "NewCommand", "the d, p, f and h arguments are mandatory");
    return(PARAMERRORCODE);
  }

  /* allocate the multigrid structure */
  theMG = CreateMultiGrid(Multigrid,BVPName,Format,heapSize,IEopt,!emptyGrid);
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"new","could not create multigrid");
    return(CMDERRORCODE);
  }

  currMG = theMG;

  return(OKCODE);
}


/** \brief Implementation of \ref open. */
static INT OpenCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Multigrid[NAMESIZE],File[NAMESIZE],BVPName[NAMESIZE],Format[NAMESIZE],type[NAMESIZE];
  char *theBVP,*theFormat,*theMGName;
  MEM heapSize;
  INT i,force,IEopt,autosave,try_load,fqn,mgpathes_set_old;

  /* get multigrid name */
  if ((sscanf(argv[0],expandfmt(CONCAT3(" open %",NAMELENSTR,"[ -~]")),File)!=1) || (strlen(File)==0))
  {
    PrintErrorMessage('E',"open","specify the name of the file to open");
    return (PARAMERRORCODE);
  }

  /* get problem and format */
  strcpy(type,"asc");
  theBVP = theFormat = theMGName = NULL;
  heapSize = force = autosave = fqn = 0;
  try_load = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      autosave = 1;
      break;

    case 'b' :
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[ -~]")),BVPName)!=1)
      {
        PrintErrorMessage('E', "OpenCommand", "cannot read BndValProblem specification");
        return(PARAMERRORCODE);
      }
      theBVP = BVPName;
      break;

    case 'f' :
      if (sscanf(argv[i],expandfmt(CONCAT3("f %",NAMELENSTR,"[ -~]")),Format)!=1)
      {
        PrintErrorMessage('E', "OpenCommand", "cannot read format specification");
        return(PARAMERRORCODE);
      }
      theFormat = Format;
      break;

    case 'F' :
      force = 1;
      break;

    case 'n' :
      IEopt = false;
      break;

    case 'm' :
      if (sscanf(argv[i],expandfmt(CONCAT3("m %",NAMELENSTR,"[ -~]")),Multigrid)!=1)
      {
        PrintErrorMessage('E', "OpenCommand", "cannot read multigrid specification");
        return(PARAMERRORCODE);
      }
      theMGName = Multigrid;
      break;

    case 't' :
      if (strncmp(argv[i],"try",3)==0)
      {
        try_load = true;
        break;
      }
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintErrorMessage('E', "OpenCommand", "cannot read type specification");
        return(PARAMERRORCODE);
      }
      break;

    case 'h' :
      if (ReadMemSizeFromString(argv[i]+1,&heapSize)!=0)                           /* skip leading 'h' in argv */
      {
        PrintErrorMessage('E', "OpenCommand", "cannot read heapsize specification");
        return(PARAMERRORCODE);
      }
      break;

    case 'z' :
      fqn = 1;
      break;

    default :
      PrintErrorMessageF('E', "OpenCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (fqn) {
    mgpathes_set_old = mgpathes_set;
    mgpathes_set = 0;
  }

  /* allocate the multigrid structure */
  theMG = LoadMultiGrid(theMGName,File,type,theBVP,theFormat,
                        heapSize,force,IEopt,autosave);

  if (fqn) mgpathes_set = mgpathes_set_old;

  if (theMG==NULL)
  {
    PrintErrorMessage('E',"open","could not open multigrid");
    if (try_load)
      return(CMDERRORCODE);
    else
      RETURN(CMDERRORCODE);
  }
  currMG = theMG;

  return(OKCODE);
}


/** \brief Implementation of \ref save. */
static INT SaveCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Name[NAMESIZE],type[NAMESIZE],Comment[LONGSTRSIZE];
  INT i,autosave,rename,res;
  int ropt;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"save","no open multigrid");
    return (CMDERRORCODE);
  }

  /* scan name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" save %",NAMELENSTR,"[ -~]")),Name)!=1)
    strcpy(Name,ENVITEM_NAME(theMG));

  /* check options */
  autosave=rename=0;
  strcpy(Comment,NO_COMMENT);
  strcpy(type,"asc");
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      if (sscanf(argv[i],expandfmt(CONCAT3(" c %",LONGSTRLENSTR,"[ -~]")),Comment)!=1)
      {
        PrintErrorMessage('E',"save","couldn't read the comment string");
        return (PARAMERRORCODE);
      }
      break;

    case 't' :
      if (sscanf(argv[i],expandfmt(CONCAT3("t %",NAMELENSTR,"[ -~]")),type)!=1)
      {
        PrintErrorMessage('E', "SaveCommand", "cannot read type specification");
        return(PARAMERRORCODE);
      }
      break;

    case 'a' :
      autosave=1;
      break;

    case 'r' :
      res = sscanf(argv[i]," r %d",&ropt);
      if (res==0 || (res==1 && ropt==1)) rename = 1;
      break;

    default :
      PrintErrorMessageF('E', "SaveCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (SaveMultiGrid(theMG,Name,type,Comment,autosave,rename)) return (CMDERRORCODE);

  return(OKCODE);
}


/** \brief Implementation of \ref savedomain. */
static INT SaveDomainCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char Name[NAMESIZE];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"savedomain","no open multigrid");
    return (CMDERRORCODE);
  }

  /* scan name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" savedomain %",NAMELENSTR,"[ -~]")),Name)!=1)
    strcpy(Name,BVPD_NAME(MG_BVPD(theMG)));

  if (BVP_Save(MG_BVP(theMG),Name,ENVITEM_NAME(theMG),MGHEAP(theMG),argc,argv)) return (CMDERRORCODE);

  return(OKCODE);
}


/** \brief Implementation of \ref changemc. */
static INT ChangeMagicCookieCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"changemc","no open multigrid");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0]," changemc %d",&iValue)!=1)
  {
    PrintErrorMessage('E',"changemc","cannot read magic-cookie");
    return (CMDERRORCODE);
  }
  MG_MAGIC_COOKIE(theMG) = iValue;
  return(OKCODE);
}


/** \brief Implementation of \ref level. */
static INT LevelCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  /* following variables: keep type for sscanf */
  int l;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"level","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check parameters */
  if (sscanf(argv[0]," level %d",&l)==1)
  {
    if ((l<BOTTOMLEVEL(theMG)) || (l>TOPLEVEL(theMG)))
    {
      PrintErrorMessage('E',"level","level out of range");
      return (PARAMERRORCODE);
    }
    else
      CURRENTLEVEL(theMG) = l;
  }
  else if (strchr(argv[0],'+')!=NULL)
  {
    if (CURRENTLEVEL(theMG)==TOPLEVEL(theMG))
    {
      PrintErrorMessage('W',"level","already on TOPLEVEL");
      return (OKCODE);
    }
    else
      CURRENTLEVEL(theMG)++;
  }
  else if (strchr(argv[0],'-')!=NULL)
  {
    if (CURRENTLEVEL(theMG)==BOTTOMLEVEL(theMG))
    {
      PrintErrorMessage('W',"level","already on BOTTOMLEVEL");
      return (OKCODE);
    }
    else
      CURRENTLEVEL(theMG)--;
  }
  else
  {
    PrintErrorMessage('E',"level","specify <level>, + or - with the level command");
    return (CMDERRORCODE);
  }

  UserWriteF("  current level is %d (bottom level %d, top level %d)\n",
             CURRENTLEVEL(theMG),BOTTOMLEVEL(theMG),TOPLEVEL(theMG));

  return(OKCODE);
}


/** \brief Implementation of \ref renumber. */
static INT RenumberMGCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"renumber","no open multigrid");
    return (CMDERRORCODE);
  }

  if (RenumberMultiGrid(theMG,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0)!=GM_OK)
  {
    PrintErrorMessage('E',"renumber","renumbering of the mg failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref mglist. */
static INT MGListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG,*theCurrMG;
  INT i,longformat;

  theCurrMG = GetCurrentMultigrid();
  if (theCurrMG==NULL)
  {
    PrintErrorMessage('W',"mglist","no multigrid open\n");
    return (OKCODE);
  }

  longformat = true;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      longformat = false;
      break;

    case 'l' :
      /* old syntax */
      longformat = true;
      break;

    default :
      PrintErrorMessageF('E', "MGListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  ListMultiGridHeader(longformat);

  for (theMG=GetFirstMultigrid(); theMG!=NULL; theMG=GetNextMultigrid(theMG))
    ListMultiGrid(theMG,(theMG==theCurrMG),longformat);

  return (OKCODE);
}


/** \brief Implementation of \ref glist. */
static INT GListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: GListCommand(): me not in Context,"\
                     " no listing of grid\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  ListGrids(theMG);

  return (OKCODE);
}


/** \brief Implementation of \ref nlist. */
static INT NListCommand (INT argc, char **argv)
{

  MULTIGRID *theMG;
  INT i,fromN,toN,res,mode,idopt,dataopt,boundaryopt,neighbouropt,verboseopt;
  char buff[32];

  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: NListCommand(): me not in Context," \
                     " no listing of nodes\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"nlist","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  idopt = LV_ID;
  dataopt = boundaryopt = neighbouropt = verboseopt = mode = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromN = f;
      toN   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"nlist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toN = fromN;
      else if (fromN>toN)
      {
        PrintErrorMessage('E',"nlist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromN = toN = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromN = toN = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"nlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'd' :
      dataopt = true;
      break;

    case 'b' :
      boundaryopt = true;
      break;

    case 'n' :
      neighbouropt = true;
      break;

    case 'v' :
      verboseopt = true;
      break;

    default :
      PrintErrorMessageF('E', "NListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  switch (mode)
  {
  case DO_ID :
    ListNodeRange(theMG,fromN,toN,idopt,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case DO_ALL :
    ListNodeRange(theMG,0,MAX_I,idopt,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case DO_SELECTION :
    ListNodeSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  default :
    PrintErrorMessage('E',"nlist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}


/** \brief Implementation of \ref elist. */
static INT EListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,fromE,toE,res,mode,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt;
  char buff[32];

  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: EListCommand(): me not in Context," \
                     " no listing of elements\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"elist","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  idopt = LV_ID;
  dataopt = boundaryopt = neighbouropt = verboseopt = levelopt = mode = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromE = f;
      toE   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"elist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toE = fromE;
      else if (fromE>toE)
      {
        PrintErrorMessage('E',"elist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromE = toE = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromE = toE = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"elist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'l' :
      levelopt = true;
      break;

    case 'd' :
      dataopt = true;
      break;

    case 'b' :
      boundaryopt = true;
      break;

    case 'n' :
      neighbouropt = true;
      break;

    case 'v' :
      verboseopt = true;
      break;

    default :
      PrintErrorMessageF('E', "EListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  switch (mode)
  {
  case DO_ID :
    ListElementRange(theMG,fromE,toE,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt);
    break;

  case DO_ALL :
    ListElementRange(theMG,0,MAX_I,idopt,dataopt,boundaryopt,neighbouropt,verboseopt,levelopt);
    break;

  case DO_SELECTION :
    ListElementSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  default :
    PrintErrorMessage('E',"elist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}


/** \brief Implementation of \ref slist. */
static INT SelectionListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,dataopt,boundaryopt,neighbouropt,verboseopt;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SelectionListCommand(): me not in Context,"\
                     " no listing of selection\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"slist","no open multigrid");
    return (CMDERRORCODE);
  }

  if (SELECTIONSIZE(theMG)==0)
  {
    PrintErrorMessage('W',"slist","nothing selected");
    return (OKCODE);
  }

  /* check options */
  dataopt = boundaryopt = neighbouropt = verboseopt = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      dataopt = true;
      break;

    case 'b' :
      boundaryopt = true;
      break;

    case 'n' :
      neighbouropt = true;
      break;

    case 'v' :
      verboseopt = true;
      break;

    default :
      PrintErrorMessageF('E', "SelectionListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  switch (SELECTIONMODE(theMG))
  {
  case elementSelection :
    ListElementSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case nodeSelection :
    ListNodeSelection(theMG,dataopt,boundaryopt,neighbouropt,verboseopt);
    break;

  case vectorSelection :
    UserWrite("sorry, this service is not available for vector selections\n");
    break;

  default :
    PrintErrorMessage('W',"slist","selectionmode ???");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}


/** \brief Implementation of \ref rlist. */
static INT RuleListCommand (INT argc, char **argv)
{
  INT i,allopt,rn,tag;
  char etype[32];

  rn = -1;
  allopt = false;

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      allopt = true;
      break;

    default :
      PrintErrorMessageF('E', "RuleListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  /* scan parameters */
  if (allopt == false)
    sscanf(argv[0],"rlist %31[triquatethexa] %d",etype,&rn);
  else
    sscanf(argv[0],"rlist %31[triaquadtetrahexa]",etype);

  tag = -1;
        #ifdef __TWODIM__
  if (strcmp("tri",etype)==0) tag = TRIANGLE;
  if (strcmp("qua",etype)==0) tag = QUADRILATERAL;
        #endif
        #ifdef __THREEDIM__
  if (strcmp("tet",etype)==0) tag = TETRAHEDRON;
  if (strcmp("hex",etype)==0) tag = HEXAHEDRON;
        #endif

  if (tag==-1)
  {
    PrintErrorMessage('E',"rlist","wrong element type");
    return (CMDERRORCODE);
  }

  if (rn==-1 && allopt==false || rn>=0 && allopt==true)
  {
    PrintErrorMessage('E',"rlist","specify rulenumber OR $a option!");
    return (CMDERRORCODE);
  }

  if (allopt == true)
    for (i=0; i<MaxRules[tag]; i++)
      ShowRefRule(tag,i);
  else
    ShowRefRule(tag,rn);

  return(OKCODE);
}


/** \todo Please doc me! */
static INT PrintValueCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  double val;
  char name[NAMESIZE];
  char value[VALUELEN];
  int found = false;
  int idx;
  int n;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"printvalue","no open multigrid");
    return (CMDERRORCODE);
  }
  if (sscanf(argv[0],"printvalue %s %d",name,&n)!=2)
  {
    PrintErrorMessage('E',"printvalue","could not scan vec desc and selection number");
    return (PARAMERRORCODE);
  }
  theVD = GetVecDataDescByName(theMG,name);
  if (theVD==NULL)
  {
    PrintErrorMessageF('E',"printvalue","vec desc '%s' not found",name);
    return (PARAMERRORCODE);
  }

  if ((SELECTIONMODE(theMG)==vectorSelection) && (SELECTIONSIZE(theMG)>n))
  {
    VECTOR *vec = (VECTOR *)SELECTIONOBJECT(theMG,n);
    if (VD_ISDEF_IN_TYPE(theVD,VTYPE(vec)))
    {
      val = VVALUE(vec,VD_CMP_OF_TYPE(theVD,VTYPE(vec),0));
      idx = VINDEX(vec);
      found = true;
    }
  }
  if (found)
    sprintf(buffer,"%.10e",val);
  else
    sprintf(buffer,"---");

  UserWriteF("value 0 of %s in vec %d = %s\n",name,idx,buffer);
  if (ReadArgvChar("s",value,argc,argv)==0)
    if (SetStringVar(value,buffer))
    {
      PrintErrorMessageF('E',"printvalue","coul not write onto string var '%s'",value);
      return (PARAMERRORCODE);
    }

  return OKCODE;
}


/** \brief Implementation of \ref vmlist. */
static INT VMListCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  FORMAT *theFormat;
  GRID *theGrid;
  INT i,j,fl,tl,fromV,toV,res,mode,idopt,dataopt,matrixopt,vclass,vnclass,datatypes,modifiers;
  VECDATA_DESC *theVD;
  MATDATA_DESC *theMD;
  char value[VALUELEN];
  char buff[32];
  /* following variables: keep type for sscanf */
  long f,t;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: VMListCommand(): me not in Context," \
                     " no listing of VM\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"vmlist","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));
  theFormat = MGFORMAT(theMG);

  if (ReadArgvINT("vclass",&vclass,argc,argv))
    vclass = 3;
  if (ReadArgvINT("vnclass",&vnclass,argc,argv))
    vnclass = 3;

  if (ReadArgvChar("vmlist",value,argc,argv) == 0) {
    theVD = GetVecDataDescByName(theMG,value);
    if (theVD != NULL) {
      if (ReadArgvOption("S",argc,argv))
        PrintSVector(theMG,theVD);
            #ifdef __INTERPOLATION_MATRIX__
      else if (ReadArgvOption("I",argc,argv))
        PrintIMatrix(theGrid,theVD,vclass,vnclass);
            #endif
      else if (ReadArgvOption("s",argc,argv))
      {
        /* get selection list */
        if ((SELECTIONMODE(theMG)==vectorSelection) && (SELECTIONSIZE(theMG)>=1))
        {
          VECTOR **vlist = (VECTOR**)malloc((SELECTIONSIZE(theMG)+1)*sizeof(VECTOR*));
          if (vlist!=NULL)
          {
            int i;
            for (i=0; i<SELECTIONSIZE(theMG); i++)
              vlist[i] = (VECTOR *)SELECTIONOBJECT(theMG,i);
            vlist[SELECTIONSIZE(theMG)] = NULL;
            PrintVectorListX((const VECTOR **)vlist,theVD,vclass,vnclass,UserWriteF);
            free(vlist);
          }
        }
      }
      else
        PrintVector(theGrid,theVD,vclass,vnclass);
      return(OKCODE);
    }
    theMD = GetMatDataDescByName(theMG,value);
    if (theMD != NULL) {
      if (ReadArgvOption("T",argc,argv))
        PrintTMatrix(theGrid,theMD,vclass,vnclass);
      else if (ReadArgvOption("D",argc,argv))
        PrintDiagMatrix(theGrid,theMD,vclass,vnclass);
      else
        PrintMatrix(theGrid,theMD,vclass,vnclass);
      return(OKCODE);
    }
  }
  modifiers = LV_MOD_DEFAULT;
  if (ReadArgvINT("skip",&j,argc,argv)==0)
  {
    if (j)
      SET_FLAG(modifiers,LV_SKIP);
    else
      CLEAR_FLAG(modifiers,LV_SKIP);
  }
  if (ReadArgvINT("pos",&j,argc,argv)==0)
  {
    if (j)
      SET_FLAG(modifiers,LV_POS);
    else
      CLEAR_FLAG(modifiers,LV_POS);
  }
  if (ReadArgvINT("obj",&j,argc,argv)==0)
  {
    if (j)
      SET_FLAG(modifiers,LV_VO_INFO);
    else
      CLEAR_FLAG(modifiers,LV_VO_INFO);
  }

  /* check options */
  datatypes = 0;
  idopt = LV_ID;
  dataopt = matrixopt = mode = false;
  fl = tl = CURRENTLEVEL(theMG);
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'i' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromV = f;
      toV   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"vmlist","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toV = fromV;
      else if (fromV>toV)
      {
        PrintErrorMessage('E',"vmlist","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

#ifdef ModelP
    case 'g' :
      mode = DO_ID;
      idopt = LV_GID;
      res = sscanf(argv[i]," g %s",buff);
      fromV = toV = (DDD_GID) strtol(buff, NULL, 0);
      break;
#endif
    case 'k' :
      mode = DO_ID;
      idopt = LV_KEY;
      res = sscanf(argv[i]," k %s",buff);
      fromV = toV = (INT) strtol(buff, NULL, 0);
      break;

    case 's' :
      if (strncmp(argv[i],"skip",4)==0)
        /* handled by ReadArgvINT */
        break;
      if (mode!=false)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'a' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'l' :
      res = sscanf(argv[i]," l %ld %ld",&f,&t);
      fl = f;
      tl = t;
      if (res!=2)
      {
        PrintErrorMessage('E',"vmlist","specify from and to level with the l option");
        return (PARAMERRORCODE);
      }
      else if (fl>tl)
      {
        PrintErrorMessage('E',"vmlist","from level > to level");
        return (PARAMERRORCODE);
      }
      break;

    case 'd' :
      dataopt = true;
      break;

    case 't' :
      for (j=0; j<NVECTYPES; j++)
        if (FMT_S_VEC_TP(theFormat,j)>0)
          if (strchr(argv[i]+1,FMT_VTYPE_NAME(theFormat,j))!=NULL)
            datatypes |= BITWISE_TYPE(j);
      break;

    case 'm' :
      matrixopt = true;
      break;

    case 'z' :
      matrixopt = -true;
      break;

    case 'p' :
    case 'o' :
      /* handled by ReadArgvINT */
      break;

    default :
      PrintErrorMessageF('E', "VMListCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }
  if (datatypes==0)
    /* default */
    for (j=0; j<NVECTYPES; j++)
      datatypes |= BITWISE_TYPE(j);

  switch (mode)
  {
  case DO_ID :
    ListVectorRange(theMG,fl,tl,fromV,toV,idopt,matrixopt,dataopt,datatypes,modifiers);
    break;

  case DO_ALL :
    ListVectorRange(theMG,fl,tl,0,MAX_I,idopt,matrixopt,dataopt,datatypes,modifiers);
    break;

  case DO_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      ListVectorOfElementSelection(theMG,matrixopt,dataopt,modifiers);
    else
      ListVectorSelection(theMG,matrixopt,dataopt,modifiers);
    break;

  default :
    PrintErrorMessage('E',"vmlist","specify either the a, s or i option");
    return (PARAMERRORCODE);
  }

  return(OKCODE);
}


/** \todo Please doc me! */
static int ReadMatrixDimensions (char *name, int *n, int *na)
{
  int i;

  FILE *stream = fileopen(name,"r");
  if (stream == NULL) return(1);
  fscanf(stream," %d\n",n);
  for (i=0; i<=*n; i++)
    fscanf(stream," %d ",na);
  fclose(stream);

  return(0);
}

/** \todo Please doc me! */
static int ReadMatrix (char *name, int n, int *ia, int *ja, double *a)
{
  int i;

  FILE *stream = fileopen(name,"r");
  if (stream == NULL) return(1);
  fscanf(stream," %d\n",&i);
  if (i != n) return(1);
  for (i=0; i<=n; i++)
    fscanf(stream," %d ",ia+i);
  fscanf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fscanf(stream," %d ",ja+i);
  fscanf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fscanf(stream," %lf ",a+i);
  fscanf(stream,"\n");
  fclose(stream);

  return(0);
}

/** \todo Please doc me! */
static int WriteMatrix (char *name, int n, int *ia, int *ja, double *a)
{
  int i;

  FILE *stream = fileopen(name,"w");
  if (stream == NULL) return(1);
  fprintf(stream," %d\n",n);
  for (i=0; i<=n; i++)
    fprintf(stream," %d ",ia[i]);
  fprintf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fprintf(stream," %d ",ja[i]);
  fprintf(stream,"\n");
  for (i=0; i<ia[n]; i++)
    fprintf(stream," %f ",a[i]);
  fprintf(stream,"\n");
  fclose(stream);

  return(0);
}

/** \todo Please doc me! */
static int WriteMatrixfmt (char *name, int n, int *ia, int *ja, double *a,
                           int inc)
{
  int i;

  FILE *stream = fileopen(name,"w");
  if (stream == NULL) return(1);
  fprintf(stream,"%d %d",n,ia[n]+inc);
  for (i=0; i<=n; i++) {
    if ((i%10) == 0) fprintf(stream,"\n");
    fprintf(stream,"%6d",ia[i]+inc);
  }
  for (i=0; i<ia[n]; i++) {
    if ((i%3) == 0) fprintf(stream,"\n");
    fprintf(stream,"%6d %18.9f",ja[i]+inc,a[i]);
  }
  fprintf(stream,"\n");
  fclose(stream);

  return(0);
}

/** \brief Implementation of \ref convert. */
static INT ConvertCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  HEAP *theHeap;
  MATDATA_DESC *A;
  int n,nn,*ia,*ja,inc;
  double *a,*r;
  char name[32];
  INT i,j,MarkKey,symmetric,ncomp;

  theMG = GetCurrentMultigrid();
  if (theMG==NULL) {
    PrintErrorMessage('E',"convert","no current multigrid");
    return(CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));
  if ((A = ReadArgvMatDesc(theMG,"convert",argc,argv))==NULL) {
    PrintErrorMessage('E',"convert","could not read symbol");
    return (PARAMERRORCODE);
  }
  theHeap = MGHEAP(theMG);
  MarkTmpMem(theHeap,&MarkKey);
  symmetric = ReadArgvOption("symmetric",argc,argv);
  inc = ReadArgvOption("inc",argc,argv);
  if (ReadArgvINT("ncomp",&ncomp,argc,argv))
    ncomp = 1;
  if (ReadArgvChar("r",name,argc,argv) == 0) {
    if (ReadMatrixDimensions(name,&n,&nn)) {
      PrintErrorMessage('E',"convert",
                        "could not read matrix dimensions");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
    ia = (int *)GetTmpMem(theHeap,sizeof(int) * (n+1),MarkKey);
    a = (double *)GetTmpMem(theHeap,sizeof(double) * nn,MarkKey);
    ja = (int *)GetTmpMem(theHeap,sizeof(int) * nn,MarkKey);
    if ((ia == NULL) || (a == NULL) || (ja == NULL)) {
      PrintErrorMessage('E',"convert",
                        "could not allocate memory");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
    if (ReadMatrix(name,n,ia,ja,a)) {
      PrintErrorMessage('E',"convert","could write matrix");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
  }
  else if (ConvertMatrix(theGrid,MGHEAP(theMG),MarkKey,A,symmetric,
                         &n,&ia,&ja,&a)) {
    PrintErrorMessage('E',"convert","could not read matrix");
    ReleaseTmpMem(MGHEAP(theMG),MarkKey);
    return(CMDERRORCODE);
  }
  if (ReadArgvChar("f",name,argc,argv) == 0)
  {
    if (ReadArgvOption("fmt",argc,argv)) {
      if (WriteMatrixfmt(name,n,ia,ja,a,inc)) {
        PrintErrorMessage('E',"convert","could write matrix");
        ReleaseTmpMem(MGHEAP(theMG),MarkKey);
        return(CMDERRORCODE);
      }
    }
    else if (WriteMatrix(name,n,ia,ja,a)) {
      PrintErrorMessage('E',"convert","could write matrix");
      ReleaseTmpMem(MGHEAP(theMG),MarkKey);
      return(CMDERRORCODE);
    }
  }
  if (ReadArgvOption("p",argc,argv)) {
    r = (DOUBLE *)GetTmpMem(MGHEAP(theMG),sizeof(DOUBLE) * n,MarkKey);
    for (i=0; i<n; i++) {
      for (j=0; j<n; j++)
        r[j] = 0.0;
      for (j=ia[i]; j<ia[i+1]; j++)
        r[ja[j]] = a[j];
      for (j=0; j<n; j++)
        UserWriteF("%8.4f",r[j]);
      UserWrite("\n");
    }
  }
  ReleaseTmpMem(MGHEAP(theMG),MarkKey);
  return (OKCODE);
}



/** \brief Implementation of \ref in. */
static INT InsertInnerNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  DOUBLE xc[DIM];

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"in","no open multigrid");
    return (CMDERRORCODE);
  }

  if (sscanf(argv[0],"in %lf %lf %lf",xc,xc+1,xc+2)!=DIM)
  {
    PrintErrorMessageF('E',"in","specify %d coordinates for an inner node",(int)DIM);
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by InsertInnerNode() */
  if (InsertInnerNode(GRID_ON_LEVEL(theMG,0),xc)==NULL)
  {
    PrintErrorMessage('E',"in","inserting an inner node failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/** \todo Please doc me! */
static INT NGInsertInnerNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  DOUBLE xc[DIM];
  static int n;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"in","no open multigrid");
    return (CMDERRORCODE);
  }

  UserWriteF("# IPoint %d\n",n);
  n++;
  UserWriteF("# %s\n",argv[0]);
  if (sscanf(argv[0],"ngin %lf %lf %lf",xc,xc+1,xc+2)!=DIM)
  {
    PrintErrorMessageF('E',"in","specify %d coordinates for an inner node",(int)DIM);
    return (PARAMERRORCODE);
  }

  UserWriteF("I %lf %lf %lf;\n",xc[0],xc[1],xc[2]);

  return (OKCODE);
}


/** \brief Implementation of \ref bn. */
static INT InsertBoundaryNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"bn","no open multigrid");
    return (CMDERRORCODE);
  }

  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),argc,argv);
  if (bndp == NULL)
  {
    PrintErrorMessage('E',"bn","inserting a boundary point failed");
    return (CMDERRORCODE);
  }

  if (InsertBoundaryNode(GRID_ON_LEVEL(theMG,0),bndp)==NULL)
  {
    PrintErrorMessage('E',"bn","inserting a boundary node failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \todo Please doc me! */
static INT NGInsertBoundaryNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;
  static int i;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ngbn","no open multigrid");
    return (CMDERRORCODE);
  }

  UserWriteF("# BPoint %d \n",i);
  /* this works only for LGM domain and does no real insertion !!! */
  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),argc,argv);
  if (bndp == NULL)
  {
    i++;
    return (OKCODE);
  }
  return (CMDERRORCODE);
}


/** \brief Implementation of \ref gn. */
static INT InsertGlobalNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BNDP *bndp;
  int i;
  int ropt = false;
  int err = OKCODE;
  DOUBLE resolution;
  INT n = 2;
  INT my_argc = 0;
  char **my_argv;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"gn","no open multigrid");
    return (CMDERRORCODE);
  }

  /* assemble command line for bn */
  if (ReadArgvDOUBLE("r",&resolution,argc,argv)==0)
  {
    ropt = true;
    n++;
  }
  my_argv = (char**)malloc(n*sizeof(char*));
  if (my_argv==NULL)
    return CMDERRORCODE;
  my_argv[my_argc] = StrDup(argv[0]);
  if (my_argv[my_argc]==NULL)
  {
    err = CMDERRORCODE;
    goto Exit_gn;
  }
  my_argv[my_argc][0] = 'b';                    /* gn --> bn */
  my_argc++;

  my_argv[my_argc] = StrDup("g");
  if (my_argv[my_argc]==NULL)
  {
    err = CMDERRORCODE;
    goto Exit_gn;
  }
  my_argc++;

  if (ropt)
  {
    char r_opt[64];
    sprintf(r_opt,"$r %g",resolution);
    my_argv[my_argc] = StrDup(r_opt);
    if (my_argv[my_argc]==NULL)
    {
      err = CMDERRORCODE;
      goto Exit_gn;
    }
    my_argc++;
  }
  ASSERT(n==my_argc);

  /* try inserting a boundary node */
  bndp = BVP_InsertBndP (MGHEAP(theMG),MG_BVP(theMG),my_argc,my_argv);
  if (bndp == NULL)
  {
    double x[DIM_MAX];
    DOUBLE xc[DIM];

    /* failed: try inserting an inner node */
    if (sscanf(argv[0],"gn %lf %lf %lf",x,x+1,x+2)!=DIM)
    {
      PrintErrorMessageF('E',"gn","specify %d global coordinates",(int)DIM);
      err = PARAMERRORCODE;
      goto Exit_gn;
    }
    for (i=0; i<DIM; i++)
      xc[i] = x[i];

    /* NB: toplevel=0 is checked by InsertInnerNode() */
    if (InsertInnerNode(GRID_ON_LEVEL(theMG,0),xc)==NULL)
    {
      PrintErrorMessage('E',"gn","inserting an inner node failed");
      err = CMDERRORCODE;
      goto Exit_gn;
    }
    UserWrite("  ### gn: inserted a in\n");
  }
  else if (InsertBoundaryNode(GRID_ON_LEVEL(theMG,0),bndp)==NULL)
  {
    PrintErrorMessage('E',"gn","inserting a boundary node failed");
    err =  CMDERRORCODE;
    goto Exit_gn;
  }
  else
    UserWrite("  ### gn: inserted a bn\n");

Exit_gn:
  for (i=0; i<my_argc; i++)
    if (my_argv[i]!=NULL)
      free(my_argv[i]);
  free(my_argv);

  return err;
}


/** \brief Implementation of \ref deln. */
static INT DeleteNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT sopt,i;

  /* following variables: keep type for sscanf */
  int id;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"deln","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  sopt = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      sopt = true;
      break;

    default :
      PrintErrorMessageF('E', "DeleteNodeCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (sopt)
  {
    if (SELECTIONMODE(theMG)==nodeSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if (DeleteNode(GRID_ON_LEVEL(theMG,0),(NODE *)SELECTIONOBJECT(theMG,i))!=GM_OK)
        {
          PrintErrorMessage('E',"deln","deleting the node failed");
          return (CMDERRORCODE);
        }
    ClearSelection(theMG);

    return (OKCODE);
  }

  if (sscanf(argv[0],"deln %d",&id)!=1)
  {
    PrintErrorMessage('E',"deln","specify the ID of the node to be deleted");
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by DeleteNode() */
  if (DeleteNodeWithID(GRID_ON_LEVEL(theMG,0),id)!=GM_OK)
  {
    PrintErrorMessage('E',"deln","deleting the node failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref move. */
static INT MoveNodeCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VERTEX *myVertex;
  NODE *theNode;
  DOUBLE xc[DIM];
  INT type,i,j,level,relative;

  /* following variables: keep type for sscanf */
  double x[DIM_MAX];
  int id,segid;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"move","no open multigrid");
    return (CMDERRORCODE);
  }

  theNode = NULL;

  /* check parameters */
  if (sscanf(argv[0],"move %d",&id)==1)
  {
    /* search node */
    for (level=0; level<=TOPLEVEL(theMG); level++)
      if ((theNode=FindNodeFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
        break;
    if (theNode==NULL)
    {
      PrintErrorMessageF('E',"move","node with ID %ld not found",(long)id);
      return (CMDERRORCODE);
    }
  }

  /* check options */
  relative = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      if (SELECTIONMODE(theMG)==nodeSelection)
      {
        PrintErrorMessage('E',"move","there is no node in the selection");
        return (PARAMERRORCODE);
      }
      if (SELECTIONSIZE(theMG)!=1)
      {
        PrintErrorMessage('E',"move","there is more than one node in the selection");
        return (PARAMERRORCODE);
      }
      theNode = (NODE *)SELECTIONOBJECT(theMG,0);
      break;

    case 'i' :
      if (OBJT(MYVERTEX(theNode))!=IVOBJ)
      {
        PrintErrorMessageF('E',"move","node with ID %ld is no inner node",(long)id);
        return (CMDERRORCODE);
      }
      type = IVOBJ;
      if (sscanf(argv[i],"i %lf %lf %lf",x,x+1,x+2)!=DIM)
      {
        PrintErrorMessageF('E',"move","specify %d new coordinates for an inner node",(int)DIM);
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        xc[j] = x[j];
      break;

    case 'b' :
      if (OBJT(MYVERTEX(theNode))!=BVOBJ)
      {
        PrintErrorMessageF('E',"move","node with ID %ld is no boundary node",(long)id);
        return (CMDERRORCODE);
      }
      type = BVOBJ;
      if (sscanf(argv[i],"b %d %lf %lf",&segid,x,x+1)!=1+DIM_OF_BND)
      {
        PrintErrorMessageF('E',"move","specify the segment if and %d new coordinates for a boundary node",(int)DIM_OF_BND);
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM_OF_BND; j++)
        xc[j] = x[j];
      break;

    case 'r' :
      relative = true;
      break;

    default :
      PrintErrorMessageF('E', "MoveNodeCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (theNode==NULL)
  {
    PrintErrorMessage('E',"move","you have to either specify\n"
                      "the ID of the node to move or the s option");
    return (PARAMERRORCODE);
  }

  myVertex = MYVERTEX(theNode);
  if (type==IVOBJ)
  {
    if (relative)
    {
      /* move relative to old position */
      for (j=0; j<DIM; j++)
        xc[j] += CVECT(myVertex)[j];
    }
    if (MoveNode(theMG,theNode,xc,true)!=GM_OK)
    {
      PrintErrorMessage('E',"move","failed moving the node");
      return (CMDERRORCODE);
    }
  }
  else if (type==BVOBJ)
  {
    PrintErrorMessage('E',"move","moving boundary nodes not implemented yet");
    return (CMDERRORCODE);
  }
  else
  {
    PrintErrorMessage('E', "MoveNodeCommand", "either i or b option is mandatory");
    return (PARAMERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref ie. */
static INT InsertElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  NODE *theNode,*theNodes[MAX_CORNERS_OF_ELEM];
  char *token,*vstr;
  INT i,nNodes,Id[MAX_CORNERS_OF_ELEM];

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ie","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  nNodes = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      if (SELECTIONMODE(theMG)==nodeSelection)
        for (nNodes=0; nNodes<SELECTIONSIZE(theMG); nNodes++)
        {
          theNode = (NODE *)SELECTIONOBJECT(theMG,nNodes);
          if (nNodes>=MAX_CORNERS_OF_ELEM)
          {
            PrintErrorMessage('E',"ie","too many nodes are in the selection");
            return (CMDERRORCODE);
          }
          theNodes[nNodes] = theNode;
        }
      else
      {
        PrintErrorMessage('E',"ie","objects other than nodes are in the selection");
        return (PARAMERRORCODE);
      }
      if (nNodes==0)
      {
        PrintErrorMessage('E',"ie","no nodes are in the selection");
        return (PARAMERRORCODE);
      }
      break;

    default :
      PrintErrorMessageF('E', "InsertElementCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  /* got the nodes via s option? */
  if (nNodes>0)
  {
    if (InsertElement(GRID_ON_LEVEL(theMG,0),nNodes,theNodes,NULL,NULL,NULL)==NULL)
    {
      PrintErrorMessage('E',"ie","inserting the element failed");
      return (CMDERRORCODE);
    }
    else
    {
      return (OKCODE);
    }
  }

  /* set vstr after 'ie' */
  if ((vstr=strchr(argv[0],'e'))!=NULL)
    ++vstr;
  else
    return (CMDERRORCODE);

  /* we need the string split into tokens */
  nNodes = 0;
  token = strtok(vstr,WHITESPACE);
  while (token!=NULL)
  {
    if (nNodes>=MAX_CORNERS_OF_ELEM)
    {
      PrintErrorMessageF('E',"ie","specify at most %d id's",(int)MAX_CORNERS_OF_ELEM);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    if (sscanf(token," %d",&id)!=1)
    {
      PrintErrorMessageF('E',"ie","could not read the id of corner no %d",(int)i);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    Id[nNodes++] = id;
    token = strtok(NULL,WHITESPACE);
  }

  /* NB: toplevel=0 is checked by InsertElementFromIDs() */
  if (InsertElementFromIDs(GRID_ON_LEVEL(theMG,0),nNodes,Id,NULL)==NULL)
  {
    PrintErrorMessage('E',"ie","inserting the element failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}

/** \todo Please doc me! */
static INT NGInsertElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char *token,*vstr;
  INT i,bf,nNodes,Id[MAX_CORNERS_OF_ELEM];
  static int n;

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ngie","no open multigrid");
    return (CMDERRORCODE);
  }

  /* set vstr after 'ngie' */
  if ((vstr=strchr(argv[0],'e'))!=NULL)
    ++vstr;
  else
    return (CMDERRORCODE);

  UserWriteF("# %s\n",argv[0]);
  UserWriteF("# element %d\n",n);
  n++;
  UserWriteF("E ");

  /* we need the string split into tokens */
  nNodes = bf = 0;
  token = strtok(vstr,WHITESPACE);
  i = 0;
  while (token!=NULL)
  {
    /* separator for boundary faces */
    if (strcmp(token,"F") == 0)
    {
      UserWriteF("\n");
      bf = 1;
      goto NEXTTOKEN;
    }

    /* read next boundary face */
    if (bf > 0)
    {

      if (sscanf(token," %d",&id)!=1)
      {
        PrintErrorMessageF('E',"ngie","could not read the id of boundary face no %d",(int)bf);
        return (PARAMERRORCODE);                                        /* too many items */
      }
      UserWriteF("	F");
      switch (nNodes)
      {
      case 4 :
      case 5 :
      case 6 :
        UserWriteF("ngie: elementtype = %d not implemented!\n",nNodes);
        break;
      case 8 :
      {
        INT n;
        /* assert(id<SIDES_OF_ELEM_TAG(nNodes));*/

        for (n=0; n<CORNERS_OF_SIDE_TAG(7,id); n++)
        {
          UserWriteF(" %d",Id[CORNER_OF_SIDE_TAG(7,id,n)]);
        }
        UserWriteF("\n");
      }
      break;
      default :
        assert(0);
      }

      bf++;
      goto NEXTTOKEN;
    }

    /* read next node */
    if (nNodes>=MAX_CORNERS_OF_ELEM)
    {
      PrintErrorMessageF('E',"ngie","specify at most %d id's",(int)MAX_CORNERS_OF_ELEM);
      return (PARAMERRORCODE);                                  /* too many items */
    }
    if (sscanf(token," %d",&id)!=1)
    {
      PrintErrorMessageF('E',"ngie","could not read the id of corner no %d",(int)nNodes);
      return (PARAMERRORCODE);                                  /* too many items */
    }

    /* first id is subdomain */
    if (i > 0)
    {
      Id[nNodes] = id;
      nNodes++;
    }
    UserWriteF(" %d",id);

NEXTTOKEN:
    token = strtok(NULL,WHITESPACE);
    i++;
  }

  UserWriteF(";\n");

  return (OKCODE);
}


/** \brief Implementation of \ref dele. */
static INT DeleteElementCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,sopt;

  /* following variables: keep type for sscanf */
  int id;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"dele","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  sopt = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 's' :
      sopt = true;
      break;

    default :
      PrintErrorMessageF('E', "DeleteElementCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (sopt)
  {
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if (DeleteElement(theMG,(ELEMENT *)SELECTIONOBJECT(theMG,i))!=GM_OK)
        {
          PrintErrorMessage('E',"dele","deleting the element failed");
          return (CMDERRORCODE);
        }
    ClearSelection(theMG);

    return (OKCODE);
  }

  if (sscanf(argv[0],"dele %d",&id)!=1)
  {
    PrintErrorMessage('E',"dele","specify the ID of the element to be deleted");
    return (PARAMERRORCODE);
  }

  /* NB: toplevel=0 is checked by DeleteElement() */
  if (DeleteElementWithID(theMG,id)!=GM_OK)
  {
    PrintErrorMessage('E',"dele","deleting the element failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref adapt. */
static INT AdaptCommand (INT argc, char **argv)
{
  MULTIGRID       *theMG;
  INT i,mode,mark,rv;
  INT seq;
  INT mgtest;
  EVECTOR         *theElemEvalDirection;

        #ifdef ModelP
  if (!CONTEXT(me))
  {
    PRINTDEBUG(ui,0,("%2d: AdaptCommand(): me not in Context,"
                     " grid not refined\n",me))
    return (OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"adapt","no open multigrid");
    return (CMDERRORCODE);
  }

  /* init defaults */
  seq = GM_REFINE_PARALLEL;
  mgtest = GM_REFINE_NOHEAPTEST;

  /* check options */
  theElemEvalDirection = NULL;
  mode = GM_REFINE_TRULY_LOCAL;
  mark = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      mark = MARK_ALL;
      break;

            #ifdef __THREEDIM__
    case 'd' :
      if (sscanf(argv[i],"a %s",buffer)==1)
        theElemEvalDirection = GetElementVectorEvalProc(buffer);
      if (theElemEvalDirection==NULL)
        UserWrite("direction eval fct not found: taking shortest interior edge\n");
      break;
                        #endif

    case 'g' :
      mode = mode | GM_COPY_ALL;
      break;

    case 'h' :
      mode = mode | GM_REFINE_NOT_CLOSED;
      break;

    case 's' :
      seq = GM_REFINE_SEQUENTIAL;
      break;

    case 't' :
      mgtest = GM_REFINE_HEAPTEST;
      break;

    default :
      PrintErrorMessageF('E', "AdaptCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

        #ifdef ModelP
  /* currently only this is supported in parallel */
  if (0)
    if (procs > 1)
    {
      mark = MARK_ALL;
      mode = mode | GM_REFINE_NOT_CLOSED;
    }
        #endif

  if (mark == MARK_ALL)
  {
    INT l,nmarked;
    ELEMENT *theElement;

    nmarked = 0;

    for (l=TOPLEVEL(theMG); l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l)); theElement!=NULL; theElement=SUCCE(theElement))
      {
        if (EstimateHere(theElement))
        {
          if ((rv = MarkForRefinement(theElement,RED,0))!=0)
          {
            l = TOPLEVEL(theMG);
            break;
          }
          else
            nmarked++;
        }
      }
    UserWriteF("%d: %d elements marked for regular refinement\n",me,nmarked);
  }

  /* get velocity */
        #ifdef __THREEDIM__
  SetAlignmentPtr (theMG, theElemEvalDirection);
        #endif

  rv = AdaptMultiGrid(theMG,mode,seq,mgtest);

  switch (rv)
  {
  case GM_OK :
    UserWriteF(" %s refined\n",ENVITEM_NAME(theMG));
    SetStringVar(":errno","0");
    return (OKCODE);

  case GM_COARSE_NOT_FIXED :
    PrintErrorMessage('E',"refine","do 'fixcoarsegrid' first and then refine!");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  case GM_ERROR :
    PrintErrorMessage('E',"refine","could not refine, data structure still ok");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  case GM_FATAL :
    PrintErrorMessage('F',"refine","could not refine, data structure inconsistent\n");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);

  default :
    PrintErrorMessage('E',"refine","unknown error in refine");
    SetStringVar(":errno","1");
    return (CMDERRORCODE);
  }
}

/** \brief Implementation of \ref fixcoarsegrid. */
static INT FixCoarseGridCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  theMG = currMG;

  PRINTDEBUG(ui,2,("%d: FixCoarseGrid currMG %x fixed %d\n",
                   me,theMG,MG_COARSE_FIXED(theMG)));

  if (theMG==NULL) {
    PrintErrorMessage('E',"fixcoarsegrid","no open multigrid");
    return (CMDERRORCODE);
  }
  if (FixCoarseGrid(theMG)) return (CMDERRORCODE);

  PRINTDEBUG(ui,2,("%d: FixCoarseGrid currMG %x fixed %d\n",
                   me,theMG,MG_COARSE_FIXED(theMG)));

  return(OKCODE);
}


/** \brief Implementation of \ref collapse. */
static INT CollapseCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

  theMG = currMG;

  if (theMG==NULL) {
    PrintErrorMessage('E',"collapse","no open multigrid");
    return (CMDERRORCODE);
  }
  if (Collapse(theMG)) return (CMDERRORCODE);

  return(OKCODE);
}


/** \brief Implementation of \ref mark. */
static INT MarkCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  ELEMENT *theElement;
  char rulename[32];
  INT i,j,l,mode,rv,sid;
  enum RefinementRule Rule;
  DOUBLE_VECTOR global;
  DOUBLE x,y;
  long nmarked;
#       ifdef __THREEDIM__
  DOUBLE X,Y;
  DOUBLE z,Z;
#       endif

  /* following variables: keep type for sscanf */
  int id,idfrom,idto;
  INT Side;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%d: MarkCommand() me not in Context," \
                     " nothing marked\n",me))
    return (OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"mark","no open multigrid");
    return (CMDERRORCODE);
  }

  /* first check help option */
  for (i=1; i<argc; i++)
    if (argv[i][0]=='h')
    {
      UserWrite("the following rules are available:\n");
      for (j=0; j<NO_OF_RULES && myMR[j].RuleName!=NULL; j++)
      {
        UserWrite(myMR[j].RuleName);
        UserWrite("\n");
      }
      return (OKCODE);
    }

  /* scan parameters */
  /*rv = sscanf(argv[0],"mark %31[redbluecopycoarsnoi_123qtpritet2hex] %d",rulename,&Side);*/
  rv = sscanf(argv[0],"mark %31[a-z_0-9] %d",rulename,&Side);
  if (rv<1)
  {
    /* set the default rule */
    strcpy(rulename,myMR[0].RuleName);
    Rule = (enum RefinementRule)myMR[0].RuleId;
    Side = NO_SIDE_SPECIFIED;
  }
  else
  {
    Rule = (enum RefinementRule)NO_RULE_SPECIFIED;
    for (i=0; i<NO_OF_RULES; i++)
      if (strcmp(rulename,myMR[i].RuleName)==0)
      {
        Rule = (enum RefinementRule)myMR[i].RuleId;
        break;
      }

    if (Rule==NO_RULE_SPECIFIED)
    {
      PrintErrorMessageF('E',"mark","unknown rule '%s'",rulename);
      return (PARAMERRORCODE);
    }

    if (rv!=2)
      Side = NO_SIDE_SPECIFIED;
  }

  if (ReadArgvOption("c",argc, argv))
  {
    for (i=0; i<=TOPLEVEL(theMG); i++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,i));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          MarkForRefinement(theElement,NO_REFINEMENT,0);

    UserWrite("all refinement marks removed\n");

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("x",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (XC(MYVERTEX(CORNER(theElement,j))) < x)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in x < %f marked for refinement\n",
               (float) x);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("X",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (XC(MYVERTEX(CORNER(theElement,j))) > x)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in x > %f marked for refinement\n",
               (float) x);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("y",&y,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (YC(MYVERTEX(CORNER(theElement,j))) < y)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in y < %f marked for refinement\n",
               (float) y);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("Y",&y,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (YC(MYVERTEX(CORNER(theElement,j))) > y)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in y > %f marked for refinement\n",
               (float) y);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("stripes",&x,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
        {
          INT flag=1;
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
          {
            DOUBLE xc=YC(MYVERTEX(CORNER(theElement, j)));
            xc=fmod(xc,(4*x));
            if ((xc<0.9*x)||(xc>2.1*x)) flag=0;
          }
          if(flag)
            MarkForRefinement(theElement,Rule,0);
        }
    UserWriteF("stripes %f\n", (float) x);

    return(OKCODE);
  }

  if (ReadArgvINT("S",&sid,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (SUBDOMAIN(theElement) == sid)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in subdomain %d marked for refinement\n",sid);

    return(OKCODE);
  }

#ifdef __THREEDIM__
  if ((ReadArgvDOUBLE("x0",&x,argc, argv)==0) &&
      (ReadArgvDOUBLE("x1",&X,argc, argv)==0) &&
      (ReadArgvDOUBLE("y0",&y,argc, argv)==0) &&
      (ReadArgvDOUBLE("y1",&Y,argc, argv)==0) &&
      (ReadArgvDOUBLE("z0",&z,argc, argv)==0) &&
      (ReadArgvDOUBLE("z1",&Z,argc, argv)==0))
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if ((XC(MYVERTEX(CORNER(theElement,j))) < X) &&
                (XC(MYVERTEX(CORNER(theElement,j))) > x) &&
                (YC(MYVERTEX(CORNER(theElement,j))) < Y) &&
                (YC(MYVERTEX(CORNER(theElement,j))) > y) &&
                (ZC(MYVERTEX(CORNER(theElement,j))) < Z) &&
                (ZC(MYVERTEX(CORNER(theElement,j))) > z))
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in box marked for refinement\n");

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("z",&z,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (ZC(MYVERTEX(CORNER(theElement,j))) < z)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in z < %f marked for refinement\n",
               (float) z);

    return(OKCODE);
  }

  if (ReadArgvDOUBLE("Z",&z,argc, argv)==0)
  {
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement))
        if (EstimateHere(theElement))
          for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            if (ZC(MYVERTEX(CORNER(theElement,j))) > z)
              MarkForRefinement(theElement,Rule,0);

    UserWriteF("all elements in z > %f marked for refinement\n",
               (float) z);

    return(OKCODE);
  }

#endif

  if (ReadArgvPosition("pos",argc,argv,global)==0)
  {
    DOUBLE r;

    if (ReadArgvDOUBLE("r",&r,argc, argv)==0) {
      for (l=0; l<=TOPLEVEL(theMG); l++)
        for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
             theElement!=NULL; theElement=SUCCE(theElement))
          if (EstimateHere(theElement))
            for (j=0; j<CORNERS_OF_ELEM(theElement); j++)
            {
              DOUBLE dist;

              V_DIM_EUKLIDNORM_OF_DIFF(global,
                                       CVECT(MYVERTEX(CORNER(theElement,j))),dist);
              if (dist <= r) {
                MarkForRefinement(theElement,Rule,0);
                break;
              }
            }
      UserWriteF("all elements in |x - p|  < %f marked for refinement\n",
                 (float) r);

      return(OKCODE);
    }
    theElement = FindElementOnSurface(theMG,global);
    if (theElement != NULL) {
      MarkForRefinement(theElement,Rule,0);
        #ifdef ModelP
      j = (INT) UG_GlobalSumDOUBLE(1.0);
      i = DDD_InfoGlobalId(PARHDRE(theElement));
    }
    else {
      j = (INT) UG_GlobalSumDOUBLE(0.0);
      i = -1;
    }
    if (j == 0)
      return(PARAMERRORCODE);

    for (l=0; l<j; l++) {
      rv = UG_GlobalMaxINT(i);
      UserWriteF("element GID %08x marked for refinement\n",rv);
      if (rv == i) i = -1;
    }
        #else
      UserWriteF("element %d marked for refinement\n",ID(theElement));
    }
    else
      return(PARAMERRORCODE);
            #endif

    return (OKCODE);
  }

  /* check options */
  mode = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_ALL;
      break;

    case 'i' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_ID;
      j = sscanf(argv[i],"i %d %d", &idfrom, &idto);
      if (j<1 || j>2)
      {
        PrintErrorMessage('E',"mark","cannot scan id(s)");
        return (PARAMERRORCODE);
      }
      if (j == 1) idto = idfrom;
      break;

    case 's' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"mark","specify only one option of a, b, i, s");
        return (PARAMERRORCODE);
      }
      mode = MARK_SELECTION;
      break;

    default :
      PrintErrorMessageF('E', "MarkCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (mode==false)
  {
    PrintErrorMessage('E',"mark","specify exactly one option of a, b, i, s");
    return (PARAMERRORCODE);
  }

  /* print rule and side */
  if (Side==NO_SIDE_SPECIFIED)
    UserWriteF("   using rule %s (no side given)\n",rulename);
  else
    UserWriteF("   using rule %s, side %d\n",rulename,(int)Side);

  nmarked = 0;
  switch (mode)
  {
  case MARK_ALL :
    for (l=0; l<=TOPLEVEL(theMG); l++)
      for (theElement=FIRSTELEMENT(GRID_ON_LEVEL(theMG,l));
           theElement!=NULL; theElement=SUCCE(theElement)) {
        if (EstimateHere(theElement))
        {
          if ((rv = MarkForRefinement(theElement,
                                      Rule,Side))!=0)
          {
            l = TOPLEVEL(theMG);
            break;
          }
          else
            nmarked++;
        }
      }
    break;

  case MARK_ID :
    theElement = NULL;
    for (id=idfrom; id<=idto; id++) {
      for (l=0; l<=TOPLEVEL(theMG); l++)
        if ((theElement=FindElementFromId(GRID_ON_LEVEL(theMG,l),id))!=NULL)
          break;

      if (theElement==NULL)
      {
        PrintErrorMessageF('W',"mark","element with ID %ld could not be found, nothing marked",id);
      }

      if (EstimateHere(theElement))
      {
        if ((rv = MarkForRefinement(theElement,
                                    Rule,Side))!=0)
          break;
        else
          nmarked++;
      }
    }
    break;

  case MARK_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
      {
        theElement = (ELEMENT *)SELECTIONOBJECT(theMG,i);
        if (EstimateHere(theElement))
        {
          if ((rv = MarkForRefinement(theElement,
                                      Rule,Side))!=0)
            break;
          else
            nmarked++;
        }
      }
    break;
  }

        #ifdef ModelP
  nmarked = UG_GlobalSumINT(nmarked);
        #endif
  UserWriteF(" %ld elements marked for refinement\n",nmarked);

  if (rv && theElement)
  {
    PrintErrorMessageF('W',"mark","rule could not be applied for element with ID %ld, nothing marked",ID(theElement));
    return (CMDERRORCODE);
  }
  else
    return(OKCODE);
}


/** \brief Implementation of \ref ordernodes. */
static INT OrderNodesCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT i,res,level,fromLevel,toLevel;
  INT sign[DIM],order[DIM],xused,yused,zused,error,AlsoOrderLinks;
  char ord[3];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"ordernodes","no open multigrid");
    return (CMDERRORCODE);
  }
  fromLevel = 0;
  toLevel   = TOPLEVEL(theMG);

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(argv[0],expandfmt("ordernodes %2[rlud]"),ord);
        #else
  res = sscanf(argv[0],expandfmt("ordernodes %3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintErrorMessage('E', "OrderNodesCommand", "could not read order type");
    return(PARAMERRORCODE);
  }
  if (strlen(ord)!=DIM)
  {
    PrintErrorMessage('E', "OrderNodesCommand", "specify DIM chars out of 'rlud' or 'rlbfud' resp.");
    return(PARAMERRORCODE);
  }
  error = xused = yused = zused = false;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = true;
      xused = true;
      order[i] = _X_; sign[i] =  1; break;
    case 'l' :
      if (xused) error = true;
      xused = true;
      order[i] = _X_; sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] =  1; break;
    case 'd' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] =  1; break;
    case 'f' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] = -1; break;

    case 'u' :
      if (zused) error = true;
      zused = true;
      order[i] = _Z_; sign[i] =  1; break;
    case 'd' :
      if (zused) error = true;
      zused = true;
      order[i] = _Z_; sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintErrorMessage('E', "OrderNodesCommand", "bad combination of 'rludr' or 'rlbfud' resp.");
    return(PARAMERRORCODE);
  }

  /* check options */
  AlsoOrderLinks = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'l' :
      res = sscanf(argv[i],"l %d",&level);
      if (res!=1)
      {
        PrintErrorMessage('E',"ordernodes","could not read level");
        return(PARAMERRORCODE);
      }
      if ((level>=fromLevel)&&(level<=toLevel))
        fromLevel = toLevel = level;
      else
      {
        PrintErrorMessage('E',"ordernodes","level out of range");
        return(PARAMERRORCODE);
      }
      break;

    case 'L' :
      AlsoOrderLinks = true;
      break;

    default :
      PrintErrorMessageF('E', "OrderNodesCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  /* first we renumber the multigrid (to have node-IDs coinciding with lists) */
  if (RenumberMultiGrid(theMG,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0)!=GM_OK)
  {
    PrintErrorMessage('E',"ordernodes","renumbering of the mg failed");
    return (CMDERRORCODE);
  }

  /* now we reorder the nodes on the specified levels */
  for (level=fromLevel; level<=toLevel; level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

    UserWriteF(" [%d:",level);

    if (OrderNodesInGrid(theGrid,order,sign,AlsoOrderLinks)!=GM_OK)
    {
      PrintErrorMessage('E',"ordernodes","OrderNodesInGrid failed");
      return (CMDERRORCODE);
    }
    UserWrite("o]");
  }

  UserWrite("\n");

  return (OKCODE);
}



/** \brief Implementation of \ref lexorderv. */
static INT LexOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT i,res,level,fromLevel,toLevel;
  INT sign[DIM],order[DIM],which,xused,yused,zused,rused,pused,error,AlsoOrderMatrices,SpecialTreatSkipVecs,mode;
  char ord[3];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"lexorderv","no open multigrid");
    return (CMDERRORCODE);
  }
  fromLevel = 0;
  toLevel   = TOPLEVEL(theMG);

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(argv[0],expandfmt("lexorderv %2[rludIOPN]"),ord);
        #else
  res = sscanf(argv[0],expandfmt("lexorderv %3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintErrorMessage('E', "LexOrderVectorsCommand", "could not read order type");
    return(PARAMERRORCODE);
  }
  if (strlen(ord)!=DIM)
  {
    PrintErrorMessage('E', "LexOrderVectorsCommand", "specify DIM chars out of 'rlud', 'IOPN' or 'rlbfud' resp.");
    return(PARAMERRORCODE);
  }
  error = xused = yused = zused = rused = pused = false;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = true;
      xused = true;
      order[i] = _X_; sign[i] =  1; break;
    case 'l' :
      if (xused) error = true;
      xused = true;
      order[i] = _X_; sign[i] = -1; break;

    case 'u' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] =  1; break;
    case 'd' :
      if (yused) error = true;
      yused = true;
      order[i] = _Y_; sign[i] = -1; break;

#ifdef __THREEDIM__
    case 'b' :
      if (zused) error = true;
      zused = true;
      order[i] = _Z_; sign[i] =  1; break;
    case 'f' :
      if (zused) error = true;
      zused = true;
      order[i] = _Z_; sign[i] = -1; break;
#endif

                        #ifdef __TWODIM__

    /* polar coordiante directions */
    case 'I' :                          /* capital i */
      if (rused) error = true;
      rused = true;
      order[i] = 0; sign[i] =  1; break;

    case 'O' :
      if (rused) error = true;
      rused = true;
      order[i] = 0; sign[i] = -1; break;

    case 'P' :
      if (pused) error = true;
      pused = true;
      order[i] = 1; sign[i] =  1; break;

    case 'N' :
      if (pused) error = true;
      pused = true;
      order[i] = 1; sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintErrorMessage('E', "LexOrderVectorsCommand", "bad combination of 'rludr' or 'rlbfud' resp.");
    return(PARAMERRORCODE);
  }
  mode = OV_CARTES;
  if (rused || pused)
  {
    if (!(rused && pused))
    {
      PrintErrorMessage('E', "LexOrderVectorsCommand", "bad combination of cartesian/polar direction");
      return(PARAMERRORCODE);
    }
    else
      mode = OV_POLAR;
  }

  /* check options */
  AlsoOrderMatrices = SpecialTreatSkipVecs = false;
  which = GM_TAKE_SKIP | GM_TAKE_NONSKIP;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'l' :
      res = sscanf(argv[i],"l %d",&level);
      if (res!=1)
      {
        PrintErrorMessage('E',"lexorderv","could not read level");
        return(PARAMERRORCODE);
      }
      if ((level>=fromLevel)&&(level<=toLevel))
        fromLevel = toLevel = level;
      else
      {
        PrintErrorMessage('E',"lexorderv","level out of range");
        return(PARAMERRORCODE);
      }
      break;

    case 'm' :
      AlsoOrderMatrices = true;
      break;

    case 'w' :
      which = 0;
      if (strchr(argv[i],'s')!=NULL)
        which |= GM_TAKE_SKIP;
      if (strchr(argv[i],'n')!=NULL)
        which |= GM_TAKE_NONSKIP;
      break;

    case 's' :
      if              (strchr(argv[i],'<')!=NULL)
        SpecialTreatSkipVecs = GM_PUT_AT_BEGIN;
      else if (strchr(argv[i],'>')!=NULL)
        SpecialTreatSkipVecs = GM_PUT_AT_END;
      else if (strchr(argv[i],'0')!=NULL)
        SpecialTreatSkipVecs = false;
      else
      {
        PrintErrorMessage('E',"lexorderv","use < or > with s-option");
        return(PARAMERRORCODE);
      }
      break;

    default :
      PrintErrorMessageF('E', "LexOrderVectorsCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  /* now we reorder the vectors on the specified levels */
  for (level=fromLevel; level<=toLevel; level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

    UserWriteF(" [%d:",level);

    if (LexOrderVectorsInGrid(theGrid,mode,order,sign,which,SpecialTreatSkipVecs,AlsoOrderMatrices)!=GM_OK)
    {
      PrintErrorMessage('E',"lexorderv","LexOrderVectorsInGrid failed");
      return (CMDERRORCODE);
    }
    UserWrite("ov]");
  }

  UserWrite("\n");

  return (OKCODE);
}


/** \brief Implementation of \ref shellorderv. */
static INT ShellOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  VECTOR *seed;
  char option;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"shellorderv","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  if (sscanf(argv[0],"shellorderv %c",&option)!=1)
  {
    PrintErrorMessage('E',"shellorderv","specify f, l or s");
    return (CMDERRORCODE);
  }

  switch (option)
  {
  case 'f' :
    seed = FIRSTVECTOR(theGrid);
    break;
  case 'l' :
    seed = LASTVECTOR(theGrid);
    break;
  case 's' :
    if (SELECTIONMODE(theMG)!=vectorSelection)
    {
      PrintErrorMessage('E',"shellorderv","no vector selection");
      return (CMDERRORCODE);
    }
    if (SELECTIONSIZE(theMG)!=1)
    {
      PrintErrorMessage('E',"shellorderv","select ONE vector");
      return (CMDERRORCODE);
    }
    seed = (VECTOR *)SELECTIONOBJECT(theMG,0);
    break;
  default :
    PrintErrorMessage('E',"shellorderv","specify f, l or s");
    return (CMDERRORCODE);
  }

  if (ShellOrderVectors(theGrid,seed)!=0)
  {
    PrintErrorMessage('E',"shellorderv","ShellOrderVectors failed");
    return (CMDERRORCODE);
  }
  else
  {
    l_setindex(theGrid);
    return (OKCODE);
  }
}


/** \brief Implementation of \ref orderv. */
static INT OrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char modestr[7];              /* <-- with change of size also change format string in sscanf! */
  char *dep,*dep_opt,*cut;
  INT mode,i,levels,PutSkipFirst,SkipPat;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"orderv","no open multigrid");
    return (CMDERRORCODE);
  }

  levels           = GM_CURRENT_LEVEL;
  mode             = false;
  dep              = dep_opt = cut = NULL;
  PutSkipFirst = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'm' :
      if (sscanf(argv[i],"m %6[FCL]",modestr)!=1)
      {
        PrintErrorMessage('E', "OrderVectorsCommand", "could not read the mode");
        return (PARAMERRORCODE);
      }
      if (strcmp(modestr,"FCFCLL")==0)
        mode = GM_FCFCLL;
      else if (strcmp(modestr,"FFLLCC")==0)
        mode = GM_FFLLCC;
      else if (strcmp(modestr,"FFLCLC")==0)
        mode = GM_FFLCLC;
      else if (strcmp(modestr,"CCFFLL")==0)
        mode = GM_CCFFLL;
      else
      {
        PrintErrorMessage('E', "OrderVectorsCommand",
                          "you have to specify FFLLCC, FFLCLC, CCFFLL or FCFCLL as mode");
        return (PARAMERRORCODE);
      }
      break;

    case 'd' :
      /* skip leading blanks */
      dep = argv[i]+1;
      while ((*dep!='\0') && (strchr(WHITESPACE,*dep)!=NULL))
        dep++;
      break;

    case 'o' :
      /* skip leading blanks */
      dep_opt = argv[i]+1;
      while ((*dep_opt!='\0') && (strchr(WHITESPACE,*dep_opt)!=NULL))
        dep_opt++;
      break;

    case 'c' :
      /* skip leading blanks */
      cut = argv[i]+1;
      while ((*cut!='\0') && (strchr(WHITESPACE,*cut)!=NULL))
        cut++;
      break;

    case 's' :
      PutSkipFirst = true;
      if (sscanf(argv[i],"s %x",&iValue)!=1)
      {
        PrintErrorMessage('E',"orderv","could not read skip pattern");
        return(PARAMERRORCODE);
      }
      SkipPat = iValue;
      break;

    case 'a' :
      levels = GM_ALL_LEVELS;
      break;

    default :
      PrintErrorMessageF('E', "OrderVectorsCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (mode==false)
  {
    PrintErrorMessage('E',"orderv","the m option is mandatory");
    return (PARAMERRORCODE);
  }

  if (dep==NULL)
  {
    UserWrite("WARNING: no dependency specified\n");
    if (dep_opt!=NULL)
    {
      UserWrite("WARNING: ignore specified options for dependency\n");
      dep_opt=NULL;
    }
  }

  if (dep!=NULL && dep_opt==NULL)
  {
    PrintErrorMessage('E',"orderv","the o option is mandatory if dopt specified");
    return (PARAMERRORCODE);
  }

  if (OrderVectors(theMG,levels,mode,PutSkipFirst,SkipPat,dep,dep_opt,cut)!=GM_OK)
  {
    PrintErrorMessage('E',"orderv","order vectors failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref revvecorder. */
static INT RevertVecOrderCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT i,from,to,l;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"revvecorder","no open multigrid");
    return (CMDERRORCODE);
  }

  from = to = CURRENTLEVEL(theMG);
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      from = 0;
      break;

    default :
      PrintErrorMessageF('E', "RevertVecOrderCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  for (l=from; l<=to; l++)
  {
    RevertVecOrder(GRID_ON_LEVEL(theMG,l));
    UserWriteF(" [%d:rev]",l);
  }
  UserWrite("\n");

  return (OKCODE);
}


/** \brief Implementation of \ref lineorderv. */
static INT LineOrderVectorsCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char *dep,*dep_opt,*cut;
  INT i,levels,verboselevel;
  int iValue;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"lineorderv","no open multigrid");
    return (CMDERRORCODE);
  }

  levels           = GM_CURRENT_LEVEL;
  dep              = dep_opt = cut = NULL;
  verboselevel = 0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      /* skip leading blanks */
      dep = argv[i]+1;
      while ((*dep!='\0') && (strchr(WHITESPACE,*dep)!=NULL))
        dep++;
      break;

    case 'o' :
      /* skip leading blanks */
      dep_opt = argv[i]+1;
      while ((*dep_opt!='\0') && (strchr(WHITESPACE,*dep_opt)!=NULL))
        dep_opt++;
      break;

    case 'c' :
      /* skip leading blanks */
      cut = argv[i]+1;
      while ((*cut!='\0') && (strchr(WHITESPACE,*cut)!=NULL))
        cut++;
      break;

    case 'a' :
      levels = GM_ALL_LEVELS;
      break;

    case 'v' :
      if (sscanf(argv[i],"v %d",&iValue)!=1)
      {
        PrintErrorMessage('E',"lineorderv","specify integer with v option");
        return (CMDERRORCODE);
      }
      verboselevel = iValue;
      break;

    default :
      PrintErrorMessageF('E', "LineOrderVectorsCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (dep==NULL)
  {
    PrintErrorMessage('E',"lineorderv","the d option is mandatory");
    return (PARAMERRORCODE);
  }

  if (dep_opt==NULL)
  {
    PrintErrorMessage('E',"lineorderv","the o option is mandatory");
    return (PARAMERRORCODE);
  }

  if (LineOrderVectors(theMG,levels,dep,dep_opt,cut,verboselevel)!=GM_OK)
  {
    PrintErrorMessage('E',"lineorderv","order vectors failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref setindex. */
static INT SetIndexCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"setindex","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  if (l_setindex(theGrid)!=0)
  {
    PrintErrorMessage('E',"setindex","l_setindex failed");
    return (CMDERRORCODE);
  }
  else
    return (OKCODE);
}


/** \brief Implementation of \ref find. */
static INT FindCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  NODE *theNode;
  VECTOR *theVector;
  ELEMENT *theElement;
  DOUBLE xc[DIM],tolc[DIM];
  INT i,j,select,isNode,isElement,isVector;

  /* following variables: keep type for sscanf */
  double x[DIM_MAX],tol;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"find","no open multigrid");
    return (CMDERRORCODE);
  }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  /* check pararmeters */
  if (sscanf(argv[0],"find %lf %lf %lf",x,x+1,x+2)!=DIM)
  {
    PrintErrorMessage('E', "FindCommand", "could not get coordinates");
    return (PARAMERRORCODE);
  }
  for (i=0; i<DIM; i++)
    xc[i] = x[i];

  /* check options */
  select = isNode = isElement = isVector = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'n' :
      if (sscanf(argv[i],"n %lf",&tol)!=1)
      {
        PrintErrorMessage('E', "FindCommand", "could not read tolerance");
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        tolc[j] = tol;
      theNode = FindNodeFromPosition(theGrid,xc,tolc);
      if (theNode==NULL)
      {
        PrintErrorMessage('W',"find","no node is matching");
        return (CMDERRORCODE);
      }
      isNode = true;
      break;

    case 'v' :
      if (sscanf(argv[i],"v %lf",&tol)!=1)
      {
        PrintErrorMessage('E', "FindCommand", "could not read tolerance");
        return (PARAMERRORCODE);
      }
      for (j=0; j<DIM; j++)
        tolc[j] = tol;
      theVector = FindVectorFromPosition(theGrid,xc,tolc);
      if (theVector==NULL)
      {
        PrintErrorMessage('W',"find","no vector is matching");
        return (CMDERRORCODE);
      }
      isVector = true;
      break;

    case 'e' :
      theElement = FindElementFromPosition(theGrid,xc);
      if (theElement==NULL)
      {
        PrintErrorMessage('W',"find","no element is matching");
        return (CMDERRORCODE);
      }
      isElement = true;
      break;

    case 's' :
      select = true;
      break;

    default :
      PrintErrorMessageF('E', "FindCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  if (select)
  {
    if (isNode)
      if (AddNodeToSelection(theMG,theNode)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the node failed");
        return (CMDERRORCODE);
      }

    if (isVector)
      if (AddVectorToSelection(theMG,theVector)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the vector failed");
        return (CMDERRORCODE);
      }

    if (isElement)
      if (AddElementToSelection(theMG,theElement)!=GM_OK)
      {
        PrintErrorMessage('E',"find","selecting the element failed");
        return (CMDERRORCODE);
      }
  }
  else
  {
    if (isNode)
      ListNode(theMG,theNode,false,false,false,false);

    if (isVector)
      ListVector(theMG,theVector,false,false,LV_MOD_DEFAULT);

    if (isElement)
      ListElement(theMG,theElement,false,false,false,false);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref select. */
static INT SelectCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  NODE *theNode;
  ELEMENT *theElement;
  VECTOR *theVector;
  INT i,level;
  char c;

  /* following variables: keep type for sscanf */
  int id;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: SelectCommand(): me not in Context," \
                     " no selection of elements\n",me))
    return(OKCODE);
  }
        #endif

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"select","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      ClearSelection(theMG);
      break;

    case 'n' :
      if (sscanf(argv[i],"n %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search node */
        theNode = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theNode=FindNodeFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theNode==NULL)
        {
          PrintErrorMessageF('E',"select","node with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddNodeToSelection(theMG,theNode)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the node failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==nodeSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theNode = (NODE *)SELECTIONOBJECT(theMG,i);
            if (ID(theNode)==id)
              break;
          }
        if (theNode==NULL)
        {
          PrintErrorMessageF('E',"select","node with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveNodeFromSelection(theMG,theNode)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the node failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'e' :
      if (sscanf(argv[i],"e %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search element */
        theElement = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theElement=FindElementFromId(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theElement==NULL)
        {
          PrintErrorMessageF('E',"select","element with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddElementToSelection(theMG,theElement)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the element failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==elementSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theElement = (ELEMENT *)SELECTIONOBJECT(theMG,i);
            if (ID(theElement)==id)
              break;
          }
        if (theElement==NULL)
        {
          PrintErrorMessageF('E',"select","element with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveElementFromSelection(theMG,theElement)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the element failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'v' :
      if (sscanf(argv[i],"v %c %d",&c,&id)!=2)
      {
        PrintErrorMessage('E',"select","could not get +/- or ID");
        return (PARAMERRORCODE);
      }
      if (c=='+')
      {
        /* search vector */
        theVector = NULL;
        for (level=0; level<=TOPLEVEL(theMG); level++)
          if ((theVector=FindVectorFromIndex(GRID_ON_LEVEL(theMG,level),id))!=NULL)
            break;
        if (theVector==NULL)
        {
          PrintErrorMessageF('E',"select","vector with ID %ld not found",(long)id);
          return (CMDERRORCODE);
        }
        if (AddVectorToSelection(theMG,theVector)!=GM_OK)
        {
          PrintErrorMessage('E',"select","selecting the vector failed");
          return (CMDERRORCODE);
        }
      }
      else if (c=='-')
      {
        if (SELECTIONMODE(theMG)==vectorSelection)
          for (i=0; i<SELECTIONSIZE(theMG); i++)
          {
            theVector = (VECTOR *)SELECTIONOBJECT(theMG,i);
            if (ID(theVector)==id)
              break;
          }
        if (theVector==NULL)
        {
          PrintErrorMessageF('E',"select","vector with ID %ld is not in selection",(long)id);
          return (CMDERRORCODE);
        }
        if (RemoveVectorFromSelection(theMG,theVector)!=GM_OK)
        {
          PrintErrorMessage('E',"select","removing the vector failed");
          return (CMDERRORCODE);
        }
      }
      else
      {
        PrintErrorMessage('E',"select","specify + or - with n option");
        return (PARAMERRORCODE);
      }
      break;

    case 'i' :
      if (SELECTIONSIZE(theMG)==0)
        UserWrite("nothing selected\n");
      else switch (SELECTIONMODE(theMG))
        {
        case elementSelection :
          UserWriteF("%d elements selected (use for example 'elist $s')\n",SELECTIONSIZE(theMG));
          break;
        case nodeSelection :
          UserWriteF("%d nodes selected (use for example 'nlist $s')\n",SELECTIONSIZE(theMG));
          break;
        case vectorSelection :
          UserWriteF("%d vectors selected (use for example 'vmlist $s')\n",SELECTIONSIZE(theMG));
          break;
        default :
          UserWrite("unknown selection type\n");
        }
      break;

    default :
      PrintErrorMessageF('E', "SelectCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  return (OKCODE);
}


/** \brief Implementation of \ref extracon. */
static INT ExtraConnectionCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  VECTOR *vec;
  MATRIX *mat;
  INT Delete,i,nextra,nc;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"extracon","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  Delete = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'd' :
      Delete = true;
      break;

    default :
      PrintErrorMessageF('E', "ExtraConnectionCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }
  theGrid = GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG));

  /* count extra connections on current level */
  nextra = 0;
  for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
    for (mat=MNEXT(VSTART(vec)); mat!=NULL; mat=MNEXT(mat))
      if (CEXTRA(MMYCON(mat)))
        nextra++;
  nextra /= 2;                  /* have been counted twice */

  nc = NC(theGrid);
        #ifdef ModelP
  nextra = UG_GlobalSumINT(nextra);
  nc = UG_GlobalSumINT(nc);
        #endif

  UserWriteF("%d extra connections on level %d (total %d)\n",
             (int)nextra,(int)CURRENTLEVEL(theMG),(int)NC(theGrid));

  SetStringValue(":extraconratio",nextra/((DOUBLE)nc));

  if (Delete)
  {
    if (DisposeExtraConnections(theGrid)!=GM_OK)
    {
      PrintErrorMessage('E',"extracon","deleting extra connections failed");
      return (CMDERRORCODE);
    }
    UserWrite("...deleted\n");
  }

  return (OKCODE);
}



/** \brief Implementation of \ref check. */
static INT CheckCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  INT checkgeom,checkalgebra,checklists,checkbvp,checknp;
        #ifdef ModelP
  INT checkif;
        #endif
  INT level,err,i;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"check","no open multigrid");
    return (CMDERRORCODE);
  }

  /* set default options */
  checkgeom = true;
  checkalgebra = checklists = checkbvp = checknp = false;
        #ifdef ModelP
  checkif = false;
        #endif

  /* read options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      checkgeom = checkalgebra = checklists = checknp = true;
                                #ifdef ModelP
      checkif = true;
                                #endif
      break;

    case 'g' :
      checkgeom = true;
      break;

    case 'c' :
      checkalgebra = true;
      break;

    case 'l' :
      checklists = true;
      break;

                        #ifdef ModelP
    case 'i' :
      checkif = true;
      break;
                        #endif

    case 'b' :
      checkbvp = true;
      break;

    case 'n' :
      checknp = true;
      break;

    case 'w' :
      ListAllCWsOfAllObjectTypes(UserWriteF);
      break;

    default :
      if (!checknp) {
        PrintErrorMessageF('E', "CheckCommand", "Unknown option '%s'", argv[i]);
        return (PARAMERRORCODE);
      }
    }
  err = 0;

  /* check BVP if */
  if (checkbvp==true)
    if (BVP_Check (MG_BVP(theMG)))
      err++;

  for (level=0; level<=TOPLEVEL(theMG); level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    UserWriteF("[%d:",(int)level);

                #ifndef ModelP
    if (CheckGrid(theGrid,checkgeom,checkalgebra,checklists)!=GM_OK)
                #else
    if (CheckGrid(theGrid,checkgeom,checkalgebra,checklists,checkif)!=GM_OK)
                #endif
      err++;

    UserWrite("]\n");
  }
  UserWrite("\n");

  if (checknp)
    if (CheckNP(theMG,argc,argv))
      err++;

  if (err)
    return (CMDERRORCODE);
  else
    return (OKCODE);
}



/****************************************************************************/
/** \brief Calculate minimal and maximal angle of an element
 *
 * @param theMG - pointer to multigrid
 * @param theElement - pointer to Element
 *
   This function calculates the minimal and the maximal angle of elements
   and lists elements with angle \< or \> given angles.


 */
/****************************************************************************/

INT NS_DIM_PREFIX QualityElement (MULTIGRID *theMG, ELEMENT *theElement)
{
  INT error;

  min = 360.0;
  max = 0.0;

  if ((error=MinMaxAngle(theElement,&min,&max))!=GM_OK)
    return(error);
  minangle = MIN(min,minangle);
  maxangle = MAX(max,maxangle);
  if ((lessopt && (min<themin)) && (greateropt && (max>themax)))
  {
    UserWrite(minmaxtext);
    ListElement(theMG,theElement,false,false,false,false);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }
  else if (lessopt && (min<themin))
  {
    UserWrite(mintext);
    ListElement(theMG,theElement,false,false,false,false);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }
  else if (greateropt && (max>themax))
  {
    UserWrite(maxtext);
    ListElement(theMG,theElement,false,false,false,false);
    if (selectopt) AddElementToSelection(theMG,theElement);
  }

  return(0);
}

/** \brief Implementation of \ref quality. */
static INT QualityCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *theGrid;
  ELEMENT *theElement;
  INT error,i,fromE,toE,res,mode;

  /* following variables: keep type for sscanf */
  double angle;
  long f,t;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"quality","no open multigrid");
    return (CMDERRORCODE);
  }

  /* check options */
  lessopt = greateropt = selectopt = mode = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ALL;
      break;

    case 'i' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_ID;
      res = sscanf(argv[i]," i %ld %ld",&f,&t);
      fromE = f;
      toE   = t;
      if (res<1)
      {
        PrintErrorMessage('E',"quality","specify at least one id with the i option");
        return (PARAMERRORCODE);
      }
      else if (res==1)
        toE = fromE;
      else if (fromE>toE)
      {
        PrintErrorMessage('E',"quality","from ID > to ID");
        return (PARAMERRORCODE);
      }
      break;

    case '<' :
      lessopt = true;
      if (sscanf(argv[i],"< %lf",&angle)!=1)
      {
        PrintErrorMessage('E',"quality","could not get angle of < option");
        return (CMDERRORCODE);
      }
      themin = angle;
      break;

    case '>' :
      greateropt = true;
      if (sscanf(argv[i],"> %lf",&angle)!=1)
      {
        PrintErrorMessage('E',"quality","could not get angle of > option");
        return (CMDERRORCODE);
      }
      themax = angle;
      break;

    case 's' :
      if (mode!=false)
      {
        PrintErrorMessage('E',"quality","specify either the a, s or i option");
        return (PARAMERRORCODE);
      }
      mode = DO_SELECTION;
      break;

    case 'S' :
      selectopt = true;
      ClearSelection(theMG);
      break;

    default :
      PrintErrorMessageF('E', "QualityCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  sprintf(mintext," < %g: ",(float)themin);
  sprintf(maxtext," > %g: ",(float)themax);
  sprintf(minmaxtext," < %g and > %g: ",(float)themin,(float)themax);

  minangle =      MAX_D;
  maxangle = -MAX_D;

  error=0;
  switch (mode)
  {
  case DO_ID :
    for (theGrid=GRID_ON_LEVEL(theMG,0); theGrid!=NULL; theGrid=UPGRID(theGrid))
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
        if ((ID(theElement)>=fromE) && (ID(theElement)<=toE))
          if ((error=QualityElement(theMG,theElement))!=0)
            break;
    break;

  case DO_ALL :
    for (theGrid=GRID_ON_LEVEL(theMG,0); theGrid!=NULL; theGrid=UPGRID(theGrid))
      for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
        if ((error=QualityElement(theMG,theElement))!=0)
          break;
    break;

  case DO_SELECTION :
    if (SELECTIONMODE(theMG)==elementSelection)
      for (i=0; i<SELECTIONSIZE(theMG); i++)
        if ((error=QualityElement(theMG,(ELEMENT *)SELECTIONOBJECT(theMG,i)))!=0)
          break;
    break;

  default :
    PrintErrorMessage('E',"quality","specify one option of a, s or i");
    return (PARAMERRORCODE);
  }

  if (error)
  {
    PrintErrorMessage('E',"quality","error in QualityElement/MinMaxAngle");
    return (CMDERRORCODE);
  }

  UserWriteF(" min angle = %20.12f\n max angle = %20.12f\n",(float)minangle,(float)maxangle);

  return(OKCODE);
}

/** \brief Implementation of \ref fiflel. */
#ifdef __THREEDIM__
static INT FindFlippedElementsCommand(INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT verbose;

  theMG = GetCurrentMultigrid();
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"fiflel","no current multigrid");
    return (CMDERRORCODE);
  }

  /* verbose mode */
  verbose = ReadArgvOption("v",argc,argv);

  if(FindFlippedElements(theMG,verbose))
    return (CMDERRORCODE);

  return(OKCODE);
}
#endif

/** \brief Implementation of \ref status. */
static INT StatusCommand  (INT argc, char **argv)
{
  MULTIGRID       *theMG;
  INT i,grid,green,load,verbose;

  /* get current multigrid */
  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"status command","no open multigrid");
    return (CMDERRORCODE);
  }
  grid = green = load = 0;
  verbose = 1;

  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      grid = 1;
      green = 1;
                                #ifdef ModelP
      load = 1;
                                #endif
      break;
    case 'g' :
      green = 1;
      break;
                        #ifdef ModelP
    case 'l' :
      load = 1;
      sscanf(argv[i],"l %d",&load);
      break;
                        #endif
    case 'm' :
      grid = 1;
      break;
    default :
      break;
    }

  if (MultiGridStatus(theMG,grid,green,load,verbose) != 0)
  {
    PrintErrorMessage('E',"GridStatus()","execution failed");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref setcurrmg. */
static INT SetCurrentMultigridCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  char mgname[NAMESIZE];

  NO_OPTION_CHECK(argc,argv);

  /* get multigrid name */
  if (sscanf(argv[0],expandfmt(CONCAT3(" setcurrmg %",NAMELENSTR,"[ -~]")),mgname)!=1)
  {
    PrintErrorMessage('E', "SetCurrentMultigridCommand", "specify current multigrid name");
    return(PARAMERRORCODE);
  }

  theMG = GetMultigrid(mgname);

  if (theMG==NULL)
  {
    PrintErrorMessage('E',"setcurrmg","no multigrid with this name open");
    return (CMDERRORCODE);
  }

  if (SetCurrentMultigrid(theMG)!=0)
    return (CMDERRORCODE);

  return(OKCODE);
}


/** \brief Implementation of \ref updateDoc. */
static INT UpdateDocumentCommand (INT argc, char **argv)
{
        #ifdef ModelP
  if (me!=master) return (OKCODE);
        #endif

  NO_OPTION_CHECK(argc,argv);

  return (OKCODE);
}


/** \brief Implementation of \ref clear. */
static INT ClearCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  VECTOR *v;
  INT i,l,fl,tl,n,skip,xflag;
  int j;
  double value;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"clear","no current multigrid");
    return(CMDERRORCODE);
  }
  theVD = ReadArgvVecDesc(theMG,"clear",argc,argv);
  if (theVD == NULL) {
    PrintErrorMessage('E',"clear","could not read data descriptor");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("d",argc,argv)) {
    for (i=theMG->bottomLevel; i<=TOPLEVEL(theMG); i++)
      ClearVecskipFlags(GRID_ON_LEVEL(theMG,i),theVD);
    return (OKCODE);
  }
  if (ReadArgvOption("r",argc,argv)) {
    i = CURRENTLEVEL(theMG);
    l_dsetrandom(GRID_ON_LEVEL(theMG,i),theVD,EVERY_CLASS,1.0);
    if (ReadArgvOption("d",argc,argv))
      ClearDirichletValues(GRID_ON_LEVEL(theMG,i),theVD);
    return (OKCODE);
  }
  /* check options */
  fl = tl = CURRENTLEVEL(theMG);
  skip = false;
  xflag = -1;
  value = 0.0;
  j = -1;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      fl = 0;
      break;

    case 's' :
      skip = true;
      break;

    case 'x' :
      xflag = 0;
      break;

    case 'y' :
      xflag = 1;
      break;

    case 'z' :
      xflag = 2;
      break;

    case 'i' :
      if (sscanf(argv[i],"i %d",&j)!=1)
      {
        PrintErrorMessage('E',"clear","could not read value");
        return(CMDERRORCODE);
      }
      break;

    case 'v' :
      if (sscanf(argv[i],"v %lf",&value)!=1)
      {
        PrintErrorMessage('E',"clear","could not read value");
        return(CMDERRORCODE);
      }
      break;

    default :
      PrintErrorMessageF('E', "ClearCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }
  if (j >= 0) {
    for (v = FIRSTVECTOR(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)));
         v != NULL; v = SUCCVC(v)) {
      n = VD_NCMPS_IN_TYPE(theVD,VTYPE(v));
      if (j < n) {
        VVALUE(v,VD_CMP_OF_TYPE(theVD,VTYPE(v),j)) = value;
        return (OKCODE);
      }
      j -= n;
    }
    return (CMDERRORCODE);
  }
  if (xflag != -1) {
    for (l=fl; l<=tl; l++)
      for (v=FIRSTVECTOR(GRID_ON_LEVEL(theMG,l)); v!=NULL; v=SUCCVC(v))
      {
        DOUBLE_VECTOR pos;

        if (VD_NCMPS_IN_TYPE(theVD,VTYPE(v)) == 0) continue;
        if (VectorPosition(v,pos)) continue;
        VVALUE(v,VD_CMP_OF_TYPE(theVD,VTYPE(v),0)) = pos[xflag];
      }
    return (OKCODE);
  }
  if (skip) {
    if (a_dsetnonskip(theMG,fl,tl,theVD,EVERY_CLASS,value)
        !=NUM_OK)
      return (CMDERRORCODE);
  }
  else {
    if (dset(theMG,fl,tl,ALL_VECTORS,theVD,value)!=NUM_OK)
      return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref makevdsub. */
static INT MakeVDsubCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD,*subVD;
  VEC_TEMPLATE *vt;
  INT sub;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"makevdsub","no current multigrid");
    return(CMDERRORCODE);
  }

  theVD = ReadArgvVecDescX(theMG,"makevdsub",argc,argv,NO);

  if (theVD == NULL) {
    PrintErrorMessage('E',"makevdsub","could not read data descriptor");
    return (PARAMERRORCODE);
  }
  vt = ReadArgvVecTemplateSub(MGFORMAT(theMG),"sub",argc,argv,&sub);
  if (vt==NULL)
    REP_ERR_RETURN(PARAMERRORCODE);

  if (VDsubDescFromVT(theVD,vt,sub,&subVD))
    REP_ERR_RETURN(CMDERRORCODE);

  UserWriteF("sub descriptor '%s' for '%s' created\n",ENVITEM_NAME(subVD),ENVITEM_NAME(theVD));

  return (OKCODE);
}


/** \brief Implementation of \ref rand. */
static INT RandCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  GRID *g;
  VECDATA_DESC *theVD;
  INT i,fl,tl,skip;
  double from_value,to_value;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"rand","no current multigrid");
    return(CMDERRORCODE);
  }

  /* check options */
  fl = tl = CURRENTLEVEL(theMG);
  skip = 0;
  from_value = 0.0;
  to_value = 1.0;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'a' :
      fl = 0;
      break;

    case 's' :
      skip = 1;
      break;

    case 'f' :
      if (sscanf(argv[i],"f %lf",&from_value)!=1)
      {
        PrintErrorMessage('E',"rand","could not read from value");
        return(CMDERRORCODE);
      }
      break;

    case 't' :
      if (sscanf(argv[i],"t %lf",&to_value)!=1)
      {
        PrintErrorMessage('E',"rand","could not read to value");
        return(CMDERRORCODE);
      }
      break;

    default :
      PrintErrorMessageF('E', "RandCommand", "Unknown option '%s'", argv[i]);
      return (PARAMERRORCODE);
    }

  theVD = ReadArgvVecDesc(theMG,"rand",argc,argv);

  if (theVD == NULL)
  {
    PrintErrorMessage('E',"rand","could not read data descriptor");
    return (PARAMERRORCODE);
  }

  for (i=fl; i<=tl; i++)
  {
    g = GRID_ON_LEVEL(theMG,i);
    if (l_dsetrandom2(g,theVD,EVERY_CLASS,(DOUBLE)from_value,(DOUBLE)to_value,skip)) return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref copy. */
static INT CopyCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *from,*to;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  from = ReadArgvVecDescX(theMG,"f",argc,argv,NO);
  to = ReadArgvVecDesc(theMG,"t",argc,argv);

  if (from == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (to == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }

  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dcopy(theMG,fl,tl,ALL_VECTORS,to,from) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref add. */
static INT AddCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  y = ReadArgvVecDesc(theMG,"y",argc,argv);

  if (x == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (y == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dadd(theMG,fl,tl,ALL_VECTORS,x,y) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref sub. */
static INT SubCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  INT fl,tl;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"copy","no current multigrid");
    return(CMDERRORCODE);
  }
  fl = tl = CURRENTLEVEL(theMG);
  if (argc<3 || argc>4)
  {
    PrintErrorMessage('E',"copy","specify exactly the f and t option");
    return(PARAMERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  y = ReadArgvVecDesc(theMG,"y",argc,argv);

  if (x == NULL) {
    PrintErrorMessage('E',"copy","could not read 'f' symbol");
    return (PARAMERRORCODE);
  }
  if (y == NULL) {
    PrintErrorMessage('E',"copy","could not read 't' symbol");
    return (PARAMERRORCODE);
  }
  if (ReadArgvOption("a",argc,argv)) fl = 0;

  if (dsub(theMG,fl,tl,ALL_VECTORS,x,y) != NUM_OK)
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref homotopy. */
static INT HomotopyCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *x,*y;
  DOUBLE mu;
  DOUBLE v[MAX_VEC_COMP];
  INT i;

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"homotopy","no current multigrid");
    return(CMDERRORCODE);
  }

  x = ReadArgvVecDesc(theMG,"x",argc,argv);
  if (x == NULL) {
    PrintErrorMessage('E',"homotopy","could not read 'x' symbol");
    return (PARAMERRORCODE);
  }
  y = ReadArgvVecDesc(theMG,"y",argc,argv);
  if (y == NULL) {
    PrintErrorMessage('E',"homotopy","could not read 'y' symbol");
    return (PARAMERRORCODE);
  }

  if (ReadArgvDOUBLE("v",&mu,argc,argv))
    return (PARAMERRORCODE);

  if (ReadArgvOption("a",argc,argv))
  {
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = 1.0 - mu;
    if (a_dscale(theMG,0,CURRENTLEVEL(theMG),x,EVERY_CLASS,v)!=NUM_OK)
      return (CMDERRORCODE);
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = mu;
    if (a_daxpy(theMG,0,CURRENTLEVEL(theMG),x,EVERY_CLASS,v,y)!=NUM_OK)
      return (CMDERRORCODE);
  }
  else
  {
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = 1.0 - mu;
    if (l_dscale(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)),
                 x,EVERY_CLASS,v)!=NUM_OK)
      return (CMDERRORCODE);
    for (i=0; i<VD_NCOMP(x); i++)
      v[i] = mu;
    if (l_daxpy(GRID_ON_LEVEL(theMG,CURRENTLEVEL(theMG)),
                x,EVERY_CLASS,v,y)!=NUM_OK)
      return (CMDERRORCODE);
  }

  return (OKCODE);
}


/** \brief Implementation of \ref interpolate. */
static INT InterpolateCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  VECDATA_DESC *theVD;
  INT lev,currlev;

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"interpolate","no current multigrid");
    return(CMDERRORCODE);
  }

  theVD = ReadArgvVecDescX(theMG,"interpolate",argc,argv,NO);

  if (theVD == NULL) {
    PrintErrorMessage('E',"interpolate","could not read symbol");
    return (PARAMERRORCODE);
  }

  currlev = CURRENTLEVEL(theMG);
  for (lev=1; lev<=currlev; lev++)
    if (StandardInterpolateNewVectors(GRID_ON_LEVEL(theMG,lev),theVD)!=NUM_OK)
      return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref reinit. */
static INT ReInitCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  BVP *theBVP;
  BVP_DESC theBVPDesc,*theBVPD;
  INT i,bopt;
  char BVPName[NAMESIZE];

  /* check options */
  bopt = false;
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'b' :
      if (argv[i][1]!=' ') continue;
      if (sscanf(argv[i],expandfmt(CONCAT3("b %",NAMELENSTR,"[0-9a-zA-Z/_ ]")),BVPName)!=1)
      {
        PrintErrorMessage('E',"reinit","could not read BndValProblem string");
        return(PARAMERRORCODE);
      }
      bopt = true;
      break;

      /* no default because param list is passed to reinit function */
    }

  /* set the document */
  if (bopt)
  {
    theBVP = BVP_GetByName(BVPName);
    if(theBVP==NULL)
    {
      PrintErrorMessageF('E',"reinit","could not interpret '%s' as a BVP name",BVPName);
      return (CMDERRORCODE);
    }
    if (BVP_SetBVPDesc(theBVP,&theBVPDesc)) return (CMDERRORCODE);
    theBVPD = &theBVPDesc;
  }
  else
  {
    theMG = currMG;
    if (theMG==NULL)
    {
      PrintErrorMessage('E',"reinit","no open multigrid (specify problem and domain instead)");
      return (CMDERRORCODE);
    }
    theBVP = MG_BVP(theMG);
    theBVPD = MG_BVPD(theMG);
  }

  if (BVPD_CONFIG(theBVPD)!=NULL)
    if ((*BVPD_CONFIG (theBVPD))(argc,argv)!=0)
      return (CMDERRORCODE);

  return(OKCODE);
}

/** \brief Implementation of \ref resetCEstat. */
static INT ResetCEstatCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  ResetCEstatistics();

  return (OKCODE);
}


/** \brief Implementation of \ref printCEstat. */
static INT PrintCEstatCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  PrintCEstatistics();

  return (OKCODE);
}


/** \brief Implementation of \ref heapstat. */
static INT HeapStatCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: HeapStatCommand(): me not in Context,"\
                     " no heap stat\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  HeapStat((const HEAP *)MGHEAP(theMG));

  return (OKCODE);
}



/** \brief Implementation of \ref getheapused. */
static INT GetHeapUsedCommand (INT argc, char **argv)
{
  MULTIGRID *theMG;
  INT used;

        #ifdef ModelP
  if (!CONTEXT(me)) {
    PRINTDEBUG(ui,0,("%2d: GetHeapUsedCommand(): me not in Context,"\
                     " no heap info\n",me))
    return(OKCODE);
  }
        #endif

  NO_OPTION_CHECK(argc,argv);

  theMG = currMG;
  if (theMG==NULL)
  {
    UserWrite("no multigrid open\n");
    return (OKCODE);
  }

  used = (INT)HeapUsed(MGHEAP(theMG));

        #ifdef ModelP
  used = UG_GlobalMaxINT(used);
        #endif

  if (SetStringValue(":HEAPUSED",used)!=0) {
    PrintErrorMessage('E',"getheapused","could not get string variable :HEAPUSED");
    return (CMDERRORCODE);
  }

  return (OKCODE);
}
/****************************************************************************/
/** \brief Create struct where findrange stores results (min and max)

   This function creates the struct ':findrange'.

   RETURN VALUE:
   .n    0 if ok
   .n    1 if error occured.
 */
/****************************************************************************/

static INT InitFindRange (void)
{
  /* install a struct for findrange */
  if (MakeStruct(":findrange")!=0)
    return (1);
  return (0);
}

/** \brief Implementation of \ref lb. */
INT NS_DIM_PREFIX LBCommand (INT argc, char **argv)
{
                #ifndef ModelP
  /* dummy command in seriell version */
  return(OKCODE);
                #endif

                #ifdef ModelP
  INT cmd_error,i;
  int minlevel;
  char levelarg[32];
  MULTIGRID *theMG;

  theMG = currMG;

  if (theMG == NULL)
  {
    UserWrite("LBCommand: no open multigrid\n");
    return(OKCODE);
  }

  if (procs==1) return(OKCODE);

  /* defaults */
  minlevel                = 1;

  /* parse options */
  for (i=1; i<argc; i++)
    switch (argv[i][0])
    {
    case 'c' :
      sscanf(argv[i],"c %d",&minlevel);
      break;

    default :
      UserWriteF("lb [<strategy>] [$c <minlevel>]\n");
      UserWriteF("default lb 0 $c 1\n");
      break;
    }

  /* check for parameter consistency! */
  cmd_error = 0;

  if ((minlevel<0)||(minlevel>TOPLEVEL(theMG)))
  {
    UserWriteF("Choose <minlevel>: 0-%d (toplevel)\n",TOPLEVEL(theMG));
    cmd_error = 1;
  }

  if (cmd_error) return(CMDERRORCODE);

  sprintf(levelarg,"%d",minlevel);
  lbs(levelarg, theMG);

  return(OKCODE);
                #endif
}


#ifdef ModelP
/** \brief Implementation of \ref lbs. */
static INT LBSCommand (INT argc, char **argv)
{
  MULTIGRID *theCurrMG;

  theCurrMG = currMG;
  if (theCurrMG==NULL)
  {
    PrintErrorMessage('W',"mglist","no multigrid open\n");
    return (OKCODE);
  }

  if (argc==2)
    lbs(argv[1], theCurrMG);
  else
    lbs("0", theCurrMG);

  return(OKCODE);
}


/** \brief Implementation of \ref context. */
static INT ContextCommand (INT argc, char **argv)
{
  INT proc = INT_MAX;
  INT flag_all, flag_empty, flag_invert;


  flag_all   = ReadArgvOption("a", argc, argv);
  flag_empty = ReadArgvOption("e", argc, argv);
  flag_invert= ReadArgvOption("i", argc, argv);

  ReadArgvINT("context", &proc, argc, argv);
  if (proc<0 || proc>=procs)
  {
    if (proc!=INT_MAX && me==0)
      UserWriteF("context: invalid processor id (procs=%d)\n", procs);
  }
  else
  {
    /* switch context for proc on/off */
    CONTEXT(proc) = 1-CONTEXT(proc);
  }

  if (proc==INT_MAX)
  {
    if (flag_all && !flag_empty)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 1;
    }
    if (flag_empty && !flag_all)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 0;
    }
    if (flag_empty && flag_all)
    {
      if (me==0)
        UserWriteF("context: invalid option combination\n");
    }

    if (flag_invert)
    {
      int p; for(p=0; p<procs; p++) CONTEXT(p) = 1-CONTEXT(p);
    }
  }

  ddd_DisplayContext();
  return(OKCODE);
}


/** \brief Implementation of \ref pstat. */
static INT PStatCommand (INT argc, char **argv)
{
  if (argc!=2)
    return (CMDERRORCODE);

  ddd_pstat(argv[1]);
  return(OKCODE);
}

#endif /* ModelP */


#ifdef Debug
/** \brief Implementation of \ref debug. */
static INT DebugCommand (INT argc, char **argv)
{
  char *module;
  int l;

  if (argc<2 || argc>3)
  {
    UserWriteF("usage: debug $<module> [$<level>]\n");
    return (CMDERRORCODE);
  }

  if (argc==3)
  {
    if              (strcmp("init",argv[1])==0) Debuginit               = atoi(argv[2]);
    else if (strcmp("dddif",argv[1])==0) Debugdddif              = atoi(argv[2]);
    else if (strcmp("dev",argv[1])==0) Debugdev                = atoi(argv[2]);
    else if (strcmp("dom",argv[1])==0) Debugdom                = atoi(argv[2]);
    else if (strcmp("gm",argv[1])==0) Debuggm                 = atoi(argv[2]);
    else if (strcmp("graph",argv[1])==0) Debuggraph              = atoi(argv[2]);
    else if (strcmp("low",argv[1])==0) Debuglow                = atoi(argv[2]);
    else if (strcmp("machines",argv[1])==0) Debugmachines   = atoi(argv[2]);
    else if (strcmp("np",argv[1])==0) Debugnp             = atoi(argv[2]);
    else if (strcmp("ui",argv[1])==0) Debugui                 = atoi(argv[2]);
    else if (strcmp("time",argv[1])==0) Debugtime               = atoi(argv[2]);
    else if (strcmp("pclib",argv[1])==0) Debugpclib              = atoi(argv[2]);
    else if (strcmp("appl",argv[1])==0) Debugappl               = atoi(argv[2]);
    else
    {
      UserWriteF("no debug variable for module %s found!\n",argv[1]);
      return (CMDERRORCODE);
    }
    UserWriteF("set debuglevel for module %s to %d\n",argv[1],atoi(argv[2]));
  }
  else
  {
    if              (strcmp("init",argv[1])==0)             {module="init";         l=Debuginit;}
    else if (strcmp("dddif",argv[1])==0)    {module="dddif";        l=Debugdddif;}
    else if (strcmp("dev",argv[1])==0)              {module="dev";          l=Debugdev;}
    else if (strcmp("dom",argv[1])==0)              {module="dom";          l=Debugdom;}
    else if (strcmp("gm",argv[1])==0)               {module="gm";           l=Debuggm;}
    else if (strcmp("graph",argv[1])==0)    {module="graph";        l=Debuggraph;}
    else if (strcmp("low",argv[1])==0)              {module="low";          l=Debuglow;}
    else if (strcmp("machines",argv[1])==0) {module="machines";     l=Debugmachines;}
    else if (strcmp("np",argv[1])==0)           {module="np";           l=Debugnp;}
    else if (strcmp("ui",argv[1])==0)               {module="ui";           l=Debugui;}
    else if (strcmp("time",argv[1])==0)             {module="time";         l=Debugtime;}
    else if (strcmp("pclib",argv[1])==0)    {module="pclib";        l=Debugpclib;}
    else if (strcmp("appl",argv[1])==0)             {module="appl";         l=Debugappl;}
    else
    {
      UserWriteF("no debug variable for module %s found!\n",argv[1]);
      return (CMDERRORCODE);
    }

    UserWriteF("debuglevel for module %s is %d\n",module,l);
  }
  return(OKCODE);
}
#endif


#ifdef Debug
/** \brief Implementation of \ref trace. */
static INT TraceCommand (INT argc, char **argv)
{
  int i;

  for (i=1; i<argc; i++)
    STR_SWITCH(argv[i])
    STR_CASE("blas")
    int n;
  if (sscanf(argv[i],"blas %d",&n)==1)
    TraceUGBlas(n);
  else
    TraceUGBlas(TRBL_PARAMS);
  STR_BREAK

  STR_DEFAULT
  PrintErrorMessageF('E', "TraceCommand", "Unknown option '%s'", argv[i]);
  return (PARAMERRORCODE);
  STR_BREAK

    STR_SWITCH_END

  return (OKCODE);
}
#endif


#ifdef Debug
/** \brief Implementation of \ref reperr. */
static INT RepErrCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  PrintRepErrStack(UserWriteF);

  return (OKCODE);
}
#endif



#ifdef Debug
/** \brief Implementation of \ref timing. */
static INT TimingCommand (INT argc, char **argv)
{
  INT i;

  if (ReadArgvOption("r",argc,argv)) {
    DEBUG_TIME_RESET;
    return (OKCODE);
  }
  if (__debug_time_count==0)
    UserWrite("no timing\n");
  else
  {
    UserWrite("timing:\n\n");
    for (i=0; i<__debug_time_count; i++) {
      UserWriteF("%2d: File:%15s, Line:%5d elapsed time%10.4f",
                 i,__debug_time_file[i],__debug_time_line[i],
                 __debug_time[i]-__debug_time[0]);
      if (i > 0) UserWriteF(" diff%8.4f",
                            __debug_time[i]-__debug_time[i-1]);
      UserWriteF("\n");
    }
  }
  return (OKCODE);
}
#endif


/** \brief Implementation of \ref showconfig. */
static INT ShowConfigCommand (INT argc, char **argv)
{
  NO_OPTION_CHECK(argc,argv);

  UserWrite("Configuration of this program:\n");

#ifdef UG_DIM_2
  UserWrite("    Dimension:    2\n");
#elif defined UG_DIM_3
  UserWrite("    Dimension:    3\n");
#else
  UserWrite("    Dimension:    unknown\n");
#endif

#ifdef ModelP
  UserWrite("    Model:        parallel\n");
#else
  UserWrite("    Model:        sequential\n");
#endif

#ifdef Debug
  UserWrite("    Debugging:    ON\n");
#elif defined NDEBUG
  UserWrite("    Debugging:    OFF\n");
#else
  UserWrite("    Debugging:    unknown\n");
#endif

#ifdef RIF_SOCKETS
  UserWrite("    remote:       ON\n");
#else
  UserWrite("    remote:       OFF\n");
#endif

  return (OKCODE);
}



static INT ClearArray (ARRAY *theAR)
{
  INT i, size;

  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  for (i=0; i<size; i++)
    AR_DATA(theAR,i) = 0.0;

  return (0);
}

/****************************************************************************/
/** \brief Allocate a new array structure

   .  name - name under which the array is allocated in '/Array'
   .  nVar - number of dimensions of the data field
   .  VarDim - extension of the data field in each dimension

   Allocate a new array structure in the directory '/Array' and
   allocate the data field. The maximum number of dimensions is
   'AR_NVAR_MAX'.

   RETURN VALUE:
   .n     pointer to allocated array
   .n     NULL on error.
 */
/****************************************************************************/

static ARRAY *CreateArray (char *name, INT nVar, INT *VarDim)
{
  INT i, size;
  ARRAY *theAR;

  if (nVar<1 || nVar>AR_NVAR_MAX) return (NULL);

  /* change to directory */
  if (ChangeEnvDir("/Array")==NULL)
    return(NULL);

  /* get size */
  size = sizeof(DOUBLE);
  for (i=0; i<nVar; i++)
    size *= VarDim[i];
  size += sizeof(ARRAY) - sizeof(DOUBLE);

  /* allocate structure */
  theAR = (ARRAY*)MakeEnvItem (name,theArrayVarID,size);
  if (theAR==NULL) return (NULL);

  /* init structure */
  ENVITEM_LOCKED(theAR) = 0;
  AR_NVAR(theAR) = nVar;
  for (i=0; i<nVar; i++)
    AR_VARDIM(theAR,i) = VarDim[i];

  if (ClearArray(theAR)) return (NULL);

  return (theAR);
}

/****************************************************************************/
/** \brief Set one single entry of the data field of the array

   .  theAR - array structure to work on
   .  Point - specify the coordinate of the entry in each dimension
   .  value - value to be stored

   Set one single entry of the data field of the array to the given value.

   RETURN VALUE:
   .n    0 always
 */
/****************************************************************************/

static INT WriteArray (ARRAY *theAR, INT *Point, DOUBLE value)
{
  INT i, pos;

  pos = Point[AR_NVAR(theAR)-1];
  for (i=AR_NVAR(theAR)-2; i>=0; i--)
    pos = Point[i] + AR_VARDIM(theAR,i)*pos;
  AR_DATA(theAR,pos) = value;

  return (0);
}

/****************************************************************************/
/** \brief Read one single entry of the data field of the array

   .  theAR - array structure to work on
   .  Point - specify the coordinate of the entry in each dimension
   .  value - read value

   Read one single entry of the data field of the array.

   RETURN VALUE:
   .n    0 always
 */
/****************************************************************************/

static INT ReadArray (ARRAY *theAR, INT *Point, DOUBLE *value)
{
  INT i, pos;

  pos = Point[AR_NVAR(theAR)-1];
  for (i=AR_NVAR(theAR)-2; i>=0; i--)
    pos = Point[i] + AR_VARDIM(theAR,i)*pos;
  value[0] = AR_DATA(theAR,pos);

  return (0);
}


/** \brief Implementation of \ref crar. */
static INT CreateArrayCommand (INT argc, char **argv)
{
  INT i, nVar, VarDim[AR_NVAR_MAX];
  int iValue;
  char name[128];

  nVar = argc-2;
  if (nVar<1 || nVar>AR_NVAR_MAX)
    return (CMDERRORCODE);
  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  for (i=0; i<nVar; i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<1)
      return (CMDERRORCODE);
    VarDim[i] = iValue;
  }

  /* create Array */
  if (CreateArray(name,nVar,VarDim)==NULL)
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref dear. */
static INT DeleteArrayCommand (INT argc, char **argv)
{
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  /* find array */
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"DeleteArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  /* delete Array */
  if (RemoveEnvItem((ENVITEM *)theAR))
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref saar. */
static INT SaveArrayCommand (INT argc, char **argv)
{
  INT i, size;
  char name[128];
  ARRAY *theAR;
  FILE *stream;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  /* find array */
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"SaveArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  /* save Array */
  strcat(name,".array");
  if (arraypathes_set)
    stream = FileOpenUsingSearchPaths(name,"w","arraypathes");
  else
    stream = fileopen(name,"w");
  if (stream==NULL)
  {
    PrintErrorMessage('E',"SaveArrayCommand","cannot open file");
    return(CMDERRORCODE);
  }

  /* store */
  if (fwrite((void*)(&(theAR->nVar)),sizeof(INT),1,stream)!=1) return(CMDERRORCODE);
  if (fwrite((void*)theAR->VarDim,sizeof(INT),AR_NVAR(theAR),stream)!=AR_NVAR(theAR)) return(CMDERRORCODE);
  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  if (fwrite((void*)(theAR->data),sizeof(DOUBLE),size,stream)!=size) return(CMDERRORCODE);
  if (fclose(stream)) return(CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref loar. */
static INT LoadArrayCommand (INT argc, char **argv)
{
  INT i, size, nVar, VarDim[AR_NVAR_MAX];
  char name[128], filename[128];
  ARRAY *theAR;
  FILE *stream;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }

  strcpy(filename,name);
  strcat(filename,".array");
  if (arraypathes_set)
    stream = FileOpenUsingSearchPaths(filename,"r","arraypathes");
  else
    stream = fileopen(filename,"r");
  if (stream==NULL)
  {
    PrintErrorMessage('E',"LoadArrayCommand","cannot open file");
    return(CMDERRORCODE);
  }
  if (fread((void*)(&nVar),sizeof(INT),1,stream)!=1)
    return(CMDERRORCODE);
  if (nVar>AR_NVAR_MAX)
    return (CMDERRORCODE);
  if (fread((void*)VarDim,sizeof(INT),nVar,stream)!=nVar)
    return(CMDERRORCODE);
  theAR = CreateArray (name,nVar,VarDim);
  if (theAR==NULL)
    return(CMDERRORCODE);
  size = 1;
  for (i=0; i<AR_NVAR(theAR); i++)
    size *= AR_VARDIM(theAR,i);
  if (fread((void*)(theAR->data),sizeof(DOUBLE),size,stream)!=size)
    return(CMDERRORCODE);
  if (fclose(stream))
    return(CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref wrar. */
static INT WriteArrayCommand (INT argc, char **argv)
{
  INT i, Point[AR_NVAR_MAX];
  int iValue;
  double Value;
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"WriteArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (AR_NVAR(theAR) != argc-3)
    return (CMDERRORCODE);
  for (i=0; i<AR_NVAR(theAR); i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<0 || iValue>=AR_VARDIM(theAR,i))
    {
      PrintErrorMessage( 'E', "WriteArrayCommand", "Index Range Error" );
      return (CMDERRORCODE);
    }
    Point[i] = iValue;
  }

  /* write */
  if (sscanf(argv[argc-1],"v %lf",&Value)!=1)
    return (CMDERRORCODE);
  if (WriteArray(theAR,Point,(DOUBLE)Value))
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref rear. */
static INT ReadArrayCommand (INT argc, char **argv)
{
  INT i, Point[AR_NVAR_MAX];
  int iValue;
  DOUBLE value;
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"ReadArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (AR_NVAR(theAR) != argc-2)
    return (CMDERRORCODE);
  for (i=0; i<AR_NVAR(theAR); i++)
  {
    if (sscanf(argv[i+2],"%d",&iValue)!=1)
      return (CMDERRORCODE);
    if (iValue<0 || iValue>=AR_VARDIM(theAR,i))
    {
      PrintErrorMessage( 'E', "ReadArrayCommand", "Index Range Error" );
      return (CMDERRORCODE);
    }
    Point[i] = iValue;
  }

  /* read */
  if (ReadArray(theAR,Point,&value))
    return (CMDERRORCODE);
  if (SetStringValue(":ARRAY_VALUE",(double)value))
    return (CMDERRORCODE);

  return (OKCODE);
}


/** \brief Implementation of \ref clar. */
static INT ClearArrayCommand (INT argc, char **argv)
{
  char name[128];
  ARRAY *theAR;

  if (argv[1][0]=='n')
  {
    if (sscanf(argv[1],"n %s",name)!=1)
      return (CMDERRORCODE);
  }
  if (ChangeEnvDir("/Array")==NULL)
  {
    PrintErrorMessage('F',"ClearArrayCommand","could not changedir to /Array");
    return(CMDERRORCODE);
  }
  theAR = (ARRAY *)SearchEnv(name,".",theArrayVarID,SEARCHALL);
  if (theAR==NULL)
    return (CMDERRORCODE);

  if (ClearArray(theAR))
    return (CMDERRORCODE);

  return (OKCODE);
}

/****************************************************************************/
/** \brief Initialization of the array commands

   This function does initialization of the ug-commands concerning arrays.

   \sa   array, crar, dear, wrar, rear, saar, loar, clar

   @return
   .n    0 if ok
   .n    __LINE__ if error occured.
 */
/****************************************************************************/

static INT InitArray (void)
{
  /* install the /Array directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitArray","could not changedir to root");
    return(__LINE__);
  }
  theArrayDirID = GetNewEnvDirID();
  if (MakeEnvItem("Array",theArrayDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitArray","could not install '/Array' dir");
    return(__LINE__);
  }
  theArrayVarID = GetNewEnvVarID();

  /* path to dir for 'array' files */
  arraypathes_set = false;
  if (ReadSearchingPaths(DEFAULTSFILENAME,"arraypathes")==0)
    arraypathes_set = true;

  return (0);
}



/** \brief Implementation of \ref dumpalg. */
static INT DumpAlgCommand(INT argc, char **argv)
{
  INT level, comp;
  VECTOR *v;
  MULTIGRID *theMG;
  GRID *theGrid;
  VECDATA_DESC *v_desc;
  char buffer[1024];

  theMG = currMG;
  if (theMG==NULL)
  {
    PrintErrorMessage('E',"dumpalg","no open multigrid");
    return (CMDERRORCODE);
  }

  v_desc = ReadArgvVecDesc(theMG,"v",argc,argv);
  if (v_desc == NULL)
  {
    PrintErrorMessage('E',"dumpalg","wrong vector specification");
    return (CMDERRORCODE);
  }
  UserWriteF(DISPLAY_NP_FORMAT_SS,"vector displayed",ENVITEM_NAME(v_desc));
  DisplayVecDataDesc(v_desc,~0,buffer);

  for (level=0; level<=TOPLEVEL(theMG); level++)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);

#ifdef ModelP
    for( v=PFIRSTVECTOR(theGrid); v!=NULL; v=SUCCVC(v) )
#else
    for( v=FIRSTVECTOR(theGrid); v!=NULL; v=SUCCVC(v) )
#endif
    {
      printf( "Vec key=%d level=%d type=%d pe=%d fine=%d new_def=%d ",
              KeyForObject((KEY_OBJECT*)v),level,VTYPE(v),me,
              FINE_GRID_DOF(v),NEW_DEFECT(v) );
      for( comp=0; comp<VD_NCMPS_IN_TYPE(v_desc,VTYPE(v)); comp++ )
        printf(" %g ",comp,VVALUE(v,VD_CMP_OF_TYPE(v_desc,VTYPE(v),comp)));
      printf("\n");
    }
  }

  /* aus nstools.c
          {
                  MATRIX *m;
              DisplayMatDataDesc(m,buffer);
              fprintf(fptr,"%s",buffer);

              fprintf(fptr,"Matrix:\n");
              for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
              {
                  for (mat=VSTART(vec); mat!=NULL; mat=MNEXT(mat))
                  {
                      fprintf(fptr,"%d\t%d\n",(int)VINDEX(vec),(int)VINDEX(MDEST(mat)));
                      fprintf(fptr,"f\tt");
                      for (i=0; i<MD_ROWS_IN_RT_CT(m,VTYPE(vec),VTYPE(MDEST(mat))); i++)
                      {
                          for (j=0; j<MD_COLS_IN_RT_CT(m,VTYPE(vec),VTYPE(MDEST(mat))); j+
     +)
                              fprintf(fptr,"\t%.9e",(double)MVALUE(mat,MD_IJ_CMP_OF_RT_CT
          (m,VTYPE(vec),VTYPE(MDEST(mat)),i,j)));
                          fprintf(fptr,"\n\t");
                      }
                      fprintf(fptr,"\n");
                  }
              }
          }
   */

  return(OKCODE);
}

/****************************************************************************/
/* Quick Hack for periodic boundaries                                       */
/****************************************************************************/

#ifdef __PERIODIC_BOUNDARY__
static INT ListPeriodicPosCommand (INT argc, char **argv)
{
  MULTIGRID *theMG = currMG;
  DOUBLE_VECTOR pos;

  if (theMG == NULL)
  {
    UserWrite("ListPeriodicPos: no open multigrid\n");
    return(OKCODE);
  }

#ifdef __THREEDIM__
  if (sscanf(argv[0],"lppos %lf %lf %lf",pos,pos+1,pos+2) != 3)
#else
  if (sscanf(argv[0],"lppos %lf %lf",pos,pos+1) != 2)
#endif
  {
    if (me == master)
      UserWriteF("ListPeriodicPos wrong number of coords\n");
  }

  if (MG_ListPeriodicPos(theMG,0,TOPLEVEL(theMG),pos))
    REP_ERR_RETURN (CMDERRORCODE);

  return (OKCODE);
}

static INT MakePeriodicCommand (INT argc, char **argv)
{
  MULTIGRID *theMG = currMG;

  if (theMG == NULL)
  {
    UserWrite("MakePeriodic: no open multigrid\n");
    return(OKCODE);
  }

  if (MG_GeometricToPeriodic(theMG,0,TOPLEVEL(theMG)))
    REP_ERR_RETURN (CMDERRORCODE);

  return (OKCODE);
}
#endif


/****************************************************************************/
/** \brief Initialization of the commands

   This function does initialization of all ug-commands, using
   'CreateCommand'.
   It initializes 'clock', 'findrange' and 'array'
   commands.

   SEE ALSO:
   commands

   RETURN VALUE:
   .n    0 if ok
   .n    __LINE__ if error occured.
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitCommands ()
{
  /* quick hack */
#ifdef __PERIODIC_BOUNDARY__
  if (CreateCommand("makeperiodic",       MakePeriodicCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("lppos",                      ListPeriodicPosCommand                  )==NULL) return (__LINE__);
#endif

  /* general commands */
  if (CreateCommand("exitug",                     ExitUgCommand                                   )==NULL) return (__LINE__);

  /* commands for environement management */
  if (CreateCommand("cd",                         ChangeEnvCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("ls",                         ListEnvCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("pwd",                        PrintEnvDirCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("envinfo",            EnvInfoCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("set",                        SetCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("dv",                         DeleteVariableCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("ms",                         MakeStructCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("cs",                         ChangeStructCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("pws",                        PrintWorkStructCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("ds",                         DeleteStructCommand                     )==NULL) return (__LINE__);


  /* commands for protocol and logfile output */
  if (CreateCommand("protoOn",            ProtoOnCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("protoOff",           ProtoOffCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("protocol",           ProtocolCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("logon",                      LogOnCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("logoff",             LogOffCommand                                   )==NULL) return (__LINE__);
        #ifdef __TWODIM__
  if (CreateCommand("cnom",                       CnomCommand                                             )==NULL) return (__LINE__);
        #endif

  /* commands for grid management */
  if (CreateCommand("configure",          ConfigureCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("setcurrmg",          SetCurrentMultigridCommand              )==NULL) return (__LINE__);
  if (CreateCommand("new",                        NewCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("open",                       OpenCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("close",                      CloseCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("save",                       SaveCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("savedomain",         SaveDomainCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("changemc",           ChangeMagicCookieCommand                )==NULL) return (__LINE__);
  if (CreateCommand("level",                      LevelCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("renumber",           RenumberMGCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("ordernodes",         OrderNodesCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("lexorderv",          LexOrderVectorsCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("orderv",             OrderVectorsCommand                     )==NULL) return (__LINE__);
  if (CreateCommand("lineorderv",         LineOrderVectorsCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("revvecorder",        RevertVecOrderCommand                   )==NULL) return (__LINE__);
  if (CreateCommand("shellorderv",        ShellOrderVectorsCommand                )==NULL) return (__LINE__);
  if (CreateCommand("setindex",           SetIndexCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("extracon",           ExtraConnectionCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("check",                      CheckCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("in",                         InsertInnerNodeCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("ngin",                       NGInsertInnerNodeCommand                )==NULL) return (__LINE__);
  if (CreateCommand("bn",                         InsertBoundaryNodeCommand               )==NULL) return (__LINE__);
  if (CreateCommand("ngbn",                       NGInsertBoundaryNodeCommand             )==NULL) return (__LINE__);
  if (CreateCommand("gn",                         InsertGlobalNodeCommand                 )==NULL) return (__LINE__);
  if (CreateCommand("deln",                       DeleteNodeCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("move",                       MoveNodeCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("ie",                         InsertElementCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("ngie",                       NGInsertElementCommand                  )==NULL) return (__LINE__);
  if (CreateCommand("dele",                       DeleteElementCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("refine",             AdaptCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("adapt",                      AdaptCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("fixcoarsegrid",      FixCoarseGridCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("collapse",           CollapseCommand                         )==NULL) return (__LINE__);
  if (CreateCommand("mark",                       MarkCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("find",                       FindCommand                                     )==NULL) return (__LINE__);
  if (CreateCommand("select",             SelectCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("mglist",             MGListCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("glist",                      GListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("nlist",                      NListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("elist",                      EListCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("slist",                      SelectionListCommand                    )==NULL) return (__LINE__);
  if (CreateCommand("rlist",                      RuleListCommand                                 )==NULL) return (__LINE__);
  if (CreateCommand("printvalue",         PrintValueCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("vmlist",             VMListCommand                                   )==NULL) return (__LINE__);
  if (CreateCommand("convert",        ConvertCommand                  )==NULL) return(__LINE__);
  if (CreateCommand("quality",            QualityCommand                                  )==NULL) return (__LINE__);
  if (CreateCommand("status",                     StatusCommand                                   )==NULL) return (__LINE__);
#ifdef __THREEDIM__
  if (CreateCommand("fiflel",                     FindFlippedElementsCommand              )==NULL) return (__LINE__);
#endif

  /* commands for window and picture management */
  if (CreateCommand("updateDoc",          UpdateDocumentCommand                   )==NULL) return (__LINE__);

  /* commands for problem management */
  if (CreateCommand("reinit",             ReInitCommand                                   )==NULL) return (__LINE__);

  /* vectors and matrices */
  if (CreateCommand("clear",                      ClearCommand                                    )==NULL) return (__LINE__);
  if (CreateCommand("makevdsub",      MakeVDsubCommand                )==NULL) return (__LINE__);

  if (CreateCommand("rand",                       RandCommand                                             )==NULL) return (__LINE__);
  if (CreateCommand("copy",                       CopyCommand                                             )==NULL) return (__LINE__);
  if (CreateCommand("add",                        AddCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("sub",                        SubCommand                                              )==NULL) return (__LINE__);
  if (CreateCommand("homotopy",       HomotopyCommand                 )==NULL) return(__LINE__);
  if (CreateCommand("interpolate",        InterpolateCommand                              )==NULL) return (__LINE__);

  /* miscellaneous commands */
  if (CreateCommand("resetCEstat",        ResetCEstatCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("printCEstat",        PrintCEstatCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("heapstat",           HeapStatCommand                             )==NULL) return (__LINE__);
  if (CreateCommand("getheapused",        GetHeapUsedCommand                          )==NULL) return (__LINE__);

  /* commands for debugging */
        #ifdef Debug
  if (CreateCommand("debug",                      DebugCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("trace",                      TraceCommand                                )==NULL) return (__LINE__);
  if (CreateCommand("reperr",             RepErrCommand                               )==NULL) return (__LINE__);
  if (CreateCommand("timing",             TimingCommand                               )==NULL) return (__LINE__);
        #endif
  if (CreateCommand("showconfig",         ShowConfigCommand                           )==NULL) return (__LINE__);

#ifdef ModelP
  /* commands for parallel version */
  if (CreateCommand("lb",                         LBCommand                                               )==NULL) return (__LINE__);
  if (CreateCommand("ptest",                      LBSCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("lbs",                        LBSCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("context",            ContextCommand                              )==NULL) return (__LINE__);
  if (CreateCommand("pstat",                      PStatCommand                                )==NULL) return (__LINE__);
#endif /* ModelP */

  /* array commands */
  if (CreateCommand("crar",               CreateArrayCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("dear",               DeleteArrayCommand                                      )==NULL) return (__LINE__);
  if (CreateCommand("saar",               SaveArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("loar",               LoadArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("wrar",               WriteArrayCommand                                       )==NULL) return (__LINE__);
  if (CreateCommand("rear",               ReadArrayCommand                                        )==NULL) return (__LINE__);
  if (CreateCommand("clar",               ClearArrayCommand                                       )==NULL) return (__LINE__);

  if (CreateCommand("dumpalg",            DumpAlgCommand                                  )==NULL) return (__LINE__);

  if (InitFindRange()     !=0) return (__LINE__);
  if (InitArray()                 !=0) return (__LINE__);

  return(0);
}

/** @} */
