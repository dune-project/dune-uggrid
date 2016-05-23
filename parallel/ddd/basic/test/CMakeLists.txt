# -*- tab-width: 4; indent-tabs-mode: nil -*-
set(TESTPROGS  testbtree)
# programs just to build when "make check" is used
set(check_PROGRAMS  ${TESTPROGS})
# programs to run when "make check" is used
set(TESTS  ${TESTPROGS})
# define the programs
add_executable(testbtree testbtree.cc)
add_dune_ug_flags(testbtree )
#include $(top_srcdir)/am/global-rules
foreach(i ${TESTS})
  add_test(${i} ${i})
endforeach(i ${TESTS})
# We do not want want to build the tests during make all
# but just build them on demand
add_directory_test_target(_test_target)
add_dependencies(${_test_target} ${TESTS} ${COMPILE_XFAIL_TESTS})