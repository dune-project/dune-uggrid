// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  fileopen.c													*/
/*																			*/
/* Purpose:   definition of a fopen fct. that accepts UNIX-style pathnames	*/
/*																			*/
/* Author:	  Henrik Rentz-Reichert                                                                                 */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de							    */
/*																			*/
/* History:   02.02.95 new for ug version 3.0								*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/


/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <cassert>

#include "ugtypes.h"
#include "ugtime.h"

/* includes for filesize(), filetype() */
#ifdef __MACINTOSH__
#include <unistd.h>
#include <stat.h>
#include <Files.h>
#include <TextUtils.h>
#include <Errors.h>
/* NB: On Macs the structs of <types.h> are defined locally in <stat.h> */
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif


/* low module */
/* avoid assertion in REP_ERR_RETURN for ModelP and GUI */
#if defined(ModelP) && defined(__GUI__)
#define PARALLEL
#undef ModelP
#endif

#include "debug.h"

#ifdef PARALLEL
#define ModelP
#undef PARALLEL
#endif

#include "general.h"
#include "ugenv.h"
#include <dune/uggrid/ugdevices.h>

#include "fileopen.h"

#if defined __HP__ || __SGI__ || __T3E__ || __PARAGON__ || __DEC__ || __SUN__ || __PC__ || __LINUXPPC__
#include <dirent.h>
#endif

USING_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MAXPATHLENGTH           256
#define MAXPATHS                        16

#define SEPERATOR                       " \t"

#define MAX_PATH_LEN            1024
#define BASE_PATH_SIZE          512

#ifndef __MACINTOSH__
        #define ConvertUNIX_2_MachinePath(fname)                fname
        #define ConvertMachine_2_UNIXPath(fname)                fname
#endif

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*                                                                          */
/****************************************************************************/

typedef char PATH[MAXPATHLENGTH];

typedef struct
{

  /* env item */
  ENVVAR v;

  INT nPaths;                                   /* number of paths stored						*/
  PATH path[1];                         /* begin of path list							*/

} PATHS;

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static INT thePathsDirID;
static INT thePathsVarID;


static char BasePath[BASE_PATH_SIZE] = "./";

REP_ERR_FILE


/****************************************************************************/
/*																			*/
/* Function:  GetPaths														*/
/*																			*/
/* Purpose:   find the Paths environment item with name                                         */
/*																			*/
/* Input:	  name of the Paths to find                                                                     */
/*																			*/
/* Output:	  PATHS *: pointer to the multigrid struct						*/
/*			  NULL: if an error occured                                                                     */
/*																			*/
/****************************************************************************/

static PATHS *GetPaths (const char *name)
{
  return ((PATHS *) SearchEnv(name,"/Paths",thePathsVarID,thePathsDirID));
}

/****************************************************************************/
/*D
        ConvertUNIX_2_MachinePath - convert UNIX-style file paths to machine format

        SYNOPSIS:
        const char *ConvertUNIX_2_MachinePath (const char *fname)


    PARAMETERS:
   .   fname - filename with path convention in UNIX-style

        DESCRIPTION:
        Convert UNIX-style paths to machine format. If '__MACINTOSH__'
        is defined (Apple Macintosh computer) the UNIX-sytle path is converted to
        Macintosh-style.

        For other platforms 'ConvertUNIX_2_MachinePath' returns 'fname' (macro).

        RETURN VALUE:
        char *
   .n   converted file name (NULL if error)
   D*/
/****************************************************************************/

#ifdef __MACINTOSH__

