// NOTE: The current revision of this file was left untouched when the DUNE source files were reindented!
// NOTE: It contained invalid syntax that could not be processed by uncrustify.

/****************************************************************************/
/*																			*/
/* File:	  handler.c														*/
/*																			*/
/* Purpose:   defines the handlers used by ddd during data management.      */
/*			  																*/
/*																			*/
/* Author:	  Stefan Lang                       				 			*/
/*			  Institut fuer Computeranwendungen III 						*/
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: stefan@ica3.uni-stuttgart.de							*/
/*			  phone: 0049-(0)711-685-7003									*/
/*			  fax  : 0049-(0)711-685-7000									*/
/*																			*/
/* History:   10.05.95 begin, ugp version 3.0								*/
/*																			*/
/* Remarks: 																*/
/*																			*/
/****************************************************************************/

/* TODO: delete this */
/* #include "conf.h" */

#ifdef ModelP

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files 									*/
/*																			*/
/****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "compiler.h"
#include "domain.h"
#include "parallel.h"
#include "heaps.h"


/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define MAX_EDGES 50

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/



/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

/* data for CVS */
static char rcsid[] = "$Header$";

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


static GRID *GetGridOnDemand (MULTIGRID *mg, int level)
{
	while (level > TOPLEVEL(mg))
	{
		CreateNewLevel(mg);
		UserWriteF("CreateNewLevel %d", TOPLEVEL(mg));
		/* TODO error handling, CreateNewLevel() may return NULL */
	}

	return GRID_ON_LEVEL(mg,level);
}




/****************************************************************************/
/*																			*/
/*  	For data management during redistribution and communication 		*/
/*		DDD needs for each DDD (data) object several handlers.				*/
/*		These are:															*/
/*			HANDLER_LDATACONSTRUCTOR,  handler: init object's LDATA parts   */
/*			HANDLER_UPDATE,            handler: update objects internals    */
/*			HANDLER_OBJMKCONS,         handler: make obj consistent         */
/*			HANDLER_DESTRUCTOR,        handler: destruct object             */
/*			HANDLER_XFERCOPY,          handler: copy cmd during xfer        */
/*			HANDLER_XFERDELETE,        handler: delete cmd during xfer      */
/*			HANDLER_XFERGATHER,        handler: send additional data        */
/*			HANDLER_XFERSCATTER,       handler: recv additional data        */
/*			HANDLER_COPYMANIP,         handler: manipulate incoming copy    */
/*																			*/
/*																			*/
/*	    For each of the ddd (data) objects the handlers needed for the      */
/*	    specific object are defined below in the order handlers for         */
/*																			*/
/*			DDD objects:													*/
/*				*dimension independent										*/
/*				 TypeVector, TypeIVertex, TypeBVertex, TypeNode				*/
/*				*dimension dependent										*/
/*				 2-Dim:														*/
/*				 TypeTrElem, TypeTrBElem,									*/
/*				 TypeQuElem, TypeQuBElem									*/
/*				 3-Dim:														*/
/*				 TypeTeElem, TypeTeBElem									*/
/*				 TypePyElem, TypePyBElem									*/
/*				 TypeHeElem, TypeHeBElem									*/
/*																			*/
/*			DDD data objects:												*/
/*				TypeMatrix, 												*/
/*				TypeVSegment,												*/
/*				TypeEdge													*/
/*																			*/
/*		NOT all handlers are to be specified for each object!				*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typevector												*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

void VectorUpdate (DDD_OBJ obj)
{
	VECTOR *pv = (VECTOR *)obj;
	VECTOR *after = NULL;
	GRID *theGrid = NULL;

	PRINTDEBUG(dddif,1,("%2d: VectorUpdate(): v=%x VEOBJ=%d\n",me,pv,OBJT(pv)))

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	after = LASTVECTOR(theGrid);

        /* insert in vector list */
        if (after==NULL)
        {
                SUCCVC(pv) = (VECTOR*)theGrid->firstVector;
                PREDVC(pv) = NULL;
                if (SUCCVC(pv)!=NULL)
                        PREDVC(SUCCVC(pv)) = pv;
                theGrid->firstVector = (void*)pv;
                if (theGrid->lastVector==NULL)
                        theGrid->lastVector = (void*)pv;
        }
        else
        {
                SUCCVC(pv) = SUCCVC(after);
                PREDVC(pv) = after;
                if (SUCCVC(pv)!=NULL)
                        PREDVC(SUCCVC(pv)) = pv;
                else
                        theGrid->lastVector = (void*)pv;
                SUCCVC(after) = pv;
        }

		VSTART(pv) = NULL;

        /* counters */
        theGrid->nVector++;
}

