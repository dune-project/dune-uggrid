// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  wop.h                                                                                                                 */
/*																			*/
/* Purpose:   header file for work functions on pictures					*/
/*																			*/
/* Author:	  Klaus Johannsen												*/
/*			  Institut fuer Computeranwendungen                                                     */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70569 Stuttgart												*/
/*			  internet: klaus@ica3.uni-stuttgart.de                                                 */
/*																			*/
/* History:   8.12.94 begin, ug3-version									*/
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* auto include mechanism and other include files							*/
/*																			*/
/****************************************************************************/

#ifndef __WOP__
#define __WOP__

#include "compiler.h"

#ifndef __GM__
#include "gm.h"
#endif

#ifndef __WPM__
#include "wpm.h"
#endif

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/* macros for matrix-vector operations */
#define V2_TRAFOM3_V2(A,M,B)               {(B)[0] = (M)[0]*(A)[0] + (M)[3]*(A)[1] + (M)[6];\
                                            (B)[1] = (M)[1]*(A)[0] + (M)[4]*(A)[1] + (M)[7];}

#define V3_TRAFOM4_V3(A,M,B)               {(B)[0] = (M)[0]*(A)[0] + (M)[4]*(A)[1] + (M)[8]*(A)[2] + (M)[12];\
                                            (B)[1] = (M)[1]*(A)[0] + (M)[5]*(A)[1] + (M)[9]*(A)[2] + (M)[13];\
                                            (B)[2] = (M)[2]*(A)[0] + (M)[6]*(A)[1] + (M)[10]*(A)[2] + (M)[14];}

#define V3_TRAFO4_SC(A,M,b)                {(b) = (M)[2]*(A)[0] + (M)[6]*(A)[1] + (M)[10]*(A)[2] + (M)[14];}

/****************************************************************************/
/*																			*/
/* Defines and Macros for DRAWINGOBJECT                                                                         */
/*																			*/
/****************************************************************************/

#undef __DO_HEAP_USED__
/*#define __DO_HEAP_USED__*/
#define DO_SIZE                                                 30000

#define DO_NO_INST                                              0
#define DO_RANGE                                                1
#define DO_LINE                                                 2
#define DO_ARROW                                                3
#define DO_INVERSE_LINE                                 4
#define DO_POLYLINE                                     5
#define DO_POLYGON                                              6
#define DO_INVERSE_POLYGON                              7
#define DO_SURRPOLYGON                                  8
#define DO_ERASE_POLYGON                                9
#define DO_ERASE_SURRPOLYGON                    10
#define DO_TEXT                                                 11
#define DO_POLYMARK                                     12

/* text position */
#define TEXT_NOT_CENTERED                               0
#define TEXT_CENTERED                                   1

/* text modes */
#define TEXT_REGULAR                                    0
#define TEXT_INVERSE                                    1

/* increment */
#define DO_inc(p)                                               (p)+=1;
#define DO_inc_n(p,n)                                   (p)+=n;
#define DO_strlen(p)                                    ((((int)strlen((char*)(p))+1)/((int)sizeof(DRAWINGOBJ)))+1)
#define DO_inc_str(p)                                   (p)+=DO_strlen(p);

#define DO_inc_RANGE(p)                                 (p)+=3;
#define DO_inc_LINE(p,d)                                (p)+=2+2*d;
#define DO_inc_ARROW(p,d)                               (p)+=2+2*d;
#define DO_inc_INVERSE_LINE(p,d)                (p)+=1+2*d;
#define DO_inc_POLYLINE(p,d)                    (p)+=3+((char*)(p+1))[0]*d;
#define DO_inc_POLYGON(p,d)                     (p)+=3+((char*)(p+1))[0]*d;
#define DO_inc_INVERSE_POLYGON(p,d)     (p)+=2+((char*)(p+1))[0]*d;
#define DO_inc_SURRPOLYGON(p,d)                 (p)+=4+((char*)(p+1))[0]*d;
#define DO_inc_ERASE_POLYGON(p,d)               (p)+=2+((char*)(p+1))[0]*d;
#define DO_inc_ERASE_SURRPOLYGON(p,d)   (p)+=3+((char*)(p+1))[0]*d;
#define DO_inc_TEXT(p,d)                                (p)+=5+d+DO_strlen(p+5+d);
#define DO_inc_POLYMARK(p,d)                    (p)+=5+((char*)(p+1))[0]*d;

