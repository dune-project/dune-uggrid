if(UG_ENABLE_2D)
  add_definitions("-DUG_DIM_2")
  add_library(gg2 OBJECT  ggmain.cc ggaccel.cc ggm.cc)
endif()