void VectorXferCopy (DDD_OBJ obj, int proc, int prio)
{
	int 	nmat=0;
	MATRIX	*mat;
	VECTOR  *vec = (VECTOR *)obj;
	size_t  sizeArray[30]; /* TODO: define this static global TODO: take size as maximum of possible connections */

	PRINTDEBUG(dddif,2,("%2d:  VectorXferCopy(): v=%x AddData nmat=%d\n",me,vec,nmat))

	for(mat=VSTART(vec); mat!=NULL; mat=MNEXT(mat))
	{
		if (XFERMATX(mat) == COPY) 
		{
			PRINTDEBUG(dddif,3,("%2d: VectorXferCopy(): v=%x COPYFLAG already set for mat=%x\n",me,vec,mat))
			continue;
		}

		if (XFERMATX(mat) == TOUCHED || MDIAG(mat))
		{
			/* set XFERMATX to copy */
			SETXFERMATX(mat,COPY);

			PRINTDEBUG(dddif,3,("%2d: VectorXferCopy():  v=%x mat=%x XFERMATX=%d\n",me,vec,mat,XFERMATX(mat)))

			sizeArray[nmat++] = (MDIAG(mat)?MSIZE(mat):2*MSIZE(mat));
		}
		else 
		{
			SETXFERMATX(MADJ(mat),TOUCHED);
		}
	}

	PRINTDEBUG(dddif,2,("%2d:  VectorXferCopy(): v=%x AddData nmat=%d\n",me,vec,nmat))

	DDD_XferAddDataX(nmat,TypeMatrix,sizeArray);
}

void VectorGatherConnX (DDD_OBJ obj, int cnt, DDD_TYPE type_id, void **Data)
{
	VECTOR *vec = (VECTOR *)obj;
	CONNECTION *conn;
	int nconn=0;
	int Size;

	PRINTDEBUG(dddif,3,("%2d:  VectorGatherConnX(): v=%x ID=%d cnt=%d type=%d veobj=%d\n",me,vec,ID(VOBJECT(vec)),cnt,type_id,OBJT(vec)))
	if (cnt<=0) return;

	for (conn=VSTART((VECTOR *) vec); conn!=NULL; conn=MNEXT(conn))
	{
		if (XFERMATX(conn)==COPY)
		{
			IFDEBUG(dddif,0)
			if (cnt<nconn+1)
			{
				PRINTDEBUG(dddif,0,("%2d:  VectorGatherConnX(): v=%x cnt=%d nconn=%d type=%d veobj=%d\n",me,vec,cnt,nconn,type_id,OBJT(vec)))
				return;
			}
			ENDDEBUG

			Size = (MDIAG(conn)?MSIZE(conn):2*MSIZE(conn));
			PRINTDEBUG(dddif,3,("%2d:  VectorGatherConnX(): v=%x conn=%x Size=%d nodetoID=%d\n",me,vec,conn,Size,ID(VOBJECT(MDEST(conn)))))
			memcpy(Data[nconn],MMYCON(conn),Size);

			/* save pointer to destination vector */
			if (MDEST(CMATRIX0(MMYCON(conn)))==vec && !MDIAG(conn)) 
				MDEST(CMATRIX0((CONNECTION *)Data[nconn])) = MDEST(conn);

			nconn++;
		}				
	}
}

void VectorScatterConnX (DDD_OBJ obj, int cnt, DDD_TYPE type_id, void **Data)
{
	VECTOR *vec = (VECTOR *)obj;
	CONNECTION *conn,*prev;
	GRID *theGrid = NULL;
	int nconn=0;
	INT RootType, DestType, MType, ds, Diag, Size, i;

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	Diag = 1; /* TODO: Is diagonal element?? */
	ds;   /* TODO: How big is ds?        */

	PRINTDEBUG(dddif,3,("%2d:  VectorScatterConnX(): v=%x cnt=%d type=%d veobj=%d\n",me,vec,cnt,type_id,OBJT(vec)))
	if (cnt<=0) return;

	Diag  = MDIAG((MATRIX *)Data[nconn]);
	Size  = MSIZE((MATRIX *)Data[nconn]);
	if (!Diag) Size  *= 2;
	if (MSIZEMAX<Size)
	{
		PRINTDEBUG(dddif,0,("%2d:  VectorScatterConnX(): Size=%d but MSIZEMAX=%d\n",Size,MSIZEMAX))
		return;
	}

	conn = (CONNECTION *)GetMem(dddctrl.currMG->theHeap,Size,FROM_BOTTOM);
	if (conn==NULL)
	{
		UserWriteF("%2d:  VectorScatterConnX(): can't get mem for conn=%x\n",conn);
		return;
	}

	PRINTDEBUG(dddif,4,("%2d:  VectorScatterConnX(): v=%x conn=%x Size=%d\n",me,vec,conn,Size))
	memcpy(conn,Data[nconn++],Size);

	/* look which matrix belongs to that vector 				*/
	/* TODO: change this to faster macro sequence if stable 	*/
	if (!MDIAG(conn))
	{
		if (XFERMATX(CMATRIX0(conn))==COPY)			conn = CMATRIX0(conn);
		else if (XFERMATX(CMATRIX1(conn))==COPY)	
		{
			conn = CMATRIX1(conn);
			/* restore pointer to destination vector */
			MDEST(conn) = MDEST(CMATRIX0(MMYCON(conn)));
			MDEST(CMATRIX0(MMYCON(conn))) = vec;
		}
		else UserWrite("%2d NodeScatterEdge(): 	NO copy flag in conn=%x\n",me,conn);
	}

	/* TODO: this loop is no nice programming */
	for (i=1,VSTART((VECTOR *)vec)=conn; i<cnt; i++,MNEXT(prev)=conn)
	{
		Diag  = MDIAG((MATRIX *)Data[nconn]);
		Size  = MSIZE((MATRIX *)Data[nconn]);
		if (!Diag) Size  *= 2;
		if (MSIZEMAX<Size)
		{
			UserWrite("%2d:  VectorScatterConnX(): Size=%d but MSIZEMAX=%d\n",Size,MSIZEMAX);
			return;
		}

		prev = conn;
		conn = (CONNECTION*)GetMem(dddctrl.currMG->theHeap,Size,FROM_BOTTOM);
		if (conn==NULL)
		{
			UserWrite("%2d:  VectorScatterConnX(): ERROR can't get mem for conn=%x\n",conn);
		}

		PRINTDEBUG(dddif,4,("%2d:  VectorScatterConnX(): v=%x conn=%x Size=%d\n",me,vec,conn,Size))
		memcpy(conn,Data[nconn++],Size);

		/* look which matrix belongs to that vector 				*/
		/* TODO: change this to faster macro sequence if stable 	*/
		if (!MDIAG(conn))
		{
			if (XFERMATX(CMATRIX0(conn))==COPY)			conn = CMATRIX0(conn);
			else if (XFERMATX(CMATRIX1(conn))==COPY)
			{
				conn = CMATRIX1(conn);
				/* restore pointer to destination vector */
				MDEST(conn) = MDEST(CMATRIX0(MMYCON(conn)));
				MDEST(CMATRIX0(MMYCON(conn))) = vec;
			}
			else UserWrite("%2d NodeScatterEdge(): 	NO copy flag in conn=%x\n",me,conn);
		}
	}
	
	MNEXT(conn) = NULL;

	NC(theGrid) += cnt;
}

