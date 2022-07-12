// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      typemgr.c                                                     */
/*                                                                          */
/* Purpose:   declaring and defining DDD_TYPEs                              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/22 kb  begin                                            */
/*            94/09/13 kb  added DDD_Status                                 */
/*            95/11/03 kb  complete rewrite of StructRegister code          */
/*            95/11/16 kb  copied from main.c, introduced clean type concept*/
/*            95/12/07 jb  added fortran frontend                           */
/*            96/04/02 kb  added EL_CONTINUE feature to TypeDefine()        */
/*            97/02/12 kb  added CPP_FRONTEND                               */
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

/* standard C library */
#include <config.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <stdarg.h>
#include <cstring>

#include <iomanip>
#include <iostream>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

  START_UGDIM_NAMESPACE

/*
   #define DebugTypeDefine
   #define DebugCopyMask
   #define DebugNoStructCompress
 */


/****************************************************************************/
/*                                                                          */
/* definition of constants                                                  */
/*                                                                          */
/****************************************************************************/

enum DDD_TypeModes
{
  DDD_TYPE_INVALID = 0,            /* DDD_TYPE not declared, not defined      */
  DDD_TYPE_DECLARED,               /* DDD_TYPE declared, but not defined      */
  DDD_TYPE_CONTDEF,                /* DDD_TYPE declared and partially defined */
  DDD_TYPE_DEFINED                 /* DDD_TYPE declared and defined           */
};

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

static void InitHandlers (TYPE_DESC *);


int ddd_TypeDefined (TYPE_DESC *desc)
{
  return(desc->mode==DDD_TYPE_DEFINED);
}


/****************************************************************************/


/*
        print out trailing part of error message during TypeDefine process

        error occurred during TypeDefine for desc, with argument argno
 */
class RegisterError
{
public:
  RegisterError(TYPE_DESC *desc, int argno)
    : desc(desc)
    , argno(argno)
    {}

  friend std::ostream& operator<<(std::ostream& out, const RegisterError& e)
  {
    if (e.argno != 0)
      out << ", arg " << e.argno << " of ";
    else
      out << " in ";
    out << "DDD_TypeDefine(\"" << e.desc->name << "/" << e.desc->currTypeDefCall << "\")";
    return out;
  }

private:
  TYPE_DESC* desc;
  int argno;
};




/*
        check ELEM_DESC for plausibility
 */

static int CheckBounds (TYPE_DESC *desc, ELEM_DESC *el, int argno)
{
  if (el->offset < 0)
  {
    Dune::dwarn << "negative offset" << RegisterError(desc, argno) << "\n";
    return(ERROR);
  }

  if (el->size<=0)
  {
    Dune::dwarn << "illegal element size" << RegisterError(desc, argno) << "\n";
    return (ERROR);
  }

  return 0;
}



/*
        check ELEM_DESC list of given TYPE_DESC for bad overlapping
 */

static int CheckOverlapEls (TYPE_DESC *desc)
{
  int i;
  int ok = true;

  for(i=0; i<desc->nElements; i++)
  {
    ELEM_DESC *e1 = &desc->element[i];

    if (i<desc->nElements-1)
    {
      ELEM_DESC *e2 = &desc->element[i+1];

      if (e1->offset+e1->size > e2->offset)
      {
        ok = false;
        Dune::dwarn << "element too big (offset=" << e1->offset << ")"
                    << RegisterError(desc, 0) << "\n";
      }
    }

    else
    {
      if (e1->offset+e1->size > desc->size)
      {
        ok = false;
        Dune::dwarn << "element too big (offset=" << e1->offset << ")"
                    << RegisterError(desc, 0) << "\n";
      }
    }
  }
  return ok;
}



/*
        constructor for ELEM_DESC
 */

static void ConstructEl (ELEM_DESC *elem, int t, int o, size_t s, DDD_TYPE rt)
{
  elem->type    = t;
  elem->offset  = o;
  elem->size    = s;

  /*
          for OBJPTR elements, store referenced DDD_TYPE here.
          the default is EL_DDDHDR, i.e., if this feature is
          not used, the DDD_HDR will be assumed to be at the
          beginning of each structure (offsetHeader==0).
   */
  EDESC_SET_REFTYPE(elem, rt);
  elem->reftypeHandler = NULL;

  /*
          for GBITS elements, store array of bits. 1=GDATA,
          0=LDATA.
   */
  if (t==EL_GBITS)
  {
    elem->gbits = std::make_unique<unsigned char[]>(s);
  }
}


