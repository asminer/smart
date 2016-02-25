
// $Id$

#include "evm_form.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/formalism.h"

#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/dd_front.h"
#include "dsde_hlm.h"

// **************************************************************************
// *                                                                        *
// *                            tam_border class                            *
// *                                                                        *
// **************************************************************************

class tam_border : public symbol {
  long which;
public:
  tam_border(const type* t, char* n, long v);
  virtual void Compute(traverse_data &x);
};

tam_border::tam_border(const type* t, char* n, long v)
 : symbol(0, -1, t, n)
{
  which = v;
}

void tam_border::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  x.answer->setInt(which);
}

// **************************************************************************
// *                                                                        *
// *                             tam_glue class                             *
// *                                                                        *
// **************************************************************************

class tam_glue : public model_var {
  bool strength_defined;
  long strength;
public:
  tam_glue(const symbol* w, const model_instance* p);

  inline bool hasStrengthDefined() const { return strength_defined; }
  inline long getStrength() const { 
    DCASSERT(strength_defined);
    return strength;
  }
  inline void setStrength(long s) {
    strength = s;
    strength_defined = true;
  }
};

tam_glue::tam_glue(const symbol* w, const model_instance* p) : model_var(w, p)
{
  setStateType(No_State);
  strength_defined = false;
  strength = 0;
}

inline const char* SafeName(const tam_glue* g)
{
  if (g) return g->Name();
  return "";
}

// **************************************************************************
// *                                                                        *
// *                             tam_tile class                             *
// *                                                                        *
// **************************************************************************

class tam_tile : public model_var {
  const tam_glue* north;
  const tam_glue* east;
  const tam_glue* south;
  const tam_glue* west;
  long index;
public:
  tam_tile(const symbol* w, const model_instance* p);

  inline long getIndex() const { return index; }
  inline void setIndex(long i) { index = i;    }

  inline void setNorth(const tam_glue* n) { north = n;  }
  inline void setEast(const tam_glue* e)  { east = e;   }
  inline void setSouth(const tam_glue* s) { south = s;  }
  inline void setWest(const tam_glue* w)  { west = w;   }

  inline const tam_glue* getNorth() const  { return north; }
  inline const tam_glue* getEast() const   { return east;  }
  inline const tam_glue* getSouth() const  { return south; }
  inline const tam_glue* getWest() const   { return west;  }

  inline long getNorthStrength() const {
    return north ? north->getStrength() : 0;
  }
  inline long getEastStrength() const {
    return east ? east->getStrength() : 0;
  }
  inline long getSouthStrength() const {
    return south ? south->getStrength() : 0;
  }
  inline long getWestStrength() const {
    return west ? west->getStrength() : 0;
  }

  inline long getBinding( const tam_tile* n, const tam_tile* e, 
                          const tam_tile* s, const tam_tile* w) 
  {
    long x = 0;
    if (n && n->getSouth()  == north) x += getNorthStrength();
    if (e && e->getWest()   == east ) x += getEastStrength();
    if (s && s->getNorth()  == south) x += getSouthStrength();
    if (w && w->getEast()   == west ) x += getWestStrength();
    return x;
  }

  void Export(OutputStream &s) const;
};

tam_tile::tam_tile(const symbol* w, const model_instance* p) : model_var(w, p)
{
  setStateType(No_State);
  north = east = south = west = 0;
  index = 0;
}

void tam_tile::Export(OutputStream &s) const
{
  s << "TILENAME " << Name() << "\n";
  s << "LABEL\n";
  s << "NORTHBIND " << getNorthStrength() << "\n";
  s << "EASTBIND " << getEastStrength() << "\n";
  s << "SOUTHBIND " << getSouthStrength() << "\n";
  s << "WESTBIND " << getWestStrength() << "\n";
  s << "NORTHLABEL " << SafeName(north) << "\n";
  s << "EASTLABEL " << SafeName(east) << "\n";
  s << "SOUTHLABEL " << SafeName(south) << "\n";
  s << "WESTLABEL " << SafeName(west) << "\n";
  // no TILECOLOR yet
  s << "CREATE\n\n";
}

// **************************************************************************
// *                                                                        *
// *                            tam_square class                            *
// *                                                                        *
// **************************************************************************

/** The board is a collection of these.
*/
class tam_square : public model_statevar {
  const tam_tile* init;
  long prio;
  bool has_prio;
public:
  tam_square(const char* fn, int ln, const type* t, char* n, const model_instance* p);
  inline void setInit(const tam_tile* i) {
    init = i;
  }
  inline bool hasInit() const {
    return init;
  }
  inline const tam_tile* getInit() const { 
    return init;
  }
  inline bool hasPrio() const {
    return has_prio;
  }
  inline long getPrio() const {
    return prio;
  }
  inline void setPrio(int p) {
    prio = p;
    has_prio = true;
  }
  const tam_tile* getCurrentTile(shared_state* st) const;
};

// **************************************************************************
// *                                                                        *
// *                            tam_canput class                            *
// *                                                                        *
// **************************************************************************

/** Function call used for event enabling.
*/
class tam_canput : public custom_internal {
public:
  tam_canput();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

tam_canput the_tam_canput;

// ******************************************************************
// *                       tam_canput methods                       *
// ******************************************************************

tam_canput::tam_canput()
 : custom_internal("canput", "proc bool canput(t, q, n, e, s, w)")
{
  SetDocumentation("Determine if tile t can be placed in square q, given neighbors n, e, s, w.");
}

void tam_canput::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(6==np);
  tam_tile* t = smart_cast <tam_tile*>(pass[0]);
  DCASSERT(t);

