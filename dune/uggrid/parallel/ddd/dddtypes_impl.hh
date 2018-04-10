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

} /* namespace DDD */

#endif
