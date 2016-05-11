if(UG_ENABLE_2D)
  add_definitions("-DUG_DIM_2")
  add_library(gg2 OBJECT  ggmain.cc ggaccel.cc ggm.cc)
  if(DUNE_BUILD_BOTH_LIBS)
    # For shared libraries we need position independent code
    set_property(TARGET gg2 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  endif()
endif()
