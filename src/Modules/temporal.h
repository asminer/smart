
#ifndef TEMPORAL_H
#define TEMPORAL_H

#include "../ExprLib/type.h"

// ******************************************************************
// *                                                                *
// *                      temporal_type  class                      *
// *                                                                *
// ******************************************************************

class temporal_type : public simple_type {
  public:
    temporal_type(bool pathform, const char* name, const char* short_doc, const char* long_doc);

    inline bool isPathFormula() const {
      return is_path_formula;
    }

    inline bool isStateFormula() const {
      return !is_path_formula;
    }

    /**
        Specify what happens to this type if we
        add a path quantifier, or path operator, out front.
    */
    inline void setQPtypes(const temporal_type* qt, const temporal_type* pt) {
      qtype = qt;
      ptype = pt;
      DCASSERT( (0==is_path_formula) == (0==qtype) );
    }

    inline const temporal_type* quantify() const {
      return qtype;
    }
    inline const temporal_type* pathify() const {
      return ptype;
    }

    /**
        Specify what happens to this type if 
        subjected to logic operators (default: stay the same)
    */
    inline void setLogic(const temporal_type* lt) {
      ltype = lt;
    }

    inline const temporal_type* logicify() const {
      return ltype;
    }

    virtual const simple_type* getBaseType() const;

  private:
    const temporal_type* qtype;
    const temporal_type* ptype;
    const temporal_type* ltype;
    bool is_path_formula;
};


namespace temporal_types {
  temporal_type* t_temporal;
  temporal_type* t_single_pathop;
  temporal_type* t_ctl_pathform;
  temporal_type* t_ctl_stateform;
  temporal_type* t_ltl_pathform;
  temporal_type* t_ltl_topform;
  temporal_type* t_ctlstar_pathform;
  temporal_type* t_ctlstar_stateform;
};

const simple_type* temporal_type::getBaseType() const
{
  return temporal_types::t_temporal;
}

#endif

