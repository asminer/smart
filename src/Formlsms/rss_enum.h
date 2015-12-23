
// $Id$

#ifndef RSS_ENUM_H
#define RSS_ENUM_H

#include "rss.h"

// External libs
#include "lslib.h"

class model_enum;

class enum_reachset : public reachset {
  public:
    enum_reachset(model_enum* ss);
    virtual ~enum_reachset();

    virtual void getNumStates(long &ns) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(int display_order);

    virtual void getReachable(result &ss) const;
    virtual void getPotential(expr* p, result &ss) const;
    virtual void getInitialStates(result &x) const;


    void setInitial(LS_Vector &init);

  private:
    
    /**
      Iterator for discovery, natural orders
    */
    class natural_iter : public reachset::iterator {

      // required interface
      public:
        natural_iter(model_enum* ss);
        virtual ~natural_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        model_enum* states;
        long i;
    };

    /**
      Iterator for lexical order
    */
    class lexical_iter : public reachset::iterator {

      // required interface
      public:
        lexical_iter(model_enum* ss);
        virtual ~lexical_iter();
        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

      private:
        model_enum* states;
        long* map;
        long i;
    };

  private:
    model_enum* states;
    long* state_handle;    
    iterator* natorder;
    iterator* lexorder;

    LS_Vector initial;
};

#endif
