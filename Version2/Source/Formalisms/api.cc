
// $Id$

#include "api.h"

// documentation display
#include "../Base/docs.h"

// symbol tables:
#include "../Main/tables.h"

// formalisms:
#include "mc.h"
#include "spn.h"

// generic model functions:
#include "modelfuncs.h"

model* MakeNewModel(const char* fn, int line, type t, char* name, formal_param
**pl, int np)
{
  switch(t) {
    case DTMC:
    case CTMC:
    	return MakeMarkovChain(t, name, pl, np, fn, line);

    case SPN:
	return MakePetriNet(t, name, pl, np, fn, line);

  }
  
  // still here?  Bad input file?
  Internal.Start(__FILE__, __LINE__, fn, line);
  Internal << "Cannot build model of type " << GetType(t) << "\n";
  Internal.Stop();

  // Won't get here
  return NULL;
}

bool CanDeclareType(type modeltype, type vartype)
{
  switch (modeltype) {
    case DTMC:
    case CTMC:
    		return (vartype == STATE);

    case SPN:
    		return (vartype == PLACE) || (vartype == TRANS);
  }

  // slipped through the cracks
  return false;
}

// Symbol tables
PtrTable GenericModelFuncs;
PtrTable MCModelFuncs;
PtrTable PNModelFuncs;

List <function> *FindModelFunctions(type modeltype, const char* n)
{
  // first check generic functions
  List <function> *answer = FindFunctions(&GenericModelFuncs, n);
  if (answer) return answer;

  // then check model specific functions
  switch (modeltype) {

    case DTMC:
    case CTMC:
    		return FindFunctions(&MCModelFuncs, n);
	
    case SPN:
		return FindFunctions(&PNModelFuncs, n);

    // bail out
    default:
    	return NULL;
  }

  // Keep compiler happy
  return NULL;
}

void InitModels()
{
  /* 
	Initialize model-specific and generic model builtin functions
  */
  
  // Deal with generics
  InitGenericModelFunctions(&GenericModelFuncs);

  // Deal with formalism specifics
  InitMCModelFuncs(&MCModelFuncs);
  InitPNModelFuncs(&PNModelFuncs);
  // New formalisms here...
}

// Implemented below;
void ShowModelDocs(void *x); 

// required by ShowModelDocs

type EnclosingModel;

void HelpModelFuncs()
{
  // Traverse symbol table common to all formalisms
  EnclosingModel = ANYMODEL;
  GenericModelFuncs.Traverse(ShowModelDocs);

  // Traverse Markov chain formalism
  EnclosingModel = DTMC;
  MCModelFuncs.Traverse(ShowModelDocs);

  // Traverse PN formalism
  EnclosingModel = SPN;
  PNModelFuncs.Traverse(ShowModelDocs);
  
  // New formalisms here...
}


// When adding a new formalism, you
// shouldn't have to touch anything below here 
// =======================================================================

// Global for help, defined and set in fnlib.cc
extern char* help_search_string;

void ShowModelDocs(void *x)
{
  PtrTable::splayitem *foo = (PtrTable::splayitem *) x;
  if (NULL==strstr(foo->name, help_search_string)) return;  // not interested...
  bool unshown = true;
  List <function> *bar = (List <function> *)foo->ptr;
  int i;
  for (i=0; i<bar->Length(); i++) {
    function *hit = bar->Item(i);
    DCASSERT(hit->isWithinModel());
    if (hit->IsUndocumented()) continue;  // don't show this to the public
    // show to the public
    if (unshown) {
      Output << foo->name << " within ";
      switch (EnclosingModel) {
	case DTMC:
		Output << "a Markov chain";
		break;
	case ANYMODEL:
		Output << "any";
		break;
	default:
		Output << "a " << GetType(EnclosingModel);
      }
      Output << " model\n";
      unshown = false;
    }
    Output.Pad(' ', 5);
    hit->ShowHeader(Output);
    // special type checking: documentation must display parameters, too
    const char* d = hit->GetDocumentation();
    if (d) DisplayDocs(Output, d, 5, 75, true);
    else {
      Internal.Start(__FILE__, __LINE__);
      Internal << "No documentation for this function?\n";
      Internal.Stop();
    }
    Output << "\n";
  }
}

