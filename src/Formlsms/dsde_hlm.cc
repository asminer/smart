
// $Id$

#include "../ExprLib/exprman.h"

#include "dsde_hlm.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/sets.h"
#include "../SymTabs/symtabs.h"
#include "../include/heap.h"

// #define DEBUG_PRIO
// #define DEBUG_ENABLED

// ******************************************************************
// *                                                                *
// *                       model_event methods                      *
// *                                                                *
// ******************************************************************

const char* model_event::nameOf(firing_type t)
{
  switch (t) {
    case Nondeterm:       return "Non-deterministic";
    case Hidden:          return "Hidden (and non-deterministic)";
    case Immediate:       return "Immediate";
    case Expo:            return "Timed (exponential)";
    case Phase_int:       return "Timed (phase int)";
    case Phase_real:      return "Timed (phase real)";
    case Timed_general:   return "Timed (general)";
    default:              return "Unknown";
  }
  return 0; // won't get here, avoid compiler warning
}

void model_event::buildDepList(expr* e, intset* ld, intset* vd)
{
  ld->removeAll();
  vd->removeAll();
  if (0==e) return;
  List <symbol> L;
  e->BuildSymbolList(traverse_data::GetSymbols, 0, &L);

  for (int i=0; i<L.Length(); i++) {
    symbol* s = L.Item(i);
    DCASSERT(s);
    model_statevar* mv = dynamic_cast <model_statevar*> (s);
    if (0==mv) continue;
    CHECK_RANGE(1, mv->GetPart(), ld->getSize());
    CHECK_RANGE(0, mv->GetIndex(), vd->getSize());
    ld->addElement(mv->GetPart());
    vd->addElement(mv->GetIndex());
  } 
}

model_event::model_event(const symbol* wrapper, const model_instance* p)
 : model_var(wrapper, p)
{
  Init();
}

model_event::model_event(const char* fn, int line, const type* t, char* n, 
  const model_instance* p) : model_var(fn, line, t, n, p)
{
  Init();
}

void model_event::Init()
{
  FT = Unknown;
  enabling = 0;
  nextstate = 0;
  distro = 0;
  weight = 0;
  wc = 0;
  has_prio_level = 0;
  prio_level = 0;
  prio_list_dynamic = 0;
  prio_list = 0;
  prio_length = 0;
  setStateType(No_State);
  enabling_level_dependencies = 0;
  enabling_variable_dependencies = 0;
  nextstate_level_dependencies = 0;
  nextstate_variable_dependencies = 0;
}

model_event::~model_event()
{
  delete enabling_level_dependencies;
  delete enabling_variable_dependencies;
  delete nextstate_level_dependencies;
  delete nextstate_variable_dependencies;
  delete prio_list_dynamic;
  delete[] prio_list;
}

void model_event::display(OutputStream &s) const
{
  const int width = 16;
  s << nameOf(FT) << " " << Name() << "\n";
  s.Put("enabling: ", width);
  if (enabling) enabling->Print(s, 0); else s << "null";
  s << "\n";
  s.Put("next state: ", width);
  if (nextstate) nextstate->Print(s, 0); else s << "null";
  s << "\n";
  if (distro) {
    s.Put("distro: ", width);
    distro->Print(s, 0);
    s << "\n";
  }
  if (weight) {
    s.Put("weight: ", width);
    weight->Print(s, 0);
    s << "\n";
    s.Put("wt class: ", width);
    s << wc << "\n";
  }
}

void model_event::setEnabling(expr *e)
{
  DCASSERT(e); // at least should be constant "true"
  DCASSERT(e->Type(0) == em->BOOL || e->Type(0) == em->BOOL->addProc());
  DCASSERT(0==enabling);
  enabling = e;
  if (enabling) enabling->PreCompute();
}

void model_event::setNextstate(expr *e)
{
  DCASSERT(0==nextstate);
  nextstate = e;
  if (nextstate) nextstate->PreCompute();
}

void model_event::setNondeterministic()
{
  DCASSERT(Unknown == FT);
  FT = Nondeterm;
}

void model_event::setHidden()
{
  DCASSERT(Unknown == FT);
  FT = Hidden;
}