/* cast */
#define DO_2c(p)                                                (*((char*)(p)))
#define DO_2cp(p)                                               ((char*)(p))
#define DO_2s(p)                                                (*((short*)(p)))
#define DO_2l(p)                                                (*((long*)(p)))
#define DO_2C(p)                                                (*((COORD*)(p)))
#define DO_2Cp(p)                                               ((COORD*)(p))

/****************************************************************************/
/*																			*/
/* Defines and Macros for WORKPROCS                                                                             */
/*																			*/
/****************************************************************************/

/* defines for WorkMode field */
#define ELEMENTWISE                                     1
#define NODEWISE                                                2
#define VECTORWISE                                              3
#define EXTERN                                                  4

#define MAX_NO_CYCLES                                   3

#define WP_WORKMODE(p)                                  ((p)->WorkMode)
#define WP_ELEMWISE(p)                                  (&((p)->ElemWiseWorkProcs))
#define WP_NODEWISE(p)                                  (&((p)->NodeWiseWorkProcs))
#define WP_VECTORWISE(p)                                (&((p)->VectorWiseWorkProcs))
#define WP_EXTERNWISE(p)                                (&((p)->ExternWorkProcs))

/****************************************************************************/
/*																			*/
/* Defines and Macros for PLOTOBJHANDLING									*/
/*																			*/
/****************************************************************************/

#define POH_PLOTOBJTYPE(p)                              ((p)->thePlotObjType)
#define POH_NBCYCLES(p,m)                               ((p)->NbOfCycles[m])
#define POH_WORKPROGS(p,m,n)                    (&((p)->theWorkProcs[m][n]))

/****************************************************************************/
/*																			*/
/* Defines and Macros for WORK												*/
/*																			*/
/****************************************************************************/

/* WorkID's */
#define NB_WORK                                 8
#define DRAW_WORK                               0
#define FINDRANGE_WORK                  1
#define SELECTNODE_WORK                 2
#define SELECTELEMENT_WORK              3
#define MARKELEMENT_WORK                4
#define INSERTNODE_WORK                 5
#define MOVENODE_WORK                   6
#define INSERTBNDNODE_WORK              7


#define W_ID(p)                                 ((p)->WorkID)
#define W_DRAW_WORK(p)                  (&((p)->theDrawWork))
#define W_FINDRANGE_WORK(p)     (&((p)->theFindRangeWork))
#define W_SELECTNODE_WORK(p)    (&((p)->theSelectNodeWork))
#define W_SELECTELEMENT_WORK(p) (&((p)->theSelectElementWork))
#define W_MARKELEMENT_WORK(p)   (&((p)->theMarkElementWork))
#define W_INSERTNODE_WORK(p)    (&((p)->theInsertNodeWork))
#define W_MOVENODE_WORK(p)              (&((p)->theMoveNodeWork))
#define W_INSERTBNDNODE_WORK(p) (&((p)->theInsertBndNodeWork))

#define W_ISSELECTWORK(p)               ((p)->WorkID==SELECTNODE_WORK || (p)->WorkID==SELECTELEMENT_WORK)

/****************************************************************************/
/*																			*/
/* miscellenious macros														*/
/*																			*/
/****************************************************************************/

#define WOP_ACTIVE                              0
#define WOP_NOT_ACTIVE                  1
#define WOP_WORKING                             2

/****************************************************************************/
/*																			*/
/* structures and ...														*/
/*																			*/
/****************************************************************************/

struct Draw_Work {

  INT WorkID;                                   /* unique ID of the work					*/
};

struct FindRange_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  INT put;                                              /* store values on PlotObj if YES			*/
  INT symmetric;                                /* symmetrisize range if YES				*/
  DOUBLE zoom;                                  /* factor to zoom the range                             */

  /* the result of the work */
  DOUBLE min, max;                              /* range found								*/
};

struct SelectNode_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

