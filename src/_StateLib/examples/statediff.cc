
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "statelib.h"

// #define DEEP_DEBUG_READ
// #define DEBUG_READ

bool quiet;
int line_number;
const char* fname;

int NextLeftBracket(FILE* in)
{
  for (;;) {
    int c = fgetc(in);
    if ('['==c) return c;
    if (EOF==c) return c;
    if ('\n'==c) line_number++;
  }
}

int SkipWhite(FILE* in)
{
  for (;;) {
    int c = fgetc(in);
    if (']'==c) return c;
    if (EOF==c) return c;
    if (c>='0' && c<='9') return c;
    if ('\n'==c) line_number++;
  }
}

void error(const char* erstring)
{
  fprintf(stderr, "Fatal error near line %d in file %s: %s\n", 
    line_number, fname, erstring
  );
  exit(2);
}

void ShowState(FILE* out, int* A, int n)
{
  fprintf(out, "[");
  for (int i=0; i<n; i++) {
    if (i) fprintf(out, ", ");
    fprintf(out, "%d", A[i]);
  }
  fprintf(out, "]");
}

void ReadMarkFile(StateLib::state_db* S, FILE* in, const char* name)
{
  int state_buffer[256];
  if (!quiet) fprintf(stderr, "Reading %s\n", name);
  line_number = 1;
  fname = name;
#ifdef DEBUG_READ
  long markings = 0;
#endif
  for (;;) {
    int tok = NextLeftBracket(in);
    if (EOF==tok) return;
#ifdef DEEP_DEBUG_READ
    fprintf(stderr, "Got [\n");
#endif
    int stlen = 0;
    for (stlen=0; stlen<256; stlen++) {
      tok = SkipWhite(in);
      if (EOF==tok) error("premature end of file");
      if (']'==tok) break;
#ifdef DEEP_DEBUG_READ
      fprintf(stderr, "At %c\n", tok);
#endif
      ungetc(tok, in);
      if (1>fscanf(in, "%d", state_buffer+stlen)) {
        error("couldn't scan integer");
      }
#ifdef DEEP_DEBUG_READ
      fprintf(stderr, "Got %d\n", state_buffer[stlen]);
#endif
    }
    if (stlen>=256) error("state is too long; missing ']'?");

#ifdef DEBUG_READ
    fprintf(stderr, "Got marking %5ld: ", markings);
    ShowState(stderr, state_buffer, stlen);
    fprintf(stderr, "\n");
    markings++;
#endif

    long status = S->InsertState(state_buffer, stlen);
    if (status<0) {
      switch(status) {
        case -1: error("internal problem: static database");
        case -2: error("memory error");
        case -3: error("stack overflow");
      }
    }
  }
}

int Usage(const char* name)
{
  fprintf(stderr, "\nUsage: %s mark1 mark2\n", name);
  fprintf(stderr, "\tCompares state sets listed in the two files.\n");
  fprintf(stderr, "\tStates are assumed to be listed as vectors,\n");
  fprintf(stderr, "\te.g., [ a, b, c, ..., d ], in order.  Text outside\n");
  fprintf(stderr, "\tthe brackets is ignored\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-r:\tUse red-black tree\n");
  fprintf(stderr, "\t-s:\tUse splay tree (default)\n");
  fprintf(stderr, "\t-t:\tUse hash table\n\n");
  fprintf(stderr, "\t-m:\tShow mapping between sets\n");
  fprintf(stderr, "\t-q:\tRun quietly\n");
  return 0;
}

bool isSubset(const char* fn, const StateLib::state_coll* S, StateLib::state_db* ref)
{
  bool answer = true;
  int state[256];

  for (long i=0; i<S->Size(); i++) {
    int stsize = S->GetStateUnknown(i, state, 256);
    if (stsize<0) {
      fprintf(stderr, "Internal error: can't get state %ld from %s\n", i, fn);
      exit(3);
    }
    long where = ref->FindState(state, stsize);
    if (where>=0) continue;
    if (-3==where) {
      fprintf(stderr, "Internal error: stack overflow\n");
      exit(4);
    }
    if (answer) {
      printf("Only in %s:\n", fn);
      answer = false;
    }
    printf("Marking %5ld: ", i);
    ShowState(stdout, state, stsize);
    printf("\n");
  }
  if (!answer) printf("\n");
  return answer;
}

void showMapping(const StateLib::state_coll* S, StateLib::state_db* ref)
{
  int state[256];
  for (long i=0; i<S->Size(); i++) {
    int stsize = S->GetStateUnknown(i, state, 256);
    if (stsize<0) return; // shouldn't happen
    long where = ref->FindState(state, stsize);
    if (where<0) return;  // also shouldn't happen
    printf("\t%5ld --> %5ld\n", i, where);
  }
}

int main(int argc, char** argv)
{
  quiet = false;
  bool mapping = false;
  StateLib::state_db_type which = StateLib::SDBT_Splay;
  const char* name = argv[0];

  // process command line;
  int ch;
  for (;;) {
    ch = getopt(argc, argv, "mqrst");
    if (ch<0) break;

    switch (ch) {
      case 'm':
        mapping = true;
        break;

      case 'q':
        quiet = true;
        break;

      case 'r':
        which = StateLib::SDBT_RedBlack;
        break;

      case 's':
        which = StateLib::SDBT_Splay;
        break;

      case 't':
        which = StateLib::SDBT_Hash;
        break;

      default:
        return Usage(name);
    }
  }
  argc -= optind;
  argv += optind;
  if (argc!=2) return Usage(name);
  const char* fileA = argv[0];
  const char* fileB = argv[1];

  // open files

  FILE* inA = fopen(fileA, "r");
  if (0==inA) {
    fprintf(stderr, "Couldn't open file %s\n", fileA);
    return 1;
  }

  FILE* inB = fopen(fileB, "r");
  if (0==inB) {
    fprintf(stderr, "Couldn't open file %s\n", fileB);
    return 1;
  }

  // create collections
  StateLib::state_db* dbA = StateLib::CreateStateDB(which, true, true);
  StateLib::state_db* dbB = StateLib::CreateStateDB(which, true, true);

  // Read collections
  ReadMarkFile(dbA, inA, fileA);
  fclose(inA);
  if (!quiet) fprintf(stderr, "Got %ld unique markings\n", dbA->Size());

  ReadMarkFile(dbB, inB, fileB);
  fclose(inB);
  if (!quiet) fprintf(stderr, "Got %ld unique markings\n", dbB->Size());

  // compare
  bool AsubB = isSubset(fileA, dbA->GetStateCollection(), dbB);
  bool BsubA = isSubset(fileB, dbB->GetStateCollection(), dbA);

  if (AsubB && BsubA) {
    printf("State sets %s and %s are identical.\n", fileA, fileB);
    if (mapping) {
      printf("Mapping from %s to %s:\n", fileA, fileB);
      showMapping(dbA->GetStateCollection(), dbB);
    }
  }

  return 0;
}
