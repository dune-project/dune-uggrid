// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file algebra.c
 * \ingroup gm
 */

/** \addtogroup gm
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      algebra.c                                                     */
/*                                                                          */
/* Purpose:   management for algebraic structures                           */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 294                                       */
/*            6900 Heidelberg                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* blockvector data structure:                                              */
/*           Christian Wrobel                                               */
/*           Institut fuer Computeranwendungen III                          */
/*           Universitaet Stuttgart	                                        */
/*           Pfaffenwaldring 27                                             */
/*           70569 Stuttgart                                                */
/* email:    ug@ica3.uni-stuttgart.de                                       */
/*                                                                          */
/* History:  1.12.93 begin, ug 3d                                           */
/*           26.10.94 begin combination 2D/3D version                       */
/*           28.09.95 blockvector implemented (Christian Wrobel)            */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


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

#include <config.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ugtypes.h"
#include "architecture.h"
#include "heaps.h"
#include "fifo.h"
#include "ugenv.h"
#include "debug.h"
#include "general.h"

#include <dev/ugdevices.h>

#include "algebra.h"
#include "dlmgr.h"
#include "gm.h"
#include "elements.h"
#include "refine.h"
#include "ugm.h"
#include "evm.h"
#include "misc.h"
#include "dlmgr.h"
#ifdef DYNAMIC_MEMORY_ALLOCMODEL
#include "mgheapmgr.h"
#endif

#ifdef ModelP
#include "parallel.h"
#endif

#include "cw.h"

#include "namespace.h"

USING_UG_NAMESPACE
USING_UGDIM_NAMESPACE
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

/** \brief Resolution for LexAlgDep */
#define ORDERRES                1e-3

/** \brief For GetDomainPart indicating an element is meant rather than an element side */
#define NOSIDE          -1

/** \brief Constants for the direction of domain halfening */
#define BV_VERTICAL     0
#define BV_HORIZONTAL   1

/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

const char* NS_DIM_PREFIX ObjTypeName[MAXVOBJECTS];

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static INT theAlgDepDirID;                      /* env type for Alg Dep dir	*/
static INT theAlgDepVarID;                      /* env type for ALG_DEP vars	*/

static INT theFindCutDirID;                     /* env type for FindCut dir                     */
static INT theFindCutVarID;                     /* env type for FIND_CUT vars                   */

