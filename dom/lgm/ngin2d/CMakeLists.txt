# prefix to use instead of 'yy'
set(PARSERPREFIX ng2d)

# generate header file, rename prefix
bison_target(nginyacc2 ngin-yacc.yy ${CMAKE_CURRENT_BINARY_DIR}/ngin-yac.cc
  COMPILE_FLAGS "-p ${PARSERPREFIX}")
flex_target(nginlex2 ngin-lex.ll ${CMAKE_CURRENT_BINARY_DIR}/ngin-lex.cc
  COMPILE_FLAGS "-P${PARSERPREFIX}")

add_definitions("-D_2")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dom/lgm)
add_library(ngin2 OBJECT ${BISON_nginyacc2_OUTPUTS} ${FLEX_nginlex2_OUTPUTS})
