
// $Id$

/**
     Implementation of error streams
*/

#include "errors.h"

#include <sstream>
#include <iostream>

using namespace std;

ErrorStream::ErrorStream(const char* et)
{
  errortype = et;
  out = NULL;
  display = NULL;
  ready = false;
  active = false;
}

void ErrorStream::Start(char* filename, int lineno)
{
  if (active) {
    out = new ostringstream;
    ready = true;
    *out << errortype;
    if (filename) {
      if (filename[0]=='-' && filename[1]==0) {
        // standard input
        *out << " in standard input";
      } else if (filename[0]=='>' && filename[1]==0) {
	// command line
	*out << " on command line";
	lineno = -1;
      } else {
        *out << " in file " << filename;
      }
    }
    if (lineno>=0)
      *out << " near line " << lineno;
    *out << ":\n\t";
  }
}

void ErrorStream::Stop()
{
  if (active) {
    *display << out->str().c_str() << endl;
    delete out;
    out = NULL;
    ready = false;
  }
}

void InternalStream::Start(char *srcfile, int srcline, char* fn, int ln)
{
  if (active) {
    out = new ostringstream;
    ready = true;
    *out << errortype;
    *out << " in " << srcfile << " at " << srcline;
    if (fn) {
      *out << " caused by ";
      if (fn[0]=='-' && fn[1]==0) {
        // standard input
        *out << "standard input";
      } else if (fn[0]=='>' && fn[1]==0) {
	// command line
	*out << "command line";
      } else {
        *out << "file " << fn;
      }
      if (ln>=0)
        *out << " near line " << ln;
    }
    *out << ":\n\t";
  }
}

ErrorStream Error("ERROR");
ErrorStream Warn("WARNING");
InternalStream Internal("INTERNAL");

void InitErrorStreams() 
{
  Error.SetDisplay(&cout);
  Error.Activate();

  Warn.SetDisplay(&cout);
  Warn.Activate();

  Internal.SetDisplay(&cout);
  Internal.Activate();
}