/* Macintosh computers */
static const char *ConvertUNIX_2_MachinePath (const char *fname)
{
  static char fullpath[MAXPATHLENGTH];
  int pos;

  if (*fname=='/')
    /* root not defined on Macintosh computers */
    return (NULL);
  if (*fname=='~')
    /* home directory not defined on Macintosh computers */
    return (NULL);

  /* something to convert? */
  if (strchr(fname,'/')==NULL)
    return (fname);

  pos = 0;
  while ((*fname!='\0') && (pos<MAXPATHLENGTH-2))
    switch (fname[0])
    {
    case '.' :
      if ((fname[1]=='.')&&(fname[2]=='/'))
      {
        /* "../" */

        /* if the path starts with "../" we interpret "./../", i.e. "::" */
        if (pos==0)
          fullpath[pos++] = ':';

        fullpath[pos++] = ':';
        fname += 3;
      }
      else if (fname[1]=='/')
      {
        if (pos)
        {
          /* eat ./ */
          fname += 2;
          continue;
        }

        /* "./" --> ":" */
        fullpath[pos++] = ':';
        fname += 2;
      }
      else
        fullpath[pos++] = *(fname++);
      break;

    case '/' :
      /* "/" --> ":" */
      fullpath[pos++] = ':';
      while (*fname=='/') fname++;
      break;

    default :
      fullpath[pos++] = *(fname++);
    }

  if (pos>=MAXPATHLENGTH)
    /* filename too long */
    return (NULL);

  /* 0-terminate string */
  fullpath[pos] = '\0';

  return (fullpath);
}

static const char* ConvertMachine_2_UNIXPath (const char* fname)
{
  static char fullpath[MAXPATHLENGTH];
  int pos;

  /* something to convert? */
  if (strchr(fname,':')==NULL)
    return (fname);

  pos = 0;
  while ((*fname!='\0') && (pos<MAXPATHLENGTH-2))
    switch (fname[0])
    {
    case ':' :
      if (fname[1]==':')
      {
        /* "::" */

        fullpath[pos++] = '.';
        fullpath[pos++] = '.';
        fullpath[pos++] = '/';
        fname++;
      }
      else
      {
        /* ":" */
        if (fullpath[pos]!='/')
          fullpath[pos++] = '/';
        fname++;
      }
      break;

    default :
      fullpath[pos++] = *(fname++);
    }

  if (pos>=MAXPATHLENGTH)
    /* filename too long */
    return (NULL);

  /* 0-terminate string */
  fullpath[pos] = '\0';

  return (fullpath);
}

#endif

const char * NS_PREFIX BasedConvertedFilename (const char *fname)
/* NOTE: once a filename has passed through BasedConvertedFilename() it is forbidden to
                 call BasedConvertedFilename() a second time for this filename due to
                 static result string.
 */
{
  PRINTDEBUG(low,2,("BasedConvertedFilename: fname= '%s'\n",fname));
  if (fname[0]!='/' && fname[0]!='~')                   /* use BasePath only if no absolute path specified */
  {
    static char based_filename[MAXPATHLENGTH];

    assert(fname!=based_filename);              /* avoid that the result of a previous call to BasedConvertedFilename() is fed back;
                                                                                   in this case the new result would interfere with the given input name and corrupt the result
                                                                                   (for example only ./ would be returned)*/
    strcpy(based_filename,BasePath);
    strcat(based_filename,fname);
    SimplifyPath(based_filename);
    PRINTDEBUG(low,1,("BasedConvertedFilename: based_filename= '%s'\n",based_filename));
    return ConvertUNIX_2_MachinePath(based_filename);
  }
  else
  {
    PRINTDEBUG(low,1,("BasedConvertedFilename: filename not based= '%s'\n",fname));
    return ConvertUNIX_2_MachinePath(fname);
  }
}

static int rename_if_necessary( const char *fname, int do_rename)
{
  FILE *f;

  if (do_rename && (f=fopen(fname,"r"))!=NULL)
  {
    time_t Time;
    struct stat fstat;
    char new_fname[128];

    fclose(f);

    strcpy(new_fname,fname);
    strcat(new_fname,".");

    if (stat(fname, &fstat)<0)
      return(1);
    Time = fstat.st_mtime;
    strftime(new_fname+strlen(fname)+1,64,"%y%m%d%H%M%S",localtime(&Time));

    if (rename(fname,new_fname)!=0)
      return(1);
  }
  return (0);           /* ok */
}


