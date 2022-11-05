
#include "iofuncs.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/functions.h"
#include "../SymTabs/symtabs.h"
#include "../Utils/strings.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"

#include <iostream>
#include <fstream>

// #define DEBUG_FILE

// ******************************************************************
// *                        input_file class                        *
// ******************************************************************

class input_file : public simple_internal {
        std::ifstream fin;
    public:
        input_file();
        virtual void Compute(traverse_data &x, expr** pass, int np);

        inline bool isOpen() const {
            return fin.is_open();
        }
        inline bool isClosed() const {
            return !fin.is_open();
        }
        inline void switchInput() {
            if (fin.is_open()) fin.close();
        }
        inline bool switchInput(const char* infile) {
            if (fin.is_open()) fin.close();
            fin.open(infile, std::fstream::in);
            return fin.good();
        }

        // Get next character (raw)
        inline int getc() {
            if (fin.is_open()) {
                return fin.get();
            } else {
                return std::cin.get();
            }
        }

        // Get next item (formatted)
        template <class T>
        inline bool get(T &x)
        {
            if (fin.is_open()) {
                fin >> x;
                return fin.good();
            } else {
                std::cin >> x;
                return std::cin.good();
            }
        }
};


input_file::input_file() : simple_internal(em->BOOL, "input_file", 1)
{
    SetFormal(0, em->STRING, "filename");
    SetDocumentation("Switch the input stream from the specified filename.  If the filename is null, the input stream is switched to standard input. If the filename does not exist or cannot be opened, return false. Returns true on success.");
}

void input_file::Compute(traverse_data &x, expr** pass, int np)
{
    DCASSERT(x.answer);
    DCASSERT(0==x.aggregate);
    DCASSERT(np==1);
    SafeCompute(pass[0], x);
    if (x.answer->isNull()) {
        switchInput();
        x.answer->setBool(true);
        return;
    }

    shared_string *xss = smart_cast <shared_string*> (x.answer->getPtr());
    DCASSERT(xss);
    x.answer->setBool( switchInput(xss->getStr()) );
}

// ******************************************************************
// *                        read_bool  class                        *
// ******************************************************************

class read_bool : public simple_internal {
        input_file &infile;
    public:
        read_bool(input_file &infile);
        virtual void Compute(traverse_data &x, expr** pass, int np);
};

read_bool::read_bool(input_file &_inf)
    : simple_internal(em->BOOL, "read_bool", 1), infile(_inf)
{
    SetFormal(0, em->STRING, "prompt");
    SetDocumentation("Read a boolean value from the input stream.  If the current input stream is standard input, then the string given by \"prompt\" is displayed first.");
}

void read_bool::Compute(traverse_data &x, expr** pass, int np)
{
    DCASSERT(x.answer);
    DCASSERT(0==x.aggregate);
    DCASSERT(1==np);
    SafeCompute(pass[0], x);
    char c=' ';
    while (1) {
        if (infile.isClosed()) {
            if (!x.answer->isNull()) {
                em->cout() << "Enter the [y/n] value for ";
                DCASSERT(em->STRING);
                em->STRING->print(em->cout(), *x.answer);
                em->cout() << " : ";
                em->cout().flush();
            }
        }
        infile.get(c);
        if (c=='y' || c=='Y' || c=='n' || c=='N') break;
    }
    x.answer->setBool(c=='y' || c=='Y');
}

// ******************************************************************
// *                         read_int class                         *
// ******************************************************************

class read_int : public simple_internal {
        input_file &infile;
    public:
        read_int(input_file &inf);
        virtual void Compute(traverse_data &x, expr** pass, int np);
};

read_int::read_int(input_file &_inf)
    : simple_internal(em->INT, "read_int", 1), infile(_inf)
{
    SetFormal(0, em->STRING, "prompt");
    SetDocumentation("Read an integer value from the input stream.  If the current input stream is standard input, then the string given by \"prompt\" is displayed first.");
}

void read_int::Compute(traverse_data &x, expr** pass, int np)
{
    DCASSERT(x.answer);
    DCASSERT(x.parent);
    DCASSERT(0==x.aggregate);
    DCASSERT(1==np);
    SafeCompute(pass[0], x);
    if (infile.isClosed()) {
        if (!x.answer->isNull()) {
            em->cout() << "Enter the (integer) value for ";
            DCASSERT(em->STRING);
            em->STRING->print(em->cout(), *x.answer);
            em->cout() << " : ";
            em->cout().flush();
        }
    }
    long ans;
    if (!infile.get(ans)) {
        if (em->startError()) {
            em->causedBy(x.parent);
            em->cerr() << "Expecting integer from input stream";
            em->stopIO();
        }
        x.answer->setNull();
    } else {
        x.answer->setInt(ans);
    }
}

