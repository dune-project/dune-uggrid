#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDCONTEXT_HH 1

#include <memory>
#include <dune/uggrid/parallel/ppif/ppiftypes.hh>

namespace DDD {

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

protected:
  std::shared_ptr<PPIF::PPIFContext> ppifContext_;
  void* data_ = nullptr;
};

} /* namespace DDD */

#endif
