
#include "../../config.h"
#include <stdio.h>

#ifndef HAVE_PNFRONT_H

int main()
{
  printf("This program requires library PNFront.\n");
  return 0;
}

#else

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "statelib.h"
#include "pnfront.h"
#include "timerlib.h"

// #define VERIFY_STATIC
// #define VERIFY_DYNAMIC

#include <assert.h>

StateLib::state_db* reachset = 0;
long v_max;  // max number of vanishings per elimination.

void Report();
#ifdef VERIFY_STATIC
void VerifyStatic(const pn_model* m);
#endif
#ifdef VERIFY_DYNAMIC
void VerifyDynamic(const pn_model* m);
#endif

void ShowState(const pn_model* m, const int* s, FILE* strm)
{
  fputs("[", strm);
  bool written = false;
  for (int i=0; i<m->NumPlaces(); i++) if (s[i]) {
    if (written)  fputs(", ", strm);
    fprintf(strm, "%s = %d", m->GetPlaceName(i), s[i]);
    written = true;
  }
  fputs("]", strm);
}

const int Stack_depth = 1024;  // plenty!

inline void CheckStackOver(int top, int size)
{
  if (top < size)  return;
  printf("Integer expression stack overflow\n");
  exit(1);
}

inline void CheckStackUnder(int top, int elems)
{
  if (top >= elems)  return;
  printf("Integer expression stack underflow\n");
  exit(1);
}

inline void CheckNext(int* next)
{
  if (next)  return;
  printf("Assignment expression out of place\n");
  exit(1);
}

inline void CheckAssign(bool b)
{
  if (b) return;
  printf("Assignment expression error\n");
  exit(1);
}

int EvaluateExpr(const pn_model* m, const int_expr* expr, const int* current, int* next)
{
  if (0==expr)  return 0;
  static int* stack = 0;
  if (0==stack) stack = new int[Stack_depth];
  int top = 0;
  for (int i=0; i<expr->NumTerms(); i++) {
    char what;
    int val;
    expr->GetTerm(i, what, val);
    int left, right = 0;
    switch (what) {
      case 'l':
        stack[top++] = val;
        CheckStackOver(top, Stack_depth);
        continue;

      case 'p':
        stack[top++] = current[val];
        CheckStackOver(top, Stack_depth);
        continue;

      case 't':
        printf("Transitions not supported yet in arc expressions\n");
        exit(1);

      case 'b':
        CheckStackUnder(top, 2);
        right = stack[--top];
        left = stack[--top];
        break;

      case 'u':
        CheckStackUnder(top, 1);
        left = stack[--top];
        break;

      default:
        printf("Postfix expression error %c\n", what);
        exit(1);
    }; // what
    // still here? must be an operator
    switch (val) {
      // Unary operators
      case int_expr::MINUS_UOP:
        stack[top++] = -left;
        continue;
      case int_expr::NOT_UOP:
        stack[top++] = ! left;
        continue;
      case int_expr::INCR_UOP:
        CheckNext(next);
        i++;
        CheckAssign((0==top) & (i<expr->NumTerms()));
        expr->GetTerm(i, what, val);
        CheckAssign('p' == what);
        next[val] += left;
        continue;
      case int_expr::DECR_UOP:
        CheckNext(next);
        i++;
        CheckAssign((0==top) & (i<expr->NumTerms()));
        expr->GetTerm(i, what, val);
        CheckAssign('p' == what);
        next[val] -= left;
        continue;
      // Binary operators
      case int_expr::ADD_BOP:
        stack[top++] = left + right;
        continue;
      case int_expr::SUB_BOP:
        stack[top++] = left - right;
        continue;
      case int_expr::MULT_BOP:
        stack[top++] = left * right;
        continue;
      case int_expr::DIV_BOP:
        stack[top++] = left / right;
        continue;
      case int_expr::MOD_BOP:
        stack[top++] = left % right;
        continue;
      case int_expr::MAX_BOP:
        stack[top++] = (left > right) ? left : right;
        continue;
      case int_expr::MIN_BOP:
        stack[top++] = (left < right) ? left : right;
        continue;
      case int_expr::AND_BOP:
        stack[top++] = left && right;
        continue;
      case int_expr::OR_BOP:
        stack[top++] = left || right;
        continue;
      case int_expr::IMPLIES_BOP:
        stack[top++] = (0==left) || (left && right);
        continue;
      case int_expr::EQ_BOP:
        stack[top++] = left == right;
        continue;
      case int_expr::NEQ_BOP:
        stack[top++] = left != right;
        continue;
      case int_expr::GT_BOP:
        stack[top++] = left > right;
        continue;
      case int_expr::GE_BOP:
        stack[top++] = left >= right;
        continue;
      case int_expr::LT_BOP:
        stack[top++] = left < right;
        continue;
      case int_expr::LE_BOP:
        stack[top++] = left <= right;
        continue;
      default:
        printf("Unsupported operator %s\n", 
          expr->GetOperatorName(val));
        exit(1);
    } // val
  } // for i
  if (top)  return stack[--top];
  return 0;
}

