// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cw.c                                                          */
/*                                                                          */
/* Purpose:   define global array with predefined control word entries      */
/*                                                                          */
/* Author:    Peter Bastian                                                 */
/*            Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 368                                       */
/*            6900 Heidelberg                                               */
/* Internet:  bastian@iwr1.iwr.uni-heidelberg.de                            */
/*                                                                          */
/* History:   11.01.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

#include <config.h>
#include <cstring>
#include <cstdio>

/* define this to exclude extern definition of global arrays */
#define __COMPILE_CW__


#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/low/architecture.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/misc.h>

#include "algebra.h"
#include "refine.h"
#include "cw.h"

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

#define CW_EDOBJ                (BITWISE_TYPE(EDOBJ) | BITWISE_TYPE(LIOBJ))
/* take both edges and links in one	        */
#define CW_GROBJ                BITWISE_TYPE(GROBJ)
#define CW_MGOBJ                BITWISE_TYPE(MGOBJ)
#define CW_NDOBJ                BITWISE_TYPE(NDOBJ)
#define CW_VEOBJ                BITWISE_TYPE(VEOBJ)

#define CW_VXOBJS               (BITWISE_TYPE(IVOBJ) | BITWISE_TYPE(BVOBJ))
#define CW_ELOBJS               (BITWISE_TYPE(IEOBJ) | BITWISE_TYPE(BEOBJ))
#define CW_GEOMOBJS             (CW_VXOBJS | CW_ELOBJS | CW_NDOBJ | CW_EDOBJ | CW_GROBJ)
/* NOTE: CW_GEOMOBJS and GEOM_OBJECTS differ*/

/** @name status of control word */
/*@{*/
#define CW_FREE                                         0
#define CW_USED                                         1
/*@}*/


/** @name Status of control entry */
/*@{*/
#define CE_FREE                                         0
#define CE_USED                                         1
#define CE_LOCKED                                       1
/*@}*/

/** @name Initializer macros for control entry and word predefines */
/*@{*/
#define CW_INIT(used,cw,objs)                           {used, STR(cw), cw ## CW, cw ## OFFSET,objs}
#define CW_INIT_UNUSED                                  {CW_FREE,0,0,0}
#define CE_INIT(mode,cw,ce,objs)                        {mode, STR(ce), cw ## CW, ce ## CE, ce ## SHIFT, ce ## LEN, objs}
#define CE_INIT_UNUSED                                  {CE_FREE, 0, 0, 0, 0, 0, 0}
/*@}*/



/****************************************************************************/
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/** \brief Description of a control word predefines */
typedef struct {
  INT used;             /**< Used this entry					*/
  const char *name;          /**< Name string						*/
  INT control_word_id;          /**< Index in control_words			*/
  UINT offset_in_object ;       /**< Where in object is it ?			*/
  INT objt_used;                                /**< Bitwise object ID */
} CONTROL_WORD_PREDEF;

/** \brief Description of a control entry predefines */
typedef struct {
  INT used;                  /**< Used this entry					*/
  const char *name;                  /**< Name string						*/
  INT control_word;          /**< Index of corresp. controlword	*/
  INT control_entry_id;      /**< Index in control_entries              */
  INT offset_in_word;        /**< Shift in control word			*/
  INT length;                /**< Number of bits used				*/
  INT objt_used;             /**< Bitwise object ID				*/
} CONTROL_ENTRY_PREDEF;

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

