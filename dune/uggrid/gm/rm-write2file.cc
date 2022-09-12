#include "config.h"

#include <dune/common/exceptions.hh>

#include "rm-write2file.h"

#ifdef UG_DIM_3
static NS_DIM_PREFIX REFRULE Empty_Rule =
{-1,-1,NS_DIM_PREFIX NO_CLASS,-1,
 {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
 -1,
 {{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},
  {-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},
  {-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},
  {-1,-1}},
 {{-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1},
 {-1,{-1,-1,-1,-1,-1,-1,-1,-1},{-1,-1,-1,-1,-1,-1},-1}}};


template<class T>
int writeArray(FILE* stream, T* array, int const n)
{
  int num_chars = 0;
  for(int i=0; i < n;i++)
  {
    num_chars += std::fprintf( stream,"%d,", int(array[i]));
  }
  return num_chars;
}

const char* tag2string(int const tag)
{
  switch(tag)
  {
  case NS_DIM_PREFIX TETRAHEDRON:
    return "TETRAHEDRON";
  case NS_DIM_PREFIX PYRAMID:
    return "PYRAMID";
  case NS_DIM_PREFIX PRISM:
    return "PRISM";
  case NS_DIM_PREFIX HEXAHEDRON:
    return "HEXAHEDRON";
  default:
    DUNE_THROW(Dune::Exception, "tag2string: unknown tag " << tag);
  }
}

const char* class2string(int const rclass)
{
  switch(rclass)
  {
  case NS_DIM_PREFIX NO_CLASS:
    return "NO_CLASS";
  case NS_DIM_PREFIX YELLOW_CLASS:
    return "YELLOW_CLASS";
  case NS_DIM_PREFIX GREEN_CLASS:
    return "GREEN_CLASS";
  case NS_DIM_PREFIX RED_CLASS:
    return "RED_CLASS";
  case NS_DIM_PREFIX SWITCH_CLASS:
    return "SWITCH_CLASS";
  default:
    DUNE_THROW(Dune::Exception, "class2string: unknown class " << rclass);
  }
}


int WriteSonData(std::FILE* const stream, NS_DIM_PREFIX sondata const& son)
{
  using std::fprintf;
  int num_chars = 0;

  // tag
  int tag = son.tag;
  const char* tag_s = tag2string(tag);
  num_chars += fprintf( stream,"{%s,{",tag_s);

  // corners
  num_chars += writeArray(stream, son.corners, MAX_CORNERS_OF_ELEM_DIM);
  num_chars += fprintf( stream,"},{");

  // nb
  num_chars += writeArray(stream, son.nb, MAX_SIDES_OF_ELEM_DIM);
  num_chars += fprintf(stream ,"},%d}",son.path);

  return num_chars;

}

void WriteRule2File(std::FILE* const stream, NS_DIM_PREFIX REFRULE const& theRule)
{
  using std::fprintf;

  // only used for alignment of comments
  int comment_row = 80;

  // tag, mark, rclass, nsons
  const char* tag_s = tag2string(theRule.tag);
  const char* rclass_s = class2string(theRule.rclass);
  int char0 = fprintf( stream,"  {%s,%d,%s,%d,",tag_s ,theRule.mark,rclass_s,theRule.nsons);
  fprintf( stream,"%*s// tag, mark, rclass, nsons\n", comment_row - char0," ");

  // pattern
  char0 = fprintf( stream,"   {");
  char0 += writeArray(stream, theRule.pattern, MAX_NEW_CORNERS_DIM);
  fprintf( stream,"},%*s// pattern\n", comment_row - char0 -2, " ");

  // pat
  char0 = std::fprintf( stream,"   %d,",theRule.pat);
  fprintf( stream,"%*s// pat\n", comment_row - char0, " ");

  // sonandnode
  bool alreadyCommented = false;
  char0 = fprintf( stream,"   {");
  for(int i=0; i < MAX_NEW_CORNERS_DIM; i++)
  {
    char0 += fprintf( stream,"{%d,%d},",theRule.sonandnode[i][0],theRule.sonandnode[i][1]);
    // new line after 6 sons
    if( (i%6) ==0 && i !=0)
    {
      // comment
      if (!alreadyCommented)
      {
        fprintf( stream,"%*s// sonandnode", comment_row - char0, " ");
        alreadyCommented = !alreadyCommented;
      }
      fprintf( stream,"\n    ");
    }
  }
  fprintf( stream,"},\n");

  // sons
  alreadyCommented = false;
  char0 = fprintf( stream,"   {");
  for(int i=0; i<MAX_SONS_DIM; i++)
  {
    char0 += WriteSonData(stream, theRule.sons[i]);
    char0 += fprintf( stream,",");
    // comment
    if(!alreadyCommented)
    {
      fprintf( stream,"%*s// sons", comment_row - char0, " ");
      alreadyCommented = !alreadyCommented;
    }
    fprintf( stream,"\n    ");
  }

  fprintf( stream,"}}");
}

void Write2File(std::FILE* stream, std::vector<NS_DIM_PREFIX REFRULE> const& rules, std::vector<NS_PREFIX SHORT> const& patterns)
{
  std::fprintf( stream, "// This file was generated by \"gm/rm3-writeRefRules2file\"\n\n");

  std::fprintf( stream, "static const std::size_t nTetrahedronRefinementRules = %zd;\n", rules.size() );
  std::fprintf( stream, "static REFRULE tetrahedronRefinementRules[] =\n{\n");

  for (std::size_t i=0; i < rules.size(); i++)
  {
    std::fprintf( stream, "  // Rule %lu\n",i);
    WriteRule2File(stream, rules[i]);
    std::fprintf( stream,",\n\n");
  }

  std::fprintf( stream, "};\n");

  std::fprintf( stream, "static const NS_PREFIX SHORT pattern2RuleTetrahedron[%zd] = {", patterns.size());
  writeArray(stream, patterns.data(), patterns.size());
  std::fprintf( stream,"};\n");
}


START_UGDIM_NAMESPACE
static void CorrectRule40 (refrule  *theRule)
{
  for (int i : {1,4,8})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule41 (refrule  *theRule)
{
  for (int i : {1,4,7})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule52 (refrule  *theRule)
{
  for (int i : {2,8})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}
static void CorrectRule53 (refrule  *theRule)
{
  for (int i : {2,7})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule85 (refrule  *theRule)
{
  // i == 4
  auto&& son = theRule->sons[4];
  std::swap(son.corners[0],son.corners[1]);
  std::swap(son.nb[1],son.nb[2]);
}

static void CorrectRule86 (refrule  *theRule)
{
  // i == 4
  auto&& son = theRule->sons[4];
  std::swap(son.corners[0],son.corners[1]);
  std::swap(son.nb[1],son.nb[2]);
}

static void CorrectRule111 (refrule  *theRule)
{
  for (int i : {6,8})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule112 (refrule  *theRule)
{
  for (int i : {6,7})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule135 (refrule  *theRule)
{
  for (int i : {1,3,11})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule136 (refrule  *theRule)
{
  for (int i : {2,3,11})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule155 (refrule  *theRule)
{
  for (int i : {5,7,9})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule156 (refrule  *theRule)
{
  for (int i : {5,8,9})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule183 (refrule  *theRule)
{
  for (int i : {3,7,11})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static void CorrectRule184 (refrule  *theRule)
{
  for (int i : {2,8,11})
  {
    auto&& son = theRule->sons[i];
    std::swap(son.corners[0],son.corners[1]);
    std::swap(son.nb[1],son.nb[2]);
  }
}

static int FReadRule (FILE *stream, REFRULE *theRule)
{
  int ns,p0,p1,p2,p3,p4,p5,pat;

  // init tag
  theRule->tag = TETRAHEDRON;

  if (fscanf(stream,"%d  %d %d %d %d %d %d  %d",&ns,&p0,&p1,&p2,&p3,&p4,&p5,&pat)!=8) return (1);

  theRule->nsons = ns;
  theRule->pattern[0] = p0;
  theRule->pattern[1] = p1;
  theRule->pattern[2] = p2;
  theRule->pattern[3] = p3;
  theRule->pattern[4] = p4;
  theRule->pattern[5] = p5;
  theRule->pat = pat;

  // MAXEDGES = 16 for tetrahedra
  int type,from,to,side;
  for (int i=0; i<16; i++)
  {
    if (fscanf(stream," %d %d %d %d",&type,&from,&to,&side)!=4) return (1);
  }
  // MAX_SONS = 12 for tetrahedra
  for (int i=0; i<12; i++)
  {
    int c0,c1,c2,c3,n0,n1,n2,n3,pa;
    if (fscanf(stream," %d %d %d %d %d %d %d %d %d",&c0,&c1,&c2,&c3,&n0,&n1,&n2,&n3,&pa)!=9) return (1);
    theRule->sons[i].tag = TETRAHEDRON;
    if (c0 == 10) c0 = 14;
    theRule->sons[i].corners[0] = c0;
    if (c1 == 10) c1 = 14;
    theRule->sons[i].corners[1] = c1;
    if (c2 == 10) c2 = 14;
    theRule->sons[i].corners[2] = c2;
    if (c3 == 10) c3 = 14;
    theRule->sons[i].corners[3] = c3;
    theRule->sons[i].nb[0]          = n0;
    theRule->sons[i].nb[1]          = n1;
    theRule->sons[i].nb[2]          = n2;
    theRule->sons[i].nb[3]          = n3;
    theRule->sons[i].path           = pa;
    static const int nSidesOfTetrahedron = 4; /* == SIDES_OF_TAG(TETRAHEDRON) */
    for (int j=0; j < nSidesOfTetrahedron; j++)
      if (theRule->sons[i].nb[j]>=TET_RULE_FATHER_SIDE_OFFSET)
        theRule->sons[i].nb[j] += FATHER_SIDE_OFFSET-TET_RULE_FATHER_SIDE_OFFSET;
  }

  // NEWCORNERS = 7 for tetrahedra
  // read edge node information
  int sn0,sn1;
  for (int i=0; i<6; i++)
  {
    if (fscanf(stream," %d %d",&sn0,&sn1)!=2) return (1);
    theRule->sonandnode[i][0] = sn0;
    theRule->sonandnode[i][1] = sn1;
  }
  // read center node information
  if (fscanf(stream," %d %d",&sn0,&sn1)!=2) return (1);
  theRule->sonandnode[10][0] = sn0;
  theRule->sonandnode[10][1] = sn1;

  // init Pattern and pat for center node
  if (theRule->sonandnode[10][0] != NO_CENTER_NODE)
  {
    theRule->pattern[10] = 1;
    theRule->pat |= (1<<10);
  }

  return (0);
}


void readTetrahedronRules(FILE* stream, std::vector<NS_DIM_PREFIX REFRULE>& rules, std::vector<NS_PREFIX SHORT>& patterns)
{
  USING_UGDIM_NAMESPACE
  USING_UG_NAMESPACE

  {
    int nRules;
    int nPatterns;

    // read Rules and nPatterns from file
    if (std::fscanf(stream,"%d %d\n",&nRules,&nPatterns)!=2)
      DUNE_THROW(Dune::Exception, "failed to read nRules and nPatterns from file");

    rules.assign(nRules, Empty_Rule);
    patterns.assign(nPatterns, -1);
  }

  // read Rules
  {
    int i = 0;
    for (auto& rule : rules) {
      rule.mark = i++;
      rule.rclass = RED_CLASS|GREEN_CLASS;
      if (FReadRule(stream, &rule))
        DUNE_THROW(Dune::Exception, "failed to read rule");
    }
  }

  for (auto& pattern : patterns) {
    if (std::fscanf(stream,"%hd",&pattern)!=1)
      DUNE_THROW(Dune::Exception, "failed to read pattern from file");
  }

  // bug fix
  CorrectRule40(&rules[40]);
  CorrectRule41(&rules[41]);
  CorrectRule52(&rules[52]);
  CorrectRule53(&rules[53]);
  CorrectRule85(&rules[85]);
  CorrectRule86(&rules[86]);
  CorrectRule111(&rules[111]);
  CorrectRule112(&rules[112]);
  CorrectRule135(&rules[135]);
  CorrectRule136(&rules[136]);
  CorrectRule155(&rules[155]);
  CorrectRule156(&rules[156]);
  CorrectRule183(&rules[183]);
  CorrectRule184(&rules[184]);
}

END_UGDIM_NAMESPACE
#endif //__THREEDIM
