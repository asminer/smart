
// $Id$

/**
     Implementation of error streams
*/

#include "errors.h"

#include <sstream>
#include <iostream>

using namespace std;

ErrorStream::ErrorStream(const char* et, ostream *d) : OutputStream(d)
{
  errortype = et;
  active = false;
}

void ErrorStream::Start(const char* filename, int lineno)
{
  if (active) {
    Out() << errortype;
    ready = true;
    if (filename) {
      if (filename[0]=='-' && filename[1]==0) {
        Out() << " in standard input";
      } else if (filename[0]=='>' && filename[1]==0) {
	Out() << " on command line";
	lineno = -1;
      } else {
        Out() << " in file " << filename;
      }
    }
    if (lineno>=0)
      Out() << " near line " << lineno;
    Out() << ":\n\t";
  }
}

void ErrorStream::Stop()
{
  if (active) {
    Out() << endl;
    ready = false;
  }
}

void InternalStream::Start(const char *srcfile, int srcline, 
			   const char* fn, int ln)
{
  if (active) {
    ready = true;
    Out() << errortype;
    Out() << " in " << srcfile << " at " << srcline;
    if (fn) {
      Out() << " caused by ";
      if (fn[0]=='-' && fn[1]==0) {
        Out() << "standard input";
      } else if (fn[0]=='>' && fn[1]==0) {
	Out() << "command line";
      } else {
        Out() << "file " << fn;
      }
      if (ln>=0)
        Out() << " near line " << ln;
    }
    Out() << ":\n\t";
  }
}

void InternalStream::Stop()
{
  if (active) {
    Out() << endl;
    ready = false;
  }
  exit(0);
}



ErrorStream Error("ERROR", &cout);
ErrorStream Warn("WARNING", &cout);
InternalStream Internal("INTERNAL", &cout);

void InitErrorStreams() 
{
  Error.Activate();
  Warn.Activate();
  Internal.Activate();
}

