
// $Id$

#include "api.h"
#include "docs.h"

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


// For output options:
extern option* real_format;
extern option_const* RF_GENERAL;
extern option_const* RF_FIXED;
extern option_const* RF_SCIENTIFIC;


// help functions

void show_types(OutputStream &disp, int LM, int RM)
{
  DisplayDocs(disp, 
  	"Documentation about types, modifiers, proc and anything else that relates to types goes here.  Long and nicely formatted, so we use the handy DisplayDocs function.",
  LM, RM, false);

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Simple types:\n";
  for (type t=FIRST_SIMPLE; t<EXPO; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Stochastic types:\n";
  for (type t=EXPO;  t<=LAST_SIMPLE; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Process types:\n";
  for (type t=FIRST_PROC; t<=LAST_PROC; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Model types:\n";
  for (type t=DTMC; t<=LAST_MODEL; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Model variable types:\n";
  for (type t=FIRST_VOID; t<=LAST_VOID; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }

  disp.Put('\n');
  disp.Pad(' ', LM);
  disp << "Set types:\n";
  for (type t=FIRST_SET; t<=LAST_SET; t++) {
    disp.Pad(' ', LM+5);
    disp << GetType(t) << "\n";
  }
  disp << "\n\n";
}

// Hook
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

  RF_GENERAL = new option_const("GENERAL", "\aSame as printf(%g)");
  RF_FIXED = new option_const("FIXED", "\aSame as printf(%f)");
  RF_SCIENTIFIC = new option_const("SCIENTIFIC", "\aSame as printf(%e)");

  option_const** things = new option_const*[3];
  things[0] = RF_FIXED;
  things[1] = RF_GENERAL;
  things[2] = RF_SCIENTIFIC;

  real_format = MakeEnumOption("RealFormat", "Format to use for output of reals",
  			things, 3, RF_GENERAL);

  AddOption(real_format);

  // initialize the basic help topics

  AddTopic("topics", ShowTopicNames);
  AddTopic("types", show_types);
}