/****************************************************************************/
/*D
        mkdir_r - create a directory incl. renaming option

        SYNOPSIS:
        int mkdir_r (const char *fname, mode_t mode, int do_rename)

    PARAMETERS:
   .   fname - directory name with path convention in UNIX-style; may not be passed through BasedConvertedFilename()
   .   mode - creation mode in UNIX-style (see mkdir(2))
   .   do_rename - if true an already existing subdirectory will be renamed

        DESCRIPTION:
    This function creates an directory and renames an already existing one
        instead of overwriting it if specified.
        fname may not be passed through BasedConvertedFilename().

        RETURN VALUE:
        int
   .n   0 sucessfull completion
   .n      != 0 error occured

        SEE ALSO:
        mkdir(2), fopen_r
   D*/
/****************************************************************************/

int NS_PREFIX mkdir_r (const char *fname, mode_t mode, int do_rename)
{
  const char *converted_name = BasedConvertedFilename(fname);

  if (do_rename)
  {
    if (rename_if_necessary( converted_name, do_rename)!=0)
      return (1);
#ifdef __MINGW32__
    return mkdir(converted_name);
#else
    return mkdir(converted_name,mode);
#endif
  }
  else
  {
    switch (filetype(fname))                    /* filetype needs an NOT BasedConvertedFilename'ed filename */
    {
    case FT_UNKNOWN :                           /* file doesn't exist, thus create it */
#ifdef __MINGW32__
      return mkdir(converted_name);
#else
      return mkdir(converted_name,mode);
#endif

    case FT_DIR :
      return 0;                                         /* OK, directory exists already */

    case FT_FILE :
      UserWriteF("mkdir_r(): file %s exists already as ordinary file; can't create directory with same name.\n",converted_name);
      return 1;

    case FT_LINK :
      UserWriteF("mkdir_r(): file %s exists already as a link; can't create directory with same name.\n",converted_name);
      return 1;

    default :
      UserWriteF("mkdir_r(): unknown file type %d for file %s\n",filetype(fname),converted_name);
      return 1;
    }
  }
}

/****************************************************************************/
/*D
        fopen_r - create a file incl. renaming option

        SYNOPSIS:
        int fopen_r (const char *fname, const char *mode, int do_rename)

    PARAMETERS:
   .   fname - file name with path convention in UNIX-style
   .   mode - file opening mode in UNIX-style (see fopen(2))
   .   do_rename - if true an already existing file will be renamed

        DESCRIPTION:
    This function opens a file and renames an already existing one
        instead of overwriting it if specified.

        RETURN VALUE:
        int
   .n   0 sucessfull completion
   .n      != 0 error occured

        SEE ALSO:
        fopen(2), mkdir_r
   D*/
/****************************************************************************/

FILE * NS_PREFIX fopen_r (const char *fname, const char *mode, int do_rename)
{
  if (rename_if_necessary( fname, do_rename)!=0)
    return (NULL);

  return fopen(fname,mode);
}

/****************************************************************************/
/*D
        filetype - get type of a file with given name

        SYNOPSIS:
        int filetype (const char *fname)

    PARAMETERS:
   .   fname - filename with path convention in UNIX-style; it may not be passed through BasedConvertedFilename()

        DESCRIPTION:
    This functon returns the type of the given file
    or FT_UNKNOWN if an error occurs.
        fname may not be passed through BasedConvertedFilename().

        RETURN VALUE:
        int
   .n      file type (one of FT_UNKNOWN, FT_FILE, FT_DIR, FT_LINK)

        SEE ALSO:
        fopen, fileopen
   D*/
/****************************************************************************/