#ifdef __TWODIM__
static MULTIGRID *GBNV_mg;                      /* multigrid							*/
static INT GBNV_MarkKey;                        /* key for Mark/Release					*/
#endif
static INT GBNV_n;                                      /* list items							*/
static INT GBNV_curr;                           /* curr pos								*/
static VECTOR **GBNV_list=NULL;         /* list pointer							*/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/** \brief 2 often used blockvector description formats */
const BV_DESC_FORMAT NS_DIM_PREFIX DH_bvdf =
{ 2, 16,
  { (BVD_ENTRY_TYPE)0x03,        (BVD_ENTRY_TYPE)0x0f,
            (BVD_ENTRY_TYPE)0x03f,       (BVD_ENTRY_TYPE)0x0ff,
            (BVD_ENTRY_TYPE)0x03ff,      (BVD_ENTRY_TYPE)0x0fff,
            (BVD_ENTRY_TYPE)0x03fff,     (BVD_ENTRY_TYPE)0x0ffff,
            (BVD_ENTRY_TYPE)0x03ffff,    (BVD_ENTRY_TYPE)0x0fffff,
            (BVD_ENTRY_TYPE)0x03fffff,   (BVD_ENTRY_TYPE)0x0ffffff,
            (BVD_ENTRY_TYPE)0x03ffffff,  (BVD_ENTRY_TYPE)0x0fffffff,
            (BVD_ENTRY_TYPE)0x3fffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff,  (BVD_ENTRY_TYPE)0xffffffff},
  { (BVD_ENTRY_TYPE)0xfffffffc, (BVD_ENTRY_TYPE)0xfffffff3,
            (BVD_ENTRY_TYPE)0xffffffcf, (BVD_ENTRY_TYPE)0xffffff3f,
            (BVD_ENTRY_TYPE)0xfffffcff, (BVD_ENTRY_TYPE)0xfffff3ff,
            (BVD_ENTRY_TYPE)0xffffcfff, (BVD_ENTRY_TYPE)0xffff3fff,
            (BVD_ENTRY_TYPE)0xfffcffff, (BVD_ENTRY_TYPE)0xfff3ffff,
            (BVD_ENTRY_TYPE)0xffcfffff, (BVD_ENTRY_TYPE)0xff3fffff,
            (BVD_ENTRY_TYPE)0xfcffffff, (BVD_ENTRY_TYPE)0xf3ffffff,
            (BVD_ENTRY_TYPE)0xcfffffff, (BVD_ENTRY_TYPE)0x3fffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
            (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff}};

const BV_DESC_FORMAT NS_DIM_PREFIX one_level_bvdf =
{ 32, 1,
  {     (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

const BV_DESC_FORMAT NS_DIM_PREFIX two_level_bvdf =
{ 16, 2,
  {     (BVD_ENTRY_TYPE)0x0000ffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff},
  { (BVD_ENTRY_TYPE)0xffff0000, (BVD_ENTRY_TYPE)0x0000ffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

const BV_DESC_FORMAT NS_DIM_PREFIX three_level_bvdf =
{ 10, 3,
  {     (BVD_ENTRY_TYPE)0x000003ff, (BVD_ENTRY_TYPE)0x000fffff,
                (BVD_ENTRY_TYPE)0x3fffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff,
                (BVD_ENTRY_TYPE)0xffffffff, (BVD_ENTRY_TYPE)0xffffffff},
  { (BVD_ENTRY_TYPE)0xfffffc00, (BVD_ENTRY_TYPE)0xfff003ff,
            (BVD_ENTRY_TYPE)0xc00fffff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

/* for LexOrderVectorsInGrid */
static DOUBLE InvMeshSize;

REP_ERR_FILE

/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined           */
/*                                                                          */
/****************************************************************************/

static INT CreateBVPlane( BLOCKVECTOR **bv_plane, const BV_DESC *bvd_plane, const BV_DESC_FORMAT *bvdf, VECTOR **v, INT stripes, INT vectors_per_stripe, GRID *grid );
static INT BlockHalfening( GRID *grid, BLOCKVECTOR *bv, INT left, INT bottom, INT width, INT height, INT side, INT orientation, INT leaf_size );

/****************************************************************************/
/** \brief Initialize a blockvector description format
 *
 * @param  bvdf - blockvector description format to be initialized
 * @param  max_blocks - maximum number of blocks provided for each blockvector level

   All routines working with a 'blockvector description' need a explanation
   of the internal encoding used in the description. This routine generates
   this necessary explanation. Since the encoding depends on the number
   of blockvectors you want to use on each blockvector level, you have to
   specify this quantity. The background is, that a sequence of blockvector
   numbers is encoded in a bitstring and the number of bits used for each
   level has to be fixed before the first use. In order to get fast access
   to the stored information, a couple of bitmasks are necessary and are
   precalculated and stored in the 'blockvector description format' by this
   function. The memory to store the information is provided by the caller.

   HINTS:
   Since the hole amount of bits available for encoding a sequence of
   blockvector numbers is constant, you may use only few blocks on each
   level and for that have many levels available; or if you have many
   blocks on each level you can work only with few levels at all. Thus you
   have to find a trade off of your requirements.

   Since the relevant information of the 'max_blocks' parameter is only
   the number of bits, necessary to encode this number, all numbers up to
   the next power of 2 will have the same result.

   PREDEFINED FORMATS:
   There are predefined 'blockvector description formats', which are
   often used. This formats you can use without initialization.
   <ul>
     <li>  DH_bvdf  - useful for domain halfening methods (up to 4 blocks per level) </li>
     <li>  one_level_bvdf - max. number of blocks in a level but only 1 level </li>
     <li>  two_level_bvdf - bvdf for 2 blocklevels </li>
     <li>  three_level_bvdf - bvdf for 3 blocklevels </li>
   </ul>

   EXAMPLE:
   To create a format for managing a octree-like structure (that is
   subdividing a 3D cube into 8 subcubes) you can use the following
   created format:
   \verbatim
   BV_DESC_FORMAT my_bvdf;

   InitBVDF( &my_bvdf, 8 );
   \endverbatim

 * @return <ul>
 *   <li>GM_OK if ok </li>
 *   <li>GM_OUT_OF_RANGE if 'max_blocks' < 2 or if 'max_blocks' greater than possible </li>
 * </ul>
 * \sa
 *  BLOCKVECTOR, BV_DESC, BV_DESC_FORMAT
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitBVDF( BV_DESC_FORMAT *bvdf, BLOCKNUMBER max_blocks )
{
  INT bits;
  INT level;
  BVD_ENTRY_TYPE mask;

  /* calculate number of bits to represent the numbers 0..max_blocks-1,
     i.e. the next integer greater or equal to the logarithmus dualis
     of max_blocks-1 */
  if ( max_blocks < (BLOCKNUMBER)2 )
    return GM_OUT_OF_RANGE;
  max_blocks--;
  bits = 0;
  while( max_blocks > (BLOCKNUMBER)0 )
  {
    max_blocks >>= 1;
    bits++;
  }


  bvdf->bits = bits;
  bvdf->max_level = BVD_MAX_ENTRIES / bits;
  if ( bvdf->max_level == 0 )
    return GM_OUT_OF_RANGE;

  /* calculate the necessary bitmasks */
  mask = ((BVD_ENTRY_TYPE)1 << bits) - (BVD_ENTRY_TYPE)1;
  bvdf->level_mask[0] = mask;
  bvdf->neg_digit_mask[0] = ~mask;
  for ( level = 1; level < BVD_MAX_ENTRIES; level ++ )
  {
    mask <<= bits;
    bvdf->level_mask[level] = bvdf->level_mask[level-1] | mask;
    bvdf->neg_digit_mask[level] = ~mask;
  }

  return GM_OK;
}

/****************************************************************************/
/** \brief Adds a further blockvector number to the existing sequence
 *
 * @param  bvd - blockvector description to which a further blocknumber is added
 * @param  bnr - blocknumber to be added
 * @param  bvdf - blockvector description format to work on the bvd

   In a blockvector description the position of a blockvector in a blockvector
   hierarchy is stored as a sequence of blocknumbers. With this function you
   can add a further number to the existing sequence, i.e. to describe
   a subblock of the smallest already existing block in the bvd.

   This function is embedded in the macro BVD_PUSH_ENTRY and provides
   together with the macro BVD_DISCARD_LAST_ENTRY a stack-mechanism
   for storing a sequence of blockvector numbers.

 * @return <ul>
 *   <li>GM_OK if ok </li>
 *   <li>GM_OUT_OF_RANGE if the maximum number of blockvector levels is exeeded </li>
   </ul>
 * \sa
   BLOCKVECTOR, BV_DESC

 */
/****************************************************************************/

INT NS_DIM_PREFIX PushEntry( BV_DESC *bvd, BLOCKNUMBER bnr, const BV_DESC_FORMAT *bvdf )
{
  /* exist, if there is no space for a further number */
  if ( bvd->current >= bvdf->max_level )
    return GM_OUT_OF_RANGE;

  /* store the number */
  BVD_SET_ENTRY( bvd, bvd->current, bnr, bvdf );
  bvd->current++;

  return GM_OK;
}

/****************************************************************************/
/** \brief Find the described blockvector in the grid
 *
 * @param  grid - the grid, which in the blockvector is to be searched
 * @param  bvd - blockvector description specifying the blockvector
 * @param  bvdf - blockvector description format to read the bvd

   Returns the blockvector specified by bvd.

 * @return <ul>
 *  <li> pointer to the specified blockvector </li>
 *   <li>NULL if the specified blockvector does not exist </li>
   </ul>
 * \sa
   BLOCKVECTOR, BV_DESC, BVD_PUSH_ENTRY

 */
/****************************************************************************/

BLOCKVECTOR *NS_DIM_PREFIX FindBV( const GRID *grid, const BV_DESC *bvd, const BV_DESC_FORMAT *bvdf )
{
  BLOCKVECTOR *bv;
  BLOCKNUMBER nr;
  BV_DESC copy_bvd = *bvd;

  /* initialize the search with the topmost blockvector in the grid */
  bv = GFIRSTBV( grid );
  BVD_INIT_SEQ_READ( &copy_bvd );
  nr = BVD_READ_NEXT_ENTRY( &copy_bvd, bvdf );

  /* search trough all blockvector levels defined in bvd */
  while ( true )
  {
    while ( BVNUMBER(bv) != nr )             /* search block with number nr in block list*/
    {
      bv = BVSUCC( bv );
      if( bv == NULL )
        return bv;                              /* desired number not found, return NULL */
    }

    /* block with number nr found, prepare search on next block level */
    nr = BVD_READ_NEXT_ENTRY( &copy_bvd, bvdf  );
    if ( nr != NO_BLOCKVECTOR )
      /* there is a further blockvector level specified */
      if ( BV_IS_LEAF_BV( bv ) )
        return NULL;                            /* more block level specified than present */
      else
        bv = BVDOWNBV( bv );                            /* go to next blockvector level */
    else
      /* no more block level specified, current block is the result */
      return bv;
  }
}

/****************************************************************************/
/** \brief Compute part information of geometrical object
 *
 * @param  s2p - table translating subdomain to domain part
 * @param  obj - geometric object (node, element or edge)
 * @param  side - if element side is meant for obj==element side has to be >=0, negative else

   Compute the part info for a geometrical object including element sides.

 * @return <ul>
 *   <li>   part if ok </li>
 *   <li>   -n else </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetDomainPart (const INT s2p[], const GEOM_OBJECT *obj, INT side)
{
  NODE *nd,*n0,*n1;
  EDGE *ed;
  ELEMENT *elem;
  VERTEX *v0,*v1;
  BNDS *bs;
  INT part=-1,subdom,move,left,right;

  switch (OBJT(obj))
  {
  case NDOBJ :
    nd = (NODE*)obj;
    v0 = MYVERTEX(nd);
    if (OBJT(v0)==IVOBJ) {
      subdom = NSUBDOM(nd);
      ASSERT(subdom>0);
      part = s2p[subdom];
    }
    else
    {
      /* get part info from domain module */
      if (BNDP_BndPDesc(V_BNDP(v0),&move,&part))
        REP_ERR_RETURN(-2);
      ASSERT(NSUBDOM(nd) == 0);
    }
    break;

  case IEOBJ :
  case BEOBJ :
    elem = (ELEMENT*)obj;
    if (side==NOSIDE)
    {
      /* get info for element */

      subdom = SUBDOMAIN(elem);
      ASSERT(subdom>0);
      part = s2p[subdom];
    }
    else
    {
      /* get info for element side */

      ASSERT(side<SIDES_OF_ELEM(elem));
      ASSERT(side>=0);

      if ((OBJT(elem)==BEOBJ) && ((bs = ELEM_BNDS(elem,side))!=NULL))
      {
        /* this is a boundary side: ask domain module */
        if (BNDS_BndSDesc(ELEM_BNDS(elem,side),&left,&right,&part))
          REP_ERR_RETURN(-3);
      }
      else
      {
        /* there still is the possibility that the side is a boundary side
           because VECTORs are created while CreateElement but boundary sides later
           by CreateSonElementSide.
           The vector eventually will be reallocated by ReinspectSonSideVector later */
        subdom = SUBDOMAIN(elem);
        ASSERT(subdom>0);
        part = s2p[subdom];
      }
    }
    break;

  case EDOBJ :
    ed = (EDGE*)obj;
    n0 = NBNODE(LINK0(ed));
    n1 = NBNODE(LINK1(ed));
    v0 = MYVERTEX(n0);
    v1 = MYVERTEX(n1);
    if ((OBJT(v0)==BVOBJ) && (OBJT(v1)==BVOBJ))
      if (BNDP_BndEDesc(V_BNDP(v0),V_BNDP(v1),&part) == 0)
        return(part);
    subdom = EDSUBDOM(ed);
    if (subdom > 0)
      return(s2p[subdom]);
    subdom = NSUBDOM(n0);
    if (subdom > 0)
      return(s2p[subdom]);
    subdom = NSUBDOM(n1);
    if (subdom > 0)
      return(s2p[subdom]);
    REP_ERR_RETURN(-4);

  default : REP_ERR_RETURN (-5);
  }
  return (part);
}

#ifdef ModelP
INT NS_DIM_PREFIX GetVectorSize (GRID *theGrid, INT VectorObjType, GEOM_OBJECT *object)
{
  MULTIGRID *mg;
  INT part,vtype;

  mg = MYMG(theGrid);
  part = GetDomainPart(BVPD_S2P_PTR(MG_BVPD(mg)),object,NOSIDE);
  if (part < 0)
    REP_ERR_RETURN(-1);
  vtype = FMT_PO2T(MGFORMAT(mg),part,VectorObjType);

  return(FMT_S_VEC_TP(MGFORMAT(mg),vtype));
}
#endif

/****************************************************************************/
/** \brief  Return pointer to a new vector structure
 *
 * @param  theGrid - grid where vector should be inserted
 * @param  DomPart - part of the domain where vector is created
 * @param  ObjType - one of the types defined in gm
 * @param  object  - associate vector with this object
 * @param  vHandle - handle of new vector, i.e. a pointer to a pointer where
                                a pointer to the new vector is placed.

   This function returns a pointer to a new vector structure.
   The vector type is determined by DomPart and ObjType
   First the free list is checked for a free entry, if none
   is available, a new structure is allocated from the heap.

 * @return <ul>
 *   <li>     0 if ok  </li>
 *   <li>     1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

static INT CreateVectorInPart (GRID *theGrid, INT DomPart, INT VectorObjType,
                               GEOM_OBJECT *object, VECTOR **vHandle)
{
  MULTIGRID *theMG;
  VECTOR *pv;
  INT ds, Size, vtype;

  *vHandle = NULL;

  theMG = MYMG(theGrid);
  vtype = FMT_PO2T(MGFORMAT(theMG),DomPart,VectorObjType);
  ds = FMT_S_VEC_TP(MGFORMAT(theMG),vtype);
  if (ds == 0)
    return (0);                         /* HRR: this is ok now, no XXXXVEC in part of the domain */

  Size = sizeof(VECTOR)-sizeof(DOUBLE)+ds;
  pv = (VECTOR *)GetMemoryForObject(theMG,Size,VEOBJ);
  if (pv==NULL)
    REP_ERR_RETURN(1);

  /* initialize data */
  SETOBJT(pv,VEOBJ);
  SETVTYPE(pv,vtype);
  SETVPART(pv,DomPart);
  ds = VPART(pv);
  if (ds!=DomPart)
    return (1);
  SETVDATATYPE(pv,BITWISE_TYPE(vtype));
  SETVOTYPE(pv,VectorObjType);
  SETVCLASS(pv,3);
  SETVNCLASS(pv,0);
  SETVBUILDCON(pv,1);
  SETVNEW(pv,1);
  /* SETPRIO(pv,PrioMaster); */

#ifndef ModelP
  // Dune uses the id field for face indices in sequential grids
  pv->id = (theGrid->mg->vectorIdCounter)++;
#endif

    #ifdef __BLOCK_VECTOR_DESC__
  BVD_INIT( &VBVD( pv ) );
    #endif

        #ifdef ModelP
  DDD_AttrSet(PARHDR(pv),GRID_ATTR(theGrid));
        #endif

  VOBJECT(pv) = object;
  VINDEX(pv) = (long)NVEC(theGrid);
  SUCCVC(pv) = FIRSTVECTOR(theGrid);
  VECSKIP(pv) = 0;
  VSTART(pv) = NULL;

  GRID_LINK_VECTOR(theGrid,pv,PrioMaster);

  *vHandle = pv;

  PRINTDEBUG(gm,1,("%s-vector created (%d): p=%d, t=%d\n",ObjTypeName[VOTYPE(pv)],ID(VOBJECT(pv)),VPART(pv),VTYPE(pv)));

  return (0);
}

/****************************************************************************/
/** \brief Return pointer to a new vector structure
 *
 * @param  theGrid - grid where vector should be inserted
 * @param  VectorObjType - one of the types defined in gm
 * @param  object  - associate vector with this object
 * @param  vHandle - handle of new vector, i.e. a pointer to a pointer where
                                a pointer to the new vector is placed.

   This function returns a pointer to a new vector structure. First the free list
   is checked for a free entry, if none is available, a new structure is allocated
   from the heap.

 * @return <ul>
 *   <li>     0 if ok  </li>
 *   <li>     1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateVector (GRID *theGrid, INT VectorObjType, GEOM_OBJECT *object, VECTOR **vHandle)
{
  MULTIGRID *mg;
  INT part;

  *vHandle = NULL;
  mg = MYMG(theGrid);

  part = GetDomainPart(BVPD_S2P_PTR(MG_BVPD(mg)),object,NOSIDE);

  if (part < 0)
    REP_ERR_RETURN(1);
  if (CreateVectorInPart(theGrid,part,VectorObjType,object,vHandle)) {
    REP_ERR_RETURN(1);
  }
  else
    return (0);
}

INT NS_DIM_PREFIX CreateSideVector (GRID *theGrid, INT side, GEOM_OBJECT *object, VECTOR **vHandle)
{
  MULTIGRID *mg;
  INT part;

  *vHandle = NULL;
  mg = MYMG(theGrid);
  part = GetDomainPart(BVPD_S2P_PTR(MG_BVPD(mg)),object,side);
  if (part<0)
    REP_ERR_RETURN(1);

  if (CreateVectorInPart(theGrid,part,SIDEVEC,object,vHandle))
    REP_ERR_RETURN(1);

  SETVECTORSIDE(*vHandle,side);
  SETVCOUNT(*vHandle,1);

  return (0);
}

/****************************************************************************/
/** \brief Return pointer to a new blockvector structure
 *
 * @param  theGrid - grid where blockvector should be inserted
 * @param  BVHandle - handle of new blockvector, i.e. a pointer to a pointer where
   a pointer to the new blockvector is placed. Is NULL if there was no more
   memory.

   This function returns pointer to a new blockvector structure. First
   the free list is checked for a free entry, if none is available,
   a new structure is allocated from the heap.

 * @return <ul>
 *   <li>     GM_OK if ok  </li>
 *   <li>     GM_OUT_OF_MEM if there is not not enough memory available </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateBlockvector( GRID *theGrid, BLOCKVECTOR **BVHandle )
{
  MULTIGRID *theMG;
  BLOCKVECTOR *bv;

  theMG = MYMG(theGrid);

  /* try to find an entry in the free list */
  *BVHandle = NULL;
  bv = (BLOCKVECTOR*)GetMemoryForObject(theMG,sizeof(BLOCKVECTOR),BLOCKVOBJ);
  if (bv == NULL)
    return GM_OUT_OF_MEM;

  SETOBJT(bv,BLOCKVOBJ);
  *BVHandle = bv;

  return GM_OK;
}

static INT InsertBlockvector_l0 (GRID *theGrid, BLOCKVECTOR *insertBV, BLOCKVECTOR *theBV, INT after, INT makeVC)
{
  if (GFIRSTBV(theGrid)==NULL)
  {
    GFIRSTBV(theGrid)=GLASTBV(theGrid)=insertBV;
    BVPRED(insertBV)=BVSUCC(insertBV)=NULL;

    if (makeVC==YES)
    {
      PREDVC(BVFIRSTVECTOR(insertBV)) = NULL;
      SUCCVC(BVLASTVECTOR(insertBV)) = NULL;
    }
  }
  else if (after)
  {
    if (theBV==NULL)
    {
      BVSUCC(insertBV) = GFIRSTBV(theGrid);
      BVPRED(GFIRSTBV(theGrid)) = insertBV;
      GFIRSTBV(theGrid) = insertBV;
      BVPRED(insertBV) = NULL;

      if (makeVC==YES)
      {
        PREDVC(BVFIRSTVECTOR(insertBV)) = NULL;
        SUCCVC(BVLASTVECTOR(insertBV)) = BVFIRSTVECTOR(BVSUCC(insertBV));
        PREDVC(BVFIRSTVECTOR(BVSUCC(insertBV))) = BVLASTVECTOR(insertBV);
        SFIRSTVECTOR(theGrid) = BVFIRSTVECTOR(insertBV);
      }
    }
    else
    {
      BVPRED(insertBV) = theBV;
      BVSUCC(insertBV) = BVSUCC(theBV);
      if (theBV==GLASTBV(theGrid)) GLASTBV(theGrid)=insertBV;
      else BVPRED(BVSUCC(theBV))=insertBV;
      BVSUCC(theBV) = insertBV;

      if (makeVC==YES)
      {
        SUCCVC(BVLASTVECTOR(insertBV)) = SUCCVC(BVLASTVECTOR(theBV));
        if (SUCCVC(BVLASTVECTOR(insertBV))!=NULL) PREDVC(SUCCVC(BVLASTVECTOR(insertBV))) = BVLASTVECTOR(insertBV);
        else LASTVECTOR(theGrid) = BVLASTVECTOR(insertBV);
        SUCCVC(BVLASTVECTOR(theBV)) = BVFIRSTVECTOR(insertBV);
        PREDVC(BVFIRSTVECTOR(insertBV)) = BVLASTVECTOR(theBV);
      }
    }
  }
  else
  {
    if (theBV==NULL)
      return (InsertBlockvector_l0(theGrid,insertBV,GLASTBV(theGrid),1,makeVC));
    else
      return (InsertBlockvector_l0(theGrid,insertBV,BVPRED(theBV),1,makeVC));
  }

  return (GM_OK);
}

INT NS_DIM_PREFIX CreateBlockvector_l0 (GRID *theGrid, BLOCKVECTOR **BVHandle, BLOCKVECTOR *insertBV, INT after)
{
  BLOCKVECTOR *theBV;

  if (CreateBlockvector (theGrid, &theBV)!=GM_OK) return (GM_OUT_OF_MEM);
  if (InsertBlockvector_l0(theGrid,theBV,insertBV,after,NO)) return (GM_OUT_OF_MEM);

  *BVHandle = theBV;
  return (GM_OK);
}

/****************************************************************************/
/** \brief Return pointer to a new connection structure
 *
 * @param  theGrid - grid where matrix should be inserted
 * @param  from - source vector
 * @param  to - destination vector

   This function allocates a new CONNECTION and inserts the two
   MATRIX structures in the lists of from and to vector.
   Since the operation is symmetric, the order of from and to
   is not important.

 * @return <ul>
 *   <li>   NULL if error occured. </li>
 *   <li>   else a pointer to the new CONNECTION is returned. </li>
 * </ul>
 */
/****************************************************************************/

CONNECTION * NS_DIM_PREFIX CreateConnection (GRID *theGrid, VECTOR *from, VECTOR *to)
{
  MULTIGRID *theMG;
  CONNECTION *pc;
  MATRIX *pm;
  INT RootType, DestType, MType, ds, Diag, Size;

  /* set Diag, RootType and DestType	*/
  Diag = ((from == to) ? 1 : 0);
  RootType = VTYPE(from);
  DestType = VTYPE(to);
  if (Diag)
    MType=DIAGMATRIXTYPE(RootType);
  else
    MType=MATRIXTYPE(RootType,DestType);

  /* check expected size */
  theMG = MYMG(theGrid);
  ds = FMT_S_MAT_TP(MGFORMAT(theMG),MType);
  if (ds == 0)
    return (NULL);
  Size = sizeof(MATRIX)-sizeof(DOUBLE)+ds;
  if (MSIZEMAX<Size) return (NULL);

  /* is there already the desired connection ? */
  pc = GetConnection(from,to);
  if (pc != NULL)
  {
    SETCEXTRA(pc,0);
    return (pc);
  }

  if (Diag)
    pc = (CONNECTION*)GetMemoryForObject(theMG,Size,MAOBJ);
  else
    pc = (CONNECTION*)GetMemoryForObject(theMG,2*Size,COOBJ);
  if (pc==NULL) return (NULL);

  /* initialize data */
  pm = CMATRIX0(pc);
  SETOBJT(pm,MAOBJ);
  SETMROOTTYPE(pm,RootType);
  SETMDESTTYPE(pm,DestType);
  SETMDIAG(pm,Diag);
  SETMOFFSET(pm,0);
  SETMSIZE(pm,Size);
  SETMNEW(pm,1);
  SETCEXTRA(pc,0);
  MDEST(pm) = to;
  if (!Diag)
  {
    pm = CMATRIX1(pc);
    CTRL(pm) = 0;
    SETOBJT(pm,MAOBJ);
    SETMROOTTYPE(pm,DestType);
    SETMDESTTYPE(pm,RootType);
    SETMDIAG(pm,Diag);
    SETMOFFSET(pm,1);
    SETMSIZE(pm,Size);
    SETMNEW(pm,1);
    MDEST(pm) = from;
  }

  /* set sizes */
  if (!Diag)
  {
    Size = (char*)pm - (char*)pc;
    SETMSIZE(pc,Size);
    SETMSIZE(pm,Size);
  }

  /* put in matrix list */
  if (Diag)
  {
    /* insert at first place in the list (only one matrix) */
    MNEXT(CMATRIX0(pc)) = VSTART(from);
    VSTART(from) = CMATRIX0(pc);
  }
  else
  {
    /* insert at second place in the list (both matrices) */
    pm = VSTART(from);
    if (pm == NULL)
    {
      MNEXT(CMATRIX0(pc)) = NULL;
      VSTART(from) = CMATRIX0(pc);
    }
    else
    {
      MNEXT(CMATRIX0(pc)) = MNEXT(pm);
      MNEXT(pm) = CMATRIX0(pc);
    }

    pm = VSTART(to);
    if (pm == NULL)
    {
      MNEXT(CMATRIX1(pc)) = NULL;
      VSTART(to) = CMATRIX1(pc);
    }
    else
    {
      MNEXT(CMATRIX1(pc)) = MNEXT(pm);
      MNEXT(pm) = CMATRIX1(pc);
    }
  }

  /* counters */
  theGrid->nCon++;

  return(pc);
}

/****************************************************************************/
/** \brief Return pointer to a new matrix structure with extra flag set.
 *
 * @param  theGrid - grid level where connection will be inserted.
 * @param  from,to - Pointers to vectors where connection is inserted.

   This function returns a pointer to a new CONNECTION
   structure with extra flag set. This e.g. for a direct solver
   or ILU with fill in. The new connections can be distinguished
   from the connections necessary for the stiffness matrix.

 * @return <ul>
 *   <li>   NULL if error occured. </li>
 *   <li>   else pointer to new CONNECTION </li>
   </ul>
 */
/****************************************************************************/

CONNECTION      *NS_DIM_PREFIX CreateExtraConnection    (GRID *theGrid, VECTOR *from, VECTOR *to)
{
  CONNECTION *pc;

  pc = CreateConnection(theGrid,from,to);
  if (pc==NULL) return(NULL);
  SETCEXTRA(pc,1);
  return(pc);
}

INT NS_DIM_PREFIX CreateElementList (GRID *theGrid, NODE *theNode, ELEMENT *theElement)
{
  ELEMENTLIST *pel;

  for (pel=NODE_ELEMENT_LIST(theNode); pel!=NULL; pel=NEXT(pel))
    if(pel->el==theElement)
      return(0);

  pel = (ELEMENTLIST *)GetMemoryForObject(theGrid->mg,
                                          sizeof(ELEMENTLIST),MAOBJ);
  if (pel == NULL)
    return(1);

  pel->next = NODE_ELEMENT_LIST(theNode);
  pel->el = theElement;
  NDATA(theNode) = (void *) pel;

  return(0);
}

/****************************************************************************/
/** \brief Remove vector from the data structure
 *
 * @param  theGrid - grid level where theVector is in.
 * @param  theVector - VECTOR to be disposed.

   This function removes vector from the data structure and places
   it in the free list.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeVector (GRID *theGrid, VECTOR *theVector)
{
  MATRIX *theMatrix, *next;
  INT Size;

#ifdef __PERIODIC_BOUNDARY__
  SETPVCOUNT(theVector,PVCOUNT(theVector)-1);
  if (((INT)PVCOUNT(theVector)) > 0)
  {
    PRINTDEBUG(gm,1,(PFMT "DisposeVector: v=" VINDEX_FMTX
                     " NOT disposed count=%d\n",
                     me,VINDEX_PRTX(theVector),PVCOUNT(theVector)));
    return (0);
  }
  PRINTDEBUG(gm,1,(PFMT "DisposeVector: v=" VINDEX_FMTX
                   " disposed count=%d\n",
                   me,VINDEX_PRTX(theVector),PVCOUNT(theVector)));
#endif

  if (theVector == NULL)
    return(0);

  HEAPFAULT(theVector);

  /* remove all connections concerning the vector */
  for (theMatrix=VSTART(theVector); theMatrix!=NULL; theMatrix=next)
  {
    next = MNEXT(theMatrix);
    if (DisposeConnection(theGrid,MMYCON(theMatrix)))
      RETURN (1);
  }

  /* now remove vector from vector list */
  GRID_UNLINK_VECTOR(theGrid,theVector);

  /* reset count flags */
  SETVCOUNT(theVector,0);


  /* delete the vector itself */
  Size = sizeof(VECTOR)-sizeof(DOUBLE)
         + FMT_S_VEC_TP(MGFORMAT(MYMG(theGrid)),VTYPE(theVector));
  if (PutFreeObject(theGrid->mg,theVector,Size,VEOBJ))
    RETURN(1);

  return(0);
}

/****************************************************************************/
/* \brief Change vector allocated with wrong part

 * @param  g - grid level where the element is in.
 * @param  elem - element of side vector
 * @param  side - element side
 * @param  vHandle - handle to side vector (inialized with old, may be changed)

   This changes a side vector which was allocated with the wrong part (maybe unknown for side vectors
   when creating an element).

 * @return <ul>
 *   <li>   GM_OK if ok
 *   <li>   GM_ERROR if error occured.
 */
/****************************************************************************/

INT NS_DIM_PREFIX ReinspectSonSideVector (GRID *g, ELEMENT *elem, INT side, VECTOR **vHandle)
{
  MULTIGRID *mg;
  VECTOR *vold,*vnew;
  FORMAT *fmt;
  INT partnew,partold,vtnew,vtold,dsnew,dsold;

  mg  = MYMG(g);
  fmt = MGFORMAT(mg);

  vold = *vHandle;

  /* check whether part has actually changed */
  partold = (vold!=NULL) ? VPART(vold) : BVPD_S2P(MG_BVPD(mg),SUBDOMAIN(elem));
  partnew = GetDomainPart(BVPD_S2P_PTR(MG_BVPD(mg)),(GEOM_OBJECT*)elem,side);
  if (partnew<0)
    REP_ERR_RETURN(GM_ERROR);
  if (partnew==partold)
    return (GM_OK);

  /* check whether vtype has actually changed */
  vtold = (vold!=NULL) ? VTYPE(vold) : FMT_PO2T(fmt,partold,SIDEVEC);
  vtnew = FMT_PO2T(fmt,partnew,SIDEVEC);
  if (vtnew==vtold)
  {
    if (vold!=NULL)
    {
      /* just change part */
      SETVPART(vold,partnew);
    }
    PRINTDEBUG(gm,1,("SIDEVEC (%d,%d): part\n",ID(elem),side));
    return (GM_OK);
  }

  /* check whether size has actually changed */
  dsold = FMT_S_VEC_TP(fmt,vtold);
  dsnew = FMT_S_VEC_TP(fmt,vtnew);
  if (dsold==dsnew)
  {
    if (vold!=NULL)
    {
      /* just change part and type */
      SETVTYPE(vold,vtnew);
      SETVPART(vold,partnew);

      DisposeConnectionFromVector(g,vold);

      SETVBUILDCON(vold,1);
    }
    PRINTDEBUG(gm,1,("SIDEVEC (%d,%d): part and type\n",ID(elem),side));
    return (GM_OK);
  }

  PRINTDEBUG(gm,1,("SIDEVEC (%d,%d): part, type and size\n",ID(elem),side));

  /* create new vector */
  if (CreateVectorInPart(g,partnew,SIDEVEC,(GEOM_OBJECT*)elem,&vnew))
    REP_ERR_RETURN(GM_ERROR);

  /* now we can dispose the old vector */
  if (DisposeVector(g,vold))
    REP_ERR_RETURN(GM_ERROR);

  *vHandle = vnew;

  return (GM_OK);
}

/****************************************************************************/
/** \brief Dispose blockvector
 *
 * @param  theGrid - grid level where bv is in.
 * @param  bv - BLOCKVECTOR to be disposed.

   This function places the blockvector in the free list. The data structure
   must be kept consistent by the caller! If bv is the NULL-vector nothing
   will happen.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeBlockvector( GRID *theGrid, BLOCKVECTOR *bv )
{
  if ( bv == NULL ) return 0;
  return PutFreeObject( MYMG(theGrid), bv,sizeof(BLOCKVECTOR),BLOCKVOBJ);
}

/****************************************************************************/
/** \brief Frees blockvector-list and all its sons
 * @param  grid - the grid from which the blockvectors are to be removed
 * @param  bv - start of the blockvector-list to be removed

   Frees double linked blockvector-list, starting with bv, and all
   its sons recursively, i.e. all the
   subblockvectors. The vectors belonging to the blockvectors
   are not influenced.

 * \sa
   BLOCKVECTOR, FreeAllBV

 */
/****************************************************************************/

static void FreeBVList (GRID *grid, BLOCKVECTOR *bv)
{
  BLOCKVECTOR *bv_h;

  if ( bv == NULL )
    return;

  /* free all blocks in the current block list */
  for ( ; bv != NULL; )
  {
    /* free rekursively all higher block levels */
    if ( !BV_IS_LEAF_BV( bv ) )
      FreeBVList( grid, BVDOWNBV( bv ) );

    /* free the current block and go to the next */
    bv_h = BVSUCC( bv );
    DisposeBlockvector( grid, bv );
    bv = bv_h;
  }
}

/****************************************************************************/
/** \brief Frees all allocated blockvectors in the grid
 *
 * @param  grid - the grid from which all blockvectors are to be removed

   Frees all allocated blockvectors in the grid and resets GFIRSTBV(grid)
   and GLASTBV(grid) to NULL and gives the memory back to the memory
   of the MULTIGRID.

 * \sa
   BLOCKVECTOR, FreeBVList

 */
/****************************************************************************/

void NS_DIM_PREFIX FreeAllBV (GRID *grid)
{
  FreeBVList( grid, GFIRSTBV( grid ) );
  GFIRSTBV( grid ) = NULL;
  GLASTBV( grid ) = NULL;
}

/****************************************************************************/
/** \brief Remove connection from the data structure
 *
 * @param  theGrid - the grid to remove from
 * @param  theConnection - connection to dispose

   This function removes a connection from the data structure. The connection
   is removed from the list of the two vectors and is placed in the
   free list.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeConnection (GRID *theGrid, CONNECTION *theConnection)
{
  VECTOR *from, *to;
  MATRIX *Matrix, *ReverseMatrix, *SearchMatrix;

  HEAPFAULT(theConnection);

  /* remove matrix(s) from their list(s) */
  Matrix = CMATRIX0(theConnection);
  to = MDEST(Matrix);
  if (MDIAG(Matrix))
  {
    from = to;
    VSTART(to) = MNEXT(Matrix);
  }
  else
  {
    ReverseMatrix = CMATRIX1(theConnection);
    from = MDEST(ReverseMatrix);
    if (VSTART(from) == Matrix)
      VSTART(from) = MNEXT(Matrix);
    else
      for (SearchMatrix=VSTART(from); SearchMatrix!=NULL; SearchMatrix=MNEXT(SearchMatrix))
        if (MNEXT(SearchMatrix) == Matrix)
          MNEXT(SearchMatrix) = MNEXT(Matrix);
    if (VSTART(to) == ReverseMatrix)
      VSTART(to) = MNEXT(ReverseMatrix);
    else
      for (SearchMatrix=VSTART(to); SearchMatrix!=NULL; SearchMatrix=MNEXT(SearchMatrix))
        if (MNEXT(SearchMatrix) == ReverseMatrix)
          MNEXT(SearchMatrix) = MNEXT(ReverseMatrix);
  }

  /* free connection object */
  if (MDIAG(Matrix))
    PutFreeObject(MYMG(theGrid),Matrix,UG_MSIZE(Matrix),MAOBJ);
  else
    PutFreeObject(MYMG(theGrid),Matrix,2*UG_MSIZE(Matrix),COOBJ);

  theGrid->nCon--;

  /* return ok */
  return(0);
}

/****************************************************************************/
/** \brief Dispose one of two vectors associated to a face
 *
 * @param  theGrid - pointer to grid
 * @param  Elem0,side0 - first element and side
 * @param  Elem1,side1 - second element and side

   After grid refinement it may happen that two side vector are associated to a face
   in the grid.  The elements on both sides of the face each have their own side
   vector.  This is an inconsistent state -- there should be only one vector per face.
   This routine deletes one of the two side vectors and makes the data structure
   consistent again.  This is easier than only creating one unique side vector
   to begin with.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

#ifdef __THREEDIM__
INT NS_DIM_PREFIX DisposeDoubledSideVector (GRID *theGrid, ELEMENT *Elem0, INT Side0, ELEMENT *Elem1, INT Side1)
{
  VECTOR *Vector0, *Vector1;

  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    assert(NBELEM(Elem0,Side0)==Elem1 && NBELEM(Elem1,Side1)==Elem0);
    Vector0 = SVECTOR(Elem0,Side0);
    Vector1 = SVECTOR(Elem1,Side1);
    if (Vector0 == Vector1)
      return (0);
    if ((Vector0==NULL) || (Vector1==NULL))
      /* this is the case at boundaries between different parts
         the part not using vectors in SIDEs will not need a pointer
         to the side vector */
      return (0);
    assert(VCOUNT(Vector0)==1 && VCOUNT(Vector1)==1);
    assert(VSTART(Vector0)==NULL || VSTART(Vector1)==NULL);
    if (VSTART(Vector0)==NULL)
    {
      SET_SVECTOR(Elem0,Side0,Vector1);
      SETVCOUNT(Vector1,2);
      if (DisposeVector (theGrid,Vector0))
        RETURN (1);
    }
    else
    {
      SET_SVECTOR(Elem1,Side1,Vector0);
      SETVCOUNT(Vector0,2);
      if (DisposeVector (theGrid,Vector1))
        RETURN (1);
    }
    return (0);

  }

  RETURN (1);
}
#endif