void model_event::setTimed(expr *dist)
{
  DCASSERT(Unknown == FT);
  DCASSERT(0==distro);
  if (0==dist) return;
  distro = dist;
  distro->PreCompute();
  
  const type* dt = distro->Type(0);
  DCASSERT(dt);

  const type* et = em->EXPO;
  if (et) {
    if (dt == et || dt == et->addProc())  {
      FT = Expo;
      return;
    }
  }

  const type* pi = em->INT;
  DCASSERT(pi);
  pi = pi->modifyType(PHASE);
  if (pi) {
    if (dt == pi || dt == pi->addProc()) {
      FT = Phase_int;
      return;
    }
  }

  const type* pr = em->REAL;
  DCASSERT(pr);
  pr = pr->modifyType(PHASE);
  if (pr) {
    if (dt == pr || dt == pr->addProc()) {
      FT = Phase_real;
      return;
    }
  }

  FT = Timed_general;
}

void model_event::setImmediate()
{
  DCASSERT(Unknown == FT);
  DCASSERT(0==distro);
  FT = Immediate;
}

void model_event::setWeight(int cl, expr *w)
{
  DCASSERT(0==weight);
  DCASSERT(cl>0);
  wc = cl;
  weight = w;
  if (weight) weight->PreCompute();
}

void model_event
::finishPriorityInfo(List <model_event> &keep, List <model_event> *ignored)
{
  has_prio_level = true;

  DCASSERT(0==prio_list);
  if (0==prio_list_dynamic) return;
  DCASSERT(prio_length>0);

  // remove anything from the list that has a different priority level.
  keep.Clear();
  for (int i=0; i<prio_list_dynamic->Length(); i++) {
    model_event* e = prio_list_dynamic->Item(i);
    if (e->getPriorityLevel() != getPriorityLevel()) {
      if (ignored) ignored->Append(e);
    } else {
      keep.Append(e);
    }
  }
  delete prio_list_dynamic;
  prio_list_dynamic = 0;

  prio_length = keep.Length();
  prio_list = keep.CopyAndClear();
#ifdef DEBUG_PRIO
  fprintf(stderr, "Priority list for event %s: ", Name());
  for (int i=0; i<prio_length; i++) {
    if (i) fprintf(stderr, ", ");
    fprintf(stderr, "%s", prio_list[i]->Name());
  }
  fprintf(stderr, "\n");
#endif
}

void model_event::decideEnabled(traverse_data &x)
{
#ifdef DEBUG_ENABLED
  fprintf(stderr, "IN  decideEnabled for event %s\n", Name());
#endif
  DCASSERT(x.answer);
  DCASSERT(unknown == enable_data);
  for (int i = prio_length-1; i>=0; i--) {
    DCASSERT(prio_list[i]);
    if (prio_list[i]->unknownIfEnabled()) prio_list[i]->decideEnabled(x);
    if (!x.answer->isNormal()) return;
    if (prio_list[i]->isEnabled()) {
      enable_data = disabled;
      x.answer->setBool(false);
#ifdef DEBUG_ENABLED
      fprintf(stderr, "OUT, priority disabled event %s, enable_data is %d\n", 
              Name(), enable_data);
#endif
      return;
    }
  } // for i
  // still here? might be enabled!
  if (0==enabling) {  
    // Empty enabling condition means "true".
    x.answer->setBool(true);
  } else {
    enabling->Compute(x);
  }
  if (x.answer->isNormal() && x.answer->getBool()) {
    enable_data = enabled;
  } else {
    enable_data = disabled;
  }
#ifdef DEBUG_ENABLED
  fprintf(stderr, "OUT decideEnabled for event %s, enable_data is %d\n", Name(), enable_data);
#endif
}


// ******************************************************************
// *                                                                *
// *                        dsde_hlm  methods                       *
// *                                                                *
// ******************************************************************

named_msg dsde_hlm::ignored_prio;

dsde_hlm::dsde_hlm(const model_instance* p, model_statevar** sv, int nv, 
                    model_event** ed, int ne)
: hldsm(Unknown)
{
  SetParent(p);
  state_data = sv;
  num_vars = nv;
  event_data = ed;
  num_events = ne;
  assertions = 0;
  num_assertions = 0;
  num_priolevels = 0;
  last_timed = last_immed = 0;
  lltype = lldsm::Unknown;
  determineModelType();
  ProcessEvents();
}

dsde_hlm::~dsde_hlm()
{
  // trash the assertions
  for (int e=0; e<num_assertions; e++) {
    Delete(assertions[e]);
  }
  delete[] assertions;

  // trash the events
  for (int e=0; e<num_events; e++) {
    Delete(event_data[e]);
  }
  delete[] event_data;
  delete[] last_immed;
  delete[] last_timed;

  // trash the state variables
  for (int i=0; i<num_vars; i++) {
    Delete(state_data[i]);
  }
  delete[] state_data;
}