constexpr INT MAX_CONTROL_WORDS = 11;
CONTROL_WORD NS_DIM_PREFIX control_words[MAX_CONTROL_WORDS];
CONTROL_ENTRY NS_DIM_PREFIX control_entries[MAX_CONTROL_ENTRIES];

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static CONTROL_WORD_PREDEF cw_predefines[MAX_CONTROL_WORDS] = {
  CW_INIT(CW_USED,VECTOR_,                        CW_VEOBJ),
  CW_INIT(CW_USED,VERTEX_,                        CW_VXOBJS),
  CW_INIT(CW_USED,NODE_,                          CW_NDOBJ),
  CW_INIT(CW_USED,LINK_,                          CW_EDOBJ),
  CW_INIT(CW_USED,EDGE_,                          CW_EDOBJ),
  CW_INIT(CW_USED,ELEMENT_,                       CW_ELOBJS),
  CW_INIT(CW_USED,FLAG_,                          CW_ELOBJS),
  CW_INIT(CW_USED,PROPERTY_,                      CW_ELOBJS),
  CW_INIT(CW_USED,GRID_,                          CW_GROBJ),
  CW_INIT(CW_USED,GRID_STATUS_,           CW_GROBJ),
  CW_INIT(CW_USED,MULTIGRID_STATUS_,      CW_MGOBJ)
};

static CONTROL_ENTRY_PREDEF ce_predefines[MAX_CONTROL_ENTRIES] = {
  CE_INIT(CE_LOCKED,      VECTOR_,                VOTYPE_,                CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VCOUNT_,                CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VECTORSIDE_,    CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VCLASS_,                CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VDATATYPE_,             CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VNCLASS_,               CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VNEW_,                  CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VCCUT_,                 CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VTYPE_,                 CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VPART_,                 CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VCCOARSE_,              CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                FINE_GRID_DOF_, CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                NEW_DEFECT_,    CW_VEOBJ),
  CE_INIT(CE_LOCKED,      VECTOR_,                VACTIVE_,       CW_VEOBJ),

  CE_INIT(CE_LOCKED,      GENERAL_,               OBJ_,                   (CW_GEOMOBJS | CW_VEOBJ)),
  CE_INIT(CE_LOCKED,      GENERAL_,               USED_,                  (CW_GEOMOBJS | CW_VEOBJ)),
  CE_INIT(CE_LOCKED,      GENERAL_,               THEFLAG_,               (CW_GEOMOBJS | CW_VEOBJ)),
  CE_INIT(CE_LOCKED,      GENERAL_,               LEVEL_,                 CW_GEOMOBJS),

  CE_INIT(CE_LOCKED,      VERTEX_,                MOVE_,                  CW_VXOBJS),
  CE_INIT(CE_LOCKED,      VERTEX_,                MOVED_,                 CW_VXOBJS),
  CE_INIT(CE_LOCKED,      VERTEX_,                ONEDGE_,                CW_VXOBJS),
  CE_INIT(CE_LOCKED,      VERTEX_,                ONSIDE_,                CW_VXOBJS),
  CE_INIT(CE_LOCKED,      VERTEX_,                ONNBSIDE_,              CW_VXOBJS),
  CE_INIT(CE_LOCKED,      VERTEX_,                NOOFNODE_,              CW_VXOBJS),

  CE_INIT(CE_LOCKED,      NODE_,                  NSUBDOM_,               CW_NDOBJ),
  CE_INIT(CE_LOCKED,      NODE_,                  NPROP_,                 CW_NDOBJ),
  CE_INIT(CE_LOCKED,      NODE_,                  NCLASS_,                CW_NDOBJ),
  CE_INIT(CE_LOCKED,      NODE_,                  NNCLASS_,               CW_NDOBJ),
  CE_INIT(CE_LOCKED,      NODE_,                  MODIFIED_,              (CW_NDOBJ | CW_GROBJ)),
  CE_INIT(CE_LOCKED,      NODE_,                  NTYPE_,                 CW_NDOBJ),

  CE_INIT(CE_USED,        LINK_,                  LOFFSET_,               CW_EDOBJ),

  CE_INIT(CE_USED,        EDGE_,                  AUXEDGE_,               CW_EDOBJ),
  CE_INIT(CE_USED,        EDGE_,                  PATTERN_,               CW_EDOBJ),
  CE_INIT(CE_USED,        EDGE_,                  ADDPATTERN_,    CW_EDOBJ),
  CE_INIT(CE_USED,        EDGE_,                  EDGENEW_,               CW_EDOBJ),
  CE_INIT(CE_USED,        EDGE_,                  EDSUBDOM_,              CW_EDOBJ),
  CE_INIT(CE_USED,        EDGE_,                  NO_OF_ELEM_,    CW_EDOBJ),

  CE_INIT(CE_USED,        ELEMENT_,               REFINE_,                CW_ELOBJS),
  CE_INIT(CE_USED,        ELEMENT_,               ECLASS_,                CW_ELOBJS),
  CE_INIT(CE_USED,        ELEMENT_,               NSONS_,                 CW_ELOBJS),
  CE_INIT(CE_USED,        ELEMENT_,               REFINECLASS_,   CW_ELOBJS),
  CE_INIT(CE_USED,        ELEMENT_,               NEWEL_,                 CW_ELOBJS),
  CE_INIT(CE_USED,        ELEMENT_,               TAG_,                   CW_ELOBJS),

  CE_INIT(CE_USED,        FLAG_,                  MARK_,                  CW_ELOBJS),
  CE_INIT(CE_USED,        FLAG_,                  COARSEN_,               CW_ELOBJS),
  CE_INIT(CE_USED,        FLAG_,                  DECOUPLED_,             CW_ELOBJS),
  CE_INIT(CE_USED,        FLAG_,                  UPDATE_GREEN_,  CW_ELOBJS),
  CE_INIT(CE_USED,        FLAG_,                  SIDEPATTERN_,   CW_ELOBJS),
  CE_INIT(CE_USED,        FLAG_,                  MARKCLASS_,             CW_ELOBJS),

  CE_INIT(CE_USED,        PROPERTY_,              SUBDOMAIN_,             CW_ELOBJS),
  CE_INIT(CE_USED,        PROPERTY_,              NODEORD_,               CW_ELOBJS),
  CE_INIT(CE_USED,        PROPERTY_,              PROP_,                  CW_ELOBJS),

        #ifdef ModelP
  CE_INIT(CE_USED,        VECTOR_,                XFERVECTOR_,    CW_VEOBJ),
        #endif /* ModelP */
};

