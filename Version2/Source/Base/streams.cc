
// $Id$

/**
     Implementation of streams
*/

#include "streams.h"
#include "options.h"
#include "../defines.h"

// defined in topmost api...
void smart_exit();

// Output options:

option* real_format;
option_const* RF_GENERAL;
option_const* RF_FIXED;
option_const* RF_SCIENTIFIC;

// ==================================================================
// |                                                                |
// |                      OutputStream methods                      |
// |                                                                |
// ==================================================================

void OutputStream::ExpandBuffer(int wantsize)
{
  if (wantsize < bufsize) return;
  int newsize = bufsize;
  while (wantsize >= newsize) {
    newsize *= 2;
    if (newsize > 1000000) {
      fprintf(stderr, "Smart Panic: Output buffer overflow, bailing out\n");
      exit(SMART_PANIC);
    }
  }
  char* newbuffer = (char*) realloc(buffer, newsize);
  if (NULL==newbuffer) {
    fprintf(stderr, "Smart Panic: memory error for output buffer\n");
    exit(SMART_PANIC);
  }
  bufsize = newsize;
  buffer = newbuffer;  // just in case
}

void OutputStream::DetermineRealFormat()
{
  const option_const* foo = real_format->GetEnum();
  if (foo==RF_GENERAL) {
    	floatformat = "%g";
	doubleformat = "%lg";
	doublewidthformat = "%*lg";
	doubleprecformat = "%*.*lg";
	return;
  }
  if (foo==RF_FIXED) {
    	floatformat = "%f";
	doubleformat = "%lf";
	doublewidthformat = "%*lf";
	doubleprecformat = "%*.*lf";
        return;
  }
  if (foo==RF_SCIENTIFIC) {
    	floatformat = "%e";
	doubleformat = "%le";
	doublewidthformat = "%*le";
	doubleprecformat = "%*.*le";
	return;
  }
  fprintf(stderr, "Smart Panic: unknown real format ");
  if (foo) fprintf(stderr, "\"%s\" ", foo->name);
  fprintf(stderr, "for output\n");
  exit(SMART_PANIC);
}

OutputStream::OutputStream()
{
  static const int initsize = 1024;
  buffer = (char*) malloc(initsize);
  bufsize = initsize;
  buftop = 0;
  buffer[0] = 0;
  ready = true; 
}

OutputStream::~OutputStream()
{
  free(buffer);
}

void OutputStream::Pad(int s)
{
  if (!ready) return;
  if (s<=0) return;
  ExpandBuffer(buftop+s+1);
  for (; s>0; s--) {
    buffer[buftop] = ' ';
    buftop++;
  }
  buffer[buftop] = 0;
}

