#ifndef DUNE_UGGRID_PARALLEL_PPIF_PPIFTYPES_HH
#define DUNE_UGGRID_PARALLEL_PPIF_PPIFTYPES_HH 1

#include <cstddef>

namespace PPIF {

/**
 * opaque type for communication channels
 */
struct VChannel;

using VChannelPtr = VChannel*;

/**
 * opaque type for messages
 */
struct Msg;

using msgid = Msg*;

constexpr static msgid NO_MSGID = nullptr;

/**
 * indicate support for context objects in PPIF
 */
#ifndef DUNE_UGGRID_HAVE_PPIFCONTEXT
#  define DUNE_UGGRID_HAVE_PPIFCONTEXT 1
#endif

class PPIFContext;

} /* namespace PPIF */

#endif