  tam_square* q = smart_cast <tam_square*>(pass[1]);
  DCASSERT(q);
  
  // if q already has a tile, then definitely no
  if (q->getCurrentTile(x.current_state)) {
    x.answer->setBool(false);
    return;
  }

  tam_square* n = smart_cast <tam_square*>(pass[2]);
  tam_square* e = smart_cast <tam_square*>(pass[3]);
  tam_square* s = smart_cast <tam_square*>(pass[4]);
  tam_square* w = smart_cast <tam_square*>(pass[5]);

  DCASSERT(x.current_state);
  const tam_tile* nt = n ? n->getCurrentTile(x.current_state) : 0;
  const tam_tile* et = e ? e->getCurrentTile(x.current_state) : 0;
  const tam_tile* st = s ? s->getCurrentTile(x.current_state) : 0;
  const tam_tile* wt = w ? w->getCurrentTile(x.current_state) : 0;

  long bond = t->getBinding(nt, et, st, wt);

  DCASSERT(x.answer);
  x.answer->setBool(bond>1);
}

int tam_canput::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:  
        x.the_type = em->BOOL->addProc();
        return 0;

    case traverse_data::Typecheck:
        if (np<6)  return NotEnoughParams(np);
        if (np>6)  return TooManyParams(np);
        if (0==dynamic_cast<tam_tile*>(pass[0]))  return BadParam(0, np);
        for (int i=1; i<6; i++) {
          if (0==pass[i]) {
            if (1==i) return BadParam(i, np);
            continue;
          }
          if (0==dynamic_cast<tam_square*>(pass[i])) return BadParam(i, np);
        }
        return 0;

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

// **************************************************************************
// *                                                                        *
// *                             tam_hlm  class                             *
// *                                                                        *
// **************************************************************************

class tam_hlm : public dsde_hlm {
// style for printing state here?

  tam_tile** tileset;
  int num_tiles;

  tam_glue** glueset;
  int num_glues;

  // for state printing
  int x_width;

public:
  tam_hlm(const model_instance* s, model_statevar** V, int nv, model_event** E, int ne);

  virtual ~tam_hlm();

  inline void set_x_width(int xw) { x_width = xw; }

  // required for hldsm:
  virtual void showState(OutputStream &s, const shared_state* x) const;
  inline void showTile(OutputStream &s, bool un, int t) const {
    if (un) {
      s.Put('?');
    } else if (0==t) {
      s.Put('_');
    } else {
      DCASSERT(tileset[t]);
      s.Put(tileset[t]->Name());
    }
  }

  inline const tam_tile* getTile(int t) const {
    DCASSERT(tileset);
    CHECK_RANGE(0, t, num_tiles);
    return tileset[t];
  }

  // required for dsde_hlm:
  virtual int NumInitialStates() const;
  virtual double GetInitialState(int n, shared_state* s) const;

  // glueset defintion
  void setGlues(named_msg &warn, tam_glue** gs, int ng);
  // tileset defintion
  void setTiles(named_msg &warn, tam_tile** ts, int nt);

  // Super handy
  bool Export(OutputStream &s) const;
};

// ******************************************************************
// *                                                                *
// *                       tam_square methods                       *
// *                                                                *
// ******************************************************************

tam_square::tam_square(const char* fn, int ln, const type* t, char* n, 
 const model_instance* p) : model_statevar(fn, ln, t, n, p, 0)
{
  init = 0;
  has_prio = false;
  prio = 0;
}

const tam_tile* tam_square::getCurrentTile(shared_state* st) const 
{
  DCASSERT(parent);
  const hldsm* hm = parent->GetCompiledModel();
  DCASSERT(hm);
  const tam_hlm* p = smart_cast <const tam_hlm*>(hm);
  DCASSERT(p);
  DCASSERT(!st->unknown(GetIndex()));
  return p->getTile(st->get(GetIndex()));
}

// ******************************************************************
// *                                                                *
// *                        tam_hlm  methods                        *
// *                                                                *
// ******************************************************************

tam_hlm::tam_hlm(const model_instance* s, model_statevar** V, int nv, model_event** E, int ne)
 : dsde_hlm(s, V, nv, E, ne)
{
  tileset = 0;
  num_tiles = 0;
  glueset = 0;
  num_glues = 0;
}

tam_hlm::~tam_hlm()
{
  for (int i=0; i<num_tiles; i++) Delete(tileset[i]);
  delete[] tileset;
  for (int i=0; i<num_glues; i++) Delete(glueset[i]);
  delete[] glueset;
}

void tam_hlm::showState(OutputStream &s, const shared_state* st) const
{
  DCASSERT(st);

  bool printed = false;
  int i;
  s << '[';
  for (i=0; i<num_vars; i++) {
    if (printed) {
      if (i%x_width) s << ", "; else s << ",\n\t";
    } else {
      printed = true;
    }
    bool un = st->unknown(state_data[i]->GetIndex());
    int t = un ? 1 : st->get(state_data[i]->GetIndex());
    showTile(s, un, t);
  } // for i
  s << ']';
}

int tam_hlm::NumInitialStates() const
{
  return 1;
}

double tam_hlm::GetInitialState(int n, shared_state* st) const
{
  DCASSERT(0==n);
  DCASSERT(st);
  DCASSERT(state_data);
  for (int i=0; i<num_vars; i++) {
    tam_square* sq = smart_cast <tam_square*> (state_data[i]);
    DCASSERT(sq);
    const tam_tile* init = sq->getInit();
    st->set(sq->GetIndex(), init ? init->getIndex() : 0);
  }
  return 1.0;
}