struct SelectElement_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

struct MarkElement_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

struct InsertNode_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

struct InsertBndNode_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

struct MoveNode_Work {

  INT WorkID;                                   /* unique ID of the work					*/

  /* specification of the work */
  short PixelX;                                 /* x pixel coordinate						*/
  short PixelY;                                 /* y pixel coordinate						*/
};

union Work {

  INT WorkID;                                                                           /* unique ID of the work					*/
  struct Draw_Work theDrawWork;                                         /* desc. for drawing an object				*/
  struct FindRange_Work theFindRangeWork;                       /* desc. for finding the range of an object */
  struct SelectNode_Work theSelectNodeWork;                     /* desc. for selecting a node of an object	*/
  struct SelectElement_Work theSelectElementWork;       /* desc. for selecting an element of an obj.*/
  struct MarkElement_Work theMarkElementWork;           /* desc. for marking an element of an obj.	*/
  struct InsertNode_Work theInsertNodeWork;                     /* desc. for inserting an inner node in obj.*/
  struct InsertBndNode_Work theInsertBndNodeWork;       /* desc. for inserting an bnd node in obj.	*/
  struct MoveNode_Work theMoveNodeWork;                         /* desc. for moving an node of an obj.		*/
};

typedef COORD DRAWINGOBJ;

/* general procedure ptr */
typedef INT (*GEN_PreProcessProcPtr)(PICTURE *, union Work *);
typedef INT (*GEN_ExecuteProcPtr)(DRAWINGOBJ *);
typedef INT (*GEN_PostProcessProcPtr)(PICTURE *, union Work *);

/* elementwise procedure ptr */
typedef ELEMENT *                                               (*EW_GetFirstElementProcPtr)(MULTIGRID *, INT, INT);
typedef ELEMENT *                                               (*EW_GetNextElementProcPtr)(ELEMENT *);
typedef EW_GetFirstElementProcPtr (*EW_GetFirstElementProcProcPtr)(VIEWEDOBJ *);
typedef EW_GetNextElementProcPtr (*EW_GetNextElementProcProcPtr)(VIEWEDOBJ *);
typedef INT (*EW_EvaluateProcPtr)(ELEMENT *, DRAWINGOBJ *);

/* nodewise procedure ptr */
typedef NODE *                                                  (*NW_GetFirstNodeProcPtr)(MULTIGRID *, INT, INT);
typedef NODE *                                                  (*NW_GetNextNodeProcPtr)(NODE *);
typedef NW_GetFirstNodeProcPtr (*NW_GetFirstNodeProcProcPtr)(VIEWEDOBJ *);
typedef NW_GetNextNodeProcPtr (*NW_GetNextNodeProcProcPtr)(VIEWEDOBJ *);
typedef INT (*NW_EvaluateProcPtr)(NODE *, DRAWINGOBJ *);

/* vectorwise procedure ptr */
typedef VECTOR *                                                (*VW_GetFirstVectorProcPtr)(MULTIGRID *, INT, INT);
typedef VECTOR *                                                (*VW_GetNextVectorProcPtr)(VECTOR *);
typedef VW_GetFirstVectorProcPtr (*VW_GetFirstVectorProcProcPtr)(VIEWEDOBJ *);
typedef VW_GetNextVectorProcPtr (*VW_GetNextVectorProcProcPtr)(VIEWEDOBJ *);
typedef INT (*VW_EvaluateProcPtr)(VECTOR *, DRAWINGOBJ *);

/* extern procedure ptr */
typedef INT (*EXT_EvaluateProcPtr)(DRAWINGOBJ *, INT *);

struct ElemWiseWork {

  INT WorkMode;                                                                                                         /* see above									*/
  GEN_PreProcessProcPtr EW_PreProcessProc;                                                      /* preprocess procedure                                                 */
  GEN_ExecuteProcPtr EW_ExecuteProc;                                                                    /* execute procedure							*/
  GEN_PostProcessProcPtr EW_PostProcessProc;                                            /* postprocess procedure						*/