int NS_PREFIX filetype (const char *fname)
{
  struct stat fstat;
  int r;

  /* get Unix file descriptor */
  PRINTDEBUG(low,1,("filetype\n"));
  if ((r=stat(BasedConvertedFilename(fname), &fstat))<0)
    return(FT_UNKNOWN);

        #ifdef __CC__
  switch (fstat.st_mode & _S_IFMT)
  {
  case _S_IFREG :   return FT_FILE;
  case _S_IFDIR :   return FT_DIR;
#ifdef S_IFLNK
  case _S_IFLNK :   return FT_LINK;
#endif
  }
#else
  switch (fstat.st_mode & S_IFMT)
  {
  case S_IFREG :   return FT_FILE;
  case S_IFDIR :   return FT_DIR;
#ifdef S_IFLNK
  case S_IFLNK :   return FT_LINK;
#endif
  }
#endif
  return(FT_UNKNOWN);
}

/****************************************************************************/
/*D
        ReadSearchingPaths - read searching paths from a defaults file

        SYNOPSIS:
        INT ReadSearchingPaths (const char *filename, const char *paths)

    PARAMETERS:
   .   filename - search paths in a defaults file with this name (most likely to be
                        `the` --> 'defaults' file, use 'DEFAULTSFILENAME')
   .   paths - name of the paths item looked for in a defaults file

        DESCRIPTION:
        From a defaultsfile using --> 'GetDefaultValue' the specified paths item
        is read containing one or more paths sperated by blanks (tab or space).
        The paths are stored in an environment item with that same name in the
        environment directory '/Paths'. The function --> 'FileOpenUsingSearchPaths'
        is looking up the paths to be tryed there.

        RETURN VALUE:
        INT
   .n   1: failed to 'GetDefaultValue'
   .n   2: more than 'MAXPATHS' specified in the defaults file
   .n   3: failed to 'MakePathsItem'
   .n   0: ok

        SEE ALSO:
        FileOpenUsingSearchPaths
   D*/
/****************************************************************************/

INT NS_PREFIX ReadSearchingPaths (const char *filename, const char *paths)
{
  return (1);
}

/****************************************************************************/
/*D
        DirCreateUsingSearchPaths - create a directory searching in the directories specified
                        in the environment item '/Paths/<paths>'

        SYNOPSIS:
        int DirCreateUsingSearchPaths (const char *fname, const char *paths);

        PARAMETERS:
   .   fname - subdirectory name to be created
   .   paths - try paths specified in the environment item '/Paths/<paths>' which was
                        set by --> 'ReadSearchingPaths'

        DESCRIPTION:
        The functions trys to create a directory with 'filename' using one by one the
        paths specified in the environment item '/Paths/<paths>' which was
        set by --> 'ReadSearchingPaths'. It is used in several places in ug (all paths
        are read from the standard --> 'defaults' file)":"

   .n   'scriptpaths' is used by the interpreter for script execution
   .n   'gridpaths' is used by ugio to read grids from (they are stored in the
   .n   first path

        RETURN VALUE:
        int
   .n   0 sucessfull completion
   .n      != 0 error occured

        SEE ALSO:
        DirCreateUsingSearchPaths_r, mkdir(2)
   D*/
/****************************************************************************/

int NS_PREFIX DirCreateUsingSearchPaths (const char *fname, const char *paths)
{
  return DirCreateUsingSearchPaths_r ( fname, paths, false);            /* no renaming */
}

/****************************************************************************/
/*D
        DirCreateUsingSearchPaths_r - create a subdirectory searching in the directories specified
                        in the environment item '/Paths/<paths>' incl. renaming option

        SYNOPSIS:
        int DirCreateUsingSearchPaths (const char *fname, const char *paths, int rename);

        PARAMETERS:
   .   fname - subdirectory name to be created
   .   paths - try paths specified in the environment item '/Paths/<paths>' which was
                        set by --> 'ReadSearchingPaths'
   .   rename - if true an already existing subdirectory will be renamed

        DESCRIPTION:
        The functions trys to create a subdirectory with 'filename' using one by one the
        paths specified in the environment item '/Paths/<paths>' which was
        set by --> 'ReadSearchingPaths'. It is used in several places in ug (all paths
        are read from the standard --> 'defaults' file)":"

   .n   'srciptpaths' is used by the interpreter for script execution
   .n   'gridpaths' is used by ugio to read grids from (they are stored in the
   .n   first path

        RETURN VALUE:
        int
   .n   0 sucessfull completion
   .n      != 0 error occured

        SEE ALSO:
        DirCreateUsingSearchPaths, mkdir(2)
   D*/