/****************************************************************************/
/** \brief Print all control entries of an objects control word

 * @param obj - object pointer
 * @param offset - controlword offset in (UINT) in object

   This function prints the contents of all control entries of an objects control word at a
   given offset.

 */
/****************************************************************************/

void NS_DIM_PREFIX ListCWofObject (const void *obj, UINT offset)
{
  INT i,n,ce,last_ce,sub,min,cw_objt,oiw;

  ASSERT(obj!=NULL);

  cw_objt = BITWISE_TYPE(OBJT(obj));
  sub = -1;
  last_ce = -1;

  /* print control word entries in ascending order of offsets in word */
  do
  {
    min = INT_MAX;
    for (i=0; i<MAX_CONTROL_ENTRIES; i++)
      if (control_entries[i].used)
        if (control_entries[i].objt_used & cw_objt)
          if (control_entries[i].offset_in_object==offset)
          {
            oiw = control_entries[i].offset_in_word;
            if ((oiw<min) && (oiw>=sub))
            {
              if ((oiw==sub) && (i<=last_ce))
                continue;
              ce = i;
              min = oiw;
            }
          }
    if (min==INT_MAX)
      break;

    n = CW_READ(obj,ce);
    UserWriteF("  ce %s with offset in cw %3d: %10d\n",control_entries[ce].name,min,n);
    sub = min;
    last_ce = ce;
  }
  while (true);

  ASSERT(sub>=0);
}

/****************************************************************************/
/** \brief Print all control entries of all control words of an object

 * @param obj - object pointer

   This function prints the contents of all control entries of all control words
   of the object. 'ListCWofObject' is called.
 */
/****************************************************************************/

