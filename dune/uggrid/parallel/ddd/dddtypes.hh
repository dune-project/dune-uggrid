#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH 1

namespace DDD {

class DDDContext;

using DDD_TYPE = unsigned int;
using DDD_IF = unsigned int;
using DDD_PROC = unsigned int;
using DDD_PRIO = unsigned int;
using DDD_ATTR = unsigned int;

namespace Basic {

/*
 * types used by lowcomm
 */
struct MSG_TYPE;
struct MSG_DESC;
using LC_MSGHANDLE = MSG_DESC*;
using AllocFunc = void* (*)(std::size_t);
using FreeFunc = void (*)(void*);

/*
 * types used by notify
 */
struct NOTIFY_DESC;
struct NOTIFY_INFO;

} /* namespace Basic */

} /* namespace DDD */

#endif
