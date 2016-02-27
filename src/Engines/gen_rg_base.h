
// $Id$

#ifndef GEN_RG_BASE_H
#define GEN_RG_BASE_H

#include "../ExprLib/engine.h"

class timer;
class lldsm;
class hldsm;

// **************************************************************************
// *                                                                        *
// *                        process_generator  class                        *
// *                                                                        *
// **************************************************************************

/// underlying process generation engine base class.
class process_generator : public subengine {
protected:
  static named_msg report;
  static named_msg debug;
  static int remove_vanishing;
  friend class init_procgen;
public:
  static const int BY_PATH = 0;
  static const int BY_SUBGRAPH = 1;
public:
  process_generator();
  virtual ~process_generator();

  // returns true if the report stream is open
  static bool startGen(const hldsm& mdl, const char* whatproc);
  // returns true if the report stream is open
  static bool stopGen(bool err, const char* name, const char* whatproc, const timer &w);
  // returns true if the report stream is open
  static bool startCompact(const hldsm& mdl, const char* whatproc);
  // returns true if the report stream is open
  static bool stopCompact(const char* name, const char* wp, const timer &w, const lldsm* proc);
};

#endif
