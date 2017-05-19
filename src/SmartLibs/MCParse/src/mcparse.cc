
// $Id$

#include <stdio.h>
#include <string.h>
#include "mcparse.h"
#include "revision.h"

const char* mc_builder::getParserVersion()
{
  static char buffer[100];
  snprintf(buffer, 99, "Markov Chain Parser, version 0.5.%d", REVISION_NUMBER);
  return buffer;
}

mc_builder::mc_builder(FILE* err)
{
  errlog = err;
  line_number = 1;
  pstate = xTMC;
  curr_line = 0;
  curr_char = 0;
  filename = 0;
}

mc_builder::~mc_builder()
{
}

void mc_builder::startError()
{
  if (filename) {
    fprintf(errlog, "In file %s at line %ld\n", filename, line_number);
  } else {
    fprintf(errlog, "In line %ld\n", line_number);
  }
  if (curr_line) {
    int i = 0;
    for (;;) {
      if (0==curr_line[i]) {
        fputc('\n', errlog);
        break;
      }
      fputc(curr_line[i], errlog);
      if ('\n'==curr_line[i]) break;
      i++;
    }
    fprintf(errlog, "%*s\n", curr_char, "^");
  }
  fprintf(errlog, "Error: ");
}

void mc_builder::doneError()
{
  fprintf(errlog, "\n");
}

bool mc_builder::parse_file(FILE *in, const char* fname)
{
  line_number = 1;
  filename = fname;
  const int max_line = 1024;
  char buffer[max_line];
  buffer[max_line-2]=0;
  for (;;) {
    if (!fgets(buffer, max_line, in)) break;
    if (buffer[max_line-2]) {
      fprintf(errlog, "Error reading line %ld: line is too long!\n", line_number);
      return false;
    }
    if (!parse_lines(buffer)) return false;
  }
  return true;
}

inline int advanceToToken(const char* line, int p)
{
  bool in_comment = false;
  for (;; p++) {
    if ('\n' == line[p]) return p;
    if (0 == line[p])    return p;
    if (' ' == line[p]) continue;
    if ('\t' == line[p]) continue;
    if ('\r' == line[p]) continue;
    if ('#' == line[p]) in_comment = true;
    if (!in_comment) return p;
  }
}

inline bool isSpace(char c)
{
  return (' '==c || '\t'==c || '\r'==c || '\n' == c || 0==c);
}

// returns -1 on no match; otherwise, new char position
inline int advancePastKeyword(const char* line, int p, const char* keyword)
{
  const char* tok = line+p;
  for (int i=0; ; i++) {
    if (0==keyword[i] && isSpace(tok[i])) return p+i;
    if (tok[i] != keyword[i]) return -1;
  }
}

inline int advanceToSpace(const char* line, int p)
{
  for (;; p++) {
    if (isSpace(line[p])) return p;
  }
}

inline bool consumeInteger(const char* line, int &p, long& n)
{
  n = 0;
  for (;; p++) {
    if (0==line[p] || ' '==line[p] || '\t'==line[p] || '\n'==line[p] || ':'==line[p])
      return true;
    if (line[p]<'0' || line[p]>'9') return false;
    n *= 10;
    n += line[p]-'0';
  }
}

inline bool consumeReal(const char* line, int &p, double& n)
{
  double frac = 0;
  n = 0;
  for (;; p++) {
    if (0==line[p] || ' '==line[p] || '\t'==line[p] || '\n'==line[p])
      return true;
    if (line[p]=='.') {
      if (frac) return false;
      frac = 1;
      continue;
    }
    if (line[p]<'0' || line[p]>'9') return false;
    int digit = line[p]-'0';
    if (frac) {
      frac /= 10.0;
      n += frac * digit;
    } else {
      n *= 10.0;
      n += digit;
    }
  }
}