lldsm::model_type dsde_hlm::GetProcessType() const
{
  DCASSERT(lldsm::Unknown != lltype);
  return lltype;
}

int dsde_hlm::NumStateVars() const
{
  return num_vars;
}

bool dsde_hlm::containsListVar() const
{
  for (int i=0; i<num_vars; i++) {
    DCASSERT(state_data);
    if (state_data[i]->getStateType() == model_var::Int_List) return true;
  }
  return false;  
}

void dsde_hlm::determineListVars(bool* ilv) const
{
  for (int i=0; i<num_vars; i++) {
    DCASSERT(state_data);
    ilv[i] = (state_data[i]->getStateType() == model_var::Int_List);
  }
}

void dsde_hlm::reindexStateVars(int &start)
{
  for (int i=0; i<num_vars; i++) {
    DCASSERT(state_data[i]);
    // TBD: check type of state_data[i] here.
    state_data[i]->SetIndex(start);
    start++;
  } // for i
}

void dsde_hlm::useDefaultVarOrder()
{
  int num_levels = 0;
  for (int i=0; i<num_vars; i++) {
    num_levels = MAX(num_levels, state_data[i]->GetPart());
  }

  if (1==num_levels) {
    if (em->startWarning()) {
      em->noCause();
      em->warn() << "user-specified partition groups all variables together;";
      em->newLine();
      em->warn() << "possibly missing call to partition()?";
      DoneError();
    }
  }

  setPartInfo((const model_statevar**)state_data, num_vars);
}

void dsde_hlm::checkAssertions(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  x.answer->setBool(true);
  for (int i=0; i<num_assertions; i++) {
    DCASSERT(assertions);
    DCASSERT(assertions[i]);
    assertions[i]->Compute(x);
    if (x.answer->isNormal() && x.answer->getBool()) continue;  // passed
    if (StartError(assertions[i])) {
        em->cerr() << "Assertion ";
        assertions[i]->Print(em->cerr(), 0);
        em->cerr() << " failed in state ";
        showState(em->cerr(), x.current_state);
        DoneError();
    }
    // make sure we consistently return "false"
    x.answer->setBool(false);
    return;
  } // for i
}

void dsde_hlm::checkVanishing(traverse_data &x)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  ResetEnabledList();
  int i = 0;
  for (int pl=0; ; pl++) {
    // For this priority level
    // Check immediates; if any are enabled, then we're vanishing.
    for (; i<last_immed[pl]; i++) {
      DCASSERT(event_data[i]);
      if (event_data[i]->unknownIfEnabled()) {
        event_data[i]->decideEnabled(x);  
        if (!x.answer->isNormal())  return;
        if (x.answer->getBool())    return;  
        continue;
      }
      if (event_data[i]->isEnabled()) {
        x.answer->setBool(true);
        return;
      }
    } // for i
    if (pl+1 == num_priolevels) break;  // no more immediates to check

    // Check timed; if any are enabled, then we're tangible.
    for (; i<last_timed[pl]; i++) {
      DCASSERT(event_data[i]);
      if (event_data[i]->unknownIfEnabled()) {
        event_data[i]->decideEnabled(x);  
        if (!x.answer->isNormal())  return;
        if (x.answer->getBool())    {
          x.answer->setBool(!x.answer->getBool());
          return;
        }
        continue;
      }
      if (event_data[i]->isEnabled()) {
        x.answer->setBool(false);
        return;
      }
    } // for i
  } // for pl
  x.answer->setBool(false);
}

void dsde_hlm::makeEnabledList(traverse_data &x, List <model_event> *EL)
{
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  if (EL) EL->Clear();
  ResetEnabledList();
  int i = 0;
  for (int pl=0; pl<num_priolevels; pl++) {
    for (; i<last_timed[i]; i++) {
      if (event_data[i]->unknownIfEnabled()) {
        event_data[i]->decideEnabled(x);
      } else {
        x.answer->setBool(event_data[i]->isEnabled());
      }
  
      if (!x.answer->isNormal()) {
        if (EL) {
          EL->Clear();
          EL->Append(event_data[i]);
        }
        return;
      }
  
      if (x.answer->getBool() && EL)  EL->Append(event_data[i]);
    } // for i
  } // for pl
}