void VanishEnabledEvents(const pn_model* m, const int* mark, bool* enabled)
{
  for (int i=m->NumTrans(); i; ) {
    i--;
    if (0==m->IsImmediate(i))  {
      enabled[i] = 0;
      continue;
    }
    const int_expr* t = m->GetTransitionEnablingExpr(i);
    enabled[i] = EvaluateExpr(m, t, mark, 0);
  }
}

void TangibleEnabledEvents(const pn_model* m, const int* mark, bool* enabled)
{
  for (int i=m->NumTrans(); i; ) {
    i--;
    if (1==m->IsImmediate(i))  {
      enabled[i] = 0;
      continue;
    }
    const int_expr* t = m->GetTransitionEnablingExpr(i);
    enabled[i] = EvaluateExpr(m, t, mark, 0);
  }
}

void GetNextState(const pn_model* m, int t, const int* curr, int* next)
{
  memcpy(next, curr, m->NumPlaces() * sizeof(int));
  EvaluateExpr(m, m->GetTransitionFiringExpr(t), curr, next);
}

bool IsVanishing(const pn_model* m, const int* mark)
{
  for (int i=m->NumTrans(); i; ) {
    i--;
    if (0==m->IsImmediate(i))  continue;
    const int_expr* t = m->GetTransitionEnablingExpr(i);
    if (EvaluateExpr(m, t, mark, 0))  return true;
  }
  return false;
}

long Gencmp(const pn_model* m)
{
  StateLib::state_db* rscmp = 
    StateLib::CreateStateDB(StateLib::SDBT_Splay, true, false);
  if (0==rscmp) {
    fputs("Couldn't create comparison set\n", stdout);
    return -2;
  }
  for (int i=0; i<m->NumTrans(); i++) {
    if (m->IsImmediate(i)) {
      fputs("Model has immediate transitions, comparison not supported\n", stdout);
      return -2;
    }
  }

  int* current = new int[m->NumPlaces()];
  int* next = new int[m->NumPlaces()];
  bool* enabled = new bool[m->NumTrans()];

  m->GetInitialMarking(current);
  reachset->InsertState(current, m->NumPlaces());
  rscmp->InsertState(current, m->NumPlaces());
  long t_exp = 0;

  for (;;) {
    if (t_exp < reachset->Size()) {
      reachset->GetStateKnown(t_exp, current, m->NumPlaces());
    } else {
  // no tangibles, bail out
        break;
    } 
    TangibleEnabledEvents(m, current, enabled);  
    for (int e=0; e<m->NumTrans(); e++) if (enabled[e]) {
      GetNextState(m, e, current, next);

      // add next state to appropriate slot
      long to, tocmp;
      try {
        to = reachset->InsertState(next, m->NumPlaces());
        tocmp = rscmp->InsertState(next, m->NumPlaces());
      }
      catch (StateLib::error e) {
        printf("Error: %s\n", e.getName());
        printf("Bailing out after %ld states discovered\n", reachset->Size());
        return -1;
      }

      if (to != tocmp) {
        printf("\nDisagreement when exploring state %ld:\n", t_exp);
        printf("\trss gets index %ld\n", to);
        printf("\tcmp gets index %ld\n", tocmp);

/*
        printf("\tcmp ");
        rscmp->DumpState(stdout, tocmp);
        printf("\trss ");
        reachset->DumpState(stdout, tocmp);
        printf("\trss ");
        reachset->DumpState(stdout, to);
*/

        reachset->ConvertToStatic(true);
        return -1;
      }

    } // for e
    
    // Advance 
    t_exp++;

  } // infinite explore loop

  delete[] current;
  delete[] next;
  delete[] enabled;
  
  delete rscmp;
  return reachset->Size();

}

