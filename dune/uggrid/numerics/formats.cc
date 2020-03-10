// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      formats.c                                                     */
/*                                                                          */
/* Purpose:   definition of user data and symbols                           */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   27.03.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cstdio>
#include <cstring>
#include <ctype.h>

#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/algebra.h>
#include <dune/uggrid/gm/enrol.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/misc.h>
#include <dune/uggrid/low/scan.h> // for ReadArgvChar
#include <dune/uggrid/low/ugenv.h>
#include <dune/uggrid/low/ugstruct.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/numerics/formats.h>


USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#define MAX_PRINT_SYM                                   5

/** @name Format for PrintVectorData and PrintMatrixData */
/*@{*/
#define VFORMAT                                                 " %c=%11.4E"
#define MFORMAT                                                 " %c%c=%11.4E"
/*@}*/

/** @name Separators */
/*@{*/
#define NAMESEP                                                 ':'
#define BLANKS                                                  " \t"
#define LIST_SEP                                                " \t,"
#define IN_PARTS                                                "in"
/*@}*/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static char default_type_names[MAXVECTORS];

/** @name Print symbol counters and lists */
/*@{*/
static INT NPrintVectors=0;
static INT NPrintMatrixs=0;
static VECDATA_DESC *PrintVector[MAX_PRINT_SYM];
static MATDATA_DESC *PrintMatrix[MAX_PRINT_SYM];
/*@}*/

/** @name Environment dir and var ids */
/*@{*/
static INT theNewFormatDirID;                   /* env type for NewFormat dir           */
static INT theVecVarID;                                 /* env type for VEC_TEMPLATE vars       */
static INT theMatVarID;                                 /* env type for MAT_TEMPLATE vars       */
/*@}*/

REP_ERR_FILE


static char *DisplayVecDD (const VECDATA_DESC *vd, INT type, const DOUBLE *data, const char *indent, char *s)
{
  INT i,n,off;

  n = VD_NCMPS_IN_TYPE(vd,type);
  if (n==0) return (s);

  off = VD_OFFSET(vd,type);

  s += sprintf(s,"%s%s:",indent,ENVITEM_NAME(vd));
  for (i=0; i<n; i++)
    s += sprintf(s,VFORMAT,VM_COMP_NAME(vd,off+i),data[VD_CMP_OF_TYPE(vd,type,i)]);

  *(s++) = '\n';

  return (s);
}

/****************************************************************************/
/** \brief Print selected vector user data for the 'nsr' format

   \param type - consider only this type
   \param data - user data
   \param indent - is printed at the beginning of lines
   \param s - output string

        Print selected vector user data for the 'nsr' format.

        \return
        0: ok

        \sa
        setformat, showformat
 */
/****************************************************************************/

static INT PrintTypeVectorData (INT type, void *data, const char *indent, char *s)
{
  INT i;

  for (i=0; i<NPrintVectors; i++)
    s = DisplayVecDD(PrintVector[i],type,(double*)data,indent,s);

  /* remove last \n */
  *s = '\0';

  return(0);
}

static char *DisplayMatDD (const MATDATA_DESC *md, INT type, const DOUBLE *data, const char *indent, char *s)
{
  INT comp,i,j,nr,nc,off;

  nr = MD_ROWS_IN_MTYPE(md,type);
  nc = MD_COLS_IN_MTYPE(md,type);
  if (nr==0) return (s);

  /* diagonals get the same name */
  off = MD_MTYPE_OFFSET(md,MTP(MTYPE_RT(type),MTYPE_CT(type)));

  for (i=0; i<nr; i++)
  {
    s += sprintf(s,"%s%s:",indent,ENVITEM_NAME(md));
    for (j=0; j<nc; j++)
    {
      comp = MD_IJ_CMP_OF_MTYPE(md,type,i,j);
      if (comp<0)
        s += sprintf(s,MFORMAT,VM_COMP_NAME(md,2*(off+i*nc+j)),
                     VM_COMP_NAME(md,2*(off+i*nc+j)+1),
                     0.0);
      else
        s += sprintf(s,MFORMAT,VM_COMP_NAME(md,2*(off+i*nc+j)),
                     VM_COMP_NAME(md,2*(off+i*nc+j)+1),
                     data[MD_IJ_CMP_OF_MTYPE(md,type,i,j)]);
    }
    *(s++) = '\n';
  }

  return (s);
}