void dsde_hlm
::makeVanishingEnabledList(traverse_data &x, List <model_event> *EL)
{
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  if (EL) EL->Clear();
  ResetEnabledList();
  bool has_enabled = false;
  bool first_enabled = true;
  int the_wc_index = 0;
  int i = 0;
  for (int pl=0; pl<num_priolevels; pl++) {
    for (; i<last_immed[pl]; i++) {
      if (event_data[i]->unknownIfEnabled()) {
        event_data[i]->decideEnabled(x);
      } else {
        x.answer->setBool(event_data[i]->knownEnabled());
      }
  
      if (!x.answer->isNormal()) {
        if (EL) {
          EL->Clear();
          EL->Append(event_data[i]);
        }
        return;
      } // if not normal
  
      if (x.answer->getBool()) {
        has_enabled = true;
        if (EL) EL->Append(event_data[i]);
        if (first_enabled) {
          the_wc_index = i;
          first_enabled = false;
        } else if (event_data[the_wc_index]->getWeightClass() 
                    != event_data[i]->getWeightClass()) {
        
          // two events with different weight class, both enabled
          if (EL) {
            EL->Clear();
            EL->Append(event_data[the_wc_index]);
            EL->Append(event_data[i]);
          }
          x.answer->setNull();
          return;
        } // if first enabled
      } // if event enabled
    } // for i
    if (has_enabled) return;
    for (; i<last_timed[pl]; i++) {
      event_data[i]->setDisabled();
    }
  } // for pl
}

void dsde_hlm
::makeTangibleEnabledList(traverse_data &x, List <model_event> *EL)
{
  DCASSERT(x.answer);
  DCASSERT(x.current_state);
  if (EL) EL->Clear();
  ResetEnabledList();

  int i=0;
  bool has_enabled = false;
  for (int pl=0; pl<num_priolevels; pl++) {
    for (; i<last_immed[pl]; i++) {
      DCASSERT(event_data[i]);
      event_data[i]->setDisabled();
    } // for i
    for (; i<last_timed[pl]; i++) {
      if (event_data[i]->unknownIfEnabled()) {
        event_data[i]->decideEnabled(x);
      } else {
        x.answer->setBool(event_data[i]->knownEnabled());
      }
  
      if (!x.answer->isNormal()) {
        if (EL) {
          EL->Clear();
          EL->Append(event_data[i]);
        }
        return;
      }
      if (x.answer->getBool()) {
        has_enabled = true;
        if (EL)  EL->Append(event_data[i]);
      }
    } // for i
    if (has_enabled) return;
  } // for pl
}



void dsde_hlm::determineModelType()
{
  DCASSERT(Type() == Unknown);

  // Finalize event priorities
  List <model_event> tmp;
  List <model_event>* display = 0;
  if (ignored_prio.isActive()) {
    display = new List <model_event>;
  }
  bool have_ignored = false;
  for (int i=0; i<num_events; i++) {
    event_data[i]->finishPriorityInfo(tmp, display);
    if (0==display || 0==display->Length()) continue;

    if (!have_ignored) {
      have_ignored = StartWarning(ignored_prio, 0);
      if (!have_ignored) continue;
      em->warn() << "Ignoring priority pairs across priority levels:";
      em->changeIndent(1);
    }
    em->newLine();
    em->warn() << "{";
    for (int j=0; j<display->Length(); j++) {
      if (j) em->warn() << ", ";
      em->warn() << display->Item(j)->Name();
    } // for j
    em->warn() << "} : " << event_data[i]->Name();

    display->Clear();
  }
  if (have_ignored) {
    em->changeIndent(-1);
    DoneWarning();
  }
  delete display;

  // TBD: check for cyclic dependencies in priority lists here

  // Determine the model type based on event firings.
  int num_asynch = 0;
  int num_synch = 0;
  int num_general = 0;
  int num_nondeterm = 0;
  for (int i=0; i<num_events; i++) {

    switch (event_data[i]->getFiringType()) {
      case model_event::Nondeterm:
      case model_event::Hidden:
        num_nondeterm++;
        continue;

      case model_event::Phase_int:
        num_synch++;
        continue;

      case model_event::Timed_general:
        num_general++;
        continue;

      case model_event::Expo:
      case model_event::Phase_real:
        num_asynch++;
        continue;

      case model_event::Immediate:
        continue;

      default:
        // unknown?
        DCASSERT(0);
    } // switch
  } // for i

  if (num_events == num_nondeterm) {
    setType(Asynch_Events);
    lltype = lldsm::FSM;
    return;
  }

  DCASSERT(num_nondeterm == 0);   // deal with MDPs later

  if (num_general || (num_asynch && num_synch)) {
    setType(General_Events);
    lltype = lldsm::GSP;
    return;
  }

  if (num_synch) {
    setType(Synch_Events);
    lltype = lldsm::DTMC;
    return;
  }

  if (num_asynch) {
    setType(Asynch_Events);
    lltype = lldsm::CTMC;
    return;
  }

  // all immediate, I suppose this works:
  setType(Asynch_Events);
  lltype = lldsm::CTMC;
}

