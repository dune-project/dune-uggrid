#ifndef DUNE_UGGRID_PARALLEL_PPIF_PPIFCONTEXT_HH
#define DUNE_UGGRID_PARALLEL_PPIF_PPIFCONTEXT_HH 1

#include <memory>

#if ModelP
#  include <mpi.h>
#endif

#include <dune/uggrid/parallel/ppif/ppiftypes.hh>

namespace PPIF {

/**
 * context object for low-level parallel communication
 */
class PPIFContext
{
public:
  /**
   * constructor.
   *
   * By default `MPI_COMM_WORLD` is used for communication.
   *
   * \note This is a MPI collective operation (invokes `MPI_Comm_dup`)
   */
  PPIFContext();

  /**
   * destructor
   *
   * \note This is a MPI collective operation (invokes `MPI_Comm_free`)
   */
  ~PPIFContext();

  /**
   * our rank; in the interval [0, procs)
   */
  int me() const
  { return me_; }

  /**
   * rank of the master process (usually 0)
   */
  int master() const
  { return master_; }

  /**
   * number of processes
   */
  int procs() const
  { return procs_; }

protected:
  int me_ = 0;
  int master_ = 0;
  int procs_ = 1;

#if ModelP || DOXYGEN
public:
  /**
   * maximum number of downtree nodes max
   * log2(P)
   */
  static const int MAXT = 15;

  /**
   * constructor.
   *
   * \note This is a MPI collective operation (invokes `MPI_Comm_dup`)
   */
  explicit PPIFContext(const MPI_Comm& comm);

  /**
   * MPI communicator
   */
  MPI_Comm comm() const
    { return comm_; }

  int dimX() const
    { return dims_[0]; }

  int dimY() const
    { return dims_[1]; }

  int dimZ() const
    { return dims_[2]; }

  /**
   * degree of downtree nodes
   */
  int degree() const
    { return degree_; }

  /**
   * channel uptree
   */
  VChannelPtr uptree() const
    { return uptree_; }

  /**
   * channels downtree (may be empty)
   */
  const std::array<VChannelPtr, MAXT>& downtree() const
    { return downtree_; }

  /**
   * number of processors in subtree
   */
  const std::array<int, MAXT>& slvcnt() const
    { return slvcnt_; }

protected:
  MPI_Comm comm_ = MPI_COMM_NULL;

  std::array<int, 3> dims_ = {{1, 1, 1}};
  int degree_ = 0;
  VChannelPtr uptree_ = nullptr;
  std::array<VChannelPtr, MAXT> downtree_ = {};
  std::array<int, MAXT> slvcnt_ = {};

  friend void InitPPIF(PPIFContext&);
  friend void ExitPPIF(PPIFContext&);
#endif
};

} /* namespace PPIF */

#endif
