
// $Id$

/**
     Implementation of streams
*/

#include "streams.h"
#include "stdlib.h"  // for exit

// ==================================================================
// |                                                                |
// |                      OutputStream methods                      |
// |                                                                |
// ==================================================================

OutputStream::OutputStream(FILE* d)
{
  display = deflt = d;
  ready = true; 
}

void OutputStream::SwitchDisplay(FILE* out)
{
    if (display != deflt) { 
      fclose(display);
    }  
    display = out ? out : deflt;
}

void OutputStream::Activate()
{
  ready = true;
}

void OutputStream::Deactivate()
{
  ready = false;
}

void OutputStream::Put(bool data)
{
  if (ready) {
    if (data) fputs("true", display);
    else fputs("false", display);
  }
}

void OutputStream::Put(char data)
{
  if (ready) {
    fputc(data, display);
  }
}

void OutputStream::Put(int data)
{
  if (ready) {
    fprintf(display, "%d", data);
  }
}

void OutputStream::Put(float data)
{
  if (ready) {
    /* Eventually.. check options for how to print floats */
    fprintf(display, "%f", data);
  }
}

void OutputStream::Put(double data)
{
  if (ready) {
    // check options here
    fprintf(display, "%lf", data);
  }
}

void OutputStream::Put(const char* data)
{
  if (ready) {
    fputs(data, display);
  }
}

// ==================================================================
// |                                                                |
// |                      ErrorStream  methods                      |
// |                                                                |
// ==================================================================

ErrorStream::ErrorStream(const char* et, FILE* d) : OutputStream(d)
{
  errortype = et;
  active = false;
}

void ErrorStream::Start(const char* filename, int lineno)
{
  if (active) {
    ready = true;
    Put(errortype);
    ready = true;
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
    ready = false;
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
    ready = false;
  }
  exit(0);
}


// ==================================================================
// |                                                                |
// |                      InputStream  methods                      |
// |                                                                |
// ==================================================================


// ==================================================================
// |                                                                |
// |                         Global Streams                         |
// |                                                                |
// ==================================================================

OutputStream Output(stdout);
OutputStream Verbose(stdout);
OutputStream Report(stdout);
ErrorStream Error("ERROR", stderr);
ErrorStream Warn("WARNING", stderr);
InternalStream Internal("INTERNAL", stderr);

void InitStreams() 
{
  Output.Activate();
  Verbose.Deactivate();
  Report.Deactivate();
  Error.Activate();
  Warn.Activate();
  Internal.Activate();
}

