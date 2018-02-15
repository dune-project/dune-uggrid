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

class PPIFContext;

} /* namespace PPIF */

#endif
