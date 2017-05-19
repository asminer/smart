
// $Id$

#include <stdio.h>
#include "mcparse.h"

#define DEBUG_PARSER

class empty_builder : public mc_builder {
#ifdef DEBUG_PARSER
  void print(solution_type which) const {
    switch (which) {
      case Steady_state:  fprintf(errlog, "Steady_state");  break;
      case Accumulated:   fprintf(errlog, "Accumulated");   break;
      case Transient:     fprintf(errlog, "Transient");     break;
      default:            fprintf(errlog, "unknown");
    };
  }
  bool report(const char* x) {
    fprintf(errlog, "%s\n", x);
    return true;
  }
#else
  bool report(const char*) {
    return true;
  }
#endif
  public:
    empty_builder(FILE *err) : mc_builder(err) { }
    virtual bool startDTMC(const char* who) {
#ifdef DEBUG_PARSER
      if (who)  fprintf(errlog, "startDTMC(\"%s\")\n", who);
      else      fprintf(errlog, "startDTMC()\n");
#endif
      return true;
    }
    virtual bool startCTMC(const char* who) {
#ifdef DEBUG_PARSER
      if (who)  fprintf(errlog, "startCTMC(\"%s\")\n", who);
      else      fprintf(errlog, "startCTMC()\n");
#endif
      return true;
    }
    virtual bool specifyStates(long num_states) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "specifyStates(%ld)\n", num_states);
#endif
      return true;
    }
    virtual bool startInitial() {
      return report("startInitial()");
    }
    virtual bool addInitial(long state, double weight) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "addInitial(%ld, %lf)\n", state, weight);
#endif
      return true;
    }
    virtual bool doneInitial() {
      return report("doneInitial()");
    }
    virtual bool startEdges(long num_edges = -1) {
#ifdef DEBUG_PARSER
      if (num_edges>=0)
        fprintf(errlog, "startEdges(%ld)\n", num_edges);
      else
        fprintf(errlog, "startEdges()\n");
#endif
      return true;
    }
    virtual bool addEdge(long from, long to, double wt) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "addEdge(%ld, %ld, %lf)\n", from, to, wt);
#endif
      return true;
    }
    virtual bool doneEdges() {
      return report("doneEdges()");
    }

    virtual bool startMeasureCollection(solution_type which, int time) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "startMeasureCollection(");
      print(which);
      fprintf(errlog, ", %d)\n", time);
#endif
      return true;
    }
    virtual bool startMeasureCollection(solution_type which, double time) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "startMeasureCollection(");
      print(which);
      fprintf(errlog, ", %lf)\n", time);
#endif
      return true;
    } 
    virtual bool startMeasure(const char* name) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "startMeasure(%s)\n", name);
#endif
      return true;
    }
    virtual bool addToMeasure(long state, double value) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "addToMeasure(%ld, %lf)\n", state, value);
#endif
      return true;
    }
    virtual bool doneMeasure() {
      return report("doneMeasure()");
    }
    virtual bool doneMeasureCollection() {
      return report("doneMeasureCollection()");
    }
    virtual bool assertClasses(long nc) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "assertClasses(%ld)\n", nc);
#endif
      return true;
    }
    virtual bool assertAbsorbing(long st) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "assertAbsorbing(%ld)\n", st);
#endif
      return true;
    }
    virtual bool assertTransient(long s) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "assertTransient(%ld)\n", s);
#endif
      return true;
    }
    virtual bool startRecurrentAssertion() {
      return report("startRecurrentAssertion()");
    }
    virtual bool assertRecurrent(long s) {
#ifdef DEBUG_PARSER
      fprintf(errlog, "assertRecurrent(%ld)\n", s);
#endif
      return true;
    }
    virtual bool doneRecurrentAssertion() {
      return report("doneRecurrentAssertion()");
    }
};

int Usage(const char* name)
{
  printf("Usage: %s <mcfile1> <mcfile2> ... <mcfilen>\n\tIf n=0 then we read from standard input\n", name);
  return 1;
}

int main(int argc, const char** argv)
{
  bool ok;
  puts(mc_builder::getParserVersion());

  empty_builder foo(stderr);

  if (1==argc) {
    printf("Reading from standard input\n");
    ok = foo.parse_file(stdin, 0);
    if (!ok) return 1;
    foo.done_parsing();
    printf("Have a nice day\n");
    return 0;
  }

  for (int i=1; i<argc; i++) {
    FILE* infile = fopen(argv[i], "r");
    if (0==infile) {
      printf("Couldn't open file %s\n", argv[i]);
      return 2;
    }
    printf("Reading from file %s\n", argv[i]);
    ok = foo.parse_file(infile, argv[i]);
    if (!ok) return 1;
    fclose(infile);
  }
  foo.done_parsing();
  printf("Have a nice day\n");
  return 0;
}
