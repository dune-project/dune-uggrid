#include "config.h"

#include <cstdio>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/test/testsuite.hh>

#include <initug.h>

#include "evm.h"
#include "gm.h"
#include "rm.h"

USING_UGDIM_NAMESPACE
USING_UG_NAMESPACE

static bool CheckVolumes(REFRULE const *Rule)
{
  bool pass = true;

  DOUBLE_VECTOR coords [MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM];
  DOUBLE_VECTOR coord;
  DOUBLE_VECTOR p[MAX_CORNERS_OF_ELEM];

  /* this function work only for TETRAHEDRA!!! */
  int tag = TETRAHEDRON;

  /* reset coords */
  for (int i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
    V3_CLEAR(coords[i])

  /* compute coordinates of corners */
  int j = 0;
  for (int i=0; i<CORNERS_OF_REF(tag); i++,j++)
    V3_COPY(LOCAL_COORD_OF_REF(tag,i),coords[j])

  /* compute coordinates of midnodes */
  for (int i=0; i<EDGES_OF_REF(tag); i++,j++)
  {
    V3_CLEAR(coord)
    for (int k=0; k<CORNERS_OF_EDGE; k++)
    {
      V3_ADD(LOCAL_COORD_OF_REF(tag,CORNER_OF_EDGE_REF(tag,i,k)),coord,coord)
    }
    V3_SCALE(1.0/CORNERS_OF_EDGE,coord)
    V3_COPY(coord,coords[j]);
  }

  /* compute coordinates of sidenodes */
  for (int i=0; i<SIDES_OF_REF(tag); i++,j++)
  {
    V3_CLEAR(coord)
    for (int k=0; k<CORNERS_OF_SIDE_REF(tag,i); k++)
      V3_ADD(LOCAL_COORD_OF_REF(tag,CORNER_OF_SIDE_REF(tag,i,k)),coord,coord)
      V3_SCALE(1.0/CORNERS_OF_SIDE_REF(tag,i),coord)
      V3_COPY(coord,coords[j]);
  }

  /* compute coordinates of center node */
  V3_CLEAR(coord)
  for (int i=0; i<CORNERS_OF_REF(tag); i++)
  {
    V3_ADD(LOCAL_COORD_OF_REF(tag,i),coord,coord)
  }
  V3_SCALE(1.0/CORNERS_OF_REF(tag),coord)
  V3_COPY(coord,coords[j]);

  for (int i=0; i<MAX_CORNERS_OF_ELEM+MAX_NEW_CORNERS_DIM; i++)
  {
    printf("CheckVolumes(): i=%d x=%.8f y=%.8f z=%.8f\n",
           i,coords[i][0],coords[i][1],coords[i][2]);
  }

  /* check the son volumes being really greater than zero */
  double sum=0.0;
  for (int i=0; i<NSONS_OF_RULE(Rule); i++)
  {
    for (int k=1; k<4; k++)
    {
      V3_SUBTRACT(coords[SON_CORNER_OF_RULE(Rule,i,k)],coords[SON_CORNER_OF_RULE(Rule,i,0)],p[k])
    }

    double sp;
    // triple product of son
    V3_VECTOR_PRODUCT(p[1],p[2],p[0])
    V3_SCALAR_PRODUCT(p[0],p[3],sp)

    if (sp<=0.0)
    {
      printf("negative volume=%f for son=%d rule=%d\n",sp,i,MARK_OF_RULE(Rule));
      pass = false;
    }
    sum += sp;

  }

  // check if volume of sons add up to volume of original tetrahedra
  // 6*Volume_original == sum over 6*volume_sons
  // exception for first rule
  if( sum != 1.0 && Rule->mark != 0)
  {
    printf("Rule %d :sum over sons = %f != 1\n",Rule->mark,sum);
    pass = false;
  }

  return pass;
}

int main(int argc, char** argv)
{
  Dune::MPIHelper::instance(argc, argv);
  InitUg(&argc, &argv);

  bool pass = true;

  std::printf("Testing %d refinement rules for the tetrahedron...\n", MaxRules[TETRAHEDRON]);
  REFRULE* rules = RefRules[TETRAHEDRON];
  for (int i = 0; i < MaxRules[TETRAHEDRON]; ++i)
    pass = pass && CheckVolumes(rules + i);

  ExitUg();

  return pass ? 0 : 1;
}