void dsde_hlm::ProcessEvents()
{
#ifdef DEBUG_PRIO
  em->cout() << "Ordering events by priority level, then by timing rules.\n";
#endif
  HeapSortAbstract(this, num_events);

  // Count number of priority levels (use 1 for priority lists)
  num_priolevels = 1;
  if (num_events) {
    int last = event_data[0]->getPriorityLevel();
    for (int i=1; i<num_events; i++) {
      if (last != event_data[i]->getPriorityLevel()) {
        last = event_data[i]->getPriorityLevel();
        num_priolevels++;
      }
    } // for i
  }
#ifdef DEBUG_PRIO
  em->cout() << num_priolevels << " priority levels\n";
#endif

  // Build indexes by priority level
  last_immed = new int[num_priolevels];
  last_timed = new int[num_priolevels];
  int pl = 0;
  bool last_was_immed = true;
  int e = 0;
  if (num_events) {
    int last_level = event_data[0]->getPriorityLevel();
    for ( ; e<num_events; e++) {
      // did we change levels?
      if (event_data[e]->getPriorityLevel() != last_level) {
        DCASSERT(event_data[e]->getPriorityLevel() < last_level);
        last_level = event_data[e]->getPriorityLevel();
        if (last_was_immed) last_immed[pl] = e;
        last_timed[pl] = e;
        pl++;
        CHECK_RANGE(0, pl, num_priolevels);
        last_was_immed = event_data[e]->actsLikeImmediate();
        if (!last_was_immed) last_immed[pl] = e;
        continue;
      }
      // did we change from immed to timed?
      if (event_data[e]->actsLikeImmediate() != last_was_immed) {
        DCASSERT(last_was_immed);
        last_immed[pl] = e;
        last_was_immed = false;
      }
    } // for e
  }
  if (last_was_immed) last_immed[pl] = e;
  last_timed[pl] = e;

#ifdef DEBUG_PRIO
  em->cout() << "Successfully reordered events, new order:\n";
  e = 0;
  for (int pl=0; pl<num_priolevels; pl++) {
    em->cout() << "Priority level " << event_data[e]->getPriorityLevel();
    em->cout() << "\n\tImmediate:\n";
    for (; e<last_immed[pl]; e++) {
      em->cout() << "\t\t" << event_data[e]->Name() << "\n";
    }
    em->cout() << "\tTimed:\n";
    for (; e<last_timed[pl]; e++) {
      em->cout() << "\t\t" << event_data[e]->Name() << "\n";
    }
  } // for pl
  em->cout().flush();
#endif
}



// **************************************************************************
// *                                                                        *
// *                            dsde_def methods                            *
// *                                                                        *
// **************************************************************************

named_msg dsde_def::dup_part;
named_msg dsde_def::no_part;
named_msg dsde_def::dup_prio;

dsde_def::dsde_def(const char* fn, int line, const type* t, char*n, formal_param **pl, int np) : model_def(fn, line, t, n, pl, np)
{
}

dsde_def::~dsde_def()
{
}

void dsde_def
::SetLevelOfStateVars(const expr* call, int level, shared_set* pset)
{
  DCASSERT(pset);
  result elem;
  bool reset_warning = false;
  for (int z=0; z<pset->Size(); z++) {
    pset->GetElement(z, elem);
    DCASSERT(elem.isNormal());
    model_var* foo = smart_cast <model_var*> (elem.getPtr());
    DCASSERT(foo);  
    if (!isVariableOurs(foo, call, "ignoring partition assignment")) continue;

    model_statevar* pl = dynamic_cast <model_statevar*> (foo);
    // TBD: should write an error message
    DCASSERT(pl);
    if (pl->GetPart()) {
      if (reset_warning) {
        em->warn() << ", " << pl->Name();
      } else {
        reset_warning = StartWarning(dup_part, call);
        if (reset_warning) {
          em->warn() << "Moving {" << pl->Name();
        }
      }
    } 
    pl->SetPart(level);
  } // for z
  if (reset_warning) {
    em->warn() << "} into group " << level;
    DoneWarning();
  }
}