/****************************************************************************/
/** \brief Print selected matrix user data for the 'nsr' format

   \param type - consider this mat type
   \param data - user data
   \param indent - is printed at the beginning of lines
   \param s - output string

        Print selected matrix user data for the 'nsr' format.

        \return
        0: ok

        \sa
        setformat, showformat
 */
/****************************************************************************/

static INT PrintTypeMatrixData (INT type, void *data, const char *indent, char *s)
{
  INT i;

  for (i=0; i<NPrintMatrixs; i++)
    s = DisplayMatDD(PrintMatrix[i],type,(double*)data,indent,s);

  /* remove last \n */
  *s = '\0';

  return(0);
}

/****************************************************************************/
/** \brief Create a VEC_TEMPLATE

   \param theMG		- multigrid
   \param name		- name of the VEC_TEMPLATE

        Create a VEC_TEMPLATE in the /newformat directory of the environment.

        \return <ul>
        <li> Pointer to VEC_TEMPLATE if ok </li>
        <li> NULL if an error occured </li>
        </ul>
 */
/****************************************************************************/

static VEC_TEMPLATE *CreateVecTemplate (const char *name)
{
  VEC_TEMPLATE *vt;
  const char *token;

  if (name == NULL) REP_ERR_RETURN_PTR (NULL);
  if (ChangeEnvDir("/newformat")==NULL)
    REP_ERR_RETURN_PTR(NULL);

  vt = (VEC_TEMPLATE *) MakeEnvItem (name,theVecVarID,sizeof(VEC_TEMPLATE));
  if (vt==NULL) REP_ERR_RETURN_PTR (NULL);
  VT_NSUB(vt) = 0;
  VT_NID(vt) = NO_IDENT;
  token = DEFAULT_NAMES;
  for (size_t j=0; j<MAX(MAX_VEC_COMP,strlen(DEFAULT_NAMES)); j++)
    VT_COMPNAME(vt,j) = token[j];

  return (vt);
}

static INT RemoveTemplateSubs (FORMAT *fmt)
{
  ENVITEM *item;
  VEC_TEMPLATE *vt;
  MAT_TEMPLATE *mt;
  INT i;

  for (item=ENVITEM_DOWN(fmt); item != NULL; item = NEXT_ENVITEM(item))
    if (ENVITEM_TYPE(item) == theVecVarID)
    {
      vt = (VEC_TEMPLATE*) item;

      for (i=0; i<VT_NSUB(vt); i++)
        if (VT_SUB(vt,i)!=NULL)
          FreeEnvMemory(VT_SUB(vt,i));
      VT_NSUB(vt) = 0;
    }
    else if (ENVITEM_TYPE(item) == theMatVarID)
    {
      mt = (MAT_TEMPLATE*) item;

      for (i=0; i<MT_NSUB(mt); i++)
        if (MT_SUB(mt,i)!=NULL)
          FreeEnvMemory(MT_SUB(mt,i));
      MT_NSUB(mt) = 0;
    }
  return (0);
}