/*
        register previously defined TYPE_DESC during TypeDefine
 */

static int RecursiveRegister (DDD::DDDContext& context, TYPE_DESC *desc,
                              int i, DDD_TYPE typ, int offs, int argno)
{
  TYPE_DESC *d2 = &context.typeDefs()[typ];
  int j;

  /* inherit elements of other ddd-type */
  for(j=0; j<d2->nElements && i < TYPE_DESC::MAX_ELEMDESC; j++, i++)
  {
    ConstructEl(&desc->element[i],
                d2->element[j].type,
                d2->element[j].offset + offs,
                d2->element[j].size,
                EDESC_REFTYPE(&(d2->element[j])));
    if (CheckBounds(desc, &desc->element[i], argno) == ERROR)
      return(ERROR);
  }

  /* inherit other properties */
  desc->nPointers += d2->nPointers;
  if (d2->hasHeader)
  {
    if (!desc->hasHeader)
    {
      desc->hasHeader = true;
      desc->offsetHeader = d2->offsetHeader + offs;
    }
    else
    {
      if (desc->offsetHeader == d2->offsetHeader+offs)
      {
        Dune::dwarn << "two DDD_HDRs, same offset"
                    << RegisterError(desc, argno) << "\n";
      }
      else
      {
        Dune::dwarn << "only one DDD_HDR allowed"
                    << RegisterError(desc, argno) << "\n";
        return(ERROR);
      }
    }
  }

  return i;
}


/*
        constructor for TYPE_DESC
 */

static void ConstructDesc (TYPE_DESC *desc)
{
  InitHandlers(desc);

  desc->nPointers = 0;
  desc->nElements = 0;
  desc->cmask     = nullptr;
  desc->hasHeader = false;
  desc->offsetHeader = 0;

}



#ifndef DebugNoStructCompress

/*
        remove first element in given ELEM_DESC-list,
        adjust remaining elements
 */

static void DeleteFirstEl (ELEM_DESC *elarray, int n)
{
  int i;

  for(i=1; i<n; i++)
  {
    elarray[i-1] = std::move(elarray[i]);
  }
}

#endif



/*
        normalize ELEM_DESC-list of given TYPE_DESC

        this consists of two parts:
                1) sort ELEM_DESC-list by offset (necessary!!)
                2) compress ELEM_DESC-list according to a set of rules
 */

static int NormalizeDesc (TYPE_DESC *desc)
{
  ELEM_DESC  *elems = desc->element;

  /* sort element array by offset */
  std::sort(
    elems, elems + desc->nElements,
    [](const ELEM_DESC& a, const ELEM_DESC& b) { return a.offset < b.offset; }
    );

  /* check for overlapping elements */
  if (! CheckOverlapEls(desc))
    return false;


#       ifndef DebugNoStructCompress
  {
    /* compile this only if Debug-flag not set */
    int i;

    /* compress element description */
    for(i=0; i<desc->nElements-1; i++)
    {
      size_t realsize;

      /* 1) type must be equal */
      if (elems[i].type != elems[i+1].type)
        continue;

      /* 2) nothing can melt into DDD_HEADER */
      if (desc->hasHeader && elems[i+1].offset==desc->offsetHeader)
        continue;

      /* 3) gap between elements is allowed only for EL_LDATA */
      if ((elems[i].offset+elems[i].size != elems[i+1].offset) &&
          (elems[i].type!=EL_LDATA) )
        continue;

      /* 4) EL_OBJPTRs with different reftypes can't be compressed */
      if (elems[i].type==EL_OBJPTR &&
          ((EDESC_REFTYPE(elems+i)!= EDESC_REFTYPE(elems+(i+1))) ||
           (EDESC_REFTYPE(elems+i)==DDD_TYPE_BY_HANDLER)) )
        continue;

      /* 5) EL_GBITS can't be compressed */
      if (elems[i].type == EL_GBITS)
        continue;

      /* all conditions fit: compress elements */
      realsize = elems[i+1].offset - elems[i].offset;
      elems[i].size = realsize + elems[i+1].size;

      desc->nElements--;
      DeleteFirstEl(&elems[i+1], desc->nElements - i);

      i--;                   /* skip one element back and try again */
    }
  }
#       endif

  return true;
}