/****************************************************************************/

int NS_PREFIX DirCreateUsingSearchPaths_r (const char *fname, const char *paths, int rename)
{
  PATHS *thePaths;
  FILE *parentDir;

  char fullname[MAXPATHLENGTH];
  INT i,fnamelen,error;
  mode_t mode;

  fnamelen = strlen(fname);
        #ifndef __MACINTOSH__
  mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP;
        #else
  mode = 0;       /* ignored on Macintosh */
        #endif

  PRINTDEBUG(low,1,("DirCreateUsingSearchPaths\n"));
  if (paths == NULL)
  {
    if ((error=mkdir_r(fname,mode,rename))!=0)
      return (1);
    return (0);
  }

  if ((thePaths=GetPaths(paths))==NULL)
    return (1);

  for (i=0; i<thePaths->nPaths; i++)
  {
    /* test whether parent directory exists */
    if ( (parentDir=fopen(thePaths->path[i],"r")) == NULL )
      continue;                         /* this parent directory doesn't exist; try the next one */
    if( (error=fclose(parentDir)) != 0 )
      return (1);

    if (strlen(thePaths->path[i])+fnamelen>MAXPATHLENGTH)
      return (1);

    strcpy(fullname,thePaths->path[i]);
    strcat(fullname,fname);

    if ((error=mkdir_r(fullname,mode,rename))!=0)
      return (1);
    return (0);                 /* subdirectory created sucessfully */
  }
  return (1);
}

/****************************************************************************/
/*D
        FileOpenUsingSearchPaths - open file searching in the directories specified
                        in the environment item '/Paths/<paths>'

        SYNOPSIS:
        FILE *FileOpenUsingSearchPaths (const char *fname, const char *mode, const char *paths)

    PARAMETERS:
   .   fname - file name to be opened
   .   mode - see ANSI-C 'fopen'
   .   paths - try paths specified in the environment item '/Paths/<paths>' which was
                        set by --> 'ReadSearchingPaths'

        DESCRIPTION:
        The functions trys to open the file with 'filename' using one by one the
        paths specified in the environment item '/Paths/<paths>' which was
        set by --> 'ReadSearchingPaths'. It is used in several places in ug (all paths
        are read from the standard --> 'defaults' file)":"

   .n   'srciptpaths' is used by the interpreter for script execution
   .n   'gridpaths' is used by ugio to read grids from (they are stored in the
   .n   first path

        RETURN VALUE:
        FILE *
   .n   pointer to file opened, 'NULL' if error

        SEE ALSO:
        FileOpenUsingSearchPaths_r, FileOpenUsingSearchPath, FileOpenUsingSearchPath_r, GetPaths, ReadSearchingPaths, fileopen
   D*/
/****************************************************************************/

FILE * NS_PREFIX FileOpenUsingSearchPaths (const char *fname, const char *mode, const char *paths)
{
  return FileOpenUsingSearchPaths_r( fname, mode, paths, false );       /* no renaming */
}

/****************************************************************************/
/*D
        FileOpenUsingSearchPaths_r - open file searching in the directories specified
                        in the environment item '/Paths/<paths>' incl. renaming option

        SYNOPSIS:
        FILE *FileOpenUsingSearchPaths_r (const char *fname, const char *mode, const char *paths, int rename)

    PARAMETERS:
   .   fname - file name to be opened
   .   mode - see ANSI-C 'fopen'
   .   paths - try paths specified in the environment item '/Paths/<paths>' which was
                        set by --> 'ReadSearchingPaths'
   .   rename - if true an already existing file will be renamed (wise only for writing)

        DESCRIPTION:
        The functions trys to open the file with 'filename' using one by one the
        paths specified in the environment item '/Paths/<paths>' which was
        set by --> 'ReadSearchingPaths'. It is used in several places in ug (all paths
        are read from the standard --> 'defaults' file)":"

   .n   'srciptpaths' is used by the interpreter for script execution
   .n   'gridpaths' is used by ugio to read grids from (they are stored in the
   .n   first path

        RETURN VALUE:
        FILE *
   .n   pointer to file opened, 'NULL' if error

        SEE ALSO:
        FileOpenUsingSearchPaths, FileOpenUsingSearchPath, FileOpenUsingSearchPath_r, GetPaths, ReadSearchingPaths, fileopen
   D*/