void dsde_def::PartitionVars(model_statevar** V, int nv)
{
  DCASSERT(V);

  // get max and min group numbers
  int min_g = 0;
  int max_g = 0;
  for (int i=0; i<nv; i++) {
    min_g = MIN(min_g, V[i]->GetPart());
    max_g = MAX(max_g, V[i]->GetPart());
  }
  DCASSERT(min_g <= 0);
  DCASSERT(max_g >= 0);

  // group places with same partition index
  int N = max_g - min_g + 1;
  DCASSERT(N>0);
  model_statevar** group = new model_statevar*[N];
  for (int i=0; i<N; i++) group[i] = 0;
  for (int i=0; i<nv; i++) {
    int n = V[i]->GetPart() - min_g;
    CHECK_RANGE(0, n, N);
    V[i]->LinkTo(group[n]);
    group[n] = V[i];
  }  // for i

  // warning for ungrouped vars, but only if some vars have been grouped
  if ((max_g > min_g) && group[-min_g] && StartWarning(no_part, 0)) {
    em->warn() << "Places {";
    for (symbol* ptr = group[-min_g]; ptr; ptr=ptr->Next()) {
      if (ptr != group[-min_g]) em->warn() << ", ";
      em->warn() << ptr->Name();
    }
    em->warn() << "} default to group 0";
    DoneWarning();
  }
    
  // renumber the groups, from 1 to L
  // reorder the state variables, by groups
  int gnum = 0;
  int pnum = nv-1;
  for (int i=0; i<N; i++) {
    if (0==group[i]) continue;
    gnum++;
    while (group[i]) {
      symbol* next = group[i]->Next();
      group[i]->LinkTo(0);
      group[i]->SetPart(gnum);
      // reorder places
      group[i]->SetIndex(pnum);
      V[pnum] = group[i];
      pnum--;
      group[i] = smart_cast <model_statevar*> (next);
    }
  } // for i
  delete[] group;
  
#ifdef DEBUG_PART
  for (int i=0; i<nv; i++) {
    em->cout() << V[i]->Name();
    em->cout() << " has group: " << V[i]->GetPart() << "\n";
    em->cout().flush();
  }
#endif
}

void dsde_def::SetPriorityLevel(const expr* call, int level, shared_set* tset)
{
  DCASSERT(tset);
  result elem;
  List <model_event> reset;
  for (int z=0; z<tset->Size(); z++) {
    tset->GetElement(z, elem);
    DCASSERT(elem.isNormal());
    model_var* foo = smart_cast <model_var*> (elem.getPtr());
    DCASSERT(foo);  
    if (!isVariableOurs(foo, call, "ignoring priority assignment")) continue;

    model_event* t = dynamic_cast <model_event*> (foo);
    DCASSERT(t);

    if (t->hasPriorityLevel()) {
      reset.Append(t);
    }
    t->setPriorityLevel(level);
  } // for z

  if (reset.Length() && StartWarning(dup_prio, call)) {
    em->warn() << "Reset event";
    if (reset.Length()>1) em->warn() << "s {";
    else em->warn() << " ";
    for (int i=0; i<reset.Length(); i++) {
      if (i) em->warn() << ", ";
      em->warn() << reset.Item(i)->Name();
    }
    if (reset.Length()>1) em->warn() << "}";
    em->warn() << " to priority level " << level;
    DoneWarning();
  }
}

void dsde_def::SetPriorityOver(const expr* call, shared_set* a, shared_set* b)
{
  DCASSERT(a);
  DCASSERT(b);
  result ae, be;

  // Check ownership first
  for (int z=0; z<a->Size(); z++) {
    a->GetElement(z, ae);
    DCASSERT(ae.isNormal());
    model_var* foo = smart_cast <model_var*> (ae.getPtr());
    DCASSERT(foo);
    if (!isVariableOurs(foo, call, "ignoring priority assignments")) continue;
    DCASSERT(smart_cast <model_event*> (foo));
  }
  for (int z=0; z<b->Size(); z++) {
    b->GetElement(z, be);
    DCASSERT(be.isNormal());
    model_var* foo = smart_cast <model_var*> (be.getPtr());
    DCASSERT(foo);
    if (!isVariableOurs(foo, call, "ignoring priority assignments")) continue;
    DCASSERT(smart_cast <model_event*> (foo));
  }

  // Ok, enumerate all pairs
  for (int az=0; az<a->Size(); az++) {
    a->GetElement(az, ae);
    model_event* at = smart_cast <model_event*> (ae.getPtr());
    for (int bz=0; bz<b->Size(); bz++) {
      b->GetElement(bz, be);
      model_event* bt = smart_cast <model_event*> (be.getPtr());

      // at has priority over bt
      bt->addToPriorityList(at);

    } // for bz
  } // for az
}