/****************************************************************************/
/** \brief Remove all connections associated with a vector
 *
 * @param  theGrid - grid level where vector belongs to
 * @param  theVector - vector where connections are disposed from

   This function removes all connections from a vector.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeConnectionFromVector (GRID *theGrid, VECTOR *theVector)
{
  while(VSTART(theVector) != NULL)
    if (DisposeConnection (theGrid,MMYCON(VSTART(theVector))))
      return (1);

  return (0);
}


/****************************************************************************/
/** \brief Removes all connections from all vectors associated with an element
 *
 * @param  theGrid - grid level where element is on
 * @param  theElement - element from which to dispose connections

   This function removes all connections from all vectors
   associated with an element.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeConnectionFromElement (GRID *theGrid, ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
  {
    GetVectorsOfElement(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
    {
      if (DisposeConnectionFromVector(theGrid,vList[i])) RETURN(GM_ERROR);
      SETVBUILDCON(vList[i],1);
    }
  }
    #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
    {
      if (DisposeConnectionFromVector(theGrid,vList[i])) RETURN(GM_ERROR);
      SETVBUILDCON(vList[i],1);
    }
  }
    #endif
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
  {
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
    {
      if (DisposeConnectionFromVector(theGrid,vList[i])) RETURN(GM_ERROR);
      SETVBUILDCON(vList[i],1);
    }
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
  {
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
    {
      if (DisposeConnectionFromVector(theGrid,vList[i])) RETURN(GM_ERROR);
      SETVBUILDCON(vList[i],1);
    }
  }

  return(GM_OK);
}

/****************************************************************************/
/** \brief Remove matrices
 *
 * @param  theGrid - the grid to remove from
 * @param  theElement - that element
 * @param  Depth -  that many slices around the element

   This function removes connections concerning an element from the data structure
   and stores flags saying: "connection has to be rebuild",
   it does this in a neighborhood of the elem of depth Depth, where depth
   is the distance in the element-neighborship-graph (see also FORMAT).

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

static INT DisposeConnectionFromElementInNeighborhood (GRID *theGrid, ELEMENT *theElement, INT Depth)
{
  INT i;

  if (Depth < 0) RETURN (GM_ERROR);

  if (theElement==NULL) RETURN (GM_OK);

  /* create connection at that depth */
        #ifndef DYNAMIC_MEMORY_ALLOCMODEL
  if(!EBUILDCON(theElement))
        #endif
  if (DisposeConnectionFromElement(theGrid,theElement))
    RETURN (GM_ERROR);
  SETEBUILDCON(theElement,1);

  /* dispose connection in neighborhood */
  if (Depth > 0)
  {
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (DisposeConnectionFromElementInNeighborhood(theGrid,NBELEM(theElement,i),Depth-1))
        RETURN (GM_ERROR);
  }

  RETURN (GM_OK);
}

INT NS_DIM_PREFIX DisposeConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement)
{
  INT Depth;
  Depth = (INT)(floor(0.5*(double)FMT_CONN_DEPTH_MAX(MGFORMAT(MYMG(theGrid)))));
  return(DisposeConnectionFromElementInNeighborhood(theGrid,theElement,Depth));
}

INT NS_DIM_PREFIX DisposeConnectionsFromMultiGrid (MULTIGRID *theMG)
{
  INT i;

  for (i=0; i<=TOPLEVEL(theMG); i++)
  {
    GRID *theGrid = GRID_ON_LEVEL(theMG,i);
    ELEMENT *theElement;
    NODE *theNode;

    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
      if (DisposeConnectionsInNeighborhood(theGrid,theElement))
        REP_ERR_RETURN(1);

    if (NELIST_DEF_IN_GRID(theGrid))
      for (theNode = PFIRSTNODE(theGrid); theNode != NULL;
           theNode = SUCCN(theNode))
        if (DisposeElementList(theGrid,theNode))
          REP_ERR_RETURN(1);
  }

  return(0);
}

INT NS_DIM_PREFIX DisposeElementFromElementList (GRID *theGrid, NODE *theNode,
                                                 ELEMENT *theElement)
{
  ELEMENTLIST *pel,*next;

  pel = NODE_ELEMENT_LIST(theNode);
  if (pel == NULL) return(0);
  if (pel->el == theElement) {
    NDATA(theNode) = (void *) pel->next;
    return(PutFreeObject(theGrid->mg,pel,sizeof(ELEMENTLIST),MAOBJ));
  }
  next = pel->next;
  while (next != NULL) {
    if (next->el == theElement) {
      pel->next = next->next;
      return(PutFreeObject(theGrid->mg,next,sizeof(ELEMENTLIST),MAOBJ));
    }
    pel = next;
    next = pel->next;
  }

  return(0);
}

INT NS_DIM_PREFIX DisposeElementList (GRID *theGrid, NODE *theNode)
{
  ELEMENTLIST *pel,*next;

  pel = NODE_ELEMENT_LIST(theNode);
  while (pel != NULL) {
    next = pel->next;
    if (PutFreeObject(theGrid->mg,pel,sizeof(ELEMENTLIST),MAOBJ))
      return(1);
    pel = next;
  }
  NDATA(theNode) = NULL;

  return(0);
}

/****************************************************************************/
/** \brief Return pointer to matrix if it exists

 * @param FromVector - starting vector of the Matrix
 * @param ToVector - destination vector of the Matrix

   This function returns pointer to matrix if it exists. The function
   runs through the single linked list, since the list is
   assumed to be small (sparse matrix!) the cost is assumed to be negligible.

 * @return <ul>
 *   <li>      pointer to Matrix, </li>
 *   <li>      NULL if Matrix does not exist. </li>
   </ul>
 */
/****************************************************************************/

MATRIX * NS_DIM_PREFIX GetMatrix (const VECTOR *FromVector, const VECTOR *ToVector)
{
  MATRIX *theMatrix;

  for (theMatrix=VSTART(FromVector); theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
    if (MDEST(theMatrix)==ToVector)
      return (theMatrix);

  /* return not found */
  return (NULL);
}

MATRIX *NS_DIM_PREFIX GetOrderedMatrix (const VECTOR *FromVector, const VECTOR *ToVector)
{
  MATRIX *theMatrix;

  if (FromVector == ToVector)
    return(VSTART(ToVector));
  else if (VINDEX(FromVector) > VINDEX(ToVector)) {
    for (theMatrix=MNEXT(VSTART(FromVector));
         theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
      if (MDEST(theMatrix)==ToVector)
        return (theMatrix);
  }
  else {
    for (theMatrix=MNEXT(VSTART(ToVector));
         theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
      if (MDEST(theMatrix)==FromVector)
        return (MADJ(theMatrix));
  }

  /* return not found */
  return (NULL);
}

/****************************************************************************/
/** \brief Return pointer to connection if it exists

 * @param FromVector - starting vector of the con
 * @param ToVector - destination vector of the con

   This function returns pointer to connection if it exists.

 * @return <ul>
 *   <li>      pointer to </li>
 *   <li>      NULL if connection does not exist.  </li>
   </ul>
 */
/****************************************************************************/

CONNECTION * NS_DIM_PREFIX GetConnection (const VECTOR *FromVector, const VECTOR *ToVector)
{
  MATRIX *Matrix;

  Matrix = GetMatrix(FromVector,ToVector);
  if (Matrix != NULL)
    return (MMYCON(Matrix));

  /* return not found */
  return (NULL);
}

/****************************************************************************/
/** \brief Get a pointer list to all element data

 * @param theElement - that element
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function returns a pointer to the VECTOR associated with the
   element (if the element is allowed to have one). cnt will either
   be 0 or 1.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfElement (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  *cnt = 0;
  if (EVECTOR(theElement) != NULL)
    vList[(*cnt)++] = EVECTOR(theElement);

  IFDEBUG(gm,0)
  if (*cnt)
  {
    assert(vList[0] != NULL);
    assert(VOTYPE(vList[0]) == ELEMVEC);
  }
  ENDDEBUG

  return(GM_OK);
}

/****************************************************************************/
/** \brief Get a pointer list to all side data

 * @param theElement - that element
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function gets a pointer array to all VECTORs in sides of the given element.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

#ifdef __THREEDIM__
INT NS_DIM_PREFIX GetVectorsOfSides (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  INT i;

  *cnt = 0;

  for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    if (SVECTOR(theElement,i) != NULL)
      vList[(*cnt)++] = SVECTOR(theElement,i);

  IFDEBUG(gm,0)
  for (i=0; i<(*cnt); i++)
  {
    assert(vList[i] != NULL);
    assert(VOTYPE(vList[i]) == SIDEVEC);
  }
  ENDDEBUG

  return(GM_OK);
}
#endif

/****************************************************************************/
/** \brief Get a pointer list to all edge data

 * @param theElement -  that element
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function gets a pointer array to all VECTORs in edges of the element.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfEdges (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  EDGE *theEdge;
  INT i;

  *cnt = 0;
  for (i=0; i<EDGES_OF_ELEM(theElement); i++)
    if ((theEdge =
           GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                   CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)))) != NULL)
      if (EDVECTOR(theEdge) != NULL)
        vList[(*cnt)++] = EDVECTOR(theEdge);

  IFDEBUG(gm,0)
  for (i=0; i<(*cnt); i++)
  {
    assert(vList[i] != NULL);
    assert(VOTYPE(vList[i]) == EDGEVEC);
  }
  ENDDEBUG

  return(GM_OK);
}

/****************************************************************************/
/** \brief Get a pointer list to all node data

 * @param theElement -  that element
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function gets a pointer array to all VECTORs in nodes of the element.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfNodes (const ELEMENT *theElement, INT *cnt, VECTOR **vList)
{
  INT i;

  *cnt = 0;
  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
    if (NVECTOR(CORNER(theElement,i)) != NULL)
      vList[(*cnt)++] = NVECTOR(CORNER(theElement,i));

  IFDEBUG(gm,0)
  for (i=0; i<(*cnt); i++)
  {
    assert(vList[i] != NULL);
    assert(VOTYPE(vList[i]) == NODEVEC);
  }
  ENDDEBUG

  return (GM_OK);
}

/****************************************************************************/
/** \brief Get a pointer list to all vector data of specified object type

 * @param theElement -  that element
 * @param type - fill array with vectors of this type
 * @param cnt - how many vectors
 * @param vList - array to store vector list

   This function returns a pointer array to all VECTORs of the element of the specified type.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfOType (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList)
{
  switch (type)
  {
  case NODEVEC :
    return (GetVectorsOfNodes(theElement,cnt,vList));
  case ELEMVEC :
    return (GetVectorsOfElement(theElement,cnt,vList));
  case EDGEVEC :
    return (GetVectorsOfEdges(theElement,cnt,vList));
        #ifdef __THREEDIM__
  case SIDEVEC :
    return (GetVectorsOfSides(theElement,cnt,vList));
        #endif
  }
  RETURN (GM_ERROR);
}

/****************************************************************************/
/** \brief Remove vectors with non-matching type from list

 * @param dt - data type including all vtypes needed
 * @param vec - vector list
 * @param cnt  - number of vectors return in the list

   This function removes vectors with non-matching types from the list.

 * @return
 *   Number of remaining vectors in the list
 */
/****************************************************************************/

INT NS_DIM_PREFIX DataTypeFilterVList (INT dt, VECTOR **vec, INT *cnt)
{
  INT i,n;

  n = *cnt;
  *cnt = 0;
  for (i=0; i<n; i++)
    if (VDATATYPE(vec[i]) & dt)
      vec[(*cnt)++] = vec[i];

  return (*cnt);
}

/****************************************************************************/
/** \brief Get vector list including all vectors of specified vtypes

 * @param theElement - pointer to an element
 * @param dt  - data type including all vtypes needed
 * @param obj - flags for objects types needed
 * @param cnt - number of vectors return in the list
 * @param vec - vector list

   This function gets a list of vectors of the specified vtypes corresponding to an element.

 * @return <ul>
 *   <li>   GM_OK if ok </li>
 *   <li>   GM_ERROR if error occured </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetVectorsOfDataTypesInObjects (const ELEMENT *theElement, INT dt, INT obj, INT *cnt, VECTOR *VecList[])
{
  INT i,n;

  *cnt = n = 0;

  if (obj & BITWISE_TYPE(NODEVEC))
  {
    if (GetVectorsOfNodes(theElement,&i,VecList) != GM_OK)
      return(GM_ERROR);
    n += i;
  }

  if (obj & BITWISE_TYPE(EDGEVEC))
  {
    if (GetVectorsOfEdges(theElement,&i,VecList+n) != GM_OK)
      return(GM_ERROR);
    n += i;
  }

  if (obj & BITWISE_TYPE(ELEMVEC))
  {
    if (GetVectorsOfElement(theElement,&i,VecList+n) != GM_OK)
      return(GM_ERROR);
    n += i;
  }

    #ifdef __THREEDIM__
  if (obj & BITWISE_TYPE(SIDEVEC))
  {
    if (GetVectorsOfSides(theElement,&i,VecList+n) != GM_OK)
      return(GM_ERROR);
    n += i;
  }
    #endif

  *cnt = n;

  /* remove vectors of types not needed */
  DataTypeFilterVList(dt,VecList,cnt);

  return (GM_OK);
}



static void PrintVectorTriple (int i)
{
  VECTOR *vec0 = GBNV_list[i];
  VECTOR *vec1 = GBNV_list[i+1];
  VECTOR *vec2 = GBNV_list[i+2];
  VERTEX *vtx0 = MYVERTEX((NODE*)VOBJECT(vec0));
  VERTEX *vtx1 = MYVERTEX((NODE*)VOBJECT(vec1));
  VERTEX *vtx2 = MYVERTEX((NODE*)VOBJECT(vec2));
  PrintDebug("0: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec0),XC(vtx0),YC(vtx0));
  PrintDebug("1: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec1),XC(vtx1),YC(vtx1));
  PrintDebug("2: VTYPE=%d XC=%.5g YC=%.5g\n",VTYPE(vec2),XC(vtx2),YC(vtx2));
}

/****************************************************************************/
/** \brief Prepare GetBoundaryNeighbourVectors

 * @param theGrid - grid level
 * @param MaxListLen - max size of neighbourhood and therefore max list lenght of the
   VecList array of GetBoundaryNeighbourVectors

   This function stores lists of boundary vector neighbourhoods in temp storage
   of the multigrid. The neighborhoods of the boundary vectors can be received
   via a call of GetBoundaryNeighbourVectors one by one.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   GetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX PrepareGetBoundaryNeighbourVectors (GRID *theGrid, INT *MaxListLen)
{
#ifdef __TWODIM__
  ELEMENT *elem;
  VECTOR *vec,*v0,*v1;
  INT i;

  if (GBNV_list!=NULL)
    /* last process not finished properly by call of GetBoundaryNeighbourVectors */
    REP_ERR_RETURN(1);

  /* count boundary node vectors */
  GBNV_n = 0;
  for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
    if (VOTYPE(vec)==NODEVEC)
      if (OBJT(MYVERTEX((NODE*)VOBJECT(vec)))==BVOBJ)
        GBNV_n++;

  /* allocate list storage: 3 pointers each */
  GBNV_mg = MYMG(theGrid);
  MarkTmpMem(MGHEAP(GBNV_mg),&GBNV_MarkKey);
  GBNV_list = (VECTOR **) GetTmpMem(MGHEAP(GBNV_mg),3*GBNV_n*sizeof(VECTOR *),GBNV_MarkKey);
  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  /* store offset in vector index field */
  i = 0;
  for (vec=FIRSTVECTOR(theGrid); vec!=NULL; vec=SUCCVC(vec))
    if (VOTYPE(vec)==NODEVEC)
      if (OBJT(MYVERTEX((NODE*)VOBJECT(vec)))==BVOBJ)
      {
        VINDEX(vec) = i;
        GBNV_list[i] = vec;
        i += 3;
      }

  /* loop elements and fill neighbours */
  /* TODO: parallel also orphan(?) elements to be complete */
  for (elem=FIRSTELEMENT(theGrid); elem!=NULL; elem=SUCCE(elem))
    if (OBJT(elem)==BEOBJ)
      for (i=0; i<SIDES_OF_ELEM(elem); i++)
        if (ELEM_BNDS(elem,i)!=NULL)
        {
          /* 2D: two corners */
          v0 = NVECTOR(CORNER(elem,CORNER_OF_SIDE(elem,i,0)));
          v1 = NVECTOR(CORNER(elem,CORNER_OF_SIDE(elem,i,1)));

          ASSERT(GBNV_list[VINDEX(v0)]==v0);
          ASSERT(GBNV_list[VINDEX(v1)]==v1);
          GBNV_list[VINDEX(v0)+2] = v1;
          GBNV_list[VINDEX(v1)+1] = v0;
        }
  GBNV_curr = 0;

  /* this is simple in 2D: center, pred and succ in positive sense */
  *MaxListLen = 3;

  IFDEBUG(gm,2)
  PrintDebug("PrepareGetBoundaryNeighbourVectors:\n");
  for (i=0; i<GBNV_n; i++)
    PrintVectorTriple(3*i);
  ENDDEBUG

  return (0);

#endif /* __TWODIM__ */

#ifdef __THREEDIM__
  /* 3D requires somewhat more work! */

  /* but it should be possible to create an oriented list
     of neighbours for each boundary vector */

  REP_ERR_RETURN (1);
#endif /* __THREEDIM__ */
}

/****************************************************************************/
/** \brief Reset current neighbourhood to begin of list

   This function resets the pointer to the current neighbourhood to the beginning of
   the list. GetBoundaryNeighbourVectors will start again with the first one.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   GetBoundaryNeighbourVectors, PrepareGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX ResetGetBoundaryNeighbourVectors (void)
{
  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  GBNV_curr = 0;
  return (0);
}

/****************************************************************************/
/** \brief Get a neighbourhood of boundary vectors

 * @param dt - datatypes of center vectors (bitwise)
 * @param obj - object types of center vectors (bitwise)
 * @param cnt - vector list length
 * @param VecList - vector list
 * @param end - if YES the currently returned list was the last one

   This function returns a neighbourhood of boundary vectors, center first and the
   remainder oriented in positiv sense. The boundary vector heighbourhood lists
   are created by a call of PrepareGetBoundaryNeighbourVectors. Use
   ResetGetBoundaryNeighbourVectors to begin again with the first
   neighbourhood and finish processing the boundary vectors with a call of
   FinishBoundaryNeighbourVectors which releases the temporary storage
   occupied by PrepareGetBoundaryNeighbourVectors..

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>
 * \sa
   PrepareGetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, FinishBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetBoundaryNeighbourVectors (INT dt, INT obj, INT *cnt, VECTOR *VecList[])
{
  VECTOR *vec;

  *cnt = 0;

  if (GBNV_list==NULL)
    REP_ERR_RETURN(1);

  /* find next center vec matching data type */
  for (vec=GBNV_list[GBNV_curr]; GBNV_curr<3*GBNV_n; GBNV_curr+=3, vec=GBNV_list[GBNV_curr])
    if (BITWISE_TYPE(VTYPE(vec)) & dt)
      break;
  if (GBNV_curr>=3*GBNV_n)
    /* no (more) vector found */
    return (0);

  if (VOTYPE(vec)!=NODEVEC)
    REP_ERR_RETURN(1);

  IFDEBUG(gm,2)
  PrintDebug("GetBoundaryNeighbourVectors:\n");
  PrintVectorTriple(GBNV_curr);
  ENDDEBUG

  /* vector, pre and succ in positive sense */
    VecList[(*cnt)++] = GBNV_list[GBNV_curr];
  VecList[(*cnt)++] = GBNV_list[GBNV_curr+1];
  VecList[(*cnt)++] = GBNV_list[GBNV_curr+2];

  /* move on to next position */
  GBNV_curr += 3;

  return (0);
}

/****************************************************************************/
/** \brief Finish processing of boundary vectors

   This function releases the temporary memory allocated by PrepareGetBoundaryNeighbourVectors
   from the multigrid heap.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   >0 else </li>
   </ul>

 * \sa
   PrepareGetBoundaryNeighbourVectors, ResetGetBoundaryNeighbourVectors, GetBoundaryNeighbourVectors
 */
/****************************************************************************/

INT NS_DIM_PREFIX FinishBoundaryNeighbourVectors ()
{
        #ifdef __TWODIM
  if (ReleaseTmpMem(MGHEAP(GBNV_mg)),GBNV_MarkKey)
    REP_ERR_RETURN(1);
        #endif

  GBNV_list = NULL;

  return (0);
}

/****************************************************************************/
/** \brief Get vector list

 * @param theGrid - pointer to a grid
 * @param theElement - pointer to an element
 * @param vec - vector list

   This function gets a list of vectors corresponding to an element.

 * @return <ul>
 *   <li>   number of components </li>
 *   <li>   -1 if error occured </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GetAllVectorsOfElement (GRID *theGrid, ELEMENT *theElement, VECTOR **vec)
{
  INT i;
  INT cnt;

  cnt = 0;
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
  {
    if (GetVectorsOfNodes(theElement,&i,vec) == GM_ERROR)
      RETURN(-1);
    cnt += i;
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
  {
    if (GetVectorsOfEdges(theElement,&i,vec+cnt) == GM_ERROR)
      RETURN(-1);
    cnt += i;
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
  {
    if (GetVectorsOfElement(theElement,&i,vec+cnt) == GM_ERROR)
      RETURN(-1);
    cnt += i;
  }
    #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    if (GetVectorsOfSides(theElement,&i,vec+cnt) == GM_ERROR)
      RETURN(-1);
    cnt += i;
  }
    #endif

  return (cnt);
}

/****************************************************************************/
/** \brief Remove all extra connections from the grid

 * @param theGrid - grid to remove from

   This function removes all extra connections from the grid, i.e. those
   that have been allocated with the CreateExtraConnection function.

 * @return <ul>
 *   <li>      0 if ok
 *   <li>      1 if error occured
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX DisposeExtraConnections (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix, *nextMatrix;
  CONNECTION *theCon;

  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    theMatrix = VSTART(theVector);
    while (theMatrix!=NULL)
    {
      nextMatrix = MNEXT(theMatrix);
      theCon = MMYCON(theMatrix);
      if (CEXTRA(theCon)) DisposeConnection(theGrid,theCon);
      theMatrix = nextMatrix;
    }
  }
  return(GM_OK);
}

INT NS_DIM_PREFIX DisposeConnectionsInGrid (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix, *nextMatrix;
  CONNECTION *theCon;

  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    theMatrix = VSTART(theVector);
    while (theMatrix!=NULL)
    {
      nextMatrix = MNEXT(theMatrix);
      theCon = MMYCON(theMatrix);
      DisposeConnection(theGrid,theCon);
      theMatrix = nextMatrix;
    }
  }
  return(GM_OK);
}

/****************************************************************************/
/** \brief Create connections of two elements

 * @param theGrid - pointer to grid
 * @param Elem0,Elem1 - elements to be connected
 * @param ActDepth - distance of the two elements in the element neighborship graph
 * @param ConDepth - Array containing the connection depth desired
 * @param MatSize - Array containing the connection size. This is constructed from the information in the FORMAT.

   This function creates connections between all the VECTORs associated
   with two given elements according to the specifications in the ConDepth array.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT ElementElementCreateConnection (GRID *theGrid, ELEMENT *Elem0, ELEMENT *Elem1, INT ActDepth, INT *ConDepth, INT *MatSize)
{
  INT cnt0,cnt1,i,j,itype,jtype,mtype,size;
  VECTOR *vec0[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];
  VECTOR *vec1[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];

  cnt0 = GetAllVectorsOfElement(theGrid,Elem0,vec0);
  if (Elem0 == Elem1)
  {
    for (i=0; i<cnt0; i++)
    {
      itype=VTYPE(vec0[i]);
      for (j=i; j<cnt0; j++)
      {
        if (i==j)
        {
          mtype = DIAGMATRIXTYPE(itype);
          size  = MatSize[mtype];
        }
        else
        {
          jtype = VTYPE(vec0[j]);
          size  = MatSize[MATRIXTYPE(jtype,itype)];
          mtype = MATRIXTYPE(itype,jtype);
          if (MatSize[mtype]>size) size=MatSize[mtype];
        }
        if (size > 0)
          if (ActDepth <= ConDepth[mtype])
            if (CreateConnection(theGrid,vec0[i],vec0[j])==NULL)
              RETURN(GM_ERROR);
      }
    }
    if (NELIST_DEF_IN_GRID(theGrid))
      for (i=0; i<CORNERS_OF_ELEM(Elem0); i++)
        if (CreateElementList(theGrid,CORNER(Elem0,i),Elem0))
          RETURN(GM_ERROR);

    return (0);
  }

  cnt1 = GetAllVectorsOfElement(theGrid,Elem1,vec1);
  for (i=0; i<cnt0; i++)
  {
    itype=VTYPE(vec0[i]);
    for (j=0; j<cnt1; j++)
    {
      if (vec0[i]==vec1[j])
      {
        mtype = DIAGMATRIXTYPE(itype);
        size  = MatSize[mtype];
      }
      else
      {
        jtype = VTYPE(vec1[j]);
        size  = MatSize[MATRIXTYPE(jtype,itype)];
        mtype = MATRIXTYPE(itype,jtype);
        if (MatSize[mtype]>size) size=MatSize[mtype];
      }
      if (size > 0)
        if (ActDepth <= ConDepth[mtype])
          if (CreateConnection(theGrid,vec0[i],vec1[j])==NULL)
            RETURN(GM_ERROR);
    }
  }

  return (0);
}

/****************************************************************************/
/** \brief Get pointers to elements having a common side

 * @param theVector - given vector associated with a side of an element in 3D
 * @param Elements - array to be filled with two pointers of elements
 * @param Sides - array to be filled with side number within respective element

   Given a VECTOR associated with the side of an element, this function
   retrieves pointers to at most two elements having this side in common.
   If the side is part of the exterior boundary of the domain, then
   Elements[1] will be the NULL pointer.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    1 if error occured.
   </ul>
 */
/****************************************************************************/

#ifdef __THREEDIM__
INT NS_DIM_PREFIX GetElementInfoFromSideVector (const VECTOR *theVector, ELEMENT **Elements, INT *Sides)
{
  INT i;
  ELEMENT *theNeighbor;

  if (VOTYPE(theVector) != SIDEVEC)
    RETURN (1);
  Elements[0] = (ELEMENT *)VOBJECT(theVector);
  Sides[0] = VECTORSIDE(theVector);

  /* find neighbor */
  Elements[1] = theNeighbor = NBELEM(Elements[0],Sides[0]);
  if (theNeighbor == NULL) return (0);

  /* search the side */
  for (i=0; i<SIDES_OF_ELEM(theNeighbor); i++)
    if (NBELEM(theNeighbor,i) == Elements[0])
      break;

  /* found ? */
  if (i<SIDES_OF_ELEM(theNeighbor))
  {
    Sides[1] = i;
    return (0);
  }
  RETURN (1);
}
#endif

/****************************************************************************/
/** \brief Reset all USED flags in neighborhood of an element

 * @param theElement - given element
 * @param ActDepth - recursion depth
 * @param MaxDepth - end of recursion

   This function calls itself recursively and resets all USED flags in the
   neighborhood of depth MaxDepth of theElement. For the first call
   ActDepth should be zero.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT ResetUsedFlagInNeighborhood (ELEMENT *theElement, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0) SETUSED(theElement,0);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ResetUsedFlagInNeighborhood(NBELEM(theElement,i),ActDepth+1,MaxDepth)) RETURN (1);

  return (0);
}

static INT ConnectWithNeighborhood (ELEMENT *theElement, GRID *theGrid, ELEMENT *centerElement, INT *ConDepth, INT *MatSize, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if (ElementElementCreateConnection(theGrid,centerElement,theElement,
                                       ActDepth,ConDepth,MatSize))
      RETURN (1);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ConnectWithNeighborhood(NBELEM(theElement,i),theGrid,
                                  centerElement,ConDepth,MatSize,
                                  ActDepth+1,MaxDepth)) RETURN (1);

  return (0);
}

/****************************************************************************/
/** \brief Create connection of an element

 * @param theGrid - pointer to grid
 * @param theElement - pointer to an element

   This function creates connection for all VECTORs of an element
   with the depth specified in the FORMAT structure.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement)
{
  FORMAT *theFormat;
  INT MaxDepth;
  INT *ConDepth;
  INT *MatSize;

  /* set pointers */
  theFormat = GFORMAT(theGrid);
  MaxDepth = FMT_CONN_DEPTH_MAX(theFormat);
  ConDepth = FMT_CONN_DEPTH_PTR(theFormat);
  MatSize = FMT_S_MATPTR(theFormat);

  /* reset used flags in neighborhood */
  if (ResetUsedFlagInNeighborhood(theElement,0,MaxDepth))
    RETURN (1);

  /* create connection in neighborhood */
  if (ConnectWithNeighborhood(theElement,theGrid,theElement,ConDepth,
                              MatSize,0,MaxDepth))
    RETURN (1);

  return (0);
}

/****************************************************************************/
/** \brief Create connection of an inserted element

   This function creates connection of an inserted element ,
   i.e. an element is inserted interactively by the user.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT ConnectInsertedWithNeighborhood (ELEMENT *theElement, GRID *theGrid, INT ActDepth, INT MaxDepth)
{
  int i;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if (CreateConnectionsInNeighborhood(theGrid,theElement))
      RETURN (1);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (ConnectInsertedWithNeighborhood(NBELEM(theElement,i),theGrid,ActDepth+1,MaxDepth)) RETURN (1);

  return (0);
}

/****************************************************************************/
/** \brief Create connection of an inserted element

 * @param theGrid - grid level
 * @param theElement -  pointer to an element

   This function creates connections of an inserted element ,
   i.e. when an element is inserted interactively by the user. This function
   is inefficient and only intended for that purpose.

 * @return <ul>
 *   <li>  0 if ok
 *   <li>  1 if error occured.
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX InsertedElementCreateConnection (GRID *theGrid, ELEMENT *theElement)
{
  FORMAT *theFormat;
  INT MaxDepth;

  if (!MG_COARSE_FIXED(MYMG(theGrid)))
    RETURN (1);

  /* set pointers */
  theFormat = GFORMAT(theGrid);
  MaxDepth = (INT)(floor(0.5*(double)FMT_CONN_DEPTH_MAX(theFormat)));

  /* reset used flags in neighborhood */
  if (ResetUsedFlagInNeighborhood(theElement,0,MaxDepth))
    RETURN (1);

  /* call 'CreateConnectionsInNeighborhood'
     in a neighborhood of theElement */
  if (ConnectInsertedWithNeighborhood (theElement,theGrid,0,MaxDepth))
    RETURN (1);

  return (0);
}

