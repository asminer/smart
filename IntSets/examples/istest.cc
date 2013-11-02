
// $Id$

#include <stdio.h>
#include <string.h>
#include "../src/intset.h"


const int max_slots = 16;

FILE* input;
long lineno;
intset sets[max_slots];

int grabChar(const char* accept)
{
  int len = strlen(accept);
  while (1) {
    int c = fgetc(input);
    if (EOF == c) return c;
    if ('\n' == c) {
      lineno++;
      continue;
    }
    for (int i=0; i<len; i++)
      if (c == accept[i]) return c;   
  }
}

void readBits(intset &s)
{
  for (long i=0; i<s.getSize(); i++) {
    int c = grabChar("01");
    if (EOF == c) return;
    if ('0' == c) {
      s.removeElement(i);
    } else {
      s.addElement(i);
    } 
  }
}

void showBits(const intset &s)
{
  for (long i=0; i<s.getSize(); i++)
    if (s.contains(i)) fputc('1', stdout);
    else               fputc('0', stdout);
  fputc('\n', stdout);
}

int usage(const char* a)
{
  printf("Usage: %s <file1> <file2> ...\n\n", a);
  printf("Reads a sequence of commands from each <file>.  Use '-' for standard input.\n");
  printf("Commands:\n");
  printf("\tn <slot> <size> <bitvector>: make a new set\n");
  printf("\ts <slot>: show a set\n");
  printf("\n");
  printf("\tc <slotA> <slotB>:  A = ~B\n");
  printf("\td <slotA> <slotB> <slotC>:  A = B - C\n");
  printf("\ti <slotA> <slotB> <slotC>:  A = B * C\n");
  printf("\tu <slotA> <slotB> <slotC>:  A = B + C\n");
  printf("\n");
  printf("\ta <t|f> <slotA> <op> <slotB>: assert that A <op> B is true/false\n");
  printf("\t\t<op> is one of: eq, ne, gt, ge, lt, le\n");
  return 0;
}

void MakeNewSet()
{
  int slot;
  int size;
  fscanf(input, "%d %d", &slot, &size);
  intset answer(size);
  readBits(answer);
  if (slot>=0 && slot < max_slots)
    sets[slot] = answer;   
}

void ShowSet()
{
  int slot;
  fscanf(input, "%d", &slot);
  if (slot>=0 && slot < max_slots) {
    printf("Set: ");
    showBits(sets[slot]);
  }
}

void Complement()
{
  int slotA, slotB;
  fscanf(input, "%d %d", &slotA, &slotB);
  if (slotA<0 || slotB<0 || slotA>=max_slots || slotB>=max_slots) return;
  sets[slotA] = !sets[slotB];
}

void Difference()
{
  int slotA, slotB, slotC;
  fscanf(input, "%d %d %d", &slotA, &slotB, &slotC);
  if (slotA<0 || slotA>=max_slots) return;
  if (slotB<0 || slotB>=max_slots) return;
  if (slotC<0 || slotC>=max_slots) return;
  if (slotA == slotB) {
    sets[slotA] -= sets[slotC];
  } else {
    sets[slotA] = sets[slotB] - sets[slotC];
  }
}

void Intersect()
{
  int slotA, slotB, slotC;
  fscanf(input, "%d %d %d", &slotA, &slotB, &slotC);
  if (slotA<0 || slotA>=max_slots) return;
  if (slotB<0 || slotB>=max_slots) return;
  if (slotC<0 || slotC>=max_slots) return;
  if (slotA == slotB) {
    sets[slotA] *= sets[slotC];
  } else {
    sets[slotA] = sets[slotB] * sets[slotC];
  }
}

void Union()
{
  int slotA, slotB, slotC;
  fscanf(input, "%d %d %d", &slotA, &slotB, &slotC);
  if (slotA<0 || slotA>=max_slots) return;
  if (slotB<0 || slotB>=max_slots) return;
  if (slotC<0 || slotC>=max_slots) return;
  if (slotA == slotB) {
    sets[slotA] += sets[slotC];
  } else {
    sets[slotA] = sets[slotB] + sets[slotC];
  }
}

bool Failed(bool t, int A, const char* op, int B)
{
  printf("FAILED ");
  if (t) printf("true"); else printf("false");
  printf(" assertion %d %s %d near line %ld\n", A, op, B, lineno);
#ifdef INTSET_DEVELOPMENT_CODE
  printf("\t%d is ", A);
  sets[A].dump(stdout);
  printf("\n");
  printf("\t%d is ", B);
  sets[B].dump(stdout);
  printf("\n");
#endif
  return false;
}

bool Assertion()
{
  int c = grabChar("tf");
  if (EOF==c) return false;
  bool correct = (c=='t');
  int slotA, slotB;
  fscanf(input, "%d", &slotA);
  int op1, op2;
  op1 = grabChar("engl");
  op2 = grabChar("qet");
  fscanf(input, "%d", &slotB);

  if (slotA<0 || slotA>=max_slots) return true;
  if (slotB<0 || slotB>=max_slots) return true;

  switch (op1) {
    case 'e':
      if (correct == (sets[slotA] == sets[slotB])) return true;
      return Failed(correct, slotA, "==", slotB);

    case 'n':
      if (correct == (sets[slotA] != sets[slotB])) return true;
      return Failed(correct, slotA, "!=", slotB);
    
    case 'g':
      switch (op2) {
        case 'e':
          if (correct == (sets[slotA] >= sets[slotB])) return true;
          return Failed(correct, slotA, ">=", slotB);

        case 't':
          if (correct == (sets[slotA] > sets[slotB])) return true;
          return Failed(correct, slotA, ">", slotB);

        default: return true;
      } // switch op2  
    
    case 'l':
      switch (op2) {
        case 'e':
          if (correct == (sets[slotA] <= sets[slotB])) return true;
          return Failed(correct, slotA, "<=", slotB);

        case 't':
          if (correct == (sets[slotA] < sets[slotB])) return true;
          return Failed(correct, slotA, "<", slotB);

        default: return true;
      } // switch op2  
  } // switch op1
  return true;
}

void EndOfFile(const char* fn)
{
  if (fn[0] == '-' && fn[1] == 0) return;
  printf("OK\n");
  fclose(input);
}

void RunFile(const char* fn)
{
  if (fn[0] == '-' && fn[1] == 0) {
    input = stdin;
  } else {
    input = fopen(fn, "r");
    if (0==input) {
      printf("Couldn't open file %s\n", fn);
      return;
    }
    printf("%20s      ", fn);
  }
  lineno = 1;

  while (1) {
    int c = grabChar("nscdiua");
    switch (c) {
      case EOF:  EndOfFile(fn);      return;
      case 'n':  MakeNewSet();       break;
      case 's':  ShowSet();          break;
 
      case 'c':  Complement();       break;
      case 'd':  Difference();       break;
      case 'i':  Intersect();        break;
      case 'u':  Union();            break;

      case 'a':  
        if (Assertion()) break;
        return;
    }
  } // while 1
}

int main(int argc, const char** argv)
{
  if (argc < 2) return usage(argv[0]);
  for (int i=1; i<argc; i++) {
    RunFile(argv[i]);
  }
  return 0;
}