void tam_hlm::setGlues(named_msg &warn, tam_glue** gs, int ng) 
{
  DCASSERT(0==glueset);
  glueset = gs;
  num_glues = ng;

  if (!warn.isActive()) return;

  // check that glues have strength defined
  bool all_ok = true;
  for (int i=0; i<num_glues; i++) {
    if (0==glueset[i]) continue;
    if (glueset[i]->hasStrengthDefined()) continue;
    all_ok = false;
    break;
  }
  if (all_ok) return;
  StartWarning(warn, 0);
  em->warn() << "These glue types have no strength defined, using default of 0:";
  em->newLine(1);
  bool printed = false;
  for (int i=0; i<num_glues; i++) {
    if (0==glueset[i]) continue;
    if (glueset[i]->hasStrengthDefined()) continue;
    if (printed) em->warn() << ", "; else printed = true;
    em->warn() << glueset[i]->Name();
  }
  em->changeIndent(-1);
  DoneWarning();
}


void tam_hlm::setTiles(named_msg &warn, tam_tile** ts, int nt) 
{
  DCASSERT(0==tileset);
  tileset = ts;
  num_tiles = nt;
  for (int i=0; i<num_tiles; i++) if (tileset[i]) tileset[i]->setIndex(i);

  if (!warn.isActive()) return;

  // check that tiles have been defined
  bool all_ok = true;
  for (int i=0; i<num_tiles; i++) {
    if (0==tileset[i]) continue;
    if (tileset[i]->getNorth()) continue;
    if (tileset[i]->getEast()) continue;
    if (tileset[i]->getSouth()) continue;
    if (tileset[i]->getWest()) continue;
    all_ok = false;
    break;
  }
  if (all_ok) return;
  StartWarning(warn, 0);
  em->warn() << "These tiles have no glue on any border:";
  em->newLine(1);
  bool printed = false;
  for (int i=0; i<num_tiles; i++) {
    if (0==tileset[i]) continue;
    if (tileset[i]->getNorth()) continue;
    if (tileset[i]->getEast()) continue;
    if (tileset[i]->getSouth()) continue;
    if (tileset[i]->getWest()) continue;
    if (printed) em->warn() << ", "; else printed = true;
    em->warn() << tileset[i]->Name();
  }
  em->changeIndent(-1);
  DoneWarning();
}

bool tam_hlm::Export(OutputStream &s) const
{
  for (int i=0; i<num_tiles; i++) if (tileset[i]) tileset[i]->Export(s);
  return true;
}

// **************************************************************************
// *                                                                        *
// *                             tam_def  class                             *
// *                                                                        *
// **************************************************************************

class tam_def : public model_def {
  symbol* tiles;
  int num_tiles;
  symbol* glues;
  int num_glues;

  // board dimensions
  bool board_is_set;
  long x_low;
  long x_high;
  long y_low;
  long y_high;

  // board squares and such
  tam_square **board;
  const char* b_fn;
  int b_ln;

  // "globals"

  static const type* tile_type;
  static const type* glue_type;
  static const type* border_type;

  static named_msg tam_debug;
  static named_msg dup_tiledef;
  static named_msg dup_gluedef;
  static named_msg dup_init;
  static named_msg dup_prio;
  static named_msg dup_board;
  static named_msg no_tiledef;
  static named_msg no_gluedef;
  static named_msg no_init;
  static named_msg no_board;
  static named_msg empty_board;
  static named_msg empty_tileset;

  static const int NORTH = 1;
  static const int EAST = 2;
  static const int SOUTH = 3;
  static const int WEST = 4;
  inline static const char* nameOf(int b) {
    switch (b) {
      case NORTH:   return "north";
      case EAST:    return "east";
      case SOUTH:   return "south";
      case WEST:    return "west";
      default:      DCASSERT(0);  return 0;
    };
  }

  friend class init_tamform;

public:
  tam_def(const char* fn, int line, const type* t, char* n, 
          formal_param** pl, int np);
  virtual ~tam_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  // Construction methods:

  void setStrength(const expr* cause, tam_glue* g, const result &s);

  void setBorder(const expr* cause, int b, tam_tile* t, tam_glue* g);

  void setBoardSize(const expr* cause, long xl, long xh, long yl, long yh);

  void setInit(const expr* cause, long r, long c, tam_tile* t);

  void setPriority(const expr* cause, long r, long c, long p);

protected:
  virtual void InitModel();
  virtual void FinalizeModel(OutputStream &ds);

  template <class SYMB>
  static inline SYMB** list2array(SYMB* front, int size) {
    SYMB** myset = new SYMB*[size+1]; // element 0 is null
    for (int i=size; i>=0; i--) {
      myset[i] = front;
      if (front) front = smart_cast<SYMB*>(front->Next());
      if (myset[i]) myset[i]->LinkTo(0);
    }
    return myset;
  }

  inline long pos2index(long x, long y) const {
    CHECK_RANGE(x_low, x, x_high+1);
    CHECK_RANGE(y_low, y, y_high+1);
    return (y-y_low)*(x_high+1-x_low) + x-x_low;
  }

  inline const tam_square* readSq(long x, long y) const {
    DCASSERT(board_is_set);
    DCASSERT(board);
    return board[pos2index(x, y)];
  }

  inline tam_square* getSq(long x, long y) {
    DCASSERT(board_is_set);
    DCASSERT(board);
    return board[pos2index(x, y)];
  }

