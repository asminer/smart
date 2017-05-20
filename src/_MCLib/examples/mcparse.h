
// $Id$

#ifndef MCPARSE_H
#define MCPARSE_H

/**
  Abstract base class for parsing.
  You must derive a class from this one, and implement
  the virtual functions.
*/
class mc_builder {
  public:
    mc_builder(FILE* err);
    virtual ~mc_builder();

    /** Indicates that no more lines will be parsed.
        Returns true iff the overall parsing was successful
        (i.e., if it is legal to end the input here).
    */
    bool done_parsing();

    /** Parse this file.
        Multiple calls to this function are equivalent
        to if the files were concatenated.
        Returns true iff parsing was allowed to complete.
    */
    bool parse_file(FILE*, const char* fname);

    /// Like parse_file, but for the given character string.
    inline bool parse_string(char* prog) {
      line_number = 1;
      return parse_lines(prog);
    }

    static const char* getParserVersion();

  protected:
    /// Error messages are written here.
    FILE* errlog; 

    enum solution_type {
      Steady_state,
      Accumulated,
      Transient,
      Assert,
      None
    };

    void startError();
    const char* getFilename() { return filename; }
    long getLinenumber() { return line_number; }
    void doneError();

    // hooks for however the Markov chain is stored.
    // all of these should return true iff the parser should continue.

    virtual bool startDTMC(const char* name) = 0;
    virtual bool startCTMC(const char* name) = 0;

    virtual bool specifyStates(long ns) = 0;

    virtual bool startInitial() = 0;
    virtual bool addInitial(long state, double weight) = 0;
    virtual bool doneInitial() = 0;

    virtual bool startEdges(long num_edges = -1) = 0;
    virtual bool addEdge(long from, long to, double wt) = 0;
    virtual bool doneEdges() = 0;

    virtual bool startMeasureCollection(solution_type which, double time) = 0;
    virtual bool startMeasureCollection(solution_type which, int time) = 0;
    virtual bool startMeasure(const char* name) = 0;
    virtual bool addToMeasure(long state, double value) = 0;
    virtual bool doneMeasure() = 0;
    virtual bool doneMeasureCollection() = 0;

    /// Check number of recurrent classes of size greater than one.
    virtual bool assertClasses(long nc) = 0;

    virtual bool assertAbsorbing(long st) = 0;
    virtual bool assertTransient(long s) = 0;

    virtual bool startRecurrentAssertion() = 0;
    virtual bool assertRecurrent(long s) = 0;
    virtual bool doneRecurrentAssertion() = 0;

  private:  // nothing to see here, move along.

    enum parse_state {
      xTMC = 0,
      STATES,
      INIT,
      ARCS,
      END,
      MSRS,
      MSR_NAME,
      MSR_REW,
      ASRT_TYPE,
      TRANSLIST,
      RECURLIST,
      ABSLIST
    };

    parse_state pstate;
    bool is_dtmc;
    const char* filename;
    long line_number;
    char* curr_line;
    int curr_char;
    bool parse_lines(char* buffer);
    bool expectingError(const char* kw);

    solution_type peekAtMsrType();
};

#endif
