
/*
  Another bitvector test, based on
  reading input files.
*/

#include "bitvector.h"
#include "vectlib.h"
#include "timers.h"

#include <stdio.h>
#include <iostream> // for getopt

using namespace std;

int width;

class mytraverse : public bitvector_traverse {
  bitvector* fill;
public:
  mytraverse(bitvector* f) : bitvector_traverse() { fill = f; }
  virtual void Visit(long index) {
    fill->Set(index);
  }
};

long NextIndex()
{
  long answer = 0;
  int count = fread(&answer, width, 1, stdin);
  if (count<1) return -1;
  return answer;
}

int Usage(const char* name)
{
  fprintf(stderr, "Usage: %s\n\nSwitches:\n", name);
  fprintf(stderr, "\t?: Print usage and exit\n");
  fprintf(stderr, "\ta: add only, do not delete\n");
  fprintf(stderr, "\tl: use linked list only\n");
  fprintf(stderr, "\tt: use tree only\n");
  fprintf(stderr, "\ts: show the final set\n");
  fprintf(stderr, "\tw n: sets word width to n (default 1)\n");
  return 0;
}


int main(int argc, char** argv)
{
  printf("%s\n", SV_LibraryVersion());
  char* name = argv[0];
  // process command line
  int ch;
  width = 1;
  bool show = false;
  bool addonly = false;
  int tree = -1;
  for (;;) {
    ch = getopt(argc, argv, "?altsw:");
    if (ch<0) break;
    switch (ch) {
      case 'a':
          addonly = true;
          break;

      case 'l':
          tree = 0;
          break;

      case 't':
          tree = 1;
          break;

      case 's':
          show = true;
          break;

      case 'w':
          if (optarg) {
            int r = atoi(optarg);
            if (r>0 && r<4) {
              width = r;
            }
          }
          break;

      default:
          return Usage(name);
    } // switch
  } // loop to process arguments

  // allocate an explicit bitvector
  long bvdim = 1;
  for (int w=0; w<width; w++)
  bvdim *= 256;
  bitvector set1(bvdim);
  set1.UnsetAll();

  // allocate a sparse bitvector
  sparse_bitvector *set2;
  switch (tree) {
    case 0:
          printf("Using linked list set\n");
          set2 = SV_CreateSparseBitvector(1000000000, 1000000000);
          break;

    case 1:
          printf("Using tree set\n");
          set2 = SV_CreateSparseBitvector(0, 0);
          break;

    default:
          printf("Using list/tree combination set\n");
          set2 = SV_CreateSparseBitvector(-1, -1);
  }
  if (0==set2) {
    printf("Couldn't create sparse bitvector\n");
    return 1;
  }

  // read standard input and add / remove elements accordingly

  timer sw;
  sw.Start();
  long ops = 0;
  for (;;) {
    long add = NextIndex();
    if (add < 0) break;

    set1.Set(add);
    set2->SetElement(add);
    ops++;

    if (addonly) continue;

    long remove = NextIndex();
    if (remove < 0) break;
    
    set1.Unset(remove);
    set2->ClearElement(remove);
    ops++;
  }
  sw.Stop();

  printf("Done processing file, %ld operations\n", ops);
  double proctime = sw.User_Seconds();
  printf("Took %lg seconds\n", proctime);
  if (ops) {
    proctime /= ops;
    printf("Time per operation: %lg seconds\n", proctime);
  }
  printf("Comparing sets...\n");
  bitvector set3(bvdim);
  set3.UnsetAll();
  mytraverse foo(&set3); 
  set2->Traverse(&foo);

  for (long i=0; i<bvdim; i++) if (set1.IsSet(i) != set3.IsSet(i)) {
    printf("Discrepency:\nset 1 ");
    if (set1.IsSet(i))  printf("contains"); 
    else                printf("does not contain");
    printf(" element %ld\nset 2 ", i);
    if (set3.IsSet(i))  printf("contains"); 
    else                printf("does not contain");
    printf(" element %ld\n", i);
    return 1;
  }

  printf("The sets match\n");
  if (show) {
    printf("Both are {");
    bool written = false;
    for (long i=0; i<bvdim; i++) if (set1.IsSet(i)) {
      if (written) printf(", ");
      written = true;
      printf("%ld", i);
    }
    printf("}\n");
  }
  printf("The sets currently contain %ld elements\n", set2->NumNonzeroes());

  delete set2;
  printf("That's all, folks\n");
  return 0;
}
