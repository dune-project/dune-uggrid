set(SOURCES
  amg_blas.cc
  amg_coarsen.cc
  amg_iter.cc
  amg_low.cc
  amg_solve.cc
  amg_sp.cc
  amg_ug.cc)

ug_add_dim_libs(amg OBJECT SOURCES ${SOURCES})
