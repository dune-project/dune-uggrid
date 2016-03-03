set(SOURCES  wpm.cc graph.cc wop.cc initgraph.cc plotproc.cc placer.cc bullet.cc)
# bullet.h graph.h initgraph.h plotproc.h wop.h wpm.h iso.cc iso.h pixel.h)

ug_add_dim_libs(uggraph OBJECT SOURCES ${SOURCES})
