// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      fileopen.h                                                                                                    */
/*                                                                                                                                                      */
/* Author:    Henrik Rentz-Reichert                                                                                 */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70569 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                           */
/*                                                                                                                                                      */
/* History:   13.02.95 begin, ug version 3.0                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __FILEOPEN__
#define __FILEOPEN__

#include <cstdio>
#include <fcntl.h>

#include <sys/stat.h>

#include "ugtypes.h"
#include "namespace.h"

START_UG_NAMESPACE

/****************************************************************************/
/*                                                                                                                                                      */
/* defines in the following order                                                                                       */
/*                                                                                                                                                      */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/


/* return constants for filetype() */
enum FileTypes { FT_UNKNOWN, FT_FILE, FT_DIR, FT_LINK };

#define fileopen(fname,mode)            fopen_r(BasedConvertedFilename(fname),(mode),0)
#define fileopen_r(fname,mode,r)        fopen_r(BasedConvertedFilename(fname),(mode),(r))

enum DIRWALK_ERR
{
  PATH_INVALID    = 1,
  PATH_NO_DIR,
  NAME_TOO_LONG,
  NOT_IMPLEMENTED,
  VOLUME_NOT_FOUND,
  NO_DEFAULT_VOLUME,
  IO_ERROR,
  BAD_FILENAME,
  FILE_NOT_FOUND,
  DIR_NOT_FOUND,
  ACCESS_ERROR
};

/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/

typedef void (*ProcessFileProc)(const char *fname);

/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/

/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

const char*     BasedConvertedFilename          (const char *fname);
size_t          filesize                    (const char *fname);
int             filetype                    (const char *fname);
INT                     DirWalk                                         (const char *dir, ProcessFileProc fcn);
INT                     ReadSearchingPaths                      (const char *filename, const char *pathsvar);
int                     DirCreateUsingSearchPaths       (const char *fname, const char *paths);
int                     DirCreateUsingSearchPaths_r     (const char *fname, const char *paths, int rename);
FILE            *FileOpenUsingSearchPaths       (const char *fname, const char *mode, const char *pathsvar);
FILE            *FileOpenUsingSearchPaths_r     (const char *fname, const char *mode, const char *pathsvar, int rename);
FILE            *FileOpenUsingSearchPath        (const char *fname, const char *mode, const char *path);
FILE            *FileOpenUsingSearchPath_r      (const char *fname, const char *mode, const char *path, int rename);
int             FileTypeUsingSearchPaths        (const char *fname, const char *pathsvar);
FILE            *fopen_r                                        (const char *fname, const char *mode, int do_rename);
int             mkdir_r                                         (const char *fname, mode_t mode, int do_rename);

int                     AppendTrailingSlash                     (char *path);
char*           SimplifyPath                            (char *path);
const char*     GetBasePath                                     (void);
const char*     SetBasePath                                     (const char *path);
const char*     AddBasePath                                     (const char *path);

INT                     InitFileOpen                            (void);

END_UG_NAMESPACE

#endif
