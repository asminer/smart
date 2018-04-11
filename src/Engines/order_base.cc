
#include "order_base.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../Formlsms/dsde_hlm.h"

#include "../Options/options.h"

#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <queue>
#include <iostream>
#include <limits>
#include <random>
#include <cmath>
#include <stdint.h>

// **************************************************************************
// *                                                                        *
// *                        static_varorder  methods                        *
// *                                                                        *
// **************************************************************************

named_msg static_varorder::report;
named_msg static_varorder::debug;

static_varorder::static_varorder()
: subengine()
{
}

static_varorder::~static_varorder()
{
}

// **************************************************************************
// *                                                                        *
// *                          user_varorder  class                          *
// *                                                                        *
// **************************************************************************

class user_varorder : public static_varorder {
public:
  user_varorder();
  virtual ~user_varorder();
  
  virtual bool AppliesToModelType(hldsm::model_type mt) const;
  virtual void RunEngine(hldsm* m, result &);
};
user_varorder the_user_varorder;

// **************************************************************************
// *                         user_varorder  methods                         *
// **************************************************************************

user_varorder::user_varorder()
{
}

user_varorder::~user_varorder()
{
}

bool user_varorder::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Asynch_Events == mt);
}

void user_varorder::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  
  if (hm->hasPartInfo()) return;  // already done!
  
  dsde_hlm* dm = dynamic_cast <dsde_hlm*> (hm);
  DCASSERT(dm);
  
  if (debug.startReport()) {
    debug.report() << "using user-defined variable order\n";
    debug.stopIO();
  }
  dm->useDefaultVarOrder();
}

// **************************************************************************
// *                                                                        *
// *                      heuristic_varorder class                          *
// *                                                                        *
// **************************************************************************

class heuristic_varorder : public user_varorder {
public:
  heuristic_varorder(int heuristic, double factor);
  virtual ~heuristic_varorder();
  virtual void RunEngine(hldsm* m, result &);
  static double alphaParameter;
private:
  double factor;
  int heuristic;
  
};
heuristic_varorder the_force_000_varorder(0, 0.0);
heuristic_varorder the_force_025_varorder(0, 0.25);
heuristic_varorder the_force_050_varorder(0, 0.5);
heuristic_varorder the_force_0625_varorder(0, 0.625);
heuristic_varorder the_force_075_varorder(0, 0.75);
heuristic_varorder the_force_0875_varorder(0, 0.875);
heuristic_varorder the_force_100_varorder(0, 1.0);
heuristic_varorder the_noack_varorder(1, 0);
heuristic_varorder the_force_param(0, 0.0);
double heuristic_varorder::alphaParameter = -1.0;
// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_static_varorder : public initializer {
public:
  init_static_varorder();
  virtual bool execute();
};
init_static_varorder the_static_varorder_initializer;

init_static_varorder::init_static_varorder() : initializer("init_static_varorder")
{
  usesResource("em");
  buildsResource("varorders");
  buildsResource("engtypes");
}

bool init_static_varorder::execute()
{
  if (0==em)  return false;
  
  // Initialize options
  option* report = em->findOption("Report");
  option* debug = em->findOption("Debug");
  
  static_varorder::report.Initialize(report,
                                     "varorder",
                                     "When set, static variable ordering heuristic performance is reported.",
                                     false
                                     );
  
  static_varorder::debug.Initialize(debug,
                                    "varorder",
                                    "When set, static variable ordering heuristic details are displayed.",
                                    false
                                    );
  
  MakeEngineType(em,
                 "VariableOrdering",
                 "Algorithm to use to determine the (static) variable order for a high-level model",
                 engtype::Model
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "USER_DEFINED",
                 "Variable order is determined by calls to partition() in the model",
                 &the_user_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE000",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_000_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE025",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_025_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE050",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_050_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE0625",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_0625_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE075",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_075_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE0875",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_0875_varorder
                 );
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "FORCE100",
                 "Variable order is determined by calls to partition() in the model",
                 &the_force_100_varorder
                 );
  
  
  // BEGIN EXPERIMENTAL DOUBLE RETRIEVAL TEST CODE HERE:
  
  engine* variable_engine = RegisterEngine(em,
                                           "VariableOrdering",
                                           "FORCEPARAM",
                                           "Variable order is determined by calls to partition() in the model",
                                           &the_force_param
                                           );
  //
  variable_engine->AddOption(
                             MakeRealOption(
                                            "SosSotAlpha",
                                            "Weight given to sum of spans vs. sum of tops.",
                                            heuristic_varorder::alphaParameter,
                                            true, true, 0.0,
                                            true, true, 1.0
                                            )
                             );
  //settings->DoneAddingOptions();
  
  
  // END EXPERIMENTAL DOUBLE RETRIEVAL TEST CODE HERE:
  
  
  RegisterEngine(em,
                 "VariableOrdering",
                 "NOACK",
                 "Variable order is determined by calls to partition() in the model",
                 &the_noack_varorder
                 );
  
  return true;
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                  Static Variable Ordering                      *
// *                                                                *
// *                                                                *
// ******************************************************************


typedef uint_fast64_t u64;

struct DoubleInt {
  double theDouble;
  int theInt;
};

struct OrderPair {
  int name;
  int item;
};

struct ArcPair {
  int placeToTrans;
  int transToPlace;
};

struct ARC {
  int source;
  int target;
  int cardinality;
};

struct MODEL {
  int numPlaces;
  int numTrans;
  int numArcs;
  int * placeInits;
  ARC * theArcs;
};

struct TransTree {
  std::map<int, ArcPair> arcs;
};

u64 cogFix(MODEL & theModel, std::vector<int>& theOrder);
double getSpanTopParam(MODEL & theModel, std::vector<int> & theOrder, double param);

// sort by
bool comparePair(DoubleInt a, DoubleInt b) {
  return (a.theDouble < b.theDouble);
}

// sort by
bool comparePairName(OrderPair a, OrderPair b) {
  return (a.name < b.name);
}

// sort by
bool comparePairItem(OrderPair a, OrderPair b) {
  return (a.item < b.item);
}

// sort by values
bool compareArcPair(ArcPair a, ArcPair b) {
  if (a.placeToTrans == b.placeToTrans) {
    return (a.transToPlace < b.transToPlace);
  }
  return (a.placeToTrans < b.placeToTrans);
}