  inline tam_square* makeSq(int x, int y) {
    DCASSERT(board_is_set);
    DCASSERT(board);
    char buffer[50];
    snprintf(buffer, 50, "sq(%d,%d)", x, y);
    char* n = strdup(buffer);
    return (
      board[pos2index(x, y)] = new tam_square(b_fn, b_ln, tile_type, n, current)
    );
  }
};

const type* tam_def::tile_type;
const type* tam_def::glue_type;
const type* tam_def::border_type;

named_msg tam_def::tam_debug;
named_msg tam_def::dup_tiledef;
named_msg tam_def::dup_gluedef;
named_msg tam_def::dup_init;
named_msg tam_def::dup_prio;
named_msg tam_def::dup_board;
named_msg tam_def::no_tiledef;
named_msg tam_def::no_gluedef;
named_msg tam_def::no_init;
named_msg tam_def::no_board;
named_msg tam_def::empty_board;
named_msg tam_def::empty_tileset;


// ******************************************************************
// *                        tam_def  methods                        *
// ******************************************************************

tam_def::tam_def(const char* fn, int line, const type* t, char*n, 
    formal_param **pl, int np) : model_def(fn, line, t, n, pl, np)
{
  tiles = glues = 0;
  num_tiles = num_glues = 0;
  board = 0;
}

tam_def::~tam_def()
{
  // anything need to be deleted?
}

model_var* tam_def::MakeModelVar(const symbol* wrap, shared_object* bnds)
{
  DCASSERT(wrap);
  DCASSERT(0==bnds);

  if (tam_debug.startReport()) {
    tam_debug.report() << "adding ";
    tam_debug.report() << wrap->Type()->getName();
    tam_debug.report() << " " << wrap->Name() << "\n";
    tam_debug.stopIO();
  }

  if (wrap->Type() == tile_type) {
    model_var* v = new tam_tile(wrap, current);
    num_tiles++;
    v->LinkTo(tiles);
    tiles = v;
    return v;
  }

  if (wrap->Type() == glue_type) {
    model_var* v = new tam_glue(wrap, current);
    num_glues++;
    v->LinkTo(glues);
    glues = v;
    return v;
  }

  return 0;
}

void tam_def::setStrength(const expr* cause, tam_glue* g, const result &s)
{
  DCASSERT(g);
  if (!isVariableOurs(g, cause, "ignoring strength assignment")) return;

  if (!s.isNormal() || s.getInt()>2 || s.getInt()<1) {
    if (StartError(cause)) {
      em->cerr() << "Bad strength ";
      DCASSERT(em->INT);
      em->INT->print(em->cerr(), s);
      em->cerr() << " for glue " << g->Name() << ", ignoring";
      DoneError();
    }
    return;
  }

  if (g->hasStrengthDefined()) {
    if (StartWarning(dup_gluedef, cause)) {
      em->warn() << "Duplicate strength assignment for glue ";
      em->warn() << g->Name() << ", ignoring";
      DoneWarning();
    }
    return;
  }

  if (tam_debug.startReport()) {
    tam_debug.report() << "setting strength ";
    tam_debug.report() << g->Name() << " : " << s.getInt() << "\n";
    tam_debug.stopIO();
  }

  g->setStrength(s.getInt());
}

void tam_def::setBorder(const expr* cause, int b, tam_tile* t, tam_glue* g)
{
  const char* ignore = 0;
  switch (b) {
    case NORTH:   ignore = "ignoring north border assignment";  break;
    case EAST :   ignore = "ignoring east border assignment";   break;
    case SOUTH:   ignore = "ignoring south border assignment";  break;
    case WEST :   ignore = "ignoring west border assignment";   break;
    default:
        DCASSERT(0);
  }

  if (!isVariableOurs(t, cause, ignore)) return;
  if (!isVariableOurs(g, cause, ignore)) return;

  const tam_glue* current = 0;
  switch (b) {
    case NORTH:   current = t->getNorth();  break;
    case EAST :   current = t->getEast();   break;
    case SOUTH:   current = t->getSouth();  break;
    case WEST :   current = t->getWest();   break;
  }

  if (current) {
    if (StartWarning(dup_tiledef, cause)) {
      em->warn() << "Tile " << t->Name() << " already has ";
      em->warn() << nameOf(b) << " glue " << g->Name();
      em->warn() << ", ignoring duplicate assignment";
      DoneWarning();
    }
    return;
  }

  if (tam_debug.startReport()) {
    tam_debug.report() << "setting tile " << t->Name() << " ";
    tam_debug.report() << nameOf(b) << " glue to " << g->Name() << "\n";
    tam_debug.stopIO();
  }

  switch (b) {
    case NORTH:   t->setNorth(g);   return;
    case EAST :   t->setEast(g);    return;
    case SOUTH:   t->setSouth(g);   return;
    case WEST :   t->setWest(g);    return;
  }
}

void tam_def::setBoardSize(const expr* cause, long xl, long xh, 
                            long yl, long yh)
{
  if (board_is_set) {
    if (StartWarning(dup_board, cause)) {
      em->warn() << "Duplicate board specification, ignoring";
      DoneWarning();
    }
    return;
  }
  if ((xl > xh) || (yl > yh)) {
    if (StartWarning(empty_board, cause)) {
      em->warn() << "Empty board specification [";
      em->warn() << xl << ".." << xh << "]x[";
      em->warn() << yl << ".." << yh;
      em->warn() <<"], ignoring";
      DoneWarning();
    }
    return;
  }

  x_low = xl;
  x_high = xh;
  y_low = yl;
  y_high = yh;
  board_is_set = true;

  if (tam_debug.startReport()) {
    tam_debug.report() << "setting board dimension [";
    tam_debug.report() << x_low << ".." << x_high << "]x[";
    tam_debug.report() << y_low << ".." << y_high << "]\n";
    tam_debug.stopIO();
  }

  // Allocate board
  long nsq = (x_high+1-x_low)*(y_high+1-y_low);
  board = new tam_square*[nsq];
  for (long i=0; i<nsq; i++) board[i] = 0;

  if (cause) {
    b_fn = cause->Filename();
    b_ln = cause->Linenumber();
  } else {
    b_fn = 0;
    b_ln = -1;
  }

}