// ******************************************************************
// *                        read_real  class                        *
// ******************************************************************

class read_real : public simple_internal {
        input_file &infile;
    public:
        read_real(input_file &inf);
        virtual void Compute(traverse_data &x, expr** pass, int np);
};

read_real::read_real(input_file &_inf)
    : simple_internal(em->REAL, "read_real", 1), infile(_inf)
{
    SetFormal(0, em->STRING, "prompt");
    SetDocumentation("Read a real value from the input stream.  If the current input stream is standard input, then the string given by \"prompt\" is displayed first.");
}

void read_real::Compute(traverse_data &x, expr** pass, int np)
{
    DCASSERT(x.answer);
    DCASSERT(x.parent);
    DCASSERT(0==x.aggregate);
    DCASSERT(1==np);
    SafeCompute(pass[0], x);
    if (infile.isClosed()) {
        if (!x.answer->isNull()) {
            em->cout() << "Enter the (real) value for ";
            DCASSERT(em->STRING);
            em->STRING->print(em->cout(), *x.answer);
            em->cout() << " : ";
            em->cout().flush();
        }
    }
    double ans;
    if (!infile.get(ans)) {
        if (em->startError()) {
            em->causedBy(x.parent);
            em->cerr() << "Expecting real from input stream";
            em->stopIO();
        }
        x.answer->setNull();
    } else {
        x.answer->setReal(ans);
    }
}

// ******************************************************************
// *                       read_string  class                       *
// ******************************************************************

class read_string : public simple_internal {
        input_file &infile;
    public:
        read_string(input_file &_inf);
        virtual void Compute(traverse_data &x, expr** pass, int np);
};

read_string::read_string(input_file &_inf)
    : simple_internal(em->STRING, "read_string", 2), infile(_inf)
{
    SetFormal(0, em->STRING, "prompt");
    SetFormal(1, em->INT, "n");
    SetDocumentation("Read at most n characters, or until whitespace is seen, from the input stream.  If the current input stream is standard input, then the string given by \"prompt\" is displayed first.");
}

void read_string::Compute(traverse_data &x, expr** pass, int np)
{
    DCASSERT(x.answer);
    DCASSERT(x.parent);
    DCASSERT(0==x.aggregate);
    DCASSERT(2==np);
    SafeCompute(pass[1], x);
    if (!x.answer->isNormal()) {
        x.answer->setNull();
        return;
    }
    int length = x.answer->getInt();
    SafeCompute(pass[0], x);
    if (infile.isClosed()) {
        if (!x.answer->isNull()) {
            em->cout() << "Enter the (string, length " << length << ") value for ";
            DCASSERT(em->STRING);
            em->STRING->print(em->cout(), *x.answer);
            em->cout() << " : ";
            em->cout().flush();
        }
    }
    if (length <= 0) {
        x.answer->setNull();
        return;
    }
    char* buffer = new char[length+2];
    // Skip whitespace for first character
    char c;
    if (!infile.get(c)) {
        if (em->startError()) {
            em->causedBy(x.parent);
            em->cerr() << "End of input stream before expected string\n";
            em->stopIO();
        }
        x.answer->setNull();
        delete[] buffer;
        return;
    }
    buffer[0] = c;
    for (int i=1; i<length; i++) {
        int rc = infile.getc();
        if (EOF == rc) {
            buffer[i] = 0;
            break;
        }
        if ((rc==' ') || (rc=='\t') || (rc=='\n')) {
            buffer[i] = 0;
            break;
        }
        buffer[i] = rc;
    }
    buffer[length] = 0; // failsafe
    x.answer->setPtr(new shared_string(buffer));
}

// ******************************************************************
// *                        print_type class                        *
// ******************************************************************

class print_type : public custom_internal {
public:
  print_type();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

print_type::print_type()
 : custom_internal("print_type", "void print_type(x)")
{
  SetDocumentation("Print the type of expression x.");
}

void print_type::Compute(traverse_data &x, expr** pass, int np)
{
  if (x.answer)  if (x.stopExecution())  return;
  DCASSERT(1==np);
  if (0==pass)  return;
  if (pass[0]) {
    em->cout() << "Expression ";
    pass[0]->Print(em->cout(), 0);
    em->cout() << " has type: ";
    pass[0]->PrintType(em->cout());
  } else {
    em->cout() << "Expression null has type: null";
  }
  em->cout().Put('\n');
  em->cout().flush();
}

int print_type::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = em->VOID;
        return 0;