// sort by (SOUPS uniqueness technique)
bool compareTree(const TransTree  & a, const TransTree  & b) {
  std::map<int, ArcPair>::const_iterator itA = a.arcs.begin();
  int aBottom = itA->first;
  std::map<int, ArcPair>::const_iterator itB = b.arcs.begin();
  int bBottom = itB->first;
  if (aBottom != bBottom) return (aBottom < bBottom); // bottoms are different
  
  std::set<int> levSet; // sorted set of levels with arcs
  
  for (; itA!=a.arcs.end(); ++itA) {
    levSet.emplace(itA->first);
  }
  for (; itB!=b.arcs.end(); ++itB) {
    levSet.emplace(itB->first);
  }
  
  for (auto it = levSet.begin(); it !=levSet.end(); ++it) {
    itA = a.arcs.find(*it);
    if (itA == a.arcs.end()) {
      // no arc at this level, point of difference
      return true;  // arc pair a < arc pair b
    }
    itB = b.arcs.find(*it);
    if (itB == b.arcs.end()) {
      // no arc. point of difference
      return false;
    }
    
    ArcPair apA = itA->second;
    ArcPair apB = itB->second;
    if (apA.placeToTrans != apB.placeToTrans) {
      return (apA.placeToTrans < apB.placeToTrans);
    }
    if (apA.transToPlace != apB.transToPlace) {
      return (apA.transToPlace < apB.transToPlace);
    }
    // must be equal arc pairs at this point
  }
  // must be equal transitions at this point
  return false;
}

// check if a set contains an item
// not thread safe
bool setContains(std::set<int> theSet, int element) {
  std::set<int>::iterator isUsed = theSet.find(element);
  return (isUsed != theSet.end());
} 


// check if a map contains a given key
// not thread safe
bool mapContainsKey(std::map<int, double> theMap, int key) {
  std::map<int, double>::iterator isUsed = theMap.find(key);
  return (isUsed != theMap.end());
} 

void outputTree(TransTree & a) {
  for (auto it = a.arcs.begin(); it != a.arcs.end(); ++it) {
    ArcPair ap = it->second;
    std::cout << "outputTree has ap value " << ap.placeToTrans << "," << ap.transToPlace << 
    " at it->first " << it->first << std::endl;
  }
}


int pointOfUnique(TransTree & a, TransTree & b) {
  if (a.arcs.empty()) {
    // a was empty
    std::map<int, ArcPair>::iterator itB = b.arcs.begin();
    return itB->first;
  }
  
  std::map<int, ArcPair>::iterator itA = a.arcs.begin();
  int aBottom = itA->first;
  std::map<int, ArcPair>::iterator itB = b.arcs.begin();
  int bBottom = itB->first;
  if (aBottom != bBottom) return (bBottom); // bottoms are different
  
  std::set<int> levSet; // sorted set of levels with arcs
  
  for (; itA!=a.arcs.end(); ++itA) {
    levSet.emplace(itA->first);
  }
  for (; itB!=b.arcs.end(); ++itB) {
    levSet.emplace(itB->first);
  }
  
  int current = aBottom;
  
  do {
    auto itA = a.arcs.lower_bound(current);
    auto itB = b.arcs.lower_bound(current);
    int otherIndex = itA->first;
    int thisIndex = itB->first;
    if (thisIndex < otherIndex) {
      return current;
    }
    if (thisIndex > otherIndex) {
      return otherIndex;
    }
    ArcPair apA = itA->second;
    ArcPair apB = itB->second;
    if ((apA.placeToTrans != apB.placeToTrans) || (apA.transToPlace != apB.transToPlace)) {
      return current;
    }  
    itA = a.arcs.upper_bound(current);
    itB = b.arcs.upper_bound(current);
    current = std::min(itA->first, itB->first);
  } while (true) ;
  
  return 0; // should never get here
}

int getTop(TransTree & tt) {
  std::map<int, ArcPair>::reverse_iterator rit = tt.arcs.rbegin();
  return rit->first;
}

int getProdSpan(TransTree & tt) {
  int top = getTop(tt);
  for (auto it = tt.arcs.begin(); it != tt.arcs.end(); ++it) {
    ArcPair ap = it->second;
    if (ap.placeToTrans != ap.transToPlace) {
      return ((top - it->first) + 1); // it has a productive span
    } 
  }
  return 0;
}

// returns the binary tree data structure for the current model
// assuming that the startOrder is a mapping (place->level) IMPORTANT! BS
std::vector<TransTree> getTrees(MODEL & theModel, std::vector<int> & startOrder) {
  std::map<int, ArcPair>::iterator found;
  
  std::vector<int> plOrder (startOrder);
  for (int index = 0; index < theModel.numPlaces; index++) {
    plOrder[startOrder[index]] = index;// try this
  }
  
  
  // create the empty trees for each transition
  std::vector<TransTree> result;
  for (int t = 0; t < theModel.numTrans; t++) {
    TransTree temp;
    result.emplace_back(temp);
  }
  // for each arc, add it to the appropriate transition's tree
  for (int a = 0; a < theModel.numArcs; a++) {
    int source = theModel.theArcs[a].source;
    int target = theModel.theArcs[a].target;
    int cardinality = theModel.theArcs[a].cardinality;
    if (source < target) {
      // source is the place
      TransTree & current = result[target - theModel.numPlaces];
      found = current.arcs.find(plOrder[source]);
      if (found != current.arcs.end()) {
        // arc pair already there
        ArcPair ap = found->second;
        ap.placeToTrans = cardinality;
        current.arcs[plOrder[source]] = ap;
      } else {
        ArcPair ap;
        ap.placeToTrans = cardinality;
        ap.transToPlace = 0;
        current.arcs[plOrder[source]] = ap;
      }
    } else {
      // target is the place
      TransTree & current2 = result[source - theModel.numPlaces];
      found = current2.arcs.find(plOrder[target]);
      if (found != current2.arcs.end()) {
        // arc pair already there
        ArcPair ap = found->second;
        ap.transToPlace = cardinality;
        current2.arcs[plOrder[target]] = ap;
      } else {
        ArcPair ap;
        ap.transToPlace = cardinality;
        ap.placeToTrans = 0;
        current2.arcs[plOrder[target]] = ap;
      }
    }
  }
  return result;
}


DoubleInt getSOUPS(std::vector<TransTree> & theTrees, int abandon) {
  // sort the trans trees
  std::sort(theTrees.begin(), theTrees.end(), compareTree);
  
  // use the sorted order to find lowest point of uniqueness
  TransTree prev; // an empty tree, to force the first transition to be all unique
  
  int soups = 0;
  int countTrans = 0;
  for (auto it = theTrees.begin(); it != theTrees.end(); ++it) {
    TransTree curr = *it;
    int pou = pointOfUnique(prev, curr);
    int us = (getTop(curr) - pou) + 1;
    int prod = getProdSpan(curr);
    int top = getTop(curr);
    if (prod < us) {
      soups += prod;
    } else {
      soups += us;
    }
    prev = curr;
    if (soups > abandon) break;
    countTrans++;
    
  }
  DoubleInt result;
  result.theDouble = 1.0 - ((double)countTrans) / ((double)theTrees.size());
  result.theInt = soups;
  return result;
}

// calculate the SOUPS heuristic for a given ordering
double getSOUPSScore(MODEL & theModel, std::vector<int> & theOrder, double param) {
  // param is ignored, but kept to have the same signature as the spantop calc
  std::vector<TransTree> trees = getTrees(theModel, theOrder);
  
  int score = 2147483647;
  DoubleInt result = getSOUPS(trees, score);
  
  double toDouble = (double)result.theInt;
  return toDouble;
}