long Generate(const pn_model* m, bool quiet, bool show, bool debug)
{
  StateLib::state_db* vanishing = 0;
  const StateLib::state_coll* vcoll = 0;
  vanishing = StateLib::CreateStateDB(StateLib::SDBT_Splay, true, false);
  if (0==vanishing) {
    fputs("Couldn't create vanishing set\n", stdout);
    return -2;
  }
  vcoll = vanishing->GetStateCollection();
  
  bool has_immed = false;
  for (int i=0; i<m->NumTrans(); i++) {
    if (m->IsImmediate(i)) {
      has_immed = true;
      break;
    }
  }
  int* current = new int[m->NumPlaces()];
  int* next = new int[m->NumPlaces()];
  bool* enabled = new bool[m->NumTrans()];

  if (!quiet) {
    if (has_immed)  fputs("Petri net has immediate transitions\n", stdout);
    else            fputs("Petri net has only timed transitions\n", stdout);
  }

  timer watch;

  m->GetInitialMarking(current);
  // TO DO: check if initial state is vanishing
  reachset->InsertState(current, m->NumPlaces());
  long v_exp = 0;
  long t_exp = 0;
  v_max = 0;
  if (show) fputs("Reachable states:\n", stdout);
  bool current_is_vanishing = false;

  for (;;) {

    if (v_exp < vanishing->Size()) {
      // there are vanishings to explore
      vcoll->GetStateKnown(v_exp, current, m->NumPlaces());
      current_is_vanishing = true;
    } else {
      // No vanishings to explore
      if (current_is_vanishing) {
        vanishing->Clear();
        if (v_exp > v_max)  v_max = v_exp;
        v_exp = 0;
      } // if current_is_vanishing

      if (t_exp < reachset->Size()) {
        reachset->GetStateKnown(t_exp, current, m->NumPlaces());
        current_is_vanishing = false;
      } else {
  // no tangibles either, bail out
        break;
      }
    } 
    if (debug) {
      if (current_is_vanishing)
        printf("Exploring V# %-9ld", v_exp);
      else
        printf("Exploring T# %-9ld", t_exp);
      ShowState(m, current, stdout);
      fputc('\n', stdout);
      fflush(stdout);
    }
    if (show && !current_is_vanishing) {
      printf("State %ld: ", t_exp);
      ShowState(m, current, stdout);
      fputc('\n', stdout);
    }
    if (current_is_vanishing) VanishEnabledEvents(m, current, enabled);
    else                      TangibleEnabledEvents(m, current, enabled);  
    for (int e=0; e<m->NumTrans(); e++) if (enabled[e]) {
      GetNextState(m, e, current, next);
      bool next_is_vanishing = IsVanishing(m, next);

      // add next state to appropriate slot
      long to;
      try {
        if (next_is_vanishing) {
          // new state is vanishing
          to = vanishing->InsertState(next, m->NumPlaces());
        } else {
          // new state is tangible
          to = reachset->InsertState(next, m->NumPlaces());
        } // if next_is_vanishing
      }
      catch (StateLib::error e) {
        printf("Error: %s\n", e.getName());
        printf("Bailing out after %ld states discovered\n", reachset->Size());
        return -1;
      }

      if (debug) {
        printf("\t");
        if (current_is_vanishing) printf("V# %-9ld", v_exp);
        else                      printf("T# %-9ld", t_exp);
        printf("via %-10s to ", m->GetTransName(e));
        if (next_is_vanishing)  fputs("V", stdout);
        else                    fputs("T", stdout);
        printf("# %-9ld", to);
        ShowState(m, next, stdout);
        fputc('\n', stdout);
      }

    } // for e
    
    // Advance 
    if (current_is_vanishing)   v_exp++;
    else                        t_exp++;

  } // infinite explore loop

  if (!quiet) {
    printf("Generation took %lf seconds\n", watch.elapsed_seconds());

    Report();
  }

  if (vanishing) {
    if (!quiet) {
      fputs("Vanishing states:\n", stdout);
      printf("\tPeak number of states: %ld\n", v_max);
      printf("\tPeak memory: %ld bytes\n", vanishing->ReportMemTotal());
    }
    delete vanishing;
  }

#ifdef VERIFY_STATIC
  VerifyStatic(m);
#endif
#ifdef VERIFY_DYNAMIC
  VerifyDynamic(m);
#endif

  delete[] current;
  delete[] next;
  delete[] enabled;
  
  return reachset->Size();
}