// return: -1 on error, otherwise, number of decimal points
inline int consumeNumber(const char* line, int &p, int& i, double& r)
{
  i = 0;
  double frac = 1;
  int points = 0;
  for (;; p++) {
    if (0==line[p] || ' '==line[p] || '\t'==line[p] || '\n'==line[p])
      return points;
    if (line[p]=='.') {
      if (points) return -1;
      r = i;
      points++;
      continue;
    }
    if (line[p]<'0' || line[p]>'9') return -1;
    int digit = line[p]-'0';
    if (points) {
      frac /= 10.0;
      r += frac * digit;
    } else {
      i *= 10;
      i += digit;
    }
  }
}

bool mc_builder::expectingError(const char* kw)
{
  startError();
  fprintf(errlog, "expecting %s", kw);
  doneError();
  return false;
}

mc_builder::solution_type mc_builder::peekAtMsrType()
{
  int p = advancePastKeyword(curr_line, curr_char, "STEADY");
  if (p>=0) return Steady_state;
  p = advancePastKeyword(curr_line, curr_char, "ACCUMULATED");
  if (p>=0) return Accumulated;
  p = advancePastKeyword(curr_line, curr_char, "TIME");
  if (p>=0) return Transient;
  p = advancePastKeyword(curr_line, curr_char, "ASSERT");
  if (p>=0) return Assert;
  return None;
}