void doSwap(std::vector<TransTree> & trees, int a, int b) {
  if (a == b) return;
  std::map<int, ArcPair>::iterator found;
  for (int it = 0; it < trees.size(); it++) {  
    TransTree & tt = trees[it];
    found = tt.arcs.find(a);
    ArcPair apA;
    ArcPair apB;
    if (found != tt.arcs.end()) {
      apA = tt.arcs[a];
      tt.arcs.erase(a);
      found = tt.arcs.find(b);
      if (found != tt.arcs.end()) {
        apB = tt.arcs[b];
        tt.arcs[a] = apB;
      }
      tt.arcs[b] = apA;
    } else {
      found = tt.arcs.find(b);
      if (found != tt.arcs.end()) {
        apB = tt.arcs[b];
        tt.arcs.erase(b);
        tt.arcs[a] = apB;
      } else {
        // neither found, ignore
      }
    }
  }
}


// generate a good SOUPS order using simulated annealing
std::vector<int> generateSASOUPSOrder(MODEL theModel, int numIter, std::vector<int> startOrder) {
  std::vector<int> current (startOrder);  // gets changed
  std::vector<int> resultOrder (startOrder);
  std::default_random_engine generator;
  std::uniform_int_distribution<int> dist(0, theModel.numPlaces - 1);
  std::uniform_real_distribution<double> arand(0.0, 1.0);
  
  std::vector<TransTree> trees = getTrees(theModel, startOrder);
  
  int a = 0;  // starting swaps (no swap in first iteration)
  int b = 0;
  
  int score = 2147483647; // "best" score so far
  int prevScore = score;  // score from previous iteration
  for (int iter = 0; iter < numIter; iter++) {
    // get the current soups score
    DoubleInt SOUPS = getSOUPS(trees, prevScore); // current order and trees must always match
    if (SOUPS.theInt < score) {
      // this order is the best so far, so keep it
      //std::cout << "SIMULATED ANNEALING FOUND NEW BEST ORDER AT ITER " << iter << " WITH SCORE: " << SOUPS.theInt << std::endl;
      score = SOUPS.theInt;
      prevScore = SOUPS.theInt;
      for (int index = 0; index < theModel.numPlaces; index++) {
        resultOrder[index] = current[index];
      }
    } else {
      double temperature = (double)(iter) / (double)(numIter);
      double decide = arand(generator);
      double ex = exp(-(SOUPS.theDouble) * temperature);
      if (decide < ex) {
        // reset the trees to order prior to current swap
        int temp = current[a];
        current[a] = current[b];
        current[b] = temp;
        doSwap(trees, a, b);
      } 
      if (arand(generator) > .98) {
        trees = getTrees(theModel, resultOrder);
        for (int index = 0; index < theModel.numPlaces; index++) {
          current[index] = resultOrder[index];
        }
      }
    }
    prevScore = SOUPS.theInt;
    prevScore = std::max(prevScore, score);
    // perform a new random swap
    do {
      a = dist(generator);
      b = dist(generator);
    } while (a == b);
    int temp = current[a];
    current[a] = current[b];
    current[b] = temp;
    doSwap(trees, a, b);
  }
  
  return resultOrder;
}



// used by Noack
std::map<int, std::set<int> > getDotP(MODEL theModel) {
  std::map<int, std::set<int> > result;
  
  for (int p = 0; p < theModel.numPlaces; p++) {
    std::set<int> temp;
    result[p] = temp;
  }
  
  for (int a = 0; a < theModel.numArcs; a++) {
    if (theModel.theArcs[a].source < theModel.numPlaces) {
      // source is a place
    } else {
      // target is the place
      result[theModel.theArcs[a].target].insert(theModel.theArcs[a].source);
    }
  }
  
  return result;
}


// used by Noack
std::map<int, std::set<int> > getPDot(MODEL theModel) {
  std::map<int, std::set<int> > result;
  
  for (int p = 0; p < theModel.numPlaces; p++) {
    std::set<int> temp;
    result[p] = temp;
  }
  
  for (int a = 0; a < theModel.numArcs; a++) {
    if (theModel.theArcs[a].source < theModel.numPlaces) {
      // source is a place
      result[theModel.theArcs[a].source].insert(theModel.theArcs[a].target);
    } else {
      // target is the place
    }
  }
  
  return result;
}


// used by Noack
std::vector<int> getpUp(std::map<int, std::set<int> > dotp,
                        std::map<int, std::set<int> > pdot, MODEL theModel) {
  std::vector<int> result;
  
  for (int p = 0; p < theModel.numPlaces; p++) {
    std::set<int> temp (dotp[p]);
    std::set<int>& pp = pdot[p];
    temp.insert(pp.begin(), pp.end());
    //for (auto pp : pdot[p]) { temp.insert(pp); }
    result.push_back(temp.size());
  }
  
  return result;
}


// used by Noack
std::map<int, std::set<int> > getDotT(MODEL theModel) {
  std::map<int, std::set<int> > result;
  
  for (int p = 0; p < theModel.numTrans; p++) {
    std::set<int> temp;
    result[p + theModel.numPlaces] = temp;
  }
  
  for (int a = 0; a < theModel.numArcs; a++) {
    if (theModel.theArcs[a].source < theModel.numPlaces) {
      // source is a place
      result[theModel.theArcs[a].target].insert(theModel.theArcs[a].source);
    } else {
      // target is the place
    }
  }
  
  return result;
}


// used by Noack
std::map<int, std::set<int> > getTDot(MODEL theModel) {
  std::map<int, std::set<int> > result;
  
  for (int p = 0; p < theModel.numTrans; p++) {
    std::set<int> temp;
    result[p + theModel.numPlaces] = temp;
  }
  
  for (int a = 0; a < theModel.numArcs; a++) {
    if (theModel.theArcs[a].source < theModel.numPlaces) {
      // source is a place
    } else {
      // target is the place
      result[theModel.theArcs[a].source].insert(theModel.theArcs[a].target);
    }
  }
  
  return result;
}

