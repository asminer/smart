
// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "statelib.h"

// #define DEEP_DEBUG_READ
// #define DEBUG_READ
#define DEBUG_TEST

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

// returns marking length, -1 on eof or error.
int ReadMark(FILE* in, int* state_buffer, int bufsize)
{
  int tok = NextLeftBracket(in);
  if (EOF==tok) return -1;
#ifdef DEEP_DEBUG_READ
  fprintf(stderr, "Got [\n");
#endif
  int stlen = 0;
  for (stlen=0; stlen<bufsize; stlen++) {
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
  if (stlen>=bufsize) error("state is too long; missing ']'?");
  return stlen;
}

void ReadMarkFile(StateLib::state_db* S, FILE* in, const char* name)
{
  int state_buffer[256];
  line_number = 1;
  fname = name;
#ifdef DEBUG_READ
  long markings = 0;
#endif
  for (;;) {
    int stlen = ReadMark(in, state_buffer, 256);
    if (stlen<0) return;

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

bool VerifyMarkFile(StateLib::state_db* S, FILE* in)
{
  int state_buffer[256];
  line_number = 1;
  for (;;) {
    int stlen = ReadMark(in, state_buffer, 256);
    if (stlen<0) return true;

    long where = S->FindState(state_buffer, stlen);
    if (where<0) return false;
  }
}

bool VerifyColl(StateLib::state_db* S)
{
  int state_buffer[256];
  const StateLib::state_coll* C = S->GetStateCollection();
  for (long i=0; i<C->Size(); i++) {
    int stsize = C->GetStateUnknown(i, state_buffer, 256);
    if (stsize<0) return false;
    long where = S->FindState(state_buffer, stsize);
    if (where != i) {
#ifdef DEBUG_TEST
      printf("Failed, for marking %ld\nGot:", i);
      ShowState(stdout, state_buffer, stsize);
      printf("\nFind:%ld\n\n", where);
#endif
      return false;
    }
  }
  return true;
}

int Usage(const char* name)
{
  fprintf(stderr, "\nUsage: %s [options] file1 file2 file3 ...\n", name);
  fprintf(stderr, "\tFor each file, build a state collection, then re-read\n");
  fprintf(stderr, "\tthe file, verifying that all states are in the collection\n");
  fprintf(stderr, "\tStates are assumed to be listed as vectors,\n");
  fprintf(stderr, "\te.g., [ a, b, c, ..., d ], in order.  Text outside\n");
  fprintf(stderr, "\tthe brackets is ignored\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-r:\tUse red-black tree\n");
  fprintf(stderr, "\t-s:\tUse splay tree (default)\n");
  fprintf(stderr, "\t-t:\tUse hash table\n\n");
  return 0;
}

int main(int argc, char** argv)
{
  StateLib::state_db_type which = StateLib::SDBT_Splay;
  const char* name = argv[0];

  // process command line;
  int ch;
  for (;;) {
    ch = getopt(argc, argv, "rst");
    if (ch<0) break;

    switch (ch) {
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

  while (argc>0) {
    FILE* in = fopen(argv[0], "r");
    if (0==in) {
      fprintf(stderr, "Couldn't open file %s\n", argv[0]);
      return 1;
    }
    StateLib::state_db* db = StateLib::CreateStateDB(which, true, true);

    printf("%-30s", argv[0]);
    fflush(stdout);
    ReadMarkFile(db, in, argv[0]);
    printf("\n\tfinds: ");
    fflush(stdout);
    rewind(in);
    if (VerifyMarkFile(db, in)) {
      printf(" OK \n");
    } else {
      printf(" FAILED! \n");
    }
    fclose(in);
    printf("\tgets : ");
    fflush(stdout);
    if (VerifyColl(db)) {
      printf(" OK \n");
    } else {
      printf(" FAILED! \n");
    }
    delete db;

    argc--;
    argv++;
  }
  return 0;
}