void VectorObjMkCons(DDD_OBJ obj)
{
	CONNECTION *conn;
	VECTOR *vector	= (VECTOR *) obj;
	VECTOR *vectorto;

	PRINTDEBUG(dddif,2,("%2d: VectorObjMkCons(): v=%x VEOBJ=%d\n",me,vector,OBJT(vector)))

	for (conn=VSTART(vector); conn!=NULL; conn=MNEXT(conn))
	{
		if (MDIAG(conn)) continue;

		/* TODO: Durch schnelle Version ersetzen */
		if (XFERMATX(conn)==COPY) {
			vectorto = MDEST(conn);
		}
		else
		{
			if (XFERMATX(MADJ(conn)) != COPY) 
			{
				UserWriteF("%2d VectorObjMkCons():     NO copy flag in conn with matrix=%x matrix=%x\n",me,conn,MADJ(conn));
			}
			continue;
		}
		/* reconstruct pointers of adj matrix */
		MNEXT(MADJ(conn)) = MNEXT(VSTART(vectorto));
		MNEXT(VSTART(vectorto)) = MADJ(conn);
		MDEST(MADJ(conn)) = vector;
	}

}


/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeivertex											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typebvertex											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

void VertexUpdate (DDD_OBJ obj)
{
	VERTEX  *pv = (VERTEX *) obj;
	VERTEX  *after;
	GRID  *theGrid;

	PRINTDEBUG(dddif,1,("%2d: VertexUpdate(): v=%x I/BVOBJ=%d\n",me,pv,OBJT(pv)))

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	after = LASTVERTEX(theGrid);

        /* insert in vertex list */
        if (after==NULL)
        {
                SUCCV(pv) = FIRSTVERTEX(theGrid);
                PREDV(pv) = NULL;
                if (SUCCV(pv)!=NULL) PREDV(SUCCV(pv)) = pv;
                else LASTVERTEX(theGrid) = pv;
                FIRSTVERTEX(theGrid) = pv;
        }
        else
        {
                SUCCV(pv) = SUCCV(after);
                PREDV(pv) = after;
                if (SUCCV(pv)!=NULL) PREDV(SUCCV(pv)) = pv;
                else LASTVERTEX(theGrid) = pv;
                SUCCV(after) = pv;
        }

        /* counters */
        theGrid->nVert++;

		/* update ID of vertex */
		/* TODO: change to global id */
		ID(pv) = (theGrid->mg->vertIdCounter)++;

	/* update corner information for corner vertices on level 0 grid */
	PRINTDEBUG(dddif,3,("%2d: VertexUpdate(): ID(%x)=%d\n",me,pv,ID(pv)))
	if (theGrid->level==0 && ID(pv)<dddctrl.currMG->numOfCorners) {
		dddctrl.currMG->corners[ID(pv)] = pv;
	}
}

void BVertexXferCopy (DDD_OBJ obj, int proc, int prio)
{
	int nvseg=0;
	VERTEX   *ver = (VERTEX *)obj;
	VSEGMENT *vseg;

	for (vseg=VSEG((VERTEX *)ver); vseg!=NULL; vseg=NEXTSEG(vseg))
	{
		nvseg++;
	}

	PRINTDEBUG(dddif,2,("%2d:  BVertexXferCopy(): v=%x AddData nvseg=%d\n",me,ver,nvseg))

	if (nvseg>0)	DDD_XferAddData(nvseg,TypeVSegment); 
}

void BVertexGatherVSegment (DDD_OBJ ver, int cnt, DDD_TYPE type_id, void *Data)
{
	char *data;
    INT id;
	VSEGMENT *vseg;

	data = (char *)Data;

	PRINTDEBUG(dddif,3,("%2d:  BVertexGatherVSegment(): v=%x nvseg=%d type=%d bvobj=%d\n",me,ver,cnt,type_id,OBJT(ver)))

	for (vseg=VSEG((VERTEX *)ver); vseg!=NULL; vseg=NEXTSEG(vseg))
	{
		PRINTDEBUG(dddif,4,("%2d:  BVertexGatherVSegment(): v=%x vseg=%x\n",me,ver,vseg))

		memcpy(data,vseg,sizeof(VSEGMENT));
		/* copy segment id too */
        id = Patch_GetPatchID(VS_PATCH(vseg));
		memcpy(data+sizeof(VSEGMENT),&id,sizeof(INT));
		data += CEIL(sizeof(VSEGMENT)+sizeof(INT));
	}
}