/**********************************************************************************/
/** \page newformat newformat - Init a format and allocate templates for vector and matrix descriptors

        The 'newformat' command enrols a format for multigrid user data.
        It also creates templates for vector and matrix descriptors.

   \section Syntax
   \verbatim
   newformat <format name>
        {$T <type specifier>}*
        {$V <dofs per type list>: <vector template name> <total needed>
                [$comp <comp names>
                        [$ident <comp identification>]
                        [$sub <sub name> <vector comp name list>]*]
        }+
        {{$M implicit(<rvt>[,<cvt>][|<sparse_matrix_name>]): <matrix template name> <total needed>
 |
         $M <matrix size list>: <matrix template name> <total needed>
                [$comp <matrix comp names>]
        }
                [$sub <sub name> {<rows>x<cols> <matrix comp name list>} | {implicit(<rsv>/<rvt>[,<csv>/<cvt>])} [$alloc <n>]]
        }+
    [$d <type name1>x<type name2><connection depth>]
    [$I <type name><mat_size>]
    [$NE]
    [$e <size>]
    [$n <size>]
   \endverbatim

        Use T-option(s) for definition of types (may be omitted, see below):
        <ul>
        <li> \<type~specifier\>   - \<type name\> in \<domain part list\>: \<object list\> </li>
        <li> \<type~name\>        - \<character\> </li>
        <li> \<domain~part~list\> - \<int\> {, \<int\>}* </li>
        <li> \<object~list\>      - \<obj\> {, \<obj\>}* </li>
        <li> \<obj\>              - nd | ed | el | si </li>
        </ul>

        NB: If no T-option is found at all it is assumed that default types are defined:
        <ul>
        <li>
        $T n in 0,...: nd $T k in 0,...: ed $T e in 0,...: el $T s in 0,...: si
        </li>
        </ul>
        to ensure downward compatibility.


        Use V-options for definition of vector templates:~
   .     <dofs~per~type~list>		- {<type name><int>}+
   .     <total~needed>			- int for total number of this template needed
                                                                        simultaneously

        Use comp-option following a V-option to specify component names:~
   .     <comp~names>				- string of single chracters, one per dof

        Use ident-option following a comp-option to specify component identification:~
   .     <comp~identification>		- string of single chracters, one per dof, multiple occurence
                                                                        of a character will invoke identification of the resp.
                                                                        components for the convergence test as well as output of
                                                                        defect and defect-reduction in solvers

        Use sub-option(s) following a comp-option to define sub templates for
        vector templates:~
   .     <vector~comp~name~list>	- any combination of characters from <comp~names>

        Use M implicit-option(s) to define matrix templates as tensor product <rvt>x<cvt>:~
   .     <rvt>						- row vector template name (defined above in a V-option)
   .     <cvt>						- col vector template name (defined above in a V-option)
        If ',<cvt' is omitted, the tensor product <rvt>x<rvt> is take.

        Use sub-option similar to those for vector templates (component names are defined in
        canonic way):~
   .     <matrix~comp~name~list>	- {<matrix comp name> }+
   .     <matrix~comp~name>		- two characters referring to the component
                                                                        names of vt1/2 (which have to be given
                                                                        above!) indicating row and col

        Alternatively specify sub-matrices by an implicit declaration deriving the sub-matrix
        as tensor product of one (or two) sub-vectors for certain vector templates:~
   .n    implicit(<rsv>/<rvt>[,<csv>/<cvt>])
   .		rsv - row sub vector name of
   .		rvt - row vector template name
   .		csv - col sub vector name of
   .		cvt - col vector template name
        If ',<csv>/<cvt>' is omitted, the tensor product of <rsv>x<rsv> is take.

        If additional storage allocation for submatrices is desired use option
   .n    alloc~<n> - allocate storage for <n> extra submatrices
        following immediately after a sub-matrix declaration.

        A second syntax for M-option(s) is still supported:~
   .     <matrix~size~list>		- {<matrix size> }+
   .     <matrix~size>				- <dofs per type list>x<dofs per type list>

        To define sub-matrix-templates as above you then have to specify component
        names with a comp-option following the M-option (second format only).

        \subsection fo Further Options
   .	  d							- specify connection depth other than 0 (inside element only)
                                                                  for <type name1>x<type name2> connections
   .	  I							- (capital i) specify interpolation matrices
   .	  NE						- require node element lists to be generated
   .	  e							- user data in elements in bytes
   .	  n							- user data in nodes in bytes

   \subsection moreInfo More Information
        The sparse_matrix_name must be a structure in
        ':SparseFormats' that contains for every combination of types a
        string 'T<type~name><type~name>' (three characters).  Since
        usually a modified sparse structure will be needed for "diagonal"
        matrices (here we mean matrices pointing to the vector itself), a
        string 'D<type~name><type~name>' has to be supplied as well.
        This string consists of '0's and '*'s meaning zero resp. non-zero
        entries.  Additionally, non-zero entries may be identified by
        using the characters 'a-z' at the appropriate places.

   \section Example
   \verbatim
   newformat myfmt
        $T a in 0: nd,ed $T b in 1: el		# defines 2 abstract types:
 #      a in domain part 0 including nd and ed objects
 #      b in domain part 1 including el objects

        $V a3b2: vt 5 $comp uvwxy			# template vt contains 3 dofs in nd and ed of part 0
 #					   2 dofs in el of part 1
 # storage reservation for 5 VECDATA_DESCs of vt is made
 # component names in nodes and edges of part 0 (type a) are uvw
 # component names in elements of part 1 (type b) are xy

    $M implicit(vt): mt 2;				# implicit generation of a matrix template mt
 # with storage reservation for 2 of those matrices
   \endverbatim

   \section Keywords
        storage, format
 */
/**********************************************************************************/