/****************************************************************************/

FILE * NS_PREFIX FileOpenUsingSearchPaths_r (const char *fname, const char *mode, const char *paths, int rename)
{
  PATHS *thePaths;
  FILE *theFile;
  char fullname[MAXPATHLENGTH];
  INT i,fnamelen;

  fnamelen = strlen(fname);

  if ((thePaths=GetPaths(paths))==NULL)
    return (NULL);

  for (i=0; i<thePaths->nPaths; i++)
  {
    if (strlen(thePaths->path[i])+fnamelen>MAXPATHLENGTH)
      return (NULL);

    strcpy(fullname,thePaths->path[i]);
    strcat(fullname,fname);

    if ((theFile=fileopen_r(fullname,mode,rename))!=NULL)
      return (theFile);
  }

  return (NULL);
}

/****************************************************************************/
/*D
        FileOpenUsingSearchPath - try to open a file in the specified path

        SYNOPSIS:
        FILE *FileOpenUsingSearchPath (const char *fname, const char *mode, const char *path)

    PARAMETERS:
   .   fname - open file with this name
   .   mode - see ANSI-C 'fopen'
   .   path - path to which fname is to be appended

        DESCRIPTION:
        Try to open a file in the specified path.

        RETURN VALUE:
        FILE *
   .n   pointer to file opened, 'NULL' if error

        SEE ALSO:
        FileOpenUsingSearchPath_r, FileOpenUsingSearchPaths, FileOpenUsingSearchPaths_r, fileopen
   D*/
/****************************************************************************/

FILE * NS_PREFIX FileOpenUsingSearchPath (const char *fname, const char *mode, const char *path)
{
  return FileOpenUsingSearchPath_r( fname, mode, path, false );         /* no renaming */
}

/****************************************************************************/
/*D
        FileOpenUsingSearchPath_r - try to open a file in the specified path and renaming option

        SYNOPSIS:
        FILE *FileOpenUsingSearchPath_r (const char *fname, const char *mode, const char *path, int rename)

    PARAMETERS:
   .   fname - open file with this name
   .   mode - see ANSI-C 'fopen'
   .   path - path to which fname is to be appended
   .   rename - if true an already existing file will be renamed (wise only for writing)

        DESCRIPTION:
        Try to open a file in the specified path.

        RETURN VALUE:
        FILE *
   .n   pointer to file opened, 'NULL' if error

        SEE ALSO:
        FileOpenUsingSearchPath, FileOpenUsingSearchPaths, FileOpenUsingSearchPaths_r, fileopen
   D*/
/****************************************************************************/

FILE * NS_PREFIX FileOpenUsingSearchPath_r (const char *fname, const char *mode, const char *path, int rename)
{
  FILE *theFile;
  char fullname[MAXPATHLENGTH];

  if (strlen(path)+strlen(fname)>MAXPATHLENGTH)
    return (NULL);

  strcpy(fullname,path);
  strcat(fullname,fname);

  if ((theFile=fileopen_r(fullname,mode,rename))!=NULL)
    return (theFile);

  return (NULL);
}