bool mc_builder::parse_lines(char* buffer)
{
  curr_line = buffer;
  curr_char = 0;

  if (buffer) for (;;) {
    int p = advanceToToken(curr_line, curr_char);
    if (0==curr_line[p]) return true;
    if ('\n'==curr_line[p]) {
      line_number++;
      curr_line = curr_line+p+1;
      curr_char = 0;
      continue;
    }
    // we're at the start of a token; process it
    curr_char = p;

    switch (pstate) {
      case xTMC:
        if ('D'==curr_line[curr_char]) {
          is_dtmc = true;
          p = advancePastKeyword(curr_line, curr_char, "DTMC");  
          if (p<0) return expectingError("keyword DTMC");
          curr_char = p;
        } else if ('C'==curr_line[curr_char]) {
          is_dtmc = false;
          p = advancePastKeyword(curr_line, curr_char, "CTMC");  
          if (p<0) return expectingError("keyword CTMC");
          curr_char = p;
        } else {
          return expectingError("keyword DTMC or CTMC");
        }
        // get name of MC, if any
        curr_char = advanceToToken(curr_line, p);
        p = advanceToSpace(curr_line, curr_char);
        if (p==curr_char) {
          bool ok = is_dtmc ? startDTMC(0) : startCTMC(0);
          if (!ok) return false;
        } else {
          char old_separator = curr_line[p];
          curr_line[p] = 0;
          bool ok;
          if (is_dtmc)  ok = startDTMC(curr_line+curr_char);
          else          ok = startCTMC(curr_line+curr_char);
          curr_line[p] = old_separator;
          curr_char = p;
          if (!ok) return false;
        }
        pstate = STATES;
        continue;

      case STATES: {
        p = advancePastKeyword(curr_line, curr_char, "STATES");
        if (p<0) return expectingError("keyword STATES");
        curr_char = advanceToToken(curr_line, p);
        long nstates;
        if (!consumeInteger(curr_line, curr_char, nstates)) {
          return expectingError("integer, number of states");
        }
        // we can start the MC now
        if (!specifyStates(nstates)) return false;
        pstate = INIT;
        continue;
      }

      case INIT:
        p = advancePastKeyword(curr_line, curr_char, "INIT");
        if (p<0) return expectingError("keyword STATES");
        curr_char = p;
        pstate = ARCS;
        if (!startInitial()) return false;
        continue;

      case ARCS:
        if (curr_line[curr_char]<'0' || curr_line[curr_char]>'9') {
          p = advancePastKeyword(curr_line, curr_char, "ARCS");
          if (p<0) return expectingError("keyword ARCS");
          if (!doneInitial()) return false;
          curr_char = advanceToToken(curr_line, p);
          if ('\n' == curr_line[curr_char]) {
            if (!startEdges()) return false;
            pstate = END;
            continue;
          }
          long narcs;
          if (!consumeInteger(curr_line, curr_char, narcs)) {
            return expectingError("integer, number of arcs");
          }
          if (!startEdges(narcs)) return false;
          pstate = END;
        } else {
          // we're still consuming initial probs
          long s;
          double w;
          if (!consumeInteger(curr_line, curr_char, s)) {
            return expectingError("integer (state)");
          }
          curr_char = advanceToToken(curr_line, curr_char);
          if (':' != curr_line[curr_char]) {
            return expectingError(":");
          }
          curr_char = advanceToToken(curr_line, curr_char+1);
          if (!consumeReal(curr_line, curr_char, w)) {
            return expectingError("real (weight)");
          }
          if (!addInitial(s, w)) return false;
        }
        continue;

      case END:
        if (curr_line[curr_char]<'0' || curr_line[curr_char]>'9') {
          p = advancePastKeyword(curr_line, curr_char, "END");
          if (p<0) return expectingError("keyword END");
          if (!doneEdges()) return false;
          curr_char = p;
          pstate = MSRS;
        } else {
          // we're still consuming edges
          long f;
          long t;
          double w;
          if (!consumeInteger(curr_line, curr_char, f)) {
            return expectingError("integer (from state)");
          }
          curr_char = advanceToToken(curr_line, curr_char);
          if (':' != curr_line[curr_char]) {
            return expectingError(":");
          }
          curr_char = advanceToToken(curr_line, curr_char+1);
          if (!consumeInteger(curr_line, curr_char, t)) {
            return expectingError("integer (to state)");
          }
          curr_char = advanceToToken(curr_line, curr_char);
          if (':' != curr_line[curr_char]) {
            return expectingError(":");
          }
          curr_char = advanceToToken(curr_line, curr_char+1);
          if (!consumeReal(curr_line, curr_char, w)) {
            return expectingError("real (weight)");
          }
          if (!addEdge(f, t, w)) return false;
        }
        continue;

      case MSRS: { 
        // Check for start of next chain
        if (0<advancePastKeyword(curr_line, curr_char, "DTMC") ||
            0<advancePastKeyword(curr_line, curr_char, "CTMC"))
        {
          pstate = xTMC;
          continue;
        }

        // next thing should be measure type
        solution_type st = peekAtMsrType();
        if (None == st)
          return expectingError("measure type (ASSERT, STEADY, ACCUMULATED, TIME)");
        curr_char = advanceToSpace(curr_line, curr_char);
        if (Assert == st) {
          pstate = ASRT_TYPE;
          continue;
        }
        int it = 0;
        double rt = 0.0;
        int status = 0;
        if (Transient == st) {
          curr_char = advanceToToken(curr_line, curr_char+1);
          status = consumeNumber(curr_line, curr_char, it, rt);
          if (status < 0) {
            return expectingError("real (time)");
          }
        }
        bool ok = status
          ? startMeasureCollection(st, rt)
          : startMeasureCollection(st, it);
        if (!ok) return false;
        pstate = MSR_NAME;
        continue;
      }
        
      case MSR_NAME: { 
        // Check for start of next chain
        if (0<advancePastKeyword(curr_line, curr_char, "DTMC") ||
            0<advancePastKeyword(curr_line, curr_char, "CTMC"))
        {
          if (!doneMeasureCollection()) return false;
          pstate = xTMC;
          continue;
        }

        // next is either measure name, or back to msr 
        solution_type st = peekAtMsrType();
        if (st != None) {
          // don't advance token!
          if (!doneMeasureCollection()) return false;
          pstate = MSRS;
          continue;
        }
        p = advanceToSpace(curr_line, curr_char);
        char old_separator = curr_line[p];
        curr_line[p] = 0;
        if (!startMeasure(curr_line+curr_char)) return false;
        curr_line[p] = old_separator;
        curr_char = p;
        pstate = MSR_REW;
        continue;
      }

      case MSR_REW:   // want state : rew; stop if identifier or msr type
        if (curr_line[curr_char]>='0' && curr_line[curr_char]<='9') {
          // next thing is a state, great.
          long s;
          double w;
          if (!consumeInteger(curr_line, curr_char, s)) {
            return expectingError("integer (state)");
          }
          curr_char = advanceToToken(curr_line, curr_char);
          if (':' != curr_line[curr_char]) {
            return expectingError(":");
          }
          curr_char = advanceToToken(curr_line, curr_char+1);
          if (!consumeReal(curr_line, curr_char, w)) {
            return expectingError("real (weight)");
          }
          if (!addToMeasure(s, w)) return false;
          continue;
        } else {
          // we're definitely done with this measure. 
          if (!doneMeasure()) return false;
          // change states based on next token; do not consume it.
          if (0<advancePastKeyword(curr_line, curr_char, "DTMC") ||
              0<advancePastKeyword(curr_line, curr_char, "CTMC"))
          {
            if (!doneMeasureCollection()) return false;
            pstate = xTMC;
            continue;
          }
          solution_type st = peekAtMsrType();
          if (None == st) {
            // next token must be an identifier.
            pstate = MSR_NAME;
            continue;
          }
          // next thing must be a measure type
          if (!doneMeasureCollection()) return false;
          pstate = MSRS;
          continue;
        }

      case ASRT_TYPE:
        p = advancePastKeyword(curr_line, curr_char, "CLASSES");
        if (p>=0) {
          curr_char = advanceToToken(curr_line, p+1);
          long nc;
          if (!consumeInteger(curr_line, curr_char, nc)) {
            return expectingError("integer (number of classes)");
          }
          if (!assertClasses(nc)) return false;
          pstate = MSRS;
          continue;
        }
        p = advancePastKeyword(curr_line, curr_char, "TRANSIENT");
        if (p>0) {
          curr_char = p;
          pstate = TRANSLIST;
          continue;
        }
        p = advancePastKeyword(curr_line, curr_char, "RECURRENT");
        if (p>0) {
          curr_char = p;
          pstate = RECURLIST;
          if (!startRecurrentAssertion()) return false;
          continue;
        }
        p = advancePastKeyword(curr_line, curr_char, "ABSORBING");
        if (p>0) {
          curr_char = p;
          pstate = ABSLIST;
          continue;
        }
        return expectingError("assertion type (CLASSES, TRANSIENT, RECURRENT, ABSORBING)");

      case TRANSLIST:
        if (curr_line[curr_char]>='0' && curr_line[curr_char]<='9') {
          // next thing is a state, great.
          long s;
          if (!consumeInteger(curr_line, curr_char, s)) {
            return expectingError("integer (state)");
          }
          if (!assertTransient(s)) return false;
        } else {
          // done listing states
          pstate = MSRS;
        }
        continue;

      case RECURLIST:
        if (curr_line[curr_char]>='0' && curr_line[curr_char]<='9') {
          // next thing is a state, great.
          long s;
          if (!consumeInteger(curr_line, curr_char, s)) {
            return expectingError("integer (state)");
          }
          if (!assertRecurrent(s)) return false;
        } else {
          // done listing states
          if (!doneRecurrentAssertion()) return false;
          pstate = MSRS;
        }
        continue;
        
      case ABSLIST:
        if (curr_line[curr_char]>='0' && curr_line[curr_char]<='9') {
          // next thing is a state, great.
          long s;
          if (!consumeInteger(curr_line, curr_char, s)) {
            return expectingError("integer (state)");
          }
          if (!assertAbsorbing(s)) return false;
        } else {
          // done listing states
          pstate = MSRS;
        }
        continue;

    } // switch

  } // infinite loop
  return true; // keep compiler happy
}

bool mc_builder::done_parsing()
{
  curr_line = 0;

  // process EOF based on current state

  switch (pstate) {
    case MSRS:
      return true;

    case MSR_NAME:
      return doneMeasureCollection();
      
    case MSR_REW:
      return doneMeasure() && doneMeasureCollection();

    case TRANSLIST:
      return true;

    case RECURLIST:
      return doneRecurrentAssertion();

    case ABSLIST:
      return true;

    default:
      startError();
      fprintf(errlog, "premature end of file");
      doneError();
  } // switch
  return false;
}