static INT ScanVecOption (INT argc, char **argv,                        /* option list						*/
                          INT *curropt,                                 /* next option to scan				*/
                          INT po2t[][MAXVOBJECTS],          /* part-obj to type table			*/
                          INT MaxType,                                  /* bound for type id				*/
                          const char TypeNames[],                       /* names of types					*/
                          INT TypeUsed[],                                       /* indicate whether type is used	*/
                          INT *nvec,                                            /* just an index for templates		*/
                          SHORT VecStorageNeeded[])             /* to accumulate storage per type	*/
{
  VEC_TEMPLATE *vt,*vv;
  SUBVEC *subv;
  INT i,j,type,nsc[NMATTYPES];
  INT opt;
  SHORT offset[NMATOFFSETS];
  char tpltname[NAMESIZE],*names,*token,*p,tp;
  char ident[V_COMP_NAMES];
  int n;

  opt = *curropt;

  /* find name separator */
  if ((names=strchr(argv[opt],NAMESEP))==NULL)
  {
    PrintErrorMessageF('E',"newformat","separate names by a colon ':' from the description (in '$%s')",argv[opt]);
    REP_ERR_RETURN (1);
  }
  *(names++) = '\0';

  /* create a vector template with default name */
  if (sscanf(names,"%d",&n) == 1)
  {
    PrintErrorMessageF('E',"newformat","specifying a number only is not\n"
                       "supported anymore: see man pages (in '$%s')",argv[opt]);
    REP_ERR_RETURN (1);
  }
  if (sscanf(names,"%s",tpltname) != 1)
  {
    PrintErrorMessageF('E',"newformat","no default name specified (in '$%s')",argv[opt]);
    REP_ERR_RETURN (1);
  }
  if (strstr(tpltname,GENERATED_NAMES_SEPERATOR)!=NULL)
  {
    PrintErrorMessageF('E',"newformat",
                       "vector template name '%s' is not allowed to contain '%s' (in '$%s')",
                       tpltname,GENERATED_NAMES_SEPERATOR,argv[opt]);
    REP_ERR_RETURN (1);
  }
  (*nvec)++;
  vt = CreateVecTemplate(tpltname);
  if (vt == NULL) {
    PrintErrorMessageF('E',"newformat",
                       "could not allocate environment storage (in '$%s')",argv[opt]);
    REP_ERR_RETURN (2);
  }

  /* read types and sizes */
  for (type=0; type<NVECTYPES; type++) VT_COMP(vt,type) = 0;
  token = strtok(argv[opt]+1,BLANKS);
  while (token!=NULL) {
    if (sscanf(token,"%c%d",&tp,&n)!=2) {
      PrintErrorMessageF('E',"newformat",
                         "could not scan type and size (in '$%s')",argv[opt]);
      REP_ERR_RETURN (1);
    }
    for (type=0; type<MaxType; type++)
      if (tp==TypeNames[type])
        break;
    if (type>=MaxType)
    {
      PrintErrorMessageF('E',"newformat","no valid type name '%c' (in '$%s')",tp,argv[opt]);
      REP_ERR_RETURN (1);
    }
    TypeUsed[type] = true;
    if (VT_COMP(vt,type) !=0 ) {
      PrintErrorMessageF('E',"newformat",
                         "double vector type specification (in '$%s')",argv[opt]);
      REP_ERR_RETURN (1);
    }
    VT_COMP(vt,type) = n;
    token = strtok(NULL,BLANKS);
  }

  /* check next arg for compnames */
  if (opt+1 < argc)
    if (strncmp(argv[opt+1],"comp",4)==0) {
      opt++;
      if (sscanf(argv[opt],"comp %s",VT_COMPNAMES(vt))!=1) {
        PrintErrorMessageF('E',"newformat",
                           "no vector comp names specified with comp option (in '$%s')",argv[opt]);
        REP_ERR_RETURN (1);
      }
      ConstructVecOffsets(VT_COMPS(vt),offset);
      if (strlen(VT_COMPNAMES(vt))!=(size_t)offset[NVECTYPES]) {
        PrintErrorMessageF('E',"newformat",
                           "number of vector comp names != number of comps (in '$%s')",argv[opt]);
        REP_ERR_RETURN (1);
      }
      /* check uniqueness */
      for (p=VT_COMPNAMES(vt); *p!='\0'; p++)
        if (strchr(p+1,*p)!=NULL)
        {
          PrintErrorMessageF('E',"newformat",
                             "vec component names are not unique (in '$%s')",argv[opt]);
          REP_ERR_RETURN (1);
        }

      /* check next arg for ident */
      if (opt+1 < argc)
        if (strncmp(argv[opt+1],"ident",5)==0)
        {
          opt++;
          if (sscanf(argv[opt],"ident %s",ident)!=1) {
            PrintErrorMessageF('E',"newformat",
                               "no vector comp names specified with ident option (in '$%s')",argv[opt]);
            REP_ERR_RETURN (1);
          }
          if (strlen(ident)!=(size_t)offset[NVECTYPES]) {
            PrintErrorMessageF('E',"newformat",
                               "number of ident comp names != number of comps (in '$%s')",argv[opt]);
            REP_ERR_RETURN (1);
          }

          /* compute identification table */
          VT_NID(vt) = 0;
          for (i=0; i<offset[NVECTYPES]; i++)
            for (j=0; j<=i; j++)
              if (ident[i]==ident[j])
              {
                VT_IDENT(vt,i) = j;
                if (i==j)
                  VT_NID(vt)++;
                break;
              }
        }

      /* check next args for subv */
      while ((opt+1<argc) && (strncmp(argv[opt+1],"sub",3)==0)) {
        opt++;
        if (VT_NSUB(vt)>=MAX_SUB) {
          PrintErrorMessageF('E',"newformat",
                             "max number of vector subs exceeded (in '$%s')",argv[opt]);
          REP_ERR_RETURN (1);
        }
        subv = (SUBVEC*)AllocEnvMemory(sizeof(SUBVEC));
        if (subv == NULL) {
          PrintErrorMessageF('E',"newformat",
                             "could not allocate environment storage (in '$%s')",argv[opt]);
          REP_ERR_RETURN (2);
        }
        memset(subv,0,sizeof(SUBVEC));
        VT_SUB(vt,VT_NSUB(vt)) = subv;
        VT_NSUB(vt)++;

        /* subv name */
        token = strtok(argv[opt]+3,BLANKS);
        if (token==NULL) {
          PrintErrorMessageF('E',"newformat",
                             "specify name of subv (in '$%s')",argv[opt]);
          REP_ERR_RETURN (1);
        }
        if (strstr(token,GENERATED_NAMES_SEPERATOR)!=NULL)
        {
          PrintErrorMessageF('E',"newformat",
                             "sub vector name '%s' is not allowed to contain '%s' (in '$%s')",
                             token,GENERATED_NAMES_SEPERATOR,argv[opt]);
          REP_ERR_RETURN (1);
        }
        strcpy(SUBV_NAME(subv),token);

        /* check uniqueness of name */
        for (i=0; i<VT_NSUB(vt)-1; i++)
          if (strcmp(SUBV_NAME(VT_SUB(vt,i)),SUBV_NAME(subv))==0)
          {
            PrintErrorMessageF('E',"newformat",
                               "subv name not unique (in '$%s')",argv[opt]);
            REP_ERR_RETURN (1);
          }

        /* subv comps */
        for (type=0; type<NVECTYPES; type++) nsc[type] = 0;
        while ((token=strtok(NULL,BLANKS))!=NULL) {
          if (strlen(token)!=1) {
            PrintErrorMessageF('E',"newformat",
                               "specify one char per subv comp (in '$%s')",argv[opt]);
            REP_ERR_RETURN (1);
          }
          if (strchr(VT_COMPNAMES(vt),*token)==NULL) {
            PrintErrorMessageF('E',"newformat",
                               "wrong subv comp");
            REP_ERR_RETURN (1);
          }
          /* component relative to template */
          n = strchr(VT_COMPNAMES(vt),*token)
              - VT_COMPNAMES(vt);

          /* corresponding type */
          for (type=0; type<NVECTYPES; type++)
            if (n<offset[type+1]) break;
          if (nsc[type]>=MAX_VEC_COMP) {
            PrintErrorMessageF('E',"newformat",
                               "max number of subv comps exceeded (in '$%s')",argv[opt]);
            REP_ERR_RETURN (1);
          }
          SUBV_COMP(subv,type,nsc[type]++) = n-offset[type];
        }
        for (type=0; type<NVECTYPES; type++)
          SUBV_NCOMP(subv,type) = nsc[type];
      }
    }

  /* read names of templates */
  if (sscanf(names,"%s %d",tpltname,&n) != 2)
  {
    /* old style: template list (should be avoided) */
    n = 1;
    token = strtok(names,BLANKS);
    token = strtok(NULL,BLANKS);                /* skip first (we already have that as default) */
    while (token!=NULL) {
      n++;
      (*nvec)++;
      vv = CreateVecTemplate(token);
      if (vv == NULL) {
        PrintErrorMessageF('E',"newformat",
                           "could not allocate environment storage (in '$%s')",argv[opt]);
        REP_ERR_RETURN (2);
      }
      /* everything but the name is as in the default template vt */
      for (type=0; type<NVECTYPES; type++)
        VT_COMP(vv,type) = VT_COMP(vt,type);
      for (j=0; j<MAX_VEC_COMP; j++)
        VT_COMPNAME(vv,j) = VT_COMPNAME(vt,j);
      VT_NSUB(vv) = VT_NSUB(vt);
      for (j=0; j<VT_NSUB(vt); j++)
        VT_SUB(vv,j) = VT_SUB(vt,j);
      token = strtok(NULL,BLANKS);
    }
  }
  /* compute storage needed */
  for (type=0; type<NVECTYPES; type++)
    VecStorageNeeded[type] += n * VT_COMP(vt,type);

  *curropt = opt;

  return (0);
}