void tam_def::setInit(const expr* cause, long x, long y, tam_tile* t)
{
  DCASSERT(t);
  if (!isVariableOurs(t, cause, "ignoring initialization")) return;

  if (!board_is_set) {
    if (StartError(cause)) {
      em->cerr() << "Board not specified before initialization, ignoring";
      DoneError();
    }
    return;
  }

  if (x < x_low || x > x_high || y < y_low || y > y_high) {
    if (StartError(cause)) {
      em->cerr() << "Square (" << x << ", " << y << ") not in [";
      em->cerr() << x_low << ".." << x_high << "]x[";
      em->cerr() << y_low << ".." << y_high << "], ignoring initialization";
      DoneError();
    }
    return;
  }

  tam_square* sq = getSq(x, y);
  if (0==sq) sq = makeSq(x, y);

  if (sq->hasInit()) {
    if (StartWarning(dup_init, cause)) {
      em->warn() << "Duplicate initialization of square (";
      em->warn() << x << ", " << y << "), ignoring";
      DoneWarning();
    }
    return;
  }

  sq->setInit(t);

  if (tam_debug.startReport()) {
    tam_debug.report() << "setting initial configuration of ";
    tam_debug.report() << sq->Name() << " to " << t->Name() << "\n";
    tam_debug.stopIO();
  }

}

void tam_def::setPriority(const expr* cause, long x, long y, long p)
{
  if (!board_is_set) {
    if (StartError(cause)) {
      em->cerr() << "Board not specified before priority assignment, ignoring";
      DoneError();
    }
    return;
  }

  if (x < x_low || x > x_high || y < y_low || y > y_high) {
    if (StartError(cause)) {
      em->cerr() << "Square (" << x << ", " << y << ") not in [";
      em->cerr() << x_low << ".." << x_high << "]x[";
      em->cerr() << y_low << ".." << y_high << "], ignoring initialization";
      DoneError();
    }
    return;
  }

  tam_square* sq = getSq(x, y);
  if (0==sq) sq = makeSq(x, y);

  if (sq->hasPrio()) {
    if (StartWarning(dup_prio, cause)) {
      em->warn() << "Duplicate priority assignment for square (";
      em->warn() << x << ", " << y << "), ignoring";
      DoneWarning();
    }
    return;
  }

  sq->setPrio(p);

  if (tam_debug.startReport()) {
    tam_debug.report() << "setting priority of ";
    tam_debug.report() << sq->Name() << " to " << p << "\n";
    tam_debug.stopIO();
  }
}

void tam_def::InitModel()
{
  tiles = glues = 0;
  num_tiles = num_glues = 0;
  board_is_set = false;
  board = 0;
  b_fn = 0;
  b_ln = -1;
}

void tam_def::FinalizeModel(OutputStream &ds)
{
  // 
  // Compact glue, tile lists
  //
  DCASSERT(num_tiles >= 0);
  if (0==num_tiles) {
    if (StartWarning(empty_tileset, 0)) {
      em->warn() << "No tiles defined";
      DoneWarning();
    }
  }
  tam_glue** glueset = list2array(smart_cast<tam_glue*>(glues), num_glues);
  glues = 0;
  tam_tile** tileset = list2array(smart_cast<tam_tile*>(tiles), num_tiles);
  tiles = 0;

  //
  // build state variables
  //
  if (!board_is_set) {
    if (StartWarning(empty_board, 0)) {
      em->warn() << "No board specification; using default 1x1 board";
      DoneWarning();
    }
    setBoardSize(0, 0, 0, 0, 0);
  }
  DCASSERT(board_is_set);
  DCASSERT(board);
  for (long x = x_low; x<=x_high; x++)
    for (long y = y_low; y<=y_high; y++) {
      if (readSq(x, y)) continue;
      makeSq(x, y);
    }
  long nsq = (x_high+1-x_low)*(y_high+1-y_low);
  for (long i=0; i<nsq; i++) {
    DCASSERT(board[i]);
    board[i]->SetIndex(i);
  }

  //
  // build events
  //
  int bufsize = 0;
  for (long t = 1; t<=num_tiles; t++) {
    DCASSERT(tileset[t]);
    bufsize = MAX(bufsize, int(strlen(tileset[t]->Name())));
  }
  bufsize += 80; // overkill, for sure, but space for "put(tilename, int, int)"
  char* buffer = new char[bufsize];

  long nev = nsq * num_tiles;
  model_event** eventlist = new model_event*[nev];
  long eindx = 0;
  for (long x = x_low; x<=x_high; x++) {
    for (long y = y_low; y<=y_high; y++) {
      // determine neighbors
      tam_square* curr = getSq(x, y);
      tam_square* q = getSq(x, y);
      tam_square* n = (y<y_high) ? getSq(x, y+1) : 0;
      tam_square* e = (x<x_high) ? getSq(x+1, y) : 0;
      tam_square* s = (y>y_low)  ? getSq(x, y-1) : 0;
      tam_square* w = (x>x_low)  ? getSq(x-1, y) : 0;

      long prio = curr->getPrio();

      for (long t = 1; t<=num_tiles; t++) {
        // build event
        snprintf(buffer, bufsize, "put(%s,%ld,%ld)", tileset[t]->Name(), x, y); 
        char* en = strdup(buffer);
        CHECK_RANGE(0, eindx, nev);
        eventlist[eindx] = new model_event(b_fn, b_ln, 0, en, current);

        eventlist[eindx]->setPriorityLevel(prio);
        
        // build enabling
        expr** pass = new expr*[6];
        pass[0] = Share(tileset[t]);
        pass[1] = Share(q);
        pass[2] = Share(n);
        pass[3] = Share(e);
        pass[4] = Share(s);
        pass[5] = Share(w);
        eventlist[eindx]->setEnabling(
          em->makeFunctionCall(b_fn, b_ln, &the_tam_canput, pass, 6)
        );

        // build next-state
        eventlist[eindx]->setNextstate(
          MakeVarAssign(em, Share(curr), t)
        );

        // firing style
        eventlist[eindx]->setNondeterministic();

        eindx++;
      } // for t
    } // for y
  } // for x
  delete[] buffer;

  //
  // build tam model
  //
  tam_hlm* build = new tam_hlm(
    current, (model_statevar**)board, nsq, eventlist, nev
  );
  build->set_x_width(x_high - x_low + 1);
  build->setGlues(no_gluedef, glueset, num_glues+1);
  build->setTiles(no_tiledef, tileset, num_tiles+1);

  // Cleanup
  board = 0;

  // Done
  ConstructionSuccess(build);
}

