
// $Id$

#include "api.h"

// Some action option functions

void SetVerbose(void *x, const char*, int)
{
  bool *b = (bool*)x;
  if (b[0]) {
    Verbose.Activate();
  } else {
    Verbose.Deactivate();
  }
}

void GetVerbose(void *x)
{
  bool *b = (bool*)x;
  b[0] = Verbose.IsActive();
}

void SetReport(void *x, const char*, int)
{
  bool *b = (bool*)x;
  if (b[0]) {
    Report.Activate();
  } else {
    Report.Deactivate();
  }
}

void GetReport(void *x)
{
  bool *b = (bool*)x;
  b[0] = Report.IsActive();
}


void InitBase()
{
  InitStreams();

  // initialize options
  const char* vdoc = "Should SMART turn on the verbose output stream";
  const char* range = "[false, true]";
  const char* def = "false";
  option *verbose 
    = MakeActionOption(BOOL, "Verbose", def, vdoc, range, SetVerbose, GetVerbose);
  AddOption(verbose);

  const char* rdoc = "Should SMART turn on the report output stream";
  option *report 
    = MakeActionOption(BOOL, "Report", def, rdoc, range, SetReport, GetReport);
  AddOption(report);
}