/*
        compute copy-mask (for efficiency) and attach it to TYPE_DESC
 */

static void AttachMask(const DDD::DDDContext& context, TYPE_DESC *desc)
{
  unsigned char mask;

  /* get storage for mask */
  desc->cmask = std::make_unique<unsigned char[]>(desc->size);

  /* set default: EL_LDATA for unspecified regions (gaps) */
  for (std::size_t i=0; i<desc->size; i++)
  {
    desc->cmask[i] = 0x00;                    /* dont-copy-flag */
  }

  /* create mask from element list */
  for (int i=0; i<desc->nElements; i++)
  {
    ELEM_DESC *e = &desc->element[i];
    unsigned char* mp = desc->cmask.get() + e->offset;

    switch (e->type)
    {
    case EL_LDATA :
    case EL_OBJPTR :                        /* OBJPTR are LDATA!!! */
      mask = 0x00;                              /* dont-copy-flag */
      break;

    case EL_GDATA :
    case EL_DATAPTR :
      mask = 0xff;                              /* copy-flag */
      break;
    }

    for (std::size_t k=0; k<e->size; k++)
    {
      if (e->type==EL_GBITS)
      {
        mp[k] = e->gbits[k];                           /* copy bitwise */
      }
      else
      {
        mp[k] = mask;
      }
    }
  }

#       ifdef DebugCopyMask
  if (context.isMaster())
  {
    using std::setfill;
    using std::setw;

    Dune::dinfo << "AttachMask for " << desc->name ":";
    for(i=0; i<desc->size; i++)
    {
      if (i%8==0)
        Dune::dinfo << "\n  " << setw(4) << i << ":  ";
      Dune::dinfo << setfill('0') << setw(2) << desc->cmask[i] << setfill(' ') << " ";
    }
    Dune::dinfo << "\n";
  }
#       endif
}





