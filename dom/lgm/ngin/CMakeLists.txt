# prefix to use instead of 'yy'
set(PARSERPREFIX ng)

# generate header file, rename prefix
bison_target(nginyacc ngin-yacc.yy ${CMAKE_CURRENT_BINARY_DIR}/ngin-yac.cc
  COMPILE_FLAGS "-p ${PARSERPREFIX}")
flex_target(nginlex ngin-lex.ll ${CMAKE_CURRENT_BINARY_DIR}/ngin-lex.cc
  COMPILE_FLAGS "-P${PARSERPREFIX}")

add_definitions("-DUG_DIM_3")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/dom/lgm)
add_library(ngin3 OBJECT ${BISON_nginyacc_OUTPUTS} ${FLEX_nginlex_OUTPUTS})
