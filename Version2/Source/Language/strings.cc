
// $Id$

#include "strings.h"
#include "operators.h"

// ******************************************************************
// *                                                                *
// *                         int_add  class                         *
// *                                                                *
// ******************************************************************

/** Addition of strings.
 */

class string_add : public addop {
  char** sop;
public:
  string_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { sop = new char*[n]; }
  
  string_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { sop = new char*[2]; }

  virtual ~string_add() { delete[] sop; }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return STRING;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new string_add(Filename(), Linenumber(), x, n);
  }
};


void string_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();

  int i;
  // Clear strings
  for (i=0; i<opnd_count; i++) sop[i] = NULL;

  // Compute strings for each operand
  int total_length = 0;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    DCASSERT(operands[i]->Type(0) == STRING);
    operands[i]->Compute(0, x);
    sop[i] = (char*) x.other;
    if (x.error) break;
    if (x.null) break;
    if (sop[i])
      total_length += strlen(sop[i]); 
  }

  if (x.error || x.null) {
    // delete strings and bail out
    for (i=0; i<opnd_count; i++) delete[] sop[i]; 
    return;
  }

  // concatenate all strings
  char* answer = new char[total_length+1];  // save room for terminating 0
  answer[0] = 0;
  for (i=0; i<opnd_count; i++) if (sop[i]) {
    strcat(answer, sop[i]);
    delete[] sop[i];
  }

  x.other = answer;
}


// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* MakeStringAdd(expr** opnds, int n, const char* file, int line)
{
  return new string_add(file, line, opnds, n);
}

expr* MakeStringBinary(expr* left, int op, expr* right, const char* file, int
line)
{
  switch (op) {
    case PLUS:
      return new string_add(file, line, left, right);
  }
  return NULL;
}