/****************************************************************************/
/** \brief Create all connections needed on a grid level

 * @param theGrid - pointer to grid

   This function creates all connections needed on the grid.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    1 if error occured.
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX GridCreateConnection (GRID *theGrid)
{
  ELEMENT *theElement;
  VECTOR *vList[20];
  INT i,cnt;

  if (!MG_COARSE_FIXED(MYMG(theGrid)))
    RETURN (1);

  /* lets see if there's something to do */
  if (theGrid == NULL)
    return (0);

    #ifdef ModelP
        #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
    DDD_XferBegin();
        #endif
        #endif

  /* set EBUILDCON-flags also in elements accessing a vector with VBUILDCON true */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
  {
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
    {
      EDGE *ed;

      for (i=0; i<EDGES_OF_ELEM(theElement); i++) {
        ed = GetEdge(CORNER(theElement,
                            CORNER_OF_EDGE(theElement,i,0)),
                     CORNER(theElement,
                            CORNER_OF_EDGE(theElement,i,1)));
        ASSERT (ed != NULL);
        if (EDVECTOR(ed) == NULL) {
          CreateVector(theGrid,EDGEVEC,(GEOM_OBJECT *)ed,vList);
          EDVECTOR(ed) = vList[0];
                    #ifdef ModelP
                        #ifdef __THREEDIM__
          SETPRIO(EDVECTOR(ed),PRIO(ed));
                    #endif
                        #endif
        }
      }
    }

            #ifdef ModelP
    if (!EMASTER(theElement)) continue;
                #endif

    /* see if it is set */
    if (EBUILDCON(theElement)) continue;

    /* check flags in vectors */
                #ifdef __THREEDIM__
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
    {
      GetVectorsOfSides(theElement,&cnt,vList);
      for (i=0; i<cnt; i++)
        if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
    }
    if (EBUILDCON(theElement)) continue;
        #endif
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
    {
      GetVectorsOfEdges(theElement,&cnt,vList);
      for (i=0; i<cnt; i++)
        if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
    }
    if (EBUILDCON(theElement)) continue;
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
    {
      GetVectorsOfNodes(theElement,&cnt,vList);
      for (i=0; i<cnt; i++)
        if (VBUILDCON(vList[i])) {SETEBUILDCON(theElement,1); break;}
    }
  }

    #ifdef ModelP
        #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
    DDD_XferEnd();
        #endif
        #endif

  /* run over all elements with EBUILDCON true and build connections */
  for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
    if (EBUILDCON(theElement))             /* this is the trigger ! */
      if (CreateConnectionsInNeighborhood(theGrid,theElement))
        RETURN (1);

  return(GM_OK);
}


#ifdef ModelP
static int Gather_VectorVNew (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  ((INT *)data)[0] = VNEW(theVector);

  return(0);
}

static int Scatter_VectorVNew (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNEW(theVector,MAX(VNEW(theVector),((INT *)data)[0]));

  return(0);
}

static int Scatter_GhostVectorVNew (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNEW(theVector,((INT *)data)[0]);

  return(0);
}
#endif

INT NS_DIM_PREFIX SetSurfaceClasses (MULTIGRID *theMG)
{
  GRID *theGrid;
  ELEMENT *theElement;        VECTOR *v;
  INT level,fullrefine;

    #ifdef DYNAMIC_MEMORY_ALLOCMODEL
  level = TOPLEVEL(theMG);
  if (level > 0) {
    theGrid = GRID_ON_LEVEL(theMG,TOPLEVEL(theMG));
    ClearVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (MinNodeClass(theElement)==3)
        SeedVectorClasses(theGrid,theElement);
    }
    PropagateVectorClasses(theGrid);
    theGrid = GRID_ON_LEVEL(theMG,0);
    ClearNextVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      if (MinNextNodeClass(theElement)==3)
        SeedNextVectorClasses(theGrid,theElement);
    }
    PropagateNextVectorClasses(theGrid);
  }
  for (level--; level>0; level--)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    ClearVectorClasses(theGrid);
    ClearNextVectorClasses(theGrid);
    for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
         theElement=SUCCE(theElement)) {
      if (MinNodeClass(theElement)==3)
        SeedVectorClasses(theGrid,theElement);
      if (MinNextNodeClass(theElement)==3)
        SeedNextVectorClasses(theGrid,theElement);
    }
    PropagateVectorClasses(theGrid);
    PropagateNextVectorClasses(theGrid);
  }
    #endif
  fullrefine = TOPLEVEL(theMG);
  for (level=TOPLEVEL(theMG); level>=BOTTOMLEVEL(theMG); level--)
  {
    theGrid = GRID_ON_LEVEL(theMG,level);
    for (v=PFIRSTVECTOR(theGrid); v!= NULL; v=SUCCVC(v)) {
            #ifdef DYNAMIC_MEMORY_ALLOCMODEL
                    #ifdef Debug
      if (0)
        if (VOTYPE(v) == NODEVEC)
        {
          NODE *theNode = (NODE *) VOBJECT(v);

          if (NCLASS(theNode) > VCLASS(v))
            UserWriteF(PFMT " node=" ID_FMTX " c %d ncl %d vector="
                       VINDEX_FMTX
                       " c %d vc %d level %d\n",
                       me,
                       ID_PRTX(theNode),NCOPIES(theNode),NCLASS(theNode),
                       VINDEX_PRTX(v),NCOPIES(v),VCLASS(v),level);

          assert(NCLASS(theNode) <= VCLASS(v));


          if (NNCLASS(theNode) > VNCLASS(v))
            UserWriteF(PFMT " node=" ID_FMTX " c %d ncl %d vector="
                       VINDEX_FMTX
                       " c %d vc %d level %d\n",
                       me,
                       ID_PRTX(theNode),NCOPIES(theNode),NNCLASS(theNode),
                       VINDEX_PRTX(v),NCOPIES(v),VNCLASS(v),level);

          assert(NNCLASS(theNode) <= VNCLASS(v));
        }
                        #endif
                        #endif
      SETNEW_DEFECT(v,(VCLASS(v)>=2));
      SETFINE_GRID_DOF(v,((VCLASS(v)>=2)&&(VNCLASS(v)<=1)));
      if (FINE_GRID_DOF(v))
        fullrefine = level;
    }
  }
        #ifdef ModelP
  fullrefine = UG_GlobalMinINT(fullrefine);
        #endif

  FULLREFINELEVEL(theMG) = fullrefine;

  return(0);
}

/****************************************************************************/
/** \brief Creates the algebra for a grid

 * @param theGrid - pointer to grid

   This function allocates VECTORs in all geometrical objects of the grid
   (where described by format) and then calls GridCreateConnection.

 * @return <ul>
 *   <li>    GM_OK if ok
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateAlgebra (MULTIGRID *theMG)
{
  GRID *g;
  FORMAT *fmt;
  VECTOR *vec;
#ifdef __THREEDIM__
  VECTOR *nbvec;
  ELEMENT *nbelem;
  INT j,n;
#endif
  ELEMENT *elem;
  NODE *nd;
  LINK *li;
  EDGE *ed;
  INT side,i;


  if (MG_COARSE_FIXED(theMG) == false) {
    for (i=0; i<=TOPLEVEL(theMG); i++) {
      g = GRID_ON_LEVEL(theMG,i);

      if (NVEC(g)>0)
        continue;                               /* skip this level */

      fmt = MGFORMAT(MYMG(g));

      /* loop nodes and edges */
      for (nd=PFIRSTNODE(g); nd!=NULL; nd=SUCCN(nd)) {
        /* node vector */
        if (FMT_USES_OBJ(fmt,NODEVEC))
        {
          ASSERT(NVECTOR(nd)==NULL);
          if (CreateVector (g,NODEVEC,(GEOM_OBJECT *)nd,&vec))
            REP_ERR_RETURN (GM_ERROR);
          NVECTOR(nd) = vec;
        }

        /* edge vectors */
        if (FMT_USES_OBJ(fmt,EDGEVEC))
          for (li=START(nd); li!=NULL; li=NEXT(li)) {
            ed = MYEDGE(li);
            if (li==LINK0(ed))                                     /* to avoid double access of edges*/
            {
              ASSERT(EDVECTOR(ed)==NULL);
              if (CreateVector(g,EDGEVEC,(GEOM_OBJECT *)ed,&vec))
                REP_ERR_RETURN (GM_ERROR);
              EDVECTOR(ed) = vec;
            }
          }
      }

      /* loop elements and element sides */
      for (elem=PFIRSTELEMENT(g); elem!=NULL; elem=SUCCE(elem)) {
        /* to tell GridCreateConnection to build connections */
        if (EMASTER(elem)) SETEBUILDCON(elem,1);

        /* element vector */
        if (FMT_USES_OBJ(fmt,ELEMVEC)) {
          ASSERT(EVECTOR(elem)==NULL);
          if (CreateVector (g,ELEMVEC,(GEOM_OBJECT *)elem,&vec))
            REP_ERR_RETURN (GM_ERROR);
          SET_EVECTOR(elem,vec);
        }

        /* side vectors */
        if (FMT_USES_OBJ(fmt,SIDEVEC))
          for (side=0; side<SIDES_OF_ELEM(elem); side++)
            if (SVECTOR(elem,side)==NULL) {
              if (CreateSideVector (g,side,
                                    (GEOM_OBJECT *)elem,&vec))
                REP_ERR_RETURN (GM_ERROR);
              SET_SVECTOR(elem,side,vec);
            }
      }
    }
#ifdef __THREEDIM__
    /* dispose doubled side vectors */
    if (FMT_USES_OBJ(fmt,SIDEVEC))
      for (elem=PFIRSTELEMENT(g); elem!=NULL; elem=SUCCE(elem))
      {
        for(side=0; side<SIDES_OF_ELEM(elem); side++)
        {
          if(OBJT(elem)==BEOBJ)
          {
            if(INNER_SIDE(elem,side))
            {
              nbelem = NBELEM(elem,side);
              ASSERT(nbelem!=NULL);
              vec=SVECTOR(elem,side);
              n=0;
              for(j=0; j<SIDES_OF_ELEM(nbelem); j++)
              {
                nbvec=SVECTOR(nbelem,j);

                /* doubled sidevectors ? */
                if(NBELEM(nbelem,j)==elem)
                {
                  if(vec!=nbvec)
                  {
                    if(DisposeVector(g,nbvec))
                      REP_ERR_RETURN(GM_ERROR);
                    SET_SVECTOR(nbelem,j,vec);
                    SETVCOUNT(vec,2);                                                                     /* PB, 25 Sep 2005: changed from 1 to 2 */
                  }
                }
              }
              n++;
              ASSERT(n==1);
            }
          }else
          {
            nbelem = NBELEM(elem,side);
            ASSERT(nbelem!=NULL);
            vec=SVECTOR(elem,side);
            n=0;
            for(j=0; j<SIDES_OF_ELEM(nbelem); j++)
            {
              nbvec=SVECTOR(nbelem,j);
              ASSERT(nbvec!=NULL);

              /* doubled sidevectors ? */
              if(NBELEM(nbelem,j)==elem)
              {
                if(vec!=nbvec)
                {
                  if(DisposeVector(g,nbvec))
                    REP_ERR_RETURN(GM_ERROR);
                  SET_SVECTOR(nbelem,j,vec);
                  SETVCOUNT(vec,2);                                                               /* PB, 25 Sep 2005: changed from 1 to 2 */
                }
              }
            }
            n++;
            ASSERT(n==1);
          }
        }
      }