  EW_GetFirstElementProcProcPtr EW_GetFirstElementProcProc;             /* proc for getting step function				*/
  EW_GetNextElementProcProcPtr EW_GetNextElementProcProc;                       /* proc for getting step function				*/
  EW_EvaluateProcPtr EW_EvaluateProc;                                                                   /* evaluation procedure                                                 */
};

struct NodeWiseWork {

  INT WorkMode;                                                                                                         /* see above									*/
  GEN_PreProcessProcPtr NW_PreProcessProc;                                                      /* preprocess procedure                                                 */
  GEN_ExecuteProcPtr NW_ExecuteProc;                                                                    /* execute procedure							*/
  GEN_PostProcessProcPtr NW_PostProcessProc;                                            /* postprocess procedure						*/

  NW_GetFirstNodeProcProcPtr NW_GetFirstNodeProcProc;                                   /* proc for getting step function				*/
  NW_GetNextNodeProcProcPtr NW_GetNextNodeProcProc;                                     /* proc for getting step function				*/
  NW_EvaluateProcPtr NW_EvaluateProc;                                                                   /* evaluation procedure                                                 */
};

struct VectorWiseWork {

  INT WorkMode;                                                                                                         /* see above									*/
  GEN_PreProcessProcPtr VW_PreProcessProc;                                                      /* preprocess procedure                                                 */
  GEN_ExecuteProcPtr VW_ExecuteProc;                                                                    /* execute procedure							*/
  GEN_PostProcessProcPtr VW_PostProcessProc;                                            /* postprocess procedure						*/

  VW_GetFirstVectorProcProcPtr VW_GetFirstVectorProcProc;                       /* proc for getting step function				*/
  VW_GetNextVectorProcProcPtr VW_GetNextVectorProcProc;                         /* proc for getting step function				*/
  VW_EvaluateProcPtr VW_EvaluateProc;                                                                   /* evaluation procedure                                                 */
};

struct ExternWork {

  INT WorkMode;                                                                                 /* see above										*/
  GEN_PreProcessProcPtr EXT_PreProcessProc;                     /* preprocess procedure                                                         */
  GEN_ExecuteProcPtr EXT_ExecuteProc;                                           /* execute procedure								*/
  GEN_PostProcessProcPtr EXT_PostProcessProc;                   /* postprocess procedure							*/

  EXT_EvaluateProcPtr EXT_EvaluateProc;                                 /* evaluation procedure                                                         */
};

union WorkProcs {

  INT WorkMode;                                                                                 /* see above										*/
  struct ElemWiseWork ElemWiseWorkProcs;                                /* procedures for elementwise work					*/
  struct NodeWiseWork NodeWiseWorkProcs;                                /* procedures for nodewise work                                         */
  struct VectorWiseWork VectorWiseWorkProcs;                    /* procedures for vectorwise work					*/
  struct ExternWork ExternWorkProcs;                                            /* procedures for extern work						*/
};

struct PlotObjHandling {

  PLOTOBJTYPE thePlotObjType;                                                                                   /* abstract object definition		*/
  INT NbOfCycles[NB_WORK];                                                                                      /* number of working cycles             */
  union WorkProcs theWorkProcs[NB_WORK][MAX_NO_CYCLES];                                 /* work procs						*/
};

/****************************************************************************/
/*																			*/
/*					typedef for structs                                                                     */
/*																			*/
/****************************************************************************/

typedef union  WorkProcs WORKPROCS;
typedef struct ElemWiseWork ELEMWISEWORK;
typedef struct NodeWiseWork NODEWISEWORK;
typedef struct VectorWiseWork VECTORWISEWORK;
typedef struct ExternWork EXTERNWORK;
typedef struct TrafoContext TRAFOCONTEXT;
typedef union  Work WORK;
typedef struct PlotObjHandling PLOTOBJHANDLING;

/****************************************************************************/
/*																			*/
/* function declarations													*/
/*																			*/
/****************************************************************************/

INT             InitWOP                                 (void);

INT             WorkOnPicture                   (PICTURE *thePicture, WORK *theWork);
INT             DrawUgPicture                   (PICTURE *thePicture);
INT             DrawPictureFrame                (PICTURE *thePicture, INT mode);
INT             ErasePicture                    (PICTURE *thePicture);


#endif
