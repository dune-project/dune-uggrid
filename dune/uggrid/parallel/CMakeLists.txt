add_subdirectory(ddd)
add_subdirectory(ppif)

if(UG_ENABLE_PARALLEL)
  add_subdirectory(dddif)
endif()