// **************************************************************************
// *                                                                        *
// *                          tam_formalism  class                          *
// *                                                                        *
// **************************************************************************

class tam_formalism : public formalism {
public:
  tam_formalism(const char* n, const char* sd, const char* ld);

  virtual model_def* makeNewModel(const char* fn, int ln, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
  virtual bool includeCTL() const { return true; }
};

// ******************************************************************
// *                     tam_formalism  methods                     *
// ******************************************************************

tam_formalism
::tam_formalism(const char* n, const char* sd, const char* ld)
 : formalism(n, sd, ld)
{
}

model_def* tam_formalism::makeNewModel(const char* fn, int ln, char* name, 
          symbol** formals, int np) const
{
  return new tam_def(fn, ln, this, name, (formal_param**) formals, np);
}

bool tam_formalism::canDeclareType(const type* vartype) const
{
  if (0==vartype)                 return 0;
  if (vartype->matches("glue"))   return 1;
  if (vartype->matches("tile"))   return 1;
  return 0;
}

bool tam_formalism::canAssignType(const type* vartype) const
{
  return 0;
}


// **************************************************************************
// *                                                                        *
// *                             TAM  Functions                             *
// *                                                                        *
// **************************************************************************

// ********************************************************
// *                  tam_strength class                  *
// ********************************************************

class tam_strength : public model_internal {
public:
  tam_strength();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

tam_strength::tam_strength() : model_internal(em->VOID, "strength", 2)
{
  typelist* t = new typelist(2);
  const type* glue = em->findType("glue");
  t->SetItem(0, glue->getSetOfThis());
  t->SetItem(1, em->INT);
  SetFormal(1, t, "gset:s");
  SetRepeat(1);
  SetDocumentation("For each glue type g in the set gset, assign bond strength s to g.");
}

void tam_strength::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  tam_def* mdl = smart_cast<tam_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    result first;
    x.answer = &first;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    DCASSERT(first.isNormal());
    shared_set* gset = smart_cast <shared_set*> (first.getPtr());
    DCASSERT(gset);

    result second;
    x.answer = &second;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    
    for (int z=0; z<gset->Size(); z++) {
      result gl;
      gset->GetElement(z, gl);
      tam_glue* g = smart_cast <tam_glue*> (gl.getPtr());
      DCASSERT(g);
      mdl->setStrength(pass[i], g, second);
    }
  }
  x.aggregate = 0;
  x.answer = answer;
}

// ********************************************************
// *                  tam_tiledef  class                  *
// ********************************************************

class tam_tiledef : public model_internal {
public:
  tam_tiledef();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

tam_tiledef::tam_tiledef() : model_internal(em->VOID, "tiledef", 3)
{
  const type* tile = em->findType("tile");
  const type* border = em->findType("border");
  const type* glue = em->findType("glue");
  DCASSERT(tile);
  DCASSERT(border);
  DCASSERT(glue);

  SetFormal(1, tile, "t");
  typelist* bg = new typelist(2);
  bg->SetItem(0, border);
  bg->SetItem(1, glue);
  SetFormal(2, bg, "b:g");
  SetRepeat(2);

  SetDocumentation("Sets the border glue types for tile t.  Can be called several times, but only the first specified glue type for each tile border will hold (and a warning will be issued in case of duplicates).");
}

void tam_tiledef::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  tam_def* mdl = smart_cast<tam_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;

  DCASSERT(np>1);
  DCASSERT(pass[1]);
  result foo;
  x.answer = &foo;
  SafeCompute(pass[1], x);
  DCASSERT(foo.isNormal());
  tam_tile* t = Share(smart_cast <tam_tile*> (foo.getPtr()));
  DCASSERT(t);