/****************************************************************************/
/*D
        FileTypeUsingSearchPaths - give type of file searching in the
            directories specified in the environment item '/Paths/<paths>'

        SYNOPSIS:
        int FileTypeUsingSearchPaths (const char *fname, const char *paths)

    PARAMETERS:
   .   fname - file name to be opened
   .   paths - try paths specified in the environment item '/Paths/<paths>' which was
                        set by --> 'ReadSearchingPaths'

        DESCRIPTION:
        The functions trys to determine the file type of the file named
        'filename' using one by one the paths specified in the environment
        item '/Paths/<paths>' which was set by --> 'ReadSearchingPaths'.
        It is used in several places in ug (all paths are read from the
        standard --> 'defaults' file)":"

   .n   'srciptpaths' is used by the interpreter for script execution
   .n   'gridpaths' is used by ugio to read grids from (they are stored in the
   .n   first path)

        RETURN VALUE:
        int
   .n      file type (one of FT_UNKNOWN, FT_FILE, FT_DIR, FT_LINK)

        SEE ALSO:
        ReadSearchingPaths, filetype
   D*/
/****************************************************************************/

int NS_PREFIX FileTypeUsingSearchPaths (const char *fname, const char *paths)
{
  PATHS *thePaths;
  int ftype;
  char fullname[MAXPATHLENGTH];
  INT i,fnamelen;

  fnamelen = strlen(fname);

  if ((thePaths=GetPaths(paths))==NULL)
    return (FT_UNKNOWN);

  for (i=0; i<thePaths->nPaths; i++)
  {
    if (strlen(thePaths->path[i])+fnamelen>MAXPATHLENGTH)
      return (FT_UNKNOWN);

    strcpy(fullname,thePaths->path[i]);
    strcat(fullname,fname);

    if ((ftype=filetype(fullname))!=FT_UNKNOWN)
      return (ftype);
  }

  return (FT_UNKNOWN);
}

int NS_PREFIX AppendTrailingSlash (char *path)
{
  if (path[0]!='\0' && path[strlen(path)-1]!='/')
  {
    strcat(path,"/");
    return YES;
  }
  return NO;
}

char * NS_PREFIX SimplifyPath (char *path)
{
  const char *pf;
  char       *pt;

  PRINTDEBUG(low,2,("SimplifyPath: original path= '%s'\n",path));

  /* cancel ./ (not first one) */
  pf = pt = strchr(path,'/');
  if (pf!=NULL)
  {
    while (*pf)
    {
      if (pf[0]=='.' && pf[1]=='/')
        if (*(pf-1)=='/')
        {
          /* eat ./ */
          pf += 2;
          continue;
        }
      if (pt!=pf)
        *pt = *pf;
      pf++;
      pt++;
    }
    *pt = '\0';
  }

  PRINTDEBUG(low,2,("SimplifyPath: path= '%s'\n",path));

  /* cancel ../ where possible */
  pf = pt = path;
  for (; *pf; pf++,pt++)
  {
    if (pf[0]=='.' && pf[1]=='.' && pf[2]=='/')
      if (pf==path || *(pf-1)=='/')
      {
        char *pd = pt-1;

        while (pd>path)
          if (*(--pd)=='/')
            break;
        if (*pd=='/' && !(pd[0]=='/' && pd[1]=='.' && pd[2]=='.' && pd[3]=='/'))
        {
          /* eat ../ and reset pt */
          pf += 2;
          pt = pd;
          continue;
        }
      }
    *pt = *pf;
  }
  *pt = '\0';

  return path;
}

/****************************************************************************/
/*D
        InitFileOpen - init 'fileopen.c'

        SYNOPSIS:
        INT InitFileOpen ()

    PARAMETERS:
    --

        DESCRIPTION:
        An environment directory '/Paths' is created where the paths read by
        'ReadSearchingPaths' are stored.

        RETURN VALUE:
        INT
   .n   __LINE__: could not create '/Paths'
   .n           0: ok
   D*/
/****************************************************************************/

INT NS_PREFIX InitFileOpen ()
{
  /* install the /Paths directory */
  if (ChangeEnvDir("/")==NULL)
    return(__LINE__);

  thePathsDirID = GetNewEnvDirID();
  if (MakeEnvItem("Paths",thePathsDirID,sizeof(ENVDIR))==NULL)
    return(__LINE__);

  thePathsVarID = GetNewEnvVarID();

  return (0);
}