    case traverse_data::Typecheck:
        if (np<1)  return NotEnoughParams(np);
        if (np>1)  return TooManyParams(np);
        return 0;

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

// ******************************************************************
// *                      generic_print  class                      *
// ******************************************************************

class generic_print : public custom_internal {
public:
  generic_print(const char* name, const char* header);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  void compute(OutputStream &s, traverse_data &x, expr** pass, int np);
};

generic_print::generic_print(const char* n, const char* h)
 : custom_internal(n, h)
{
}

void generic_print::compute(OutputStream &s, traverse_data &x,
        expr** pass, int np)
{
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result item;
  result width;
  result prec;
  for (int i=0; i<np; i++) {
    // Compute next item to print
    if (0==pass[i]) {
      item.setNull();
      DCASSERT(em->INT);
      em->INT->print(s, item);
      continue;
    }
    x.aggregate = 0;
    x.answer = &item;
    pass[i]->Compute(x);
    // Determine width, if any
    if (pass[i]->NumComponents()>1) {
      x.answer = &width;
      x.aggregate = 1;
      pass[i]->Compute(x);
    } else {
      width.setNull();
    }
    // Determine precision, if any
    if (pass[i]->NumComponents()>2) {
      x.answer = &prec;
      x.aggregate = 2;
      pass[i]->Compute(x);
    } else {
      prec.setNull();
    }

    // Print!
    DCASSERT(pass[i]->Type(0));
    if (width.isNull()) {
      pass[i]->Type(0)->print(s, item);
    } else if(prec.isNull()) {
      pass[i]->Type(0)->print(s, item, width.getInt());
    } else {
      pass[i]->Type(0)->print(s, item, width.getInt(), prec.getInt());
    }
  }
  x.answer = answer;
  x.aggregate = 0;
}

int generic_print::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::Typecheck:
        for (int i=0; i<np; i++) {
          if (0==pass[i])  continue;
          const type* t = pass[i]->Type(0);
          if (!t || ! t->isPrintable())     return BadParam(i, np);
          if (pass[i]->NumComponents()==1)  continue;
          if (pass[i]->Type(1) != em->INT)  return BadParam(i, np);
          if (pass[i]->NumComponents()==2)  continue;
          if (pass[i]->Type(0) != em->REAL) return BadParam(i, np);
          if (pass[i]->Type(2) != em->INT)  return BadParam(i, np);
          if (pass[i]->NumComponents()==3)  continue;
          return BadParam(i, np);
        } // for i
        return 0;

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

// ******************************************************************
// *                         print_ci class                         *
// ******************************************************************

class print_ci : public generic_print {
public:
  print_ci();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual void PrintDocs(doc_formatter* df, const char*) const;
};

print_ci::print_ci() : generic_print("print", "print(arg1, arg2, ...)")
{
}

void print_ci::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(em->hasIO());
  compute(em->cout(), x, pass, np);
  em->cout().flush();
}

int print_ci::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = em->VOID;
        return 0;

    default:
        return generic_print::Traverse(x, pass, np);
  }
}

void print_ci::PrintDocs(doc_formatter* df, const char*) const
{
  if (0==df)    return;
  df->begin_heading();
  PrintHeader(df->Out(), true);
  df->end_heading();
  df->begin_indent();
  df->Out() << "Print each argument to output stream. ";
  df->Out() << "Arguments can be any printable type, and may include ";
  df->Out() << "an optional width specifier as \"arg:width\". ";
  df->Out() << "Real arguments may also specify the number of digits of ";
  df->Out() << "precision, as \"arg:width:prec\" (the format of reals is ";
  df->Out() << "specified with the option RealFormat).  ";
  df->Out() << "Strings may include the following special characters:";
  df->begin_description(2);
  df->item("\\a");
  df->Out() << "audible bell";
  df->item("\\b");
  df->Out() << "backspace";
  df->item("\\n");
  df->Out() << "newline";
  df->item("\\q");
  df->Out() << "double quote: \"";
  df->item("\\t");
  df->Out() << "tab character";
  df->end_description();
  df->end_indent();
}

// ******************************************************************
// *                        sprint_ci  class                        *
// ******************************************************************

class sprint_ci : public generic_print {
  StringStream strbuffer;
public:
  sprint_ci();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

sprint_ci::sprint_ci() : generic_print("sprint", "sprint(arg1, arg2, ...)")
{
  SetDocumentation("Just like \"print\", except the result is written into a string, which is returned.");
}

void sprint_ci::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  compute(strbuffer, x, pass, np);
  char* bar = strbuffer.GetString();
  strbuffer.flush();
  x.answer->setPtr(new shared_string(bar));
}

