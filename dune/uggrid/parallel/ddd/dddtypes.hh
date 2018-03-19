#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH 1

namespace DDD {

class DDDContext;

namespace Basic {

/*
 * types used by lowcomm
 */
struct MSG_TYPE;
struct MSG_DESC;
using LC_MSGHANDLE = MSG_DESC*;
using AllocFunc = void* (*)(std::size_t);
using FreeFunc = void (*)(void*);

} /* namespace Basic */

} /* namespace DDD */

#endif