void NS_DIM_PREFIX ListAllCWsOfObject (const void *obj)
{
  INT i,cw,last_cw,sub,min,cw_objt,offset;

  ASSERT(obj!=NULL);

  cw_objt = BITWISE_TYPE(OBJT(obj));
  sub = -1;
  last_cw = -1;

  /* print control word contents in ascending order of offsets */
  do
  {
    min = INT_MAX;
    for (i=0; i<MAX_CONTROL_WORDS; i++)
      if (control_words[i].used)
        if (control_words[i].objt_used & cw_objt)
        {
          offset = control_words[i].offset_in_object;
          if ((offset<min) && (offset>=sub))
          {
            if ((offset==sub) && (i<=last_cw))
              continue;
            cw = i;
            min = offset;
          }
        }
    if (min==INT_MAX)
      break;

    UserWriteF("cw %s with offset %3d:\n",control_words[cw].name,min);
    ListCWofObject(obj,min);
    sub = min;
    last_cw = cw;
  }
  while (true);

  ASSERT(sub>=0);
}

/****************************************************************************/
/** \brief Print used pattern of all control entries of an object types control word

 * @param obj - object pointer
 * @param offset - controlword offset in (UINT) in object
 * @param myprintf - pointer to a printf function (maybe UserWriteF)

   This function prints the used pattern of all control entries of an object types control word at a
   given offset.
 */
/****************************************************************************/

static void ListCWofObjectType (INT objt, UINT offset, PrintfProcPtr myprintf)
{
  INT i,ce,last_ce,sub,min,cw_objt,oiw;
  char bitpat[33];

  cw_objt = BITWISE_TYPE(objt);
  sub = -1;
  last_ce = -1;

  /* print control word entries in ascending order of offsets in word */
  do
  {
    min = INT_MAX;
    for (i=0; i<MAX_CONTROL_ENTRIES; i++)
      if (control_entries[i].used)
        if (control_entries[i].objt_used & cw_objt)
          if (control_entries[i].offset_in_object==offset)
          {
            oiw = control_entries[i].offset_in_word;
            if ((oiw<min) && (oiw>=sub))
            {
              if ((oiw==sub) && (i<=last_ce))
                continue;
              ce = i;
              min = oiw;
            }
          }
    if (min==INT_MAX)
      break;

    INT_2_bitpattern(control_entries[ce].mask,bitpat);
    myprintf("  ce %-20s offset in cw %3d, len %3d: %s\n",
             control_entries[ce].name,
             control_entries[ce].offset_in_word,
             control_entries[ce].length,
             bitpat);
    sub = min;
    last_ce = ce;
  }
  while (true);

  if (sub==-1)
    myprintf(" --- no ce found with objt %d\n",objt);
}

/****************************************************************************/
/** \brief Print used pattern of all control entries of all
    control words of an object type

 * @param obj - object pointer
 * @param myprintf - pointer to a printf function (maybe UserWriteF)

   This function prints the used pattern of all control entries of all control words
   of an object type. 'ListCWofObjectType' is called.
 */
/****************************************************************************/

static void ListAllCWsOfObjectType (INT objt, PrintfProcPtr myprintf)
{
  INT i,cw,last_cw,sub,min,cw_objt,offset;

  cw_objt = BITWISE_TYPE(objt);
  sub = -1;
  last_cw = -1;

  /* print control word contents in ascending order of offsets */
  do
  {
    min = INT_MAX;
    for (i=0; i<MAX_CONTROL_WORDS; i++)
      if (control_words[i].used)
        if (control_words[i].objt_used & cw_objt)
        {
          offset = control_words[i].offset_in_object;
          if ((offset<min) && (offset>=sub))
          {
            if ((offset==sub) && (i<=last_cw))
              continue;
            cw = i;
            min = offset;
          }
        }
    if (min==INT_MAX)
      break;

    myprintf("cw %-20s with offset in object %3d (UINTs):\n",control_words[cw].name,min);
    ListCWofObjectType(objt,min,myprintf);
    sub = min;
    last_cw = cw;
  }
  while (true);

  if (sub==-1)
    printf(" --- no cw found with objt %d\n",objt);
}

