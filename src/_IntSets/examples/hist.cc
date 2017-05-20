
/*
  Sparse integer vector test.
  Reads standard input and gives a histogram of input sequences.
*/

#include "vectlib.h"
#include "timers.h"

#include <stdio.h>
#include <iostream> // for getopt

using namespace std;

int width;

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
  fprintf(stderr, "\tn: numerical output\n");
  fprintf(stderr, "\tl: use linked list only\n");
  fprintf(stderr, "\tt: use tree only\n");
  fprintf(stderr, "\tw n: sets word width to n (default 1)\n");
  return 0;
}

int Digits(long x)  // me being lazy
{
  if (x<10) return 1;
  return 1+Digits(x/10);
}

void ShowNumerical(SV_Vector* x)
{
  long maxcount = 0;
  long maxindex = 0;
  for (long i=0; i<x->nonzeroes; i++) {
    if (x->index[i] > maxindex)  
      maxindex = x->index[i];
    if (x->i_value[i] > maxcount)
      maxcount = x->i_value[i];
  }
   
  int cd = Digits(maxcount);
  int id = Digits(maxindex);

  int col = 0;
  for (long i=0; i<x->nonzeroes; i++) {
    if (col + cd + id + 6 > 79) {
      printf("\n");
      col = 0;
    }
    printf("%*ld: %*ld    ", id, x->index[i], cd, x->i_value[i]);
    col += cd + id + 6;
  }
}

void Write(long x)
{
  int w;
  long first = 1;
  for (w=1; w<width; w++) first *= 256;
  fputc('"', stdout);
  while (first>0) {
    char c = char(x / first);
    x %= first;
    first /= 256;
    fputc(c, stdout);
  }
  fputc('"', stdout);
}

void ShowString(SV_Vector* x)
{
  long maxcount = 0;
  for (long i=0; i<x->nonzeroes; i++) {
    if (x->i_value[i] > maxcount)
      maxcount = x->i_value[i];
  }
   
  int cd = Digits(maxcount);

  int col = 0;
  for (long i=0; i<x->nonzeroes; i++) {
    if (col + cd + width + 8 > 79) {
      printf("\n");
      col = 0;
    }
    Write(x->index[i]);
    printf(": %*ld    ", cd, x->i_value[i]);
    col += cd + width + 8;
  }
}

int main(int argc, char** argv)
{
  printf("%s\n", SV_LibraryVersion());
  char* name = argv[0];
  // process command line
  int ch;
  width = 1;
  int tree = -1;
  bool numerical = false;
  for (;;) {
    ch = getopt(argc, argv, "?nltw:");
    if (ch<0) break;
    switch (ch) {
      case 'n':
          numerical = true;
          break;

      case 'l':
          tree = 0;
          break;

      case 't':
          tree = 1;
          break;

      case 'w':
          if (optarg) {
            int r = atoi(optarg);
            if (r>0 && r<5) {
              width = r;
            }
          }
          break;

      default:
          return Usage(name);
    } // switch
  } // loop to process arguments

  // allocate vector
  sparse_intvector *hist;
  switch (tree) {
    case 0:
        printf("Using linked list multiset\n");
        hist = SV_CreateSparseIntvector(1000000000, 1000000000);
        break;

    case 1:
        printf("Using tree multiset\n");
        hist = SV_CreateSparseIntvector(0, 0);
        break;

    default:
        printf("Using list/tree combination multiset\n");
        hist = SV_CreateSparseIntvector(-1, -1);
  }
  if (0==hist) {
    printf("Couldn't create sparse intvector\n");
    return 1;
  }

  // read standard input and add elements accordingly

  timer sw;
  sw.Start();
  long ops = 0;
  for (;;) {
    long add = NextIndex();
    if (add < 0) break;

    hist->ChangeElement(add, 1);
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

  hist->ConvertToStatic(true);
  SV_Vector foo;

  hist->ExportTo(&foo);

  if (numerical)
    ShowNumerical(&foo);
  else
    ShowString(&foo);

  printf("\n\nThere are %ld nonzero elements\n", foo.nonzeroes);
  delete hist;
  printf("\nThat's all, folks\n");
  return 0;
}