void OutputStream::Put(bool data)
{
  if (ready) {
    ExpandBuffer(buftop+6);
    if (data) {
      strcpy(bufptr(), "true");
      buftop += 4;
    } else {
      strcpy(bufptr(), "false");
      buftop += 5;
    }
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(char data)
{
  if (ready) {
    ExpandBuffer(buftop+1);
    buffer[buftop] = data;
    buftop++;
    buffer[buftop] = 0;
  } 
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(int data)
{
  if (ready) {
    int size = snprintf(bufptr(), bufspace(), "%d", data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), "%d", data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(float data)
{
  if (ready) {
    DetermineRealFormat();
    int size = snprintf(bufptr(), bufspace(), floatformat, data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), floatformat, data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(double data)
{
  if (ready) {
    DetermineRealFormat();
    int size = snprintf(bufptr(), bufspace(), doubleformat, data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), doubleformat, data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(const char* data)
{
  if (ready) {
    int len = strlen(data);
    ExpandBuffer(buftop+len+2);
    strncpy(bufptr(), data, len);
    buftop+=len;
    buffer[buftop]=0;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(bool data, int width)
{
  if (ready) {
    if (data) {
      Pad(width-4);
      Put("true");
    } else {
      Pad(width-5);
      Put("false");
    }
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(int data, int width)
{
  if (ready) {
    int size = snprintf(bufptr(), bufspace(), "%*d", width, data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), "%*d", width, data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(double data, int width)
{
  if (ready) {
    DetermineRealFormat();
    int size = snprintf(bufptr(), bufspace(), doublewidthformat, width, data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), doublewidthformat, width, data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(const char* data, int width)
{
  if (ready) {
    int len = strlen(data);
    Pad(width-len);
    ExpandBuffer(buftop+len+2);
    strncpy(bufptr(), data, len);
    buftop+=len;
  }
  DCASSERT(buffer[buftop]==0);
}

void OutputStream::Put(double data, int width, int prec)
{
  if (ready) {
    DetermineRealFormat();
    int size = snprintf(bufptr(), bufspace(), doubleprecformat, width, prec, data);
    if (size>=bufspace()) { // there wasn't enough space
      ExpandBuffer(buftop+size+1);
      size = snprintf(bufptr(), bufspace(), doubleprecformat, width, prec, data);
      DCASSERT(size < bufspace());
    }
    buftop += size;
  }
  DCASSERT(buffer[buftop]==0);
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
  return strndup(buffer, buftop+1);
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

DisplayStream::DisplayStream(FILE* d) : OutputStream()
{
  display = deflt = d;
}

DisplayStream::~DisplayStream()
{
  SwitchDisplay(NULL);
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
  if (ready) {
    fputs(buffer, display);
    fflush(display);
    buftop = 0;
    buffer[0] = 0;
  }
}

// ==================================================================
// |                                                                |
// |                      ErrorStream  methods                      |
// |                                                                |
// ==================================================================

ErrorStream::ErrorStream(const char* et, FILE* d) : DisplayStream(d)
{
  errortype = et;
  active = false;
}

void ErrorStream::Start(const char* filename, int lineno)
{
  if (active) {
    ready = true;
    Put(errortype);
    if (filename) {
      if (filename[0]=='-' && filename[1]==0) {
        Put(" in standard input");
      } else if (filename[0]=='>' && filename[1]==0) {
	Put(" on command line");
	lineno = -1;
      } else {
        Put(" in file ");
	Put(filename);
      }
    }
    if (lineno>=0) {
      Put(" near line ");
      Put(lineno);
    }
    Put(":\n\t");
  }
}

void ErrorStream::Stop()
{
  if (active) {
    Put("\n");
    flush();
    ready = false;
  }
}

void ErrorStream::Continue(const char* filename, int lineno)
{
  if (active) {
    ready = true;
    Put("    Caused by");
    if (filename) {
      if (filename[0]=='-' && filename[1]==0) {
        Put(" standard input");
      } else if (filename[0]=='>' && filename[1]==0) {
	Put(" command line");
	lineno = -1;
      } else {
        Put(" file ");
	Put(filename);
      }
    }
    if (lineno>=0) {
      Put(" near line ");
      Put(lineno);
    }
    Put(":\n\t");
  }
}

// ==================================================================
// |                                                                |
// |                     InternalStream methods                     |
// |                                                                |
// ==================================================================

void InternalStream::Start(const char *srcfile, int srcline, 
			   const char* fn, int ln)
{
  if (active) {
    ready = true;
    Put(errortype);
    Put(" in ");
    Put(srcfile);
    Put(" at ");
    Put(srcline);
    if (fn) {
      Put(" caused by ");
      if (fn[0]=='-' && fn[1]==0) {
        Put("standard input");
      } else if (fn[0]=='>' && fn[1]==0) {
	Put("command line");
      } else {
        Put("file ");
	Put(fn);
      }
      if (ln>=0) {
        Put(" near line ");
        Put(ln);
      }
    }
    Put(":\n\t");
  }
}

void InternalStream::Stop()
{
  if (active) {
    Put("\n");
    flush();
    ready = false;
  }
  smart_exit();
  exit(INTERNAL_ERROR);
}


// ==================================================================
// |                                                                |
// |                      InputStream  methods                      |
// |                                                                |
// ==================================================================

InputStream::InputStream(FILE* in)
{
  input = deflt = in;
}

void InputStream::SwitchInput(FILE* in)
{
  if (input!=deflt) 
    fclose(input);
  if (in) input = in;
  else input = deflt;
}

// ==================================================================
// |                                                                |
// |                         Global Streams                         |
// |                                                                |
// ==================================================================

DisplayStream Output(stdout);
DisplayStream Verbose(stdout);
DisplayStream Report(stdout);
ErrorStream Error("ERROR", stderr);
ErrorStream Warning("WARNING", stderr);
InternalStream Internal("INTERNAL", stderr);
InputStream Input(stdin);

void InitStreams() 
{
  Output.Activate();
  Verbose.Deactivate();
  Report.Deactivate();
  Error.Activate();
  Warning.Activate();
  Internal.Activate();
}

