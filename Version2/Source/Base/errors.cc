
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
      } if (filename[0]=='>' && filename[1]==0) {
	// command line
	*out << " on command line";
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

ErrorStream Error("ERROR");
ErrorStream Warn("WARNING");
ErrorStream Internal("INTERNAL");

void InitStreams() 
{
  Error.SetDisplay(&cout);
  Error.Activate();

  Warn.SetDisplay(&cout);
  Warn.Activate();

  Internal.SetDisplay(&cout);
  Internal.Activate();
}