#endif
    MG_COARSE_FIXED(theMG) = true;

    /* now connections */
    if (MGCreateConnection(theMG))
      REP_ERR_RETURN (1);
  }

    #ifdef ModelP
    #ifndef __EXCHANGE_CONNECTIONS__
  MGCreateConnection(theMG);
    #endif
  /* update VNEW-flags */
  DDD_IFExchange(BorderVectorSymmIF,sizeof(INT),
                 Gather_VectorVNew,Scatter_VectorVNew);
  DDD_IFOneway(VectorIF,IF_FORWARD,sizeof(INT),
               Gather_VectorVNew,Scatter_GhostVectorVNew);
    #else
        #ifdef DYNAMIC_MEMORY_ALLOCMODEL
  MGCreateConnection(theMG);
        #endif
        #endif

        #ifdef __PERIODIC_BOUNDARY__
  IFDEBUG(gm,1)
  INT i;

  for (i=0; i<=TOPLEVEL(theMG); i++)
    if (Grid_CheckPeriodicity(GRID_ON_LEVEL(theMG,i)))
      return (GM_ERROR);
  ENDDEBUG
        #endif

  SetSurfaceClasses(theMG);

  return(GM_OK);
}

/****************************************************************************/
/** \brief Create all connections in multigrid

 * @param theMG - pointer to mulrigrid

   This function creates all connections in multigrid.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    1 if error occured.
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX MGCreateConnection (MULTIGRID *theMG)
{
  INT i;
  GRID *theGrid;
  ELEMENT *theElement;

  if (!MG_COARSE_FIXED(theMG))
    RETURN (1);

        #ifdef DYNAMIC_MEMORY_ALLOCMODEL
  if (theMG->bottomtmpmem) return(0);
  usefreelistmemory = 0;
  if (Mark(MGHEAP(theMG),FROM_BOTTOM,&freelist_end_mark)) REP_ERR_RETURN(1);
  theMG->bottomtmpmem = 1;
        #endif

  for (i=0; i<=theMG->topLevel; i++)
  {
    theGrid = GRID_ON_LEVEL(theMG,i);
    for (theElement=FIRSTELEMENT(theGrid); theElement!=NULL; theElement=SUCCE(theElement))
      SETEBUILDCON(theElement,1);
    if (GridCreateConnection(theGrid)) RETURN (1);
  }

  return (0);
}


INT NS_DIM_PREFIX PrepareAlgebraModification (MULTIGRID *theMG)
{
  int j,k;
  ELEMENT *theElement;
  VECTOR *theVector;
  MATRIX *theMatrix;

  j = theMG->topLevel;
  for (k=0; k<=j; k++)
  {
    for (theElement=PFIRSTELEMENT(GRID_ON_LEVEL(theMG,k)); theElement!=NULL; theElement=SUCCE(theElement))
    {
      SETUSED(theElement,0);
      SETEBUILDCON(theElement,0);
    }
    for (theVector=PFIRSTVECTOR(GRID_ON_LEVEL(theMG,k)); theVector!= NULL; theVector=SUCCVC(theVector))
      SETVBUILDCON(theVector,0);
    for (theVector=PFIRSTVECTOR(GRID_ON_LEVEL(theMG,k)); theVector!= NULL; theVector=SUCCVC(theVector))
    {
      SETVNEW(theVector,0);
      for (theMatrix=VSTART(theVector); theMatrix!=NULL; theMatrix = MNEXT(theMatrix))
        SETMNEW(theMatrix,0);
    }
  }

  return (0);
}

/****************************************************************************/
/** \brief Check connection of two elements

 * @param theGrid -  grid level to check
 * @param Elem0,Elem1 - elements to be checked.
 * @param ActDepth - recursion depth
 * @param ConDepth - connection depth as provided by format.
 * @param MatSize - matrix sizes as provided by format

   This function recursively checks connection of two elements.

 * @return <ul>
 *   <li>    0 if ok
 *   <li>    != 0 if errors occured.
   </ul>
 */
/****************************************************************************/

static INT ElementElementCheck (GRID *theGrid, ELEMENT *Elem0, ELEMENT *Elem1, INT ActDepth, INT *ConDepth, INT *MatSize)
{
  INT cnt0,cnt1,i,j,itype,jtype,mtype,size;
  VECTOR *vec0[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];
  VECTOR *vec1[MAX_SIDES_OF_ELEM+MAX_EDGES_OF_ELEM+MAX_CORNERS_OF_ELEM+1];
  CONNECTION *theCon;
  char msg[128];
  INT errors = 0;

  sprintf(msg,PFMT " ERROR: missing connection between elem0=" EID_FMTX " elem1=" EID_FMTX,
          me,EID_PRTX(Elem0),EID_PRTX(Elem1));

  cnt0 = GetAllVectorsOfElement(theGrid,Elem0,vec0);
  if (Elem0 == Elem1)
  {
    for (i=0; i<cnt0; i++)
    {
      itype=VTYPE(vec0[i]);
      for (j=0; j<cnt0; j++)
      {
        if (i==j)
        {
          mtype = DIAGMATRIXTYPE(itype);
          size  = MatSize[mtype];
        }
        else
        {
          jtype = VTYPE(vec0[j]);
          size  = MatSize[MATRIXTYPE(jtype,itype)];
          mtype = MATRIXTYPE(itype,jtype);
          if (MatSize[mtype]>size) size=MatSize[mtype];
        }
        if (size > 0)
          if (ActDepth <= ConDepth[mtype])
          {
            /* check connection in both directions */
            theCon = GetConnection(vec0[i],vec0[j]);
            if (theCon==NULL)
            {
              errors++;
              UserWriteF("%s vec0[%d]=" VINDEX_FMTX
                         " to vec0[%d]=" VINDEX_FMTX "\n",
                         msg,i,VINDEX_PRTX(vec0[i]),
                         j,VINDEX_PRTX(vec0[j]));
            }
            else
            {
              theCon = GetConnection(vec0[j],vec0[i]);
              if (theCon==NULL)
              {
                errors++;
                UserWriteF("%s vec0[%d]=" VINDEX_FMTX
                           " to vec0[%d]=" VINDEX_FMTX "\n",
                           msg,j,VINDEX_PRTX(vec0[j]),
                           i,VINDEX_PRTX(vec0[i]));
              }
              else
              {
                SETCUSED(theCon,1);
              }
            }
          }
      }
    }
    return (errors);
  }

  cnt1 = GetAllVectorsOfElement(theGrid,Elem1,vec1);
  for (i=0; i<cnt0; i++)
  {
    itype=VTYPE(vec0[i]);
    for (j=0; j<cnt1; j++)
    {
      if (i==j)
      {
        mtype = DIAGMATRIXTYPE(itype);
        size  = MatSize[mtype];
      }
      else
      {
        jtype = VTYPE(vec1[j]);
        size  = MatSize[MATRIXTYPE(jtype,itype)];
        mtype = MATRIXTYPE(itype,jtype);
        if (MatSize[mtype]>size) size=MatSize[mtype];
      }
      if (size > 0)
        if (ActDepth <= ConDepth[mtype])
        {
          /* check connection in both directions */
          theCon = GetConnection(vec0[i],vec1[j]);
          if (theCon==NULL)
          {
            errors++;
            UserWriteF("%s vec0[%d]=" VINDEX_FMTX
                       " to vec1[%d]=" VINDEX_FMTX "\n",
                       msg,i,VINDEX_PRTX(vec0[i]),
                       j,VINDEX_PRTX(vec1[j]));
          }
          else
          {
            theCon = GetConnection(vec1[j],vec0[i]);
            if (theCon == NULL)
            {
              errors++;
              UserWriteF("%s vec1[%d]=" VINDEX_FMTX
                         " to vec0[%d]=%x/" VINDEX_FMTX "\n",
                         msg,j,VINDEX_PRTX(vec1[j]),
                         i,VINDEX_PRTX(vec0[i]));
            }
            else
              SETCUSED(theCon,1);
          }
        }
    }
  }

  return (errors);
}

static INT CheckNeighborhood (GRID *theGrid, ELEMENT *theElement, ELEMENT *centerElement, INT *ConDepth, INT ActDepth, INT MaxDepth, INT *MatSize)
{
  INT i,errors = 0;

  /* is anything to do ? */
  if (theElement==NULL) return (0);

  /* action */
  if (ActDepth>=0)
    if ((errors+=ElementElementCheck(theGrid,centerElement,theElement,ActDepth,ConDepth,MatSize))!=0)
      return (errors);

  /* call all neighbors recursively */
  if (ActDepth<MaxDepth)
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if ((errors+=CheckNeighborhood(theGrid,NBELEM(theElement,i),centerElement,ConDepth,ActDepth+1,MaxDepth,MatSize))!=0)
        return(errors);

  return (0);
}

/****************************************************************************/
/** \brief Check connection of the element

 * @param theGrid - pointer to grid
 * @param theElement - pointer to element

   This function checks all connections of the given element.

 * @return <ul>
 *   <li>  number of errors
 *   <li>  0 if ok
 *   <li>  != 0 if errors occured in that connection.
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX ElementCheckConnection (GRID *theGrid, ELEMENT *theElement)
{
  FORMAT *theFormat;
  INT MaxDepth;
  INT *ConDepth;
  INT *MatSize;

  /* set pointers */
  theFormat = GFORMAT(theGrid);
  MaxDepth = FMT_CONN_DEPTH_MAX(theFormat);
  ConDepth = FMT_CONN_DEPTH_PTR(theFormat);
  MatSize = FMT_S_MATPTR(theFormat);

  /* call elements recursivly */
  return(CheckNeighborhood(theGrid,theElement,theElement,ConDepth,0,MaxDepth,MatSize));
}

/****************************************************************************/
/** \brief Check if connections are correctly allocated

 * @param theGrid -  grid level to check

   This function checks if connections are correctly allocated.

 * @return <ul>
 *   <li>    GM_OK if ok
 *   <li>    GM_ERROR	if error occured.
   </ul>
 */
/****************************************************************************/

static INT CheckConnections (GRID *theGrid)
{
  ELEMENT *theElement;
  INT errors;
  INT error;

  errors = 0;

  for (theElement=FIRSTELEMENT(theGrid);
       theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    if ((error=ElementCheckConnection(theGrid,theElement))!=0)
    {
      UserWriteF(PFMT "%d: element=" EID_FMTX " has bad connections\n",
                 me, EID_PRTX(theElement));
      errors+=error;
    }
  }

  return(errors);
}


/****************************************************************************/
/** \brief Checks validity of geom_object	and its vector

 * @param fmt - FORMAT of associated multigrid
 * @param s2p - subdomain to part table
 * @param theObject - the object which points to theVector
 * @param ObjectString - for error message
 * @param theVector - the vector of theObject
 * @param VectorObjType - NODEVEC,...
 * @param side - element side for SIDEVEC, NOSIDE else

   This function checks the consistency between an geom_object and
   its vector.

 * @return <ul>
 *   <li>    GM_OK if ok
 *   <li>    GM_ERROR	if error occured.
   </ul>
 */
/****************************************************************************/

static INT CheckVector (const FORMAT *fmt, const INT s2p[], GEOM_OBJECT *theObject, const char *ObjectString,
                        VECTOR *theVector, INT VectorObjType, INT side)
{
  GEOM_OBJECT *VecObject;
  MATRIX *mat;
  INT errors = 0,vtype,DomPart,ds;

  if (theVector == NULL)
  {
    /* check if size is really 0 */
    DomPart = GetDomainPart(s2p,theObject,side);
    vtype = FMT_PO2T(fmt,DomPart,VectorObjType);
    ds = FMT_S_VEC_TP(fmt,vtype);
    if (ds>0)
    {
      errors++;
      UserWriteF("%d: %s ID=%ld  has NO VECTOR",me, ObjectString,
                 ID(theObject));
                        #ifdef ModelP
                        #ifdef __THREEDIM__
      if (VectorObjType == EDGEVEC)
        UserWriteF(" prio=%d",PRIO((EDGE*)theObject));
                        #endif
                        #endif
      UserWrite("\n");
    }
  }
  else
  {
    ds = FMT_S_VEC_TP(fmt,VTYPE(theVector));
    if (ds==0)
    {
      errors++;
      UserWriteF("%d: %s ID=%ld  exists but should not\n",me, ObjectString,
                 ID(theObject));
    }
    SETVCUSED(theVector,1);

    VecObject = VOBJECT(theVector);
    if (VecObject == NULL)
    {
      errors++;
      UserWriteF("%d: vector=" VINDEX_FMTX " %s GID=" GID_FMT " has NO BACKPTR\n",
                 me, VINDEX_PRTX(theVector), ObjectString,
                 (OBJT(theObject)==BEOBJ || OBJT(theObject)==IEOBJ) ?
                 EGID(&(theObject->el)) : (OBJT(theObject)==NDOBJ) ?
                 GID(&(theObject->nd)) :
                 GID(&(theObject->ed))
                 );
    }
    else
    {
      if (VOTYPE(theVector) != VectorObjType)
      {
        errors++;
        UserWriteF("%d: %s vector=" VINDEX_FMTX " has incompatible type=%d, "
                   "should be type=%s\n",
                   me, ObjectString, VINDEX_PRTX(theVector), VTYPE(theVector),
                   ObjTypeName[VectorObjType]);
      }

      if (VecObject != theObject)
      {
        if (OBJT(VecObject) != OBJT(theObject))
        {
          int error = 1;

          /* both objects may be elements */
          if ((OBJT(VecObject)==BEOBJ || OBJT(VecObject)==IEOBJ) &&
              (OBJT(theObject)==BEOBJ || OBJT(theObject)==IEOBJ) )
          {
            ELEMENT *theElement = (ELEMENT *)theObject;
            ELEMENT *vecElement = (ELEMENT *)VecObject;
            int i;

                                                #ifdef ModelP
            if (EMASTER(theElement) ||
                EMASTER(vecElement) )
            {
                                                #endif
            for (i=0; i<SIDES_OF_ELEM(theElement); i++)
              if (NBELEM(theElement,i) == vecElement)
              {
                /* they are neighbors -> ok */
                error = 0;
                break;
              }
                                                #ifdef ModelP
          }
                                                #endif
            if (error)
            {
              UserWriteF("%d: vector=" VINDEX_FMTX " has type %s, but points "
                         "to wrong vecobj=" EID_FMTX " NO NB of obj=" EID_FMTX "\n",
                         me,VINDEX_PRTX(theVector),ObjectString,
                         EID_PRTX(vecElement),EID_PRTX(theElement));
            }
          }
          else
          {
            errors++;
            UserWriteF("%d: vector=" VINDEX_FMTX " has type %s, but points "
                       "to wrong obj=%d type OBJT=%d\n",
                       me,VINDEX_PRTX(theVector),ObjectString,ID(VecObject),
                       OBJT(VecObject));
          }
        }
        else
        {
                                        #ifdef __THREEDIM__
          if (VectorObjType == SIDEVEC)
          {
            /* TODO: check side vector */
          }
          else
                                        #endif
                                        #ifndef __PERIODIC_BOUNDARY__
          {
            errors++;
            UserWriteF("%d: %s vector=" VINDEX_FMTX " is referenced by "
                       "obj0=%x, but points to wrong obj1=%x\n",
                       me, ObjectString, VINDEX_PRTX(theVector),
                       theObject, VecObject);
                                                #ifdef ModelP
            if (strcmp(ObjectString,"EDGE")==0)
              UserWriteF("%d: obj0: n0=%d n1=%d  obj1: "
                         "n0=%d n1=%d\n",me,
                         GID(NBNODE(LINK0(&(theObject->ed)))),
                         GID(NBNODE(LINK1(&(theObject->ed)))),
                         GID(NBNODE(LINK0(&(VecObject->ed)))),
                         GID(NBNODE(LINK1(&(VecObject->ed)))) );
                                                #endif
          }
                                        #else
          /* TODO: check whether reference to node is valid */
          if (HEAPCHECK(VecObject) || VecObject==NULL)
            UserWriteF("%d: %s vector=" VINDEX_FMTX " is referenced by "
                       "obj0=%x, but points to ZOMBIE obj1=%x\n",
                       me, ObjectString, VINDEX_PRTX(theVector),
                       theObject, VecObject);
                                        #endif
        }
      }
    }
    /* check connectivity of matrices */
    for (mat=VSTART(theVector); mat!=NULL; mat=MNEXT(mat))
      if (MDEST(mat)==NULL)
      {
        errors++;
        UserWriteF("%d: %s vector=" VINDEX_FMTX ": matrix dest==NULL\n",
                   me, ObjectString, VINDEX_PRTX(theVector));
      }
      else if (MDEST(MADJ(mat))!=theVector)
      {
        errors++;
        UserWriteF("%d: %s vector=" VINDEX_FMTX ": adj matrix dest does not coincide"
                   " with vector conn=%x mat=%x mdest=%x\n",
                   me, ObjectString, VINDEX_PRTX(theVector),MMYCON(mat),MDEST(mat),MDEST(MADJ(mat)));
      }

  }

  return(errors);
}

/****************************************************************************/
/** \brief Check the algebraic part of the data structure

 * @param theGrid -  grid level to check

   This function checks the consistency of the algebraic data structures
   including the interconnections between the geometric part.
   This function assumes a correct geometric data structure.

 * @return <ul>
 *   <li>    GM_OK if ok </li>
 *   <li>    GM_ERROR	if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX CheckAlgebra (GRID *theGrid)
{
  FORMAT *fmt;
  ELEMENT *theElement;
  NODE *theNode;
  VECTOR *theVector;
  EDGE *theEdge;
  LINK *theLink;
  INT errors;
  INT *s2p;
#       ifdef __THREEDIM__
  INT i;
#       endif

  errors = 0;

  if ((GLEVEL(theGrid)==0) && !MG_COARSE_FIXED(MYMG(theGrid)))
  {
    if ((NVEC(theGrid)>0) || (NC(theGrid)>0))
    {
      errors++;
      UserWriteF("coarse grid not fixed but vectors allocated\n");
    }
    return(errors);
  }

  s2p = BVPD_S2P_PTR(MG_BVPD(MYMG(theGrid)));
  fmt = MGFORMAT(MYMG(theGrid));

  /* reset USED flag */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    SETVCUSED(theVector,0);
  }

  /* check pointers to element, side, edge vector */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {

    /* check element vectors */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
    {
      theVector = EVECTOR(theElement);
      errors += CheckVector(fmt,s2p,(GEOM_OBJECT *) theElement, "ELEMENT",
                            theVector, ELEMVEC,NOSIDE);
    }

                #ifdef __THREEDIM__
    /* check side vectors */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
    {
      for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      {
        theVector = SVECTOR(theElement,i);
        errors += CheckVector(fmt,s2p,(GEOM_OBJECT *) theElement, "ELEMSIDE",
                              theVector, SIDEVEC,i);
      }
    }
                #endif
  }

  for (theNode=PFIRSTNODE(theGrid); theNode!=NULL; theNode=SUCCN(theNode))
  {

    /* check node vectors */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
    {
      theVector = NVECTOR(theNode);
      errors += CheckVector(fmt,s2p,(GEOM_OBJECT *) theNode, "NODE", theVector,
                            NODEVEC,NOSIDE);
    }

    /* check edge vectors */
    if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
    {
      for (theLink=START(theNode); theLink!=NULL; theLink=NEXT(theLink))
      {
        theEdge = GetEdge(theNode,NBNODE(theLink));
        if (theEdge != NULL) {
          theVector = EDVECTOR(theEdge);
          errors += CheckVector(fmt,s2p,(GEOM_OBJECT *) theEdge, "EDGE",
                                theVector, EDGEVEC,NOSIDE);
        }
      }
    }
  }

  /* check USED flag */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    if (VCUSED(theVector) != 1)
    {
      errors++;
      UserWriteF("%d: vector" VINDEX_FMTX " NOT referenced by an geom_object: "
                 "vtype=%d, objptr=%x",
                 me, VINDEX_PRTX(theVector), VTYPE(theVector), VOBJECT(theVector));
      if (VOBJECT(theVector) != NULL)
        UserWriteF(" objtype=%d\n",OBJT(VOBJECT(theVector)));
      else
        UserWrite("\n");
    }
    else
      SETVCUSED(theVector,0);
  }

  /* check validity of all defined connections */
  errors += CheckConnections(theGrid);

  /* reset flags in connections */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    MATRIX  *theMatrix;

    for (theMatrix=VSTART(theVector); theMatrix!=NULL;
         theMatrix = MNEXT(theMatrix)) SETCUSED(MMYCON(theMatrix),0);
  }

  /* set flags in connections */
