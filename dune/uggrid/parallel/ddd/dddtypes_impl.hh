#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_IMPL_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_IMPL_HH 1

#include "dddtypes.hh"

namespace DDD {

/**
 * \brief DDD object header, include this into all parallel object structures
 *
 * Some remarks:
 *
 * - don't touch the member elements of DDD_HEADER in the
 *   application program, they will be changed in further
 *   DDD versions!
 *
 * - use DDD functional interface for accessing the header fields;
 *   elements which are not accessible via the DDD functional interface
 *   should not be accessed by the application program anyway,
 */
struct DDD_HEADER
{
  /* control word elements */
  unsigned char typ;
  unsigned char prio;
  unsigned char attr;
  unsigned char flags;

  /** global object array index */
  unsigned int myIndex;

  /** global id */
  DDD_GID gid;

  /* 4 unused bytes in current impl. */
  char empty[4];
};


/**
 * \brief record for coupling of local object with foreign objects
 */
struct COUPLING
{
  COUPLING* _next;
  unsigned short _proc;
  unsigned char prio;
  unsigned char _flags;
  DDD_HDR obj;
};

/**
 * \brief description of one element in DDD object structure description
 *
 * \todo in CPP_FRONTEND only one of the versions for C_FRONTEND and
 *       F_FRONTEND is needed, depending on setting of desc->storage.
 *       this should be a union for memory efficiency reasons.
 */
struct ELEM_DESC
{
  /** element offset from object address */
  int offset;

  /** gbits array, if type==EL_GBITS */
  std::unique_ptr<unsigned char[]> gbits;

  /** size of this element */
  std::size_t size;

  /** type of element, one of EL_xxx */
  int type;

  /* if type==EL_OBJPTR, the following entries will be used: */

  /** DDD_TYPE of ref. destination */
  DDD_TYPE _reftype;

  /* if reftype==DDD_TYPE_BY_HANDLER, we must use a handler for
   * determining the reftype on-the-fly (additional parameter during
   * TypeDefine with EL_OBJPTR)
   */
  HandlerGetRefType reftypeHandler;
};


/**
 * \brief single DDD object structure description
 */
struct TYPE_DESC
{
  /** current TypeMode (DECLARE/DEFINE) */
  int mode;

  /** textual object description */
  const char* name;

  /** number of current call to TypeDefine */
  int currTypeDefCall;

  /* if C_FRONTEND or (CPP_FRONTEND and storage==STORAGE_STRUCT): */

  /** flag: real ddd type (with header)? */
  bool hasHeader;

  /** offset of header from begin of obj   */
  int offsetHeader;

  /** max. number of elements per TYPE_DESC */
  static const int MAX_ELEMDESC = 64;

  /** element description array */
  ELEM_DESC element[MAX_ELEMDESC];

  /** number of elements in object         */
  int nElements;

  /** size of object, correctly aligned    */
  std::size_t size;

  /* pointer to handler functions: */

  HandlerLDATACONSTRUCTOR handlerLDATACONSTRUCTOR;
  HandlerDESTRUCTOR handlerDESTRUCTOR;
  HandlerDELETE handlerDELETE;
  HandlerUPDATE handlerUPDATE;
  HandlerOBJMKCONS handlerOBJMKCONS;
  HandlerSETPRIORITY handlerSETPRIORITY;
  HandlerXFERCOPY handlerXFERCOPY;
  HandlerXFERDELETE handlerXFERDELETE;
  HandlerXFERGATHER handlerXFERGATHER;
  HandlerXFERSCATTER handlerXFERSCATTER;
  HandlerXFERGATHERX handlerXFERGATHERX;
  HandlerXFERSCATTERX handlerXFERSCATTERX;
  HandlerXFERCOPYMANIP handlerXFERCOPYMANIP;


  /** 2D matrix for comparing priorities */
  std::unique_ptr<DDD_PRIO[]> prioMatrix;

  /** default mode for PrioMerge */
  int prioDefault;

  /* redundancy for efficiency: */

  /** number of outside references         */
  int nPointers;

  /** mask for fast type-dependent copy    */
  std::unique_ptr<unsigned char[]> cmask;
};

} /* namespace DDD */

#endif