/****************************************************************************/
/** \brief Print used pattern of all control entries of all
    control words of all object types

 * @param obj - object pointer
 * @param myprintf - pointer to a printf function (maybe UserWriteF)

   This function prints the used pattern of all control entries of all control words
   of all object types. 'ListAllCWsOfObjectType' is called.
 */
/****************************************************************************/

void NS_DIM_PREFIX ListAllCWsOfAllObjectTypes (PrintfProcPtr myprintf)
{
  ListAllCWsOfObjectType(IVOBJ,myprintf);
  ListAllCWsOfObjectType(IEOBJ,myprintf);
  ListAllCWsOfObjectType(EDOBJ,myprintf);
  ListAllCWsOfObjectType(NDOBJ,myprintf);
  ListAllCWsOfObjectType(VEOBJ,myprintf);
  ListAllCWsOfObjectType(GROBJ,myprintf);
  ListAllCWsOfObjectType(MGOBJ,myprintf);
}

/****************************************************************************/
/** \brief Initialize control words

   This function initializes the predefined control words.

 * @return <ul>
 * <li> GM_OK if ok </li>
 * <li> GM_ERROR if error occurred </li>
 * </ul>
 */
/****************************************************************************/

static INT InitPredefinedControlWords (void)
{
  INT i,nused;
  CONTROL_WORD *cw;
  CONTROL_WORD_PREDEF *pcw;

  /* clear everything */
  memset(control_words,0,MAX_CONTROL_WORDS*sizeof(CONTROL_WORD));

  nused = 0;
  for (i=0; i<MAX_CONTROL_WORDS; i++)
    if (cw_predefines[i].used)
    {
      pcw = cw_predefines+i;
      ASSERT(pcw->control_word_id<MAX_CONTROL_WORDS);

      nused++;
      cw = control_words+pcw->control_word_id;
      if (cw->used)
      {
        printf("redefinition of control word '%s'\n",pcw->name);
        return(__LINE__);
      }
      cw->used = pcw->used;
      cw->name = pcw->name;
      cw->offset_in_object = pcw->offset_in_object;
      cw->objt_used = pcw->objt_used;
    }

  if (nused!=GM_N_CW)
  {
    printf("InitPredefinedControlWords: nused=%d != GM_N_CW=%d\n",nused,GM_N_CW);
    assert(false);
  }

  return (GM_OK);
}

/****************************************************************************/
/** \brief Initialize control word entries

   This function initializes the predefined control word entries. Predefined
   entries are not checked for overlap.

 * @return <ul>
 * <li> GM_OK if ok </li>
 * <li> GM_ERROR if error occurred </li>
 * </ul>
 */
/****************************************************************************/