void BVertexScatterVSegment (DDD_OBJ ver, int cnt, DDD_TYPE type_id, void *Data)
{
	char *data;
	int i;
	INT segmentid;
	VSEGMENT *vseg,*prev;
	GRID *theGrid = NULL;

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	data = (char *)Data;

	PRINTDEBUG(dddif,3,("%2d: BVertexScatterVSegment(): v=%x nvseg=%d\n",me,ver,cnt))

	PRINTDEBUG(dddif,4,("%2d: BVertexScatterVSegment(): v=%x vseg=%x size=%d\n",me,ver,vseg,CEIL(sizeof(VSEGMENT))))

	vseg = (VSEGMENT *)GetMem(dddctrl.currMG->theHeap,sizeof(VSEGMENT),FROM_BOTTOM);
	memcpy(vseg,data,sizeof(VSEGMENT));
	memcpy(&segmentid,data+sizeof(VSEGMENT),sizeof(INT));
	data += CEIL(sizeof(VSEGMENT)+sizeof(INT));
	VS_PATCH(vseg) = Patch_GetPatchByID(dddctrl.currMG->theBVP,segmentid); 

	for (i=1,VSEG((VERTEX *)ver)=vseg; i<cnt; i++,NEXTSEG(prev)=vseg)
	{
		PRINTDEBUG(dddif,4,("%2d: BVertexScatterVSegment(): v=%x vseg=%x size=%d\n",me,ver,vseg,CEIL(sizeof(VSEGMENT))))

		prev = vseg;
		vseg = (VSEGMENT *)GetMem(dddctrl.currMG->theHeap,sizeof(VSEGMENT),FROM_BOTTOM);
		memcpy(vseg,data,sizeof(VSEGMENT));
		memcpy(&segmentid,data+sizeof(VSEGMENT),sizeof(INT));
		data += CEIL(sizeof(VSEGMENT)+sizeof(INT));
		VS_PATCH(vseg) = Patch_GetPatchByID(dddctrl.currMG->theBVP,segmentid); 
	}
	
	MNEXT(vseg) = NULL;
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typenode												*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

void NodeCopyManip(DDD_OBJ copy)
{
	NODE *node	= (NODE *) copy;

	PRINTDEBUG(dddif,2,("%2d: NodeCopyManip(): n=%x NDOBJ=%d\n",me,node,OBJT(node)))
}

void NodeDestructor(DDD_OBJ obj)
{
	NODE *node	= (NODE *) obj;

	PRINTDEBUG(dddif,2,("%2d: NodeDestructor(): n=%x NDOBJ=%d\n",me,node,OBJT(node)))
}

void NodeObjInit(DDD_OBJ obj)
{
	NODE *node	= (NODE *) obj;

	PRINTDEBUG(dddif,2,("%2d: NodeObjInit(): n=%x NDOBJ=%d\n",me,node,OBJT(node)))
}

void NodeObjMkCons(DDD_OBJ obj)
{
	LINK *link;
	NODE *node	= (NODE *) obj;
	NODE *nodeto;

	PRINTDEBUG(dddif,2,("%2d: NodeObjMkCons(): n=%x NDOBJ=%d\n",me,node,OBJT(node)))

	for (link=START(node); link!=NULL; link=NEXT(link))
	{
		PRINTDEBUG(dddif,3,("%2d: NodeObjMkCons(): XFERLINK(link)=%d\n",me,XFERLINK(link)))
		/* TODO: Durch schnelle Version ersetzen */
		if (XFERLINK(link)==COPY) {
			nodeto = NBNODE(link);

			/* restore pointer from vector to its edge */
			if (dddctrl.edgeData) 
				VOBJECT(EDVECTOR(MYEDGE(link))) = (void*)MYEDGE(link);	
		}
		else
		{
			if (XFERLINK(REVERSE(link)) == COPY) 
			{
				continue;
			}
			else
			{
				PRINTDEBUG(dddif,0,("%2d NodeObjMkCons():     NO copy flag in edge with link=%x link\n",me,link,REVERSE(link)))
				continue;
			}
		}
		NEXT(REVERSE(link)) = START(nodeto);
		START(nodeto) = REVERSE(link);
	}

	/* reconstruct node pointer */
	if (dddctrl.nodeData) VOBJECT(NVECTOR(node)) = (void*)node;

}


/****************************************************************************/
/*																			*/
/* Function:  NodeUpdate     										        */
/*																			*/
/* Purpose:   update information related to a node.    						*/
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the node to handle							*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

void NodeUpdate (DDD_OBJ obj)
{
	NODE  *node = (NODE *)obj;
	NODE  *after;
	GRID  *theGrid;

	PRINTDEBUG(dddif,1,("%2d: NodeUpdate(): n=%x NDOBJ=%d\n",me,node,OBJT(node)))

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	after = LASTNODE(theGrid);

        /* insert in vertex list */
        if (after==NULL)
        {
                SUCCN(node) = theGrid->firstNode;
                PREDN(node) = NULL;
                if (SUCCN(node)!=NULL) PREDN(SUCCN(node)) = node;
                theGrid->firstNode = node;
                if (theGrid->lastNode==NULL) theGrid->lastNode = node;
        }
        else
        {
                SUCCN(node) = SUCCN(after);
                PREDN(node) = after;
                if (SUCCN(node)!=NULL) PREDN(SUCCN(node)) = node; else theGrid->lastNode = node;
                SUCCN(after) = node;
        }

		START(node) = NULL;

        /* incremant counter */
        theGrid->nNode++;

		/* TODO: change to global id */
		ID(node) = (theGrid->mg->nodeIdCounter)++;
}

/****************************************************************************/
/*																			*/
/* Function:  NodeXferCopy											        */
/*																			*/
/* Purpose:   initiate dependent copy of data related to a node.	 		*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the object which is transfered to proc		*/
/*			  int		proc:	destination processor for that object		*/
/*			  int		prio:	priority of object new local object			*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

void NodeXferCopy (DDD_OBJ obj, int proc, int prio)
{
	int		nlink = 0;
	int		Size,i=0;
	LINK	*link;
	NODE	*node = (NODE *)obj;
	VECTOR	*vec = NULL;
	VECTOR	*vectors[MAX_EDGES];

	PRINTDEBUG(dddif,1,("%2d: NodeXferCopy(): n=%x COPYFLAG already set for LINK=%x\n",me,node,link))

	bzero(vectors,MAX_EDGES*sizeof(VECTOR *));

	/* add links of node */
	for (link=START(node); link!=NULL; link=NEXT(link))
	{

		if (XFERLINK(link) == COPY) 
		{
			PRINTDEBUG(dddif,3,("%2d: NodeXferCopy(): n=%x COPYFLAG already set for LINK=%x\n",me,node,link))
			continue;
		}


		/* check whether corresponding node of this edge is also transferred */
		if (XFERLINK(link) == TOUCHED)
		{
			/* set XFERLINK to copy */
			SETXFERLINK(link,COPY);

			PRINTDEBUG(dddif,3,("%2d: NodeXferCopy():  n=%x link=%x XFERLINK=%d\n",me,node,link,XFERLINK(link)))

			/* increment counter for links to copy for this node */
			nlink++;

			/* store vector of this edge */	 
			if (dddctrl.edgeData)
			{
				vec = EDVECTOR(MYEDGE(link));

				  /* TODO: */
				  /* CAUTION: must be called after XferAddData because */
                  /* of reference                                      */
				  /* to primary element, which is here node			   */
/*
   DDD_XferCopyObjX(PARHDR(vec), proc, prio, dddctrl.currMG->theFormat->VectorSizes[VTYPE(vec)]);
*/
				if (i<MAX_EDGES) vectors[i++] = vec;
				else 
					PRINTDEBUG(dddif,0,("%2d: NodeXferCopy():  ERROR node=%x vec=%x number of vectors to send too big! ENLARGE MAXEDGES=%d\n",me,node,vec,MAX_EDGES));
			}
		}
		else
		{
			/* set XFERFLAG of corresponding link to TOUCHED */
			SETXFERLINK(REVERSE(link),TOUCHED);
		}
	}

	/* TODO: only send link whose corresponding node is also sent */
	/* CAUTION: must be called before any XferCopy because of reference */
	/*			to primary element, which is here the node      		*/
   	if (nlink>0)
	{
		PRINTDEBUG(dddif,2,("%2d: NodeXferCopy():  n=%x AddData nlink=%d\n",me,node,nlink))

		if (nlink >0)	DDD_XferAddData(nlink,TypeEdge);    
	}

	/* send vectors */
	if (dddctrl.edgeData) {
		if (vectors[0] != NULL)
			Size = sizeof(VECTOR)-sizeof(DOUBLE)+dddctrl.currMG->theFormat->VectorSizes[VTYPE(vectors[0])];
		for (i=0; i<MAX_EDGES && vectors[i]!=NULL; i++) {
			PRINTDEBUG(dddif,3,("%2d: NodeXferCopy():  n=%x EDGEVEC=%x size=%d\n",me,node,vec,Size))
			DDD_XferCopyObjX(PARHDR(vectors[i]), proc, prio, Size);
		}
	}

	/* copy vertex */
	PRINTDEBUG(dddif,2,("%2d: NodeXferCopy(): n=%x Xfer v=%x\n",me,node,MYVERTEX(node)))

	DDD_XferCopyObj(PARHDRV(MYVERTEX(node)), proc, PrioVertex);

	/* copy vector if defined */
	if (dddctrl.nodeData)
	  {
		vec = NVECTOR(node);
		Size = sizeof(VECTOR)-sizeof(DOUBLE)+dddctrl.currMG->theFormat->VectorSizes[VTYPE(vec)];

		PRINTDEBUG(dddif,2,("%2d: NodeXferCopy(): n=%x Xfer NODEVEC=%x size=%d\n",me,node,vec,Size))

		  DDD_XferCopyObjX(PARHDR(vec), proc, prio, Size);
	  }
}