void dsde_def::InitModel()
{
  last_level = 0;
}

// ********************************************************
// *                   dsde_part1 class                   *
// ********************************************************

class dsde_part1 : public model_internal {
public:
  dsde_part1(const type* place);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dsde_part1::dsde_part1(const type* place)
: model_internal(em->VOID, "partition", 2)
{
  DCASSERT(place);
  SetFormal(1, place->getSetOfThis(), "vset");
  SetRepeat(1);
  SetDocumentation("Groups state variables into submodels.  Each call to this version of partition makes a new group of state variables.  Equivalent to calling partition({first set}:-1, {second set}:-2, ...)");
}

void dsde_part1::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dsde_def* mdl = smart_cast<dsde_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result pset;
  x.answer = &pset;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    SafeCompute(pass[i], x);
    DCASSERT(pset.isNormal());
    shared_set* ps = smart_cast <shared_set*> (pset.getPtr());
    mdl->SetLevelOfStateVars(x.parent, ps);
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                   dsde_part2 class                   *
// ********************************************************

class dsde_part2 : public model_internal {
public:
  dsde_part2(const type* place);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dsde_part2::dsde_part2(const type* place)
: model_internal(em->VOID, "partition", 2)
{
  DCASSERT(place);
  typelist* t = new typelist(2);
  t->SetItem(0, place->getSetOfThis());
  t->SetItem(1, em->INT);
  SetFormal(1, t, "vset:pnum");
  SetRepeat(1);
  SetDocumentation("Groups state variables into submodels.  All state variables in the set {vset} will be added to group <pnum>.  pnum must be positive, to prevent conflicts with the other forms of this function.");
  // TBD: say if a higher number means higher MDD level, or lower.
}

void dsde_part2::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dsde_def* mdl = smart_cast<dsde_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result pnum;
  result pset;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    x.aggregate = 1;
    x.answer = &pnum;
    SafeCompute(pass[i], x);
    if (!pnum.isNormal() || pnum.getInt() <= 0) {
      if (mdl->StartError(pass[i])) {
        em->cerr() << "Bad group number: ";
        em->INT->print(em->cerr(), pnum, 0);
        em->cerr() << ", ignoring";
        mdl->DoneError();
      }
      continue;
    }

    x.aggregate = 0;
    x.answer = &pset;
    SafeCompute(pass[i], x);
    DCASSERT(pset.isNormal());
    shared_set* ps = smart_cast <shared_set*> (pset.getPtr());
    mdl->SetLevelOfStateVars(pass[i], pnum.getInt(), ps);
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                   dsde_part3 class                   *
// ********************************************************

class dsde_part3 : public model_internal {
public:
  dsde_part3(const type* place);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dsde_part3::dsde_part3(const type* place)
: model_internal(em->VOID, "partition", 2)
{
  DCASSERT(place);
  typelist* t = new typelist(2);
  t->SetItem(0, place->getSetOfThis());
  t->SetItem(1, place);
  SetFormal(1, t, "vset:v");
  SetRepeat(1);
  SetDocumentation("Groups state variables into submodels.  All state variables in the set {vset} will be added to the current group of state variable p.");
}

void dsde_part3::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dsde_def* mdl = smart_cast<dsde_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result pref;
  result pset;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    x.aggregate = 1;
    x.answer = &pref;
    SafeCompute(pass[i], x);
    model_statevar* pl = smart_cast <model_statevar*> (pref.getPtr());
    DCASSERT(pl);  
    if (!mdl->isVariableOurs(pl, pass[i], "ignoring partition assignment")) 
      continue;
    
    if (0==pl->GetPart()) {
      if (mdl->StartError(pass[i])) {
        em->cerr() << "Place " << pl->Name();
        em->cerr() << " is not yet assigned to any group";
        mdl->DoneError();
      }
      continue;
    }