#if defined __OVERLAP2__
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
#else
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
#endif
  {
    MATRIX  *theMatrix;

    for (theMatrix=VSTART(theVector); theMatrix!=NULL;
         theMatrix = MNEXT(theMatrix)) SETMUSED(MADJ(theMatrix),1);
  }

  /* check matrices and connections */
  for (theVector=PFIRSTVECTOR(theGrid);
       theVector!=NULL;
       theVector=SUCCVC(theVector))
  {
    MATRIX  *theMatrix;
                #ifdef ModelP
    INT prio = PRIO(theVector);
                #endif

    for (theMatrix=VSTART(theVector);
         theMatrix!=NULL;
         theMatrix = MNEXT(theMatrix))
    {
      MATRIX *Adj = MADJ(theMatrix);

      if (MDEST(theMatrix) == NULL)
      {
        errors++;
        UserWriteF(PFMT "ERROR: matrix %x has no dest, start vec="
                   VINDEX_FMTX "\n",
                   me,theMatrix,VINDEX_PRTX(theVector));
      }
      if (MDEST(Adj) != theVector)
      {
        errors++;
        UserWriteF(PFMT "ERROR: dest=%x of adj matrix "
                   " unequal vec=" VINDEX_FMTX "\n",
                   me, MDEST(Adj),VINDEX_PRTX(theVector));
      }

                        #if defined ModelP && ! defined  __OVERLAP2__
      if (prio != PrioHGhost)
                        #endif
      if (MUSED(theMatrix)!=1 &&  !CEXTRA(MMYCON(theMatrix)))
      {
        errors++;
        UserWriteF(PFMT "ERROR: connection dead vec=" VINDEX_FMTX
                   " vector=" VINDEX_FMTX " con=%x mat=%x matadj=%x level(vec)=%d is_extra_con %d\n",
                   me,VINDEX_PRTX(theVector),VINDEX_PRTX(MDEST(theMatrix)),
                   MMYCON(theMatrix),MDEST(theMatrix),MDEST(MADJ(theMatrix)),
                   GLEVEL(theGrid),CEXTRA(MMYCON(theMatrix)));
      }

                        #if defined ModelP && ! defined  __OVERLAP2__
      if (GHOSTPRIO(prio) && !CEXTRA(MMYCON(theMatrix)))
      {
        errors++;
        UserWriteF(PFMT "ERROR: ghost vector has matrix vec="
                   VINDEX_FMTX " con=%x mat=%x\n",
                   me,VINDEX_PRTX(theVector),MMYCON(theMatrix),theMatrix);
      }
                        #endif
    }
  }

  return(errors);
}

/****************************************************************************/
/** \brief Decide whether a vector corresponds to an element or not

 * @param theElement - pointer to element
 * @param theVector - pointer to a vector

   This function decides whether a given vector belongs to the given element, or
   one of its sides, edges or nodes.

 * @return <ul>
 *   <li>   0 if does not correspond </li>
 *   <li>   1 if does correspond.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX VectorInElement (ELEMENT *theElement, VECTOR *theVector)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  if (VOTYPE(theVector) == ELEMVEC)
  {
    GetVectorsOfElement(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (vList[i]==theVector) RETURN(1);
  }
        #ifdef __THREEDIM__
  if (VOTYPE(theVector) == SIDEVEC)
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (vList[i]==theVector) RETURN(1);
  }
    #endif
  if (VOTYPE(theVector) == EDGEVEC)
  {
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (vList[i]==theVector) RETURN(1);
  }
  if (VOTYPE(theVector) == NODEVEC)
  {
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++)
      if (vList[i]==theVector) RETURN(1);
  }

  return (0);
}

/****************************************************************************/
/** \brief Calc coordinate position of vector

 * @param theVector - a given vector
 * @param position - array to be filled

   This function calcs physical position of vector. For edge vectors the
   midpoint is returned, and for sides and elements the center of mass.

 * @return <ul>
 *   <li>    0 if ok                     </li>
 *   <li>    1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX VectorPosition (const VECTOR *theVector, DOUBLE *position)
{
  INT i;
  EDGE *theEdge;
        #ifdef __THREEDIM__
  ELEMENT *theElement;
  INT theSide,j;
        #endif

  ASSERT(theVector != NULL);

        #if defined __OVERLAP2__
  if( VOBJECT(theVector) == NULL )
  {
    for (i=0; i<DIM; i++)
      position[i] = -MAX_D;
    return (0);
  }
        #endif

  switch (VOTYPE(theVector))
  {
  case NODEVEC :
    for (i=0; i<DIM; i++)
      position[i] = CVECT(MYVERTEX((NODE*)VOBJECT(theVector)))[i];
    return (0);

  case EDGEVEC :
    theEdge = (EDGE*)VOBJECT(theVector);
    for (i=0; i<DIM; i++)
      position[i] = 0.5*(CVECT(MYVERTEX(NBNODE(LINK0(theEdge))))[i] +
                         CVECT(MYVERTEX(NBNODE(LINK1(theEdge))))[i]   );
    return (0);
                #ifdef __THREEDIM__
  case SIDEVEC :
    theElement = (ELEMENT *)VOBJECT(theVector);
    theSide = VECTORSIDE(theVector);
    for (i=0; i<DIM; i++)
    {
      position[i] = 0.0;
      for(j=0; j<CORNERS_OF_SIDE(theElement,theSide); j++)
        position[i] += CVECT(MYVERTEX(CORNER(theElement,CORNER_OF_SIDE(theElement,theSide,j))))[i];
      position[i] /= CORNERS_OF_SIDE(theElement,theSide);
    }
    return (0);
                #endif
  case ELEMVEC :
    /* calculate center of mass */
    CalculateCenterOfMass( (ELEMENT *) VOBJECT(theVector), position );
    return (0);

  default : PrintErrorMessage('E',"VectorPosition","unrecognized object type for vector");
    assert(0);
  }

  RETURN (GM_ERROR);
}


/****************************************************************************/
/** \brief Initialize vector classes

 * @param theGrid - given grid
 * @param theElement - given element

   Initialize vector class in all vectors associated with given element with 3.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SeedVectorClasses (GRID *theGrid, ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
  {
    GetVectorsOfElement(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  }
        #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  }
    #endif
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
  {
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
  {
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVCLASS(vList[i],3);
  }
  return (0);
}

/****************************************************************************/
/** \brief Reset vector classes

 * @param theGrid - pointer to grid

   Reset all vector classes in all vectors of given grid to 0.

 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX ClearVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    SETVCLASS(theVector,0);

  return(0);
}


#ifdef ModelP
static int Gather_VectorVClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  PRINTDEBUG(gm,1,(PFMT "Gather_VectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   me,VINDEX_PRTX(theVector),VCLASS(theVector)))

    ((INT *)data)[0] = VCLASS(theVector);

  return(0);
}

static int Scatter_VectorVClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVCLASS(theVector,MAX(VCLASS(theVector),((INT *)data)[0]));

  PRINTDEBUG(gm,2,(PFMT "Scatter_VectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   me,VINDEX_PRTX(theVector),VCLASS(theVector)))

  return(0);
}

static int Scatter_GhostVectorVClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVCLASS(theVector,((INT *)data)[0]);

  PRINTDEBUG(gm,1,(PFMT "Scatter_GhostVectorVClass(): v=" VINDEX_FMTX " vclass=%d\n",
                   me,VINDEX_PRTX(theVector),VCLASS(theVector)))

  return(0);
}
#endif

static INT PropagateVectorClass (GRID *theGrid, INT vclass)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the algebraic neighborhood to vclass-1 */
  /* use matrices to determine next vectors!!!!!                   */
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
    if ((VCLASS(theVector)==vclass)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL;
           theMatrix=MNEXT(theMatrix))
        if ((VCLASS(MDEST(theMatrix))<vclass)
            &&(CEXTRA(MMYCON(theMatrix))!=1))
          SETVCLASS(MDEST(theMatrix),vclass-1);

  /* only for this values valid */
  ASSERT(vclass==3 || vclass==2);

  return(0);
}
#ifdef __PERIODIC_BOUNDARY__
static INT PropagatePeriodicVectorClass (GRID *theGrid, INT vclass)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the algebraic neighborhood to vclass-1 */
  /* use matrices to determine next vectors!!!!!                   */
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    if ((VCLASS(theVector)==vclass)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL;
           theMatrix=MNEXT(theMatrix))
        if ((VCLASS(MDEST(theMatrix))<vclass)
            &&(CEXTRA(MMYCON(theMatrix))!=1)) {
          SETVCLASS(MDEST(theMatrix),vclass);
        }

  /* only for this value valid */
  ASSERT(vclass==1);

  return(0);
}
#endif


/****************************************************************************/
/** \brief Compute vector classes after initialization

 * @param theGrid - pointer to grid

   After vector classes have been reset and initialized, this function
   now computes the class 2 and class 1 vectors.

 * @return <ul>
 *   <li>     0 if ok </li>
 *   <li>     1 if error occured	 </li>
   </ul>
 */
/****************************************************************************/
INT NS_DIM_PREFIX PropagateVectorClasses (GRID *theGrid)
{
    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateVectorClasses():"
                   " 1. communication on level %d\n",me,GLEVEL(theGrid)))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

  /* set vector classes in the algebraic neighborhood to 2 */
  if (PropagateVectorClass(theGrid,3)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateVectorClasses(): 2. communication\n",
                   me))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

  /* set vector classes in the algebraic neighborhood to 1 */
  if (PropagateVectorClass(theGrid,2)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateVectorClasses(): 3. communication\n",
                   me))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
    #endif

#ifdef __PERIODIC_BOUNDARY__
  /* set vector classes in the periodic neighbourhood to 1 */
  if (PropagatePeriodicVectorClass(theGrid,1)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateVectorClasses(): 4. communication\n",
                   me))
  /* exchange VCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVClass, Scatter_VectorVClass);
        #endif
#endif

        #ifdef ModelP
  /* send VCLASS to ghosts */
  DDD_IFAOneway(VectorIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_VectorVClass, Scatter_GhostVectorVClass);
    #endif

  return(0);
}

/****************************************************************************/
/** \brief Reset class of the vectors on the next level

 * @param theGrid - pointer to grid

   This function clears VNCLASS flag in all vectors. This is the first step to
   compute the class of the dofs on the *NEXT* level, which
   is also the basis for determining copies.

 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured.			 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX ClearNextVectorClasses (GRID *theGrid)
{
  VECTOR *theVector;

  /* reset class of each vector to 0 */
  for (theVector=PFIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    SETVNCLASS(theVector,0);

  /* now the refinement algorithm will initialize the class 3 vectors */
  /* on the *NEXT* level.                                                                                       */
  return(0);
}

/****************************************************************************/
/** \brief Set VNCLASS in all vectors associated with element

 * @param theGrid - given grid
 * @param theElement - pointer to element

   Set VNCLASS in all vectors associated with the element to 3.

 * @return <ul>
 *   <li>    0 if ok  </li>
 *   <li>    1 if error occured.	 </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX SeedNextVectorClasses (GRID *theGrid, ELEMENT *theElement)
{
  INT i;
  VECTOR *vList[20];
  INT cnt;

  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
  {
    GetVectorsOfElement(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  }
        #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  }
        #endif
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
  {
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
  {
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) SETVNCLASS(vList[i],3);
  }
  return (0);
}




#ifdef ModelP
static int Gather_VectorVNClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  PRINTDEBUG(gm,2,(PFMT "Gather_VectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   me,VINDEX_PRTX(theVector),VNCLASS(theVector)))

    ((INT *)data)[0] = VNCLASS(theVector);

  return(GM_OK);
}

static int Scatter_VectorVNClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNCLASS(theVector,MAX(VNCLASS(theVector),((INT *)data)[0]));

  PRINTDEBUG(gm,2,(PFMT "Scatter_VectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   me,VINDEX_PRTX(theVector),VNCLASS(theVector)))

  return(GM_OK);
}

static int Scatter_GhostVectorVNClass (DDD_OBJ obj, void *data)
{
  VECTOR *theVector = (VECTOR *)obj;

  SETVNCLASS(theVector,((INT *)data)[0]);

  PRINTDEBUG(gm,2,(PFMT "Scatter_GhostVectorVNClass(): v=" VINDEX_FMTX " vnclass=%d\n",
                   me,VINDEX_PRTX(theVector),VNCLASS(theVector)))

  return(GM_OK);
}
#endif

static INT PropagateNextVectorClass (GRID *theGrid, INT vnclass)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the algebraic neighborhood to vnclass-1 */
  /* use matrices to determine next vectors!!!!!                   */
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
    if ((VNCLASS(theVector)==vnclass)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL;
           theMatrix=MNEXT(theMatrix))
        if ((VNCLASS(MDEST(theMatrix))<vnclass)
            &&(CEXTRA(MMYCON(theMatrix))!=1))
          SETVNCLASS(MDEST(theMatrix),vnclass-1);

  /* only for this values valid */
  ASSERT(vnclass==3 || vnclass==2);

  return(0);
}

#ifdef __PERIODIC_BOUNDARY__
static INT PropagatePeriodicNextVectorClass (GRID *theGrid)
{
  VECTOR *theVector;
  MATRIX *theMatrix;

  /* set vector classes in the periodic neighborhood to 1 */
  /* use matrices to determine next vectors!!!!!                   */
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL;
       theVector=SUCCVC(theVector))
    if ((VNCLASS(theVector)==1)&&(VSTART(theVector)!=NULL))
      for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL;
           theMatrix=MNEXT(theMatrix))
        if ((VNCLASS(MDEST(theMatrix))<1)
            &&(CEXTRA(MMYCON(theMatrix))!=1)) {
          SETVNCLASS(MDEST(theMatrix),1);
          PRINTDEBUG(gm,1,("VNCLASS(%d)=1\n",VINDEX(MDEST(theMatrix))));
        }

  return(0);
}
#endif

/****************************************************************************/
/** \brief Compute VNCLASS in all vectors of a grid level

 * @param theGrid - pointer to grid

   Computes values of VNCLASS field in all vectors after seed.

 * @return <ul>
 *   <li>   0 if ok              </li>
 *   <li>   1 if error occured </li>
   </ul>
 */
/****************************************************************************/
INT NS_DIM_PREFIX PropagateNextVectorClasses (GRID *theGrid)
{
    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateNextVectorClasses(): 1. communication\n",me))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
    #endif

  if (PropagateNextVectorClass(theGrid,3)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateNextVectorClasses(): 2. communication\n",me))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
    #endif

  if (PropagateNextVectorClass(theGrid,2)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateNextVectorClasses(): 3. communication\n",me))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
        #endif

#ifdef __PERIODIC_BOUNDARY__
  if (PropagatePeriodicNextVectorClass(theGrid)) REP_ERR_RETURN(1);

    #ifdef ModelP
  PRINTDEBUG(gm,1,("\n" PFMT "PropagateNextVectorClasses(): 4. communication\n",me))
  /* exchange VNCLASS of vectors */
  DDD_IFAExchange(BorderVectorSymmIF,GRID_ATTR(theGrid),sizeof(INT),
                  Gather_VectorVNClass, Scatter_VectorVNClass);
        #endif
#endif

        #ifdef ModelP
  /* send VCLASS to ghosts */
  DDD_IFAOneway(VectorIF,GRID_ATTR(theGrid),IF_FORWARD,sizeof(INT),
                Gather_VectorVNClass, Scatter_GhostVectorVNClass);
    #endif

  return(0);
}

/****************************************************************************/
/** \brief Returns highest vector class of a dof on next level

 * @param theGrid - pointer to a grid
 * @param theElement - pointer to a element

   This function returns highest VNCLASS of a vector associated with the
   element.

 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX MaxNextVectorClass (GRID *theGrid, ELEMENT *theElement)
{
  INT i,m;
  VECTOR *vList[20];
  INT cnt;

  m = 0;
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,ELEMVEC))
  {
    GetVectorsOfElement(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  }
        #ifdef __THREEDIM__
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,SIDEVEC))
  {
    GetVectorsOfSides(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  }
        #endif
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,EDGEVEC))
  {
    GetVectorsOfEdges(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  }
  if (VEC_DEF_IN_OBJ_OF_GRID(theGrid,NODEVEC))
  {
    GetVectorsOfNodes(theElement,&cnt,vList);
    for (i=0; i<cnt; i++) m = MAX(m,VNCLASS(vList[i]));
  }
  return (m);
}

/****************************************************************************/
/** \brief Create a new find cut procedure in environment

 * @param name - name
 * @param FindCutProc -  the find cut procedure

   This function creates a new find cut dependency in environement.

 */
/****************************************************************************/

FIND_CUT * NS_DIM_PREFIX CreateFindCutProc (const char *name, FindCutProcPtr FindCutProc)
{
  FIND_CUT *newFindCut;

  if (ChangeEnvDir("/FindCut")==NULL)
  {
    UserWrite("cannot change to dir '/FindCut'\n");
    return(NULL);
  }
  newFindCut = (FIND_CUT *) MakeEnvItem (name,theFindCutVarID,sizeof(FIND_CUT));
  if (newFindCut==NULL) return(NULL);

  newFindCut->FindCutProc = FindCutProc;

  return (newFindCut);
}

/****************************************************************************/
/** \brief Create a new algebraic dependency in environment

 * @param name - name
 * @param DependencyProc -  the dependency procedure

   This function creates a new algebraic dependency in environement.

 */
/****************************************************************************/

ALG_DEP * NS_DIM_PREFIX CreateAlgebraicDependency (const char *name, DependencyProcPtr DependencyProc)
{
  ALG_DEP *newAlgDep;

  if (ChangeEnvDir("/Alg Dep")==NULL)
  {
    UserWrite("cannot change to dir '/Alg Dep'\n");
    return(NULL);
  }
  newAlgDep = (ALG_DEP *) MakeEnvItem (name,theAlgDepVarID,sizeof(ALG_DEP));
  if (newAlgDep==NULL) return(NULL);

  newAlgDep->DependencyProc = DependencyProc;

  return (newAlgDep);
}

/****************************************************************************/
/** \brief Put `every` vector !VCUSED into the new order list

 * @param theGrid - pointer to grid
   .  CutVectors -
   .  nb -

   This function can be used as a find-cut-procedure for the streamwise ordering
   algorithm. But it just puts `all` the remaining vectors into the new order
   list instead of removing only as much as necessary to get rid of cyclic
   dependencies.

   It is used as default when no cut procedure is specified.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static VECTOR *FeedbackVertexVectors (GRID *theGrid, VECTOR *LastVector, INT *nb)
{
  VECTOR *theVector;

  /* push all remaining vectors */
  *nb = 0;
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    if (!VCUSED(theVector))
    {
      (*nb)++;
      PREDVC(LastVector) = theVector;
      LastVector = theVector;
      SETVCUSED(theVector,1);
    }
  return (LastVector);
}


/****************************************************************************/
/** \brief Dependency function for lexicographic ordering

 * @param theGrid - pointer to grid
 * @param data - option string from orderv command

   This function defines a dependency function for lexicographic ordering.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT LexAlgDep (GRID *theGrid, const char *data)
{
  MULTIGRID *theMG;
  VECTOR *theVector,*NBVector;
  MATRIX *theMatrix;
  DOUBLE_VECTOR pos,nbpos;
  DOUBLE diff[DIM];
  INT i,order,res;
  INT Sign[DIM],Order[DIM],xused,yused,zused,error,SpecialTreatSkipVecs;
  char ord[3];

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(data,expandfmt("%2[rlud]"),ord);
        #else
  res = sscanf(data,expandfmt("%3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintErrorMessage('E',"LexAlgDep","could not read order type");
    return(1);
  }
  if (strlen(ord)!=DIM)
  {
                #ifdef __TWODIM__
    PrintErrorMessage('E',"LexAlgDep","specify 2 chars out of 'rlud'");
                #else
    PrintErrorMessage('E',"LexAlgDep","specify 3 chars out of 'rlbfud'");
                #endif
    return(1);
  }
  error = xused = yused = zused = false;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = true;
      xused = true;
      Order[i] = _X_; Sign[i] =  1; break;
    case 'l' :
      if (xused) error = true;
      xused = true;
      Order[i] = _X_; Sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'd' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'f' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] = -1; break;

    case 'u' :
      if (zused) error = true;
      zused = true;
      Order[i] = _Z_; Sign[i] =  1; break;
    case 'd' :
      if (zused) error = true;
      zused = true;
      Order[i] = _Z_; Sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintErrorMessage('E',"LexAlgDep","bad combination of 'rludr' or 'rlbfud' resp.");
    return(1);
  }

  /* treat vectors with skipflag set specially? */
  SpecialTreatSkipVecs = false;
  if              (strchr(data,'<')!=NULL)
    SpecialTreatSkipVecs = GM_PUT_AT_BEGIN;
  else if (strchr(data,'>')!=NULL)
    SpecialTreatSkipVecs = GM_PUT_AT_END;

  theMG   = MYMG(theGrid);

  /* find an approximate measure for the mesh size */
  // The following method wants the domain radius, which has been removed.
  // Dune has been setting this radius to 1.0 for years now, so I don't think it matters.
  DOUBLE BVPD_RADIUS = 1.0;
  InvMeshSize = POW2(GLEVEL(theGrid)) * pow(NN(GRID_ON_LEVEL(theMG,0)),1.0/DIM) / BVPD_RADIUS;

  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    VectorPosition(theVector,pos);

    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
    {
      NBVector = MDEST(theMatrix);

      SETMUP(theMatrix,0);
      SETMDOWN(theMatrix,0);

      if (SpecialTreatSkipVecs)
      {
        if (VECSKIP(theVector) && !VECSKIP(NBVector))
        {
          if (SpecialTreatSkipVecs==GM_PUT_AT_BEGIN)
            order = -1;
          else
            order =  1;
        }

        if (VECSKIP(NBVector) && !VECSKIP(theVector))
        {
          if (SpecialTreatSkipVecs==GM_PUT_AT_BEGIN)
            order =  1;
          else
            order = -1;
        }
      }
      else
      {
        VectorPosition(NBVector,nbpos);

        V_DIM_SUBTRACT(nbpos,pos,diff);
        V_DIM_SCALE(InvMeshSize,diff);

        if (fabs(diff[Order[DIM-1]])<ORDERRES)
        {
                                        #ifdef __THREEDIM__
          if (fabs(diff[Order[DIM-2]])<ORDERRES)
          {
            if (diff[Order[DIM-3]]>0.0) order = -Sign[DIM-3];
            else order =  Sign[DIM-3];
          }
          else
                                        #endif
          if (diff[Order[DIM-2]]>0.0) order = -Sign[DIM-2];
          else order =  Sign[DIM-2];
        }
        else
        {
          if (diff[Order[DIM-1]]>0.0) order = -Sign[DIM-1];
          else order =  Sign[DIM-1];
        }
      }
      if (order==1) SETMUP(theMatrix,1);
      else SETMDOWN(theMatrix,1);
    }
  }

  return (0);
}

