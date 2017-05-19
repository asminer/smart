
// $Id$

// Interactive test of state database class.

#include <unistd.h>
#include <stdio.h>
#include "statelib.h"

using namespace StateLib;

bool ReadState(int* state, int size)
{
  for (int i=0; i<size; i++) {
    int foo = scanf("%d", state+i);
    if (0==foo) return false;
    if (EOF==foo) return false;
    if (state[i]<0) return false;
  }
  return true;
}

void PrintState(FILE* out, int* state, int size)
{
  fprintf(out, "[%d", state[0]);
  for (int i=1; i<size; i++) fprintf(out, ", %d", state[i]);
  fprintf(out, "]");
}

int Usage(char* name)
{
  printf("\nUsage: %s [-h|-i] [-r|-s|-t]\n", name);
  puts("\nDatabase testing utility.");
  puts("\nOptions:");
  puts("\t-h:\tUse handles to identify states");
  puts("\t-i:\tUse indexes to identify states (default)\n");
  puts("\t-r:\tUse red-black tree");
  puts("\t-s:\tUse splay tree (default)");
  puts("\t-t:\tUse hash table\n");
  return 0;
}

int main(int argc, char** argv)
{
  fputs(LibraryVersion(), stderr);
  fputc('\n', stderr);
  state_db_type which = SDBT_Splay;
  bool use_index = true;
  char* name = argv[0];
  int ch;
  for (;;) {
    ch = getopt(argc, argv, "hirst");
    if (ch<0) break;
    switch (ch) {
      case 'h':
        use_index = false;
        break;
      case 'i':
        use_index = true;
        break;
      case 'r':
        which = SDBT_RedBlack;
        break;
      case 's':
        which = SDBT_Splay;
        break;
      case 't':
        which = SDBT_Hash;
        break;
      default:
        return Usage(name);
    } // switch
  } // for

  state_db* reachset = CreateStateDB(which, use_index, false);
  if (NULL==reachset) {
    printf("NULL database\n");
    return 0;
  }
  switch(which) {
    case SDBT_RedBlack:   fprintf(stderr, "Using red-black tree ");  break;
    case SDBT_Splay:      fprintf(stderr, "Using splay tree ");    break;
    case SDBT_Hash:       fprintf(stderr, "Using hash table ");    break;
    default:              fprintf(stderr, "Using unknown data structure ");
  }
  if (use_index)  fprintf(stderr, "with indexes\n"); 
  else            fprintf(stderr, "with handles\n");
  
  int size;
  fprintf(stderr, "Enter state sizes:\n");
  scanf("%d", &size);
  if (size<1) return 0;

  int* state = new int[size];

  fprintf(stderr, "Enter states of size %d, negatives to quit\n", size);

  while (ReadState(state, size)) {
    long h = reachset->InsertState(state, size);
    fprintf(stderr, "Added ");
    PrintState(stderr, state, size);
    fprintf(stderr, "\tIndex %ld\n", h);
  } // while

  reachset->DumpDot(stdout);
  
  return 0;
}