/****************************************************************************/
/*                                                                          */
/* Function:  DDD_TypeDefine                                                */
/*                                                                          */
/* Purpose:   define object structure at runtime                            */
/*                                                                          */
/* Input:     context typ, t0, p0, s0, [r0 [, rh0]], t1, p1, s1 ...         */
/*            with typ:  previously declared DDD_TYPE                       */
/*                 t:    element type                                       */
/*                 p:    offset of member in data structure                 */
/*                 s:    size of element in byte                            */
/*                 r:    (only for EL_OBJPTR): referenced DDD_TYPE          */
/*                 rt:   (only for EL_OBJPTR and DDD_TYPE_BY_HANDLER):      */
/*                          handler for getting reftype on-the-fly.         */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_TypeDefine(DDD::DDDContext& context, DDD_TYPE typ, ...)
{
  auto& nDescr = context.typemgrContext().nDescr;

  size_t argsize;
  size_t argoffset;
  int argtyp, argno;
  DDD_TYPE argrefs;
  int i, nPtr;
  va_list ap;
  char      *gbits;

  /* TODO: only master should be able to define types, other
          procs should receive the correct definition from master.
          (with the current implementation inconsistencies might occur)
   */

  /* test whether typ is valid */
  if (typ>=nDescr)
    DUNE_THROW(Dune::Exception, "invalid DDD_TYPE");

  /* get object description */
  TYPE_DESC* desc = &context.typeDefs()[typ];
  desc->currTypeDefCall++;

  if (desc->mode!=DDD_TYPE_DECLARED && desc->mode!=DDD_TYPE_CONTDEF)
  {
    if (desc->mode==DDD_TYPE_DEFINED)
      DUNE_THROW(Dune::Exception, "DDD_TYPE already defined");
    else
      DUNE_THROW(Dune::Exception, "undeclared DDD_TYPE");
  }


  /* initialize TYPE_DESC struct, only on first call */
  if (desc->currTypeDefCall==1)
    ConstructDesc(desc);


  if (typ==0)        /* i.e. typ==EL_DDDHDR */
  {
    /* DDD_HDR also contains a DDD_HDR (sic!) */
    desc->hasHeader = true;
  }

#       ifdef DebugTypeDefine
  Dune::dinfo << "   DDD_TypeDefine(" << desc->name
              << "/" << desc->currTypeDefCall << ")\n";
#       endif


  /* start variable arguments after "typ"-parameter */
  va_start(ap, typ);

  argno = 1;


  /* loop over variable argument list */

  i = desc->nElements;
  while ((i < TYPE_DESC::MAX_ELEMDESC) &&
         ((argtyp= va_arg(ap, int))!=EL_END) && (argtyp!=EL_CONTINUE))
  {
    HandlerGetRefType arg_rt_handler = NULL;

    /* get the offset of the member in the object */
    argoffset = va_arg(ap, size_t);
    argno+=2;

    /* handle several types of ELEM_DESCs */
    switch (argtyp)
    {
    /* 1) ELEM_DESC is a pointer [array] */
    case EL_OBJPTR :
    case EL_DATAPTR :
      /* get third argument of this definition line */
      argsize = va_arg(ap, size_t); argno++;

      /* EL_OBJPTR have to be specified with target type */
      if (argtyp==EL_OBJPTR)
      {

        /* get fourth argument: referenced DDD_TYPE */
        argrefs = (DDD_TYPE) va_arg(ap, int); argno++;

        /* check whether target type is DDD_TYPE_BY_HANDLER */
        /* this is currently supported only by C_FRONTEND */
        if (argrefs==DDD_TYPE_BY_HANDLER)
        {
          arg_rt_handler = va_arg(ap, HandlerGetRefType);
          argno++;
        }
        else
        {
          /* check whether target type is valid */
          if (argrefs>=nDescr ||
              context.typeDefs()[argrefs].mode==DDD_TYPE_INVALID)
          {
            DUNE_THROW(Dune::Exception,
                       "referencing invalid DDD_TYPE" << RegisterError(desc,argno));
          }
        }
      }
      else
      {
        /* to target type for EL_DATAPTR */
        argrefs = EL_DDDHDR;
      }

      /* compute #pointers (in array) */
      nPtr = argsize / sizeof(void *);

      /* check for plausibility */
      if (nPtr*sizeof(void *) != argsize)
      {
        DUNE_THROW(Dune::Exception,
                   "invalid sizeof" << RegisterError(desc,argno));
      }

      /* remember #pointers */
      desc->nPointers += nPtr;


      /* initialize ELEM_DESC */
      ConstructEl(&desc->element[i],
                  argtyp, argoffset, argsize, argrefs);

      /* set reftype-handler function pointer, if any */
      if (argrefs==DDD_TYPE_BY_HANDLER)
        desc->element[i].reftypeHandler = arg_rt_handler;

      if (CheckBounds(desc, &desc->element[i], argno) == ERROR)
        return;
      i++;

#                               ifdef DebugTypeDefine
      Dune::dinfo << "    PTR, " << std::setw(5) << argoffset
                  << ", " << std::setw(6) << argsize << "\n";
#                               endif

      break;


    /* 2) ELEM_DESC is global or local data */
    case EL_GDATA :
    case EL_LDATA :
      /* get third argument of this definition line */
      argsize = va_arg(ap, size_t); argno++;

      /* initialize ELEM_DESC */
      ConstructEl(&desc->element[i],
                  argtyp, argoffset, argsize, 0);

      if (CheckBounds(desc, &desc->element[i], argno) == ERROR)
        return;
      i++;

#                               ifdef DebugTypeDefine
      Dune::dinfo << "    DAT, " << std::setw(5) << argoffset
                  << ", " << std::setw(6) << argsize << "\n";
#                               endif

      break;



    /* 3) ELEM_DESC is bitwise global or local */
    case EL_GBITS :
      /* get third argument of this definition line */
      argsize = va_arg(ap, size_t); argno++;

      /* initialize ELEM_DESC */
      ConstructEl(&desc->element[i],
                  argtyp, argoffset, argsize, 0);

      /* read forth arg from cmdline */
      gbits = va_arg(ap, char *); argno++;

      /* fill gbits array, read forth arg from cmdline */
      memcpy(desc->element[i].gbits.get(), gbits, argsize);

      if (CheckBounds(desc, &desc->element[i], argno) == ERROR)
        return;

#                               ifdef DebugTypeDefine
      Dune::dinfo << "   BITS, " << std::setw(5) << argoffset
                  << ", " << std::setw(6) << argsize << ", ";
      Dune::dinfo << std::setfill('0') << std::hex;
      for(int ii=0; ii<argsize; ii++)
        Dune::dinfo << std::setw(2) << int(desc->element[i].gbits[ii]) << " ";
      Dune::dinfo << std::setfill(' ') << std::dec << "\n";
#                               endif

      i++;

      break;


    /* 4) ELEM_DESC is a recursively defined DDD_TYPE */
    default :
      /* hierarchical registering of known ddd_types */
      /* no third argument here */

      /* check for plausibility of given DDD_TYPE */
      if (argtyp<0 || argtyp>=nDescr || argtyp==typ)
      {
        DUNE_THROW(Dune::Exception,
                   "undefined DDD_TYPE=" << argtyp
                   << RegisterError(desc,argno-1));
      }

      /* check whether given DDD_TYPE has been defined already */
      if (context.typeDefs()[argtyp].mode==DDD_TYPE_DEFINED)
      {
        /* do recursive TypeDefine */
        i = RecursiveRegister(context, desc,
                              i, argtyp, argoffset, argno);
        if (i==ERROR) HARD_EXIT;                                       /* return; */

#ifdef DebugTypeDefine
        Dune::dinfo
          << "    " << std::setw(3) << argtyp
          << ", " << std::setw(5) << argoffset
          << ", " << std::setw(6) << context.typeDefs()[argtyp].size << "\n";
#endif
      }
      else
      {
        DUNE_THROW(Dune::Exception,
                   "undefined DDD_TYPE " << context.typeDefs()[argtyp].name
                   << RegisterError(desc,argno-1));
      }


      break;
    }
  }


  /* check whether loop has come to a correct end */
  if (i >= TYPE_DESC::MAX_ELEMDESC && argtyp!=EL_END && argtyp!=EL_CONTINUE)
    DUNE_THROW(Dune::Exception, "too many elements" << RegisterError(desc, 0));


  /* remember #elements in TYPE_DESC */
  desc->nElements = i;


  if (argtyp==EL_END)        /* and not EL_CONTINUE */
  {
    /* compute aligned object length */
    desc->size = va_arg(ap, size_t);
    desc->size = CEIL(desc->size);

    /* do normalization */
    if (! NormalizeDesc(desc))
      HARD_EXIT;                                 /*return;*/

    /* attach copy-mask for efficient copying */
    AttachMask(context, desc);

    /* change TYPE_DESC state to DEFINED */
    desc->mode = DDD_TYPE_DEFINED;
  }
  else        /* argtyp==EL_CONTINUE */
  {
    /* change TYPE_DESC state to CONTDEF */
    desc->mode = DDD_TYPE_CONTDEF;
  }

  va_end(ap);
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_TypeDeclare                                               */
/*                                                                          */
/* Purpose:   declare DDD_TYPE at runtime                                   */
/*                                                                          */
/* Input:     name:  object name string                                     */
/*                                                                          */
/* Output:    id of object-description                                      */
/*            -1 on error                                                   */
/*                                                                          */
/****************************************************************************/

DDD_TYPE DDD_TypeDeclare(DDD::DDDContext& context, const char *name)
{
  auto& nDescr = context.typemgrContext().nDescr;
  TYPE_DESC* desc = &context.typeDefs()[nDescr];

  /* check whether there is one more DDD_TYPE */
  if (nDescr==MAX_TYPEDESC)
    DUNE_THROW(Dune::Exception, "no more free DDD_TYPEs");

  /* set status to DECLARED and remember textual type name */
  desc->mode = DDD_TYPE_DECLARED;
  desc->name = name;

  desc->prioMatrix  = nullptr;
  desc->prioDefault = PRIOMERGE_DEFAULT;

  /* increase #DDD_TYPEs, but return previously defined one */
  nDescr++; return(nDescr-1);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_TypeDisplay                                               */
/*                                                                          */
/* Purpose:   show defined DDD_TYPE                                         */
/*                                                                          */
/* Input:     id: DDD_TYPE which will be shown                              */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_TypeDisplay(const DDD::DDDContext& context, DDD_TYPE id)
{
  using std::setw;

  std::ostream& out = std::cout;
  int i;

  /* only master should display DDD_TYPEs */
  if (context.isMaster())
  {
    /* check for plausibility */
    if (id >= context.typemgrContext().nDescr)
      DUNE_THROW(Dune::Exception, "invalid DDD_TYPE " << id);

    const TYPE_DESC* desc = &context.typeDefs()[id];
    if (desc->mode != DDD_TYPE_DEFINED)
      DUNE_THROW(Dune::Exception, "undefined DDD_TYPE " << id);

    /* print header */
    out << "/ Structure of " << (desc->hasHeader ? "DDD" : "data")
        << "--object '" << desc->name <<"', id " << id
        << ", " << desc->size << " byte\n"
        << "|--------------------------------------------------------------\n";

    /* print one line for each element */
    for(i=0; i<desc->nElements; i++)
    {
      const ELEM_DESC *e = &desc->element[i];

      int realnext = (i==desc->nElements-1) ? desc->size : (e+1)->offset;
      int estinext = e->offset+e->size;

      /* handle gap at the beginning */
      if (i==0 && e->offset!=0)
        out << "|" << setw(5) << 0
            << " " << setw(5) << e->offset
            << "    gap (local data)\n";

      /* do visual compression of elems inherited from DDD_HDR */
      if (id==EL_DDDHDR ||
          (!desc->hasHeader) ||
          e->offset < desc->offsetHeader ||
          e->offset >= desc->offsetHeader + context.typeDefs()[EL_DDDHDR].size)
      {
        out << "|" << setw(5) << e->offset
            << " " << setw(5) << e->size << "    ";

        /* print one line according to type */
        switch (e->type)
        {
        case EL_GDATA : out << "global data\n"; break;
        case EL_LDATA : out << "local data\n"; break;
        case EL_DATAPTR : out << "data pointer\n"; break;
        case EL_OBJPTR :
          if (EDESC_REFTYPE(e)!=DDD_TYPE_BY_HANDLER)
            out << "obj pointer (refs "
                << context.typeDefs()[EDESC_REFTYPE(e)].name << ")\n";
          else
            out << "obj pointer (reftype on-the-fly)\n";
          break;
        case EL_GBITS :
          out << "bitwise global: ";
          out << std::setfill('0') << std::hex;
          for (size_t ii=0; ii<e->size; ii++)
            out << setw(2) << int(e->gbits[ii]) << " ";
          out << std::setfill(' ') << std::dec << "\n";
          break;
        }

        /* handle gap */
        if (estinext != realnext)
          out << "|" << setw(5) << estinext << " "
              << setw(5) << (realnext-estinext)
              << "    gap (local data)\n";
      }
      else
      {
        /* handle included DDD_HDR */
        if (e->offset == desc->offsetHeader)
          out << "|" << setw(5) << e->offset
              << " " << setw(5) << context.typeDefs()[EL_DDDHDR].size
              << "    ddd-header\n";
      }

    }
    out << "\\--------------------------------------------------------------\n";
  }
}




static void InitHandlers (TYPE_DESC *desc)
{
  /* set all handler functions to default (=none) */
  desc->handlerLDATACONSTRUCTOR = NULL;
  desc->handlerDESTRUCTOR = NULL;
  desc->handlerDELETE = NULL;
  desc->handlerUPDATE = NULL;
  desc->handlerOBJMKCONS = NULL;
  desc->handlerSETPRIORITY = NULL;
  desc->handlerXFERCOPY = NULL;
  desc->handlerXFERDELETE = NULL;
  desc->handlerXFERGATHER = NULL;
  desc->handlerXFERSCATTER = NULL;
  desc->handlerXFERGATHERX = NULL;
  desc->handlerXFERSCATTERX = NULL;
  desc->handlerXFERCOPYMANIP = NULL;
}

#define DEFINE_DDD_SETHANDLER(name)                                     \
  void DDD_SetHandler##name(DDD::DDDContext& context, DDD_TYPE type_id, Handler##name funcptr) { \
    TYPE_DESC& desc = context.typeDefs()[type_id];                      \
    assert(desc.mode == DDD_TYPE_DEFINED);                              \
    desc.handler##name = funcptr;                                       \
  }

DEFINE_DDD_SETHANDLER(LDATACONSTRUCTOR)
DEFINE_DDD_SETHANDLER(DESTRUCTOR)
DEFINE_DDD_SETHANDLER(DELETE)
DEFINE_DDD_SETHANDLER(UPDATE)
DEFINE_DDD_SETHANDLER(OBJMKCONS)
DEFINE_DDD_SETHANDLER(SETPRIORITY)
DEFINE_DDD_SETHANDLER(XFERCOPY)
DEFINE_DDD_SETHANDLER(XFERDELETE)
DEFINE_DDD_SETHANDLER(XFERGATHER)
DEFINE_DDD_SETHANDLER(XFERSCATTER)
DEFINE_DDD_SETHANDLER(XFERGATHERX)
DEFINE_DDD_SETHANDLER(XFERSCATTERX)
DEFINE_DDD_SETHANDLER(XFERCOPYMANIP)

#undef DEFINE_DDD_SETHANDLER

/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoTypes                                                 */
/*                                                                          */
/* Purpose:   returns number of declared types                              */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    number of declared types                                      */
/*                                                                          */
/****************************************************************************/

int DDD_InfoTypes(const DDD::DDDContext& context)
{
  return context.typemgrContext().nDescr;
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoHdrOffset                                             */
/*                                                                          */
/* Purpose:   returns offset of DDD_HEADER for a given DDD_TYPE in bytes    */
/*            NOTE: output will be invalid for DDD_TYPEs without header!    */
/*                                                                          */
/* Input:     typeId:  DDD_TYPE for which header offset is returned        */
/*                                                                          */
/* Output:    nheader offset for given type                                 */
/*                                                                          */
/****************************************************************************/


int DDD_InfoHdrOffset(const DDD::DDDContext& context, DDD_TYPE typeId)
{
  const TYPE_DESC& desc = context.typeDefs()[typeId];

  return desc.offsetHeader;
}


/****************************************************************************/
/*                                                                          */
/* Function:  ddd_TypeMgrInit                                               */
/*                                                                          */
/* Purpose:   init TypeMgr module                                           */
/*            (DDD_HEADER is declared and defined as first DDD_TYPE)        */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void ddd_TypeMgrInit(DDD::DDDContext& context)
{
  /* set all theTypeDefs to INVALID, i.e., no DDD_TYPE has been defined */
  for (TYPE_DESC& typeDef : context.typeDefs()) {
    typeDef.mode = DDD_TYPE_INVALID;
    typeDef.currTypeDefCall = 0;
  }


  /* reset declared types */
  context.typemgrContext().nDescr = 0;


  /* init DDD_HEADER as first type, with DDD_TYPE=0 */
  {
    DDD_TYPE hdr_type;

    /* hdr_type will be EL_DDDHDR (=0) per default */
    hdr_type = DDD_TypeDeclare(context, "DDD_HDR");
    DDD_TypeDefine(context, hdr_type,

                   EL_GDATA, offsetof(DDD_HEADER,typ),     sizeof(DDD_HEADER::typ),
                   EL_LDATA, offsetof(DDD_HEADER,prio),    sizeof(DDD_HEADER::prio),
                   EL_GDATA, offsetof(DDD_HEADER,attr),    sizeof(DDD_HEADER::attr),
                   EL_LDATA, offsetof(DDD_HEADER,flags),   sizeof(DDD_HEADER::flags),
                   EL_LDATA, offsetof(DDD_HEADER,myIndex), sizeof(DDD_HEADER::myIndex),
                   EL_GDATA, offsetof(DDD_HEADER,gid),     sizeof(DDD_HEADER::gid),

                   EL_END,   sizeof(DDD_HEADER)  );
  }
}


/****************************************************************************/
/*                                                                          */
/* Function:  ddd_TypeMgrExit                                               */
/*                                                                          */
/* Purpose:   exit and clean up TypeMgr module                              */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void ddd_TypeMgrExit(DDD::DDDContext& context)
{
  /* free memory */
  for (auto& typeDef : context.typeDefs()) {
    typeDef.cmask = nullptr;
  }
}

/****************************************************************************/

END_UGDIM_NAMESPACE