void NodeGatherEdge (DDD_OBJ n, int cnt, DDD_TYPE type_id, void *Data)
{
	char	*data;
	LINK	*link;
	EDGE	*edge;
	NODE	*node = (NODE *) n;

	data = (char *)Data;

	PRINTDEBUG(dddif,3,("%2d:NodeGatherEdge(): n=%x cnt=%d type=%d ndobj=%d\n",me,node,cnt,type_id,OBJT(node)))

	/* copy edge(s) of node */
	for (link=START(node); link!=NULL; link=NEXT(link))
	{
		PRINTDEBUG(dddif,4,("%2d:NodeGatherEdge():  n=%x link=%x XFERLINK=%d\n",me,node,link,XFERLINK(link)))

		switch (XFERLINK(link))
		{
		case COPY:
				PRINTDEBUG(dddif,4,("%2d:NodeGatherEdge():   n=%x copy link=%x\n",me,node,link))
				memcpy(data,MYEDGE(link),sizeof(EDGE));
				data += CEIL(sizeof(EDGE));

				/* clear XFERFLAG */
/*
				SETXFERLINK(link,CLEAR);
*/
				break;
		case TOUCHED:
				/* clear XFERFLAG */
				SETXFERLINK(link,CLEAR);
				break;
		default:
				break;
		}
	}
}