static INT ScanTypeOptions (INT argc, char **argv, INT po2t[][MAXVOBJECTS], INT *MaxTypes, char TypeNames[])
{
  INT i,j,opt,found,max;
  INT nparts,partlist[MAXDOMPARTS];
  INT nobjs,objlist[MAXDOMPARTS];
  char *objstr,*partstr,c,*token;
  int part;

  /* init po2t */
  for (i=0; i<MAXDOMPARTS; i++)
    for (j=0; j<MAXVOBJECTS; j++)
      po2t[i][j] = NOVTYPE;

  /* scan type specifications from option list */
  found = max = 0;
  for (opt=1; opt<argc; opt++)
    if (argv[opt][0]=='T')
    {
      if (max>=(1<<VTYPE_LEN))
      {
        printf("I would love to define another type for you, but control flags are rare... (in '$%s')",argv[opt]);
        ASSERT(false);
        REP_ERR_RETURN(1);
      }

      found++;

      /* scan type name */
      if (sscanf(argv[opt],"T %c",&c)!=1)
      {
        PrintErrorMessageF('E',"newformat",
                           "type name not found (in '$%s')",argv[opt]);
        REP_ERR_RETURN(1);
      }
      for (i=0; i<max; i++)
        if (c==TypeNames[i])
        {
          PrintErrorMessageF('E',"newformat",
                             "duplicate type names '%c' (in '$%s')",c,argv[opt]);
          REP_ERR_RETURN(1);
        }
      if ((c<FROM_VTNAME) || (TO_VTNAME<c))
      {
        PrintErrorMessageF('E',"newformat",
                           "type name '%c' out of range [%c-%c] (in '$%s')",c,FROM_VTNAME,TO_VTNAME,argv[opt]);
        REP_ERR_RETURN(1);
      }
      TypeNames[max] = c;

      /* separate object list */
      if ((objstr=strchr(argv[opt],NAMESEP))==NULL)
      {
        PrintErrorMessageF('E',"newformat",
                           "no type sperator ':' found in T-option (in '$%s')",argv[opt]);
        REP_ERR_RETURN(1);
      }
      *(objstr++) = '\0';

      /* scan part list */
      if ((partstr=strstr(argv[opt],IN_PARTS))==NULL)
      {
        PrintErrorMessageF('E',"newformat",
                           "no '%s' token found in T-option (in '$%s')",IN_PARTS,argv[opt]);
        REP_ERR_RETURN(1);
      }
      token = strtok(partstr+strlen(IN_PARTS),LIST_SEP);
      nparts = 0;
      while (token!=NULL)
      {
        if (sscanf(token,"%d",&part)!=1)
        {
          PrintErrorMessageF('E',"newformat",
                             "could not scan parts in part-list (in '$%s')",argv[opt]);
          REP_ERR_RETURN(1);
        }
        if ((part<0) || (MAXDOMPARTS<=part))
        {
          PrintErrorMessageF('E',"newformat",
                             "part out of range [%d-%d] (in '$%s')",0,MAXDOMPARTS-1,argv[opt]);
          REP_ERR_RETURN(1);
        }
        partlist[nparts++] = part;
        token = strtok(NULL,LIST_SEP);
      }

      /* scan object list */
      token = strtok(objstr,LIST_SEP);
      nobjs = 0;
      while (token!=NULL)
      {
        for (i=0; i<MAXVOBJECTS; i++)
          if (strcmp(token,ObjTypeName[i])==0)
            break;
        if (i>=MAXVOBJECTS)
        {
          PrintErrorMessageF('E',"newformat",
                             "could not scan object '%s' in object-list (in '$%s')",token,argv[opt]);
          REP_ERR_RETURN(1);
        }
        objlist[nobjs++] = i;
        token = strtok(NULL,LIST_SEP);
      }

      /* update po2t table */
      for (i=0; i<nparts; i++)
        for (j=0; j<nobjs; j++)
          if (po2t[partlist[i]][objlist[j]]!=NOVTYPE)
          {
            PrintErrorMessageF('E',"newformat",
                               "the combination of obj %s in part %d is already defined (in '$%s')",ObjTypeName[objlist[j]],partlist[i],argv[opt]);
            REP_ERR_RETURN(1);
          }
          else
            po2t[partlist[i]][objlist[j]] = max;
      max++;
    }

  if (!found)
  {
    /* no T-option: set default types in part 0 */
    for (max=0; max<MAXVOBJECTS; max++)
    {
      TypeNames[max] = default_type_names[max];
      po2t[0][max] = max;
    }
  }

  *MaxTypes = max;

  return (0);
}

