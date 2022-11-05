
/**
     Implementation of streams
*/

#include "../include/defines.h"
#include "../Utils/location.h"
#include "../Utils/strings.h"
#include "streams.h"

#include <string.h>
#include <stdlib.h>

#define CATCH_TERM

#ifdef CATCH_TERM
#include <signal.h>
#endif

// #define DEBUG_STREAM


// ==================================================================
// |                                                                |
// |                      OutputStream methods                      |
// |                                                                |
// ==================================================================

void OutputStream::Panic(const char* msg) const
{
  if (parent) {
    parent->Panic(msg);
  } else {
    fputs(msg, stderr);
    exit(2);
  }
}

void OutputStream::ExpandBuffer(int wantsize)
{
  if (wantsize < bufsize) return;
  int newsize = bufsize;
  while (wantsize >= newsize) {
    newsize *= 2;
    if (newsize > 1000000)  Panic("output buffer overflow");
  }
  char* newbuffer = (char*) realloc(buffer, newsize);
  if (0==newbuffer) Panic("memory error for output buffer");
  bufsize = newsize;
  buffer = newbuffer;  // just in case
}

OutputStream::OutputStream()
{
  parent = 0;
  static const int initsize = 1024;
  buffer = (char*) malloc(initsize);
  bufsize = initsize;
  buftop = 0;
  buffer[0] = 0;
  ready = true;
  SetRealFormat(RF_GENERAL);
  intbuf = (char*) malloc(256);
  thousands = new shared_string(strdup(""));
}

void OutputStream::SetParent(const io_environ* p)
{
  parent = p;
}

OutputStream::~OutputStream()
{
  free(buffer);
}

void OutputStream::Pad(char space, int s)
{
  if (!ready) return;
  if (s<=0)   return;
  ExpandBuffer(buftop+s+1);
  for (; s>0; s--) {
    buffer[buftop] = space;
    buftop++;
  }
  buffer[buftop] = 0;
}

void OutputStream::Put(bool data)
{
  if (!ready) return;
  ExpandBuffer(buftop+6);
  if (data) {
      strcpy(bufptr(), "true");
      buftop += 4;
  } else {
      strcpy(bufptr(), "false");
      buftop += 5;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(char data)
{
  if (!ready) return;
  ExpandBuffer(buftop+1);
  buffer[buftop] = data;
  buftop++;
  buffer[buftop] = 0;
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(int data)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%d", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%d", data);
#endif
  PutInteger(intbuf, 0);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(long data)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%ld", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%ld", data);
#endif
  PutInteger(intbuf, 0);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(unsigned long data)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%lu", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%lu", data);
#endif
  PutInteger(intbuf, 0);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(long long data)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%lld", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%lld", data);
#endif
  PutInteger(intbuf, 0);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::PutHex(unsigned char data)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), "%#02x", data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), "%#02x", data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::PutHex(unsigned long data)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), "%#010lx", data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), "%#010lx", data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(float data)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), floatformat, data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), floatformat, data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
#ifdef DEBUG_STREAM
  fprintf(stderr, "out << float %f format %s\n", data, floatformat);
#endif
}

void OutputStream::Put(double data)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), doubleformat, data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), doubleformat, data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
#ifdef DEBUG_STREAM
  fprintf(stderr, "out << double %lf format %s\n", data, doubleformat);
#endif
}

void OutputStream::Put(const char* data)
{
  if (!ready) return;
  int len = strlen(data);
  ExpandBuffer(buftop+len+2);
  strncpy(bufptr(), data, len);
  buftop+=len;
  buffer[buftop]=0;
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(bool data, int width)
{
  if (!ready) return;
  if (data) {
    Pad(' ', width-4);
    Put("true");
  } else {
    Pad(' ', width-5);
    Put("false");
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(long data, int width)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%ld", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%ld", data);
#endif
  PutInteger(intbuf, width);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(unsigned long data, int width)
{
  if (!ready) return;
#ifdef DEVELOPMENT_CODE
  int size = snprintf(intbuf, 256, "%lu", data);
  DCASSERT(size < 256);
#else
  snprintf(intbuf, 256, "%lu", data);
#endif
  PutInteger(intbuf, width);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(double data, int width)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), doublewidthformat, width, data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), doublewidthformat, width, data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
  DCASSERT(buffer[buftop]==0);
#ifdef DEBUG_STREAM
  fprintf(stderr, "out << double %lf witdh %d\n", data, width);
#endif
}