  for (int i=2; i<np; i++) {
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    DCASSERT(foo.isNormal());
    long b = foo.getInt();
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    DCASSERT(foo.isNormal());
    tam_glue* g = smart_cast <tam_glue*> (foo.getPtr());

    mdl->setBorder(pass[i], b, t, g);
  } // for i
  Delete(t);
  x.aggregate = 0;
  x.answer = answer;
}


// ********************************************************
// *                   tam_board  class                   *
// ********************************************************

class tam_board : public model_internal {
  result tmp;
public:
  tam_board();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  inline bool grabInt(traverse_data &x, model_def* m, 
                      const char* who, expr* p, long &L) 
  {
    result* answer = x.answer;
    x.answer = &tmp;
    SafeCompute(p, x);
    x.answer = answer;
    if (!tmp.isNormal()) {
      if (m->StartError(p)) {
        em->cerr() << "Bad value ";
        em->INT->print(em->cerr(), tmp);
        em->cerr() << " for " << who << ", ignoring board specification";
        m->DoneError();
      }
      return false;
    }
    L = tmp.getInt();
    return true;
  }
};

tam_board::tam_board() : model_internal(em->VOID, "board", 5)
{
  SetFormal(1, em->INT, "x_low");
  SetFormal(2, em->INT, "x_high");
  SetFormal(3, em->INT, "col_low");
  SetFormal(4, em->INT, "col_high");
  SetDocumentation("Specify a board for placing tiles.  The board allows x values between x_low and x_high (inclusive) and allows y values between y_low and y_high (inclusive).");
}

void tam_board::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  DCASSERT(5==np);
  tam_def* mdl = smart_cast<tam_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;

  long x_low, x_high, y_low, y_high;
  if (!grabInt(x, mdl, "x_low", pass[1], x_low)) return;
  if (!grabInt(x, mdl, "x_high", pass[2], x_high)) return;
  if (!grabInt(x, mdl, "y_low", pass[3], y_low)) return;
  if (!grabInt(x, mdl, "y_high", pass[4], y_high)) return;

  mdl->setBoardSize(pass[0], x_low, x_high, y_low, y_high);
}

// ********************************************************
// *                    tam_init class                    *
// ********************************************************

class tam_init : public model_internal {
public:
  tam_init();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  inline bool okInt(const result &I, long &L,  model_def* m, expr* p, const char* who) {
    if (I.isNormal()) {
      L = I.getInt();
      return true;
    }

    if (m->StartError(p)) {
      em->cerr() << "Bad " << who << " value ";
      em->INT->print(em->cerr(), I);
      em->cerr() << ", ignoring initialization";
      m->DoneError();
    }
    return false;
  }
};

tam_init::tam_init() : model_internal(em->VOID, "init", 2)
{
  typelist* t = new typelist(3);
  const type* tile = em->findType("tile");
  DCASSERT(tile);
  t->SetItem(0, em->INT);
  t->SetItem(1, em->INT);
  t->SetItem(2, tile);
  SetFormal(1, t, "x:y:t");
  SetRepeat(1);
  SetDocumentation("Set the initial configuration to contain tile t at position x,y.");
}

void tam_init::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  tam_def* mdl = smart_cast<tam_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;

  result* answer = x.answer;
  result tmp;
  x.answer = &tmp;

  for (int i=1; i<np; i++) {
    long x_pos, y_pos;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    if (!okInt(tmp, x_pos, mdl, pass[i], "x")) continue;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    if (!okInt(tmp, y_pos, mdl, pass[i], "y")) continue;
    x.aggregate = 2;
    SafeCompute(pass[i], x);
    if (!tmp.isNormal()) continue;
    DCASSERT(tmp.isNormal());
    tam_tile* t = smart_cast <tam_tile*> (tmp.getPtr());
    
    mdl->setInit(pass[i], x_pos, y_pos, t);
  }
  x.aggregate = 0;
  x.answer = answer;
}

// ********************************************************
// *                    tam_prio class                    *
// ********************************************************

class tam_prio : public model_internal {
public:
  tam_prio();
  virtual void Compute(traverse_data &x, expr** pass, int np);
protected:
  inline bool okInt(const result &I, long &L,  model_def* m, expr* p, const char* who) {
    if (I.isNormal()) {
      L = I.getInt();
      return true;
    }

    if (m->StartError(p)) {
      em->cerr() << "Bad " << who << " value ";
      em->INT->print(em->cerr(), I);
      em->cerr() << ", ignoring priority assignment";
      m->DoneError();
    }
    return false;
  }
};

tam_prio::tam_prio() : model_internal(em->VOID, "priority", 2)
{
  typelist* t = new typelist(3);
  t->SetItem(0, em->INT);
  t->SetItem(1, em->INT);
  t->SetItem(2, em->INT);
  SetFormal(1, t, "x:y:p");
  SetRepeat(1);
  SetDocumentation("Set the priority of placing a tile at position x,y to p.  If unspecified, the default priority is 0.");
}

void tam_prio::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  tam_def* mdl = smart_cast<tam_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;

  result* answer = x.answer;
  result tmp;
  x.answer = &tmp;

  for (int i=1; i<np; i++) {
    long x_pos, y_pos, prio;
    x.aggregate = 0;
    SafeCompute(pass[i], x);
    if (!okInt(tmp, x_pos, mdl, pass[i], "x")) continue;
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    if (!okInt(tmp, y_pos, mdl, pass[i], "y")) continue;
    x.aggregate = 2;
    SafeCompute(pass[i], x);
    if (!okInt(tmp, prio, mdl, pass[i], "p")) continue;
    
    mdl->setPriority(pass[i], x_pos, y_pos, prio);
  }
  x.aggregate = 0;
  x.answer = answer;
}