// generate an order using the Noack method with given parameters
// OrderPair format in case this ordering is used as input to force|other algorithms
std::vector<OrderPair> noackParam(MODEL theModel, double param) {
  std::vector<OrderPair> resultOrder;
  const double PARAM_W = param;
  // get the model information organized
  std::map<int, std::set<int> > dotPs = getDotP(theModel);
  std::map<int, std::set<int> > pDots = getPDot(theModel);
  std::map<int, std::set<int> > dotTs = getDotT(theModel);
  std::map<int, std::set<int> > tDots = getTDot(theModel);
  std::vector<int> pUp = getpUp(dotPs, pDots, theModel);
  
  double bestStart = 0.0;
  int start = 0;
  std::set<int> unused;
  for (int p = 0; p < theModel.numPlaces; p++) {
    double current = 0.0;
    unused.insert(p);
    //for (auto t : dotPs[p]) current += dotTs[t].size();
    for (unsigned t = dotPs[p].size()-1; t >= 0; t--)
      current += dotTs[t].size();
    
    //for (auto t : pDots[p]) current += PARAM_W * (tDots[t].size());
    for (unsigned t = pDots[p].size()-1; t >= 0; t--)
      current += PARAM_W * (tDots[t].size());
    
    if (pUp[p] > 0) {
      current /= (double)pUp[p];
      if (current > bestStart) {
        start = p;
        bestStart = current;
      }
    }
  }
  
  std::map<int, std::map<int, double> > adders;
  //for (auto p : unused) {
  for (std::set<int>::iterator p = unused.begin(); p != unused.end(); p++) {
    std::map<int, double> totals;
    //for (auto t : dotPs[p]) {
    for (std::set<int>::iterator t = dotPs[*p].begin(); t != dotPs[*p].end(); t++) {
      double fraction = 1.0 / dotTs[*t].size();
      // for (auto pp : dotTs[t]) {
      for (std::set<int>::iterator pp = dotTs[*t].begin(); pp != dotTs[*t].end(); pp++) {
        if (!mapContainsKey(totals, *pp)) {
          totals[*pp] = fraction;
        } else {
          totals[*pp] += fraction;
        }
      }
    }
    // for (auto t : pDots[p]) {
    for (std::set<int>::iterator t = pDots[*p].begin(); t != pDots[*p].end(); t++) {
      double fraction = PARAM_W / dotTs[*t].size();
      // for (auto pp : tDots[t]) {
      for (std::set<int>::iterator pp = tDots[*t].begin(); pp != tDots[*t].end(); pp++) {
        if (!mapContainsKey(totals, *pp)) {
          totals[*pp] = fraction;
        } else {
          totals[*pp] += fraction;
        }
      }
    }
    adders[*p] = totals;
  }
  
  double * currentWeights = new double[theModel.numPlaces];
  int current = start;
  int count = 0;
  OrderPair op;
  op.name = count;
  op.item = start;
  resultOrder.push_back(op);
  count++;
  
  do {
    // keep track of best weight so far
    double maxWeight = -1.0;
    int best = -1;
    unused.erase(current);
    //for (auto p : unused) {
    for (std::set<int>::iterator p = unused.begin(); p!= unused.end(); p++) {
      double toAdd = 0.0;
      // auto check = adders.find(p);
      std::map<int, std::map<int, double> >::iterator check = adders.find(*p);
      if (check != adders.end()) {
        // auto check2 = adders[p].find(current);
        std::map<int, double>::iterator check2 = adders[*p].find(current);
        if (check2 != adders[*p].end()) toAdd = adders[*p][current] / pUp[*p];
      }
      currentWeights[*p] += toAdd;
      if (currentWeights[*p] > maxWeight) {
        best = *p;
        maxWeight = currentWeights[*p];
      }
    }
    current = best;
    OrderPair theNext;
    theNext.name = count;
    theNext.item = best;
    count++;
    resultOrder.push_back(theNext);
  } while (unused.size() > 1);
  
  delete [] currentWeights;
  return resultOrder;
}




// generate an order using the Noack method with default parameters
// OrderPair format in case this ordering is used as input to force|other algorithms
std::vector<OrderPair> noack(MODEL theModel) {
  printf("In %s\n", __func__);
  return noackParam(theModel, 2.0);
}



// generate an int std::vector order from a pair-style
std::vector<int> pairToInt(std::vector<OrderPair> paired) {
  std::vector<int> result (paired.size(), 0);
  //for (auto a : paired) {
  for (std::vector<OrderPair>::iterator a = paired.begin();
       a != paired.end(); a++) {
    result[a->name] = a->item;
  }
  return result;
}


// generate an int std::vector order from a pair-style
std::vector<OrderPair> intToPair(std::vector<int> asInt) {
  OrderPair fill;
  std::vector<OrderPair> result (asInt.size(), fill);
  for (int index = 0; index < int(asInt.size()); index++) {
    OrderPair temp;
    temp.name = index;
    temp.item = asInt[index];
    result[index] = temp;
  }
  return result;
}

// generate an order using the parameterized FORCE method
// param should be between 0.0 and 1.0 inclusive
std::vector<int> generateForceOrder(MODEL theModel, int numIter, std::vector<int> startOrder, double param) {
  std::vector<int> current (startOrder);  // gets changed
  std::vector<int> resultOrder (startOrder);
  const int CYCLE = 5;
  double bestScore = std::numeric_limits<double>::max();
  
  u64 * cycleCheck = new u64[CYCLE];
  for (int index = 0; index < CYCLE; index++) {
    cycleCheck[index] = 0;
  }
  
  int iter = 0; // in case # iterations wanted after a cycle breaks
  for (iter = 0; iter < numIter; iter++) {
    u64 theNew = cogFix(theModel, current);
    int hash = theNew % CYCLE;
    if (theNew == cycleCheck[hash]) break;
    cycleCheck[hash] = theNew;
    
    double score = getSpanTopParam(theModel, current, param);
    if (score < bestScore) {
      bestScore = score;
      for (int index = 0; index < theModel.numPlaces; index++) {
        resultOrder[index] = current[index];
      }
    }
  }
  
  delete [] cycleCheck;
  
  return resultOrder;
}

std::vector<int> searchOrderAlpha(MODEL theModel, int numIter, std::vector<int> startOrder, double param) {
  std::vector<int> current (startOrder);  // gets changed
  std::vector<int> resultOrder (startOrder);
  /*
   std::vector<int> inverse (theModel.numPlaces, 0);
   for (int index = 0; index < theModel.numPlaces; index++) {
   inverse[current[index]] = index;
   }
   
   double bestScore = getSpanTopParam(theModel, current, param); // find the score to beat
   
   for (int index = 0; index < (theModel.numPlaces - 1); index++) {
   for (int swap = index + 1; swap < theModel.numPlaces; swap++) {
   int tempA = inverse[index];
   int tempB = inverse[swap];
   
   // swap in current
   current[tempA] = swap;
   current[tempB] = index;
   
   // measure
   double score = getSpanTopParam(theModel, current, param);
   if (score < bestScore) {
   bestScore = score;
   for (int index = 0; index < theModel.numPlaces; index++) {
   resultOrder[index] = current[index];
   }
   // "fix" the swap (make it permanent)
   int temp = inverse[index];
   inverse[index] = inverse[swap];
   inverse[swap] = temp;
   } else {
   // undo the swap
   current[tempA] = index;
   current[tempB] = swap;
   }
   }
   }
   // output score?
   */
  
  return resultOrder;
}


std::vector<int> swapPositionAlpha(MODEL theModel, int numIter, std::vector<int> startOrder, double param) {
  std::vector<int> current (startOrder);  // gets changed
  std::vector<int> resultOrder (startOrder);
  
  double currentBest = getSpanTopParam(theModel, current, param); // find the score to beat
  
  int changes = 0;
  int iters = 0;
  do {
    changes = 0;
    
    for (int index = 0; index < (theModel.numPlaces - 1); index++) {
      for (int swap = index + 1; swap < theModel.numPlaces; swap++) {
        
        // swap in current
        int temp = current[index];
        current[index] = current[swap];
        current[swap] = temp;
        
        // measure, keep if better
        double score = getSpanTopParam(theModel, current, param);
        if (score < currentBest) {
          currentBest = score;
          changes++;
          for (int i = 0; i < theModel.numPlaces; i++) {
            resultOrder[i] = current[i];
          }
        } 
        // undo the swap
        temp = current[index];
        current[index] = current[swap];
        current[swap] = temp;
      }
    }
    
    
    for (int i = 0; i < theModel.numPlaces; i++) {
      current[i] = resultOrder[i];
    }
    iters++;
  } while ((changes != 0) && (iters < numIter));
  
  return resultOrder;
}