void OutputStream::Put(char data, int width)
{
  if (!ready) return;
  if (width > 1) Pad(' ', width-1);
  ExpandBuffer(buftop+1);
  buffer[buftop] = data;
  buftop++;
  buffer[buftop] = 0;
  if (width < -1)  Pad(' ', (-width)-1);
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(const char* data, int width)
{
  if (!ready) return;
  int len = strlen(data);
  if (width >= 0) {
    Pad(' ', width-len);
  }
  ExpandBuffer(buftop+len+2);
  strncpy(bufptr(), data, len);
  buftop+=len;
  buffer[buftop] = 0;
  if (width < 0) {
    Pad(' ', (-width)-len);
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(double data, int width, int prec)
{
  if (!ready) return;
  int size = snprintf(bufptr(), bufspace(), doubleprecformat, width, prec, data);
  if (size>=bufspace()) { // there wasn't enough space
    ExpandBuffer(buftop+size+1);
    size = snprintf(bufptr(), bufspace(), doubleprecformat, width, prec, data);
    DCASSERT(size < bufspace());
  }
  buftop += size;
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::PutInteger(const char* data, int width)
{
  if (!ready) return;
  const int thousands_len = thousands->length();
  if (0==thousands_len) {
    Put(data, width);
    return;
  }
  int len = strlen(data);
  int next_comma = len % 3;
  DCASSERT(len>0);
  bool negative = (data[0] == '-');
  int num_commas = negative ? (len-2)/3 : (len-1)/3;
  len += num_commas * thousands_len;
  // pad in front if necessary
  if (width >= 0) {
    Pad(' ', width-len);
  }
  // make space for integer
  ExpandBuffer(buftop+len+2);
  // print integer with commas
  int i;
  if (negative) {
    buffer[buftop] = data[0];
    buftop++;
    buffer[buftop] = data[1];
    buftop++;
    i = 2;
  } else {
    buffer[buftop] = data[0];
    buftop++;
    i = 1;
  }
  if (i>next_comma) next_comma += 3;
  while (data[i]) {
    if (i==next_comma) {
      strncpy(bufptr(), thousands->getStr(), thousands_len);
      buftop += thousands_len;
      next_comma += 3;
    }
    buffer[buftop] = data[i];
    buftop++;
    i++;
  } // while
  // pad in back if necessary
  if (width < 0) {
    Pad(' ', (-width)-len);
  }
  buffer[buftop]=0;
}


// TBD: deprecated;
// use location::show instead
void OutputStream::PutFile(const char* fn, int ln)
{
  if (!ready) return;
  if (0==fn)  return;
  if (0==fn[1]) {
    // special files
    switch (fn[0]) {
      case '-':
          Put("in standard input");
          break;

      case '>':
          Put("on command line");
          ln = -1;
          break;

      case '<':
          Put("at end of input");
          ln = -1;
          break;

      default:
          Put("in file");
          Put(fn);
    } // switch
  } else if (' ' == fn[0]) {
    Put(fn+1);
  } else {
    Put("in file ");
    Put(fn);
  }
  if (ln>=0) {
    Put(" near line ");
    Put(ln);
  }
}

void OutputStream::PutMemoryCount(size_t bytes, int prec)
{
  if (!ready) return;
  double kilo = bytes / 1024.0;
  double mega = kilo / 1024.0;
  double giga = mega / 1024.0;
  double tera = giga / 1024.0;

  const char* units = " bytes";
  double show = bytes;
  if (tera > 1.0) {
    show = tera;
    units = " Tibytes";
  } else if (giga > 1.0) {
    show = giga;
    units = " Gibytes";
  } else if (mega > 1.0) {
    show = mega;
    units = " Mibytes";
  } else if (kilo > 1.0) {
    show = kilo;
    units = " Kibytes";
  }
  if (show >= 10.0)  prec++;
  if (show >= 100.0)  prec++;
  Put(show, 0, prec);
  Put(units);
}

void OutputStream::can_flush()
{
  // nothing!
}

void OutputStream::SetRealFormat(real_format rf)
{
  switch (rf) {
    case RF_FIXED:
        floatformat = "%f";
        doubleformat = "%f";
        doublewidthformat = "%*f";
        doubleprecformat = "%*.*f";
        return;

    case RF_SCIENTIFIC:
        floatformat = "%e";
        doubleformat = "%e";
        doublewidthformat = "%*e";
        doubleprecformat = "%*.*e";
        return;

    default:  // includes RF_GENERAL
        floatformat = "%g";
        doubleformat = "%g";
        doublewidthformat = "%*g";
        doubleprecformat = "%*.*g";
  } // switch
}

OutputStream::real_format OutputStream::GetRealFormat() const
{
  switch (floatformat[1]) {
    case 'f':   return RF_FIXED;
    case 'e':   return RF_SCIENTIFIC;
    default:    return RF_GENERAL;
  }
}


// ==================================================================
// |                                                                |
// |                      StringStream methods                      |
// |                                                                |
// ==================================================================

StringStream::StringStream() : OutputStream()
{
}

char* StringStream::GetString() const
{
  DCASSERT(buffer[buftop]==0);
  return strdup(buffer);
}

void StringStream::flush()
{
  buftop = 0;
  buffer[0] = 0;
}

// ==================================================================
// |                                                                |
// |                     DisplayStream  methods                     |
// |                                                                |
// ==================================================================

const int DisplayStream::display_buffer_size = 1023;

DisplayStream::DisplayStream() : OutputStream()
{
  display = deflt = 0;
}

DisplayStream::DisplayStream(FILE* d)
{
  display = deflt = d;
}

void DisplayStream::Initialize(FILE* d)
{
  display = deflt = d;
}

DisplayStream::~DisplayStream()
{
  SwitchDisplay(0);
}

void DisplayStream::SwitchDisplay(FILE* out)
{
  flush();
  if (display != deflt) {
    fclose(display);
  }
  display = out ? out : deflt;
}

void DisplayStream::Activate()
{
  ready = true;
}

void DisplayStream::Deactivate()
{
  ready = false;
}

void DisplayStream::flush()
{
  if (ready) if (buftop) {
    fputs(buffer, display);
    fflush(display);
    buftop = 0;
    buffer[0] = 0;
  }
}

void DisplayStream::can_flush()
{
  Check();
}


// ==================================================================
// |                                                                |
// |                       io_environ methods                       |
// |                                                                |
// ==================================================================

io_environ::io_environ()
{
  WhichError = 0;
  // Input.Initialize(stdin);

  Output.Initialize(stdout);
  Output.SetParent(this);
  Output.Activate();

  Report.Initialize(stdout);
  Report.SetParent(this);
  Report.Deactivate();

  Warning.Initialize(stderr);
  Warning.SetParent(this);
  Warning.Deactivate();

  Error.Initialize(stderr);
  Error.SetParent(this);
  Error.Deactivate();

  Internal.Initialize(stderr);
  Internal.SetParent(this);
  Internal.Deactivate();

  sigx = 0;
  catchterm = false;
  indents = 0;
}

io_environ::~io_environ()
{
  Output.flush();
}

#ifdef CATCH_TERM
void io_environ::ResumeTerm()
{
  catchterm = false;
  if (0==sigx) return;
  // bail out
  Output.flush();
  Error.Activate();
  Error << "Caught signal " << sigx << ", terminating.\n";
  Error.flush();
  Error.Deactivate();
  exit(3);
}
#endif

bool io_environ::StartInternal(const char* srcfile, int srcline)
{
  if (WhichError) {
    Panic("already writing to error stream");
    return false;  // just in case
  }
  WhichError = 3;
  Internal.Activate();
  Internal << "INTERNAL in file " << srcfile << " on line " << srcline;
  indents = 1;
  return true;
}


bool io_environ::StartError(const location &L, const char* text)
{
    if (WhichError) {
        Panic("already writing to error stream");
        return false;  // just in case
    }
    WhichError = 2;
    Error.Activate();
    Error.Put("ERROR");
    indents = 1;

    if (L) {
        Error << " " << L;
        if (text) {
            Error << " at text '" << text << "'";
        }
    }
    Error.Put(':');
    NewLine();

    return true;
}

bool io_environ::StartWarning(const location &L, const char* text)
{
    if (WhichError) {
        Panic("already writing to error stream");
        return false;  // just in case
    }
    WhichError = 1;
    Warning.Activate();
    Warning.Put("WARNING");
    indents = 1;

    if (L) {
        Warning << " " << L;
        if (text) {
            Warning << " at text '" << text << "'";
        }
    }
    Warning.Put(':');
    NewLine();

    return true;
}



bool io_environ::StartError()
{
  if (WhichError) {
    Panic("already writing to error stream");
    return false;  // just in case
  }
  WhichError = 2;
  Error.Activate();
  Error.Put("ERROR");
  indents = 1;
  return true;
}

bool io_environ::StartWarning()
{
  if (WhichError) {
    Panic("already writing to error stream");
    return false;  // just in case
  }
  WhichError = 1;
  Warning.Activate();
  Warning.Put("WARNING");
  indents = 1;
  return true;
}

void io_environ::CausedBy(const char* file, int line)
{
  DisplayStream* out = 0;
  bool cont = false;
  switch (WhichError) {
    case 3:
        out = &Internal;
        cont = true;
        break;

    case 2:
        out = &Error;
        break;

    case 1:
        out = &Warning;
        break;

    default:
        return;
  }
  if (file) {
    if (cont) {
      NewLine();
      out->Put("caused ");
    } else {
      out->Put(' ');
    }
    out->PutFile(file, line);
  }
  out->Put(':');
  NewLine();
}

void io_environ::NoCause()
{
  switch (WhichError) {
    case 3:
        Internal.Put(':');
        NewLine();
        return;

    case 2:
        Error.Put(':');
        NewLine();
        return;

    case 1:
        Warning.Put(':');
        NewLine();
        return;
  }
}

void io_environ::ChangeIndent(int delta)
{
  indents += delta;
  if (indents<0) indents = 0;
}

void io_environ::NewLine()
{
  switch (WhichError) {
    case 3:
        Internal.Put("\n");
        Internal.Pad(' ', indents*4);
        return;

    case 2:
        Error.Put("\n");
        Error.Pad(' ', indents*4);
        return;

    case 1:
        Warning.Put("\n");
        Warning.Pad(' ', indents*4);
        return;
  }
}

void io_environ::NewLine(const char* what)
{
  Report.Put('\n');
  Report << what << ": ";
  Report.Pad(' ', indents*4);
}

bool io_environ::StartReport(const char* what)
{
  if (WhichError) {
    Panic("already writing to error stream");
    return false;  // just in case
  }
  Output.flush();
  Report.Activate();
  Report << what << ": ";
  indents = 0;
  return true;
}

void io_environ::Stop()
{
  switch (WhichError) {
    case 3:
        Internal.Put('\n');
        Output.flush();
        Internal.flush();
        Internal.Deactivate();
        Exit();
        break;

    case 2:
        Error.Put('\n');
        Output.flush();
        Error.flush();
        Error.Deactivate();
        break;

    case 1:
        Warning.Put('\n');
        Output.flush();
        Warning.flush();
        Warning.Deactivate();
        break;

    default:
        Output.flush();
        Report.flush();
        Report.Deactivate();
  }
  WhichError = 0;
}

void io_environ::Exit()
{
  exit(1);
}

void io_environ::Panic(const char* msg) const
{
  fputs(msg, stderr);
  exit(2);
}

#ifdef CATCH_TERM
io_environ* catcher = 0;

void mycatcher(int sig)
{
  DCASSERT(catcher);
  catcher->raiseTerm(sig);
}
#endif

#ifdef CATCH_TERM
void CatchSignals(io_environ *e)
{
  if (e) {
    if (catcher) {
      e->StartInternal(__FILE__, __LINE__);
      e->NoCause();
      e->Internal << "Multiple sigterm catchers";
      e->Stop();
    } else {
      catcher = e;
#ifdef SIGHUP
      signal(SIGHUP, mycatcher);
#endif
      signal(SIGINT, mycatcher);
      signal(SIGTERM, mycatcher);
    }
  } else {
    catcher = 0;
#ifdef SIGHUP
    signal(SIGHUP, SIG_DFL);
#endif
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
  }
}
#else
void CatchSignals(io_environ* )
{
}
#endif

// ==================================================================
// |                                                                |
// |                     doc_formatter  methods                     |
// |                                                                |
// ==================================================================

doc_formatter::doc_formatter()
{
}

doc_formatter::~doc_formatter()
{
}

bool doc_formatter::Matches(const char* item, const char* keyword) const
{
  if (NULL==keyword) return true;
  int slen = strlen(keyword);
  int last = strlen(item) - slen;
  for (int i=0; i<=last; i++) {
    if (0==strncasecmp(item+i, keyword, slen)) return true;
  }
  return false;
}

#include "textfmt.h"

