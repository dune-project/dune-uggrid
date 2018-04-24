#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDTYPES_HH 1

#include <cinttypes>

namespace DDD {

class DDDContext;

using DDD_GID = std::uint_least64_t;
using DDD_TYPE = unsigned int;
using DDD_IF = unsigned int;
using DDD_PROC = unsigned int;
using DDD_PRIO = unsigned int;
using DDD_ATTR = unsigned int;

using DDD_OBJ = char*;

struct DDD_HEADER;
using DDD_HDR = DDD_HEADER*;

/* handler prototypes */

/* handlers related to certain DDD_TYPE (i.e., member functions) */
using HandlerLDATACONSTRUCTOR = void (*)(DDDContext& context, DDD_OBJ);
using HandlerDESTRUCTOR       = void (*)(DDDContext& context, DDD_OBJ);
using HandlerDELETE           = void (*)(DDDContext& context, DDD_OBJ);
using HandlerUPDATE           = void (*)(DDDContext& context, DDD_OBJ);
using HandlerOBJMKCONS        = void (*)(DDDContext& context, DDD_OBJ, int);
using HandlerSETPRIORITY      = void (*)(DDDContext& context, DDD_OBJ, DDD_PRIO);
using HandlerXFERCOPY         = void (*)(DDDContext& context, DDD_OBJ, DDD_PROC, DDD_PRIO);
using HandlerXFERDELETE       = void (*)(DDDContext& context, DDD_OBJ);
using HandlerXFERGATHER       = void (*)(DDDContext& context, DDD_OBJ, int, DDD_TYPE, void *);
using HandlerXFERSCATTER      = void (*)(DDDContext& context, DDD_OBJ, int, DDD_TYPE, void *, int);
using HandlerXFERGATHERX      = void (*)(DDDContext& context, DDD_OBJ, int, DDD_TYPE, char **);
using HandlerXFERSCATTERX     = void (*)(DDDContext& context, DDD_OBJ, int, DDD_TYPE, char **, int);
using HandlerXFERCOPYMANIP    = void (*)(DDDContext& context, DDD_OBJ);

/* handlers not related to DDD_TYPE (i.e., global functions) */
using HandlerGetRefType = DDD_TYPE (*)(DDDContext& context, DDD_OBJ, DDD_OBJ);

using ExecProcPtr  = int (*)(DDDContext& context, DDD_OBJ);
using ExecProcXPtr = int (*)(DDDContext& context, DDD_OBJ, DDD_PROC, DDD_PRIO);
using ComProcPtr2  = int (*)(DDDContext& context, DDD_OBJ, void *);
using ComProcXPtr  = int (*)(DDDContext& context, DDD_OBJ, void *, DDD_PROC, DDD_PRIO);

/* PRIVATE INTERFACE */

using RETCODE = int;

struct COUPLING;
struct ELEM_DESC;
struct TYPE_DESC;

namespace Basic {

/*
 * types used by lowcomm
 */

/**
 * opaque data type for message types
 */
struct MSG_TYPE;

/**
 * opaque data type for messages
 */
struct MSG_DESC;

/**
 * handle for messages
 */
using LC_MSGHANDLE = MSG_DESC*;

/**
 * handle for message types (on send AND recv side)
 */
using LC_MSGTYPE = MSG_TYPE*;

/**
 * component of message (on send AND recv side)
 */
using LC_MSGCOMP = int;

using AllocFunc = void* (*)(std::size_t);
using FreeFunc = void (*)(void*);

/*
 * types used by notify
 */
struct NOTIFY_DESC;
struct NOTIFY_INFO;

} /* namespace Basic */

namespace Ident {

enum class IdentMode : unsigned char;

struct ID_PLIST;

} /* namespace Ident */

namespace Join {

enum class JoinMode : unsigned char;

struct JIJoinSet;
struct JIAddCplSet;

} /* namespace Join */

namespace Prio {

enum class PrioMode : unsigned char;

} /* namespace Prio */

} /* namespace DDD */

#endif
