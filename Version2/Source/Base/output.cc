
// $Id$

/**
     Implementation of output streams
*/

#include "output.h"

#include <iostream>

using namespace std;

OutputStream::OutputStream(std::ostream* d)
{
  deflt = d;
  display = d;
  active = false;
}

void OutputStream::SwitchDisplay(std::ostream* out)
{
    if (display != deflt) { 
      delete display; // I assume this will close it?
    }  
    display = out ? out : deflt;
}


OutputStream Output(&cout);
OutputStream Verbose(&cout);
OutputStream Report(&cout);

void InitOutputStreams() 
{
  Output.active = true;
  Verbose.active = false;
  Report.active = false;
}