/****************************************************************************/
/** \brief Dependency function for lexicographic ordering

 * @param theGrid - pointer to grid
 * @param data - option string from orderv command

   This function defines a dependency function for lexicographic ordering.

 * @return <ul>
 *   <li>   0 if ok
 *   <li>   1 if error occured.
   </ul>
 */
/****************************************************************************/

static INT StrongLexAlgDep (GRID *theGrid, const char *data)
{
  MULTIGRID *theMG;
  VECTOR *theVector,*NBVector;
  MATRIX *theMatrix;
  DOUBLE_VECTOR pos,nbpos;
  DOUBLE diff[DIM];
  INT i,order,res;
  INT Sign[DIM],Order[DIM],xused,yused,zused,error;
  char ord[3];

  /* read ordering directions */
        #ifdef __TWODIM__
  res = sscanf(data,expandfmt("%2[rlud]"),ord);
        #else
  res = sscanf(data,expandfmt("%3[rlbfud]"),ord);
        #endif
  if (res!=1)
  {
    PrintErrorMessage('E',"LexAlgDep","could not read order type");
    return(1);
  }
  if (strlen(ord)!=DIM)
  {
                #ifdef __TWODIM__
    PrintErrorMessage('E',"LexAlgDep","specify 2 chars out of 'rlud'");
                #else
    PrintErrorMessage('E',"LexAlgDep","specify 3 chars out of 'rlbfud'");
                #endif
    return(1);
  }
  error = xused = yused = zused = false;
  for (i=0; i<DIM; i++)
    switch (ord[i])
    {
    case 'r' :
      if (xused) error = true;
      xused = true;
      Order[i] = _X_; Sign[i] =  1; break;
    case 'l' :
      if (xused) error = true;
      xused = true;
      Order[i] = _X_; Sign[i] = -1; break;

                        #ifdef __TWODIM__
    case 'u' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'd' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] = -1; break;
                        #else
    case 'b' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] =  1; break;
    case 'f' :
      if (yused) error = true;
      yused = true;
      Order[i] = _Y_; Sign[i] = -1; break;

    case 'u' :
      if (zused) error = true;
      zused = true;
      Order[i] = _Z_; Sign[i] =  1; break;
    case 'd' :
      if (zused) error = true;
      zused = true;
      Order[i] = _Z_; Sign[i] = -1; break;
                        #endif
    }
  if (error)
  {
    PrintErrorMessage('E',"LexAlgDep","bad combination of 'rludr' or 'rlbfud' resp.");
    return(1);
  }

  theMG   = MYMG(theGrid);

  /* find an approximate measure for the mesh size */
  // The following method wants the domain radius, which has been removed.
  // Dune has been setting this radius to 1.0 for years now, so I don't think it matters.
  DOUBLE BVPD_RADIUS = 1.0;
  InvMeshSize = POW2(GLEVEL(theGrid)) * pow(NN(GRID_ON_LEVEL(theMG,0)),1.0/DIM);

  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    VectorPosition(theVector,pos);

    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
    {
      NBVector = MDEST(theMatrix);

      SETMUP(theMatrix,0);
      SETMDOWN(theMatrix,0);
      SETMUSED(theMatrix,0);

      VectorPosition(NBVector,nbpos);

      V_DIM_SUBTRACT(nbpos,pos,diff);
      V_DIM_SCALE(InvMeshSize,diff);

      if (fabs(diff[Order[DIM-1]])<ORDERRES)
      {
                                #ifdef __THREEDIM__
        if (fabs(diff[Order[DIM-2]])<ORDERRES)
        {
          if (diff[Order[DIM-3]]>0.0) order = -Sign[DIM-3];
          else order =  Sign[DIM-3];
        }
        else
                                #endif
        if (diff[Order[DIM-2]]>0.0) order = -Sign[DIM-2];
        else order =  Sign[DIM-2];
        SETMUSED(theMatrix,1);
      }
      else
      {
        if (diff[Order[DIM-1]]>0.0) order = -Sign[DIM-1];
        else order =  Sign[DIM-1];
      }
      switch (order)
      {
      case  1 : SETMUP(theMatrix,1); break;
      case -1 : SETMDOWN(theMatrix,1); break;
      case  0 : SETMUP(theMatrix,1); SETMDOWN(theMatrix,1); break;
      }
    }
  }
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
  {
    SETVCUSED(theVector,0);
    SETVCFLAG(theVector,0);

    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
      if (MUP(theMatrix) && !MUSED(theMatrix))
        break;
    if (theMatrix==NULL)
      SETVCUSED(theVector,1);
  }
  for (theVector=FIRSTVECTOR(theGrid); theVector!=NULL; theVector=SUCCVC(theVector))
    for (theMatrix=MNEXT(VSTART(theVector)); theMatrix!=NULL; theMatrix=MNEXT(theMatrix))
      if (MUSED(theMatrix) && MUSED(MADJ(theMatrix)))
      {
        SETMUP(theMatrix,1);
        SETMDOWN(theMatrix,1);
      }

  return (0);
}

#ifdef __BLOCK_VECTOR_DESC__
/****************************************************************************/
/** \brief Set the level-field in a blockvector-tree
 *
 * @param bv - first of the blockvectors whose levelnumber should be set
 * @param level - levelnumber of this blockvector
 *
 * The blockvectors form a hierarchical structure; thus each blockvector can
 * be assigned a 'level'. This function traverses the blockvector-tree
 * starting at the given 'bv' and sets in each blockvector the level field
 * starting with the given 'level'. In this way it is possible to number also
 * parts of the global blockvector-tree (to skip for example meaningless
 * administrative, covering blocks).
 *
 * \sa BLOCKVECTOR
 *
 */
/****************************************************************************/

void NS_DIM_PREFIX SetLevelnumberBV( BLOCKVECTOR *bv, INT level )
{
  ASSERT( level < (1<<BVLEVEL_LEN) );           /* increase BVLEVEL_LEN in gm.h */

  if ( bv == NULL )
    return;

  if ( level >= 0 )
    SETBVLEVEL(bv,level);
  else
    SETBVLEVEL(bv,0);

  if ( !BV_IS_LEAF_BV( bv ) )
    for ( bv = BVDOWNBV(bv); bv != NULL; bv = BVSUCC(bv) )
      SetLevelnumberBV( bv, level + 1 );
}


/****************************************************************************/
/** \brief Creates a stripewise decomposition of a 2D rectangular domain with a regular mesh
 *
 * @param bv_plane - handle for the blockvector covering the plane
 * @param bvd_plane - blockvector description for the block 'bv_plane'
 * @param bvdf - blockvector description format for 'bvd_plane'
 * @param v - input: handle to the first vector; output: handle to the 'last' vector in this plane
 * @param stripes  - number of stripes to be constructed
 * @param vectors_per_stripe - number of vectors a stripe should contain
 * @param grid - the grid containing the vectors to be structured
 *
   From the list of vectors the blockvector-list is generated in
   the following way:
   First a blockvector 'bv_plane' corresponding to 'bvd_plane' is constructed
   which will cover all the blockvectors for the single stripes and
   thus all the vectors of the plane. Beginning
   at the start of the vector-list ('v'), blocks are constructed containing
   'vectors_per_stripe' consecutive vectors. The start of the new
   blockvector-list is anchored in the blockvector 'bv_plane'.
   This blockvectors are
   numbered beginning from 0 along the construction. If the vector-list
   is not long enough, the function will not be troubled but construct
   as far as possible the structure and GM_INCONSISTENCY is returned.

   REQUIREMENTS:
   The macro '__BLOCK_VECTOR_DESC__' must be defined in 'gm.h' to enable
   blockvector descriptions.

   REMARK:
   This is only an auxiliary function. In the returned 'bv_plane' the
   components BVSUCC, BVPRED, BVNUMBER are not yet set.

   APPLICATION:
   If the vector-list is ordered lexicographic, each line resp. column
   containing 'vectors_per_stripe' vectors, then this function constructs
   the blockvector structure belonging to a
   `linewise domain decomposition`.

   If 'vectors_per_stripe' is a natural multiple of the actual linewidth
   (resp. columnlength), this function constructs the blockvector
   structure belonging to a `stripewise domain decomposition`, each stripe
   consisting of 'vectors_per_stripe'/linewidth lines (resp. columns).

 * @return <ul>
 *   <li>GM_OK if ok </li>
 *   <li>GM_OUT_OF_MEM if there was not enough memory to allocate all blockvectors </li>
 *   <li>GM_INCONSISTENCY if the vector-list was too short </li>
   </ul>
 * \sa
   BLOCKVECTOR, CreateBVStripe2D, CreateBVStripe3D, CreateBVDomainHalfening

 */
/****************************************************************************/

static INT CreateBVPlane( BLOCKVECTOR **bv_plane, const BV_DESC *bvd_plane, const BV_DESC_FORMAT *bvdf, VECTOR **v, INT stripes, INT vectors_per_stripe, GRID *grid )
{
  BLOCKVECTOR *bv;
  BLOCKVECTOR *prev;
  INT i, j;
  VECTOR *pred_v;

  (void)CreateBlockvector( grid, bv_plane );
  if ( *bv_plane == NULL )
    return GM_OUT_OF_MEM;

  SETBVDOWNTYPE( *bv_plane, BVDOWNTYPEBV );             /* further blocks follow */
  SETBVTVTYPE( *bv_plane, BV1DTV );
  SETBVORIENTATION( *bv_plane, BVNOORIENTATION );
  BVFIRSTVECTOR( *bv_plane ) = *v;

  /* construct for each stripe a blockvector */
  for ( i = 0; (i < stripes) && (*v != NULL); i++ )
  {
    (void)CreateBlockvector( grid, &bv );

    if ( i == 0 )
    {
      BVDOWNBV( *bv_plane ) = bv;                   /* anchor stripe-blocks in the block of the plane */
      if ( bv == NULL )
        return GM_OUT_OF_MEM;
      BVPRED( bv ) = NULL;
    }
    else
    {
      BVSUCC( prev ) = bv;                              /* continue blockvector list */
      if ( bv == NULL )
        return GM_OUT_OF_MEM;
      BVPRED( bv ) = prev;
    }

    prev = bv;                                                          /* prepare for the next loop */

    SETBVDOWNTYPE( bv, BVDOWNTYPEVECTOR );              /* block is at last block level */
    SETBVTVTYPE( bv, BV1DTV );
    SETBVORIENTATION( bv, BVHORIZONTAL );
    BVNUMBER( bv ) = i;

    /* let the blockvector point to the vector */
    BVDOWNVECTOR( bv ) = *v;
    BVFIRSTVECTOR( bv ) = *v;

    /* find successor of the last vector of this blockvector and store it;
       update the blockvector description for all vectors of this
       blockvector
     */
    for ( j = vectors_per_stripe; (j > 0) && (*v != NULL); j-- )
    {
      VBVD( *v ) = *bvd_plane;
      BVD_PUSH_ENTRY( &VBVD( *v ), i, bvdf );
      pred_v = *v;
      *v = SUCCVC( *v );
    }
    BVLASTVECTOR( bv ) = pred_v;
    BVNUMBEROFVECTORS( bv ) = vectors_per_stripe - j;
  }
  BVSUCC( bv ) = NULL;                          /* end of the blockvector list */

  *v = pred_v;
  BVLASTVECTOR( *bv_plane ) = *v;
  BVDOWNBVLAST( *bv_plane ) = bv;
  BVNUMBEROFVECTORS( *bv_plane ) = stripes * vectors_per_stripe - j;

  /* the for loop exited premature because of v == NULL */
  if ( (i < stripes) || (j != 0) )
    return GM_INCONSISTENCY;

  return GM_OK;
}
#endif /* __BLOCK_VECTOR_DESC__ */


/****************************************************************************/
/** \brief Creates a stripewise domain decomposition of a 2D rectangular domain with a regular mesh
 *
 * @param grid - the grid containing the vectors to be structured
 * @param vectors  - number of vectors, i.e. gridpoints, in the inner
 * @param vectors_per_stripe - number of vectors a stripe should contain
 *
   From the list of vectors the blockvector-list is generated in
   the following way:
   The first blockvector level consists of 2 blockvectors: the first
   covering all inner vectors (number 0), the second all boundary vectors
   (number 1). Beginning
   at the start of the vector-list, blocks are constructed containing
   'vectors_per_stripe' consecutive vectors. The start of the new
   blockvector-list is anchored in the blockvector for the inner vectors.
   This blockvectors are
   numbered beginning from 0 along the construction. If the vector-list
   is shorter than 'vectors', the function will not be troubled
   and GM_INCONSISTENCY is returned. If the vector-list
   is longer than 'vectors', these vectors are covered in a separate
   blockvector with number 1 (this overlapping vectors might be for
   example dirichlet boundary vectors).

   For the blockvectors in the tree with root 'all inner' the level-field is
   set beginning with 0.

   REQUIREMENTS:
   The macro '__BLOCK_VECTOR_DESC__' must be defined in 'gm.h' to enable
   blockvector descriptions.

   APPLICATION:
   If the vector-list is ordered lexicographic, each line resp. column
   containing 'vectors_per_stripe' vectors, then this function constructs
   the blockvector structure belonging to a
   `linewise domain decomposition`.

   If 'vectors_per_stripe' is a natural multiple of the actual linewidth
   (resp. columnlength), this function constructs the blockvector
   structure belonging to a `stripewise domain decomposition`, each stripe
   consisting of 'vectors_per_stripe'/linewidth lines (resp. columns).

 * @return <ul>

 *   <li>GM_OK if ok </li>
 *   <li>GM_OUT_OF_MEM if there was not enough memory to allocate all blockvectors </li>
 *   <li>GM_INCONSISTENCY if the vector-list was too short </li>
   </ul>
 * \sa
   BLOCKVECTOR, CreateBVStripe3D, CreateBVDomainHalfening

 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateBVStripe2D( GRID *grid, INT vectors, INT vectors_per_stripe )
{
#ifdef __BLOCK_VECTOR_DESC__
  BLOCKVECTOR *bv_inner, *bv_boundary;
  VECTOR *v;
  INT nr_blocks, ret;
  BV_DESC bvd;

  if ( GFIRSTBV( grid ) != NULL )
    FreeAllBV( grid );

  /* number of blockvectors to be constructed */
  nr_blocks = ( vectors + vectors_per_stripe - 1) / vectors_per_stripe;

  BVD_INIT( &bvd );
  BVD_PUSH_ENTRY( &bvd, 0, &two_level_bvdf );
  v = FIRSTVECTOR( grid );
  ret = CreateBVPlane( &bv_inner, &bvd, &two_level_bvdf, &v, nr_blocks, vectors_per_stripe, grid );
  if ( ret != GM_OK )
  {
    FreeBVList( grid, bv_inner );
    return ret;
  }
  v = SUCCVC( v );

  (void)CreateBlockvector( grid, &bv_boundary );
  if ( bv_boundary == NULL )
  {
    FreeBVList( grid, bv_inner );
    return GM_OUT_OF_MEM;
  }

  GFIRSTBV( grid ) = bv_inner;          /* anchor blockvector list in the grid */
  GLASTBV( grid ) = bv_boundary;

  BVPRED( bv_inner ) = NULL;
  BVSUCC( bv_inner ) = bv_boundary;
  BVNUMBER( bv_inner ) = 0;

  BVPRED( bv_boundary ) = bv_inner;
  BVSUCC( bv_boundary ) = NULL;
  BVNUMBER( bv_boundary ) = 1;
  SETBVDOWNTYPE( bv_boundary, BVDOWNTYPEVECTOR );       /* block is at last block level */
  SETBVTVTYPE( bv_boundary, BV1DTV );
  SETBVORIENTATION( bv_boundary, BVNOORIENTATION );
  BVNUMBEROFVECTORS( bv_boundary ) = NVEC( grid ) - BVNUMBEROFVECTORS( bv_inner );
  BVFIRSTVECTOR( bv_boundary ) = v;
  BVLASTVECTOR( bv_boundary ) = LASTVECTOR( grid );

  /* complete the boundary vector list */
  BVD_INC_LAST_ENTRY( &bvd, 1, &two_level_bvdf );
  for ( ; v != NULL; v = SUCCVC( v ) )
    VBVD( v ) = bvd;

  SetLevelnumberBV( bv_inner, 0 );
  return GM_OK;
#else
  return(1);
#endif /* __BLOCK_VECTOR_DESC__ */
}


/****************************************************************************/
/** \brief Creates a plane- and stripewise domain decomposition of a 3D rectangular domain with a regular mesh

 * @param grid - the grid containing the vectors to be structured
 * @param inner_vectors  - number of vectors, i.e. gridpoints, in the inner
 * @param stripes_per_plane - number of stripes forming a plane
 * @param vectors_per_stripe - number of vectors a stripe should contain

   From the list of vectors the blockvector-list is generated in
   the following way:
   The first blockvector level consists of 2 blockvectors: the first
   covering all inner vectors (number 0), the second all other vectors
   (number 1). Beginning
   at the start of the vector-list, plane-blockvectors are constructed: each
   plane-blockvector consists of a blockvector list of 'stripes_per_plane'
   stripe-blockvectors and each
   stripe-blockvector contains 'vectors_per_stripe' vectors. Plane-blocks are
   constructed as long as not 'inner_vectors' vectors are processed
   or all vectors in the grid are processed. The start of the new
   plane-blockvector-list is anchored in the blockvector for the
   inner vectors. The blockvectors in each list are
   numbered beginning from 0 along the construction. If the vector-list
   is shorter than 'inner_vectors', the function will not be troubled
   and GM_INCONSISTENCY is returned. If the vector-list
   is longer than 'vectors', these vectors are covered in a separate
   blockvector with number 1 (this overlapping vectors might be for
   example dirichlet boundary vectors).

   REQUIREMENTS:
   The macro '__BLOCK_VECTOR_DESC__' must be defined in 'gm.h' to enable
   blockvector descriptions.

   APPLICATION:
   If the vector-list is ordered lexicographic and each line contains
   'vectors_per_stripe' vectors and each plane contains 'stripes_per_plane'
   lines, then this function constructs the blockvector structure belonging
   to a `plane- and linewise domain decomposition`.

 * @return <ul>
 *   <li>GM_OK if ok </li>
 *   <li>GM_OUT_OF_MEM if there was not enough memory to allocate all blockvectors </li>
 *   <li>GM_INCONSISTENCY if the vector-list was too short </li>
 * </ul>
 * \sa
   BLOCKVECTOR, CreateBVStripe2D, CreateBVDomainHalfening

 */
/****************************************************************************/