    x.aggregate = 0;
    x.answer = &pset;
    SafeCompute(pass[i], x);
    DCASSERT(pset.isNormal());
    shared_set* ps = smart_cast <shared_set*> (pset.getPtr());
    mdl->SetLevelOfStateVars(pass[i], pl->GetPart(), ps);
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                 dsde_priolevel class                 *
// ********************************************************

class dsde_priolevel : public model_internal {
public:
  dsde_priolevel(const type* trans);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dsde_priolevel::dsde_priolevel(const type* trans)
: model_internal(em->VOID, "priority", 2)
{
  DCASSERT(trans);
  typelist* t = new typelist(2);
  t->SetItem(0, trans->getSetOfThis());
  t->SetItem(1, em->INT);
  SetFormal(1, t, "tset:plev");
  SetRepeat(1);
  SetDocumentation("Define the priority of every event in set tset to the level given by plev.  Larger integers specify higher priorities.  In a model with priority levels, if two events are enabled concurrently, if one has lower priority level then it cannot fire.  Events with no priority level specified are given a default level of 0.");
}

void dsde_priolevel::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dsde_def* mdl = smart_cast<dsde_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result plev;
  result tset;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    x.aggregate = 1;
    x.answer = &plev;
    SafeCompute(pass[i], x);
    if (!plev.isNormal() || plev.getInt() <= 0) {
      if (mdl->StartError(pass[i])) {
        em->cerr() << "Bad priority level: ";
        em->INT->print(em->cerr(), plev, 0);
        em->cerr() << ", ignoring";
        mdl->DoneError();
      }
      continue;
    }

    x.aggregate = 0;
    x.answer = &tset;
    SafeCompute(pass[i], x);
    DCASSERT(tset.isNormal());
    shared_set* ts = smart_cast <shared_set*> (tset.getPtr());
    mdl->SetPriorityLevel(pass[i], plev.getInt(), ts);
  }
  x.answer = answer;
  x.aggregate = 0;
}

// ********************************************************
// *                 dsde_priolist  class                 *
// ********************************************************

class dsde_priolist : public model_internal {
public:
  dsde_priolist(const type* trans);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

dsde_priolist::dsde_priolist(const type* trans)
: model_internal(em->VOID, "priority", 2)
{
  DCASSERT(trans);
  typelist* t = new typelist(2);
  t->SetItem(0, trans->getSetOfThis());
  t->SetItem(1, trans->getSetOfThis());
  SetFormal(1, t, "dset:eset");
  SetRepeat(1);
  SetDocumentation("Specify that every event in set dset has priority over every event in set eset.  These events must have the same priority level (otherwise this information is either redudant, or causes a dependency cycle).  Multiple calls to this function could create a dependency cycle; these are detected when the model is instantiated.");
}

void dsde_priolist::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  dsde_def* mdl = smart_cast<dsde_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result highset;
  result lowset;
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    x.aggregate = 0;
    x.answer = &highset;
    SafeCompute(pass[i], x);
    DCASSERT(highset.isNormal());
    shared_set* hs = smart_cast <shared_set*> (highset.getPtr());

    x.aggregate = 1;
    x.answer = &lowset;
    SafeCompute(pass[i], x);
    DCASSERT(lowset.isNormal());
    shared_set* ls = smart_cast <shared_set*> (lowset.getPtr());

    mdl->SetPriorityOver(pass[i], hs, ls);
  }
  x.answer = answer;
  x.aggregate = 0;
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_dsde : public initializer {
  public:
    init_dsde();
    virtual bool execute();
};
init_dsde the_dsde_initializer;

init_dsde::init_dsde() : initializer("init_dsde")
{
  usesResource("em");
}

bool init_dsde::execute()
{
  if (0==em) return false;

  option* warning = em->findOption("Warning");
  dsde_def::dup_part.Initialize(warning,
    "dup_part",
    "For multiple partition definitions for a state variable",
    true
  );
  dsde_def::no_part.Initialize(warning,
    "no_part",
    "If some, but not all, state variables are assiged to groups using partition",
    true
  );
  dsde_def::dup_prio.Initialize(warning,
    "dup_prio",
    "For multiple priority level definitions for a model event",
    true
  );
  dsde_hlm::ignored_prio.Initialize(warning,
    "ignored_prio",
    "For ignored priority pairs (between events in different priority levels)",
    true
  );
  
  return true;
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void Add_DSDE_varfuncs(const type* svt, symbol_table* syms)
{
  syms->AddSymbol(  new dsde_part1(svt) );
  syms->AddSymbol(  new dsde_part2(svt) );
  syms->AddSymbol(  new dsde_part3(svt) );
}

void Add_DSDE_eventfuncs(const type* evt, symbol_table* syms)
{
  syms->AddSymbol(  new dsde_priolevel(evt) );
  syms->AddSymbol(  new dsde_priolist(evt)  );
}