static INT InitPredefinedControlEntries (void)
{
  CONTROL_ENTRY *ce,*test_ce;
  CONTROL_WORD *cw;
  CONTROL_ENTRY_PREDEF *pce,*test_pce;
  INT i,j,k,mask,error,nused;

  /* clear everything */
  memset(control_entries,0,MAX_CONTROL_ENTRIES*sizeof(CONTROL_ENTRY));

  error = 0;
  nused = 0;
  for (i=0; i<MAX_CONTROL_ENTRIES; i++)
    if (ce_predefines[i].used)
    {
      pce = ce_predefines+i;
      ASSERT(pce->control_entry_id<MAX_CONTROL_ENTRIES);

      nused++;
      ce = control_entries+pce->control_entry_id;
      if (ce->used)
      {
        printf("redefinition of control entry '%s'\n",pce->name);
        return(__LINE__);
      }
      cw = control_words+pce->control_word;
      ASSERT(cw->used);
      ce->used = pce->used;
      ce->name = pce->name;
      ce->control_word = pce->control_word;
      ce->offset_in_word = pce->offset_in_word;
      ce->length = pce->length;
      ce->objt_used = pce->objt_used;
      ce->offset_in_object = cw->offset_in_object;
      ce->mask = (POW2(ce->length)-1)<<ce->offset_in_word;
      ce->xor_mask = ~ce->mask;

      PRINTDEBUG(gm,1,("ceID %d used %d name %s cw %d\n",
                       i,ce->used,ce->name,ce->control_word));

      ASSERT(ce->objt_used & cw->objt_used);                                    /* ce and cw have! common objects */

      /* set mask in all cws that use some of the ces objects and have the same offset than cw */
      const UINT offset = ce->offset_in_object;
      mask   = ce->mask;
      for (k=0; k<MAX_CONTROL_WORDS; k++)
      {
        cw = control_words+k;

        if (!cw->used)
          continue;
        if (!(ce->objt_used & cw->objt_used))
          continue;
        if (cw->offset_in_object!=offset)
          continue;

        /* do other control entries overlap? */
        if (cw->used_mask & mask)
        {
          IFDEBUG(gm,1)
          printf("predef ctrl entry '%s' has overlapping bits with previous ctrl entries:\n",pce->name);
          for (j=0; j<i; j++)
          {
            test_pce = ce_predefines+j;
            test_ce  = control_entries+test_pce->control_entry_id;
            if (test_ce->objt_used & ce->objt_used)
              if (test_ce->offset_in_object==offset)
                if (test_ce->mask & mask)
                  printf(" '%s'",test_pce->name);

          }
          printf("\n");
          ENDDEBUG
            error++;
        }
        cw->used_mask |= mask;
      }
    }

  IFDEBUG(gm,1)
  ListAllCWsOfAllObjectTypes(printf);
  ENDDEBUG

  /* TODO: enable next lines for error control */
  IFDEBUG(gm,1)
  if (error)
    return (__LINE__);
  ENDDEBUG

  if (nused!=REFINE_N_CE)
  {
    printf("InitPredefinedControlEntries: nused=%d != REFINE_N_CE=%d\n",nused,REFINE_N_CE);
    assert(false);
  }

  return (GM_OK);
}


/****************************************************************************/
/** \brief Function to replace CW_READ macro and does extended error checks

 * @param obj - object pointer
 * @param ceID - control entry ID

   This function is to replace the CW_READ and CW_READ_STATIC macros of gm.h and does extended
   error checks:~
   <ul>
   <li> obj != NULL </li>
   <li> HEAPFAULT </li>
   <li> ceID in valid range </li>
   <li> control entry used </li>
   <li> the object type is checked (with the natural exception of the first SETOBJ access)  </li>
   </ul>
   Additionally the read accesses to a control entry are counted.

   CAUTION:
   set #define _DEBUG_CW_ to replace CW_READ by ReadCW but be aware of the
   slowing down of the program in this case (no large problems!).

   @return
   Number read from the control entry of the object
 */
/****************************************************************************/

UINT NS_DIM_PREFIX ReadCW (const void *obj, INT ceID)
{
  CONTROL_ENTRY *ce;
  UINT off_in_obj,mask,i,off_in_wrd,cw,cw_objt;

  ASSERT(obj!=NULL);

  if ((ceID<0) || (ceID>=MAX_CONTROL_ENTRIES))
  {
    printf("ReadCW: ceID=%d out of range\n",ceID);
    assert(false);
  }

  ce = control_entries+ceID;

  if (!ce->used)
  {
    printf("ReadCW: ceID=%d unused\n",ceID);
    assert(false);
  }

  cw_objt = BITWISE_TYPE(OBJT(obj));
  if (!(cw_objt & ce->objt_used))
  {
    if (ce->name!=NULL)
      printf("ReadCW: invalid objt %d for ce %s\n",OBJT(obj),ce->name);
    else
      printf("ReadCW: invalid objt %d for ce %d\n",OBJT(obj),ceID);
    assert(false);
  }

  off_in_wrd = ce->offset_in_word;
  off_in_obj = ce->offset_in_object;
  mask = ce->mask;
  cw = ((UINT *)(obj))[off_in_obj];
  i = cw & mask;
  i = i>>off_in_wrd;

  return (i);
}

