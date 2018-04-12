#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH 1

#include <memory>
#include <vector>

#include <dune/uggrid/parallel/ddd/dddconstants.hh>
#include <dune/uggrid/parallel/ddd/dddtypes.hh>
#include <dune/uggrid/parallel/ddd/dddtypes_impl.hh>
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

namespace Ctrl {

struct ConsContext
{
  Basic::LC_MSGTYPE consmsg_t;
  Basic::LC_MSGCOMP constab_id;
};

} /* namespace Ctrl */

namespace If {

struct IfUseContext
{
  int send_mesgs;
};

} /* namespace If */

namespace Mgr {

struct ObjmgrContext
{
  DDD_GID theIdCount;
};

} /* namespace Mgr */

namespace Prio {

struct PrioContext
{
  PrioMode prioMode;
};

} /* namespace Prio */

struct CouplingContext
{
  std::vector<COUPLING*> cplTable;
  std::vector<short> nCplTable;
};

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

  Ctrl::ConsContext& consContext()
    { return consContext_; }

  If::IfUseContext& ifUseContext()
    { return ifUseContext_; }

  Mgr::ObjmgrContext& objmgrContext()
    { return objmgrContext_; }

  Prio::PrioContext& prioContext()
    { return prioContext_; }

  const Prio::PrioContext& prioContext() const
    { return prioContext_; }

  CouplingContext& couplingContext()
    { return couplingContext_; }

  const CouplingContext& couplingContext() const
    { return couplingContext_; }

  const std::vector<DDD_HDR>& objTable() const
    { return objTable_; }

  std::vector<DDD_HDR>& objTable()
    { return objTable_; }

  int nObjs() const
    { return nObjs_; }

  void nObjs(int n)
    { nObjs_ = n; }

  std::array<TYPE_DESC, MAX_TYPEDESC>& typeDefs()
    { return typeDefs_; }

  const std::array<TYPE_DESC, MAX_TYPEDESC>& typeDefs() const
    { return typeDefs_; }

protected:
  std::shared_ptr<PPIF::PPIFContext> ppifContext_;
  void* data_ = nullptr;
  Basic::LowCommContext lowCommContext_;
  Basic::NotifyContext notifyContext_;
  Basic::TopoContext topoContext_;
  Ctrl::ConsContext consContext_;
  If::IfUseContext ifUseContext_;
  Mgr::ObjmgrContext objmgrContext_;
  Prio::PrioContext prioContext_;
  CouplingContext couplingContext_;

  std::vector<DDD_HDR> objTable_;
  int nObjs_;

  std::array<TYPE_DESC, MAX_TYPEDESC> typeDefs_;
};

} /* namespace DDD */

#endif