void NodeScatterEdge (DDD_OBJ n, int cnt, DDD_TYPE type_id, void *Data)
{
	char	*data;
	int		i;
	EDGE	*edge;
	LINK	*link,*prev;
	NODE	*node = (NODE *) n;
	GRID	*grid;

	grid = GRID_ON_LEVEL(dddctrl.currMG,0);
	data = (char *)Data;

	/* increment counter */
	grid->nEdge+=cnt;

	PRINTDEBUG(dddif,3,("%2d:NodeScatterEdge(): n=%x cnt=%d type=%d ndobj=%d\n",me,node,cnt,type_id,OBJT(node)))

	edge = (EDGE *)GetMem(dddctrl.currMG->theHeap,sizeof(EDGE),FROM_BOTTOM);
	PRINTDEBUG(dddif,4,("%2d:NodeScatterEdge(): n=%x edge=%x size=%d\n",me,node,edge,CEIL(sizeof(EDGE))))


	/* copy data out of message */
	memcpy(edge,data,sizeof(EDGE));

	data+=CEIL(sizeof(EDGE));

	/* look which link belongs to that node 				*/
	/* TODO: change this to faster macro sequence if stable */
	if (XFERLINK(LINK0(edge))==COPY)			link = LINK0(edge);
	else if (XFERLINK(LINK1(edge))==COPY)		link = LINK1(edge);
	else PRINTDEBUG(dddif,0,("%2d NodeScatterEdge(): 	NO copy flag in edge=%x\n",me,edge))

	for (i=0,START(node)=link; i<cnt-1; i++,NEXT(prev)=link)
	{
		prev = link;

		/* CAUTION: perhaps need to change into +CEIL(SIZEOF(EDGE)) */
		edge = (EDGE *)GetMem(dddctrl.currMG->theHeap,sizeof(EDGE),FROM_BOTTOM);
		PRINTDEBUG(dddif,4,("%2d:NodeScatterEdge(): n=%x edge=%x size=%d\n",me,node,edge,CEIL(sizeof(EDGE))))

		/* copy data out of message */
		memcpy(edge,data,sizeof(EDGE));
		data+=CEIL(sizeof(EDGE));
		link++;

		/* look which link belongs to that node 				*/
		/* TODO: change this to faster macro sequence if stable */
		if (XFERLINK(LINK0(edge))==COPY)		link = LINK0(edge);
		else if (XFERLINK(LINK1(edge))==COPY)	link = LINK1(edge);
		else PRINTDEBUG(dddif,0,("%2d NodeScatterEdge(): 	NO copy flag in edge=%x\n",me,edge))
	}
	
	MNEXT(link) = NULL;
}
	
/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeelement											*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* Function:  ElementUpdate  												*/
/*																			*/
/* Purpose:   update information related to an element.						*/
/*			  current implementation only for level 0 grids					*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the element to handle						*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

void ElementLDataConstructor (DDD_OBJ obj)
{
	int			i,sides;
	ELEMENT		*pe = (ELEMENT *)obj;
	ELEMENT		*after = NULL;
	GRID		*theGrid = NULL;
	int         level = DDD_InfoAttr(PARHDRE(pe));

	PRINTDEBUG(dddif,2,("%2d: ElementUpdate(): e=%x EOBJ=%d l=%d\n",\
		me,pe,OBJT(pe),level))

	theGrid = GetGridOnDemand(dddctrl.currMG,level);
	after = LASTELEMENT(theGrid);
	SETLEVEL(pe,level);
	sides = 0;

	/* insert in element list */
	if (after==NULL)
	{
		SUCCE(pe) = theGrid->elements;
		PREDE(pe) = NULL;
		if (SUCCE(pe)!=NULL) 
			PREDE(SUCCE(pe)) = pe;
		else 
			theGrid->lastelement = pe;
		theGrid->elements = pe;
	}
	else
	{
		SUCCE(pe) = SUCCE(after);
		PREDE(pe) = after;
		if (SUCCE(pe)!=NULL)
			PREDE(SUCCE(pe)) = pe;
		else
			theGrid->lastelement = pe;
		SUCCE(after) = pe;
	}

	if (OBJT(pe)==BEOBJ)
	{
		for (i=0; i<SIDES_OF_ELEM(pe); i++) 
			if (SIDE(pe,i)!=NULL) sides++;
		theGrid->nSide += sides;
	}

	theGrid->nElem++;

	/* TODO: in global id umrechnen */
	ID(pe) = (theGrid->mg->elemIdCounter)++;
}

