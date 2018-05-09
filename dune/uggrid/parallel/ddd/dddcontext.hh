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
  std::vector<NOTIFY_INFO> allInfoBuffer;
  std::vector<NOTIFY_DESC> theDescs;
  std::vector<int> theRouting;
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

namespace Ident {

struct IdentContext
{
  ID_PLIST* thePLists;
  int cntIdents;
  int nPLists;
  IdentMode identMode;
};

} /* namespace Ident */

namespace If {

struct IfCreateContext
{
  IF_DEF theIf[MAX_IF];
  int nIfs;
};

struct IfUseContext
{
  int send_mesgs;
};

} /* namespace If */

namespace Join {

struct JoinContext
{
  /** mode of join module */
  JoinMode joinMode;

  /* description for phase1 message */
  Basic::LC_MSGTYPE phase1msg_t;
  Basic::LC_MSGCOMP jointab_id;

  /* description for phase2 message */
  Basic::LC_MSGTYPE phase2msg_t;
  Basic::LC_MSGCOMP addtab_id;

  /* description for phase3 message */
  Basic::LC_MSGTYPE phase3msg_t;
  Basic::LC_MSGCOMP cpltab_id;

  /* entry points for global sets */
  JIJoinSet   *setJIJoin;
  JIAddCplSet *setJIAddCpl2;
  JIAddCplSet *setJIAddCpl3;
};

} /* namespace Join */

namespace Mgr {

struct CplmgrContext
{
  CplSegm *segmCpl = nullptr;
  COUPLING *memlistCpl = nullptr;
  int *localIBuffer;
  int nCplSegms;
};

struct ObjmgrContext
{
  DDD_GID theIdCount;
};

struct TypemgrContext
{
  int nDescr;
};

} /* namespace Mgr */

namespace Prio {

struct PrioContext
{
  PrioMode prioMode;
};

} /* namespace Prio */

namespace Xfer {

struct CmdmsgContext
{
  Basic::LC_MSGTYPE cmdmsg_t;
  Basic::LC_MSGCOMP undelete_id;
};

struct CplmsgContext
{
  Basic::LC_MSGTYPE cplmsg_t;
  Basic::LC_MSGCOMP delcpl_id;
  Basic::LC_MSGCOMP modcpl_id;
  Basic::LC_MSGCOMP addcpl_id;
};

/**
 * global data for xfer module
 */
struct XferContext
{
  /** mode of xfer module */
  XferMode xferMode;

  /* description for object message */
  Basic::LC_MSGTYPE objmsg_t;
  Basic::LC_MSGCOMP symtab_id, objtab_id;
  Basic::LC_MSGCOMP newcpl_id, oldcpl_id;
  Basic::LC_MSGCOMP objmem_id;


  /* entry points for global sets */
  XICopyObjSet *setXICopyObj;
  XISetPrioSet *setXISetPrio;

  XICopyObj* theXIAddData;

  AddDataSegm* segmAddData = nullptr;
  SizesSegm* segmSizes = nullptr;

#define SLL_MEMBERS(T) Segm##T* segms##T; T* list##T; int n##T;
  SLL_MEMBERS(XIDelCmd)
  SLL_MEMBERS(XIDelObj)
  SLL_MEMBERS(XINewCpl)
  SLL_MEMBERS(XIOldCpl)
  SLL_MEMBERS(XIAddCpl)
  SLL_MEMBERS(XIDelCpl)
  SLL_MEMBERS(XIModCpl)
#undef SLL_MEMBERS
};

} /* namespace Xfer */

struct CouplingContext
{
  std::vector<COUPLING*> cplTable;
  std::vector<short> nCplTable;

  /** number of coupling lists */
  int nCpls;

  /* number of couplings */
  int nCplItems;
};

class DDDContext {
public:
  DDDContext(const std::shared_ptr<PPIF::PPIFContext>& ppifContext, const std::shared_ptr<void>& data);

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
    { return data_.get(); }

  /**
   * return const pointer to user data
   */
  const void* data() const
    { return data_.get(); }

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

  Ident::IdentContext& identContext()
    { return identContext_; }

  const Ident::IdentContext& identContext() const
    { return identContext_; }

  If::IfCreateContext& ifCreateContext()
    { return ifCreateContext_; }

  const If::IfCreateContext& ifCreateContext() const
    { return ifCreateContext_; }

  If::IfUseContext& ifUseContext()
    { return ifUseContext_; }

  Join::JoinContext& joinContext()
    { return joinContext_; }

  const Join::JoinContext& joinContext() const
    { return joinContext_; }

  Mgr::CplmgrContext& cplmgrContext()
    { return cplmgrContext_; }

  const Mgr::CplmgrContext& cplmgrContext() const
    { return cplmgrContext_; }

  Mgr::ObjmgrContext& objmgrContext()
    { return objmgrContext_; }

  Mgr::TypemgrContext& typemgrContext()
    { return typemgrContext_; }

  const Mgr::TypemgrContext& typemgrContext() const
    { return typemgrContext_; }

  Prio::PrioContext& prioContext()
    { return prioContext_; }

  const Prio::PrioContext& prioContext() const
    { return prioContext_; }

  Xfer::CmdmsgContext& cmdmsgContext()
    { return cmdmsgContext_; }

  const Xfer::CmdmsgContext& cmdmsgContext() const
    { return cmdmsgContext_; }

  Xfer::CplmsgContext& cplmsgContext()
    { return cplmsgContext_; }

  const Xfer::CplmsgContext& cplmsgContext() const
    { return cplmsgContext_; }

  Xfer::XferContext& xferContext()
    { return xferContext_; }

  const Xfer::XferContext& xferContext() const
    { return xferContext_; }

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

  std::array<int, OPT_END>& options()
    { return options_; }

  const std::array<int, OPT_END>& options() const
    { return options_; }

protected:
  std::shared_ptr<PPIF::PPIFContext> ppifContext_;
  std::shared_ptr<void> data_;
  Basic::LowCommContext lowCommContext_;
  Basic::NotifyContext notifyContext_;
  Basic::TopoContext topoContext_;
  Ctrl::ConsContext consContext_;
  Ident::IdentContext identContext_;
  If::IfCreateContext ifCreateContext_;
  If::IfUseContext ifUseContext_;
  Join::JoinContext joinContext_;
  Mgr::CplmgrContext cplmgrContext_;
  Mgr::ObjmgrContext objmgrContext_;
  Mgr::TypemgrContext typemgrContext_;
  Prio::PrioContext prioContext_;
  CouplingContext couplingContext_;
  Xfer::CmdmsgContext cmdmsgContext_;
  Xfer::CplmsgContext cplmsgContext_;
  Xfer::XferContext xferContext_;

  std::vector<DDD_HDR> objTable_;
  int nObjs_;

  std::array<TYPE_DESC, MAX_TYPEDESC> typeDefs_;

  std::array<int, OPT_END> options_;
};

} /* namespace DDD */

#endif