void MemShow(const char* what, double mem)
{
  fputs(what, stdout);
  if (mem < 2000) {
    fprintf(stdout, "%lg bytes\n", mem);
    return;
  }
  mem /= 1024.0;
  if (mem < 2000) {
    fprintf(stdout, "%lg Kb\n", mem);
    return;
  }
  mem /= 1024.0;
  if (mem < 2000) {
    fprintf(stdout, "%lg Mb\n", mem);
    return;
  }
  mem /= 1024.0;
  fprintf(stdout, "%lg Gb\n", mem);
}

void Report()
{
  const StateLib::state_coll* collection = reachset->GetStateCollection();

  fputs("Tangible states:\n", stdout);
  for (int m=0; m<collection->NumEncodingMethods(); m++) {
    int cnt = collection->ReportEncodingCount(m);
    if (0==cnt) continue;
    fprintf(stdout, "\t %d encodings are %s\n", 
    cnt, collection->EncodingMethod(m) );
  }
  fprintf(stdout, "\t %ld states total\n", collection->Size());
  MemShow("\tState memory: ", collection->ReportMemTotal());
  double bps = collection->ReportMemTotal();
  bps /= collection->Size();
  fprintf(stdout, "\tAvg. bytes per state: %lg\n", bps);
  long dbmem = reachset->ReportMemTotal() - collection->ReportMemTotal();
  MemShow("\tDatabase memory: ", dbmem);
  MemShow("\tGrand total: ", reachset->ReportMemTotal());
}

#ifdef VERIFY_STATIC
void VerifyStatic(const pn_model* m)
{
  const state_coll* collection = reachset->GetStateCollection();
  timer watch;
  fputs("Converting to static mode\n", stdout);
  statedb_error e = reachset->ConvertToStatic(true);
  if (e != SDB_Success) {
    switch (e) {
      case SDB_NoMemory:
          fputs("Not enough memory\n", stdout);
          return;
      default:
          fputs("Couldn't convert to static (unexpected error)\n", stdout);
          return;
    }
  }
  fprintf(stdout, "Converstion took %lf seconds\n", watch.elapsed_seconds());
  int* current = new int[m->NumPlaces()];
  fputs("Verifying states...\n", stdout);
  watch.reset();
  for (long i=0; i<reachset->Size(); i++) {
    collection->GetStateKnown(i, current, m->NumPlaces());
    long where = reachset->FindState(current, m->NumPlaces());
    if (where != i) {
        fprintf(stdout, "discrepency, state %ld: ", i);
        ShowState(m, current, stdout);
        fputs(" not found\n", stdout);
        return;
    }
  }
  fprintf(stdout, "Verification took %lf seconds\n", watch.elapsed_seconds());
}
#endif

#ifdef VERIFY_DYNAMIC
void VerifyDynamic(const pn_model* m)
{
  const state_coll* collection = reachset->GetStateCollection();
  timer watch;
  fputs("Converting to dynamic mode\n", stdout);
  statedb_error e = reachset->ConvertToDynamic(true);
  if (e != SDB_Success) {
    switch (e) {
      case SDB_NoMemory:
          fputs("Not enough memory\n", stdout);
          return;
      default:
          fputs("Couldn't convert to static (unexpected error)\n", stdout);
          return;
    }
  }
  fprintf(stdout, "Converstion took %lf seconds\n", watch.elapsed_seconds());
  int* current = new int[m->NumPlaces()];
  fputs("Verifying states...\n", stdout);
  watch.reset();
  for (long i=0; i<reachset->Size(); i++) {
    collection->GetStateKnown(i, current, m->NumPlaces());
    long where = reachset->FindState(current, m->NumPlaces());
    if (where != i) {
        fprintf(stdout, "discrepency, state %ld: ", i);
        ShowState(m, current, stdout);
        fputs(" not found\n", stdout);
        return;
    }
  }
  fprintf(stdout, "Verification took %lf seconds\n", watch.elapsed_seconds());
}
#endif