static INT CleanupTempDir (void)
{
  ENVDIR *dir;

  dir = ChangeEnvDir("/newformat");
  if (dir == NULL) {
    PrintErrorMessage('E',"CleanupTempDir","/newformat does not exist");
    REP_ERR_RETURN (1);
  }

  if (RemoveTemplateSubs((FORMAT *) dir))
    REP_ERR_RETURN (1);

  ChangeEnvDir("/");
  ENVITEM_LOCKED(dir) = 0;
  if (RemoveEnvDir((ENVITEM *) dir))
    REP_ERR_RETURN (1);

  return (0);
}

INT NS_DIM_PREFIX CreateFormatCmd (INT argc, char **argv)
{
  FORMAT *newFormat;
  ENVDIR *dir;
  VectorDescriptor vd[MAXVECTORS];
  MatrixDescriptor md[MAXMATRICES*MAXVECTORS];
  INT opt,i,j,size,type,type2,rtype,ctype,nvec,nmat,nvd,nmd;
  INT po2t[MAXDOMPARTS][MAXVOBJECTS],MaxTypes,TypeUsed[MAXVECTORS];
  SHORT ImatTypes[NVECTYPES];
  SHORT VecStorageNeeded[NVECTYPES],MatStorageNeeded[NMATTYPES];
  char formatname[NAMESIZE],TypeNames[NVECTYPES];

  /* scan name of format */
  if (sscanf(argv[0],expandfmt(" newformat %" NAMELENSTR "[ -~]"), formatname) != 1 || strlen(formatname) == 0) {
    PrintErrorMessage('E',"newformat","no format name specified");
    REP_ERR_RETURN (1);
  }
  if (GetFormat(formatname) != NULL) {
    PrintErrorMessage('W',"newformat","format already exists");
    return 0;
  }

  /* install the /newformat directory */
  if (ChangeEnvDir("/")==NULL) {
    PrintErrorMessage('F',"InitFormats","could not changedir to root");
    REP_ERR_RETURN(__LINE__);
  }
  if (MakeEnvItem("newformat",theNewFormatDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitFormats",
                      "could not install '/newformat' dir");
    REP_ERR_RETURN(__LINE__);
  }

  /* init */
  for (type=0; type<NVECTYPES; type++)
    ImatTypes[type] = VecStorageNeeded[type] = TypeUsed[type] = 0;
  for (type=0; type<NMATTYPES; type++)
    MatStorageNeeded[type] = 0;
  nvec = nmat = 0;

  /* scan type option or set default po2t */
  if (ScanTypeOptions(argc,argv,po2t,&MaxTypes,TypeNames)) {
    CleanupTempDir();
    REP_ERR_RETURN(1);
  }

  /* scan other options */
  for (opt=1; opt<argc; opt++)
    switch (argv[opt][0])
    {
    case 'V' :
      if (ScanVecOption(argc,argv,&opt,po2t,MaxTypes,TypeNames,TypeUsed,&nvec,VecStorageNeeded)) {
        CleanupTempDir();
        REP_ERR_RETURN(1);
      }
      break;
    default :
      PrintErrorMessageF('E',"newformat","(invalid option '%s')",argv[opt]);
      CleanupTempDir();
      REP_ERR_RETURN (1);
    }

  /* remove types not needed from po2t */
  for (i=0; i<MAXDOMPARTS; i++)
    for (j=0; j<MAXVOBJECTS; j++)
      if (po2t[i][j] != NOVTYPE && !TypeUsed[po2t[i][j]])
        po2t[i][j] = NOVTYPE;

  /* now we are ready to create the format */

  /* fill degrees of freedom needed */
  nvd = 0;
  for (type=0; type<NVECTYPES; type++)
    if (VecStorageNeeded[type]>0)
    {
      vd[nvd].tp    = type;
      vd[nvd].size  = VecStorageNeeded[type]*sizeof(DOUBLE);
      vd[nvd].name  = TypeNames[type];
      nvd++;

      if (nvd > MAXVECTORS) {
        PrintErrorMessage('E',"newformat","increase MAXVECTORS");
        CleanupTempDir();
        REP_ERR_RETURN (1);
      }
    }

  /* fill connections needed */
  nmd = 0;
  for (type=0; type<NMATTYPES; type++)
  {
    rtype = MTYPE_RT(type); ctype = MTYPE_CT(type);

    size = MatStorageNeeded[type];

    if (ctype==rtype)
    {
      /* ensure diag/matrix coexistence (might not be necessary) */
      type2=(type<NMATTYPES_NORMAL) ? DMTP(rtype) : MTP(rtype,rtype);
      if ((size<=0) && (MatStorageNeeded[type2]<=0)) continue;
    }
    else
    {
      /* ensure symmetry of the matrix graph */
      type2=MTP(ctype,rtype);
      if ((size<=0) && (MatStorageNeeded[type2]<=0)) continue;
    }

    md[nmd].from  = rtype;
    md[nmd].to    = ctype;
    md[nmd].diag  = (type>=NMATTYPES_NORMAL);
    md[nmd].size  = size*sizeof(DOUBLE);
    md[nmd].depth = 0;
    nmd++;

    if (nmd > MAXMATRICES*MAXVECTORS) {
      PrintErrorMessage('E',"newformat","increase MAXMATRICES");
      CleanupTempDir();
      REP_ERR_RETURN (1);
    }
  }

  /* create format */
  newFormat = CreateFormat(formatname,nvd,vd,nmd,md,ImatTypes,po2t);
  if (newFormat==NULL)
  {
    PrintErrorMessage('E',"newformat","failed creating the format");
    REP_ERR_RETURN (CleanupTempDir());
  }

  /* move templates into the new directory */
  dir = ChangeEnvDir("/newformat");
  if (dir == NULL) {
    PrintErrorMessage('E',"newformat","failed moving template");
    REP_ERR_RETURN (4);
  }
  if (ENVITEM_DOWN((ENVDIR *)newFormat) != NULL) {
    PrintErrorMessage('E',"newformat","failed moving template");
    REP_ERR_RETURN (4);
  }
  ENVITEM_DOWN((ENVDIR *)newFormat) = ENVITEM_DOWN(dir);
  ENVITEM_DOWN(dir) = NULL;
  ENVITEM_LOCKED(dir) = 0;
  ChangeEnvDir("/");
  if (RemoveEnvDir((ENVITEM *)dir))
    PrintErrorMessage('W',"InitFormats","could not remove newformat dir");

  return 0;
}


/****************************************************************************/
/*                                                                          */
/* Function:  InitFormats	                                                */
/*                                                                          */
/* Purpose:   calls all inits of format definitions                         */
/*                                                                          */
/* Input:     none                                                          */
/*                                                                          */
/* Output:    INT 0: everything ok                                          */
/*            INT 1: fatal error (not enough env. space, file not found...  */
/*                                                                          */
/****************************************************************************/

INT NS_DIM_PREFIX InitFormats ()
{
  INT tp;

  theNewFormatDirID = GetNewEnvDirID();
  theVecVarID = GetNewEnvVarID();
  theMatVarID = GetNewEnvVarID();

  if (MakeStruct(":SparseFormats")!=0) return(__LINE__);

  /* init default type names */
  for (tp=0; tp<MAXVECTORS; tp++)
    switch (tp) {
    case NODEVEC : default_type_names[tp] = 'n'; break;
    case EDGEVEC : default_type_names[tp] = 'k'; break;
    case ELEMVEC : default_type_names[tp] = 'e'; break;
    case SIDEVEC : default_type_names[tp] = 's'; break;
    default :
      PrintErrorMessage('E',"newformat","Huh");
      return (__LINE__);
    }

  return (0);
}
