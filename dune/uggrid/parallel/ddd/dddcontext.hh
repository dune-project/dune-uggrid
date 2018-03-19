#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH 1

#include <memory>
#include <dune/uggrid/parallel/ddd/dddtypes.hh>
#include <dune/uggrid/parallel/ppif/ppiftypes.hh>

namespace DDD {

namespace Basic {

struct LowCommContext
{
  MSG_TYPE *MsgTypes = nullptr;
  MSG_DESC *SendQueue = nullptr;
  MSG_DESC *RecvQueue = nullptr;
  int nSends = 0;
  int nRecvs = 0;
  char *theRecvBuffer;
  LC_MSGHANDLE *theRecvArray = nullptr;
  MSG_DESC *FreeMsgDescs = nullptr;

  AllocFunc DefaultAlloc, SendAlloc, RecvAlloc;
  FreeFunc DefaultFree, SendFree, RecvFree;
};

struct NotifyContext
{
  NOTIFY_INFO* allInfoBuffer;
  NOTIFY_DESC* theDescs;
  int* theRouting;
  int maxInfos;
  int lastInfo;
  int nSendDescs;
};

struct TopoContext
{
  PPIF::VChannelPtr* theTopology;
  DDD_PROC* theProcArray;
};

} /* namespace Basic */

class DDDContext {
public:
  DDDContext(const std::shared_ptr<PPIF::PPIFContext>& ppifContext);

  const PPIF::PPIFContext& ppifContext() const
    { return *ppifContext_; }

  PPIF::PPIFContext& ppifContext()
    { return *ppifContext_; }

  /**
   * \see PPIF::PPIFContext::me()
   */
  int me() const;

  /**
   * \see PPIF::PPIFContext::procs()
   */
  int procs() const;

  /**
   * \see PPIF::PPIFContext::isMaster()
   */
  bool isMaster() const;

  /**
   * return pointer to user data
   */
  void* data()
    { return data_; }

  /**
   * return const pointer to user data
   */
  const void* data() const
    { return data_; }

  Basic::LowCommContext& lowCommContext()
    { return lowCommContext_; }

  const Basic::LowCommContext& lowCommContext() const
    { return lowCommContext_; }

  Basic::NotifyContext& notifyContext()
    { return notifyContext_; }

  Basic::TopoContext& topoContext()
    { return topoContext_; }

  const Basic::TopoContext& topoContext() const
    { return topoContext_; }

protected:
  std::shared_ptr<PPIF::PPIFContext> ppifContext_;
  void* data_ = nullptr;
  Basic::LowCommContext lowCommContext_;
  Basic::NotifyContext notifyContext_;
  Basic::TopoContext topoContext_;
};

} /* namespace DDD */

#endif