/****************************************************************************/
/*																			*/
/* Function:  ElementXferCopy	  											*/
/*																			*/
/* Purpose:   initiate dependent copy of data related to an element. 		*/
/*			  this handler is implemented for an arbitrary element type.	*/
/*																			*/
/* Input:	  DDD_OBJ	obj:	the object which is transfered to proc		*/
/*			  int		proc:	destination processor for that object		*/
/*			  int		prio:	priority of object new local object			*/
/*																			*/
/* Output:	  void															*/
/*																			*/
/****************************************************************************/

void ElementXferCopy (DDD_OBJ obj, int proc, int prio)
{
	int      i,nelemside;
	int		 Size;
	ELEMENT  *pe	=	(ELEMENT *)obj;
	VECTOR	 *vec;
	NODE	 *node;

	PRINTDEBUG(dddif,1,("%d: ElementXferCopy(): pe=%x proc=%d prio=%d EOBJT=%d\n", me, obj, proc, prio, OBJT(pe)))

	/* add element sides; must be done before any XferCopyObj-call! herein */
	/* or directly after XferCopyObj-call */
	nelemside = 0;
	if (OBJT(pe)==BEOBJ)
	{
		for (i=0; i<SIDES_OF_ELEM(pe); i++)
			if (SIDE(pe,i) != NULL) nelemside++;

		PRINTDEBUG(dddif,2,("%2d: ElementXferCopy():  e=%x AddData nelemside=%d\n",me,pe,nelemside))

		if (nelemside>0) DDD_XferAddData(nelemside,TypeElementSide);
	}

	/* send father element. this is a temporary solution, which will
	   be removed lateron when thinking about real load balancing. */
	{
		ELEMENT *f = EFATHER(pe);
		if (f!=0)
			DDD_XferCopyObjX(PARHDRE(f), proc, prio,
				(OBJT(f)==BEOBJ) ? BND_SIZE(TAG(f)) : INNER_SIZE(TAG(f))
        	);
	}
	
	/* copy corner nodes */
	for(i=0; i<CORNERS_OF_ELEM(pe); i++)
	{
		node = CORNER(pe,i);
		if (XFERNODE(node) == 0)
		{
			PRINTDEBUG(dddif,2,("%2d:ElementXferCopy():  e=%x Xfer n=%x i=%d\n",me,pe,node,i))
			DDD_XferCopyObj(PARHDR(node), proc, PrioNode);
			SETXFERNODE(node,1);
		}
	}

	/* copy element vector */
	if (dddctrl.elemData)
	  {
		vec = EVECTOR(pe);
		Size = sizeof(VECTOR)-sizeof(DOUBLE)+dddctrl.currMG->theFormat->VectorSizes[VTYPE(vec)];
		
		PRINTDEBUG(dddif,2,("%2d:ElementXferCopy(): e=%x ELEMVEC=%x size=%d\n",me,pe,vec,Size))

		  DDD_XferCopyObjX(PARHDR(vec), proc, PrioVector, Size);
	  }

	/* copy sidevectors */
	if (dddctrl.sideData)
	  {
		for (i=0; i<SIDES_OF_ELEM(pe); i++)
		  {
			vec = SVECTOR(pe,i);
			Size = sizeof(VECTOR)-sizeof(DOUBLE)+dddctrl.currMG->theFormat->VectorSizes[VTYPE(vec)];
			if (XFERVECTOR(vec) == 0)
			  {
				PRINTDEBUG(dddif,2,("%2d:ElementXferCopy(): e=%x SIDEVEC=%x size=%d\n",me,pe,vec,Size))
				  
			    DDD_XferCopyObjX(PARHDR(vec), proc, prio, Size);
				SETXFERVECTOR(vec,1);
			  }
		  }
	  }
}


void ElemGatherElemSide (DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data)
{
	int i;
	char *data;
    INT id;
	ELEMENTSIDE *elemside;
	ELEMENT  *pe	=	(ELEMENT *)obj;

	data = (char *)Data;

	PRINTDEBUG(dddif,3,("%2d:  ElemGatherElemSide(): pe=%x nelemside=%d type=%d bvobj=%d\n",me,pe,cnt,type_id,OBJT(pe)))

	for (i=0; i<SIDES_OF_ELEM(pe); i++)
	{
		if (SIDE(pe,i) != NULL) 
		{
			id = Patch_GetPatchID(VS_PATCH(SIDE(pe,i)));
			PRINTDEBUG(dddif,4,("%2d:  ElemGatherElemSide(): e=%x elemside=%x side=%d segid=%d\n",me,pe,SIDE(pe,i),i,id))
			memcpy(data,SIDE(pe,i),sizeof(ELEMENTSIDE));
			/* copy segment id too */
            memcpy(data+sizeof(ELEMENTSIDE),&id,sizeof(INT));
			data += CEIL(sizeof(ELEMENTSIDE)+sizeof(INT));
		}
	}
}

