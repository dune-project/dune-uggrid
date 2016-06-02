/* begin dune-uggrid */
/* Define to the version of dune-common */
#define DUNE_UGGRID_VERSION "${DUNE_UGGRID_VERSION}"

/* Define to the major version of dune-common */
#define DUNE_UGGRID_VERSION_MAJOR ${DUNE_UGGRID_VERSION_MAJOR}

/* Define to the minor version of dune-common */
#define DUNE_UGGRID_VERSION_MINOR ${DUNE_UGGRID_VERSION_MINOR}

/* Define to the revision of dune-common */
#define DUNE_UGGRID_VERSION_REVISION ${DUNE_UGGRID_VERSION_REVISION}

/* If this is set, the operating system heap is used instead of UG's own heap
   data structure. */
#cmakedefine UG_USE_SYSTEM_HEAP 1

/* begin private section */

/* UG memory allocation model */
#define DYNAMIC_MEMORY_ALLOCMODEL 1

/* see parallel/ddd/dddi.h */
#cmakedefine DDD_MAX_PROCBITS_IN_GID ${UG_DDD_MAX_MACROBITS}

/* Define to 1 if you have `alloca', as a function or macro. */
#cmakedefine HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix). */
#cmakedefine HAVE_ALLOCA_H 1

/* Define to 1 if you have the `bzero' function. */
#cmakedefine HAVE_BZERO 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'. */
#cmakedefine HAVE_DIRENT_H 1

/* Define to 1 if you have the <float.h> header file, and it defines. */
#cmakedefine HAVE_FLOAT_H 1

/* Define to 1 if you have the floor function. */
#cmakedefine HAVE_FLOOR 1

/* Define to 1 if you have the <limits.h> header file, and it defines. */
#cmakedefine HAVE_LIMITS_H 1

/* Define to 1 if you have the <malloc.h> header file, and it defines. */
#cmakedefine HAVE_MALLOC_H 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#cmakedefine HAVE_MALLOC 1

/* Define to 1 if you have the `memmove' function. */
#cmakedefine HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file, and it defines. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `memcpy' function. */
// Was never set with autotools for whatever reason.
//#cmakedefine HAVE_MEMCPY 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Define to 1 if you have the `realloc' function. */
#cmakedefine HAVE_REALLOC 1

/* Define to 1 if you have the <stdlib.h> header file, and it defines. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#cmakedefine HAVE_STRCHR 1

/* Define to 1 if you have the `strrchr' function. */
#cmakedefine HAVE_STRRCHR 1

/* Define to 1 if you have the `strdup' function. */
#cmakedefine HAVE_STRDUP 1

/* Define to 1 if you have the <string.h> header file, and it defines. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <strings.h> header file, and it defines. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the `strrchr' function. */
#cmakedefine HAVE_STRRCHR 1

/* Define to 1 if you have the <strrchr.h> header file, and it defines. */
#cmakedefine HAVE_STRRCHR_H 1

/* Define to 1 if you have the `strstr' function. */
#cmakedefine HAVE_STRSTR 1

/* Define to 1 if you have the <strstr.h> header file, and it defines. */
#cmakedefine HAVE_STRSTR_H 1

/* Define to 1 if you have the `strchr' function. */
#cmakedefine HAVE_STRTOL 1

/* Define to 1 if you have the <strtol.h> header file, and it defines. */
#cmakedefine HAVE_STRTOL_H 1

/* Define to 1 if `vfork' works. */
#cmakedefine HAVE_WORKING_VFORK 1

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME 1

/* end private section */

/* end dune-uggrid */