int sprint_ci::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = em->STRING;
        return 0;

    default:
        return generic_print::Traverse(x, pass, np);
  }
}

// ******************************************************************
// *                       generic_file class                       *
// ******************************************************************

class generic_file : public simple_internal {
public:
  generic_file(const char* name);
  void compute(DisplayStream &s, traverse_data &x, expr** pass, int np) const;
};

generic_file::generic_file(const char* name)
 : simple_internal(em->BOOL, name, 1)
{
  SetFormal(0, em->STRING, "filename");
}

void generic_file::compute(DisplayStream &s, traverse_data &x, expr** pass, int np) const
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  SafeCompute(pass[0], x);
  if (x.answer->isNull()) {
#ifdef DEBUG_FILE
      fprintf(stderr, "Switching stream to normal display\n");
#endif
      s.SwitchDisplay(0);
      x.answer->setBool(true);
      return;
  }
  shared_string* xss = smart_cast <shared_string*> (x.answer->getPtr());
  DCASSERT(xss);
#ifdef DEBUG_FILE
  fprintf(stderr, "Switching stream to file %s...\n", xss->getStr());
#endif
  FILE* outfile = fopen(xss->getStr(), "a");
  if (outfile) {
      s.SwitchDisplay(outfile);
      x.answer->setBool(true);
#ifdef DEBUG_FILE
      fprintf(stderr, "...successful\n");
#endif
  } else {
      x.answer->setBool(false);
#ifdef DEBUG_FILE
      fprintf(stderr, "...error opening\n");
#endif
  }
}

// ******************************************************************
// *                       output_file  class                       *
// ******************************************************************

class output_file : public generic_file {
public:
  output_file();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

output_file::output_file() : generic_file("output_file")
{
  SetDocumentation("Append the output stream to the specified filename. If the file does not exist, it is created. If the filename is null, the output stream is switched to standard output. Returns true on success.");
}

void output_file::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (em->hasIO())  compute(em->cout(), x, pass, np);
  else              x.answer->setBool(false);
}

// ******************************************************************
// *                       report_file  class                       *
// ******************************************************************

class report_file : public generic_file {
public:
  report_file();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

report_file::report_file() : generic_file("report_file")
{
  SetDocumentation("Append the report stream to the specified filename. If the file does not exist, it is created. If the filename is null, the report stream is switched to standard output. Returns true on success.");
}

void report_file::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (em->hasIO())  compute(em->report(), x, pass, np);
  else              x.answer->setBool(false);
}

// ******************************************************************
// *                       warning_file class                       *
// ******************************************************************

class warning_file : public generic_file {
public:
  warning_file();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

warning_file::warning_file() : generic_file("warning_file")
{
  SetDocumentation("Append the warning stream to the specified filename. If the file does not exist, it is created. If the filename is null, the warning stream is switched to standard error. Returns true on success.");
}

void warning_file::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (em->hasIO())  compute(em->warn(), x, pass, np);
  else              x.answer->setBool(false);
}

// ******************************************************************
// *                        error_file class                        *
// ******************************************************************

class error_file : public generic_file {
public:
  error_file();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

error_file::error_file() : generic_file("error_file")
{
  SetDocumentation("Append the error stream to the specified filename. If the file does not exist, it is created. If the filename is null, the error stream is switched to standard error. Returns true on success.");
}

void error_file::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (em->hasIO())  compute(em->cerr(), x, pass, np);
  else              x.answer->setBool(false);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_iofuncs : public initializer {
  public:
    init_iofuncs();
    virtual bool execute();
};
init_iofuncs the_iofunc_initializer;

init_iofuncs::init_iofuncs() : initializer("init_iofuncs")
{
  usesResource("em");
  usesResource("st");
  usesResource("types");
}

bool init_iofuncs::execute()
{
  if (0==st || 0==em)  return false;

  input_file* inf = new input_file;
  st->AddSymbol(  inf  );

  st->AddSymbol(  new read_bool(*inf)   );
  st->AddSymbol(  new read_int(*inf)    );
  st->AddSymbol(  new read_real(*inf)   );
  st->AddSymbol(  new read_string(*inf) );

  st->AddSymbol(  new print_type    );
  st->AddSymbol(  new sprint_ci     );
  st->AddSymbol(  new print_ci      );
  st->AddSymbol(  new output_file   );
  st->AddSymbol(  new report_file   );
  st->AddSymbol(  new warning_file  );
  st->AddSymbol(  new error_file    );

  return true;
}