// generate a breadth first order from the given model, starting with variable "start"
std::vector<int> getBFSx(MODEL theModel, int start) {
  
  std::vector<int> result;
  
  std::set<int> used;
  std::set<int>::iterator isUsed;
  
  std::map<int, std::set<int> > connections;
  std::map<int, std::set<int> >::iterator found;
  
  for (int a = 0; a < theModel.numArcs; a++) {
    int source = theModel.theArcs[a].source;
    int target = theModel.theArcs[a].target;
    found = connections.find(source);
    if (found == connections.end()) {
      // not there yet
      std::set<int> temp;
      temp.insert(target);
      connections[source] = temp;
    } else {
      // set already present
      connections[source].insert(target);
    }
  }
  
  for (int p = 0; p < theModel.numPlaces; p++) {
    int s = ((p + start) % theModel.numPlaces);
    std::queue<int> theQueue;
    isUsed = used.find(s);
    if (isUsed == used.end()) {
      result.push_back(s);
      used.insert(s);
      theQueue.push(s);
      while (!theQueue.empty()) {
        int c = theQueue.front();
        theQueue.pop();
        found = connections.find(c);
        if (found != connections.end()) {
          std::set<int> cc = connections[c];
          for (std::set<int>::iterator it = cc.begin(); it != cc.end(); ++it) {
            std::map<int, std::set<int> >::iterator foundI = connections.find(*it);
            if (foundI != connections.end()) {
              std::set<int> ct = connections[*it];
              for (std::set<int>::iterator itt = ct.begin(); itt != ct.end(); ++itt) {
                isUsed = used.find(*itt);
                int n = *itt;
                if (isUsed == used.end()) {
                  result.push_back(n);
                  theQueue.push(n);
                  used.insert(n);
                }
              }
            }
          }
        }
      }
    }
  }
  
  std::vector<int> inv_result(result.size());
  for (int i = 0; i < int(result.size()); i++) {
    inv_result[result[i]] = i;
  }
  
  return inv_result;
}


// generate a breadth-first order from the given model
std::vector<int> getBFS(MODEL theModel) {
  return getBFSx(theModel, 0);
}


// return a vector of variable indices, sorted by how well connected
std::vector<int> leastToMostConnected(MODEL theModel) {
  std::vector<DoubleInt> counts;
  for (int index = 0; index < theModel.numPlaces; index++) {
    DoubleInt temp;
    temp.theInt = index;
    temp.theDouble = 0.0;
    counts.push_back(temp);
  }
  for (int a = 0; a < theModel.numArcs; a++) {
    int source = theModel.theArcs[a].source;
    int target = theModel.theArcs[a].target;
    if (source < target) {
      counts[source].theDouble += 1.0;
    } else {
      counts[target].theDouble += 1.0;
    }
  }
  std::stable_sort(counts.begin(), counts.end(), comparePair);
  std::vector<int> result (theModel.numPlaces, 0);
  for (int index = 0; index < theModel.numPlaces; index++) {
    result[index] = counts[index].theInt;
    //std::cout << "LTM " << index << " is " << result[index] << " with " << counts[index].theDouble << std::endl;
  }
  return result;
}


// default parameters ordering
std::vector<int> defaultOrder(MODEL theModel, double paramAlpha, int maxIter, int bfsIters,
                              named_msg& out){
  // paramAlpha: 1.0 is all spans, 0.0 is all tops, recommend >= 100 for iter
  int soupsIters = 10000;
  
  std::vector<int> best (theModel.numPlaces, 0);
  // start with best as the given order until found otherwise
  for (int index = 0; index < theModel.numPlaces; index++) {
    best[index] = index;  
  }
  
  if (out.startReport()) {
    out.report() << "In default order : " << paramAlpha << "\n\n";
    out.stopIO();
  }
  
  double bestScore = getSOUPSScore(theModel, best, paramAlpha);// to test soups
  
  if (out.startReport()) {
    out.report() << "Score[given-order]: " << bestScore << "\n\n";
    out.stopIO();
  }
  
  std::vector<int> startOrder (best); // try force on the given order
  std::vector<int> forceGiven = generateForceOrder(theModel, maxIter, startOrder, paramAlpha);
  std::vector<int> soupsGiven = generateSASOUPSOrder(theModel, soupsIters, forceGiven);
  
  double newScore = getSOUPSScore(theModel, soupsGiven, paramAlpha);
  if (newScore < bestScore) {
    bestScore = newScore;
    for (int index = 0; index < theModel.numPlaces; index++) {
      best[index] = soupsGiven[index];
    }
  }
  
  if (out.startReport()) {
    out.report() << "Score[SA(force(given-order))]: " << newScore << "\n";
    out.report() << "Best Score: " << bestScore << "\n\n";
    out.stopIO();
  }
  
  forceGiven = getBFS(theModel);
  soupsGiven = generateSASOUPSOrder(theModel, soupsIters, forceGiven);
  newScore = getSOUPSScore(theModel, soupsGiven, paramAlpha);// to test soups
  if (newScore < bestScore) {
    bestScore = newScore;
    for (int index = 0; index < theModel.numPlaces; index++) {
      best[index] = soupsGiven[index];
    }
  }
  
  if (out.startReport()) {
    out.report() << "Score[SA(bfs)]: " << newScore << "\n";
    out.report() << "Best Score: " << bestScore << "\n\n";
    out.stopIO();
  }
  
  int size = theModel.numPlaces - 1;
  
  // try force on a number of different BFS orders (bfsIters should be as many as wanted to get good results) 10??
  for (int iter = 0; (iter < 1) && (iter < theModel.numPlaces); iter++) {
    std::vector<int> startOrderBFS = getBFSx(theModel, iter);
    startOrderBFS = generateSASOUPSOrder(theModel, soupsIters, startOrderBFS);
    newScore = getSOUPSScore(theModel, startOrderBFS, paramAlpha);// to test soups
    if (newScore < bestScore) {
      bestScore = newScore;
      for (int index = 0; index < theModel.numPlaces; index++) {
        best[index] = startOrderBFS[index];
      }
    }
    if (out.startReport()) {
      out.report() << "ITER " << iter << " Score[SA(bfs(" << iter << "))]: " << newScore << "\n";
      out.report() << "Best Score: " << bestScore << "\n\n";
      out.stopIO();
    }
    
    
    
    std::vector<int> forceBFS = generateForceOrder(theModel, maxIter, startOrderBFS, paramAlpha);
    soupsGiven = generateSASOUPSOrder(theModel, soupsIters, forceBFS);
    newScore = getSOUPSScore(theModel, soupsGiven, paramAlpha);
    if (newScore < bestScore) {
      bestScore = newScore;
      for (int index = 0; index < theModel.numPlaces; index++) {
        best[index] = soupsGiven[index];
      }
    }
    if (out.startReport()) {
      out.report() << "Score[SA(force(bfs(" << iter << ")))]: " << newScore << "\n";
      out.report() << "Best Score: " << bestScore << "\n\n";
      out.stopIO();
    }
    
    // reverse the order
    for (int fixrev = 0; fixrev < theModel.numPlaces; fixrev++) {
      startOrderBFS[fixrev] = (theModel.numPlaces - 1) - startOrderBFS[fixrev];
    }
    soupsGiven = generateSASOUPSOrder(theModel, soupsIters, startOrderBFS);
    newScore = getSOUPSScore(theModel, soupsGiven, paramAlpha);// to test soups
    if (newScore < bestScore) {
      bestScore = newScore;
      for (int index = 0; index < theModel.numPlaces; index++) {
        best[index] = soupsGiven[index];
      }
    }
    if (out.startReport()) {
      out.report() << "Rev Score[SA(bfs(" << iter << "))]: " << newScore << "\n";
      out.report() << "Best Score: " << bestScore << "\n\n";
      out.stopIO();
    }
    
    forceBFS = generateForceOrder(theModel, maxIter, startOrderBFS, paramAlpha);
    soupsGiven = generateSASOUPSOrder(theModel, soupsIters, forceBFS);
    newScore = getSOUPSScore(theModel, soupsGiven, paramAlpha);// to test soups
    if (newScore < bestScore) {
      bestScore = newScore;
      for (int index = 0; index < theModel.numPlaces; index++) {
        best[index] = soupsGiven[index];
      }
    }
    if (out.startReport()) {
      out.report() << "Rev Score[SA(force(bfs(" << iter << ")))]: " << newScore << "\n";
      out.report() << "Best Score: " << bestScore << "\n\n";
      out.stopIO();
    }
  }
  
  return best;
}




