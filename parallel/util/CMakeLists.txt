add_library(parutil OBJECT xbc.cc)
add_dune_mpi_flags(parutil OBJECT)
if(DUNE_BUILD_BOTH_LIBS)
  # For shared libraries we need position independent code
  set_property(TARGET parutil PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()