/****************************************************************************/
/** \brief Function to replace CW_WRITE macro and does extended error checks

 * @param obj - object pointer
 * @param ceID - control entry ID
 * @param n - number to write to the objects control entry

   This function is to replace the CW_WRITE and CW_WRITE_STATIC macros of gm.h and does extended
   error checks:~
   <ul>
   <li>  obj != NULL </li>
   <li>  HEAPFAULT </li>
   <li>  ceID in valid range </li>
   <li>  control entry used </li>
   <li>  the object type is checked </li>
   <li>  n small enough for length of control entry </li>
   </ul>
   Additionally the write accesses to a control entry are counted.

   CAUTION:
   set #define _DEBUG_CW_ to replace CW_WRITE by WriteCW but be aware of the
   slowing down of the program in this case (no large problems!).

   @return
   Number read from the control entry of the object
 */
/****************************************************************************/

void NS_DIM_PREFIX WriteCW (void *obj, INT ceID, INT n)
{
  CONTROL_ENTRY *ce;
  UINT off_in_obj,mask,i,j,off_in_wrd,xmsk;
  UINT *pcw;

  ASSERT(obj!=NULL);

  if ((ceID<0) || (ceID>=MAX_CONTROL_ENTRIES))
  {
    printf("WriteCW: ceID=%d out of range\n",ceID);
    assert(false);
  }

  ce = control_entries+ceID;

  if (!ce->used)
  {
    printf("WriteCW: ceID=%d unused\n",ceID);
    assert(false);
  }

  const UINT cw_objt = BITWISE_TYPE(OBJT(obj));

  /* special: SETOBJT cannot be checked since at this point the OBJT is unknown of course */
  if (cw_objt==BITWISE_TYPE(0))
  {
    if (ceID!=OBJ_CE)
      if (cw_objt != ce->objt_used)
      {
        if (ce->name!=NULL)
          printf("WriteCW: objt 0 but %s rather than expected SETOBJT access\n",ce->name);
        else
          printf("WriteCW: objt 0 but %d rather than expected SETOBJT access\n",ceID);
        assert(false);
      }
  }
  else if (!(cw_objt & ce->objt_used))
  {
    if (ce->name!=NULL)
      printf("WriteCW: invalid objt %d for ce %s\n",OBJT(obj),ce->name);
    else
      printf("WriteCW: invalid objt %d for ce %d\n",OBJT(obj),ceID);
    assert(false);
  }

  off_in_wrd = ce->offset_in_word;
  off_in_obj = ce->offset_in_object;
  mask = ce->mask;
  xmsk = ce->xor_mask;
  pcw = ((UINT *)(obj)) + off_in_obj;
  i = (*pcw) & xmsk;
  j = n<<off_in_wrd;

  if (j>mask)
  {
    if (ce->name!=NULL)
      printf("WriteCW: value=%d exceeds max=%d for %s\n",
             n,POW2(ce->length)-1,ce->name);
    else
      printf("WriteCW: value=%d exceeds max=%d for %d\n",
             n,POW2(ce->length)-1,ceID);
    assert(false);
  }

  j = j & mask;

  *pcw = i | j;
}