void ElemScatterElemSide (DDD_OBJ obj, int cnt, DDD_TYPE type_id, void *Data)
{
	char *data;
	int i;
	INT segmentid;
	ELEMENTSIDE *elemside;
	ELEMENT  *pe	=	(ELEMENT *)obj;
	GRID *theGrid = NULL;

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);
	data = (char *)Data;

	PRINTDEBUG(dddif,3,("%2d: ElemScatterElemSide(): pe=%x nelemside=%d type=%d obj=%d\n",me,pe,cnt,type_id,OBJT(pe)))

	for (i=0; i<SIDES_OF_ELEM(pe); i++)
	{
		if (SIDE(pe,i) != NULL) 
		{
			elemside = (ELEMENTSIDE *)GetMem(dddctrl.currMG->theHeap,sizeof(ELEMENTSIDE),FROM_BOTTOM);
			PRINTDEBUG(dddif,4,("%2d:  ElemScatterElemSide(): e=%x elemside=%x side=%d size=%d\n",me,pe,SIDE(pe,i),i,CEIL(sizeof(ELEMENTSIDE))))

			/* copy out elementside out of message and restore SEGDESC * */ 
			memcpy(elemside,data,sizeof(ELEMENTSIDE));
			memcpy(&segmentid,data+sizeof(ELEMENTSIDE),sizeof(INT));
			data += CEIL(sizeof(ELEMENTSIDE)+sizeof(INT));
			VS_PATCH(elemside) = Patch_GetPatchByID(dddctrl.currMG->theBVP,segmentid); 
			SET_SIDE(pe,i,elemside);

			PRINTDEBUG(dddif,4,("%2d:  ElemScatterElemSide(): e=%x elemside=%x side=%d segid=%d\n",me,pe,SIDE(pe,i),i,segmentid))

			/* put into double linked list */
			PREDS(elemside) = NULL;
			SUCCS(elemside) = FIRSTELEMSIDE(theGrid);
			if (FIRSTELEMSIDE(theGrid)!=0)
				PREDS(FIRSTELEMSIDE(theGrid)) = elemside;
			FIRSTELEMSIDE(theGrid) = elemside;
		}
	}
}


void ElementObjMkCons(DDD_OBJ obj)
{
	int i;
	ELEMENT  *pe	=	(ELEMENT *)obj;

	/* reconstruct pointer from vectors */
	if (dddctrl.elemData) VOBJECT(EVECTOR(pe)) = (void*)pe;

	if (dddctrl.sideData)
	{
		for (i=0; i<SIDES_OF_ELEM(pe); i++) VOBJECT(SVECTOR(pe,i)) = (void*)pe;
	}
}

/****************************************************************************/
/****************************************************************************/
/*																			*/
/*		handlers for typeedge    	 										*/
/*																			*/
/****************************************************************************/
/****************************************************************************/

/* CAUTION: */
/* TODO: delete this, Update handlers are not called for DDD data objects */
void EdgeUpdate (DDD_OBJ obj)
{
	EDGE *pe = (EDGE *)obj;
	LINK *link0,*link1;
	GRID *theGrid = NULL;

	PRINTDEBUG(dddif,2,("%2d:EdgeUpdate(): edge=%x EDOBJT=%d\n",me,pe,OBJT(pe)))

	theGrid = GRID_ON_LEVEL(dddctrl.currMG,0);

	/* increment counter */
	theGrid->nEdge++;
}	


void ddd_HandlerInit (void)
{

	DDD_HandlerRegister(TypeVector,
		HANDLER_UPDATE,		VectorUpdate,
		HANDLER_XFERCOPY,		VectorXferCopy,
		HANDLER_XFERGATHERX,	VectorGatherConnX,
		HANDLER_XFERSCATTERX,	VectorScatterConnX,
		HANDLER_OBJMKCONS,		VectorObjMkCons,
		HANDLER_END
	);	

	DDD_HandlerRegister(TypeIVertex,
		HANDLER_UPDATE,		VertexUpdate,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeBVertex,
		HANDLER_UPDATE,		VertexUpdate,
		HANDLER_XFERCOPY,		BVertexXferCopy,
		HANDLER_XFERGATHER,		BVertexGatherVSegment,
		HANDLER_XFERSCATTER,	BVertexScatterVSegment,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeNode,
		HANDLER_COPYMANIP,			NodeCopyManip,
		HANDLER_LDATACONSTRUCTOR,	NodeObjInit,
		HANDLER_DESTRUCTOR,			NodeDestructor,
		HANDLER_OBJMKCONS,			NodeObjMkCons,
		HANDLER_UPDATE,				NodeUpdate,
		HANDLER_XFERCOPY,			NodeXferCopy,
		HANDLER_XFERGATHER,			NodeGatherEdge,
		HANDLER_XFERSCATTER,		NodeScatterEdge,
		HANDLER_END
	);


	#ifdef __TWODIM__
	DDD_HandlerRegister(TypeTrElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeTrBElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_XFERGATHER,		ElemGatherElemSide,
		HANDLER_XFERSCATTER,	ElemScatterElemSide,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeQuElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeQuBElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_XFERGATHER,		ElemGatherElemSide,
		HANDLER_XFERSCATTER,	ElemScatterElemSide,
		HANDLER_END
	);
	#endif

	#ifdef __THREEDIM__
	DDD_HandlerRegister(TypeTeElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_END
	);

	DDD_HandlerRegister(TypeTeBElem,
		HANDLER_LDATACONSTRUCTOR, ElementLDataConstructor,
		HANDLER_OBJMKCONS,		ElementObjMkCons,
		HANDLER_XFERCOPY,		ElementXferCopy,
		HANDLER_XFERGATHER,		ElemGatherElemSide,
		HANDLER_XFERSCATTER,	ElemScatterElemSide,
		HANDLER_END
	);
	#endif

	DDD_HandlerRegister(TypeEdge,
		HANDLER_UPDATE,	EdgeUpdate,
		HANDLER_END
	);
}

#endif /* ModelP */