// output an orderpair as an EDN formatted map
void outputEDN_orderMap(std::vector<OrderPair> theOrder) {
  std::cout << "{";
  // for (OrderPair op : theOrder) {
  for (std::vector<OrderPair>::iterator op = theOrder.begin(); op != theOrder.end(); op++) {
    std::cout << op->name << " " << op->item << ", ";
  }
  std::cout << "}" << std::endl;
}

// output an order std::vector as an EDN formatted std::vector
void outputEDN_orderVec(std::vector<int> theOrder) {
  std::cout << "[";
  //for (auto a : theOrder) {
  for (std::vector<int>::iterator a = theOrder.begin(); a != theOrder.end(); a++) {
    std::cout << *a << " ";
  }
  std::cout << "]" << std::endl;
}


// calculate the center of gravity
u64 cogFix(MODEL & theModel, std::vector<int>& theOrder) {
  // find the center of gravity for every variable in the model
  int * counts = new int[theModel.numTrans + theModel.numPlaces];
  DoubleInt * totals = new DoubleInt[theModel.numTrans + theModel.numPlaces];
  for (int index = 0; index < (theModel.numTrans + theModel.numPlaces); index++) {
    if (index < theModel.numPlaces) {
      counts[index] = 1;//0;
      totals[index].theDouble = 0.0;//(double)theModel.numPlaces;//0.0;
    } else {
      counts[index] = 0;
      totals[index].theDouble = 0.0;
    }
    totals[index].theInt = index; // "names" 0..numPlaces - 1 
  } 
  
  // for every arc, update the cog value
  for (int index = 0; index < theModel.numArcs; index++) {
    int source = theModel.theArcs[index].source;
    int target = theModel.theArcs[index].target;
    if (source < target) {
      // source is a place
      int currentPlace = theOrder[source];
      counts[target]++;
      totals[target].theDouble += (double)currentPlace;
    } else {
      // source is the transition
      int currentPlace = theOrder[target];
      counts[source]++;
      totals[source].theDouble += (double)currentPlace;
    }
  }
  // the cog value is the average (mean)
  for (int index = 0; index < theModel.numArcs; index++) {
    int source = theModel.theArcs[index].source;
    int target = theModel.theArcs[index].target;
    if (source < target) {
      counts[source] += counts[target];
      totals[source].theDouble += totals[target].theDouble;
    } else {
      counts[target] += counts[source];
      totals[target].theDouble += totals[source].theDouble;
    }
  }
  for (int index = 0; index < theModel.numPlaces; index++) {
    if (counts[index] != 0) {
      totals[index].theDouble /= (double)counts[index];
    } else {
      totals[index].theDouble = (double)theModel.numPlaces;
    }
    totals[index].theInt = index;
  } 
  
  // sort by the cog values
  std::vector<DoubleInt> toBeSorted;
  toBeSorted.assign(totals, totals + theModel.numPlaces);
  std::stable_sort(toBeSorted.begin(), toBeSorted.end(), comparePair);
  
  // update the order, and calc a quick hash (FNV1a - modified)
  u64 base = 109951168211ULL;//theOrder.size();
  u64 result = 14695981039346656037ULL;
  for (int index = 0; index < int(theOrder.size()); index++) {
    //theOrder[index] = toBeSorted[index].theInt;
    theOrder[toBeSorted[index].theInt] = index;
    result ^= toBeSorted[index].theInt;
    result *= base;
  }
  
  delete [] totals;
  delete [] counts;
  
  return result;
}


// calculate the combined span|top heuristic for a given ordering
double getSpanTopParam(MODEL & theModel, std::vector<int> & theOrder, double param) {
  {
  int count = theModel.numTrans + theModel.numPlaces;
  int * eventMax = new int[count];
  int * eventMin = new int[count];
  for (int index = 0; index < (count); index++) {
    eventMax[index] = 0;
    eventMin[index] = theModel.numPlaces;
  } 
  u64 tops = 0LL;
  for (int index = 0; index < theModel.numArcs; index++) {
    int source = theModel.theArcs[index].source;
    int target = theModel.theArcs[index].target;
    if (source < target) {
      int currentPlace = theOrder[source];
      if (eventMax[target] < currentPlace) {
        tops += currentPlace - eventMax[target];
        eventMax[target] = currentPlace;
      }
      if (eventMin[target] > currentPlace) {
        eventMin[target] = currentPlace;
      }
    } else {
      int currentPlace = theOrder[target];
      if (eventMax[source] < currentPlace) {
        tops += currentPlace - eventMax[source];
        eventMax[source] = currentPlace;
      }
      if (eventMin[source] > currentPlace) {
        eventMin[source] = currentPlace;
      }
    }
  }
  u64 spans = 0LL;
  for (int index = theModel.numPlaces; index < count; index++) {
    spans += eventMax[index] - eventMin[index] + 1;
  }
  delete [] eventMax;
  delete [] eventMin;
  double result = param * (double)spans + (1.0 - param) * (double)tops;
  return result;
  }
}