/****************************************************************************/
/** \brief Allocates space in object control words

 * @param cw_id - id of a control word
 * @param length - number of bits to allocate
 * @param ce_id -  returns identifier of control entry descriptor

   This function allocates 'length' consecutive bits in the control word of an
   object identified through the `control word id` 'cw_id'.
   It returns '0' and a valid id in 'ce_id' if space was available.
   The 'ce_id' can be used to read and write the requested bits with the
   'CW_READ' and 'CW_WRITE' macros as in the following example.

   The control word ids of all UG objects are defined in 'gm.h' and usually
   have the name `<object>`'_CW', except e.g. the 'ELEMENT' which has three
   words that are used bitwise.

   EXAMPLE:

   The following code fragment allocates 'NORDER_LEN' bits in the 'flag' word
   of the 'ELEMENT' structure.

   \verbatim
   INT ce_NORDER;

   if (AllocateControlEntry(FLAG_CW,NORDER_LEN,&ce_NORDER) != GM_OK) return (1);
   \endverbatim

   The following macros then read and write the requested bits

   \verbatim
 *#define NORDER(p)      CW_READ(p,ce_NORDER)
 *#define SETNORDER(p,n) CW_WRITE(p,ce_NORDER,n)
   \endverbatim


   @return
   </ul>
   <li>   GM_OK if ok </li>
   <li>   GM_ERROR if error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX AllocateControlEntry (INT cw_id, INT length, INT *ce_id)
{
  INT free, i, offset;
  CONTROL_ENTRY *ce;
  CONTROL_WORD *cw;
  UINT mask;

  /* check input */
  if ((length<0)||(length>=32)) return(GM_ERROR);
  if ((cw_id<0)||(cw_id>=MAX_CONTROL_WORDS)) return(GM_ERROR);

  /* it is sufficient to check only the control entries control word
     multiple object types are only allowed for predefines */
  cw = control_words+cw_id;

  /* find unused entry */
  for (i=0; i<MAX_CONTROL_ENTRIES; i++)
    if (!control_entries[i].used) break;
  if (i==MAX_CONTROL_ENTRIES) return(GM_ERROR);
  free = i;
  ce = control_entries+free;

  /* lets see if enough consecutive bits are available */
  mask = POW2(length)-1;
  for (i=0; i<=32-length; i++)
  {
    if ((mask&cw->used_mask)==0) break;
    mask <<= 1;
  }
  if (i>32-length) return(GM_ERROR);
  offset = i;

  /* fill new entry */
  *ce_id = free;
  ce->used = 1;
  ce->control_word = cw_id;
  ce->offset_in_object = cw->offset_in_object;
  ce->offset_in_word = offset;
  ce->length = length;
  ce->mask = mask;
  ce->xor_mask = ~mask;
  ce->name = NULL;
  ce->objt_used = cw->objt_used;

  /* remember used bits */
  cw->used_mask |= mask;

  /* ok, exit */
  return(GM_OK);
}

/****************************************************************************/
/** \brief Frees space in object control words

 * @param ce_id - control entry descriptor to free

   This function frees space in object control words that has been allocated
   with 'AllocateControlEntry'.

   @return <ul>
   <li>    GM_OK if ok </li>
   <li>    GM_ERROR if error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX FreeControlEntry (INT ce_id)
{
  CONTROL_ENTRY *ce;
  CONTROL_WORD *cw;

  /* check parameter */
  if ((ce_id<0)||(ce_id>=MAX_CONTROL_ENTRIES)) return(GM_ERROR);
  ce = control_entries+ce_id;
  cw = control_words+ce->control_word;

  /* check if locked */
  if (ce->used == 2)
    return(GM_ERROR);

  /* free used bits */
  cw->used_mask &= ce->xor_mask;

  /* free control entry */
  ce->used = 0;

  /* ok, exit */
  return(GM_OK);
}

/****************************************************************************/
/** \brief Init cw.c file

   This function initializes the control word manager.

   @return <ul>
   <li>  GM_OK if ok </li>
   <li>  > 0 line in which error occurred. </li>
   </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitCW (void)
{
  if (InitPredefinedControlWords())
    return (__LINE__);
  if (InitPredefinedControlEntries())
    return (__LINE__);

  return (GM_OK);
}