INT NS_DIM_PREFIX CreateBVStripe3D( GRID *grid, INT inner_vectors, INT stripes_per_plane, INT vectors_per_stripe )
{
#ifdef __BLOCK_VECTOR_DESC__
  BLOCKVECTOR *bv_inner, *bv_boundary, *bv_plane, *prev;
  VECTOR *v;
  INT i, nr_planes, nr_vectors, ret;
  BV_DESC bvd;

  if ( GFIRSTBV( grid ) != NULL )
    FreeAllBV( grid );

  /* number of planes to be constructed */
  nr_planes = ( inner_vectors + stripes_per_plane*vectors_per_stripe - 1) / (stripes_per_plane*vectors_per_stripe);

  v = FIRSTVECTOR( grid );

  (void)CreateBlockvector( grid, &bv_inner );
  if ( bv_inner == NULL )
    return GM_OUT_OF_MEM;

  (void)CreateBlockvector( grid, &bv_boundary );
  if ( bv_boundary == NULL )
  {
    DisposeBlockvector( grid, bv_inner );
    return GM_OUT_OF_MEM;
  }

  GFIRSTBV( grid ) = bv_inner;          /* anchor blockvector list in the grid */
  GLASTBV( grid ) = bv_boundary;

  BVPRED( bv_inner ) = NULL;
  BVSUCC( bv_inner ) = bv_boundary;
  BVNUMBER( bv_inner ) = 0;
  SETBVDOWNTYPE( bv_inner, BVDOWNTYPEBV );              /* block is parent for all planes */
  SETBVTVTYPE( bv_inner, BV1DTV );
  SETBVORIENTATION( bv_inner, BVNOORIENTATION );
  BVFIRSTVECTOR( bv_inner ) = v;

  BVPRED( bv_boundary ) = bv_inner;
  BVSUCC( bv_boundary ) = NULL;
  BVNUMBER( bv_boundary ) = 1;
  SETBVDOWNTYPE( bv_boundary, BVDOWNTYPEVECTOR );       /* block is at last block level */
  SETBVTVTYPE( bv_boundary, BV1DTV );
  SETBVORIENTATION( bv_boundary, BVNOORIENTATION );

  /* construct for each plane a blockvector-tree */
  BVD_INIT( &bvd );
  BVD_PUSH_ENTRY( &bvd, 0, &three_level_bvdf );         /* inner vectors */
  BVD_PUSH_ENTRY( &bvd, 0, &three_level_bvdf );         /* 1. plane */
  nr_vectors = 0;
  for ( i = 0; (i < nr_planes) && (v != NULL); i++ )
  {
    if ( i != 0 )
      v = SUCCVC( v );                   /* set v from the last vector of the previous block to the first of the current block */

    ret = CreateBVPlane( &bv_plane, &bvd, &three_level_bvdf, &v, stripes_per_plane, vectors_per_stripe, grid );
    SETBVTVTYPE( bv_plane, BV2DTV );                    /* use tensor product testvector (full 2D testvector) */
    if ( ret == GM_OUT_OF_MEM )
    {
      FreeAllBV( grid );
      return ret;
    }

    if ( i == 0 )
    {
      BVDOWNBV( bv_inner ) = bv_plane;                   /* anchor stripe-blocks in the block of the plane */
      BVPRED( bv_plane ) = NULL;
    }
    else
    {
      BVSUCC( prev ) = bv_plane;                                /* continue blockvector list */
      BVPRED( bv_plane ) = prev;
    }

    prev = bv_plane;                                                    /* prepare for the next loop */

    BVNUMBER( bv_plane ) = i;
    nr_vectors += BVNUMBEROFVECTORS( bv_plane );

    BVD_INC_LAST_ENTRY( &bvd, 1, &three_level_bvdf );
  }
  BVSUCC( bv_plane ) = NULL;                            /* end of the blockvector list */

  BVLASTVECTOR( bv_inner ) = v;
  BVDOWNBVLAST( bv_inner ) = bv_plane;
  BVNUMBEROFVECTORS( bv_inner ) = nr_vectors;

  v = SUCCVC( v );
  BVFIRSTVECTOR( bv_boundary ) = v;
  BVLASTVECTOR( bv_boundary ) = LASTVECTOR( grid );
  BVNUMBEROFVECTORS( bv_boundary ) = NVEC( grid ) - BVNUMBEROFVECTORS( bv_inner );

  /* complete the boundary vector list */
  BVD_INIT( &bvd );
  BVD_PUSH_ENTRY( &bvd, 1, &three_level_bvdf );         /* boundary vectors */
  for ( ; v != NULL; v = SUCCVC( v ) )
    VBVD( v ) = bvd;

  if ( ret != GM_OK )
    return ret;

  if ( BVNUMBEROFVECTORS(bv_inner) != nr_planes*stripes_per_plane*vectors_per_stripe )
    return GM_INCONSISTENCY;

  SetLevelnumberBV( bv_inner, 0 );

  return GM_OK;

#else
  return(1);
#endif /* __BLOCK_VECTOR_DESC__ */
}


/****************************************************************************/
/** \brief Creates a recursive domain halfening decomposition

 * @param grid - the grid containing the vectors to be structured
 * @param side - number of points on the side of the quadratic mesh

   The grid must be similar to a quadratic mesh. The vectors must be numbered
   linewise lexicographic. Then this function constructs a hierarchy
   of blockvectors describing a recursive domain halfening.

   A quadratic domain is halfened by cutting along a vertical line. The
   points on this line are gathered into subdomain with number 2. The points
   left to this interface-line are collected in subdomain 0, the right ones
   into subdomain 1. The subdomains are listed according to their numbers.
   Each of the 2 resulting subdomains 0 and 1 are now halfened horizontally
   and processed analogical. The halfening is recursively proceeded until
   a subdomain consists of less than 10 points. The initial grid should have
   a side length allowing all halfenings without a remainder.

   The first blockvector in the hierarchy is covering all the vectors.

   NOTE:
   If the grid contains Dirichlet vectors, they must be hidden from this
   routine; e.g. order the whole vector list such that the Dirichlet vectors
   are at the end of the list; then let the LASTVECTOR of the grid be the
   vector in front of the first Dirichlet vector and correct this temporarly
   setting after returning from this routine.

 * @return <ul>
 *   <li> GM_OK if ok </li>
 *   <li> GM_OUT_OF_MEM if there is not enough memory to allocate the blockvectors </li>
 **</ul>
 * \sa
   BLOCKVECTOR, CreateBVStripe

 */
/****************************************************************************/

#ifndef ModelP
INT NS_DIM_PREFIX CreateBVDomainHalfening( GRID *grid, INT side, INT leaf_size )
{
#ifdef __BLOCK_VECTOR_DESC__
  BLOCKVECTOR *bv;
  INT ret;
  VECTOR *v, *end_v;

  if ( GFIRSTBV( grid ) != NULL )
    FreeAllBV( grid );

  /* create first block of the hierarchy */
  if ( CreateBlockvector( grid, &bv ) != GM_OK )
    return GM_OUT_OF_MEM;

  GFIRSTBV( grid ) = bv;
  GLASTBV( grid ) = bv;
  SETBVDOWNTYPE( bv, BVDOWNTYPEVECTOR );
  SETBVTVTYPE( bv, BV1DTV );
  BVDOWNVECTOR( bv ) = FIRSTVECTOR( grid );
  BVPRED( bv ) = NULL;
  BVSUCC( bv ) = NULL;
  BVNUMBER( bv ) = 0;
  BVLASTVECTOR( bv ) = LASTVECTOR( grid );
  BVFIRSTVECTOR( bv ) = FIRSTVECTOR( grid );

  /* set this global blockvector as the first stage of the hierarchy */
  end_v = SUCCVC(LASTVECTOR( grid ));
    #ifdef __BLOCK_VECTOR_DESC__
  for ( v = FIRSTVECTOR( grid ); v != end_v; v = SUCCVC( v ) )
    BVD_PUSH_ENTRY( &VBVD( v ), 0, &DH_bvdf );
    #endif

  if ( (ret = BlockHalfening( grid, bv, 0, 0, side, side, side, BV_VERTICAL, leaf_size )) != GM_OK )
  {
    FreeAllBV( grid );
    return ret;
  }

  /* set the number of covered VECTORS */
  BVNUMBEROFVECTORS( bv ) = BVNUMBEROFVECTORS( BVDOWNBV(bv) ) +
                            BVNUMBEROFVECTORS( BVSUCC(BVDOWNBV(bv)) ) +
                            BVNUMBEROFVECTORS( BVDOWNBVLAST(bv) );

  /* set FIRST- and LASTVECTOR */
  FIRSTVECTOR( grid ) = BVFIRSTVECTOR( bv );
  LASTVECTOR( grid ) = BVLASTVECTOR( bv );

  return GM_OK;
#else
  return(1);
#endif /* __BLOCK_VECTOR_DESC__ */
}
#endif /* ModelP */

#ifdef __BLOCK_VECTOR_DESC__
/****************************************************************************/
/** \brief Internal function to perform the work of CreateBVDomainHalfening

 * @param grid - the grid containing the vectors to be structured
 * @param bv - blockvector to be subdivided
 * @param left - left side of the rectangle representing the 'bv' in the virtual mesh
 * @param bottom - bottom side of the rectangle representing the 'bv' in the virtual mesh
 * @param width -  sidewidth of the rectangle representing the 'bv' in the virtual mesh
 * @param height - sideheight of the rectangle representing the 'bv' in the virtual mesh
 * @param side - sidelength of the original square
 * @param orientation - gives the orientation of the cutting line (horizontal or vertical)

   Performs the work described for the function CreateBVDomainHalfening
   above.

   Consider a virtual equidistant mesh with meshwidth 1 in both directions.
   Together with the linewise lexicographic numbering of the vectors you can
   easily calculate the geometric position of each vector according to his
   number in this mesh. In this way it is easy to determine to which
   subdomain a given vector belongs. The "geometric" information to this
   function are related to this virtual mesh.

 * @return <ul>
 *   <li> GM_OK if ok </li>
 *   <li> GM_OUT_OF_MEM if there is not enough memory to allocate the blockvectors </li>
 * </ul>
 */
/****************************************************************************/

static INT BlockHalfening( GRID *grid, BLOCKVECTOR *bv, INT left, INT bottom, INT width, INT height, INT side, INT orientation, INT leaf_size )
{
  VECTOR *v, *vpred, *end_v, **v0, **v1, **vi;
  BLOCKVECTOR *bv0, *bv1, *bvi;
  INT index, interface, nr_0, nr_1, nr_i;
  VECTOR *pred_first_vec;

    #ifndef __BLOCK_VECTOR_DESC__
  return(1);
    #endif

  /* create a double linked list of 3 blockvectors */
  v = BVFIRSTVECTOR( bv );
  end_v = BVENDVECTOR( bv );
  pred_first_vec = PREDVC( v );

  if ( CreateBlockvector( grid, &bv0 ) != GM_OK )
    return GM_OUT_OF_MEM;

  BVPRED( bv0 ) = NULL;
  SETBVDOWNTYPE( bv0, BVDOWNTYPEVECTOR );
  SETBVTVTYPE( bv0, BV1DTV );
  BVNUMBER( bv0 ) = 0;
  v0 = &BVDOWNVECTOR( bv0 );

  if ( CreateBlockvector( grid, &bv1 ) != GM_OK )
  {
    DisposeBlockvector( grid, bv0 );
    return GM_OUT_OF_MEM;
  }

  BVSUCC( bv0 ) = bv1;
  BVPRED( bv1 ) = bv0;
  SETBVDOWNTYPE( bv1, BVDOWNTYPEVECTOR );
  SETBVTVTYPE( bv1, BV1DTV );
  BVNUMBER( bv1 ) = 1;
  v1 = &BVDOWNVECTOR( bv1 );

  if ( CreateBlockvector( grid, &bvi ) != GM_OK )
  {
    DisposeBlockvector( grid, bv1 );
    DisposeBlockvector( grid, bv0 );
    return GM_OUT_OF_MEM;
  }

  BVSUCC( bv1 ) = bvi;
  BVPRED( bvi ) = bv1;
  SETBVDOWNTYPE( bvi, BVDOWNTYPEVECTOR );
  SETBVTVTYPE( bvi, BV1DTV );
  BVNUMBER( bvi ) = 2;
  BVSUCC( bvi ) = NULL;
  vi = &BVDOWNVECTOR( bvi );

  /* all necessary memory allocated; insert new block list in father block */
  SETBVDOWNTYPE( bv, BVDOWNTYPEBV );
  SETBVTVTYPE( bv, BV1DTV );
  BVDOWNBV( bv ) = bv0;
  BVDOWNBVLAST( bv ) = bvi;

  /* coordinate of the interface line */
  interface = ( (orientation == BV_VERTICAL) ? (width-1)/2 + left : bottom + (height-1)/2 );

  /* sort each vector in one of the 3 subdomains */
  nr_0 = nr_1 = nr_i = 0;
  for ( vpred = NULL; v != end_v; vpred = v, v = SUCCVC( v ) )
  {
    /* calculate row/column number from index */
    index = ( (orientation == BV_VERTICAL) ? VINDEX(v) % side : VINDEX(v) / side );

    if ( index < interface )
    {
      /* vector belongs to subdomain 0 */
      *v0 = v;
      v0 = &SUCCVC( v );
                        #ifdef __BLOCK_VECTOR_DESC__
      BVD_PUSH_ENTRY( &VBVD( v ), 0, &DH_bvdf );
            #endif
      nr_0++;
    }
    else if ( index > interface )
    {
      /* vector belongs to subdomain 1 */
      *v1 = v;
      v1 = &SUCCVC( v );
                        #ifdef __BLOCK_VECTOR_DESC__
      BVD_PUSH_ENTRY( &VBVD( v ), 1, &DH_bvdf );
            #endif
      nr_1++;
    }
    else
    {
      /* vector belongs to subdomain 2 (interface) */
      *vi = v;
      vi = &SUCCVC( v );
                        #ifdef __BLOCK_VECTOR_DESC__
      BVD_PUSH_ENTRY( &VBVD( v ), 2, &DH_bvdf );
            #endif
      nr_i++;
    }

  }

  /* relink the 3 subdomain lists to the global vector list */
  *v0 = BVDOWNVECTOR( bv1 );
  *v1 = BVDOWNVECTOR( bvi );
  *vi = end_v;
  v = BVDOWNVECTOR( bv0 );
  vpred = pred_first_vec;
  if ( pred_first_vec != NULL )
    SUCCVC( pred_first_vec ) = v;
  /* correct the PRED-pointer chain */
  for ( ; v != end_v; vpred = v, v = SUCCVC( v ) )
    PREDVC( v ) = vpred;
  if ( v != NULL )              /* at the end of the vectors of this blockvector */
    PREDVC( v ) = vpred;

  BVNUMBEROFVECTORS( bv0 ) = nr_0;
  BVNUMBEROFVECTORS( bv1 ) = nr_1;
  BVNUMBEROFVECTORS( bvi ) = nr_i;

  /* complete the relation between blockvectors and vectors */
  BVLASTVECTOR( bv0 ) = BVPRED(BVDOWNVECTOR( bv1 ));
  BVLASTVECTOR( bv1 ) = BVPRED(BVDOWNVECTOR( bvi ));
  BVLASTVECTOR( bvi ) = vpred;
  BVLASTVECTOR( bv ) = vpred;

  BVFIRSTVECTOR( bv0 ) = BVDOWNVECTOR( bv0 );
  BVFIRSTVECTOR( bv1 ) = BVDOWNVECTOR( bv1 );
  BVFIRSTVECTOR( bvi ) = BVDOWNVECTOR( bvi );
  BVFIRSTVECTOR( bv ) = BVFIRSTVECTOR( bv0 );

  /* next loop */

  if ( orientation == BV_VERTICAL )
  {
    /* new dimensions */
    width = ( width - 1 ) / 2;

    /* if there are too many points per subdomain halfen them */
    if ( ( width * height ) > leaf_size )
    {
      if ( BlockHalfening( grid, bv0, left, bottom, width, height, side, BV_HORIZONTAL, leaf_size ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
      if ( BlockHalfening( grid, bv1, left+width+1, bottom, width, height, side, BV_HORIZONTAL, leaf_size ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
    }
  }
  else       /* orientation == BV_HORIZONTAL */
  {
    /* new dimensions */
    height = ( height - 1 ) / 2;

    /* if there are too many points per subdomain halfen them */
    if ( ( width * height ) > leaf_size )
    {
      if ( BlockHalfening( grid, bv0, left, bottom, width, height, side, BV_VERTICAL, leaf_size ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
      if ( BlockHalfening( grid, bv1, left, bottom +height+1, width, height, side, BV_VERTICAL, leaf_size ) == GM_OUT_OF_MEM )
        return GM_OUT_OF_MEM;
    }
  }

  return GM_OK;
}
#endif /* __BLOCK_VECTOR_DESC__ */


/****************************************************************************/
/** \brief Move vector within the vector list of the grid

 * @param theGrid - pointer to grid
 * @param moveVector - vector to move
 * @param destVector - vector before or after which 'moveVector' will be inserted
 * @param after - true (1) or false (0)

   This function moves a 'VECTOR' within the double linked list. If 'after' is
   true then 'moveVector' will be inserted immediately after 'destVector', if
   'after' is false then it will be inserted immediately before. If 'destVector'
   is 'NULL' then 'moveVector' is inserted at the beginning when 'after' is true
   and at the end of the list when 'after' is false.

 * @return <ul>
 *   <li>    0 if ok </li>
 *   <li>    1 if error occured. </li>
 * </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX MoveVector (GRID *theGrid, VECTOR *moveVector, VECTOR *destVector, INT after)
{
  if (theGrid==NULL || moveVector==NULL) return (1);
  if (moveVector==destVector) return (0);

  /* take vector out of list */
  if (PREDVC(moveVector)!=NULL) SUCCVC(PREDVC(moveVector))      = SUCCVC(moveVector);
  else SFIRSTVECTOR(theGrid)           = SUCCVC(moveVector);
  if (SUCCVC(moveVector)!=NULL) PREDVC(SUCCVC(moveVector))      = PREDVC(moveVector);
  else LASTVECTOR(theGrid)             = PREDVC(moveVector);

  /* put it in list */
  if (destVector!=NULL)
  {
    if (after)
    {
      if (SUCCVC(destVector)!=NULL) PREDVC(SUCCVC(destVector))      = moveVector;
      else LASTVECTOR(theGrid)             = moveVector;
      SUCCVC(moveVector) = SUCCVC(destVector);
      PREDVC(moveVector) = destVector;
      SUCCVC(destVector) = moveVector;
    }
    else
    {
      if (PREDVC(destVector)!=NULL) SUCCVC(PREDVC(destVector))      = moveVector;
      else SFIRSTVECTOR(theGrid)           = moveVector;
      PREDVC(moveVector) = PREDVC(destVector);
      SUCCVC(moveVector) = destVector;
      PREDVC(destVector) = moveVector;
    }
  }
  else
  {
    if (after)
    {
      SUCCVC(moveVector) = SFIRSTVECTOR(theGrid);
      PREDVC(moveVector) = NULL;
      SFIRSTVECTOR(theGrid) = moveVector;
      if (SUCCVC(moveVector)!=NULL) PREDVC(SUCCVC(moveVector)) = moveVector;
    }
    else
    {
      SUCCVC(moveVector) = NULL;
      PREDVC(moveVector) = LASTVECTOR(theGrid);
      LASTVECTOR(theGrid) = moveVector;
      if (PREDVC(moveVector)!=NULL) SUCCVC(PREDVC(moveVector)) = moveVector;
    }
  }

  return (0);
}

/****************************************************************************/
/** \brief Init algebra
 *
 * This function inits algebra.
 *
 * @return <ul>
 *   <li>   0 if ok </li>
 *   <li>   1 if error occured. </li>
 * </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitAlgebra (void)
{
  INT i;

  /* install the /Alg Dep directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not changedir to root");
    return(__LINE__);
  }
  theAlgDepDirID = GetNewEnvDirID();
  if (MakeEnvItem("Alg Dep",theAlgDepDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not install '/Alg Dep' dir");
    return(__LINE__);
  }
  theAlgDepVarID = GetNewEnvVarID();

  /* install the /FindCut directory */
  if (ChangeEnvDir("/")==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not changedir to root");
    return(__LINE__);
  }
  theFindCutDirID = GetNewEnvDirID();
  if (MakeEnvItem("FindCut",theFindCutDirID,sizeof(ENVDIR))==NULL)
  {
    PrintErrorMessage('F',"InitAlgebra","could not install '/FindCut' dir");
    return(__LINE__);
  }
  theFindCutVarID = GetNewEnvVarID();

  /* init standard algebraic dependencies */
  if (CreateAlgebraicDependency ("lex",LexAlgDep)==NULL) return(__LINE__);
  if (CreateAlgebraicDependency ("stronglex",StrongLexAlgDep)==NULL) return(__LINE__);

  /* init default find cut proc */
  if (CreateFindCutProc ("lex",FeedbackVertexVectors)==NULL) return(__LINE__);

  for (i=0; i<MAXVOBJECTS; i++)
    switch (i)
    {
    case NODEVEC : ObjTypeName[i] = "nd"; break;
    case EDGEVEC : ObjTypeName[i] = "ed"; break;
    case ELEMVEC : ObjTypeName[i] = "el"; break;
    case SIDEVEC : ObjTypeName[i] = "si"; break;
    default : ObjTypeName[i] = "";
    }

  return (0);
}


/*@}*/