// calculate the combined span|top heuristic for a given ordering
double getSpanTopParam(MODEL & theModel, std::vector<OrderPair> & theOrder,
                       double param) {
  int count = theModel.numTrans + theModel.numPlaces;
  int * eventMax = new int[count];
  int * eventMin = new int[count];
  for (int index = 0; index < (count); index++) {
    eventMax[index] = 0;
    eventMin[index] = theModel.numPlaces;
  }  
  u64 tops = 0LL;
  for (int index = 0; index < theModel.numArcs; index++) {
    int source = theModel.theArcs[index].source;
    int target = theModel.theArcs[index].target;
    if (source < target) {
      int currentPlace = theOrder[source].item;
      if (eventMax[target] < currentPlace) {
        tops += currentPlace - eventMax[target];
        eventMax[target] = currentPlace;
      }
      if (eventMin[target] > currentPlace) {
        eventMin[target] = currentPlace;
      }
    } else {
      int currentPlace = theOrder[target].item;
      if (eventMax[source] < currentPlace) {
        tops += currentPlace - eventMax[source];
        eventMax[source] = currentPlace;
      }
      if (eventMin[source] > currentPlace) {
        eventMin[source] = currentPlace;
      }
    }
  }
  u64 spans = 0LL;
  for (int index = theModel.numPlaces; index < count; index++) {
    spans += eventMax[index] - eventMin[index] + 1;
  }
  delete [] eventMax;
  delete [] eventMin;
  double result = param * (double)spans + (1.0 - param) * (double)tops;
  return result;
}

// calculate the saturation cost heuristic for a given ordering
// experimental, may overflow double for large enough models
// TODO
double getSatCostParam(MODEL & theModel, std::vector<int> & theOrder, double param) {
  {
  int count = theModel.numTrans + theModel.numPlaces;
  std::vector<int> eventMax(count, 0);
  std::vector<int> eventMin(count, theModel.numPlaces);
  
  // get the sums and tops for all transitions
  for (int index = 0; index < theModel.numArcs; index++) {
    int source = theModel.theArcs[index].source;
    int target = theModel.theArcs[index].target;
    if (source < target) {
      int currentPlace = theOrder[source];
      if (eventMax[target] < currentPlace) {
        eventMax[target] = currentPlace;
      }
      if (eventMin[target] > currentPlace) {
        eventMin[target] = currentPlace;
      }
    } else {
      int currentPlace = theOrder[target];
      if (eventMax[source] < currentPlace) {
        eventMax[source] = currentPlace;
      }
      if (eventMin[source] > currentPlace) {
        eventMin[source] = currentPlace;
      }
    }
  }
  // calculate union costs (alpha fixed to 1 until further testing)
  std::vector<double> unionCosts(theModel.numPlaces + 1, 1.0);
  for (int index = 0; index < theModel.numPlaces + 1; index++) {
    unionCosts[index] = index + 1; // level ^ alpha after verified
  }
  std::vector<double> satCosts(theModel.numPlaces + 1, 0.0);
  std::vector<double> sumSatCosts(theModel.numPlaces + 1, 1.0);
  std::vector<double> fireCosts(count, 0);
  for (int level = 0; level < theModel.numPlaces + 1; level++) {
    double currentCost = 0.0;
    for (int event = theModel.numPlaces; level < count; level++) {
      if (eventMax[event] == level) {
        
      }
    }
  }
  
  
  double result = 0;
  return result;
  }
}





// given a model, return an order using the given parameters
// heuristics: {0: FORCE, 1: NOACK}
std::vector<int> primaryOrder(MODEL theModel, int heuristic, double paramAlpha) {
  int maxIter = 10000;
  
  if (heuristic == 0) {
    std::vector<int> startOrder = getBFS(theModel);
    
    return generateForceOrder(theModel, maxIter, startOrder, paramAlpha);
  }
  return pairToInt(noack(theModel));  // noack with default params
}

//search for an arc in the vector of arcs
//return its enabling
/*
 int findArcInVector(std::vector<ARC> vecArcs, int from, int to )
 {
 for(std::vector<ARC>::iterator it = vecArcs.begin() ; it != vecArcs.end(); ++it)
 {
 if((it->source==from)&&(it->target==to))
 return it->cardinality;
 }
 
 return 0;
 }
 
 // for model translation
 std::vector<int> findSelfLoopItems(std::vector<int> enable_var,std::vector<int> fire_var)
 {
 std::vector<int> self_loop_var;
 for(std::vector<int>::iterator e_it = enable_var.begin() ; e_it != enable_var.end(); ++e_it)
 {
 bool found=false;
 for(std::vector<int>::iterator f_it = fire_var.begin() ; f_it != fire_var.end(); ++f_it)
 {
 if(*e_it==*f_it)
 found = true;
 }
 if(found==false)
 {
 self_loop_var.push_back(*e_it);
 }
 }
 
 return self_loop_var;
 }
 */