// ********************************************************
// *                   tam_export class                   *
// ********************************************************

class tam_export: public model_internal {
public:
  tam_export();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

tam_export::tam_export() : model_internal(em->BOOL, "export", 1)
{
  SetDocumentation("Export the model.  Writes the specification to the output stream in a format readable by another tool.  Returns true on success, false otherwise.");
}

void tam_export::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  DCASSERT(1==np);
  model_instance* mi = grabModelInstance(x, pass[0]);
  tam_hlm* m = mi ? smart_cast <tam_hlm*> (mi->GetCompiledModel()) : 0;
  if (m) {
    x.answer->setBool(m->Export(em->cout()));
  } else {
    x.answer->setBool(false);
  }
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_tamform : public initializer {
  public:
    init_tamform();
    virtual bool execute();
};
init_tamform the_tamform_initializer;

init_tamform::init_tamform() : initializer("init_tamform")
{
  usesResource("em");
  usesResource("CML");
  buildsResource("formalisms");
}

bool init_tamform::execute()
{
  if (0==em) return false;

  // types for TAMs
  simple_type* t_tile = new void_type("tile", "Tile", "Tile type in a tile assembly model.");
  t_tile->setPrintable();
  type* t_set_tile = newSetType("{tile}", t_tile);
  em->registerType(t_tile);
  em->registerType(t_set_tile);

  simple_type* t_glue = new void_type("glue", "Glue", "Glue type in a tile assembly model.");
  t_glue->setPrintable();
  type* t_set_glue = newSetType("{glue}", t_glue);
  em->registerType(t_glue);
  em->registerType(t_set_glue);

  simple_type* t_border = new void_type("border", "Border", "Border type in a tile assembly model.");
  t_border->setPrintable();
  em->registerType(t_border);

  // another formalism may have already registered these types.
  // all we care is that they are registered.
  tam_def::tile_type = em->findType("tile");
  DCASSERT(tam_def::tile_type);
  DCASSERT(em->findType("{tile}"));
  tam_def::glue_type = em->findType("glue");
  DCASSERT(tam_def::glue_type);
  DCASSERT(em->findType("{glue}"));
  tam_def::border_type = em->findType("border");
  DCASSERT(tam_def::border_type);


  // Set up and register formalism
  const char* longdocs = "The tile assembly model formalism allows definition of tile types that are automatically assembled onto a finite board.  Model definition requires declaration and definition of the tile types and specification of the board size and initial configuration, via the appropriate function calls.";

  formalism* tam = new tam_formalism("tam", "Tile assembly Model", longdocs);
  if (!em->registerType(tam)) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Couldn't register tam type";
      em->stopIO();
    }
    return false;
  }

  // fill symbol table
  symbol_table* tamsyms = MakeSymbolTable();
  tamsyms->AddSymbol(  new tam_strength );
  tamsyms->AddSymbol(  new tam_tiledef  );
  tamsyms->AddSymbol(  new tam_board    );
  tamsyms->AddSymbol(  new tam_init     );
  tamsyms->AddSymbol(  new tam_prio     );
  tamsyms->AddSymbol(  new tam_export   );
  tam->setFunctions(tamsyms); 
  tam->addCommonFuncs(CML);

  // fill identifier table
  symbol_table* tamids = MakeSymbolTable();
  tamids->AddSymbol(  
    new tam_border(tam_def::border_type, strdup("north"), tam_def::NORTH)
  );
  tamids->AddSymbol(  
    new tam_border(tam_def::border_type, strdup("south"), tam_def::SOUTH)
  );
  tamids->AddSymbol(  
    new tam_border(tam_def::border_type, strdup("east"), tam_def::EAST)
  );
  tamids->AddSymbol(  
    new tam_border(tam_def::border_type, strdup("west"), tam_def::WEST)
  );
  tam->setIdentifiers(tamids);

  // warnings, reports, and such

  option* debug = em->findOption("Debug");
  tam_def::tam_debug.Initialize(debug,
    "tams",
    "When set, diagnostic messages are displayed regarding tile assembly model construction.",
    false
  );

  option* warning = em->findOption("Warning");
  group_of_named tamwarnings(11);

  tamwarnings.AddItem(tam_def::dup_tiledef.Initialize(warning,
    "tam_dup_tiledef",
    "For multiple glue type assignments to a given tile, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::dup_gluedef.Initialize(warning,
    "tam_dup_gluedef",
    "For multiple glue strength assignments, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::dup_init.Initialize(warning,
    "tam_dup_init",
    "For multiple initializations of the same square, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::dup_prio.Initialize(warning,
    "tam_dup_prio",
    "For multiple priority assignments for the same square, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::dup_board.Initialize(warning,
    "tam_dup_board",
    "For multiple board specifications, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::no_tiledef.Initialize(warning,
    "tam_no_tiledef",
    "For tiles with no glue type assignments, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::no_gluedef.Initialize(warning,
    "tam_no_gluedef",
    "For glues with no strength assignments, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::no_init.Initialize(warning,
    "tam_no_init",
    "For missing board initializations, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::no_board.Initialize(warning,
    "tam_no_board",
    "For missing board specifications, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::empty_board.Initialize(warning,
    "tam_empty_board",
    "For board specifications with no squares, in tile assembly models",
    true
  ));
  tamwarnings.AddItem(tam_def::empty_tileset.Initialize(warning,
    "tam_empty_tileset",
    "For tile assembly models with no tiles defined",
    true
  ));
  tamwarnings.Finish(warning, "tam_ALL", "Group of all tile assembly model warnings");

  return true;
}