int Usage(char* name)
{
  puts(StateLib::LibraryVersion());
  puts(PNFRONT_LibraryVersion());

  printf("\nUsage: %s [-d] [-h|-i] [-r|-s|-t] [-q|-v] <file>\n", name);
  puts("\nReads a Petri net from the given file (using PNFront format).");
  puts("Builds the reachability set of the Petri net.");
  puts("\nOptions:");
  // puts("\t-c:\tCompare with another method\n");
  puts("\t-d:\t(Debug)  Display reachability graph information\n");
  puts("\t-h:\tUse handles to identify states");
  puts("\t-i:\tUse indexes to identify states (default)\n");
  puts("\t-r:\tUse red-black tree");
  puts("\t-s:\tUse splay tree (default)");
  puts("\t-t:\tUse hash table\n");
  puts("\t-q:\t(Quiet)   Only checks if the count is correct.");
  puts("\t-v:\t(Verbose) The reachable states will be displayed.\n");
  return 0;
}

int main(int argc, char** argv)
{
  bool compare = false;
  bool quiet = false;
  bool show_states = false;
  bool debug = false;
  bool use_index = true;
  StateLib::state_db_type which = StateLib::SDBT_Splay;
  char* name = argv[0];
  // process command line
  int ch;
  for (;;) {
    ch = getopt(argc, argv, "dhiqrstv");
    if (ch<0) break;
    switch (ch) {
/*
      case 'c':
        compare = true;
        break;
*/
      case 'h':
        use_index = false;
        break;

      case 'i':
        use_index = true;
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

      case 'q':
        debug = show_states = false;
        quiet = true;
        break;

      case 'v':
        show_states = true;
        quiet = false;
        break;

      case 'd':
        debug = true;
        quiet = false;
        break;

      default:
        return Usage(name);
    } // switch
  } // for
  argc -= optind;
  argv += optind;

  if (argc != 1)   return Usage(name);
  
  FILE* input = fopen(argv[0], "r");
  if (0==input) {
    printf("Couldn't open file %s\n", argv[0]);
    return 1;
  }

  long reachable;
  int c = getc(input);
  if (c != '#') {
    ungetc(c, input);
    reachable = -1;
  } else {
    if (1!=fscanf(input, "%ld", &reachable)) reachable = -1;
    rewind(input);
  }
  if (!quiet && reachable>0) 
    printf("Expecting %ld reachable states\n", reachable);
  pn_model* mdl = PNFRONT_Compile_PN(input, stderr);
  if (0==mdl)  return 1;

  if (quiet) {
    fprintf(stderr, "%-40s", argv[0]);
  } else {
    printf("Generating reachability set using ");
    switch(which) {
      case StateLib::SDBT_RedBlack:   printf("red-black tree\n");  break;
      case StateLib::SDBT_Splay:      printf("splay tree\n");    break;
      case StateLib::SDBT_Hash:       printf("hash table\n");    break;
      default:              printf("unknown data structure\n");
    }
  }

  reachset = StateLib::CreateStateDB(which, use_index, false); 
  if (0==reachset) {
    printf("Couldn't create reachability set!\n");
    return 1;
  }
  
  long actual;

  if (compare) {
    actual = Gencmp(mdl);
  } else {
    actual = Generate(mdl, quiet, show_states, debug);
  }

  if (reachable>0 && actual != reachable) {
    if (quiet)  fprintf(stderr, "Changed\n");
    else  printf("Input file says there should be %ld tangible states\n", reachable);
    return 1;
  } else {
    if (quiet)  fprintf(stderr, "Ok\n");
  }
  delete reachset;
  return 0;
}

#endif // giant ifdef