MODEL translateModel(dsde_hlm& smartModel) {
  MODEL result;
  result.numPlaces = smartModel.getNumStateVars();
  result.numTrans = smartModel.getNumEvents();
  std::vector<ARC> vecArcs;
  std::map<long, ARC> pairs;
  long ptBase = result.numPlaces + result.numTrans;
  
  for (int index = 0; index < result.numTrans; index++) {
    model_event * theEvent = smartModel.getEvent(index);
    int arcTarget = index + result.numPlaces;
    // using target as the transition
    //Create a list of variables that enable this transition as per smart paradigm (i.e Enabling is non-zero)
    std::vector<int> enabling_var;
    
    expr* enabling = theEvent->getEnabling();
    
    if (0 != enabling){
      List <expr> E;
      enabling->BuildExprList(traverse_data::GetProducts, 0, &E);
      for (int j=0; j<E.Length(); j++) { //E.Length() is the no. of places affected
        expr* exp = E.Item(j);
        clev_op* clev_exp = dynamic_cast<clev_op*>(exp);
        if (0!=clev_exp){
          List <symbol> S;
          clev_exp->BuildSymbolList(traverse_data::GetSymbols, 0, &S);
          DCASSERT(S.Length() == 1);
          model_statevar* v = dynamic_cast <model_statevar*> (S.Item(0));
          if (0!=v){
            enabling_var.push_back(v->GetIndex());
            long enab_con = clev_exp->getLower(); // lower is the lower bound of the enabling condition
            if (enab_con!=0) {  
              ARC tempArc;
              tempArc.target = v->GetIndex();
              tempArc.source = arcTarget;
              tempArc.cardinality = enab_con;
              //std::cout << "ENABLING ARC SOURCE " << tempArc.source << " TARGET " << tempArc.target << " CARD " << tempArc.cardinality << std::endl;
              vecArcs.push_back(tempArc);
              long ptIndex = ((long)tempArc.target) + (((long)tempArc.source) * ptBase);
              pairs[ptIndex] = tempArc;// hold onto the arcs 
            }
          } 
        }
      } 
    }
  }
  
  for (int index = 0; index < result.numTrans; index++) {
    model_event * theEvent = smartModel.getEvent(index);
    int arcTarget = index + result.numPlaces;
    expr* enabling = theEvent->getNextstate();
    
    //Create a list of variables that are affected by this transition as per smart paradigm (i.e delta is non-zero)
    if (0 != enabling) {
      List <expr> E;
      enabling->BuildExprList(traverse_data::GetProducts, 0, &E);
      for (int j=0; j<E.Length(); j++) { //E.Length() is the no. of places affected
        expr* exp = E.Item(j);
        cupdate_op* cupdate_exp = dynamic_cast<cupdate_op*>(exp);
        if (0!=cupdate_exp) {
          List <symbol> S;
          cupdate_exp->BuildSymbolList(traverse_data::GetSymbols, 0, &S);
          DCASSERT(S.Length() == 1);
          model_statevar* v = dynamic_cast <model_statevar*> (S.Item(0));
          if (0!=v) {
            ARC tempArc;
            tempArc.target = arcTarget;
            tempArc.source = v->GetIndex();
            long delta = cupdate_exp->getDelta(); // delta = firing_cardinality - enabling_cardinality
            
            long ptIndex = ((long)tempArc.source) + (((long)tempArc.target) * ptBase);
            long enable_tok = 0LL;
            auto hasEnabling = pairs.find(ptIndex);
            if (hasEnabling != pairs.end()) {
              // there is an enabling arc
              enable_tok = pairs[ptIndex].cardinality;
              pairs.erase(ptIndex);// remove it so we know what gets left in (non-productive self-loops)
            } 
            long fire_tok = delta + enable_tok;
            tempArc.cardinality = fire_tok;
            if (fire_tok != 0) {
              vecArcs.push_back(tempArc); // only store the arc if is has cardinality != 0
            } 
          }  
        }
      }
    }
  } 
  
  // self loops
  for (auto it = pairs.begin(); it != pairs.end(); ++it) {
    ARC sl = it->second;
    ARC tempArc;
    tempArc.source = sl.target;
    tempArc.target = sl.source;
    tempArc.cardinality = sl.cardinality;
    vecArcs.push_back(tempArc); // add this self-looper
  }
  
  result.numArcs = vecArcs.size();
  result.theArcs = new ARC[vecArcs.size()];
  for (unsigned index = 0; index < vecArcs.size(); index++) {
    result.theArcs[index] = vecArcs[index];
  }
  
  return result;
}


MODEL translateModelOLD(dsde_hlm& smartModel) {
  MODEL result;
  result.numPlaces = smartModel.getNumStateVars();
  result.numTrans = smartModel.getNumEvents();
  
  std::vector<ARC> vecArcs;
  
  for (int index = 0; index < result.numTrans; index++) {
    model_event * theEvent = smartModel.getEvent(index);
    int arcTarget = index + result.numPlaces;  // using target as the transition
    
    expr* enabling = theEvent->getEnabling();
    
    if (0 != enabling) {
      // Using the magic "List" from SMART with O(1) random access?
      List <symbol> L;
      enabling->BuildSymbolList(traverse_data::GetSymbols, 0, &L);
      for (int i = 0; i < L.Length(); i++) {
        symbol* s = L.Item(i);
        model_statevar* mv = dynamic_cast <model_statevar*> (s);
        if (0 != mv) {
          ARC tempArc;
          tempArc.target = arcTarget;
          tempArc.source = mv->GetIndex(); 
          tempArc.cardinality = 1;  // temporary incorrect workaround BS
          vecArcs.push_back(tempArc);
        } 
      }
    }
    
    enabling = theEvent->getNextstate();
    
    if (0 != enabling) {
      // Using the magic "List" from SMART with O(1) random access?
      List <symbol> L;
      enabling->BuildSymbolList(traverse_data::GetSymbols, 0, &L);
      for (int i = 0; i < L.Length(); i++) {
        symbol* s = L.Item(i);
        model_statevar* mv = dynamic_cast <model_statevar*> (s);
        if (0 != mv) {
          ARC tempArc;
          tempArc.target = mv->GetIndex();
          tempArc.source = arcTarget; 
          tempArc.cardinality = 1;  // temporary incorrect workaround BS
          vecArcs.push_back(tempArc);
        } 
      }
    }
  }
  
  result.numArcs = vecArcs.size();
  result.theArcs = new ARC[vecArcs.size()];
  for (unsigned index = 0; index < vecArcs.size(); index++) {
    result.theArcs[index] = vecArcs[index];
    
    printf("p: %d, t: %d\n", vecArcs[index].source, vecArcs[index].target);
  }
  
  // printf("In translateModel()\n");
  
  return result;
}


// **************************************************************************
// *                    heuristic_varorder  methods                         *
// **************************************************************************

heuristic_varorder::heuristic_varorder(int heuristic, double factor)
: heuristic(heuristic), factor(factor)
{
}

heuristic_varorder::~heuristic_varorder()
{
}

void heuristic_varorder::RunEngine(hldsm* hm, result &)
{
  DCASSERT(hm);
  DCASSERT(AppliesToModelType(hm->Type()));
  
  if (hm->hasPartInfo()) return;  // already done!
  
  dsde_hlm* dm = dynamic_cast <dsde_hlm*> (hm);
  DCASSERT(dm);
  
  if (debug.startReport()) {
    debug.report() << "using "
    << (heuristic == 0? "force": "noack")
    << "-defined variable order\n";
    
    debug.report() << "Given Order: \n";
    for (int i = 0; i < dm->getNumStateVars(); i++) {
      model_statevar* var = dm->getStateVar(i);
      debug.report() << "var: " << i << (i < 10? "  ": " ") << "part: " << var->GetPart() << (var->GetPart() < 10? "  ": " ") << var->Name() << "\n";
    }
    debug.stopIO();
  }
  
  MODEL model = translateModel(*dm);
  const int nIterations = 1000;
  const int nStartingOrders = 10;
  std::vector<int> order = defaultOrder(model, ((alphaParameter >= 0.0) ? alphaParameter : factor), nIterations, nStartingOrders, debug);
  
  
  
  
  //std::vector<int> order = defaultOrder(model, factor, nIterations, nStartingOrders, debug);
  //std::vector<int> order = defaultOrder(model, ((heuristic_varorder::alphaParameter >= 0.0) ? heuristic_varorder::alphaParameter : factor), nIterations, nStartingOrders, debug);
  
  for (int j = 0; j < int(order.size()); j++) {
    dm->getStateVar(j)->SetPart(order[j]+1);
  }
  dm->useHeuristicVarOrder();
  
  if (debug.startReport()) {
    debug.report() << "Generated Order: \n";
    for (int i = 0; i < dm->getNumStateVars(); i++) {
      model_statevar* var = dm->getStateVar(i);
      debug.report() << "var: " << i << (i < 10? "  ": " ") << "part: " << var->GetPart() << (var->GetPart() < 10? "  ": " ") << var->Name() << "\n";
    }
    debug.stopIO();
  }
}


